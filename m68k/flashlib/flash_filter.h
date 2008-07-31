/* $Id: flash_filter.h,v 1.4 2008/07/31 01:53:44 cjohns Exp $
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

#ifndef _FLASH_FILTER_H_
# define _FLASH_FILTER_H_

# include <stdint.h>

/* This one should be removed in the long term.
 */
# ifndef HOST_FLASHING
#  define HOST_FLASHING 1
# endif

# ifndef STREQ
#  define STREQ(a,b) (!strcmp((a),(b)))
# endif

# ifndef NUMOF
#  define NUMOF(ary) (sizeof(ary)/sizeof(ary[0]))
# endif

/* A driver need to register itself with this function
 */
void register_algorithm(int num, /* number that was passed to init function */
                        char *driver_magic, 
                        /* size of the chip description structure */
                        int struct_size,
                        /* dnload chip-descr */
                        int (*download_struct) (void *, uint32_t),
                        uint32_t (*search_chip) (void *, char *, uint32_t),
                        void (*erase) (void *, int32_t), 
                        int (*blank_chk) (void *, int32_t), 
                        int (*erase_wait) (void *), 
                        uint32_t (*prog) (void *, uint32_t, 
                                          unsigned char *, uint32_t),
                        /* returns the name of the entry function */
                        char *(*prog_entry) (void)
  );

/* Load target drivers. ADR and LEN define memory region in the target that
   can be used for downloading code/data.
 */
int flash_plugin (int (*prfunc) (const char *format, ...),
                  uint32_t adr, uint32_t len, char *argv[]);

/* Register a flash chip on ADR
 */
int flash_register (char *description, uint32_t adr, char *hint_driver);

int flash_erase (uint32_t adr, int32_t sec_adr);
int flash_blank_chk (uint32_t adr, int32_t sec_adr);
int flash_erase_wait (uint32_t adr);

uint32_t write_memory (uint32_t adr, unsigned char *data, uint32_t cnt);

int flash_set_var (const char *name, uint32_t value);
int flash_get_var (const char *name, uint32_t *value, uint32_t value_default);

int flash_spin (int c);

#endif
