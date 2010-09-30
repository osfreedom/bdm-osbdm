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

//static unsigned char usb_data[MAX_DATA_SIZE+2];
extern unsigned char usb_data[MAX_DATA_SIZE+2];

#define EP_OUT (LIBUSB_ENDPOINT_OUT|1) /**< EP # for Out endpoint */
#define EP_IN  (LIBUSB_ENDPOINT_IN |2) /**< EP # for In endpoint */

// Internal functions 
int bdm_usb_send_epOut(bdmusb_dev *dev, unsigned int count, unsigned char *data);
int bdm_usb_recv_epIn(bdmusb_dev *dev, unsigned int count, unsigned char *data);
int bdm_usb_transaction_ep0(bdmusb_dev *dev, unsigned int txSize, unsigned int rxSize, unsigned char *data);
int bdm_usb_transaction_epinout(bdmusb_dev *dev, unsigned int txSize, unsigned int rxSize, unsigned char *data);

//#ifdef LOG
// TEMP 
/** USBDM Command String from Command # */
static const char *const newCommandTable[]= {
   "CMD_USBDM_GET_COMMAND_RESPONSE"    , // 0
   "CMD_USBDM_SET_TARGET"              , // 1
   NULL                                , // 2
   "CMD_USBDM_DEBUG"                   , // 3
   "CMD_USBDM_GET_BDM_STATUS"          , // 4
   "CMD_USBDM_GET_CAPABILITIES"        , // 5
   "CMD_USBDM_SET_OPTIONS"             , // 6
   NULL                                , // 7
   NULL                                , // 8
   NULL                                , // 9
   NULL                                , // 10
   NULL                                , // 11
   "CMD_USBDM_GET_VER"                 , // 12
   NULL                                , // 13
   "CMD_USBDM_ICP_BOOT"                , // 14

   "CMD_USBDM_CONNECT"                 , // 15
   "CMD_USBDM_SET_SPEED"               , // 16
   "CMD_USBDM_GET_SPEED"               , // 17

   "CMD_USBDM_CONTROL_INTERFACE"       , // 18
   NULL                                , // 19

   "CMD_USBDM_READ_STATUS_REG"         , // 20
   "CMD_USBDM_WRITE_CONROL_REG"        , // 21

   "CMD_USBDM_TARGET_RESET"            , // 22
   "CMD_USBDM_TARGET_STEP"             , // 23
   "CMD_USBDM_TARGET_GO"               , // 24
   "CMD_USBDM_TARGET_HALT"             , // 25

   "CMD_USBDM_WRITE_REG"               , // 26
   "CMD_USBDM_READ_REG"                , // 27

   "CMD_USBDM_WRITE_CREG"              , // 28
   "CMD_USBDM_READ_CREG"               , // 29

   "CMD_USBDM_WRITE_DREG"              , // 30
   "CMD_USBDM_READ_DREG"               , // 31

   "CMD_USBDM_WRITE_MEM"               , // 32
   "CMD_USBDM_READ_MEM"                , // 33

   "CMD_USBDM_TRIM_CLOCK"              , // 34
   "CMD_USBDM_RS08_FLASH_ENABLE"       , // 35
   "CMD_USBDM_RS08_FLASH_STATUS"       , // 36
   "CMD_USBDM_RS08_FLASH_DISABLE"      , // 37

   "CMD_USBDM_JTAG_GOTORESET"          , // 38
   "CMD_USBDM_JTAG_GOTOSHIFT"          , // 39
   "CMD_USBDM_JTAG_WRITE"              , // 40
   "CMD_USBDM_JTAG_READ"               , // 41
};

/** USBDM Command String from Command # */
static const char *const oldCommandTable[]= {
   NULL,                    // 0
   NULL,                    // 1
   NULL,                    // 2
   NULL,                    // 3
   NULL,                    // 4
   NULL,                    // 5
   NULL,                    // 6
   NULL,                    // 7
   "CMD_DEBUG",             // 8  Debugging commands (parameter determines actual command)
   NULL,                    // 9
   "CMD_OPTION",            // 10 Get/set various options
   NULL,                    // 11
   "CMD_GET_VER",           // 12 returns 16 bit HW/SW version number, (major & minor revision
   "CMD_GET_LAST_STATUS",   // 13 returns status of the previous command
   "CMD_SET_BOOT",          // 14 request boot-loader firmware upgrade on next power-up, parameters: 'B','O','O','T', returns: none
   NULL,                    // 15
   NULL,                    // 16
   NULL,                    // 17
   NULL,                    // 18
   NULL,                    // 19
   NULL,                    // 20
   NULL,                    // 21
   NULL,                    // 22
   NULL,                    // 23
   NULL,                    // 24
   NULL,                    // 25
   NULL,                    // 26
   NULL,                    // 27
   NULL,                    // 28
   NULL,                    // 29
   "CMD_SET_TARGET",        // 30 set target, 8bit parameter: 00=HC12/HCS12(default), 01=HCS08
   "CMD_CONNECT",           // 31 try to connect to the target
   NULL,                    // 32
   "CMD_RESET",             // 33 8-bit parameter: 0=reset to Special Mode, 1=reset to Normal mode
   "CMD_GET_STATUS",        // 34 returns 16bit status word:
   "CMD_READ_BD",           // 35 parameter: 16-bit address, returns 8-bit value read from address;
   "CMD_WRITE_BD",          // 36 parameter: 16-bit address, 8-bit value to write;
   NULL,                    // 37
   NULL,                    // 38
   "CMD_HALT",              // 39 stop the CPU and bring it into background mode
   NULL,                    // 40 deprecated
   "CMD_READ_SPEED",        // 41 read speed of the target: returns 16-bit tick count
   "CMD_GO",                // 42 start code execution
   NULL,                    // 43
   "CMD_STEP",              // 44 perform single step
   NULL,                    // 45
   "CMD_SET_SPEED1",        // 46 sets-up the BDM interface for a new bit rate & tries
   "CMD_SET_DERIVATIVE",    // 47
   NULL,                    // 48
   NULL,                    // 49
   "CMD_READ_8",            // 50 parameter 16bit address, returns 8bit value read from address
   "CMD_READ_16",           // 51 parameter 16bit address, returns 16bit value read from address
   NULL,                    // 52
   "CMD_READ_REGS",         // 53 reads registers, returns 16bit values:
   NULL,                    // 54
   "CMD_READ_BLOCK1",       // 55 parameter 16bit address, 8bit count of bytes to read,
   NULL,                    // 56
   NULL,                    // 57
   NULL,                    // 58
   NULL,                    // 59
   "CMD_WRITE_8",           // 60 parameter 16bit address, 8bit value to write
   "CMD_WRITE_16",          // 61 parameter 16bit address, 16bit value to write
   NULL,                    // 62
   NULL,                    // 63
   NULL,                    // 64
   "CMD_WRITE_BLOCK1",      // 65 parameters: 16bit address, 8bit count of bytes,
   NULL,                    // 66
   NULL,                    // 67
   NULL,                    // 68
   NULL,                    // 69
   NULL,                    // 70
   NULL,                    // 71
   NULL,                    // 72
   NULL,                    // 73
   NULL,                    // 74
   NULL,                    // 75
   NULL,                    // 76
   NULL,                    // 77
   NULL,                    // 78
   NULL,                    // 79
   "CMD_WRITE_REG_PC",      // 80 parameter: 16-bit PC value
   "CMD_WRITE_REG_SP",      // 81 parameter: 16-bit SP value
   "CMD_WRITE_REG_X",       // 82 parameter: 16-bit IX (H:X) value
   "CMD_WRITE_REG_Y",       // 83 parameter: 16-bit IY value
   "CMD_WRITE_REG_D",       // 84 parameter: 16-bit B:A (x:A) value
   "CMD_WRITE_REG_CCR",     // 85 parameter: 16-bit CCR (x:CCR) value
   "CMD_CF_READ_CSR2",      // 86
   "CMD_CF_READ_CSR3",      // 87
   "CMD_CF_WRITE_CSR2",     // 88
   "CMD_CF_WRITE_CSR3",     // 89
   "CMD_CF_READ_REG",       // 90
   "CMD_CF_READ_CREG",      // 91
   "CMD_CF_READ_DREG",      // 92
   "CMD_CF_READ_XCSR",      // 93
   "CMD_CF_WRITE_REG",      // 94
   "CMD_CF_WRITE_CREG",     // 95
   "CMD_CF_WRITE_DREG",     // 96
   "CMD_CF_READ_MEM",       // 97
   "CMD_CF_WRITE_MEM",      // 98
   "CMD_CF_WRITE_XCSR",     // 99
   "CMD_READ_STATUS",       // 100
   "CMD_WRITE_CONTROL",     // 101
   "CMD_WRITE_BKPT",        // 102
   "CMD_READ_BKPT",         // 103
   "CMD_READ_SPC",          // 104
   "CMD_WRITE_SPC",         // 105
   "CMD_READ_CCR_PC",       // 106
   "CMD_WRITE_CCR_PC",      // 107
   NULL,                    // 108
   NULL,                    // 109
   "CMD_MASS_ERASE",        // 110
   NULL,                    // 111
   NULL,                    // 112
   "CMD_FLASH_DLSTART",     // 113
   "CMD_FLASH_DLEND",       // 114
   "TCMD_NEXT_DLDATA",      // 115
   NULL,                    // 116
   NULL,                    // 117
   NULL,                    // 118
   NULL,                    // 119
   "CMD_CFVx_RESYNCHRONIZE",        // 120  Resynchronize communication with the target (in case of noise, etc.) */
   "CMD_CFVx_RESET",                // 121, Reset target @param [2] 0=>reset to Special Mode, 1=>reset to Normal mode
   "CMD_CFVx_GO",                   // 122, Start code execution
   "CMD_CFVx_STEP",                 // 123, Perform single step
   "CMD_CFVx_HALT",                 // 124, Stop the CPU and bring it into background mode
   "CMD_CFVx_ASSERT_TA",            // 125  parameter: 8-bit number of 10us ticks - duration of the TA assertion */
   NULL,                            // 126
   NULL,                            // 127
   NULL,                            // 128
   NULL,                            // 129
   "CMD_CFVx_READ_MEM8",            // 130, parameter 32bit address, returns 8bit value read from address */
   "CMD_CFVx_READ_MEM16",           // 131, parameter 32bit address, returns 16bit value read from address */
   "CMD_CFVx_READ_MEM32",           // 132, parameter 32bit address, returns 32bit value read from address */
   "CMD_CFVx_READ_MEMBLOCK8",       // 133, parameter 32bit address, returns block of 8bit values read from the address and onwards */
   "CMD_CFVx_READ_MEMBLOCK16",      // 134, parameter 32bit address, returns block of 16bit values read from the address and onwards */
   "CMD_CFVx_READ_MEMBLOCK32",      // 135, parameter 32bit address, returns block of 32bit values read from the address and onwards */
   "CMD_CFVx_WRITE_MEM8",           // 136, parameter 32bit address & an 8-bit value to be written to the address */
   "CMD_CFVx_WRITE_MEM16",          // 137, parameter 32bit address & a 16-bit value to be written to the address */
   "CMD_CFVx_WRITE_MEM32",          // 138, parameter 32bit address & a 32-bit value to be written to the address */
   "CMD_CFVx_CFVx_WRITE_MEMBLOCK8", // 139, parameter 32bit address & a block of 8-bit values to be written from the address */
   "CMD_CFVx_WRITE_MEMBLOCK16",     // 140, parameter 32bit address & a block of 16-bit values to be written from the address */
   "CMD_CFVx_WRITE_MEMBLOCK32",     // 141, parameter 32bit address & a block of 32-bit values to be written from the address */
   "CMD_CFVx_READ_REG",             // 142, parameter 8-bit register number to read, returns 32-bit register contents */
   "CMD_CFVx_CFVx_WRITE_REG",       // 143, parameter 8-bit register number to write & the 32-bit register contents to be written */
   "CMD_CFVx_READ_CREG",            // 144, parameter 16-bit register address, returns 32-bit control register contents */
   "CMD_CFVx_WRITE_CREG",           // 145, parameter 16-bit register address & the 32-bit control register contents to be written */
   "CMD_CFVx_READ_DREG",            // 146, parameter 8-bit register number to read, returns 32-bit debug module register contents */
   "CMD_CFVx_WRITE_DREG",           // 147, parameter 8-bit register number to write & the 32-bit debug module register contents to be written */
   "CMD_CFVx_SET_SPEED",            // 148, param [2..3] frequency in kHz                            // 148
   NULL,                            // 149
   "CMD_JTAG_GOTORESET",    // 150, no parameters, takes the TAP to TEST-LOGIC-RESET state, re-select the JTAG target to take TAP back to RUN-TEST/IDLE */
   "CMD_JTAG_GOTOSHIFT",    // 151, parameters 8-bit path option; path option ==0 : go to SHIFT-DR, !=0 : go to SHIFT-IR (requires the tap to be in RUN-TEST/IDLE) */
   "CMD_JTAG_WRITE",        // 152, parameters 8-bit exit option, 8-bit count of bits to shift in, and the data to be shifted in (shifted in LSB (last byte) first, unused bits (if any) are in the MSB (first) byte; exit option ==0 : stay in SHIFT-xx, !=0 : go to RUN-TEST/IDLE when finished */
   "CMD_JTAG_READ",         // 153, parameters 8-bit exit option, 8-bit count of bits to shift out; exit option ==0 : stay in SHIFT-xx, !=0 : go to RUN-TEST/IDLE when finished, returns the data read out of the device (first bit in LSB of the last byte in the buffer) */
};
//#endif

/**
  * \brief Maps a command code to a string
  *
  * @param command Command number
  *
  * @return pointer to static string describing the command
  */
const char *getCommandName(bdmusb_dev *dev, unsigned char command) {
   static char const *commandName = NULL;

   if (dev->type != P_USBDM_V2) {
      if (command < sizeof(oldCommandTable)/sizeof(oldCommandTable[0]))
         commandName = oldCommandTable[command];
   }
   else {
      if (command < sizeof(newCommandTable)/sizeof(newCommandTable[0]))
         commandName = newCommandTable[command];
   }
   if (commandName == NULL)
      commandName = "UNKNOWN";
   return commandName;
}

/** Debug command string from code*/
static const char *const debugCommands[] = {
   "ACKN",              // 0
   "SYNC",              // 1
   "Test Port",         // 2
   "USB Disconnect",    // 3
   "Find Stack size",   // 4
   "Vpp Off",           // 5
   "Vpp On",            // 6
   "Flash 12V Off",     // 7
   "Flash 12V On",      // 8
   "Vdd Off",           // 9
   "Vdd 3.3V On",       // 10
   "Vdd 5V On",         // 11
   "Cycle Vdd",         // 12
   "Measure Vdd",       // 13
};

/**
  * \brief Maps a Debug Command # to a string
  *
  * @param cmd Debug command number
  *
  * @return pointer to static string describing the command
  */
const char *getDebugCommandName(unsigned char cmd) {
   char const *cmdName = NULL;
   if (cmd < sizeof(debugCommands)/sizeof(debugCommands[0]))
      cmdName = debugCommands[cmd];
   if (cmdName == NULL)
      cmdName = "UNKNOWN";
   return cmdName;
}

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
  if (i<0) return(BDM_RC_USB_ERROR); else return(BDM_RC_OK);
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
    return(BDM_RC_USB_ERROR);
  }
  bdmusb_print("USB EP0 receive request:\n");
  bdm_print_dump(data,6);
  i=libusb_control_transfer(dev->handle, 0xC0, *(data+1), (*(data+2))+256*(*(data+3)),
                            (*(data+4))+256*(*(data+5)), data, count, TIMEOUT);
  bdmusb_print("USB EP0 receive:\n");
  bdm_print_dump(data,count);
  if (i<0) return(BDM_RC_USB_ERROR); else return(BDM_RC_OK);
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

int bdm_usb_send_epOut(bdmusb_dev *dev, unsigned int count, unsigned char *data) {
   unsigned char ret_val;
   int count_sent;

   if (dev->handle==NULL) {
      bdm_print("bdm_usb_send_epOut(): device not open\n");
      return BDM_RC_USB_ERROR;
   }

//#ifdef LOG_LOW_LEVEL
   bdm_print("============================\n");
   bdm_print("bdm_usb_send_epOut() - USB EP0ut send (%s, size=%d):\n", getCommandName(dev, data[1]), count);
   if (data[1] == CMD_USBDM_DEBUG)
      bdm_print("bdm_usb_send_epOut() - Debug cmd = %s\n", getDebugCommandName(data[2]));
   bdm_print_dump(data, count);
//#endif // LOG_LOW_LEVEL

   ret_val = libusb_bulk_transfer (dev->handle,
                       EP_OUT,  // Endpoint
                       (unsigned char *)data,         // ptr to Tx data
                       count,                // number of bytes to Tx
		       &count_sent,          // number of bytes sended 
                       TIMEOUT               // timeout
                       );
   if (ret_val<0) {
      bdm_print("bdm_usb_send_epOut() - Transfer failed (USB error = %d)\n", ret_val);
      data[0] = BDM_RC_USB_ERROR;
      return BDM_RC_USB_ERROR;
   }

   return BDM_RC_OK;
}
// Temporary buffer for IN transactions
unsigned char dummyBuffer[200];

int bdm_usb_recv_epIn(bdmusb_dev *dev, unsigned int count, unsigned char *data) {
   int ret_val;
   int retry = 5;
   int count_sent;

   if (dev->handle==NULL) {
      bdm_print("bdm_usb_recv_epIn(): device not open\n");
      return BDM_RC_USB_ERROR;
   }

//#ifdef LOG_LOW_LEVEL
   bdm_print("bdm_usb_recv_epIn(%d, ...)\n", count);
//#endif // LOG_LOW_LEVEL

   do {
      ret_val = libusb_bulk_transfer (dev->handle,
                         EP_IN,     // Endpoint
                         (unsigned char*)dummyBuffer,    // ptr to Rx data buffer
                         sizeof(dummyBuffer)-5, // number of bytes to Rx
			 &count_sent,           // number of bytes sended 
                         TIMEOUT                // timeout
                         );
      if (ret_val<0) {
         bdm_print("bdm_usb_recv_epIn(count=%d) - Transfer timeout (USB error = %d) - retry\n", count, ret_val);
         //Sleep(100); // So we don't monopolise the USB
//	 Delay(100);
      }
   } while ((ret_val<0) && (--retry>0));

   if (ret_val<0) {
      bdm_print("bdm_usb_recv_epIn(count=%d) - Transfer failed (USB error = %d)\n", count, ret_val);
      data[0] = BDM_RC_USB_ERROR;
      memset(&data[1], 0x00, count-1);
      return BDM_RC_USB_ERROR;
   }

   if (count_sent != count) {
      bdm_print("bdm_usb_recv_epIn() - Expected %d; received %d\n", count, count_sent);
   }
   
   memcpy(data, dummyBuffer, count);
   if (data[0] != BDM_RC_OK) { // Error?
      //bdm_print("bdm_usb_recv_epIn() - Error Return (%s):\n", getErrorName(data[0]));
      bdm_print("bdm_usb_recv_epIn() - Error Return (%d):\n", data[0] );
      bdm_print("bdm_usb_recv_epIn(size = %d, recvd = %d):\n", count, count_sent);
      bdm_print_dump(data,ret_val);
      memset(&data[1], 0x00, count-1);
   }

//#ifdef LOG_LOW_LEVEL
   bdm_print_dump(data, count_sent);
//#endif // LOG_LOW_LEVEL

   return data[0];
}  

int bdm_usb_transaction_ep0(bdmusb_dev *dev, unsigned int txSize, unsigned int rxSize, unsigned char *data) {
    int ret_val;
    // Use only ep0
    if (txSize <= 5) {
	// Transmission fits in SETUP pkt, Use single IN Data transfer to/from EP0
	*data = rxSize;
	ret_val = bdm_usb_recv_ep0(dev, data );
    }
    else {
	// Transmission requires separate IN transaction
	*data = txSize;
	ret_val = bdm_usb_send_ep0(dev, data );
	if (ret_val == BDM_RC_OK) {
	    // Get response
	    data[0] = rxSize;
	    data[1] = CMD_USBDM_GET_COMMAND_RESPONSE; // dummy command
	    ret_val = bdm_usb_recv_ep0(dev, data);
	}
    }
    if (ret_val != BDM_RC_OK) {
	data[0] = ret_val;
	memset(&data[1], 0x00, rxSize-1);
    }
    return ret_val;
}

int bdm_usb_transaction_epinout(bdmusb_dev *dev, unsigned int txSize, unsigned int rxSize, unsigned char *data) {
    int ret_val;
    //bdm_usb_transaction_ep0(&usb_devs[dev], txSize, rxSize, data);
    
    // more complex trannsactions
    const int MaxFirstTransaction = 30;

   // An initial transaction of up to MaxFirstTransaction bytes is sent
   // This is guaranteed to fit in a single pkt (<= endpoint MAXPACKETSIZE)
   data[0] = txSize;
   ret_val = bdm_usb_send_epOut(dev, txSize>MaxFirstTransaction?MaxFirstTransaction:txSize, data);

   // Remainder of data (if any) is sent as 2nd transaction
   // The size of this transaction is know to the receiver
   if ((ret_val == BDM_RC_OK) && (txSize>MaxFirstTransaction))
      ret_val = bdm_usb_send_epOut(dev, txSize-MaxFirstTransaction, data+MaxFirstTransaction);

   if (ret_val == BDM_RC_OK) {
      // Get response
      ret_val = bdm_usb_recv_epIn(dev, rxSize, data);
   }
   else {
      data[0] = ret_val;
      memset(&data[1], 0x00, rxSize-1);
   }
   return ret_val;

}

int  bdm_usb_transaction(int dev, unsigned int txSize, unsigned int rxSize, unsigned char *data) {
    int ret_val;
    
    if (usb_devs[dev].use_only_ep0)
      ret_val = bdm_usb_transaction_ep0(&usb_devs[dev], txSize, rxSize, data);
    else
      // more complex trannsactions
      ret_val = bdm_usb_transaction_epinout(&usb_devs[dev], txSize, rxSize, data);

}