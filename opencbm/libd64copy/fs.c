/*
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2003 Michael Klein <michael.klein@puffin.lb.shuttle.de>
*/

#ifdef SAVE_RCSID
static char *rcsid =
    "@(#) $Id: fs.c,v 1.2 2004-11-15 16:11:52 strik Exp $";
#endif

#include "d64copy_int.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef WIN32
    #include <unistd.h>
#endif // #ifndef WIN32

static d64copy_settings *fs_settings;

static FILE *the_file;
static char *error_map;
static int block_count;

static int block_offset(int tr, int se)
{
    int sectors = 0, i;
    for(i = 1; i < tr; i++)
    {
        sectors += d64copy_sector_count(fs_settings->two_sided, i);
    }
    return (sectors + se) * BLOCKSIZE;
}

static int read_block(__u_char tr, __u_char se, char *block)
{
    if(fseek(the_file, block_offset(tr, se), SEEK_SET) == 0)
    {
        return fread(block, BLOCKSIZE, 1, the_file) != 1;
    }
    return 1;
}

static int write_block(__u_char tr, __u_char se, const char *blk, int size, int read_status)
{
    long ofs;

    ofs = block_offset(tr, se);
    if(fseek(the_file, ofs, SEEK_SET) == 0)
    {
        error_map[ofs / BLOCKSIZE] = (char) read_status;
        return fwrite(blk, size, 1, the_file) != 1;
    }
    return 1;
}

static int open_disk(CBM_FILE fd, d64copy_settings *settings,
                     const void *arg, int for_writing,
                     turbo_start start, d64copy_message_cb message_cb)
{
    struct _stat statrec;
    int stat_ok, is_image, error_info;
    int tr = 0;
    char *name = (char*)arg;

    the_file = NULL;
    fs_settings = settings;
    block_count = 0;

    stat_ok = stat(name, &statrec) == 0;
    is_image = error_info = 0;

    if(stat_ok)
    {
        if(statrec.st_size == D71_BLOCKS * BLOCKSIZE)
        {
            is_image = 1;
            block_count = D71_BLOCKS;
            tr = D71_TRACKS;
        }
        else if(statrec.st_size == D71_BLOCKS * (BLOCKSIZE + 1))
        {
            is_image = 1;
            error_info = 1;
            block_count = D71_BLOCKS;
            tr = D71_TRACKS;
        }
        else
        {
            block_count = STD_BLOCKS;
            for( tr = STD_TRACKS; !is_image && tr <= TOT_TRACKS; )
            {
                is_image = statrec.st_size == block_count * BLOCKSIZE;
                if(!is_image)
                {
                    error_info = is_image =
                        statrec.st_size == block_count * (BLOCKSIZE + 1);
                }
                if(!is_image)
                {
                    block_count += d64copy_sector_count( 0, tr++ );
                }
            }
            if( is_image && tr != STD_TRACKS )
            {
                message_cb(1, "non-standard number or tracks: %d", tr);
            }
        }
    }

    if(!for_writing)
    {
        if(stat_ok)
        {
            if(is_image)
            {
                the_file = fopen(name, "rb");
                if(the_file == NULL)
                {
                    message_cb(0, "could not open %s", name);
                }
                if(error_info)
                {
                    message_cb(1, "image contains error information");
                }
                if(settings->end_track == -1)
                {
                    settings->end_track = tr;
                }
                else if(settings->end_track > tr)
                {
                    message_cb(1, "resetting end track to %d", tr);
                    settings->end_track = tr;
                }
                /* FIXME: error map */
            }
            else
            {
                message_cb(0, "neither a .d64 not .d71 file: %s", name);
            }
        }
        else
        {
            message_cb(0, "could not stat %s", name);
        }
    }
    else
    {
        the_file = fopen(name, is_image ? "r+b" : "wb");
        if(the_file)
        {
            /* check whether we must resize or create an image file */
            int new_tr;
            if(settings->two_sided)
            {
                new_tr = D71_TRACKS;
            }
            else if(settings->end_track <= STD_TRACKS)
            {
                new_tr = STD_TRACKS;
            }
            else if(settings->end_track <= EXT_TRACKS)
            {
                new_tr = EXT_TRACKS;
            }
            else
            {
                new_tr = TOT_TRACKS;
            }

            /* always use maximum size for error map */
            error_map = calloc(D71_BLOCKS, 1);
            if(!error_map)
            {
                message_cb(0, "no memory for error map");
                fclose(the_file);
                if(!is_image)
                {
                    unlink(name);
                }
                return 1;
            }

            if(is_image)
            {
                if(error_info)
                {
                    if(fseek(the_file, block_count * BLOCKSIZE, SEEK_SET) != 0 ||
                       fread(error_map, block_count, 1, the_file) != 1)
                    {
                        message_cb(0, "%s: could not read error map", name);
                        fclose(the_file);
                        return 1;
                    }
                }
                if(fseek(the_file, block_count * BLOCKSIZE, SEEK_SET) != 0)
                {
                    message_cb(0, "%s: could not seek to end of file", name);
                    fclose(the_file);
                    return 1;
                }
            }

            if(new_tr > tr)
            {
                /* grow image */
                char block[BLOCKSIZE];
                memset(block, '\0', BLOCKSIZE);
                while(tr < new_tr)
                {
                    int s;
                    s = d64copy_sector_count(settings->two_sided, ++tr);
                    if(fwrite(block, BLOCKSIZE, s, the_file) != (size_t)s)
                    {
                        message_cb(0, "could not write %s", name);
                        fclose(the_file);
                        if(!is_image)
                            unlink(name);
                        return 1;
                    }
                    block_count += s;
                }
            }
        }
        else
        {
            message_cb(0, "could not open %s", name);
        }
    }
    return the_file == NULL;
}

static void close_disk(void)
{
    int i, has_errors = 0;

    switch(fs_settings->error_mode)
    {
        case em_always:
            has_errors = 1;
            break;
        case em_never:
            has_errors = 0;
            break;
        default:
            if(error_map)
            {
                for(i = 0; !has_errors && i < block_count; i++)
                {
                    has_errors = error_map[i] != 0;
                }
            }
            break;
    }

    if(has_errors)
    {
        if(fseek(the_file, block_count * BLOCKSIZE, SEEK_SET) == 0)
        {
            fwrite(error_map, block_count, 1, the_file);
        }
    } 
    else
    {
        ftruncate(fileno(the_file), block_count * BLOCKSIZE);
    }

    if(error_map)
    {
        free(error_map);
        error_map = NULL;
    }
    if(the_file)
    {
        fclose(the_file);
        the_file = NULL;
    }
}

DECLARE_TRANSFER_FUNCS(fs_transfer, 0, 0);