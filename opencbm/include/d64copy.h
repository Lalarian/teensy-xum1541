/*
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael.klein@puffin.lb.shuttle.de>
*/

/* $Id: d64copy.h,v 1.1 2004-11-07 11:05:12 strik Exp $ */

#ifndef D64COPY_H
#define D64COPY_H

#define MAX_TRACKS   70   /* for .d71 */
#define MAX_SECTORS  21
    
#ifdef __cplusplus
extern "C" {
#endif

/*
 *  known BAM modes
 */
typedef enum
{
    bm_ignore = 0,      /* all sectors                    */
    bm_allocated = 1,   /* allocated sectors              */
    bm_save = 2         /* allocated sectors + BAM track  */
} d64copy_bam_mode;

typedef enum
{
    em_always,
    em_on_error,
    em_never
} d64copy_error_mode;

typedef enum
{
    bs_invalid = 0,
    bs_dont_copy = 1,
    bs_must_copy = 2,
    bs_error = 3,
    bs_copied = 4
} d64copy_bam_status;

typedef struct
{
    int warp;
    int retries;
    int interleave;
    int start_track;
    int end_track;
    int two_sided;
    int transfer_mode;
    enum cbm_device_type_e drive_type;
    d64copy_bam_mode bam_mode;
    d64copy_error_mode error_mode;
} d64copy_settings;

typedef struct
{
    int track;
    int sector;
    int read_result;
    int write_result;
    int sectors_processed;
    int total_sectors;
    d64copy_settings *settings;
    char bam[MAX_TRACKS][MAX_SECTORS+1];
} d64copy_status;

typedef void (*d64copy_message_cb)(int sev, const char *format, ...);
typedef int (*d64copy_status_cb)(d64copy_status status);

/*
 * Build '\0'-terminated list of '\0'-terminated transfer mode names.
 * Memory should be free()'d after use.
 */
extern char *d64copy_get_transfer_modes();

/*
 * parse transfer mode name ("serial1", "s2", "parallel"...) abbreviations
 * are possible
 */
extern int d64copy_get_transfer_mode_index(const char *name);

/*
 * returns malloc()'d pointer to default settings.
 * must be free()'d after use.
 */
extern d64copy_settings *d64copy_get_default_settings(void);

/*
 * return number of sectors on a given track, -1 if invalid
 */
extern int d64copy_sector_count(int two_sided, int track);

extern int d64copy_read_image(CBM_FILE cbm_fd,
                              d64copy_settings *settings,
                              int src_drive,
                              const char *dst_image,
                              d64copy_message_cb msg_cb,
                              d64copy_status_cb status_cb);

extern int d64copy_write_image(CBM_FILE cbm_fd,
                               d64copy_settings *settings,
                               const char *src_image,
                               int dst_drive,
                               d64copy_message_cb msg_cb,
                               d64copy_status_cb status_cb);

#ifdef __cplusplus
}
#endif

#endif  /* D64COPY_H */
