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
 * Cybertec Pty Ltd.
 *
 * cjohns@cybertec.com.au
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/param.h>
#include <sys/stat.h>

#define BDM_DEFAULT_DEBUG 0

static int debugLevel = BDM_DEFAULT_DEBUG;

/*
 ************************************************************************
 *     Override the C library function.                                 *
 ************************************************************************
 */

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
  unsigned long arg;

  va_start (args, request);
  arg = va_arg (args, unsigned long *);
  return ioctl (fd, request, arg);
}

int
driver_open (const char *pathname, int flags)
{
  return open (pathname, flags);
}

#define open  ioperm_bdm_open
#define close ioperm_bdm_close
#define ioctl ioperm_bdm_ioctl
#define read  ioperm_bdm_read
#define write ioperm_bdm_write

/*
 ************************************************************************
 *     Add the missing the write/read define.                           *
 ************************************************************************
 */

#define IOC_OUTIN   0x10000000      /* copy in parameters */

//#define _IOWR(x,y,t) (IOC_OUTIN|(((long)sizeof(t)&IOCPARM_MASK)<<16)|(x<<8)|y)

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
#if 0
  volatile unsigned long junk;
  
  while (counter--) {
    junk++;
  }
#endif
}

/*
 * Delay specified number of milliseconds
 */
void 
bdm_sleep (unsigned long time)
{
  udelay (time * 1000);
}

/*
 ************************************************************************
 *     OS worker functions.                                             *
 ************************************************************************
 */

int
os_claim_io_ports (char *name, unsigned int base, unsigned int num)
{
  /*
   * I am told ioperm handles this.
   */
  return 0;
}

int
os_release_io_ports (unsigned int base, unsigned int num)
{
  return 0;
}

#define os_move_in os_copy_in

int
os_copy_in (void *dst, void *src, int size)
{
  /*
   * We run in the application and talk directly to the hardware.
   */
  memcpy (dst, src, size);
  return 0;
}

#define os_move_out os_copy_out

int
os_copy_out (void *dst, void *src, int size)
{
  memcpy (dst, src, size);
  return 0;
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
 ************************************************************************
 *       Mapping to Linux kernel functions                              *
 ************************************************************************
 */

#define PRINTF printf

/*
 ************************************************************************
 *       Include the driver code                                        *
 ************************************************************************
 */

#include "bdm.c"

/*
 ************************************************************************
 *       Good old-fashioned UNIX driver entry points                    *
 ************************************************************************
 */

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
    if ((fd >= 0) &&
        (fd < (sizeof(bdm_device_info) / sizeof(bdm_device_info[0]))))
    {
      struct BDM *self = &bdm_device_info[fd];
      ioperm (self->portBase, 3, 0);
      bdm_dev_registered = 0;
#ifdef BDM_VER_MESSAGE
      printf ("BDM driver unregistered.\n");
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
int
ioperm_bdm_init (int minor)
{
  unsigned short port;
  struct BDM     *self;
  
#ifdef BDM_VER_MESSAGE
  printf ("bdm_init %d.%d, " __DATE__ ", " __TIME__ "\n",
          BDM_DRV_VERSION >> 8, BDM_DRV_VERSION & 0xff);
#endif

  if (bdm_dev_registered)
  {
    printf ("BDM driver is already registered (Please report to BDM project).\n");
    errno = EIO;
    return -1;
  }
  
  /*
   * Choose a port number
   */
  switch (BDM_IFACE_MINOR (minor))
  {
    case 0:  port = 0x378;  break;  /* LPT1 */
    case 1:  port = 0x278;  break;  /* LPT2 */
    case 2:  port = 0x3bc;  break;  /* LPT3 */
    case 3:  port = 0x2bc;  break;  /* LPT4, ccj - made this up :-) */
    default:
      printf ("BDM driver has no address for LPT%d.\n",
              BDM_IFACE_MINOR (minor) + 1);
      errno = EIO;
      return -1;
  }

  /*
   * Set up the port.
   */

  self = &bdm_device_info[minor];
  
  /*
   * First set the default debug level.
   */
    
  self->debugFlag = BDM_DEFAULT_DEBUG;
    
  /*
   * Try the ioperm call to claim the parallel port.
   */
  if (ioperm (port, 3, 1) < 0)
    return -1;
  
  /*
   * See if the port exists
   */
    
  self->exists = 1;

  bdm_dev_registered = 1;

  outb (0x00, port);
  udelay (50);
  
  if (inb (port) != 0x00)
  {
    self->exists = 0;
    if (self->debugFlag)
      printf ("BDM driver cannot detect LPT%d.\n",
              BDM_IFACE_MINOR (minor) + 1);
    bdm_cleanup_module (minor);
    errno = EIO;
    return -1;
  }
    
  sprintf (self->name, "bdm%d", minor);
  self->portBase    = self->dataPort = port;
  self->statusPort  = port + 1;
  self->controlPort = port + 2;  
  self->delayTimer  = 0;
    
  switch (BDM_IFACE (minor))
  {
    case BDM_CPU32_PD:    cpu32_pd_init_self (self); break;
    case BDM_CPU32_ICD:   cpu32_icd_init_self (self); break;
    case BDM_COLDFIRE_PE: cf_pe_init_self (self); break;
    default:
      printf ("BDM driver has no interface for minor number\n");
      bdm_cleanup_module (minor);
      errno = EIO;
      return -1;
  }
  
  return 0;
}

/*
 * The device is a device name of the form /dev/bdmcpu320 or
 * /dev/bdmcf0 where the /dev/bdm must be present
 * the next field can be cpu32 or cf followed by
 * a number which is the port.
 */

static int remoteOpen (const char *name);

int
ioperm_bdm_open (const char *devname, int flags, ...)
{
  const char* device = devname;
  int         port = 0;

  if (strncmp (device, "/dev/bdm", sizeof ("/dev/bdm") - 1))
  {
    errno = ENOENT;
    return -1;
  }

  device += sizeof ("/dev/bdm") - 1;

  if (strncmp (device, "cpu32", 5) == 0)
  {
    port = 0;
    device += 5;  /* s.b. 5 */
  }
  else if (strncmp (device, "cf", 2) == 0)
  {
    port = 4;
    device += 2;
  }
  else
  {
    errno = ENOENT;
    return -1;
  }

  port += strtoul (device, 0, 0);

  /*
   * Try an ioperm call. If it fails, try to open the driver. If not
   * driver is found, prepend localhost and try for a local server.
   * This make an /dev/bdmcf0 open automatically attemp to open a
   * bdmServer. This local server may be using ioperm so no driver.
   */

  if (ioperm_bdm_init (port) < 0)
  {
    int fd;
    if ((fd = driver_open (devname, flags)) < 0) {
      if ((strlen (devname) + sizeof ("localhost")) < 128)
      {
        char lname[128];
        strcpy (lname, "localhost:");
        strcat (lname, devname);
        return remoteOpen (lname);
      }
      return -1;
    }
    bdm_driver_open = 1;
    return fd;
  }
  
  errno = bdm_open (port);
  if (errno)
    return -1;

  return port;
}

int
ioperm_bdm_close (int fd)
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

unsigned long
ioperm_bdm_read (int fd, char *buf, unsigned long count)
{
  if (bdm_driver_open)
    return driver_read (fd, buf, count);
  errno = bdm_read (fd, buf, count);
  if (errno)
    return -1;
  return count;
}

unsigned long
ioperm_bdm_write (int fd, char *buf, unsigned long count)
{
  if (bdm_driver_open)
    return driver_write (fd, buf, count);
  errno = bdm_write (fd, buf, count);
  if (errno)
    return -1;
  return count;
}

int
ioperm_bdm_ioctl (int fd, unsigned int cmd, ...)
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
      printf ("ioperm_bdm_ioctl cmd:0x%08x\n", cmd);
  
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
