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

#include "log.h"
#include "bdmusb.h"
#include "commands.h"
#include "bdmusb_low_level.h"

#ifndef NULL
#define NULL '\0'
#endif

extern unsigned char usb_data[MAX_DATA_SIZE+2];

/** Obtain a description of the hardware version
  *
  * @return ptr to static string describing the hardware
  */
const char *getHardwareDescription(unsigned int hardwareVersion) {
     /* BDM hardware descriptions */
   static const char
      *const hardwareDescriptions[] = {
           NULL,                                         //  0
           "USBDM - Extended TBDML/OSBDM (JB16DWE)",     //  1
           "TBDML - Minimal TBDML",                      //  2
           "TBDML - Minimal TBDML (Internal version)",   //  3
           "OSBDM - Original OSBDM",                     //  4
           "Witztronics TBDML/OSBDM",                    //  5
           "OSBDM+ - Extended OSBDM (RS08 support)",     //  6
           "USBDM - Extended TBDML/OSBDM (JMxxCLD)",     //  7
           "USBDM - Extended TBDML/OSBDM (JMxxCLC)",     //  8
           "USBSPYDER08 - SofTec USBSPYDER08",           //  9
           "USBDM - Extended TBDML/OSBDM (UF32PBE)",     // 10
           "TBLCF - Extended TBLCF (JMxxCLD)",           // 11
           "USBDM/CF - Combined USBDM/TBLCF (JMxxCLD)",  // 12
           "USBDM - Extended TBDML/OSBDM (JS16CWJ)",     // 13
           NULL,                                         // 14
           "Custom",                                     // 15
         };

   static char const *hardwareName = NULL;
   hardwareVersion &= 0x3F; // mask out BDM processor type
   if (hardwareVersion < sizeof(hardwareDescriptions)/sizeof(hardwareDescriptions[0]))
      hardwareName = hardwareDescriptions[hardwareVersion];
   if (hardwareName == NULL)
      hardwareName = "UNKNOWN";
   return hardwareName;
}

unsigned char usbdm_get_capabilities(bdmusb_dev* dev) {
    unsigned char return_value;
    bdm_print("USBDM_get_capabilities()\n");

    usb_data[0] = 0;
    usb_data[1] = CMD_USBDM_GET_CAPABILITIES;

    return_value = bdm_usb_transaction(dev->dev_ref, 2, 3, usb_data);
    if (return_value != BDM_RC_OK) {
	dev->hw_cap= 0;
	bdm_print("USBDM_GetCapabilities()=> Failed!\n");
    }
    else {
	dev->hw_cap = (usb_data[1]<<8)+usb_data[2];
	//bdm_print("USBDM_GetCapabilities()=>0x%2.2X = %s\n",
	//      dev->hw_cap, getCapabilityName(*capabilities));
	bdm_print("USBDM_GetCapabilities()=>0x%2.2X\n", dev->hw_cap);
    }

  return return_value;
}

/** Set BDM interface options
  *
  * @param bdmOptions : Options to pass to BDM interface
  *
  * @return \n
  *     BDM_RC_OK => OK \n
  *     other     => Error code - see \ref USBDM_ErrorCode
  */
unsigned char usbdm_set_options(bdmusb_dev* dev) {

//#ifdef LOG
   //bdmState.activityFlag = BDM_ACTIVE;
   /*
   bdm_print("USBDM_SetOptions():\n"
         "targetVdd           = %s\n"
         "cycleVddOnReset     = %s\n"
         "cycleVddOnConnect   = %s\n"
         "leaveTargetPowered  = %s\n"
         "autoReconnect       = %s\n"
         "guessSpeed          = %s\n"
         "useAltBDMClock      = %s\n"
         "useResetSignal      = %s\n"
         "targetClockTrim     = %d kHz\n",
         getVoltageSelectName(dev->options.targetVdd),
	 dev->options.cycleVddOnReset?"Y":"N",
         dev->options.cycleVddOnConnect?"Y":"N",
         dev->options.leaveTargetPowered?"Y":"N",
         dev->options.autoReconnect?"Y":"N",
         dev->options.guessSpeed?"Y":"N",
         getClockSelectName(dev->options.useAltBDMClock),
         dev->options.useResetSignal?"Y":"N",
         dev->options.targetClockFreq
   );*/
   
   bdm_print("USBDM_SetOptions():\n"
         "targetVdd           = %d\n"
         "cycleVddOnReset     = %s\n"
         "cycleVddOnConnect   = %s\n"
         "leaveTargetPowered  = %s\n"
         "autoReconnect       = %s\n",
         dev->options.targetVdd,
         dev->options.cycleVddOnReset?"Y":"N",
         dev->options.cycleVddOnConnect?"Y":"N",
         dev->options.leaveTargetPowered?"Y":"N",
         dev->options.autoReconnect?"Y":"N"
   );

  bdm_print("guessSpeed          = %s\n"
         "useAltBDMClock      = %d\n"
         "useResetSignal      = %s\n"
         "targetClockTrim     = %d kHz\n",
         dev->options.guessSpeed?"Y":"N",
         dev->options.useAltBDMClock,
         dev->options.useResetSignal?"Y":"N",
         dev->options.targetClockFreq
   );
//#endif

   int index = 0;
   usb_data[index++] = 0;
   usb_data[index++] = CMD_USBDM_SET_OPTIONS;
   usb_data[index++] = (uint8_t) dev->options.targetVdd;
   usb_data[index++] = (uint8_t) dev->options.cycleVddOnReset;
   usb_data[index++] = (uint8_t) dev->options.cycleVddOnConnect;
   usb_data[index++] = (uint8_t) dev->options.leaveTargetPowered;
   usb_data[index++] = (uint8_t) dev->options.autoReconnect;
   usb_data[index++] = (uint8_t) dev->options.guessSpeed;
   usb_data[index++] = (uint8_t) dev->options.useAltBDMClock;
   usb_data[index++] = (uint8_t) dev->options.useResetSignal;
   return bdm_usb_transaction(dev->dev_ref, index, 1, usb_data);
}

/**
  * Sets the BDM communication speed.
  *
  * @param frequency => BDM Communication speed in kHz \n
  * Usually equal to the CPU Bus frequency. \n
  * May be unrelated to bus speed if alternative BDM clock is in use. \n
  * For HCS12, HCS08 and RS08 targets a value of zero causes a connect sequence
  * instead i.e. automatically try to determine connection speed. \n
  *
  * @return \n
  *     BDM_RC_OK  => OK \n
  *     other      => Error code - see \ref USBDM_ErrorCode
  */
unsigned char usbdm_set_speed( bdmusb_dev* dev) {
   unsigned int value;

   //bdmState.activityFlag = BDM_ACTIVE;

   bdm_print("USBDM_SetSpeed(BDM Clk = %d kHz)\n", dev);

   switch (dev->target) {
      case T_HC12 :
      case T_HCS08 :
      case T_RS08 :
      /*case T_CFV1 :
         if (frequency == 0) {
            // A speed value of 0 causes an automatic connection.
            return USBDM_Connect();
         }
         else {
         // BDM command value is length of SYNC pulse in 60MHz ticks
         // The SYNC pulse is 128 BDM clock cycles
         value = (60000UL * 128) / frequency;
         }*/
         break;
      case T_CFVx :
      case T_JTAG :
         // BDM command value is frequency in kHz. Should be less than 1/5 target clk
         value = dev->options.targetClockFreq;
         break;
      default :
         return BDM_RC_ILLEGAL_COMMAND;
   }
   usb_data[0] = 0;
   usb_data[1] = CMD_USBDM_SET_SPEED;
   usb_data[2] = (uint8_t)(value>>8);
   usb_data[3] = (uint8_t)value;
   return bdm_usb_transaction(dev->dev_ref, 4, 1, usb_data);
}

/**
  * Connects to Target.
  *
  * This will cause the BDM module to attempt to connect to the Target.
  * In most cases the BDM module will automatically determine the connection
  * speed and successfully connect.  If unsuccessful, it may be necessary
  * to manually set the speed using set_speed().
  *
  * @return \n
  *     BDM_RC_OK  => OK \n
  *     other      => Error code - see \ref USBDM_ErrorCode
  */
unsigned char usbdm_connect(int dev) {
    int ret_val;

    bdm_print("USBDM_CONNECT\n");

    usb_data[0] = 0;
    usb_data[1] = CMD_USBDM_CONNECT;
    ret_val = bdm_usb_transaction(dev, 2, 1, usb_data);
    if (ret_val != BDM_RC_OK)
      bdm_print("USBDM_CONNECT() => rc = %d\n", ret_val);
    else
      usb_devs[dev].connected = 1;

    return ret_val;
}
