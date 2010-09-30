/*
    BDM USB common code 
    Copyright (C) 2010 Rafael Campos 
    Rafael Campos Las Heras <rafael@freedom.ind.br>

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

#ifndef _BDMUSB_H_
#define _BDMUSB_H_

#include "libusb-1.0/libusb.h"
#include "bdm_types.h"
#include "tblcf/tblcf.h"
#include "usbdm/usbdm.h"

/*static unsigned int tblcf_dev_count;
static unsigned int pe_dev_count;
static unsigned int osbdm_dev_count;
static unsigned int usb_dev_count;
static bdmusb_dev    *usb_devs;
*/
extern unsigned int tblcf_dev_count;
extern unsigned int pe_dev_count;
extern unsigned int osbdm_dev_count;
extern unsigned int usb_dev_count;
extern bdmusb_dev    *usb_devs;
/**
  * Macros to get the number of devices 
  */
#define tblcf_usb_cnt()		(tblcf_dev_count)
#define pe_usb_cnt()		(pe_dev_count)
#define osbdm_usb_cnt()		(osbdm_dev_count)
#define bdmusb_usb_cnt()	(usb_dev_count)

//void tblcf_usb_find_devices(unsigned short int product_id);
void bdmusb_find_supported_devices(void);
void bdmusb_dev_name(int dev, char *name, int namelen);

/* returns version of the DLL in BCD format */
unsigned char bdmusb_version(void);

/* initialises USB and returns number of devices found */
unsigned char bdmusb_init(void);

/* returns hardware & software version of the cable in BCD format - SW version
 * in lower byte and HW version in upper byte */
unsigned char bdmusb_get_version(bdmusb_dev* dev, usbmd_version_t* version);

/* returns status of the last command value */
unsigned char bdmusb_get_last_sts_value(int dev);

/* sets target MCU type; returns 0 on success and non-zero on failure */
unsigned char bdmusb_set_target_type(int dev, target_type_e target_type);

/* resets the target to normal or BDM mode; returns 0 on success and non-zero
 * on failure */
unsigned char bdmusb_target_reset(int dev, target_mode_e target_mode);

/* fills user supplied structure with current state of the BDM communication
 * channel; returns 0 on success and non-zero on failure */
unsigned char bdmusb_bdm_sts(int dev, bdm_status_t *bdm_status);

/* brings the target into BDM mode; returns 0 on success and non-zero on
 * failure */
unsigned char bdmusb_target_halt(int dev);

/* starts target execution from current PC address; returns 0 on success and
 * non-zero on failure */
unsigned char bdmusb_target_go(int dev);

/* steps over a single target instruction; returns 0 on success and non-zero on
 * failure */
unsigned char bdmusb_target_step(int dev);

/* reads control register at the specified address and writes its contents into
 * the supplied buffer; returns 0 on success and non-zero on failure */
unsigned char bdmusb_read_creg(int dev, unsigned int address, unsigned long int * result);

/* writes control register at the specified address; returns 0 on success and
 * non-zero on failure */
void bdmusb_write_creg(int dev, unsigned int address, unsigned long int value);

/* reads the specified debug register and writes its contents into the
 * supplied buffer; returns 0 on success and non-zero on failure */
unsigned char bdmusb_read_dreg(int dev, unsigned int dreg_index, unsigned long int * result);

/* writes specified debug register */
void bdmusb_write_dreg(int dev, unsigned int dreg_index, unsigned long int value);

/* reads the specified register and writes its contents into the supplied
 * buffer; returns 0 on success and non-zero on failure */
unsigned char bdmusb_read_reg(int dev, unsigned int reg_index, unsigned long int * result);

/* writes specified register */
void bdmusb_write_reg(int dev, unsigned int reg_index, unsigned long int value);


#endif /* _BDMUSB_H_ */
