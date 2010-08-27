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

#ifndef _BDM_TYPES_H_
#define _BDM_TYPES_H_

#include "libusb-1.0/libusb.h"

/** Type of USB BDM pod */
typedef enum {
	P_NONE     = 0,     /** - No USB POD */
	P_TBDML    = 1,     /** - TBDML */
	P_TBLCF	   = 2,     /** - TBLCF */
	P_OSBDM	   = 3,     /** - Original OSBDM */
	P_USBDM	   = 4,     /** - USBDM */
	P_USBDM_V2 = 5,     /** - USBDM v2 */
} pod_type_e;

/*
 * Manage the USB devices we have connected.
 */
typedef struct bdmusb_dev_s {
  int                             dev_ref;
  pod_type_e                      type;
  libusb_device                   *device;
  libusb_device_handle            *handle;
  struct libusb_device_descriptor desc;
  uint8_t                         bus_number;
  uint8_t                         device_address;
  char                            name[16]; /* 0000-0000 is 4 + 1 4 + 1 = 10 */
} bdmusb_dev;


#define WRITE_BLOCK_CHECK /* Keep on. CCJ */

/** Type of BDM target */
typedef enum {
	T_HC12     = 0,     /** - HC12 or HCS12 target */
	T_HCS08	   = 1,     /** - HCS08 target */
	T_RS08	   = 2,     /** - RS08 target */
	T_CFV1     = 3,     /** - Coldfire Version 1 target */
	T_CFVx     = 4,     /** - Coldfire Version 2,3,4 target */
	T_JTAG     = 5,     /** - JTAG target - TAP is set to \b RUN-TEST/IDLE */
	T_EZFLASH  = 6,     /** - EzPort Flash interface (SPI?) */
	T_OFF      = 0xFF,  /** - Turn off interface (no target) */
} target_type_e;


/** Type of reset action required */
typedef enum {
	RESET_MODE_MASK   = (3<<0), /**  Mask for reset mode (SPECIAL/NORMAL) */
	RESET_SPECIAL     = (0<<0), /**  - Special mode [BDM active, Target halted] */
	RESET_NORMAL      = (1<<0), /**  - Normal mode [usual reset, Target executes] */

	RESET_TYPE_MASK   = (3<<2), /**  Mask for reset type (Hardware/Software/Power) */
	RESET_ALL         = (0<<2), /**  Use all reset stategies as appropriate */
	RESET_HARDWARE    = (1<<2), /**  Use hardware RESET pin reset */
	RESET_SOFTWARE    = (2<<2), /**  Use software (BDM commands) reset */
	RESET_POWER       = (3<<2), /**  Cycle power */

	// Legacy methods
	SPECIAL_MODE = RESET_SPECIAL|RESET_ALL,  /**  - Special mode [BDM active, Target halted] */
	BDM_MODE     = RESET_SPECIAL|RESET_ALL,  /**  - Special mode [BDM active, Target halted, also know as BDM_MODE] */
	NORMAL_MODE  = RESET_NORMAL|RESET_ALL,   /**  - Normal mode [usual reset, Target executes] */
} target_mode_e;

/** target reset detection state */
typedef enum {	
	RESET_NOT_DETECTED=0,
	RESET_DETECTED=1
} reset_detection_e;

/** target reset state */
typedef enum {	
	RSTO_ACTIVE=0,
	RSTO_INACTIVE=1
} reset_state_e;

/** state of BDM communication */
typedef struct { 
	reset_state_e reset_state;
	reset_detection_e reset_detection;
} bdmcf_status_t;

#endif /* _BDM_TYPES_H_ */