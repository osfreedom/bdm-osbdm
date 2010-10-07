/*
 * Motorola Background Debug Mode Driver
 * Copyright (C) 2003  Chris Johns
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
 * I/O Permssion support by:
 * Chris Johns
 * cjohns@users.sourceforge.net
 *
 */

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#ifdef HAVE_IOPERM
#include <sys/io.h>
#endif
#include <sys/param.h>
#include <sys/stat.h>

#include "bdm.h"
#include "ioperm.h"
#include "bdmRemote.h"

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

static int debugLevel = BDM_DEFAULT_DEBUG;

#ifndef HAVE_IOPERM
static FILE* dev_io_handle;
#endif

static int bdm_dev_registered = 0;
static int bdm_driver_open = 0;

/*
 * Unhook module from kernel
 */
static void
bdm_cleanup_module (int fd)
{
  if (bdm_dev_registered)
  {
    if ((fd >= 0) && (fd) < bdm_get_device_info_count ())
    {
      struct BDM *self = bdm_get_device_info (fd);
#ifdef HAVE_IOPERM
      ioperm (self->portBase, 3, 0);
#else
      if (dev_io_handle)
      {
        fclose (dev_io_handle);
        dev_io_handle = NULL;
      }
#endif
      bdm_dev_registered = 0;
#ifdef BDM_VER_MESSAGE
      bdmInfo ("BDM driver unregistered.\n");
#endif
    }
  }
  else
    driver_close (fd);
}

/*
 * Try and get access to the port via ioperm. If you fail flag this and let
 * the library try for a real driver.
 */
static int
bdm_ioperm_init (int minor)
{
  unsigned short port;
  unsigned char  data;
  struct BDM     *self;

#ifdef BDM_VER_MESSAGE
  bdmInfo ("bdm_ioperm_init %d.%d, " __DATE__ ", " __TIME__ "\n",
           BDM_DRV_VERSION >> 8, BDM_DRV_VERSION & 0xff);
#endif

  /*
   * Choose a port number
   */
  switch (BDM_IFACE_MINOR (minor))
  {
    case 0:  port = 0x378;  break;  /* LPT1 */
    case 1:  port = 0x278;  break;  /* LPT2 */
    case 2:  port = 0x3bc;  break;  /* LPT3 */
    case 3:  port = 0x9400; break;  /* PCI parallel port cards */
    default:
      bdmInfo ("BDM driver has no address for LPT%d.\n",
               BDM_IFACE_MINOR (minor) + 1);
      errno = EIO;
      return -1;
  }

  /*
   * Set up the port.
   */

  self = bdm_get_device_info (minor);

  /*
   * First set the default debug level.
   */

  self->debugFlag = BDM_DEFAULT_DEBUG;

  /*
   * Try the ioperm() call to claim standard parallel ports.
   *
   * For PCI parallel port adaptors in extended IO space, use
   * iopl() instead.
   */
#ifdef HAVE_IOPERM
  if (port < 0x400)
  {
    if (ioperm (port, 3, 1) < 0)
      return -1;
  }
  else
  {
    if (iopl(3) != 0)
      return -1;
  }
#else
  if (!dev_io_handle)
  {
    dev_io_handle = fopen("/dev/io", "rw");
    if (!dev_io_handle)
      return -1;
  }
#endif

  /*
   * See if the port exists
   */

  self->exists = 1;

  bdm_dev_registered = 1;

  outb (0x00, port);
  usleep (50);
  data = inb (port);

  if (data  != 0x00)
  {
    self->exists = 0;
    if (self->debugFlag)
      bdmInfo ("BDM driver cannot detect LPT%d.\n",
               BDM_IFACE_MINOR (minor) + 1);
    bdm_cleanup_module (minor);
    errno = EIO;
    return -3;
  }
  
  sprintf (self->name, "bdm%d", minor);
  self->portBase    = self->dataPort = port;
  self->statusPort  = port + 1;
  self->controlPort = port + 2;
  self->delayTimer  = 0;

  switch (BDM_IFACE (minor))
  {
    case BDM_CPU32_PD:    bdm_cpu32_pd_init_self (self); break;
    case BDM_CPU32_ICD:   bdm_cpu32_icd_init_self (self); break;
    case BDM_COLDFIRE_PE: bdm_cf_pe_init_self (self); break;
    default:
      bdmInfo ("BDM driver has no interface for minor number\n");
      bdm_cleanup_module (minor);
      errno = EIO;
      return -1;
  }

  return 0;
}

static int
bdm_ioperm_close (int fd)
{
  if (bdm_driver_open)
  {
    bdm_driver_open = 0;
    return driver_close (fd);
  }
  bdm_close (fd);
  bdm_cleanup_module (fd);
  return 0;
}

static int
bdm_ioperm_read (int fd, unsigned char *buf, int count)
{
  if (bdm_driver_open)
    return driver_read (fd, buf, count);
  errno = bdm_read (fd, buf, count);
  if (errno)
    return -1;
  return count;
}

static int
bdm_ioperm_write (int fd, unsigned char *buf, int count)
{
  if (bdm_driver_open)
    return driver_write (fd, buf, count);
  errno = bdm_write (fd, buf, count);
  if (errno)
    return -1;
  return count;
}

static int
bdm_ioperm_ioctl (int fd, unsigned int cmd, ...)
{
  va_list       args;
  unsigned long *arg;

  int iarg;
  int err = 0;

  va_start (args, cmd);

  arg = va_arg (args, unsigned long *);

  if (bdm_driver_open)
    return driver_ioctl (fd, cmd, arg);

  /*
   * Pick up the argument
   */
  if (!bdm_dev_registered) {

    switch (cmd) {
      case BDM_DEBUG:
        err = os_copy_in ((void*) &iarg, (void*) arg, sizeof iarg);
        break;
    }
    if (err)
      return err;

    if (debugLevel > 3)
      bdmInfo ("bdm_ioperm_ioctl cmd:0x%08x\n", cmd);

    switch (cmd) {
      case BDM_DEBUG:
        debugLevel = iarg;
        break;
    }
  }

  errno = bdm_ioctl (fd, cmd, (unsigned long) arg);
  if (errno)
    return -1;
  return 0;
}

static int
bdm_ioperm_ioctl_int (int fd, int code, int *var)
{
  return bdm_ioperm_ioctl (fd, code, var);
}

static int
bdm_ioperm_ioctl_command (int fd, int code)
{
  return bdm_ioperm_ioctl (fd, code, NULL);
}

static int
bdm_ioperm_ioctl_io (int fd, int code, struct BDMioctl *ioc)
{
  return bdm_ioperm_ioctl (fd, code, ioc);
}

static bdm_iface iopermIface = {
  .open      = bdm_ioperm_open,
  .close     = bdm_ioperm_close,
  .read      = bdm_ioperm_read,
  .write     = bdm_ioperm_write,
  .ioctl_int = bdm_ioperm_ioctl_int,
  .ioctl_io  = bdm_ioperm_ioctl_io,
  .ioctl_cmd = bdm_ioperm_ioctl_command,
  .error_str = NULL
};

/*
 * The device is a device name of the form /dev/bdmcpu320 or
 * /dev/bdmcf0 where the /dev/bdm must be present
 * the next field can be cpu32 or cf followed by
 * a number which is the port.
 */

int
bdm_ioperm_open (const char *devname, bdm_iface** iface)
{
  const char* device = devname;
  int         port = -1;
  int         result = 0;

  *iface = NULL;
  
  if (bdm_dev_registered)
  {
    bdmInfo ("BDM driver is already registered (Please report to BDM project).\n");
    errno = EIO;
    return -2;
  }

  if (strncmp (device, "/dev/bdm", sizeof ("/dev/bdm") - 1) == 0)
  {
    device += sizeof ("/dev/bdm") - 1;

    if (strncmp (device, "cpu32", 5) == 0)
    {
      port = 0;
      device += 5;  /* s.b. 5 */
    }
    else if (strncmp (device, "icd", 3) == 0)
    {
      port = 8;
      device += 3;
    }
    else if (strncmp (device, "cf", 2) == 0)
    {
      port = 4;
      device += 2;
    }
    else
    {
      errno = ENOENT;
      result = -1;
    }
    
    if (result == 0)
    {
      port += strtoul (device, 0, 0);
      result = bdm_ioperm_init (port);
    }
  }

  /*
   * See if the ioperm call failed. Try to open the driver. If no
   * driver is found, prepend localhost and try for a local server.
   * This make an /dev/bdmcf0 open automatically attempt to open a
   * bdmServer. This local server may be using ioperm or usb so no driver.
   */

  if (result < 0)
  {
    if (result == -1)
    {
      int fd;
      bdmInfo ("trying kernel driver: %s\n", devname);
      if ((fd = bdmLocalOpen(devname, iface)) < 0) {
        if ((strlen (devname) + sizeof ("localhost")) < 128)
        {
          char lname[128];
          strcpy (lname, "localhost:");
          strcat (lname, devname);
          bdmInfo ("trying bdm server: %s\n", lname);
          return bdmRemoteOpen (lname, iface);
        }
        return -1;
      }
      bdm_driver_open = 1;
      return fd;
    }
    return -1;
  }
  
  errno = bdm_open (port);
  if (errno)
    return -1;

  bdm_dev_registered = 1;

  *iface = &iopermIface;
  
  return port;
}
