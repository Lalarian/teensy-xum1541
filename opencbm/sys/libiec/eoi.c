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
** \file sys/libiec/eoi.c \n
** \author Spiro Trikaliotis \n
** \version $Id: eoi.c,v 1.1 2004-11-07 11:05:14 strik Exp $ \n
** \authors Based on code from
**    Michael Klein <michael.klein@puffin.lb.shuttle.de>
** \n
** \brief Check if an EOI signal has been sent over the IEC bus
**
****************************************************************/

#include <wdm.h>
#include "cbm4win_common.h"
#include "i_iec.h"

/*! \brief Check if an EOI signal has been sent over the IEC bus

 \param Pdx
   Pointer to the device extension.
 
 \param Result 
   Pointer to the variable which will hold the return value.
   After return, it will contain 1 if there was an EOI, 0 otherwise.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/
NTSTATUS
cbmiec_get_eoi(IN PDEVICE_EXTENSION Pdx, OUT PBOOLEAN Result)
{
    FUNC_ENTER();

    // give back if we encountered an EOI

    *Result = Pdx->Eoi ? TRUE : FALSE;

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}
