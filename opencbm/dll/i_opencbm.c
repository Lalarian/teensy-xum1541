/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael.klein@puffin.lb.shuttle.de>
 *  Copyright 2001-2004 Spiro Trikaliotis
 *
 *  Parts are Copyright
 *      Jouko Valta <jopi@stekt.oulu.fi>
 *      Andreas Boose <boose@linux.rz.fh-hannover.de>
*/

/*! ************************************************************** 
** \file dll/i_opencbm.c \n
** \author Spiro Trikaliotis \n
** \version $Id: i_opencbm.c,v 1.1 2004-11-07 11:05:11 strik Exp $ \n
** \authors Based on code from
**    Michael Klein <michael.klein@puffin.lb.shuttle.de>
** \n
** \brief Helper functions for the DLL for accessing the driver,
**        and the install functions
**
****************************************************************/

// #define UNICODE 1

#include <windows.h>
#include <windowsx.h>

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#ifndef DBG_PROGNAME
    #define DBG_PROGNAME "OPENCBM.DLL"
#endif // #ifndef DBG_PROGNAME

#include "debug.h"

#include <winioctl.h>
#include "cbmioctl.h"

#include <stdlib.h>
#include <stddef.h>

#include "i_opencbm.h"

#include "version.h"


/*-------------------------------------------------------------------*/
/*--------- REGISTRY FUNCTIONS --------------------------------------*/

/*! \internal \brief Get a DWORD value from the registry

 This function gets a DWORD value in the registry. It is a simple
 wrapper for convenience.

 \param RegKey

   A handle to an already opened registry key.

 \param SubKey

   Pointer to a null-terminiated string which holds the name
   of the value to be created or changed.

 \param Value

   Pointer to a variable which will contain the value from the registry

 \return

   ERROR_SUCCESS on success, -1 otherwise

 If this function returns -1, the given Value will not be changed at all!
*/

static LONG
RegGetDWORD(IN HKEY RegKey, IN char *SubKey, OUT LPDWORD Value)
{
    DWORD valueLen;
    DWORD lpType;
    DWORD value;
    DWORD rc;

    FUNC_ENTER();

    valueLen = sizeof(value);

    rc = RegQueryValueEx(RegKey, SubKey, NULL, &lpType, (LPBYTE)&value, &valueLen);

    DBG_ASSERT(valueLen == 4);

    if ((rc == ERROR_SUCCESS) && (valueLen == 4))
    {
        DBG_SUCCESS((DBG_PREFIX "RegGetDWORD"));
        *Value = value;
    }
    else
    {
        DBG_ERROR((DBG_PREFIX "RegGetDWORD failed, returning -1"));
        rc = -1;
    }

    FUNC_LEAVE_INT(rc);
}


/*! \internal \brief Get the number of the parallel port to open

 This function checks the registry for the number of the parallel port 
 to be opened as default.

 \return 

   Returns the number of the parallel port to be opened as default,
   starting with 0.

 If the registry entry does not exist, this function returns 0, which
 is also the default after installing the driver.
*/

static int
cbm_get_default_port(VOID)
{
    DWORD ret;
    HKEY RegKey;

    FUNC_ENTER();

    DBG_PPORT((DBG_PREFIX "cbm_get_default_port()"));

    // Open a registry key to HKLM\<%REGKEY_SERVICE%>

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     CBM_REGKEY_SERVICE,
                     0,
                     KEY_QUERY_VALUE,
                     &RegKey)
       )
    {
        DBG_WARN((DBG_PREFIX "RegOpenKeyEx() failed!"));
        FUNC_LEAVE_BOOL(FALSE);
    }

    // now, get the number of the port to use

    if (RegGetDWORD(RegKey, CBM_REGKEY_SERVICE_DEFAULTLPT, &ret) != ERROR_SUCCESS)
    {
        DBG_WARN((DBG_PREFIX "No " CBM_REGKEY_SERVICE "\\" CBM_REGKEY_SERVICE_DEFAULTLPT 
            " value, setting 0."));
        ret = 0;
    }

    // We're done, close the registry handle.

    RegCloseKey(RegKey);

    DBG_PPORT((DBG_PREFIX "RETURN: cbm_get_default_port() == %u", ret));

    FUNC_LEAVE_INT(ret);
}

#if DBG

/*! \brief Set the debugging flags

 This function gets the debugging flags from the registry. If there
 are any, it sets the flags to that value.
*/

VOID
cbm_i_get_debugging_flags(VOID)
{
    DWORD ret;
    HKEY RegKey;

    FUNC_ENTER();

    // Open a registry key to HKLM\<%REGKEY_SERVICE%>

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     CBM_REGKEY_SERVICE,
                     0,
                     KEY_QUERY_VALUE,
                     &RegKey)
       )
    {
        DBG_WARN((DBG_PREFIX "RegOpenKeyEx() failed!"));
        FUNC_LEAVE();
    }

    // now, get the number of the port to use

    if (RegGetDWORD(RegKey, CBM_REGKEY_SERVICE_DLL_DEBUGFLAGS, &ret) != ERROR_SUCCESS)
    {
        DBG_WARN((DBG_PREFIX "No " CBM_REGKEY_SERVICE "\\" CBM_REGKEY_SERVICE_DLL_DEBUGFLAGS
            " value, leaving default."));
    }
    else
    {
        DbgFlags = ret;
    }

    // We're done, close the registry handle.

    RegCloseKey(RegKey);

    FUNC_LEAVE();
}

#endif // #if DBG

/*! \brief Get the name of the driver for a specific parallel port

 Get the name of the driver for a specific parallel port.

 \param PortNumber

   The port number for the driver to open. 0 means "default" driver, while
   values != 0 enumerate each driver.

 \return 
   Returns a pointer to a null-terminated string containing the
   driver name, or NULL if an error occurred.

 \bug
   PortNumber is not allowed to exceed 10. 
*/

const char *
cbm_i_get_driver_name(int PortNumber)
{
    //! \todo do not hard-code the driver name
    static char driverName[] = "\\\\.\\opencbm0";
    char *ret;

    FUNC_ENTER();

    ret = NULL;

    /*! \bug 
     * the logic does not allow more than 10 entries, 
     * thus, fail this call if we want to use a port > 10!  */

    if (PortNumber <= 10)
    {
        if (PortNumber == 0)
        {
            // PortNumber 0 has special meaning: Find out the default value

            PortNumber = cbm_get_default_port();
        }

        driverName[strlen(driverName)-1] = (PortNumber ? PortNumber-1 : 0) + '0';
        ret = driverName;
    }

    FUNC_LEAVE_STRING(ret);
}

/*! \brief Opens the driver

 This function Opens the driver.

 \param HandleDevice
  
   Pointer to a CBM_FILE which will contain the file handle of the driver.

 \param PortNumber

   The port number of the driver to open. 0 means "default" driver, while
   values != 0 enumerate each driver.

 \return 

   ==0: This function completed successfully
   !=0: otherwise

 PortNumber is not allowed to exceed 10. 

 cbm_driver_open() should be balanced with cbm_driver_close().
*/

int
cbm_i_driver_open(CBM_FILE *HandleDevice, int PortNumber)
{
    const char *driverName;

    FUNC_ENTER();

    // Get the name of the driver to be opened

    driverName = cbm_i_get_driver_name(PortNumber);

    if (driverName == NULL)
    {
        // there was a problem, thus, fail this call!

        *HandleDevice = INVALID_HANDLE_VALUE;
    }
    else 
    {
        // Open the device

        *HandleDevice = CreateFile(driverName,
            GENERIC_READ | GENERIC_WRITE, // we need read and write access
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    }

    DBG_ASSERT(*HandleDevice != INVALID_HANDLE_VALUE);

    FUNC_LEAVE_INT(*HandleDevice == INVALID_HANDLE_VALUE);
}

/*! \brief Closes the driver

 Closes the driver, which has be opened with cbm_driver_open() before.

 \param HandleDevice
  
   A CBM_FILE which contains the file handle of the driver.

 cbm_driver_close() should be called to balance a previous call to
 cbm_driver_open(). 
 
 If cbm_driver_open() did not succeed, it is illegal to 
 call cbm_driver_close().
*/

void
cbm_i_driver_close(CBM_FILE HandleDevice)
{
    FUNC_ENTER();

    DBG_ASSERT(HandleDevice != 0);

    CloseHandle(HandleDevice);

    FUNC_LEAVE();
}

/*! \brief Perform an ioctl on the driver

 This function performs an ioctl on the driver. 
 It is used internally only.

 \param HandleDevice
  
   A CBM_FILE which contains the file handle of the driver.

 \param ControlCode

   The ControlCode of the IOCTL to be performed.

 \param TextControlCode

   A string representation of the IOCTL to be performed. This is
   used for debugging purposes, only, and not available in
   free builds.

 \param InBuffer

   Pointer to a buffer which holds the input parameters for the
   IOCTL. Can be NULL if no input buffer is needed.

 \param InBufferSize

   Size of the buffer pointed to by InBuffer. If InBuffer is NULL,
   this has to be zero,

 \param OutBuffer

   Pointer to a buffer which holds the output parameters of the
   IOCTL. Can be NULL if no output buffer is needed.

 \param OutBufferSize

   Size of the buffer pointed to by OutBuffer. If OutBuffer is NULL,
   this has to be zero,

 \return

   TRUE: IOCTL succeeded, else
   FALSE  an error occurred processing the IOCTL

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

BOOL 
cbm_ioctl(IN CBM_FILE HandleDevice, IN DWORD ControlCode, 
#if DBG
          IN char *TextControlCode, 
#endif // #if DBG
          IN PVOID InBuffer, IN ULONG InBufferSize, OUT PVOID OutBuffer, IN ULONG OutBufferSize)
{
    DWORD dwBytesReturned;

    BOOL returnValue;

    FUNC_ENTER();

    // Perform the IOCTL

    returnValue = DeviceIoControl(HandleDevice, ControlCode, InBuffer, InBufferSize,
        OutBuffer, OutBufferSize, &dwBytesReturned, NULL);

    // If an error occurred, output it (in the DEBUG version of this file

    if (!returnValue)
    {
        DBG_ERROR((DBG_PREFIX "%s: Error code = %u", TextControlCode, GetLastError()));
    }
    else
    {
        // Check if the number of bytes returned equals the wanted number

        if (dwBytesReturned != OutBufferSize) 
        {
            DBG_WARN((DBG_PREFIX "%s: OutBufferSize = %u, but dwBytesReturned = %u",
                TextControlCode, OutBufferSize, dwBytesReturned));
        }
    }

    FUNC_LEAVE_BOOL(returnValue);
}

/*-------------------------------------------------------------------*/
/*--------- DEVICE DRIVER HANDLING FUNCTIONS  -----------------------*/


/*! \brief Start a device driver

 This function start a device driver. It is the programmatically
 equivalent for "net start <driver>"

 \return
   Returns TRUE on success, else FALSE.

 This function is for use of the installation routines only!
*/

BOOL
cbm_i_driver_start(VOID)
{
    SC_HANDLE schManager;
    SC_HANDLE schService;
    DWORD err;
    BOOL ret;

    FUNC_ENTER();

    ret = TRUE;

    schManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (schManager == NULL)
    {
       DBG_ERROR((DBG_PREFIX "Could not open service control manager!"));
       FUNC_LEAVE_BOOL(FALSE);
    }

    schService = OpenService(schManager, OPENCBM_DRIVERNAME, SERVICE_ALL_ACCESS);

    if (schService == NULL)
    {
        DBG_ERROR((DBG_PREFIX "OpenService (0x%02x)", GetLastError()));
        FUNC_LEAVE_BOOL(FALSE);
    }

    ret = StartService(schService, 0, NULL);

    if (ret)
    {
        DBG_SUCCESS((DBG_PREFIX "StartService"));
    }
    else
    {
        err = GetLastError();

        if (err == ERROR_SERVICE_ALREADY_RUNNING)
        {
            /* If the service is already running, don't treat it
             * as an error, but as a warning, and report success!
             */

            DBG_WARN((DBG_PREFIX "StartService, (ERROR_SERVICE_ALREADY_RUNNING)"));

            ret = TRUE;
        }
        else
        {
            DBG_ERROR((DBG_PREFIX "StartService (0x%02x)", err));
        }
    }

    CloseServiceHandle(schService);

    // Close the SCM: We don't need it anymore

    CloseServiceHandle(schManager);

    FUNC_LEAVE_BOOL(ret);
}


/*! \brief Stop a device driver

 This function stops a device driver. It is the programmatically
 equivalent for "net stop <driver>"

 \return
   Returns TRUE on success, else FALSE.

 This function is for use of the installation routines only!
*/

BOOL
cbm_i_driver_stop(VOID)
{
    SERVICE_STATUS  serviceStatus;
    SC_HANDLE schManager;
    SC_HANDLE schService;
    BOOL ret;

    FUNC_ENTER();

    schManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    schService = OpenService(schManager, OPENCBM_DRIVERNAME, SERVICE_ALL_ACCESS);

    if (schManager == NULL)
    {
       DBG_ERROR((DBG_PREFIX "Could not open service control manager!"));
       FUNC_LEAVE_BOOL(FALSE);
    }

    if (schService == NULL)
    {
        DBG_ERROR((DBG_PREFIX "OpenService (0x%02x)", GetLastError()));
        FUNC_LEAVE_BOOL(FALSE);
    }

    ret = ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus);

    if (ret)
    {
        DBG_SUCCESS((DBG_PREFIX "ControlService"));
    }
    else
    {
        DBG_ERROR((DBG_PREFIX "ControlService (0x%02x)", GetLastError()));
    }

    CloseServiceHandle(schService);

    // Close the SCM: We don't need it anymore

    CloseServiceHandle(schManager);

    FUNC_LEAVE_BOOL(ret);
}

/*! \brief Complete driver installation, "direct version"

 This function performs anything that is needed to successfully
 complete the driver installation.

 \param Buffer
   Pointer to a buffer which will return the install information

 \param BufferLen
   The length of the buffer Buffer points to (in bytes).

 \return
   FALSE on success, TRUE on error

 This function is for use of the installation routines only!

 This version is for usage in the DLL or the install package.
*/

BOOL
cbm_i_i_driver_install(OUT PULONG Buffer, IN ULONG BufferLen)
{
    PCBMT_I_INSTALL_OUT outBuffer;
    CBM_FILE HandleDevice;

    FUNC_ENTER();

    DBG_ASSERT(Buffer);
    DBG_ASSERT(BufferLen >= sizeof(ULONG));

    outBuffer = (PCBMT_I_INSTALL_OUT) Buffer;

    if (cbm_i_driver_open(&HandleDevice, 0) == 0)
    {
        outBuffer->ErrorFlags = CBM_I_DRIVER_INSTALL_0_IOCTL_FAILED;
        cbm_ioctl(HandleDevice, CBMCTRL(I_INSTALL), NULL, 0, Buffer, BufferLen);
        cbm_i_driver_close(HandleDevice);
    }
    else
    {
        outBuffer->ErrorFlags = CBM_I_DRIVER_INSTALL_0_FAILED;
        DBG_ERROR((DBG_PREFIX "Driver could not be openend"));
    }

    // if there is room in the given buffer, set the dll version

    if (BufferLen >= sizeof(outBuffer->DllVersion) + offsetof(CBMT_I_INSTALL_OUT, DllVersion))
    {
        outBuffer->DllVersion =
            CBMT_I_INSTALL_OUT_MAKE_VERSION(CBM4WIN_VERSION_MAJOR, CBM4WIN_VERSION_MINOR,
                                            CBM4WIN_VERSION_SUBMINOR, CBM4WIN_VERSION_DEVEL);
    }

    FUNC_LEAVE_INT(
        (  (outBuffer->ErrorFlags != CBM_I_DRIVER_INSTALL_0_IOCTL_FAILED)
        && (outBuffer->ErrorFlags != CBM_I_DRIVER_INSTALL_0_FAILED)
        ) ? FALSE : TRUE);
}
