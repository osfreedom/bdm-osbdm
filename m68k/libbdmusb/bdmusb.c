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
#include "bdmusb_low_level.h"

#include "commands.h"

unsigned int tblcf_dev_count;
unsigned int pe_dev_count;
unsigned int osbdm_dev_count;
unsigned int usb_dev_count;
bdmusb_dev    *usb_devs;

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

/* opens a device with given number name */
/* returns device number on success and -1 on error */
/*int bdmusb_open(const char *device) {
        bdm_print("TBLCF_OPEN: Trying to open device %s\r\n", device);
        return(bdmusb_usb_open(device));
}
*/
/* closes currently open device */
/*void bdmusb_close(int dev) {
        bdm_print("TBLCF_CLOSE: Trying to close the device\r\n");
        bdmusb_usb_close(dev);
}
*/
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
  while ( ((dev = usb_libusb_devs[i++]) != NULL) && (i < count) ) {
      dev = usb_libusb_devs[i];
      struct libusb_device_descriptor desc;
      int r = libusb_get_device_descriptor(dev, &desc);
      if (r < 0) {
          fprintf(stderr, "warning: failed to get usb device descriptor");
      } else {
          if ( ( (desc.idVendor == TBLCF_VID) && (desc.idProduct == TBLCF_PID) ) ||
	    ( (desc.idVendor == TBLCF_VID) && (desc.idProduct == TBDML_PID) ) ||
	    ( (desc.idVendor == PEMICRO_VID) && (desc.idProduct == MLCFE_PID) ) ||
	    ( (desc.idVendor == PEMICRO_VID) && (desc.idProduct == DEMOJM_PID) ) ||
	    ( (desc.idVendor == OSBDM_VID) && (desc.idProduct == OSBDM_PID) ) )
	      usb_dev_count++;
      }
  }
  
  usb_devs = calloc (usb_dev_count, sizeof (bdmusb_dev));
  if (!usb_devs) return;
  usb_dev_count = 0;
  
  /* scan through all busses and devices adding each one */
  i = 0;
  while ( ((dev = usb_libusb_devs[i++]) != NULL) && (i < count) ) {
      dev = usb_libusb_devs[i];
      bdmusb_dev *udev = &usb_devs[usb_dev_count];
      int r = libusb_get_device_descriptor(dev, &udev->desc);
      if (r >= 0) {
	  unsigned char device_found = 0;
	  switch (udev->desc.idVendor) {
	      case TBLCF_VID:
		  //if ( (desc.idProduct==TBLCF_PID) || 
		  //  (desc.idProduct==TBDML_PID) ){
		  if (udev->desc.idProduct==TBLCF_PID) {
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
		  if (udev->desc.idProduct==OSBDM_PID) {
		      udev->type = P_OSBDM;
		      osbdm_dev_count++;
		      device_found = 1;
		  }
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
	      
	      /* Check the USBDM/OSBDM version */
	      if ( (udev->desc.idVendor == OSBDM_VID) && (udev->desc.idProduct==OSBDM_PID) ) {
		  usbmd_version_t usbdm_version;
		  bdmusb_usb_open(udev->name);
		  bdmusb_get_version(udev, &usbdm_version);
		  
		  /* Know what version we have*/
		  if (usbdm_version.bdm_soft_ver <= 0x10)
		      udev->type = P_OSBDM;
		  else if (usbdm_version.bdm_soft_ver <= 0x15)
		      udev->type = P_USBDM;
		  else
		      udev->type = P_USBDM_V2;
		  
	      }
          }
      }
  }
}

/* gets version of the interface (HW and SW) in BCD format */
unsigned char bdmusb_get_version(bdmusb_dev* dev, usbmd_version_t* version) {
    static const usbmd_version_t defaultVersion = {0,0,0,0};
    unsigned char return_value;
    if (dev->type==P_TBLCF) {
	usb_data[0]=3;	/* get 3 bytes */
	usb_data[1]=CMD_TBLCF_GET_VER;
	usb_data[2]=0;
    } else {
	usb_data[0]=5;	/* get 3 bytes */
	usb_data[1]=CMD_USBDM_GET_VER;
	usb_data[2]=0;
	usb_data[3]=0;
	usb_data[4]=0;
    }
    return_value = bdm_usb_recv_ep0(dev, usb_data);
    if (return_value == BDM_RC_OK)
      *version = *((usbmd_version_t *)(&usb_data[1]));
    else
      *version = defaultVersion;
    
    if (dev->type==P_TBLCF) {
      bdm_print("BDMUSB_GET_VERSION: Reported cable version (0x%02X) - %02X.%02X (SW.HW)\r\n",
	  usb_data[0], version->bdm_soft_ver, version->bdm_hw_ver);
    } else {
      bdm_print("BDMUSB_GET_VERSION: Reported cable version (0x%02X):\r\n"
	"       BDM S/W version = %1X.%1X, H/W version (from BDM) = %1X.%X\r\n"
        "       ICP S/W version = %1X.%1X, H/W version (from ICP) = %1X.%X\r\n",
	usb_data[0],
	(version->bdm_soft_ver >> 4) & 0x0F, version->bdm_soft_ver & 0x0F,
        (version->bdm_hw_ver >> 6) & 0x03, version->bdm_hw_ver & 0x3F,
        (version->icp_soft_ver >> 4) & 0x0F, version->icp_soft_ver & 0x0F,
        (version->icp_hw_ver >> 6) & 0x03, version->icp_hw_ver & 0x3F );

    }
    
    //return((*((unsigned int *)(usb_data+1)))&0xffff);
    return return_value;
}

void bdmusb_dev_name(int dev, char *name, int namelen) {
  if (usb_devs && (dev < bdmusb_usb_cnt ())) {
    if (usb_devs && (dev < tblcf_usb_cnt ()))
      strncpy (name, usb_devs[dev].name, namelen - 1);
    else if (usb_devs && (dev < pe_usb_cnt ()))
      strncpy (name, usb_devs[dev].name, namelen - 1);
    else if (usb_devs && (dev < osbdm_usb_cnt ()))
      strncpy (name, usb_devs[dev].name, namelen - 1);
  } else
    strncpy (name, "invalid device", namelen - 1);
  name[namelen - 1] = '\0';
  
}

/* returns status of the last command: 0 on sucess and non-zero on failure */
unsigned char bdmusb_get_last_sts_value(int dev) {
	usb_data[0]=1;	 /* get 1 byte */
	usb_data[1]=(usb_devs[dev].type==P_TBLCF)?CMD_TBLCF_GET_LAST_STATUS:CMD_GET_LAST_STATUS;
	bdm_usb_recv_ep0(&usb_devs[dev], usb_data);
	bdm_print("BDMUSB_GET_LAST_STATUS: Reported last command status 0x%02X\r\n",usb_data[0]);
  return usb_data[0];
}

/* sets target MCU type */
/* returns 0 on success and non-zero on failure */
unsigned char bdmusb_set_target_type(int dev, target_type_e target_type) {
    unsigned char ret_val;
    usb_data[0]=1;	 /* get 1 byte */
    usb_data[1]=(usb_devs[dev].type==P_TBLCF)?CMD_TBLCF_SET_TARGET:CMD_USBDM_SET_TARGET;
    usb_data[2]=target_type;
    ret_val = tblcf_usb_recv_ep0(dev, usb_data);
    if (ret_val == BDM_RC_OK) {
      bdm_print("BDMUSB_SET_TARGET_TYPE: Set target type 0x%02X (0x%02X)\r\n",usb_data[2],usb_data[0]);
      if (usb_devs[dev].type==P_TBLCF) 
	ret_val = !(usb_data[0]==CMD_TBLCF_SET_TARGET);
    } else 
      bdm_print("BDMUSB_SET_TARGET_TYPE - Error %d \r\n",ret_val);
    
    return(ret_val);
}
