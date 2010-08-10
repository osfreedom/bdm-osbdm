/****************************************************************************/
/*
 *  Motorola Background Debug Mode Driver for SCO based on the linux one
 *  which it needs to compile.
 *
 *  Copyright (C) 1999  David McCullough (davidm@stallion.oz.au)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 22/10/1999 -- CCJ (ccj@acm.org)
 *     Altered to match the split driver code.
 *  
 */

#define BDM_DEFAULT_DEBUG 0

#include <sys/param.h>
#include <sys/types.h>
#include <sys/immu.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/conf.h>

#include "bdm.h"

/****************************************************************************
 *     UNIX driver support routines                                         *
 ****************************************************************************/

/*
 *  Big hack to get around differences in outb under SCO
 */

void
bdm_outb (int port, int value)
{
  outb (port, value);
}
#define outb(value, port) bdm_outb (port, value)

/****************************************************************************
 *  Other tings we need to compile                                          *
 ****************************************************************************/

#define PRINTF    printf

#define udelay(x) bdm_delay (x)

#define MINOR(x)  minor (x)

/****************************************************************************
 *  Delay for a while so target can keep up.                                *
 ****************************************************************************/

static void
bdm_delay (int counter)
{
  while (counter--)
    __asm ("nop");
}

/****************************************************************************
 *  Wait a longer while.  This function sleeps                              *
 ****************************************************************************/

static void 
bdm_sleep (u_int time)
{
  delay(time);
}

/****************************************************************************
 *     OS worker functions.                                                 *
 ****************************************************************************/

static int
os_claim_io_ports (unsigned int base, unsigned int num)
{
  return 0;
}

static int
os_release_io_ports (unsigned int base, unsigned int num)
{
  return 0;
}

static int
os_copy_in (void *dst, void *src, int size)
{
  if (copyin (src, dst, size) < 0)
    return EFAULT;
  return 0;
}

static int
os_copy_out (void *dst, void *src, int size)
{
  if (copyout (src, dst, size) < 0)
    return EFAULT;
  return 0;
}

static void
os_lock_module ()
{
}

static void
os_unlock_module ()
{
}

/****************************************************************************
 *  Include the BBM code                                                    *
 ****************************************************************************/

#include "bdm.c"

/****************************************************************************
 *  The real driver code starts here                                        *
 ****************************************************************************/

int
bdminit ()
{
  int minor, found;
  static char hex_digit[] = "0123456789ABCDEF";

  /*
   *  Set up port numbers
   */
  found = 0;
  for (minor = 0;
       minor < (sizeof bdm_device_info / sizeof bdm_device_info[0]);
       minor++) {
    extern int bdm_ports[], bdm_debug_level; /* see Space.c */
    int        port, i;
    struct BDM *self = &bdm_device_info[minor];

   /*
    *    First set the default debug level.  I use a variable
    *    here so I can set it with a debugger or just change
    *    space.c and rebuild the kernel.
    */

    self->debugFlag = bdm_debug_level;

    /*
     *    Choose a port number
     */
    i = BDM_IFACE_MINOR (minor);
    if (i >= 0 && i < 4)
      port = bdm_ports[i];
    else
      port = -1;
    if (port == -1) {
      self->exists = 0;
      continue;
    }
    
   /*
    *    See if the port exists
    */
    self->exists = 1;

    outb (0x00, port);
    udelay (50);
    if (inb (port) != 0x00) {
      self->exists = -1;
      if (self->debugFlag)
        printk("BDM driver cannot detect device %d(addr=0x%x).\n",
               BDM_IFACE_MINOR(minor), port);
    }

    bcopy ("bdm\0\0\0", self->name, 6);
    self->name[4] = hex_digit[(minor & 0xf0) >> 4];
    self->name[5] = hex_digit[minor & 0xf];
    self->portBase = self->dataPort = port;
    self->statusPort = port + 1;
    self->controlPort = port + 2;  

    switch (BDM_IFACE (minor)) {
      case BDM_CPU32_PD:    cpu32_pd_init_self (self); break;
      case BDM_CPU32_ICD:   cpu32_icd_init_self (self); break;
      case BDM_COLDFIRE_PE: cf_pe_init_self (self); break;
      default:
        self->exists = -2;
        if (self->debugFlag)
          printk("BDM driver has no interface for minor number %d\n",
                 minor);
        break;
    }
    if (self->exists) {
      printcfg("bdm", port, 4, -1, -1,
               "%d minor=%d v%d.%d, " __DATE__ ", " __TIME__,
               self->exists, minor,
               BDM_DRV_VERSION >> 8, BDM_DRV_VERSION & 0xff);
      found++;
      if (self->exists < 0)
        self->exists = 0;
    }
  }

  if (!found)
    printcfg("bdm", 0, 0, -1, -1,
             "NO-PORTS v%d.%d, " __DATE__ ", " __TIME__,
             BDM_DRV_VERSION >> 8, BDM_DRV_VERSION & 0xff);
  
  return(0);
}

/******************************************************************************/

int
bdmopen (dev_t dev, int flag, int type)
{
  u.u_error = bdm_open (MINOR(dev));
  return 0;
}

/******************************************************************************/

int
bdmclose (dev_t dev, int flag, int type)
{
  u.u_error = bdm_close (MINOR(dev));
  return 0;
}

/******************************************************************************/

int
bdmioctl (dev_t dev, int cmd, int arg, int mode)
{
  u.u_error = bdm_ioctl (MINOR(dev), cmd, arg);
  return 0;
}

/******************************************************************************/

int
bdmread (dev_t dev)
{
  u.u_error = bdm_read (MINOR (dev), u.u_base, u.u_count);
  u.u_base += u.u_count;
  u.u_count = 0;
  return(0);
}

/******************************************************************************/

int
bdmwrite (dev_t dev)
{
  u.u_error = bdm_write (MINOR (dev), u.u_base, u.u_count);
  u.u_base += u.u_count;
  u.u_count = 0;
  return (0);
}

/******************************************************************************/
