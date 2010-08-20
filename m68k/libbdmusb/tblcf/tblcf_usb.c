/*
    Turbo BDM Light ColdFire
    Copyright (C) 2006  Daniel Malik

    Changed to support the BDM project.
    Chris Johns (cjohns@user.sourgeforge.net)
    
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
#include <unistd.h>

#include "log.h"
#include "bdm-usb.h"
#include "tblcf_usb.h"
#include "tblcf_hwdesc.h"
#include "libusb-1.0/libusb.h"

#include "bdmusb.h"
#include "bdmusb-hwdesc.h"


int tblcf_usb_dev_open(int dev) {
  return (dev < tblcf_usb_cnt ()) && usb_devs[dev].handle;
}
  
/* open connection to device enumerated by tblcf_usb_find_devices */
/* returns the device number on success and -1 on error */
int tblcf_usb_open(const char *device) {
  int dev;
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

  if (libusb_open(usb_devs[dev].device, &usb_devs[dev].handle) != 0) return -1;
  bdm_print("USB Device open\n");
  /* TBLCF has only one valid configuration */
  if (libusb_set_configuration(usb_devs[dev].handle,1)) return -1;
  bdm_print("USB Configuration set\n");
  /* TBLCF has only 1 interface */
  if (libusb_claim_interface(usb_devs[dev].handle,0)) return -1;
  bdm_print("USB Interface claimed\n");
  return dev;
}

/* closes connection to the currently open device */
void tblcf_usb_close(int dev) {
  if (tblcf_usb_dev_open(dev)) {
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

/* Message data format:
        1 byte: size of cmd+data
          1 byte: cmd
        (size-1) bytes: data
*/

/* sends a message to the TBDML device over EP0 */
/* returns 0 on success and 1 on error */
/* since the EP0 transfer is unidirectional in this case, data returned by the
 * device must be read separately */
unsigned char tblcf_usb_send_ep0(int dev, unsigned char * data) {
  unsigned char * count = data;    /* data count is the first byte of the message */
  int i;
  if (!tblcf_usb_dev_open(dev)) {
    bdm_print("USB EP0 send: device not open\n");
    return(1);
  }
  bdm_print("USB EP0 send:\n");
  bdm_print_dump(data,(*count)+1);
  i=libusb_control_transfer(usb_devs[dev].handle, 0x40, *(data+1), (*(data+2))+256*(*(data+3)),
                            (*(data+4))+256*(*(data+5)), data+6,
                            (uint16_t)(((*count)>5)?((*count)-5):0), TIMEOUT);
  if (i<0) return(1); else return(0);
}

/* sends a message to the TBDML device over EP0 which instruct the device to
 * exec command and return data */
/* returns 0 on success and 1 on error */
/* data count in the message is number of bytes EXPECTED/REQUIRED from the device */
/* the device will get the cmd number and 4 following data bytes */
unsigned char tblcf_usb_recv_ep0(int dev, unsigned char * data) {
  unsigned char count = *data;    /* data count is the first byte of the message */
   int i;
  if (!tblcf_usb_dev_open(dev)) {
    bdm_print("USB EP0 receive request: device not open\n");
    return(1);
  }
  bdm_print("USB EP0 receive request:\n");
  bdm_print_dump(data,6);
  i=libusb_control_transfer(usb_devs[dev].handle, 0xC0, *(data+1), (*(data+2))+256*(*(data+3)),
                            (*(data+4))+256*(*(data+5)), data, count, TIMEOUT);
  bdm_print("USB EP0 receive:\n");
  bdm_print_dump(data,count);
  if (i<0) return(1); else return(0);
}

/* sends a message to the TBDML device over EP2 */
/* returns 0 on success and 1 on error */
/* data the device wants to return need to be read out in separate recv transaction */
unsigned char tblcf_usb_send_ep2(int dev, unsigned char *data) {
  unsigned char * count = data;    /* data count is the first byte of the message */
  int actual_length = 0;
  int i;
  if (!tblcf_usb_dev_open(dev)) {
    bdm_print("USB EP2 send: device not open\n");
    return(1);
  }
  bdm_print("USB EP2 send:\n");
  bdm_print_dump(data,(*count)+1);
  i=libusb_bulk_transfer(usb_devs[dev].handle, LIBUSB_ENDPOINT_OUT | 2, data, (*count)+1,
                         &actual_length, TIMEOUT);
  if (i<0) return(1); else return(0);
}

/* receives data from EP2 */
/* returns 0 on success and 1 on error */
/* data count in the message is number of bytes EXPECTED/REQUIRED from the device */
unsigned char tblcf_usb_recv_ep2(int dev, unsigned char * data) {
  unsigned char count = *data;    /* data count is the first byte of the message */
  int actual_length = 0;
  int i;
  if (!tblcf_usb_dev_open(dev)) {
    bdm_print("USB EP2 receive request: device not open\n");
    return(1);
  }
  bdm_print("USB EP2 receive request:\n");
  bdm_print_dump(data,6);
  i=libusb_bulk_transfer(usb_devs[dev].handle, LIBUSB_ENDPOINT_IN | 2, data, count,
                         &actual_length, TIMEOUT);
  bdm_print("USB EP2 receive (%d byte(s)):\n",i);
  bdm_print_dump(data,count);
  if (i<0) return(1); else return(0);
}

/* routines to interact with the ICP code on JB16 */

/* programs given number of bytes into flash of the device from given address */
/* returns 0 on succes, -1 on USB error and 1 on compare error */
char icp_program(int dev, unsigned char * data, unsigned int address, unsigned int count) {
  int i;
  unsigned char result;
  if (!tblcf_usb_dev_open(dev)) {
    bdm_print("ICP program: device not open\n");
    return(-1);
  }
  while (count>ICP_MAX_PACKET_SIZE) {
    bdm_print("ICP program %d byte from address 0x%04X:\n",ICP_MAX_PACKET_SIZE,address);
    bdm_print_dump(data,ICP_MAX_PACKET_SIZE);
    i=libusb_control_transfer(usb_devs[dev].handle, 0x40, ICP_PROGRAM, address,
                              address+ICP_MAX_PACKET_SIZE-1, data, ICP_MAX_PACKET_SIZE, TIMEOUT);
    if (i<0) {
      bdm_print("ICP program: request failed\n");
      return(-1);
    }
    usleep(10 * 1000 * 1000);  /* give the part plenty of time to do the programming */
    do {
      i=libusb_control_transfer(usb_devs[dev].handle, 0xC0, ICP_GET_RESULT,
                                0, 0, &result, 1, TIMEOUT);
      if (i<0) {
        bdm_print("ICP program: get result request failed\n");
        return(-1);
      }
    } while (result==ICP_RESULT_BUSY);  /* keep checking the result while busy */
    if (result!=ICP_RESULT_OK) {
      bdm_print("ICP programming error (%02X)\n",result);
      return(1);
    }
    count-=ICP_MAX_PACKET_SIZE;
    address+=ICP_MAX_PACKET_SIZE;
    data+=ICP_MAX_PACKET_SIZE;
  }
  if (count) {
    bdm_print("ICP program %d byte(s) from address 0x%04X:\n",count,address);
    bdm_print_dump(data,count);
    i=libusb_control_transfer(usb_devs[dev].handle, 0x40, ICP_PROGRAM, address, address+count-1,
                              data, count, TIMEOUT);
    if (i<0) {
      bdm_print("ICP program: request failed\n");
      return(-1);
    }
    usleep(10 * 1000 * 1000);  /* give the part plenty of time to do the programming */
    do {
      i=libusb_control_transfer(usb_devs[dev].handle, 0xC0, ICP_GET_RESULT,
                                0, 0, &result, 1, TIMEOUT);
      if (i<0) {
        bdm_print("ICP program: get result request failed\n");
        return(-1);
      }
    } while (result==ICP_RESULT_BUSY);  /* keep checking the result while busy */
    if (result!=ICP_RESULT_OK) {
      bdm_print("ICP programming error (%02X)\n",result);
      return(1);
    }
  }
  return(0);
}

/* performs mass erase of device flash */
/* returns 0 on success , -1 on USB error and 1 on operation error */
char icp_mass_erase(int dev) {
  int i;
  unsigned char result;
  if (!tblcf_usb_dev_open(dev)) {
    bdm_print("ICP mass erase: device not open\n");
    return(-1);
  }
  bdm_print("ICP mass erase\n");
  i=libusb_control_transfer(usb_devs[dev].handle, 0x40, ICP_MASS_ERASE, 0, 0, NULL, 0, TIMEOUT);
  if (i<0) {
    bdm_print("ICP mass erase request failed\n");
    return(-1);
  }
  /* wait 250ms (time of mass erase) + lots of extra to make sure the operation
   * is finished by the time new traffic apears on the USB bus */
  usleep(400 * 1000);
  do {
    i=libusb_control_transfer(usb_devs[dev].handle, 0xC0, ICP_GET_RESULT,
                              0, 0, &result, 1, TIMEOUT);
    if (i<0) {
      bdm_print("ICP mass erase: get result request failed\n");
      return(-1);
    }
  } while (result==ICP_RESULT_BUSY);  /* keep checking the result while busy */
  if (result!=ICP_RESULT_OK) {
    bdm_print("ICP mass erase error (0x%02X)\n",result);
    return(1);
  }
  return(0);
}

/* performs block erase on block containing given address */
/* returns 0 on success, -1 on USB error and 1 on operation error */
char icp_block_erase(int dev, unsigned int address) {
  int i;
  unsigned char result;
  unsigned char blank_test_array[ICP_FLASH_BLOCK_SIZE];
  if (!tblcf_usb_dev_open(dev)) {
    bdm_print("ICP block erase: device not open\n");
    return(-1);
  }
  /* make sure the address is only 16 bit and align it to the closest lower
   * block boundary */
  address&=(65536-ICP_FLASH_BLOCK_SIZE);
  bdm_print("ICP block erase\n");
  for (i=0;i<ICP_FLASH_BLOCK_SIZE;i++) blank_test_array[i]=0xff;
  i=icp_verify(dev, blank_test_array, address, ICP_FLASH_BLOCK_SIZE);
  if (i<0) {
    bdm_print("ICP block erase: verify request failed\n");
    return(-1);
  }
  if (i==0) return(0);  /* block is already erased */
  /* sector is not blank, must perform block erase */
  bdm_print("ICP block erase: block not empty, erasing...\n");
  i=libusb_control_transfer(usb_devs[dev].handle, 0x40, ICP_BLOCK_ERASE,
                            address, address+ICP_FLASH_BLOCK_SIZE-1, NULL, 0, TIMEOUT);
  if (i<0) {
    bdm_print("ICP block erase request failed\n");
    return(-1);
  }
  /* wait 10ms (time of block erase) + lots of extra to make sure the operation
   * is finished by the time new traffic apears on the USB bus */
  usleep(30 * 1000);
  do {
    i=libusb_control_transfer(usb_devs[dev].handle, 0xC0, ICP_GET_RESULT,
                              0, 0, &result, 1, TIMEOUT);
    if (i<0) {
      bdm_print("ICP block erase: get result request failed\n");
      return(-1);
    }
  } while (result==ICP_RESULT_BUSY);  /* keep checking the result while busy */
  if (result!=ICP_RESULT_OK) {
    bdm_print("ICP block erase error (0x%02X)\n",result);
    return(1);
  }
  return(0);
}

/* verifies given number of bytes against contents of flash of the device from given address */
/* returns 0 on succesful compare, -1 on USB error and 1 on compare error */
char icp_verify(int dev, unsigned char * data, unsigned int address, unsigned int count) {
  int i;
  unsigned char result;
  if (!tblcf_usb_dev_open(dev)) {
    bdm_print("ICP verify: device not open\n");
    return(-1);
  }
  while (count>ICP_MAX_PACKET_SIZE) {
    bdm_print("ICP verify %d bytes from address 0x%04X:\n",ICP_MAX_PACKET_SIZE,address);
    bdm_print_dump(data,ICP_MAX_PACKET_SIZE);
    i=libusb_control_transfer(usb_devs[dev].handle, 0x40, ICP_VERIFY,
                              address, address+ICP_MAX_PACKET_SIZE-1, data,
                              ICP_MAX_PACKET_SIZE, TIMEOUT);
    if (i<0) {
      bdm_print("ICP verify request failed\n");
      return(-1);
    }
    /* wait to make sure the operation is finished by the time new traffic
     * apears on the USB bus */
    usleep(5 * 1000);
    do {
      i=libusb_control_transfer(usb_devs[dev].handle, 0xC0, ICP_GET_RESULT,
                                0, 0, &result, 1, TIMEOUT);
      if (i<0) {
        bdm_print("ICP verify: get result request failed\n");
        return(-1);
      }
    } while (result==ICP_RESULT_BUSY);  /* keep checking the result while busy */
    if (result!=ICP_RESULT_OK) {
      bdm_print("ICP verification error\n");
      return(1);
    }
    count-=ICP_MAX_PACKET_SIZE;
    address+=ICP_MAX_PACKET_SIZE;
    data+=ICP_MAX_PACKET_SIZE;
  }
  if (count) {
    bdm_print("ICP verify %d byte(s) from address 0x%04X:\n",count,address);
    bdm_print_dump(data,count);
    i=libusb_control_transfer(usb_devs[dev].handle, 0x40, ICP_VERIFY,
                              address, address+count-1, data, count, TIMEOUT);
    if (i<0) {
      bdm_print("ICP verify request failed\n");
      return(-1);
    }
    /* wait to make sure the operation is finished by the time new traffic
     * apears on the USB bus */
    usleep(5 * 1000);
    do {
      i=libusb_control_transfer(usb_devs[dev].handle, 0xC0, ICP_GET_RESULT,
                                0, 0, &result, 1, TIMEOUT);
      if (i<0) {
        bdm_print("ICP verify: get result request failed\n");
        return(-1);
      }
    } while (result==ICP_RESULT_BUSY);  /* keep checking the result while busy */
    if (result!=ICP_RESULT_OK) {
      bdm_print("ICP verification error\n");
      return(1);
    }
  }
  return(0);
}
