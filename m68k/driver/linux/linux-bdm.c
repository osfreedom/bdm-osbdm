/*
 * Motorola Background Debug Mode Driver
 * Copyright (C) 1995  W. Eric Norum
 * Copyright (C) 1998  Chris Johns
 *
 * Based on:
 *  1. `A Background Debug Mode Driver Package for Motorola's
 *     16- and 32-Bit Microcontrollers', Scott Howard, Motorola
 *     Canada, 1993.
 *  2. `Linux device driver for public domain BDM Interface',
 *     M. Schraut, Technische Universitaet Muenchen, Lehrstuhl
 *     fuer Prozessrechner, 1995.
 * 
 * Extended to support the ColdFire BDM interface using the P&E
 * module which comes with the EVB. Currently only tested with the
 * 5206 (5V) device.
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
 * W. Eric Norum
 * Saskatchewan Accelerator Laboratory
 * University of Saskatchewan
 * 107 North Road
 * Saskatoon, Saskatchewan, CANADA
 * S7N 5C6
 * 
 * eric@skatter.usask.ca
 *
 * Coldfire support by:
 * Chris Johns
 * Objective Design Systems
 * 35 Cairo Street
 * Cammeray, Sydney, 2062, Australia
 *
 * ccj@acm.org
 *
 * 22/10/1999 -- CCJ (ccj@acm.org)
 *     Move the linux specific parts into a separate directory
 *     and file.
 */

#define BDM_DEFAULT_DEBUG 0

#include <linux/config.h>

#if CONFIG_MODVERSIONS
#define MODVERSIONS
#include <linux/modversions.h>
#endif

#include <linux/module.h>
#include <linux/version.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/segment.h>
#if (LINUX_VERSION_CODE > 131336)
#include <asm/uaccess.h>
#endif /* LINUX_VERSION_CODE */
#include <linux/errno.h>

/*
 ************************************************************************
 *     UNIX driver support routines                                     *
 ************************************************************************
 */

/*
 * Delay for a while so target can keep up.
 */
static void
bdm_delay (int counter)
{
  while (counter--) {
    asm volatile ("nop");
  }
}

/*
 * Wait a longer while .
 */
static void 
bdm_sleep (u_int time)
{
  current->state = TASK_INTERRUPTIBLE;
#if (LINUX_VERSION_CODE > 0x020101) /* 2.1.1 ?? */
  schedule_timeout (time);
#else
  current->timeout = jiffies + time;
  schedule ();
#endif
}

/*
 ************************************************************************
 *       Some Linux driver compatability code - for older kernels       *
 ************************************************************************
 */

#if (LINUX_VERSION_CODE < 131336)

static int copy_from_user (void *to, const void *from_user, unsigned long len)
{
  int error;

  error = verify_area (VERIFY_READ, from_user, len);
  if (error)
    return len;
  memcpy_fromfs (to, from_user, len);
  return 0;
}

static int copy_to_user (void *to_user, const void *from, unsigned long len)
{
  int error;
       
  error = verify_area (VERIFY_WRITE, to_user, len);
  if (error)
    return len;
  memcpy_tofs (to_user, from, len);
  return 0;
}

#endif /* LINUX_VERSION_CODE */


/*
 ************************************************************************
 *     OS worker functions.                                             *
 ************************************************************************
 */

static int
os_claim_io_ports (char *name, unsigned int base, unsigned int num)
{
  if (check_region (base, 4))
    return EBUSY;
  request_region (base, 4, name);  
  return 0;
}

static int
os_release_io_ports (unsigned int base, unsigned int num)
{
  release_region (base, 4);
  return 0;
}

#define os_move_in os_copy_in

static int
os_copy_in (void *dst, void *src, int size)
{
  if (copy_from_user (dst, src, size))
    return EFAULT;
  return 0;
}

#define os_move_out os_copy_out

static int
os_copy_out (void *dst, void *src, int size)
{
  if (copy_to_user (dst, src, size))
    return EFAULT;
  return 0;
}

static void
os_lock_module ()
{
  MOD_INC_USE_COUNT;
}

static void
os_unlock_module ()
{
  MOD_DEC_USE_COUNT;
}

/*
 ************************************************************************
 *       Mapping to Linux kernel functions                              *
 ************************************************************************
 */

#define PRINTF printk

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

static int
linux_bdm_open (struct inode *inode, struct file *file)
{
  return -bdm_open (MINOR (inode->i_rdev));
}

/*
 * The old Linux driver set the control port back to the value it
 * had when the device was opened.  This is good when the port is
 * being shared between the BDM interface and a printer, but has
 * the unfortunate side effect of freezing the target.  This makes
 * it inconvenient to use the debugger, or a downloader program,
 * since the target freezes as soon as the debugger, or downloader,
 * exits.
 */
#if (LINUX_VERSION_CODE > 0x020101) /* 2.1.1 ?? */
static int
linux_bdm_release (struct inode *inode, struct file *file)
#else
static void
linux_bdm_release (struct inode *inode, struct file *file)
#endif
{
  bdm_close (MINOR (inode->i_rdev));
  
#if (LINUX_VERSION_CODE > 0x020101) /* 2.1.1 ?? */
  return 0;
#endif
}

#if (LINUX_VERSION_CODE > 0x020101) /* 2.1.1 ?? */
static ssize_t
linux_bdm_read (struct file *file, char *buf, size_t count, loff_t *offp)
#else
static int
linux_bdm_read (struct inode *inode, struct file *file, char *buf, int count)
#endif
{
  unsigned int minor;
  int          err;
  
#if (LINUX_VERSION_CODE > 0x020101) /* 2.1.1 ?? */
  minor = MINOR (file->f_dentry->d_inode->i_rdev);
#else
  minor = MINOR (inode->i_rdev);
#endif

  err = bdm_read (minor, buf, count);
  if (err)
    return -err;
  return count;
}

#if (LINUX_VERSION_CODE > 0x020101) /* 2.1.1 ?? */
static ssize_t
linux_bdm_write (struct file *file, const char *buf, size_t count, loff_t *offp)
#else
static int
linux_bdm_write (struct inode *inode, struct file *file, const char *buf, int count)
#endif
{
  unsigned int minor;
  int          err;
  
#if (LINUX_VERSION_CODE > 0x020101) /* 2.1.1 ?? */
  minor = MINOR (file->f_dentry->d_inode->i_rdev);
#else
  minor = MINOR (inode->i_rdev);
#endif

  err = bdm_write (minor, (char*) buf, count);
  if (err)
    return -err;
  return count;
}

static int
linux_bdm_ioctl (struct inode  *inode,
                 struct file   *file,
                 unsigned int  cmd,
                 unsigned long arg)
{
  return -bdm_ioctl (MINOR (inode->i_rdev), cmd, arg);
}

/*
 * Driver entry points
 */
static struct file_operations bdm_fops;
static int bdm_dev_registered = 0;

void cleanup_module (void);

/*
 * Hook driver into kernel
 */
int
init_module (void)
{
  int minor;

  printk ("bdm_init_module %d.%d, " __DATE__ ", " __TIME__ "\n",
          BDM_DRV_VERSION >> 8, BDM_DRV_VERSION & 0xff);
  
  /*
   * Set up entry points
   * This used to be done with a static initializer, but the layout
   * of the structure seems to change substantially between kernel
   * versions.
   */

  memset (&bdm_fops, 0, sizeof bdm_fops);
  bdm_fops.read    = linux_bdm_read;
  bdm_fops.write   = linux_bdm_write;
  bdm_fops.ioctl   = linux_bdm_ioctl;
  bdm_fops.open    = linux_bdm_open;
  bdm_fops.release = linux_bdm_release;

  /*
   * Register with kernel
   */

  if (register_chrdev (BDM_MAJOR_NUMBER, "bdm", &bdm_fops)) {
    printk ("Unable to get major number %d for BDM driver.\n", BDM_MAJOR_NUMBER);
    return -EIO;
  }

  bdm_dev_registered = 1;
  
  /*
   * Set up port numbers
   */

  for (minor = 0 ;
       minor < (sizeof bdm_device_info / sizeof bdm_device_info[0]) ;
       minor++) {
    int port;
    struct BDM *self = &bdm_device_info[minor];

    /*
     * First set the default debug level.
     */
    
    self->debugFlag = BDM_DEFAULT_DEBUG;
    
    /*
     * Choose a port number
     */

    switch (BDM_IFACE_MINOR (minor)) {
      case 0:  port = 0x378;  break;  /* LPT1 */
      case 1:  port = 0x278;  break;  /* LPT2 */
      case 2:  port = 0x3bc;  break;  /* LPT3 */
      case 3:  port = 0x2bc;  break;  /* LPT4, ccj - made this up :-) */
      default:
        printk ("BDM driver has no address for LPT%d.\n", BDM_IFACE_MINOR (minor) + 1);
        cleanup_module();  
        return -EIO;
    }

    /*
     * See if the port exists
     */
    
    self->exists = 1;
    
    outb (0x00, port);
    udelay (50);
    if (inb (port) != 0x00) {
      self->exists = 0;
      if (self->debugFlag)
        printk ("BDM driver cannot detect LPT%d.\n", BDM_IFACE_MINOR (minor) + 1);
      continue;
    }
    
    sprintf (self->name, "bdm%d", minor);
    self->portBase    = self->dataPort = port;
    self->statusPort  = port + 1;
    self->controlPort = port + 2;  
    self->delayTimer  = 0;
    
    switch (BDM_IFACE (minor)) {
      case BDM_CPU32_PD:     cpu32_pd_init_self (self); break;
      case BDM_CPU32_ICD:    cpu32_icd_init_self (self); break;
      case BDM_COLDFIRE_PE:  cf_pe_init_self (self); break;
      default:
        printk ("BDM driver has no interface for minor number\n");
        cleanup_module();  
        return -EIO;
    }
  }
  
  return 0;
}

/*
 * Unhook module from kernel
 */
void
cleanup_module (void)
{
  int minor;

  for (minor = 0 ;
       minor < (sizeof bdm_device_info / sizeof bdm_device_info[0]) ;
       minor++) {
    struct BDM *self = &bdm_device_info[minor];
    if (self->exists && self->portsAreMine)
      release_region (self->portBase, 4);
  }

  if (bdm_dev_registered) {
    bdm_dev_registered = 0;
    if (unregister_chrdev (BDM_MAJOR_NUMBER, "bdm"))
      printk ("Unable to unregister BDM driver.\n");
    else
      printk ("BDM driver unregistered.\n");
  }
}
