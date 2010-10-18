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

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bdm.h"
#include "BDMlib.h"

#include "tblcf/tblcf.h"
#include "tblcf/tblcf_usb.h"

#include "bdmusb_low_level.h"

#include "bdmusb.h"

#include "bdm-usb.h"

extern int debugLevel;

int os_copy_in (void *dst, void *src, int size);
int os_copy_out (void *dst, void *src, int size);

static int
bdm_usb_close (int fd)
{
  bdm_close (fd);
  bdmusb_usb_close(fd);
  return 0;
}

static int
bdm_usb_read (int fd, unsigned char *buf, int count)
{
  errno = bdm_read (fd, buf, count);
  if (errno)
    return -1;
  return count;
}

static int
bdm_usb_write (int fd, unsigned char *buf, int count)
{
  errno = bdm_write (fd, buf, count);
  if (errno)
    return -1;
  return count;
}

static int
bdm_usb_ioctl (int fd, unsigned int cmd, ...)
{
  va_list       args;
  unsigned long *arg;

  int iarg;
  int err = 0;

  va_start (args, cmd);

  arg = va_arg (args, unsigned long *);

  /*
   * Pick up the argument
   */
  switch (cmd) {
    case BDM_DEBUG:
      err = os_copy_in ((void*) &iarg, (void*) arg, sizeof iarg);
      break;
  }
  if (err)
    return err;

  if (debugLevel > 3)
    bdmInfo ("bdm_usb_ioctl cmd:0x%08x\n", cmd);

  switch (cmd) {
    case BDM_DEBUG:
      debugLevel = iarg;
      break;
  }

  errno = bdm_ioctl (fd, cmd, (unsigned long) arg);
  if (errno)
    return -1;
  return 0;
}

static int
bdm_usb_ioctl_int (int fd, int code, int *var)
{
  return bdm_usb_ioctl (fd, code, var);
}

static int
bdm_usb_ioctl_command (int fd, int code)
{
  return bdm_usb_ioctl (fd, code, NULL);
}

static int
bdm_usb_ioctl_io (int fd, int code, struct BDMioctl *ioc)
{
  return bdm_usb_ioctl (fd, code, ioc);
}

static bdm_iface usbIface = {
  .open      = bdm_usb_open,
  .close     = bdm_usb_close,
  .read      = bdm_usb_read,
  .write     = bdm_usb_write,
  .ioctl_int = bdm_usb_ioctl_int,
  .ioctl_io  = bdm_usb_ioctl_io,
  .ioctl_cmd = bdm_usb_ioctl_command,
  .error_str = NULL
};

static usbdm_options_type_e config_options;
target_type_e targetType;

int 
bdm_usb_set_target(target_type_e type)
{
  targetType = type;
}

int 
bdm_usb_set_options(usbdm_options_type_e *cfg_options)
{

  //bdmusb_dev *udev = &usb_devs[dev_dec];
  
  /* Set the usbdm configurable options */
  config_options.targetVdd = cfg_options->targetVdd;
  config_options.cycleVddOnReset = cfg_options->cycleVddOnReset;
  config_options.cycleVddOnConnect = cfg_options->cycleVddOnConnect;
  config_options.leaveTargetPowered = cfg_options->leaveTargetPowered;
  config_options.autoReconnect = cfg_options->autoReconnect;
  config_options.guessSpeed = cfg_options->guessSpeed;
  config_options.useAltBDMClock = cfg_options->useAltBDMClock;
  config_options.useResetSignal = cfg_options->useResetSignal;
  config_options.targetClockFreq = cfg_options->targetClockFreq;
  
  //if (udev->type == P_USBDM_V2)
  //  usbdm_set_options(udev);
  
  return 0;
}

int
bdm_usb_open (const char *device, bdm_iface** iface)
{
  struct BDM *self;
  int         devs;
  int         dev;
  char        usb_device[256];
#if linux
  ssize_t     dev_size;
  char        udev_usb_device[256];
#endif

  *iface = NULL;
  
  /*
   * Initialise the USB layers.
   */
  devs = bdmusb_init ();

#ifdef BDM_VER_MESSAGE
  fprintf (stderr, "usb-bdm-init %d.%d, " __DATE__ ", " __TIME__ " devices:%d\n",
           BDM_DRV_VERSION >> 8, BDM_DRV_VERSION & 0xff, devs);
#endif

#if linux
  {
    struct stat sb;
    char        *p;

    if (lstat (device, &sb) == 0)
    {
      if (S_ISLNK (sb.st_mode))
      {
        dev_size = readlink (device, udev_usb_device, sizeof (udev_usb_device));

        if ((dev_size >= 0) || (dev_size < sizeof (udev_usb_device) - 1))
        {
          udev_usb_device[dev_size] = '\0';

          /*
           * On Linux this is bus/usb/....
           */

          if (strncmp (udev_usb_device, "bus/usb", sizeof ("bus/usb") - 1) == 0)
          {
            memmove (udev_usb_device, udev_usb_device + sizeof ("bus/usb"),
                     sizeof (udev_usb_device) - sizeof ("bus/usb") - 1);

            p = strchr (udev_usb_device, '/');

            *p = '-';

            device = udev_usb_device;
          }
        }
      }
    }
  }
#endif
  
  for (dev = 0; dev < devs; dev++)
  {
    char* name;
    int   length;

    bdmusb_dev_name (dev, usb_device, sizeof (usb_device));

    name = usb_device;
    length = strlen (usb_device);

    while (name && length)
    {
      name = strchr (name, device[0]);

      if (name)
      {
        length = strlen (name);

        if (strlen (device) <= length)
        {
          if (strncmp (device, name, length) == 0)
          {
            /*
             * Set up the self srructure. Only ever one with ioperm.
             */
            self = bdm_get_device_info (0);

            /*
             * Open the USB device.
             */
            self->usbDev = bdmusb_usb_open (usb_device);

            if (self->usbDev < 0)
            {
              errno = EIO;
              return -1;
            }

            /*
             * First set the default debug level.
             */

            self->debugFlag = BDM_DEFAULT_DEBUG;

            /*
             * Force the port to exist.
             */
            self->exists = 1;

            self->delayTimer = 0;
      
            bdm_tblcf_init_self (self);
	    
	    *iface = &usbIface;
	    
	    /* Check the USBDM/OSBDM version */
	    bdmusb_dev *udev = &usb_devs[self->usbDev];
	    //if ( (udev->desc.idVendor == OSBDM_VID) && (udev->desc.idProduct==OSBDM_PID) ) {
	    if (udev->type == P_USBDM) {
		usbmd_version_t usbdm_version;
		
		pod_type_e old_type;
		//udev->dev_ref = bdmusb_usb_open(udev->name);
		// Try first with the USBDM. 
		old_type = udev->type;
		udev->type = P_USBDM_V2;
		bdmusb_get_version(udev, &usbdm_version);
		
		// Know what version we have
		if (usbdm_version.bdm_soft_ver <= 0x10) {
		    udev->type = P_OSBDM;
		    self->interface = BDM_COLDFIRE_OSBDM;
		} else if (usbdm_version.bdm_soft_ver <= 0x15) {
		    udev->type = P_USBDM;
		    self->interface = BDM_COLDFIRE_USBDM;
		} else {
		    udev->type = P_USBDM_V2;
		    self->interface = BDM_COLDFIRE_USBDM;
		}
		
		udev->use_only_ep0 = (usbdm_version.icp_hw_ver & 0xC0) == 0;
		//bdmusb_usb_close(udev->dev_ref);
		
		// Set the target type or test it?
		//target_type_e targetType = T_CFV1;
		udev->target = targetType;
		
		/* Get the hw capabilities */
		usbdm_get_capabilities(udev);
		
		/* Set the default configurable options 
		  "TargetVdd                Off"
		  "CycleTargetVddOnReset    Disable"
		  "CycleTargetVddOnConnect  Disable"
		  "LeaveTargetVddOnExit     Disable"
		  "TargetAutoReconnect      Enable"
		  "GuessSpeed               Enable"
		  "AltBDMClock              Disable"
		  "UseResetSignal           Enable"
		  "ManualCycleVdd           Enable"
		  "DerivativeType           Unused"
		  "TargetClock              500"
		  "UsePSTSignals            Disable"
		  "MiscOptions              0"
		  */
		/*udev->options.targetVdd = 0; // 3.3V
		udev->options.cycleVddOnReset = 0;
		udev->options.cycleVddOnConnect = 0;
		udev->options.leaveTargetPowered = 0;
		udev->options.autoReconnect = 1;
		udev->options.guessSpeed = 1;
		udev->options.useAltBDMClock = 0;
		udev->options.useResetSignal = 1;
		udev->options.manuallyCycleVdd = 1;
		udev->options.derivative_type = 0;
		udev->options.targetClockFreq = 500;
		udev->options.usePSTSignals = 0;
		udev->options.miscOptions = 0;
		*/
		
		/* Set the usbdm configurable options */
		udev->options.targetVdd = config_options.targetVdd;
		udev->options.cycleVddOnReset = config_options.cycleVddOnReset;
		udev->options.cycleVddOnConnect = config_options.cycleVddOnConnect;
		udev->options.leaveTargetPowered = config_options.leaveTargetPowered;
		udev->options.autoReconnect = config_options.autoReconnect;
		udev->options.guessSpeed = config_options.guessSpeed;
		udev->options.useAltBDMClock = config_options.useAltBDMClock;
		udev->options.useResetSignal = config_options.useResetSignal;
		udev->options.targetClockFreq = config_options.targetClockFreq;
		
		usbdm_set_options(udev);
		
		if (targetType == T_CFV1)
		  self->processor = BDM_COLDFIRE_V1;
		else if (targetType == T_CFVx)
		  self->processor = BDM_COLDFIRE;
		
		if (bdmusb_set_target_type(udev->dev_ref, targetType) == 0)
		    self->cf_running = 1;
		else {
		    //bdm_usb_close(udev->dev_ref);
		    return -1;
		}
		
		// Try to connect
		usbdm_connect(udev->dev_ref);
	    }
	    
	    // Now init the HW
	    //self->init_hardware(self);
	    errno = bdm_open (0);
            if (errno)
              return -1;
            
            return 0;
          }
        }

        name++;
        length = strlen (name);
      }
    }
  }

  errno = ENOENT;
  return -1;
}
