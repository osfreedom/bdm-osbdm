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

#ifndef TBLCF_USB_H
#define TBLCF_USB_H

//unsigned int tblcf_usb_cnt(void);
int tblcf_usb_dev_open(int dev);
int tblcf_usb_open(const char *device);
void tblcf_usb_close(int dev);
unsigned char tblcf_usb_send_ep0(int dev, unsigned char * data);
unsigned char tblcf_usb_recv_ep0(int dev, unsigned char * data);
unsigned char tblcf_usb_send_ep2(int dev, unsigned char * data);
unsigned char tblcf_usb_recv_ep2(int dev, unsigned char * data);

/* in circuit programming */
#define ICP_MAX_PACKET_SIZE		64
#define ICP_VERIFY				0x87
#define ICP_MASS_ERASE			0x83
#define ICP_BLOCK_ERASE			0x82
#define ICP_GET_RESULT			0x8F
#define ICP_RESULT_OK			0x01
#define ICP_RESULT_BUSY			0x02
#define ICP_RESULT_FAIL			0x04
#define ICP_PROGRAM				0x81
#define ICP_FLASH_BLOCK_SIZE	512		/* must be power of 2 */


char icp_verify(int dev, unsigned char * data, unsigned int address, unsigned int count);
char icp_program(int dev, unsigned char * data, unsigned int address, unsigned int count);
char icp_mass_erase(int dev);
char icp_block_erase(int dev, unsigned int address);

#endif
