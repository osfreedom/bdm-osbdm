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
        bdm_print("BDMUSB_OPEN: Trying to open device %s\r\n", device);
        return(bdmusb_usb_open(device));
}
*/
/* closes currently open device */
void bdmusb_close(int dev) {
    bdm_print("BDMUSB_CLOSE: Trying to close the device\r\n");
    if (usb_devs[dev].type == P_USBDM_V2)
      bdmusb_set_target_type(dev, T_OFF);
    bdmusb_usb_close(dev);
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
		  //udev->type = P_TBLCF;
		  //pe_dev_count++;
		  //device_found = 1;
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
		  pod_type_e old_type;
		  udev->dev_ref = bdmusb_usb_open(udev->name);
		  /* Try first with the USBDM. */
		  old_type = udev->type;
		  udev->type = P_USBDM;
		  bdmusb_get_version(udev, &usbdm_version);
		  
		  /* Know what version we have*/
		  if (usbdm_version.bdm_soft_ver <= 0x10)
		      udev->type = P_OSBDM;
		  else if (usbdm_version.bdm_soft_ver <= 0x15)
		      udev->type = P_USBDM;
		  else
		      udev->type = P_USBDM_V2;
		  
		  bdmusb_usb_close(udev->dev_ref);
	      }
          }
      }
  }
}

/* gets version of the interface (HW and SW) in BCD format */
unsigned char bdmusb_get_version(bdmusb_dev* dev, usbmd_version_t* version) {
    static const usbmd_version_t defaultVersion = {0,0,0,0};
    unsigned char return_value;
    
    switch (dev->type) {
      case P_OSBDM:
	usb_data[0]=3;	/* get 3 bytes */
	usb_data[1]=CMD_OSBDM_GET_VER;
	usb_data[2]=0;
	break;
      case P_USBDM:
      case P_USBDM_V2:
	usb_data[0]=5;	/* get 5 bytes */
	usb_data[1]=CMD_USBDM_GET_VER;
	usb_data[2]=0;
	usb_data[3]=0;
	usb_data[4]=0;
	break;
      case P_NONE:
      case P_TBDML:
      case P_TBLCF:
      default: 
	usb_data[0]=3;	/* get 3 bytes */
	usb_data[1]=CMD_TBLCF_GET_VER;
	usb_data[2]=0;
	break;
    };
    
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
    switch (usb_devs[dev].type) {
      case P_OSBDM:
	usb_data[1]=CMD_OSBDM_GET_LAST_STATUS;
	break;
      case P_USBDM:
      case P_USBDM_V2:
	return BDM_RC_ILLEGAL_COMMAND;
	break;
      case P_NONE:
      case P_TBDML:
      case P_TBLCF:
      default: 
	usb_data[1]=CMD_TBLCF_GET_LAST_STATUS;
	break;
    };
    usb_data[1]=(usb_devs[dev].type==P_TBLCF)?CMD_TBLCF_GET_LAST_STATUS:CMD_OSBDM_GET_LAST_STATUS;
    bdm_usb_recv_ep0(&usb_devs[dev], usb_data);
    bdm_print("BDMUSB_GET_LAST_STATUS: Reported last command status 0x%02X\r\n",usb_data[0]);
    return usb_data[0];
}

/* sets target MCU type */
/* returns 0 on success and non-zero on failure */
unsigned char bdmusb_set_target_type(int dev, target_type_e target_type) {
    unsigned char ret_val;
    usb_data[0]=1;	 /* get 1 byte */
    switch (usb_devs[dev].type) {
      case P_OSBDM:
	usb_data[1]=CMD_OSBDM_SET_TARGET;
	break;
      case P_USBDM:
      case P_USBDM_V2:
	usb_data[1]= CMD_USBDM_SET_TARGET;
	break;
      case P_NONE:
      case P_TBDML:
      case P_TBLCF:
      default: 
	usb_data[1]=CMD_TBLCF_SET_TARGET;
	break;
    };
    usb_data[2]=target_type;
    ret_val = bdm_usb_recv_ep0(&usb_devs[dev], usb_data);
    if (ret_val == BDM_RC_OK) {
      bdm_print("BDMUSB_SET_TARGET_TYPE: Set target type 0x%02X (0x%02X)\r\n",usb_data[2],usb_data[0]);
      if (usb_devs[dev].type==P_TBLCF) 
	ret_val = !(usb_data[0]==CMD_TBLCF_SET_TARGET);
    } else 
      bdm_print("BDMUSB_SET_TARGET_TYPE - Error %d \r\n",ret_val);
    
    return(ret_val);
}

/* resets the target to normal or BDM mode */
/* returns 0 on success and non-zero on failure */
unsigned char bdmusb_target_reset(int dev, target_mode_e target_mode) {
    unsigned char ret_val;
    usb_data[0]=1;	 /* get 1 byte */
    switch (usb_devs[dev].type) {
      case P_OSBDM:
	usb_data[1]=CMD_OSBDM_TARGET_RESET;
	break;
      case P_USBDM:
      case P_USBDM_V2:
	usb_data[1]= CMD_USBDM_TARGET_RESET;
	break;
      case P_NONE:
      case P_TBDML:
      case P_TBLCF:
      default: 
	usb_data[1]=CMD_TBLCF_TARGET_RESET;
	break;
    };
    usb_data[2]=target_mode;
    ret_val = bdm_usb_recv_ep0(&usb_devs[dev], usb_data);
    if (ret_val == BDM_RC_OK) {
	bdm_print("BDMUSB_TARGET_RESET: Target reset into mode 0x%02X (0x%02X)\r\n",usb_data[2],usb_data[0]);
	if (usb_devs[dev].type==P_TBLCF) 
	    ret_val = (!(usb_data[0]==CMD_TBLCF_TARGET_RESET));
    } else 
	bdm_print("BDMUSB_TARGET_RESET - Error %d \r\n",ret_val);
    
    return ret_val;
}

/* fills user supplied structure with current state of the BDM communication channel */
/* returns 0 on success and non-zero on failure */
unsigned char bdmusb_bdm_sts(int dev, bdm_status_t *bdm_status) {
    unsigned char ret_val;
    unsigned int temp_state;
    usb_data[0]=3;	 /* get 3 bytes */
    switch (usb_devs[dev].type) {
      case P_OSBDM:
	  usb_data[1]=CMD_OSBDM_GET_BDM_STATUS;
	  break;
      case P_USBDM:
      case P_USBDM_V2:
	  usb_data[1]=CMD_USBDM_GET_BDM_STATUS;
	  break;
      case P_NONE:
      case P_TBDML:
      case P_TBLCF:
      default:
	  usb_data[1]=CMD_TBLCF_GET_BDM_STATUS;
	  break;
    }
    
    ret_val = bdm_usb_recv_ep0(&usb_devs[dev], usb_data);
    /* Analyze the response */
    if (ret_val == BDM_RC_OK) {
      temp_state = usb_data[1]*256+usb_data[2];
      switch (usb_devs[dev].type) {
	case P_OSBDM:
	case P_USBDM:
	case P_USBDM_V2:
	  bdm_status->ackn_state   = (temp_state&USBDM_ACKN)?ACKN:WAIT;
	  bdm_status->reset_state  = (temp_state&USBDM_RESET_STATE)?RSTO_INACTIVE:RSTO_ACTIVE; // Active LOW!
	  bdm_status->reset_recent = (temp_state&USBDM_RESET_DETECT)?RESET_DETECTED:NO_RESET_ACTIVITY;
	  bdm_status->halt_state   = (temp_state&USBDM_HALT)?TARGET_HALTED:TARGET_RUNNING;
	  switch(temp_state&USBDM_POWER_MASK) {
	    case USBDM_POWER_NONE:
	      /* Target has no power */
	      bdm_status->power_state = BDM_TARGET_VDD_NONE;
	      break;
	    case USBDM_POWER_EXT:
	      /* Target has external power */
	      bdm_status->power_state = BDM_TARGET_VDD_EXT;
	      break;
	    case USBDM_POWER_INT:
	      /* Target has internal power */
	      bdm_status->power_state = BDM_TARGET_VDD_INT;
	      break;
	    case USBDM_POWER_ERR:
	      /* Target power error (internal but overload) */
	      bdm_status->power_state = BDM_TARGET_VDD_ERR;
	      break;
	  };
	  switch(temp_state&USBDM_COMM_MASK) {
	    case USBDM_NOT_CONNECTED:
	      /** No connection with target */
	      bdm_status->connection_state = SPEED_NO_INFO;
	      break;
	    case USBDM_SYNC_DONE:
	      /** Target communication speed determined by BDM SYNC */
	      bdm_status->connection_state = SPEED_SYNC;
	      break;
	    case USBDM_GUESS_DONE:
	      /** Target communication speed guessed */
	      bdm_status->connection_state = SPEED_GUESSED;
	      break;
	    case USBDM_USER_DONE:
	      /**< Target communication speed specified by user */
	      bdm_status->connection_state = SPEED_USER_SUPPLIED;
	      break;
	  };
	  switch(temp_state&USBDM_VPP_MASK) {
	      case USBDM_VPP_OFF:
		/* Programming Voltage off */
		bdm_status->flash_state = BDM_TARGET_VPP_OFF;
		break;
	      case USBDM_VPP_ON:
		/* Programming Voltage on */
		bdm_status->flash_state = BDM_TARGET_VPP_ON;
		break;
	      case USBDM_VPP_STANDBY:
		/* Programming Voltage standby */
		bdm_status->flash_state = BDM_TARGET_VPP_STANDBY;
		break;
	      case USBDM_VPP_ERR:
		/* Error */
		bdm_status->flash_state = BDM_TARGET_VPP_ERROR;
		break;
	  };

	  break;
	case P_NONE:
	case P_TBDML:
	case P_TBLCF:
	default:
	  if (usb_data[0]!=CMD_TBLCF_GET_BDM_STATUS) return(BDM_RC_ILLEGAL_PARAMS);
	  bdm_status->reset_state = (temp_state&TBLCF_RST0_STATE_MASK)?RSTO_INACTIVE:RSTO_ACTIVE;
	  bdm_status->reset_recent = (temp_state&TBLCF_RESET_DETECTED_MASK)?RESET_DETECTED:RESET_NOT_DETECTED;
      };
      bdm_print("BDMUSB_BDM_STATUS: Reported communication status 0x%04X (0x%02X)\r\n",
	  temp_state,usb_data[0]);
    }
    
    return(ret_val);
}

/* brings the target into BDM mode */
/* returns 0 on success and non-zero on failure */
unsigned char bdmusb_target_halt(int dev) {
    unsigned char ret_val;
    usb_data[0]=1;	 /* get 1 byte */
    switch (usb_devs[dev].type) {
      case P_OSBDM:
	usb_data[1]=CMD_OSBDM_HALT;
	break;
      case P_USBDM:
      case P_USBDM_V2:
	usb_data[1]= CMD_USBDM_TARGET_HALT;
	break;
      case P_NONE:
      case P_TBDML:
      case P_TBLCF:
      default: 
	usb_data[1]=CMD_TBLCF_TARGET_HALT;
	break;
    };
    ret_val = bdm_usb_recv_ep0(&usb_devs[dev], usb_data);
    bdm_print("BDMUSB_TARGET_HALT: (0x%02X)\r\n",usb_data[0]);
    //return(!(usb_data[0]==CMD_HALT));
    return ret_val;
}

/* starts target execution from current PC address */
/* returns 0 on success and non-zero on failure */
unsigned char bdmusb_target_go(int dev) {
    unsigned char ret_val;
    usb_data[0]=1;	 /* get 1 byte */
    switch (usb_devs[dev].type) {
      case P_OSBDM:
	usb_data[1]=CMD_OSBDM_GO;
	break;
      case P_USBDM:
      case P_USBDM_V2:
      case P_NONE:
      case P_TBDML:
      case P_TBLCF:
      default: 
	usb_data[1]=CMD_USBDM_TARGET_GO;
	break;
    };
    ret_val = bdm_usb_recv_ep0(&usb_devs[dev], usb_data);
    bdm_print("BDMUSB_TARGET_GO: (0x%02X)\r\n",usb_data[0]);
    //return(!(usb_data[0]==CMD_GO));
    return ret_val;
}

/* steps over a single target instruction */
/* returns 0 on success and non-zero on failure */
unsigned char bdmusb_target_step(int dev) {
    unsigned char ret_val;
    usb_data[0]=1;	 /* get 1 byte */
    switch (usb_devs[dev].type) {
      case P_OSBDM:
	usb_data[1]=CMD_OSBDM_STEP;
	break;
      case P_USBDM:
      case P_USBDM_V2:
	usb_data[1]=CMD_USBDM_TARGET_STEP;
	break;
      case P_NONE:
      case P_TBDML:
      case P_TBLCF:
      default: 
	usb_data[1]=CMD_TBLCF_TARGET_STEP;
	break;
    };
    
    ret_val = bdm_usb_recv_ep0(&usb_devs[dev], usb_data);
    bdm_print("BDMUSB_TARGET_STEP: (0x%02X)\r\n",usb_data[0]);
    //return(!(usb_data[0]==CMD_STEP));
    return ret_val;
}

/* reads control register at the specified address and writes its contents into the supplied buffer */
/* returns 0 on success and non-zero on failure */
unsigned char bdmusb_read_creg(int dev, unsigned int address, unsigned long int* result) {
    unsigned char ret_val;
    
    usb_data[0]=5;	 /* get 5 bytes */
    usb_data[2]=(address>>8)&0xff;	/* address in big endian */
    usb_data[3]=address&0xff;
    
    switch (usb_devs[dev].type) {
	case P_USBDM_V2:
	    usb_data[1]=CMD_USBDM_READ_CREG;
	    break;
	case P_TBLCF:
	    usb_data[1]=CMD_TBLCF_READ_CREG;
	    break;
	case P_OSBDM:
	case P_USBDM:
	    usb_data[0]=5;	 /* get 5 bytes */
	    usb_data[1]=CMD_CF_READ_CREG;
	    usb_data[2]=address;
	    break;
	case P_NONE:
	case P_TBDML:
	default:
	    break;
    };
    ret_val = bdm_usb_recv_ep0(&usb_devs[dev], usb_data);
    *result = (((unsigned long int)usb_data[1])<<24)+(usb_data[2]<<16)+(usb_data[3]<<8)+usb_data[4];	/* result is in big endian */
    if ( (usb_devs[dev].type == P_OSBDM) || (usb_devs[dev].type == P_USBDM) ) {
      *result = *((uint32_t *)(usb_data+1)); // Coldfire register
    }
    if (usb_devs[dev].type == P_TBLCF)
      *result = (unsigned long int)(*result); 
    bdm_print("BDMUSB_READ_CREG: Read control register from address 0x%04X, result: 0x%08lX (0x%02X)\r\n",address,*result,usb_data[0]);
    //return(!(usb_data[0]==CMD_READ_CREG));
    return ret_val;
}

/* writes control register at the specified address */
void bdmusb_write_creg(int dev, unsigned int address, unsigned long int value) {
    usb_data[0]=6;	 /* send 6 bytes */
    usb_data[2]=(uint8_t)address&0xff;
    usb_data[3]=(uint8_t)(value>>24)&0xff;
    usb_data[4]=(uint8_t)(value>>16)&0xff;
    usb_data[5]=(uint8_t)(value>>8)&0xff;
    usb_data[6]=(uint8_t)(value)&0xff;
    
    switch (usb_devs[dev].type) {
	case P_USBDM_V2:
	    usb_data[0]=8;	 /* send 8 bytes */
	    usb_data[1]=CMD_USBDM_WRITE_CREG;
	    usb_data[2]=(uint8_t)(address>>8)&0xff;	/* address in big endian */
	    usb_data[3]=(uint8_t)address&0xff;
	    usb_data[4]=(uint8_t)(value>>24)&0xff;
	    usb_data[5]=(uint8_t)(value>>16)&0xff;
	    usb_data[6]=(uint8_t)(value>>8)&0xff;
	    usb_data[7]=(uint8_t)(value)&0xff;
	    break;
	case P_TBLCF:
	    usb_data[0]=7;	 /* send 7 bytes */
	    usb_data[1]=CMD_TBLCF_WRITE_CREG;
	    break;
	case P_OSBDM:
	case P_USBDM:
	    usb_data[1]=CMD_CF_WRITE_CREG;
	    
	    break;
	case P_TBDML:
	case P_NONE:
	default:
	    break;
    };
    
    bdm_print("BDMUSB_WRITE_CREG: Write control register at address 0x%04X with 0x%08lX\r\n",address,value);
    bdm_usb_send_ep0(&usb_devs[dev], usb_data);
    // Do handshake?
    //tt[0] = 1;  /* receive 1 byte */
    //tt[1] = CMD_GET_LAST_STATUS;
    //bdm_usb_recv_ep0(&usb_devs[dev], tt);

}

/* reads the specified debug register and writes its contents into the supplied buffer */
/* returns 0 on success and non-zero on failure */
unsigned char bdmusb_read_dreg(int dev, unsigned int dreg_index, unsigned long int * result) {
    unsigned char ret_val;
    
    usb_data[0]=5;	 /* get 5 bytes */
    usb_data[2]=(unsigned char)dreg_index;
    
    switch (usb_devs[dev].type) {
	case P_USBDM_V2:
	    usb_data[1]=CMD_TBLCF_READ_DREG;
	    usb_data[2]=(dreg_index>>8)&0xff;
	    usb_data[3]=dreg_index&0xff;
	    break;
	case P_TBLCF:
	    usb_data[1]=CMD_TBLCF_READ_DREG;
	    break;
	case P_OSBDM:
	case P_USBDM:
	    usb_data[0]=6;	 /* send 6 bytes */
	    usb_data[1]=CMD_CF_READ_DREG;
	    break;
	case P_TBDML:
	case P_NONE:
	default:
	    break;
    };
    ret_val = bdm_usb_recv_ep0(&usb_devs[dev], usb_data);
    *result = (((unsigned long int)usb_data[1])<<24)+(usb_data[2]<<16)+(usb_data[3]<<8)+usb_data[4];	/* result is in big endian */
    bdm_print("BDMUSB_READ_DREG: Read debug register #0x%02X, result: 0x%08lX (0x%02X)\r\n",
	dreg_index,*result,usb_data[0]);
    return ret_val;
}

/* writes specified debug register */
void bdmusb_write_dreg(int dev, unsigned int dreg_index, unsigned long int value) {
    
    usb_data[0]=6;	 /* send 6 bytes */
    usb_data[2]=dreg_index&0xff;
    usb_data[3]=(value>>24)&0xff;
    usb_data[4]=(value>>16)&0xff;
    usb_data[5]=(value>>8)&0xff;
    usb_data[6]=(value)&0xff;
    
    switch (usb_devs[dev].type) {
	case P_USBDM_V2:
	    usb_data[0]=8;	 /* send 8 bytes */
	    usb_data[1]=CMD_USBDM_WRITE_DREG;
	    usb_data[2]=(dreg_index>>8)&0xff;
	    usb_data[3]=dreg_index&0xff;
	    usb_data[4]=(value>>24)&0xff;
	    usb_data[5]=(value>>16)&0xff;
	    usb_data[6]=(value>>8)&0xff;
	    usb_data[7]=(value)&0xff;
	    break;
	case P_TBLCF:
	    usb_data[1]=CMD_TBLCF_WRITE_DREG;
	    break;
	case P_OSBDM:
	case P_USBDM:
	    usb_data[1]=CMD_CF_WRITE_DREG;
	    break;
	case P_TBDML:
	case P_NONE:
	default:
	    break;
    };
	
    bdm_print("BDMUSB_WRITE_DREG: Write debug register #0x%02X with 0x%08lX\r\n",dreg_index,value);
    bdm_usb_send_ep0(&usb_devs[dev], usb_data);
}

/* reads the specified register and writes its contents into the supplied buffer */
/* returns 0 on success and non-zero on failure */
unsigned char bdmusb_read_reg(int dev, unsigned int reg_index, unsigned long int * result) {
    unsigned char ret_val;
  
      usb_data[0]=5;	 /* get 5 bytes */
      usb_data[2]=reg_index;
      
      switch (usb_devs[dev].type) {
	case P_USBDM_V2:
	    usb_data[1]=CMD_USBDM_READ_REG;
	    usb_data[2]=(reg_index>>8)&0xff;
	    usb_data[3]=reg_index&0xff;
	    break;
	case P_TBLCF:
	    usb_data[1]=CMD_TBLCF_READ_REG;
	    break;
	case P_OSBDM:
	case P_USBDM:
	    usb_data[1]=CMD_CF_READ_REG;
	    break;
	case P_TBDML:
	case P_NONE:
	default:
	    break;
    };
    ret_val = bdm_usb_recv_ep0(&usb_devs[dev], usb_data);
    *result = (((unsigned long int)usb_data[1])<<24)+(usb_data[2]<<16)+(usb_data[3]<<8)+usb_data[4];	/* result is in big endian */
    bdm_print("BDMUSB_READ_REG: Read register #0x%02X, result: 0x%08lX (0x%02X)\r\n",
	  reg_index,*result,usb_data[0]);
    //return(!(usb_data[0]==CMD_READ_REG));
    return ret_val;
}

/* writes specified register */
void bdmusb_write_reg(int dev, unsigned int reg_index, unsigned long int value) {
    usb_data[0]=6;	 /* send 6 bytes */
    usb_data[2]=reg_index&0xff;
    usb_data[3]=(value>>24)&0xff;
    usb_data[4]=(value>>16)&0xff;
    usb_data[5]=(value>>8)&0xff;
    usb_data[6]=(value)&0xff;
    
    switch (usb_devs[dev].type) {
	case P_USBDM_V2:
	    usb_data[0]=8;	 /* send 8 bytes */
	    usb_data[1]=CMD_USBDM_WRITE_REG;
	    usb_data[2]=(reg_index>>8)&0xff;
	    usb_data[3]=reg_index&0xff;
	    usb_data[4]=(value>>24)&0xff;
	    usb_data[5]=(value>>16)&0xff;
	    usb_data[6]=(value>>8)&0xff;
	    usb_data[7]=(value)&0xff;
	    break;
	case P_TBLCF:
	    usb_data[1]=CMD_TBLCF_WRITE_REG;
	    break;
	case P_USBDM:
	case P_OSBDM:
	    usb_data[1]=CMD_CF_WRITE_REG;
	    break;
	case P_NONE:
	case P_TBDML:
	default:
	    break;
    };
    bdm_print("BDMUSB_WRITE_REG: Write register #0x%02X with 0x%08lX\r\n",reg_index,value);
    bdm_usb_send_ep0(&usb_devs[dev], usb_data);
}
