/*
    BDM USB abstraction project
    Copyright (C) 2010  Rafael Campos
    Rafael Campos Las Heras (rafael@freedom.ind.br)
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "log.h"
#include "bdmusb-hwdesc.h"
#include "bdmusb.h"
//#include "tblcf_usb.h"
#include "bdmusb_low_level.h"

#include "commands.h"

static unsigned char usb_data[MAX_DATA_SIZE+2];

/*
 * The lits of devices.
 */
//static bdmusb_dev    *usb_devs;
static libusb_device **usb_libusb_devs;

/*static int            tblcf_dev_count;
static int            pe_dev_count;
static int            osbdm_dev_count;
static int           usb_dev_count;
*/
/* provides low level USB functions which talk to the hardware */

/* initialisation */
unsigned char bdmusb_init(void) {
  libusb_init(NULL);          /* init LIBUSB */
  libusb_set_debug(NULL, 0);    /* set debug level to minimum */
  bdmusb_find_supported_devices();
  return usb_dev_count;
}

/* find all TBLCF devices attached to the computer */
void bdmusb_find_supported_devices(void) {
  libusb_device *dev;
  int           i = 0;
  int           count;
  
  if (usb_devs) return;
  
  count = libusb_get_device_list(NULL, &usb_libusb_devs);
  if (count < 0)
      return;
  
  tblcf_dev_count = 0;
  pe_dev_count = 0;
  osbdm_dev_count = 0;
  
  /* scan through all busses then devices counting the number found */
  while ((dev = usb_libusb_devs[i++]) != NULL) {
     libusb_device *dev = usb_libusb_devs[i];
      struct libusb_device_descriptor desc;
      int r = libusb_get_device_descriptor(dev, &desc);
      if (r < 0) {
          fprintf(stderr, "warning: failed to get usb device descriptor");
      } else {
          if ( ( (desc.idVendor == TBLCF_VID) && (desc.idProduct == TBLCF_PID) ) ||
	    ( (desc.idVendor == TBLCF_VID) && (desc.idProduct == TBDML_PID) ) ||
	    ( (desc.idVendor == PEMICRO_VID) && (desc.idProduct == MLCFE_PID) ) ||
	    ( (desc.idVendor == PEMICRO_VID) && (desc.idProduct == DEMOJM_PID) ) ||
	    ( (desc.idVendor == OSBDM_VID) && (desc.idProduct == OSBDM_PID) ) ) {
	      usb_dev_count++;
      }
  }
  
  usb_devs = calloc (usb_dev_count, sizeof (bdmusb_dev));
  if (!usb_devs) return;
  usb_dev_count = 0;
  
  /* scan through all busses and devices adding each one */
  i = 0;
  while ((dev = usb_libusb_devs[i++]) != NULL) {  
      bdmusb_dev *udev = &usb_devs[usb_dev_count];
      int r = libusb_get_device_descriptor(dev, &udev->desc);
      if (r >= 0) {
	  unsigned char device_found = 0;
	  switch (desc.idVendor) {
	      case TBLCF_VID:
		  //if ( (desc.idProduct==TBLCF_PID) || 
		  //  (desc.idProduct==TBDML_PID) ){
		  if (desc.idProduct==TBLCF_PID) {
		      udev->type = P_TBLCF;
		      /* found a device */
		      tblcf_dev_count++;
		      device_found = 1;
		  }
		  break;
	      case PEMICRO_VID:
		  //if ( (desc.idProduct==MLCFE_PID) || 
		  //  (desc.idProduct==DEMOJM_PID) ){
		  udev->type = P_TBLCF;
		  pe_dev_count++;
		  device_found = 1;
		  break;
	      case OSBDM_VID:
		  if (desc.idProduct==OSBDM_PID) {
		      udev->type = P_OSBDM;
		      
		      osbdm_dev_count++;
		  }
		  device_found = 1;
		  break;
	  }
	  if (device_found) {
              /* found a device */
	      udev->dev_ref = usb_dev_count;
              udev->device = dev;
              udev->bus_number = libusb_get_bus_number(dev);
              udev->device_address = libusb_get_device_address(dev);
              snprintf(udev->name, sizeof(udev->name), "%03d-%03d", udev->bus_number, udev->device_address);
              usb_dev_count++;
          }
      }
  }
}

/* gets version of the interface (HW and SW) in BCD format */
unsigned int bdmusb_get_version(int dev) {
	usb_data[0]=3;	/* get 3 bytes */
	usb_data[1]=CMD_GET_VER;
	bdm_usb_recv_ep0(&usb_devs[dev], usb_data);
	bdm_print("BDMUSB_GET_VERSION: Reported cable version (0x%02X) - %02X.%02X (HW.SW)\r\n",
              usb_data[0],usb_data[2],usb_data[1]);
	return((*((unsigned int *)(usb_data+1)))&0xffff);
}

void bdmusb_dev_name(int dev, char *name, int namelen) {
  if (usb_devs && (dev < tblcf_usb_cnt ()))
    strncpy (name, usb_devs[dev].name, namelen - 1);
  else
    strncpy (name, "invalid device", namelen - 1);
  name[namelen - 1] = '\0';
}