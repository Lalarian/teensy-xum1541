/*
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999 Michael Klein <michael.klein@puffin.lb.shuttle.de>
*/

#ifdef SAVE_RCSID
static char *rcsid =
    "@(#) $Id: flash.c,v 1.1 2004-11-07 11:05:11 strik Exp $";
#endif

#include "opencbm.h"

#ifdef WIN32
    #include "unixcompat.h"
#else
    #include <unistd.h>
#endif
#include <stdlib.h>

static unsigned char flash[] = {
#include "flash.inc"
};

int __cdecl main(int argc, char *argv[])
{
    __u_char drv = argc > 1 ? atoi(argv[1]) : 8;
    CBM_FILE fd;
    
    if(cbm_driver_open(&fd, 0) == 0)
    {
        cbm_upload(fd, drv, 0x0500, flash, sizeof(flash));
        cbm_exec_command(fd, drv, "U3:", 0);
        cbm_driver_close(fd);
        return 0;
    }
    return 1;
}
