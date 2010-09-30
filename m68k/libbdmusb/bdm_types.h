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

/** Capabilities of the hardware */
typedef enum  {
   BDM_CAP_NONE             = (0),
   BDM_CAP_RESET            = (1<<0),   /**< - RESET can be driven/sensed (HC12 support) */
   BDM_CAP_FLASH            = (1<<1),   /**< - 12 V Flash programming supply available (RS08 support) */
   BDM_CAP_VDDCONTROL       = (1<<2),   /**< - Control over target Vdd */
   BDM_CAP_VDDSENSE         = (1<<3),   /**< - Sensing of target Vdd */
   BDM_CAP_CFVx             = (1<<4),   /**< - Support for CFV 1,2 & 3 */
} target_hw_cap_type_e;


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

/** USBDM interface options */
typedef struct {
   // Options passed to the BDM
   int  targetVdd;                /**< - Target Vdd (off, 3.3V or 5V) */
   int  cycleVddOnReset;          /**< - Cycle target Power  when resetting */
   int  cycleVddOnConnect;        /**< - Cycle target Power if connection problems) */
   int  leaveTargetPowered;       /**< - Leave target power on exit */
   int  autoReconnect;            /**< - Automatically re-connect to target (for speed change) */
   int  guessSpeed;               /**< - Guess speed for target w/o ACKN */
   int  useAltBDMClock;           /**< - Use alternative BDM clock source in target */
   int  useResetSignal;           /**< - Whether to use RESET signal on BDM interface */
   int  reserved1[3];             /**< - Reserved */

   // Options used internally by DLL
   int  manuallyCycleVdd;         /**< - Prompt user to manually cycle Vdd on connection problems */
   int  derivative_type;          /**< - RS08 Derivative */
   int  targetClockFreq;          /**< - RS08 - Clock sync value to trim to (0 indicates no trim value supplied).\n */
                                  /**< - CFVx - Target clock frequency. \n */
                                  /**< - Other targets automatically determine clock frequency. */
   int  miscOptions;              /**< - Various misc options */
   int  usePSTSignals;            /**< - CFVx, PST Signal monitors */
   int  reserved2[2];             /**< - Reserved */
} usbdm_options_type_e;

/** Structure to hold version information for BDM
  * @note bdmHardwareVersion should always equal icpHardwareVersion
  */
typedef struct {
   unsigned char bdm_soft_ver; /**< Version of USBDM Firmware */
   unsigned char bdm_hw_ver; /**< Version of USBDM Hardware */
   unsigned char icp_soft_ver; /**< Version of ICP bootloader Firmware */
   unsigned char icp_hw_ver; /**< Version of Hardware (reported by ICP code) */
} usbmd_version_t;

/*
 * Manage the USB devices we have connected.
 */
typedef struct bdmusb_dev_s {
  int                             dev_ref;
  int                             use_only_ep0; //Backward compatibility mode for USBDM JS16 models
  pod_type_e                      type;
  libusb_device                   *device;
  libusb_device_handle            *handle;
  struct libusb_device_descriptor desc;
  uint8_t                         bus_number;
  uint8_t                         device_address;
  char                            name[16]; /* 0000-0000 is 4 + 1 4 + 1 = 10 */
  /* USBDM specific */
  target_hw_cap_type_e            hw_cap;
  target_type_e                   target;
  usbdm_options_type_e            options;
} bdmusb_dev;


#define WRITE_BLOCK_CHECK /* Keep on. CCJ */


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

/** Target supports ACKN or uses fixed delay {WAIT} instead
  */
typedef enum {
  ACKN  = 1,   /**< - Target supports ACKN feature and it is enabled */
  WAIT  = 0,   /**< - Use WAIT (delay) instead */
} ack_mode_type_e;

/** Target speed selection
  */
typedef enum {
  SPEED_NO_INFO        = 0,   /**< - Not connected */
  SPEED_SYNC           = 1,   /**< - Speed determined by SYNC */
  SPEED_GUESSED        = 2,   /**< - Speed determined by trial & error */
  SPEED_USER_SUPPLIED  = 3    /**< - User has specified the speed to use */
} speed_mode_type_e;

/** target reset detection state */
typedef enum {	
	RESET_NOT_DETECTED=0,
	NO_RESET_ACTIVITY=RESET_NOT_DETECTED,
	RESET_DETECTED=1
} reset_detection_e;

/** target reset state */
typedef enum {	
	RSTO_ACTIVE=0,
	RSTO_INACTIVE=1
} reset_state_e;

/** Target Halt state
  */
typedef enum {
   TARGET_RUNNING    = 0,   /**< - CFVx target running (ALLPST == 0) */
   TARGET_HALTED     = 1    /**< - CFVx target halted (ALLPST == 1) */
} target_run_state_type_e;

/** Target Voltage supply state
  */
typedef enum  {
   BDM_TARGET_VDD_NONE      = 0,   /**< - Target Vdd not detected */
   BDM_TARGET_VDD_EXT       = 1,   /**< - Target Vdd external */
   BDM_TARGET_VDD_INT       = 2,   /**< - Target Vdd internal */
   BDM_TARGET_VDD_ERR       = 3,   /**< - Target Vdd error */
} target_vdd_state_type_e;

/** Internal Target Voltage supply selection
  */
typedef enum  {
   BDM_TARGET_VDD_OFF       = 0,   /**< - Target Vdd Off */
   BDM_TARGET_VDD_3V3       = 1,   /**< - Target Vdd internal 3.3V */
   BDM_TARGET_VDD_5V        = 2,   /**< - Target Vdd internal 5.0V */
} target_vdd_select_type_e;


/** Internal Target Voltage supply selection
  */
typedef enum  {
   BDM_TARGET_VPP_OFF       = 0,   /**< - Target Vpp Off */
   BDM_TARGET_VPP_STANDBY   = 1,   /**< - Target Vpp Standby (Inverter on, Vpp off) */
   BDM_TARGET_VPP_ON        = 2,   /**< - Target Vpp On */
   BDM_TARGET_VPP_ERROR     = 3,   /**< - Target Vpp ?? */
} target_vpp_select_type_e;

/** state of BDM communication */
typedef struct { 
  target_type_e             target_type;       /**< - Type of target (HCS12, HCS08 etc) */
  ack_mode_type_e           ackn_state;        /**< - Supports ACKN ? */
  speed_mode_type_e         connection_state;  /**< - Connection status & speed determination method */
  reset_state_e             reset_state;       /**< - Current target RST0 state */
  reset_detection_e         reset_recent;      /**< - Target reset recently? */
  target_run_state_type_e   halt_state;        /**< - CFVx halted (from ALLPST)? */
  target_vdd_state_type_e   power_state;       /**< - Target has power? */
  target_vpp_select_type_e  flash_state;       /**< - State of Target Vpp */
} bdm_status_t;

#endif /* _BDM_TYPES_H_ */