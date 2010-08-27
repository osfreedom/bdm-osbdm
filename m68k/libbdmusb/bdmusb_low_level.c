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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "log.h"
#include "bdmusb-hwdesc.h"
#include "bdmusb.h"
#include "bdmusb_low_level.h"

#include "commands.h"

#include "libusb-1.0/libusb.h"

#include "tblcf/tblcf.h"
#include "tblcf/tblcf_usb.h"

#define bdmusb_print(val) bdm_print(val)

static unsigned char usb_data[MAX_DATA_SIZE+2];

int bdmusb_usb_dev_open(int dev) {
  return (dev < bdmusb_usb_cnt ()) && usb_devs[dev].handle;
}

/* sends a message to the TBDML device over EP0 */
/* returns 0 on success and 1 on error */
/* since the EP0 transfer is unidirectional in this case, data returned by the
 * device must be read separately */
unsigned char bdm_usb_send_ep0(bdmusb_dev *dev, unsigned char * data) {
  unsigned char * count = data;    /* data count is the first byte of the message */
  int i;
  if (!bdmusb_usb_dev_open(dev->dev_ref)) {
    bdmusb_print("USB EP0 send: device not open\n");
    return(1);
  }
  bdmusb_print("USB EP0 send:\n");
  bdm_print_dump(data,(*count)+1);
  i=libusb_control_transfer(usb_devs[dev->dev_ref].handle, 0x40, *(data+1), (*(data+2))+256*(*(data+3)),
                            (*(data+4))+256*(*(data+5)), data+6,
                            (uint16_t)(((*count)>5)?((*count)-5):0), TIMEOUT);
  if (i<0) return(1); else return(0);
}

/* sends a message to the TBDML device over EP0 which instruct the device to
 * exec command and return data */
/* returns 0 on success and 1 on error */
/* data count in the message is number of bytes EXPECTED/REQUIRED from the device */
/* the device will get the cmd number and 4 following data bytes */
unsigned char bdm_usb_recv_ep0(bdmusb_dev *dev, unsigned char * data) {
  unsigned char count = *data;    /* data count is the first byte of the message */
  int i;
  if (!bdmusb_usb_dev_open(dev->dev_ref)) {
    bdmusb_print("USB EP0 receive request: device not open\n");
    return(1);
  }
  bdmusb_print("USB EP0 receive request:\n");
  bdm_print_dump(data,6);
  i=libusb_control_transfer(dev->handle, 0xC0, *(data+1), (*(data+2))+256*(*(data+3)),
                            (*(data+4))+256*(*(data+5)), data, count, TIMEOUT);
  bdmusb_print("USB EP0 receive:\n");
  bdm_print_dump(data,count);
  if (i<0) return(1); else return(0);
}

/* open connection to device enumerated by tblcf_usb_find_devices */
/* returns the device number on success and -1 on error */
int bdmusb_usb_open(const char *device) {
  int dev;
  int return_val;
  for (dev = 0; dev < usb_dev_count; dev++)
    if (strcmp (device, usb_devs[dev].name) == 0)
      break;
  
  if (dev == usb_dev_count) {
    bdm_print("USB Device '%s' not found\n", device);
    return -1;
  }
  
  if (usb_devs[dev].handle) {
    bdm_print("USB Device '%s' alread open\n", device);
    return -1;
  }

  return_val = libusb_open(usb_devs[dev].device, &usb_devs[dev].handle);
  if (return_val) { 
    bdm_print("bdm_usb_open: Failed to open device\n");
    return -1;
  }
  bdm_print("USB Device open\n");
  /* USBDM/TBLCF has only one valid configuration */
  return_val = libusb_set_configuration(usb_devs[dev].handle,1);
  if (return_val) {
    bdm_print("bdm_usb_open: Failed set configuration, rc= %d\n", return_val);
    return -1;
  }
  bdm_print("USB Configuration set\n");
  /* TBLCF has only 1 interface */
  return_val = libusb_claim_interface(usb_devs[dev].handle,0);
  if (return_val) {
    bdm_print("bdm_usb_open: Failed to claim interface, rc= %d\n", return_val);
    return -1;
  }
  bdm_print("USB Interface claimed\n");
  return dev;
}

/* closes connection to the currently open device */
void bdmusb_usb_close(int dev) {
  if (bdmusb_usb_dev_open(dev)) {
    libusb_set_configuration(usb_devs[dev].handle,0);   // Un-set the configuration
    /* release the interface */
    libusb_release_interface(usb_devs[dev].handle,0);
    bdm_print("USB Interface released\n");
    /* close the device */
    libusb_close(usb_devs[dev].handle);
    bdm_print("USB Device closed\n");
    /* indicate that no device is open */
    usb_devs[dev].handle=NULL;
  }
}

/* requests bootloader execution on next power-up */
/* returns 0 on success and non-zero on failure */
unsigned char bdmusb_request_boot(int dev) {
	if (usb_devs[dev].type==P_TBLCF) {
	    usb_data[0]=1;			  	/* return 1 byte */
	    usb_data[1]=CMD_TBLCF_SET_BOOT;
	} else {
	    usb_data[0]=5;			  	/* return 1 byte */
	    usb_data[1]=CMD_USBDM_ICP_BOOT;
	}
	usb_data[2]='B';
	usb_data[3]='O';
	usb_data[4]='O';
	usb_data[5]='T';
	bdm_usb_recv_ep0(&usb_devs[dev], usb_data);
	bdm_print("BDMUSB_REQUEST_BOOT: Requested bootloader on next power-up (0x%02X)\r\n",usb_data[0]);
	return(!(usb_data[0]==CMD_SET_BOOT));
}
