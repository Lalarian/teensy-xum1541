/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2004 Michael Klein <michael.klein@puffin.lb.shuttle.de>
 *  Copyright 2001-2004 Spiro Trikaliotis <cbm4win@trikaliotis.net>
 *
 */

/*! ************************************************************** 
** \file sys/libiec/openclose.c \n
** \author Spiro Trikaliotis \n
** \version $Id: openclose.c,v 1.1 2004-11-07 11:05:14 strik Exp $ \n
** \authors Based on code from
**    Michael Klein <michael.klein@puffin.lb.shuttle.de>
** \n
** \brief Write one byte to the IEC bus
**
****************************************************************/

#include <wdm.h>
#include "cbm4win_common.h"
#include "i_iec.h"

/*! \brief Open a file on the IEC bus

 This function opens a file on the IEC bus.

 \param Pdx
   Pointer to the device extension.

 \param Device
   Device (primary) address

 \param Secaddr
   Secondary address

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/
NTSTATUS
cbmiec_open(IN PDEVICE_EXTENSION Pdx, IN UCHAR Device, IN UCHAR Secaddr)
{
    NTSTATUS ntStatus;
    USHORT sent;
    UCHAR buffer[2];

    FUNC_ENTER();

    FUNC_PARAM((DBG_PREFIX "Device = 0x%02x, Secaddr = 0x%02x", (int)Device, (int)Secaddr));

    // send a 0x2x / 0xFy (open device x, secaddr y) under control of ATN

    buffer[0] = 0x20 | Device;
    buffer[1] = 0xf0 | Secaddr;
    ntStatus = cbmiec_i_raw_write(Pdx, buffer, 2, &sent, 1, 0);

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Close a file on the IEC bus

 This function opens a file on the IEC bus.

 \param Pdx
   Pointer to the device extension.

 \param Device
   Device (primary) address

 \param Secaddr
   Secondary address

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/
NTSTATUS
cbmiec_close(IN PDEVICE_EXTENSION Pdx, IN UCHAR Device, IN UCHAR Secaddr)
{
    NTSTATUS ntStatus;
    USHORT sent;
    UCHAR buffer[2];

    FUNC_ENTER();

    FUNC_PARAM((DBG_PREFIX "Device = 0x%02x, Secaddr = 0x%02x", (int)Device, (int)Secaddr));

    // send a 0x2x / 0xFy (close device x, secaddr y) under control of ATN

    buffer[0] = 0x20 | Device;
    buffer[1] = 0xe0 | Secaddr;
    ntStatus = cbmiec_i_raw_write(Pdx, buffer, 2, &sent, 1, 0);

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
