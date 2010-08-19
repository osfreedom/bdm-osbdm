/*
 * Chris Johns <cjohns@users.sourceforge.net>
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

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "localIface.h"

/*
 * Close.
 */
static int
bdmLocalClose (int fd)
{
  return close (fd);
}

/*
 * Do an int-argument BDM ioctl
 */
static int
bdmLocalIoctlInt (int fd, int code, int *var)
{
  return ioctl (fd, code, var);
}

/*
 * Do a command (no-argument) BDM ioctl
 */
static int
bdmLocalIoctlCommand (int fd, int code)
{
  return ioctl (fd, code, NULL);
}

/*
 * Do a BDMioctl-argument BDM ioctl
 */
static int
bdmLocalIoctlIo (int fd, int code, struct BDMioctl *ioc)
{
  return ioctl (fd, code, ioc);
}

/*
 * Do a BDM read
 */
static int
bdmLocalRead (int fd, unsigned char *cbuf, int nbytes)
{
  return read (fd, cbuf, nbytes);
}

/*
 * Do a BDM write
 */
static int
bdmLocalWrite (int fd, unsigned char *cbuf, int nbytes)
{
  return write (fd, cbuf, nbytes);
}

static bdm_iface localIface = {
  .open = bdmLocalOpen,
  .close = bdmLocalClose,
  .read = bdmLocalRead,
  .write = bdmLocalWrite,
  .ioctl_int = bdmLocalIoctlInt,
  .ioctl_io = bdmLocalIoctlIo,
  .ioctl_cmd = bdmLocalIoctlCommand,
  .error_str = NULL
};

int
bdmLocalOpen(const char* name, bdm_iface** iface)
{
  int fd = open (name, O_RDWR);
  if (fd < 0)
    *iface = NULL;
  else
    *iface = &localIface;
  return fd;
}
