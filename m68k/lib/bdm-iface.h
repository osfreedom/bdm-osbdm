/*
 * Chris Johns <cjohns@users.sourceforge.net>
 * Copyright 2010
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
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _BDM_IFACE_H_
#define _BDM_IFACE_H_

#if __cplusplus
extern "C" 
{
#endif

#include "bdm.h"

/*
 * An interface defintion.
 */
  typedef struct _bdm_iface {
    int (*open)(const char* name, struct _bdm_iface** iface);
    int (*close)(int fd);
    int (*read)(int fd, unsigned char *cbuf, int nbytes);
    int (*write)(int fd, unsigned char *cbuf, int nbytes);
    int (*ioctl_int)(int fd, int code, int *var);
    int (*ioctl_io)(int fd, int code, struct BDMioctl *ioc);
    int (*ioctl_cmd)(int fd, int code);

    /*
     * Can be NULL.
     */
    const char* (*error_str)(int error_no);
  } bdm_iface;

#if __cplusplus
}
#endif

#endif
