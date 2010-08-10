/*
 * Motorola Background Debug Mode Driver
 * Copyright (C) 2000 James Housley
 * Copyright (C) 2000 Chris Johns
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
 * FreeBSD support by: (Started May 9, 2000)
 * James Housley
 * The Housleys dot Net
 * 65 Frank's Lane
 * Hanover, MA 02339, USA
 *
 * jim@thehousleys.net
 *
 * Chris Johns & Greg Tunnock
 * Redfern Broadband Networks.
 * Continued FreeBSD port, Sep 2000
 * This porting effort has been made possible by Redfern Broadband
 * Networks.
 *
 * ccj@acm.org
 *
 */

#define BDM_DEFAULT_DEBUG 0


#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/exec.h>
#include <sys/conf.h>
#include <sys/lkm.h>

#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/uio.h>
#include <sys/syslog.h>

#include <uvm/uvm.h>
#include <uvm/uvm_param.h>
#include <uvm/uvm_pmap.h>
#include <sys/errno.h>
#include <sys/ioccom.h>

/*
 ************************************************************************
 *     OpenBSD Driver Structures                                        *
 ************************************************************************
 */
#define  UNIT(d) ((unsigned int)(minor(d) & 0x07))

/*
 ************************************************************************
 *     UNIX driver support routines                                     *
 ************************************************************************
 */

/*
 * Function prototypes.
 */
static int init_module (void);
static void os_lock_module (void);
static void os_unlock_module (void);
static int bdmprobe (void);
static void bdmattach (void);
static void bdmdetach (void);
static void bdm_outb (int port, int value);

/*
 * Delay for a while so target can keep up.
 */
static void
bdm_delay (int counter)
{
  while (counter--) {
    __asm volatile ("nop");
  }
}

/*
 * Wait a longer while .
 */
static char BDMsleep;
static void 
bdm_sleep (u_int time)
{
  tsleep((void *)&BDMsleep, ((PZERO + 8)| PCATCH), "bdmslp", (int)time);
}

/*
 ************************************************************************
 *     OS worker functions.                                             *
 ************************************************************************
 */

static int
os_claim_io_ports (char *name, unsigned int base, unsigned int num)
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
  (void)memcpy(dst, src, (size_t)size);
  return 0;
}

static int
os_copy_out (void *dst, void *src, int size)
{
  (void)memcpy(dst, src, (size_t)size);
  return 0;
}

/**
 * Moves data from user-space address to kernel-space address.
 *
 * This is provide for Operating Systems with separate move and copy
 * strategies. If an OS doesn't have a move strategy this function will
 * be the same as os_copy_in().
 *
 * Param dst   Kernel-space address to copy to.
 * Param src   User-space address to copy from.
 * Param size  Number of bytes to copy.
 * 
 * Returns     0 if successful.
 */
static int 
os_move_in (void *dst, void *src, int size)
{
  return uiomove((caddr_t)dst, size, (struct uio *)src);
}  
#define BUF_INCREMENTED_BY_MOVE_IN

/**
 * Moves data from kernel-space address to user-space address.
 *
 * This is provide for Operating Systems with separate move and copy
 * strategies. If an OS doesn't have a move strategy this function will
 * be the same as os_copy_out().
 *
 * Param dst   User-space address to copy to.
 * Param src   Kernel-space address to copy from.
 * Param size  Number of bytes to copy.
 * 
 * Returns     0 if successful.
 */
static int 
os_move_out (void *dst, void *src, int size)
{
  return uiomove((caddr_t)src, size, (struct uio *)dst);
}  
#define BUF_INCREMENTED_BY_MOVE_OUT
 
static void
os_lock_module ()
{
}

static void
os_unlock_module ()
{
}

/*
 *  Big hack to get around differences in outb
 */
static void
bdm_outb (int port, int value)
{
  outb (port, value);
}
#undef outb
#define outb(value, port) bdm_outb (port, value)

/*
 ************************************************************************
 *       Mappings to OpenBSD                                            *
 ************************************************************************
 */

#define PRINTF   printf
#define udelay(x) DELAY (x)
#define MINOR(x) minor (x)
#define HZ       hz

/*
 ************************************************************************
 *       Include the driver code                                        *
 ************************************************************************
 */

#include "../bdm.c"

/*
 ************************************************************************
 *       Good old-fashioned UNIX driver entry points                    *
 ************************************************************************
 */

static int
openbsd_bdm_open (dev_t dev, int flags, int fmt, struct proc *p)
{
  return bdm_open (MINOR (dev));
}

static int
openbsd_bdm_close (dev_t dev, int fflag, int devtype, struct proc *p)
{
  return bdm_close (MINOR (dev));  
}

static int
openbsd_bdm_read (dev_t dev, struct uio *uio, int flag)
{
  return bdm_read (MINOR (dev), (char *)uio, uio->uio_resid);
}

static int
openbsd_bdm_write (dev_t dev, struct uio * uio, int ioflag)
{
  return bdm_write (MINOR (dev), (char *)uio, uio->uio_resid);
}

static int
openbsd_bdm_ioctl (dev_t dev, u_long cmd, caddr_t arg, int flag, struct proc *p)
{
  return bdm_ioctl (MINOR (dev), cmd, (unsigned long)arg);
}

/*
 * Driver entry points
 */
static int                    bdm_dev_registered = 0;

void cleanup_module (void);

/*
 * Hook driver into kernel
 */
static int
init_module ()
{
  int minor;

  PRINTF ("bdm_init_module %d.%d, " __DATE__ ", " __TIME__ "\n",
          BDM_DRV_VERSION >> 8, BDM_DRV_VERSION & 0xff);
  
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
        PRINTF ("BDM driver has no address for LPT%d.\n", BDM_IFACE_MINOR (minor) + 1);
        cleanup_module ();
        return EIO;
    }

    /*
     * See if the port exists
     */
    
    self->exists = 1;
    
    outb (0x00, port);
    bdm_delay (50);
    if (inb (port) != 0x00) {
      self->exists = 0;
      if (self->debugFlag)
        PRINTF ("BDM driver cannot detect LPT%d.\n", BDM_IFACE_MINOR (minor) + 1);
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
        PRINTF ("BDM driver has no interface for minor number\n");
        cleanup_module ();
        return EIO;
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
      ;
  }

  if (bdm_dev_registered) {
    bdm_dev_registered = 0;
    PRINTF ("BDM driver unregistered.\n");
  }
}


static struct cdevsw openbsd_devsw = 
{
  openbsd_bdm_open,    /* open */
  openbsd_bdm_close,   /* close */
  openbsd_bdm_read,    /* read */
  openbsd_bdm_write,   /* write */
  openbsd_bdm_ioctl,   /* ioctl */
  (void *)nullop,      /* stop */
  (void *)NULL,        /* tty */
  (void *)nullop,      /* select */
  (void *)nullop,      /* mmap */
  NULL                 /* strategy */
};

MOD_DEV("bdm", LM_DT_CHAR, BDM_MAJOR_NUMBER, &openbsd_devsw)


static int
bdmprobe ()
{
  /* For now, always report the BDM driver is able to drive the BDM hardware. */
  return 0;
}

static int 
bdm_load(void)
{
  int err;

  PRINTF("BDM init_module\n   %s\n   %s\n   %s\n",
         "$RCSfile: openbsd-bdm.c,v $", "$Revision: 1.1 $", "$Date: 2004/04/18 16:51:21 $");
  PRINTF("   Version %s\n   Compiled at %s %s\n",
         "PD-interface",
         __DATE__, __TIME__);

  err = bdmprobe ();
  if (err) {
    PRINTF ("BDM driver: probe failed\n");
    return err;
  }
  
  err = init_module();
  if (err) {
    PRINTF ("BDM driver: load failed\n");
    return err;
  } 
  
  PRINTF ("BDM driver: loaded\n");
  return 0;
}

static int
bdm_unload(void)
{
  PRINTF ("BDM driver: unloaded\n");
  return 0;
}

static int
bdm_handler(struct lkm_table *lkmtp, int cmd)
{
  int iStatus          = 0;
  struct lkm_dev *args = lkmtp->private.lkm_dev;

  switch ( cmd ) {
    case LKM_E_LOAD:
      iStatus = bdm_load();
      break;
    case LKM_E_UNLOAD:
      iStatus = bdm_unload();
      break;
  }

  return ( iStatus );
}

int bdm(struct lkm_table *lkmtp, int cmd, int ver)
{
  DISPATCH(lkmtp, cmd, ver, bdm_handler, lkm_nofunc, lkm_nofunc);
}
