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
** \file sys/libiec/poll.c \n
** \author Spiro Trikaliotis \n
** \version $Id: poll.c,v 1.1 2004-11-07 11:05:14 strik Exp $ \n
** \authors Based on code from
**    Michael Klein <michael.klein@puffin.lb.shuttle.de>
** \n
** \brief Write one byte to the IEC bus
**
****************************************************************/

#include <wdm.h>
#include "cbm4win_common.h"
#include "i_iec.h"

/*! \brief Polls the status of the lines on the IEC bus

 \param Pdx
   Pointer to the device extension.

 \param Result
   Pointer to a variable which will hold the value of the IEC bus

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/
NTSTATUS
cbmiec_iec_poll(IN PDEVICE_EXTENSION Pdx, OUT PUCHAR Result)
{
    UCHAR ch;

    FUNC_ENTER();

    ch = READ_PORT_UCHAR(IN_PORT);

    *Result = 0;

    if((ch & PP_DATA_IN) == 0) *Result |= IEC_LINE_DATA;
    if((ch & PP_CLK_IN)  == 0) *Result |= IEC_LINE_CLOCK;
    if((ch & PP_ATN_IN)  == 0) *Result |= IEC_LINE_ATN;

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}
