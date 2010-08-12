/*
 * Motorola Background Debug Mode Driver
 * Copyright (C) 2003-2010  Chris Johns
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * User land support by:
 * Chris Johns
 * cjohns@users.sourceforge.net
 *
 */

#include <config.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/ioctl.h>
#ifdef HAVE_IOPERM
#include <sys/io.h>
#endif
#include <sys/param.h>
#include <sys/stat.h>

#ifdef __FreeBSD__
#include <machine/cpufunc.h>
/*
 * Need to swap around the parameters to the outb call to match Linux.
 */
static inline void fb_outb (u_int port, u_char data)
{
  outb (port, data);
}

#undef outb
#define outb(d, p) fb_outb(p, d)
#endif

#define BDM_DEFAULT_DEBUG 0

int debugLevel = BDM_DEFAULT_DEBUG;

int
driver_close (int fd)
{
  return close (fd);
}

int
driver_read (int fd, char *buf, size_t count)
{
  return read (fd, buf, count);
}

int
driver_write (int fd, char *buf, size_t count)
{
  return write (fd, buf, count);
}

int
driver_ioctl (int fd, unsigned long int request, ...)
{
  va_list args;
  unsigned long *arg;
  va_start (args, request);
  arg = va_arg (args, unsigned long *);
  return ioctl (fd, request, arg);
}

int
driver_open (const char *pathname, int flags)
{
  return open (pathname, flags);
}

/*
 ************************************************************************
 *     Add the missing the write/read define.                           *
 ************************************************************************
 */

#define IOC_OUTIN   0x10000000      /* copy in parameters */

/*
 ************************************************************************
 *   Unix driver support routines                                       *
 ************************************************************************
 */

#define udelay usleep

/*
 * Delay for a while so target can keep up.
 */
void
bdm_delay (int counter)
{
  volatile unsigned long junk;
  while (counter--) {
    junk++;
  }
}

#ifndef HZ
#define HZ 1000
#endif

/*
 * Delay specified number of milliseconds
 */
void
bdm_sleep (unsigned long time)
{
  usleep (time * (100 * (1000 / HZ)));
}

/*
 ************************************************************************
 *     OS worker functions.                                             *
 ************************************************************************
 */

int
os_claim_io_ports (char *name, unsigned int base, unsigned int num)
{
  return 0;
}

int
os_release_io_ports (unsigned int base, unsigned int num)
{
  return 0;
}

int
os_copy_in (void *dst, void *src, int size)
{
  /*
   * We run in the application and talk directly to the hardware.
   */
  memcpy (dst, src, size);
  return 0;
}

int
os_move_in (void *dst, void *src, int size)
{
  return os_copy_in (dst, src, size);
}

int
os_copy_out (void *dst, void *src, int size)
{
  memcpy (dst, src, size);
  return 0;
}

int
os_move_out (void *dst, void *src, int size)
{
  return os_copy_out (dst, src, size);
}

void
os_lock_module ()
{
}

void
os_unlock_module ()
{
}

#ifdef interface
#undef interface
#endif

/*
 * Interface is exported.
 */
#define BDM_STATIC

/*
 * Print to stdout.
 */
#define PRINTF bdmInfo

/*
 ************************************************************************
 *       Include the driver code                                        *
 ************************************************************************
 */

#include "bdm.c"

