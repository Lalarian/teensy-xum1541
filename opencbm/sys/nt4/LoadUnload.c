/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004 Spiro Trikaliotis <cbm4win@trikaliotis.net>
 *
 */

/*! ************************************************************** 
** \file sys/nt4/LoadUnload.c \n
** \author Spiro Trikaliotis \n
** \version $Id: LoadUnload.c,v 1.1 2004-11-07 11:05:14 strik Exp $ \n
** \n
** \brief Load and unload the driver
**
****************************************************************/

#include <ntddk.h>
#include "cbm4win_common.h"
#include <cbmioctl.h>

#undef ExFreePool

/*! \brief create functional device object (FDO) for enumerated device

 AddDevice is responsible for creating functional device objects (FDO) or
 filter device objects (filter DO) for devices enumerated by the Plug and Play
 (PnP) Manager.

 \param DriverObject
   Pointer to a DRIVER_OBJECT structure. This is the driver's driver object.
 
 \param PdoUNUSED
   Pointer to a DEVICE_OBJECT structure representing a physical device object 
   (PDO) created by a lower-level driver.
   \todo Parameter is currently unused

 \param ParallelPortName
   Pointer to the name of the parallel port to attach to
   \todo Will be removed for WDM

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.

 A driver's AddDevice routine executes in a system thread context 
 at IRQL PASSIVE_LEVEL.

 As long as we are an NT4 style device driver, AddDevice is not automatically
 called by the system as we don't have an INF file. Thus, it is called from
 our DriverEntry by ourself.
*/
NTSTATUS
AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PdoUNUSED, IN PCWSTR ParallelPortName)
{
    PDEVICE_OBJECT fdo;
    UNICODE_STRING deviceNameNumber;
    UNICODE_STRING deviceNamePrefix;
    UNICODE_STRING deviceName;
    NTSTATUS ntStatus;
    WCHAR deviceNameNumberBuffer[50];

    /*! \todo Remove this */
    static SHORT no = 0;

    FUNC_ENTER();

    UNREFERENCED_PARAMETER(PdoUNUSED);

    // Until now, no error has occurred

    ntStatus = STATUS_SUCCESS;

    // Create the name for the fdo
    // thus, this is a named driver

    DBG_IRQL( <= DISPATCH_LEVEL);
    RtlInitUnicodeString(&deviceNamePrefix, CBMDEVICENAME);

    // Convert variable no into a UNICODE_STRING

    deviceNameNumber.Length = 0;
    deviceNameNumber.MaximumLength = sizeof(deviceNameNumberBuffer);
    deviceNameNumber.Buffer = deviceNameNumberBuffer;
    DBG_IRQL( == PASSIVE_LEVEL);
    RtlIntegerToUnicodeString(no, 10, &deviceNameNumber);

    // Increment the number for the next device

    ++no;

    // Allocate enough space for the concatenation of deviceNamePrefix
    // and deviceNameNumber

    deviceName.MaximumLength = deviceNamePrefix.Length + deviceNameNumber.Length;
    deviceName.Buffer = (PWCHAR) ExAllocatePoolWithTag(NonPagedPool, 
        deviceName.MaximumLength, MTAG_DEVNAME);

    if (!deviceName.Buffer)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        DBG_ERROR((DBG_PREFIX "Could not allocate memory for deviceName"))
        LogErrorString(NULL, CBM_START_FAILED, L"allocating memory for device name", NULL);
    }

    if (NT_SUCCESS(ntStatus))
    {
        // Concatenate both strings

        DBG_IRQL( < DISPATCH_LEVEL);
        RtlCopyUnicodeString(&deviceName, &deviceNamePrefix);
        DBG_IRQL( < DISPATCH_LEVEL);
        RtlAppendUnicodeStringToString(&deviceName, &deviceNameNumber);

        // create the FDO with the name just build

        DBG_IRQL( == PASSIVE_LEVEL);
        ntStatus = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), &deviceName,
            FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, TRUE, &fdo);

        if (!NT_SUCCESS(ntStatus))
        {
            DBG_ERROR((DBG_PREFIX "IoCreateDevice failed with 0x%08X - %s",
                ntStatus, DebugNtStatus(ntStatus)));
            LogErrorString(NULL, CBM_START_FAILED, L"creation of the device object", NULL);
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        PDEVICE_EXTENSION pdx;

        // Initialization common to WDM and NT4 driver

        ntStatus = AddDeviceCommonInit(fdo, &deviceName, ParallelPortName);

        // mark that we are running the NT4 version of the driver

        pdx = fdo->DeviceExtension;
        pdx->IsNT4 = TRUE;

        if (!NT_SUCCESS(ntStatus))
        {
            // An error has occurred, thus, delete the device

            // If necessary, delete the buffer for the device name

            if (pdx->DeviceName.Buffer)
            {
                DBG_IRQL( < DISPATCH_LEVEL);
                ExFreePool(pdx->DeviceName.Buffer);
            }

            DBG_IRQL( == PASSIVE_LEVEL);
            IoDeleteDevice(fdo);
        }
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Unload routine of the driver

 DriverUnload performs any operations that are necessary before the system unloads the driver.

 \param 
   DriverObject Caller-supplied pointer to a DRIVER_OBJECT structure. 
   This is the driver's driver object. 

 A driver's Unload routine executes in a system thread context at IRQL PASSIVE_LEVEL.
 The driver's DriverEntry routine must store the Unload routine's address in 
 DriverObject->DriverUnload. (If no routine is supplied, this pointer must be NULL.)
*/
VOID
DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
    PDEVICE_OBJECT currentDevice;

    FUNC_ENTER();

    // Make sure every device object is deleted

    while (currentDevice = DriverObject->DeviceObject) 
    {
        PDEVICE_EXTENSION pdx = currentDevice->DeviceExtension;

        DBG_ASSERT(pdx);

        // Stop the thread of that device, if necessary

        cbm_stop_thread(pdx);

        // Uninitialize the parallel port, if necessary

        ParPortDeinit(pdx);

        // If a buffer for the device name has been allocated,
        // release that

        if (pdx->DeviceName.Buffer)
        {
            DBG_IRQL( < DISPATCH_LEVEL);
            ExFreePool(pdx->DeviceName.Buffer);
        }

        // Delete the device
        // This has to be done *after* the above, as the pdx
        // is not allowed to be accessed anymore here.

        DBG_IRQL( == PASSIVE_LEVEL);
        IoDeleteDevice(currentDevice);
    }

    // some more uninitialization, common to WDM and NT4 driver

    DriverCommonUninit();

    FUNC_LEAVE();
}

/*! \brief Start routine of the driver

 DriverEntry is the first routine called after a driver is loaded, and it is
 responsible for initializing the driver.

 \param DriverObject
   Caller-supplied pointer to a DRIVER_OBJECT structure. This is the
   driver's driver object.

 \param RegistryPath
   Pointer to a counted Unicode string specifying the path to the
   driver's registry key.

 \return
   If the routine succeeds, it must return STATUS_SUCCESS. Otherwise, it
   must return one of the error status values defined in ntstatus.h.

 The DriverObject parameter supplies the DriverEntry routine with a
 pointer to the driver's driver object, which is allocated by the I/O 
 Manager. The DriverEntry routine must fill in the driver object with
 entry points for the driver's standard routines.
*/
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    PENUMERATE enumerate;
    NTSTATUS ntStatus;

    FUNC_ENTER();

    // Output a status message

    DBG_PRINT((DBG_PREFIX "CBM4NT.SYS " __DATE__ " " __TIME__));

	// Do initialization common to NT4 and WDM driver

    ntStatus = DriverCommonInit(DriverObject, RegistryPath);

    // enumerate all parallel port drivers:

    ntStatus = ParPortEnumerateOpen(&enumerate);

    if (NT_SUCCESS(ntStatus))
    {
        PCWSTR DriverName;

        do 
        {
            ntStatus = ParPortEnumerate(enumerate,&DriverName);

            if (NT_SUCCESS(ntStatus) && *DriverName)
            {
                DBG_SUCCESS((DBG_PREFIX "Drivername = \"%ws\"", DriverName));
                AddDevice(DriverObject, NULL, DriverName);
            }
        } while (NT_SUCCESS(ntStatus) && *DriverName);

        ParPortEnumerateClose(enumerate);
    }

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}
