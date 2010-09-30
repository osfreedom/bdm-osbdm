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

#ifndef _BDMUSB_LOW_LEVEL_H_
#define _BDMUSB_LOW_LEVEL_H_

int bdmusb_usb_dev_open(int dev);
/* opens a device with given number (0...), returns 0 on success and 1 on error */
int bdmusb_usb_open(const char *device);

void bdmusb_usb_close(int dev);

/* requests bootloader execution on new power-up, returns 0 on success and
 * non-zero on failure */
unsigned char bdmusb_request_boot(int dev);


unsigned char bdm_usb_recv_ep0(bdmusb_dev *dev, unsigned char * data);
unsigned char bdm_usb_send_ep0(bdmusb_dev *dev, unsigned char * data);

int bdm_usb_transaction(int dev, unsigned int txSize, unsigned int rxSize, unsigned char *data);
#endif /* _BDMUSB_LOW_LEVEL_H_ */