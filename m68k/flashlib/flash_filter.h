/* $Id: flash_filter.h,v 1.1 2003/12/29 22:53:56 joewolf Exp $
 *
 * Header for the flash filtering layer.
 *
 * 2003-12-28 Josef Wolf (jw@raven.inka.de)
 *
 * Portions of this program which I authored may be used for any purpose
 * so long as this notice is left intact. 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* This one should be removed in the long term.
 */
# ifndef HOST_FLASHING
#  define HOST_FLASHING 1
# endif

# ifndef STREQ
# define STREQ(a,b) (!strcmp((a),(b)))
# endif

# ifndef NUMOF
# define NUMOF(ary) (sizeof(ary)/sizeof(ary[0]))
# endif

/* A driver need to register itself with this function
 */
void register_algorithm (
    int num,                      /* number that was passed to init function */
    char *driver_magic,
    int struct_size,               /* size of the chip description structure */
    int (*download_struct) (void *, unsigned long),     /* dnload chip-descr */
    int (*search_chip) (void *, char *, unsigned long),
    void (*erase) (void *, long),
    int (*erase_wait) (void *),
    int (*prog) (void *, unsigned long, unsigned char *, unsigned long),
    char *(*prog_entry)(void)      /* returns the name of the entry function */
    );

/* Load target drivers. ADR and LEN define memory region in the target that
   can be used for downloading code/data.
 */
int flash_plugin (unsigned long adr, unsigned long len, char *argv[]);

/* Register a flash chip on ADR
 */
int flash_register (char *description, unsigned long adr);

int flash_erase (unsigned long adr, long sec_adr);
int flash_erase_wait (unsigned long adr);

unsigned long write_memory (unsigned long adr, unsigned char *data,
			    unsigned long cnt);
