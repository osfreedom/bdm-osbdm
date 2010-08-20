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
#include <stdio.h>
#include "log.h"
#include "bdmusb-hwdesc.h"
#include "bdmusb.h"
#include "bdmusb_low_level.h"

#include "tblcf/tblcf.h"
#include "tblcf/tblcf_usb.h"

#include "commands.h"

#define bdmusb_print(val) fprintf(stderr, val)

/* sends a message to the TBDML device over EP0 */
/* returns 0 on success and 1 on error */
/* since the EP0 transfer is unidirectional in this case, data returned by the
 * device must be read separately */
unsigned char bdm_usb_send_ep0(bdmusb_dev *dev, unsigned char * data) {
  unsigned char * count = data;    /* data count is the first byte of the message */
  int i;
  if (!tblcf_usb_dev_open(dev->dev_ref)) {
    bdmusb_print("USB EP0 send: device not open\n");
    return(1);
  }
  bdmusb_print("USB EP0 send:\n");
  bdm_print_dump(data,(*count)+1);
  i=libusb_control_transfer(dev->handle, 0x40, *(data+1), (*(data+2))+256*(*(data+3)),
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
  if (!tblcf_usb_dev_open(dev->dev_ref)) {
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
