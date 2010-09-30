/*
 * BDM USB commands. (from commands.h and USBDM_API_Private.h)
 * Copyright (C) 2010 Rafael Campos
 * Rafael Campos Las Heras <rafael@freedom.ind.br>
 *
 * This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* command format:

1   byte : command number (see below)
n   bytes: command parameters (data)

   data format:

all 16-bit and 32-bit data is transferred in big endian, i.e. MSB on lower address (first) and LSB on higher address (next)
data must be converted properly on intel machines (PCs)

*/

#ifndef _COMMANDS_H_
#define _COMMANDS_H_

/** \brief Maximum USB transfer size - entire transfer!
 *
 * Limited by BDM RAM
 * Multiple of 64 is nice since transfers may be in 8/64-byte pieces.
 */
#define MAX_PACKET_SIZE       (2*64)
#define MAX_RS08_FLASH_SIZE   (64)
#define MAX_DATA_SIZE         (MAX_PACKET_SIZE-1)


/** BDM command values
 *
 * The following values are the 1st byte in each command.  \n
 * Other parameters are as shown below. \n
 * Each command returns a status value (see \ref  USBDM_ErrorCode) as the first byte
 * followed by any results as indicated below.
 */
typedef enum {
   // Common to all targets
   CMD_USBDM_GET_COMMAND_RESPONSE  = 0,   /**< Status of last/current command */
   CMD_USBDM_SET_TARGET            = 1,   /**< Set target,  @param [2] 8-bit target value @ref TargetType_t */
   // Reserved 2
   CMD_USBDM_DEBUG                 = 3,   /**< Debugging commands (parameter determines actual command) @param [2]  Debug command see \ref DebugSubCommands */
   CMD_USBDM_GET_BDM_STATUS        = 4,   /**< Get BDM status\n @return [1] 16-bit status value reflecting BDM status */
   CMD_USBDM_GET_CAPABILITIES      = 5,   /**< Get capabilities of BDM, see \ref HardwareCapabilities_t */
   CMD_USBDM_SET_OPTIONS           = 6,   /**< Set BDM options, see \ref BDM_Options_t */
//   CMD_USBDM_GET_SETTINGS          = 7,   /**< Get BDM setting */
   // For USBDM the values 7..11 are Reserved
   CMD_USBDM_GET_VER               = 12,  /**< Sent to ep0 \n Get firmware version in BCD \n */
                                          /**< @return [1] 8-bit HW (major+minor) revision \n [2] 8-bit SW (major+minor) version number */
   // Reserved 13
   CMD_USBDM_ICP_BOOT              = 14,  /**< Sent to ep0 \n */
                                          /**< Requests reboot to ICP mode. @param [2..5] must be "BOOT" */
   CMD_SET_BOOT                    = 14,  /**< Deprecated - Previous version */

   // Target specific versions
   CMD_USBDM_CONNECT               = 15,  /**< Try to connect to the target */
   CMD_USBDM_SET_SPEED             = 16,  /**< Sets-up the BDM interface for a new bit rate & tries */
                                          /**    to enable ackn feature, @param [2..3] 16-bit tick count */
   CMD_USBDM_GET_SPEED             = 17,  /**< Read speed of the target: @return [1..2] 16-bit tick coun */

   CMD_USBDM_CONTROL_INTERFACE     = 18,  /**< Directly control BDM interface levels */
   // Reserved 19

   CMD_USBDM_READ_STATUS_REG       = 20,  /**< Get BDM status 
                                            * @return [1] 8-bit status byte made up as follows: \n 
                                            *    - (HC08/12/RS08/CFV1) bit0   - ACKN, \n 
                                            *    - (All)               bit1   - target was reset (this bit is cleared after reading),  \n 
                                            *    - (CFVx only)         bit2   - current RSTO value \n 
                                            *    - (HC08/12/RS08/CFV1) bit4-3 - comm status: 00=NOT CONNECTED, 01=SYNC, 10=GUESS,  11=USER SUPPLIED \n
                                            *    - (All)               bit7   - target has power 
					    */

   CMD_USBDM_WRITE_CONTROL_REG     = 21,  /**< Write to target Control register */

   CMD_USBDM_TARGET_RESET          = 22,  /**< Reset target @param [2] \ref TargetMode_t */
   CMD_USBDM_TARGET_STEP           = 23,  /**< Perform single step */
   CMD_USBDM_TARGET_GO             = 24,  /**< Start code execution */
   CMD_USBDM_TARGET_HALT           = 25,  /**< Stop the CPU and bring it into background mode */

   CMD_USBDM_WRITE_REG             = 26,  /**< Write to target register */
   CMD_USBDM_READ_REG              = 27,  /**< Read target register */

   CMD_USBDM_WRITE_CREG            = 28,  /**< Write target Core register */
   CMD_USBDM_READ_CREG             = 29,  /**< Read from target Core register */

   CMD_USBDM_WRITE_DREG            = 30,  /**< Write target Debufg register */
   CMD_USBDM_READ_DREG             = 31,  /**< Read from target Debug register */

   CMD_USBDM_WRITE_MEM             = 32,  /**< Write to target memory */
   CMD_USBDM_READ_MEM              = 33,  /**< Read from target memory */

   //CMD_USBDM_TRIM_CLOCK            = 34,  /**< Trim target clock - deleted in V3.2 */
   //CMD_USBDM_RS08_FLASH_ENABLE     = 35,  /**< Enable target flash programming (Vpp on) */
   //CMD_USBDM_RS08_FLASH_STATUS     = 36,  /**< Status of target flash programming */
   //CMD_USBDM_RS08_FLASH_DISABLE    = 37,  /**< Stop target flash programming (Vpp off) */

   CMD_USBDM_JTAG_GOTORESET        = 38,  /**< Reset JTAG Tap controller */
   CMD_USBDM_JTAG_GOTOSHIFT        = 39,  /**< Move JTAG TAP controller to SHIFT-IR/DR */
   CMD_USBDM_JTAG_WRITE            = 40,  /**< Write to JTAG chain */
   CMD_USBDM_JTAG_READ             = 41,  /**< Read from JTAG chain */
   CMD_USBDM_SET_VPP               = 42,  /**< Set VPP level */
   
/** TurboBdmLightCF related commands */
   CMD_TBLCF_GET_VER               = 10,  /**< TurboBdmLightCF Get version command */
   CMD_TBLCF_GET_LAST_STATUS       = 11,  /**< TurboBdmLightCF returns status of the previous command */
   CMD_TBLCF_SET_BOOT              = 12,  /**< request bootloader firmware upgrade on next power-up, parameters: 'B','O','O','T', returns: none */
   CMD_TBLCF_GET_STACK_SIZE        = 13,  /**< parameters: none, returns 16-bit stack size required by the application (so far into the execution) */
   /** TurboBdmLightCF  BDM/debugging related commands */
   CMD_TBLCF_SET_TARGET            = 20,  /**< set target, 8bit parameter: 00=ColdFire(default), 01=JTAG */
   CMD_TBLCF_TARGET_RESET          = 21,  /**< 8bit parameter: 0=reset to BDM Mode, 1=reset to Normal mode  @param [2] \ref TargetMode_t */
   CMD_TBLCF_GET_BDM_STATUS        = 22,  /**< returns 16bit status word: bit0 - target was reset since last execution of this command (this bit is cleared after reading), bit1 - current state of the RSTO pin, big endian! */
   CMD_TBLCF_TARGET_HALT           = 23,  /**< Stop the CPU and bring it into BDM mode */
   CMD_TBLCF_TARGET_GO             = CMD_USBDM_TARGET_GO,  /**< Start code execution from current PC address */
   CMD_TBLCF_TARGET_STEP           = 25,  /**< Perform single step */
   CMD_TBLCF_TARGET_RESYNCHRONIZE  = 26,  /**< resynchronize communication with the target (in case of noise, etc.) */
   CMD_TBLCF_TARGET_ASSERT_TA      = 27,  /**< parameter: 8-bit number of 10us ticks - duration of the TA assertion */
   /* CPU related commands */
   CMD_TBLCF_READ_REG              = 42,  /**< parameter 8-bit register number to read, returns 32-bit register contents */
   CMD_TBLCF_WRITE_REG             = 43,  /**< parameter 8-bit register number to write & the 32-bit register contents to be written */

   CMD_TBLCF_READ_CREG             = 44,  /**< parameter 16-bit register address, returns 32-bit control register contents */
   CMD_TBLCF_WRITE_CREG            = 45,  /**< parameter 16-bit register address & the 32-bit control register contents to be written */
   
   CMD_TBLCF_READ_DREG             = 46 ,  /**< parameter 8-bit register number to read, returns 32-bit debug module register contents */
   CMD_TBLCF_WRITE_DREG            = 47,   /**< parameter 8-bit register number to write & the 32-bit debug module register contents to be written */

   CMD_TBLCF_READ_MEM8             = 30,  /**< parameter 32bit address, returns 8bit value read from address */
   CMD_TBLCF_READ_MEM16            = 31,  /**< parameter 32bit address, returns 16bit value read from address */
   CMD_TBLCF_READ_MEM32            = 32,  /**< parameter 32bit address, returns 32bit value read from address */
   
   CMD_TBLCF_READ_MEMBLOCK8        = 33,  /**< parameter 32bit address, returns block of 8bit values read from the address and onwards */
   CMD_TBLCF_READ_MEMBLOCK16       = 34,  /**< parameter 32bit address, returns block of 16bit values read from the address and onwards */
   CMD_TBLCF_READ_MEMBLOCK32       = 35,  /**< parameter 32bit address, returns block of 32bit values read from the address and onwards */
   
   CMD_TBLCF_WRITE_MEM8            = 36,  /**< parameter 32bit address & an 8-bit value to be written to the address */
   CMD_TBLCF_WRITE_MEM16           = 37,  /**< parameter 32bit address & a 16-bit value to be written to the address */
   CMD_TBLCF_WRITE_MEM32           = 38,  /**< parameter 32bit address & a 32-bit value to be written to the address */
   
   CMD_TBLCF_WRITE_MEMBLOCK8       = 39,  /**< parameter 32bit address & a block of 8-bit values to be written from the address */
   CMD_TBLCF_WRITE_MEMBLOCK16      = 40,  /**< parameter 32bit address & a block of 16-bit values to be written from the address */
   CMD_TBLCF_WRITE_MEMBLOCK32      = 41,  /**< parameter 32bit address & a block of 32-bit values to be written from the address */

/** Generic OSBDM (Compatibility mode) */
   CMD_OSBDM_DEBUG                 =  8,  /**< Debugging commands (parameter determines actual command) @param [2]  Debug command see \ref DebugSubCommands */
   CMD_OSBDM_OPTION                = 10,  /**< Set/clear various options (only has affect after next \ref CMD_SET_TARGET) \n */
					  /**    @param [2]  Option command see \ref OptionCommands */
   CMD_OSBDM_GET_VER               = 12,  /**< Get firmware version in BCD @return [1] 8-bit HW (major+minor) revision \n [2] 8-bit SW (major+minor) version number */
   CMD_OSBDM_GET_LAST_STATUS       = 13,  /**< Get status from last command @return [0] 8-bit Error code see \ref  ErrorCodes */
   CMD_OSBDM_SET_BOOT              = 14,  /**< Requests reboot to ICP mode. @param [2..5] must be "BOOT" */
                              //   15 .. 29 Reserved for error codes
   CMD_OSBDM_SET_TARGET            = 30,  /**< Set target,  @param [2] 8-bit target value @ref target_type_e */
   /* OSBDM (Compatibility) BDM/debugging related commands */
   CMD_OSBDM_CONNECT               = 31,  /**< Try to connect to the target */
//   CMD_OSBDM_READ_SPEED            = 32,  /**< deprecated, speed has higher resolution now */
   CMD_OSBDM_TARGET_RESET          = 33,  /**< Reset target @param [2] 0=>reset to Special Mode, 1=>reset to Normal mode */

   CMD_OSBDM_GET_BDM_STATUS        = 34,  /**< Get BDM status 
                                            * @return [1] 8-bit status byte made up as follows: \n 
                                            *    - (HC08/12/RS08/CFV1) bit0   - ACKN, \n 
                                            *    - (All)               bit1   - target was reset (this bit is cleared after reading),  \n 
                                            *    - (CFVx only)         bit2   - current RSTO value \n 
                                            *    - (HC08/12/RS08/CFV1) bit4-3 - comm status: 00=NOT CONNECTED, 01=SYNC, 10=GUESS,  11=USER SUPPLIED \n 
                                            *    - (All)               bit7   - target has power 
			                    */
   CMD_OSBDM_READ_BD               = 35,  /**< Read from BDM address space @param [2..3] 16-bit address, @return [3] 8-bit value read from address; */
   CMD_OSBDM_WRITE_BD              = 36,  /**< Write to BDM address space @param [2..3] 16-bit address, @param [4] 8-bit value to write; */
   //CMD_OSBDM_GO                    = 37,  /**<  deprecated */
   //CMD_OSBDM_STEP                  = 38,  /**<  deprecated */
   CMD_OSBDM_HALT                  = 39,  /**< Stop the CPU and bring it into background mode */
   //CMD_OSBDM_SET_SPEED             = 40,  /**< deprecated */
   CMD_OSBDM_READ_SPEED            = 41,  /**< Read speed of the target: @return [1..2] 16-bit tick count see \ref SYNC_MULTIPLE */
   CMD_OSBDM_GO                    = 42,  /**< Start code execution */
   CMD_OSBDM_STEP                  = 44,  /**< Perform single step */
   CMD_OSBDM_SET_SPEED1            = 46,  /**< Sets-up the BDM interface for a new bit rate & tries 
                                            *  to enable ackn feature, @param [2..3] 16-bit tick count see \ref SYNC_MULTIPLE 
			                    */
   CMD_OSBDM_SET_DERIVATIVE        = 47,  /**< Set RS08 derivative @param [2] 8-bit derivative see \ref rs08_derivative_type_e */

   /* CPU related commands */
   CMD_OSBDM_READ_8                = 50,  /**< Read byte from memory @param [2..3] 16-bit address @return [1] 8-bit value read from address */
   CMD_OSBDM_READ_16               = 51,  /**< (HC12 only) Read word from memory @param [2..3] 16-bit address @return [1..2] 16-bit value read from address */
   //CMD_OSBDM_READ_BLOCK           /**< 52 deprecated */
   CMD_OSBDM_READ_REGS             = 53,  /**< Reads all target registers, @return [1..N] multiple 16-bit values: upper bytes are 0 when not used \n
                                            *    HC/S12(X): PC, SP, IX, IY, D(B:A), CCR;  \n
				            *    HCS08: PC, SP, H:X, A, CCR;  \n
				            *    RS08: CCR+PC, SPC, 0, A ; 
				            */
   //CMD_OSBDM_READ_BLOCK_FAST      /**< 54 deprecated */
   CMD_OSBDM_READ_BLOCK1           = 55,  /**< Read a block of memory @param [2..3] 16-bit address, @param [4] 8-bit count of bytes to read, 
                                           * @return [1..N] block of bytes from given address, 
                                           *    count MUST be <= \ref MAX_PACKET_SIZE-1 
                                           */
   CMD_OSBDM_WRITE_8               = 60,  /**< Write 8-bit value to memory @param [2..3] 16-bit address @param [4] 8-bit value to write */
   CMD_OSBDM_WRITE_16              = 61,  /**< Write 16-bit value to memory @param [2..3] 16-bit address @param [4..5] 16-bit value to write */
   //CMD_OSBDM_WRITE_BLOCK          /**< 62 deprecated */
   //CMD_OSBDM_WRITE_REGS           /**< 63 deprecated */
   //CMD_OSBDM_WRITE_BLOCK_FAST     /**< 64 deprecated */
   CMD_OSBDM_WRITE_BLOCK1          = 65,  /**< Write a block to memory @param [2..3] 16-bit address @param [4] 8-bit count of bytes, 
                                           *   @param [5..N+4] block of bytes to write, \n 
                                           *    count MUST be <= \ref MAX_PACKET_SIZE-5 
					   */
   // Coldfire V1
   CMD_CF_READ_CSR2     = 86, //!< Read CFv1 CSR2 @return [1] 8-bit value
   CMD_CF_READ_CSR3     = 87, //!< Read CFv1 CSR3 @return [1] 8-bit value
   CMD_CF_WRITE_CSR2    = 88, //!< Write CFv1 CSR2 @param [2] value
   CMD_CF_WRITE_CSR3    = 89, //!< Write CFv1 CSR3 @param [2] value
   CMD_CF_READ_REG      = 90, //!< Read CFv1 Register @param [2] 8-bit register # @return [1..4] 32-bit value
   CMD_CF_READ_CREG     = 91, //!< Read CFv1 Control register @param [2] 8-bit register # @return [1..4] 32-bit value
   CMD_CF_READ_DREG     = 92, //!< Read CFv1 Debug register @param [2] 8-bit reg # @return [1..4] 32-bit value
   CMD_CF_READ_XCSR     = 93, //!< Read CFv1 Extended status register @return [1] 8-bit value
   CMD_CF_WRITE_REG     = 94, //!< Write CFv1 Register @param [2] 8-bit reg # @param [3..6] 32-bit value
   CMD_CF_WRITE_CREG    = 95, //!< Write CFv1 Control register @param [2] 8-bit reg # @param [3..6] 32-bit value
   CMD_CF_WRITE_DREG    = 96, //!< Write CFv1 Debug register @param [2] 8-bit reg # @param [3..6] 32-bit value
   CMD_CF_READ_MEM      = 97, //!< Read CFv1 Memory @param [2..4] 24-bit address @param [5] 8-bit data unit size @param block size is implied by USB transfer size
                              //!< @return [1..N] block of bytes from given address,
                              //!<    block size MUST be <= \ref MAX_PACKET_SIZE-1
   CMD_CF_WRITE_MEM     = 98, //!< Write CFv1 Memory @param [2..4] 24-bit address @param [5] 8-bit data unit size (1,2,4) @param [6] 8-bit block size @param [7..N+6] block of data,
                              //!<    block size MUST be <= \ref MAX_PACKET_SIZE-7
   CMD_CF_WRITE_XCSR    = 99, //!< Write CFv1 Extended Control register @param [2] 8-bit value

} bdm_commands_type_e;

/** Debugging sub commands (used with \ref CMD_USBDM_DEBUG )
 * @note Not for general use! (Dangerous - don't try turning on VPP with the wrong chip!)
 */
typedef enum  {
  BDM_DBG_ACKN             = 0,  /**< - Test ACKN */
  BDM_DBG_SYNC             = 1,  /**< - Test SYNC */
  BDM_DBG_TESTPORT         = 2,  /**< - Test BDM port timing */
  BDM_DBG_USBDISCONNECT    = 3,  /**< - Test USB disconnect (don't use!) */
  BDM_DBG_STACKSIZE        = 4,  /**< - Determine stack size */
  BDM_DBG_VPP_OFF          = 5,  /**< - Remove Flash programming voltage from target */
  BDM_DBG_VPP_ON           = 6,  /**< - Apply Flash programming voltage to target */
  BDM_DBG_FLASH12V_OFF     = 7,  /**< - Turn 12V flash programming voltage source off */
  BDM_DBG_FLASH12V_ON      = 8,  /**< - Turn 12V flash programming voltage source on */
  BDM_DBG_VDD_OFF          = 9,  /**< - Turn Target Vdd supply off */
  BDM_DBG_VDD3_ON          = 10, /**< - Set  Target Vdd supply to 3V3 */
  BDM_DBG_VDD5_ON          = 11, /**< - Set Target Vdd supply to 5V */
  BDM_DBG_CYCLE_POWER      = 12, /**< - Cycle Target Vdd supply off and on */
  BDM_DBG_MEASURE_VDD      = 13, /**< - Measure Target Vdd supply */
  BDM_DBG_RS08TRIM         = 14, /**< - Calculate RS08 clock trim value */
  BDM_DBG_TESTWAITS        = 15, /**< - Tests the software counting delays used for BDM communication. (locks up BDM!) */
  BDM_DBG_TESTALTSPEED     = 16, /**< - Test bdmHC12_alt_speed_detect() */
} bdm_debug_commands_type_e;

/** Commands for BDM when in ICP mode
 */
// typedef enum {
//    ICP_GET_RESULT     =  1,            /**< Get result of last command */
//                                       /**< @return [0] 8-bit Error code, see \ref  ICP_ErrorCode_t */
//    ICP_ERASE_PAGE    =  2,            /**< Erase page (must be within a single Flash memory page) */
//                                       /**<   @param 16-bit Address within Flash page to erase */
//    ICP_PROGRAM_ROW   =  3,            /**< Program row (must be within a single Flash memory row) */
//                                       /**<   @param [0..1] 16-bit Address within Flash page to program */
//                                       /**<   @param [2..3] 16-bit Number of bytes to program */
//                                       /**<   @param [4..N] data to program */
//    ICP_VERIFY_ROW    =  4,            /**< Verify row */
//                                       /**<   @param [0..1] 16-bit Address within Flash page to verify */
//                                       /**<   @param [2..3] 16-bit Number of bytes to verify */
//                                       /**<   @param [4..N] data to verify */
//    ICP_REBOOT        =  5,            /**< Reboot device - device immediately reboots so contact is lost! */
//    ICP_GET_VER       =  CMD_USBDM_GET_VER,  /**< Get version - must be common to both modes */
//                                       /**< @return [0] 16-bit Version number major.minor */
//                                       /**< @return Error code, see \ref  ICP_ErrorCode_t */
// } bdm_icp_commands_codes_type_e;

typedef enum  {
  BDM_RC_OK                      = 0,     /**< - No error */
  BDM_RC_ILLEGAL_PARAMS          = 1,     /**< - Illegal parameters to command */
  BDM_RC_FAIL                    = 2,     /**< - General Fail */
  BDM_RC_BUSY                    = 3,     /**< - Busy with last command - try again - don't change */
  BDM_RC_ILLEGAL_COMMAND         = 4,     /**< - Illegal (unknown) command (may be in wrong target mode) */
  BDM_RC_NO_CONNECTION           = 5,     /**< - No connection to target */
  BDM_RC_OVERRUN                 = 6,     /**< - New command before previous command completed */
  BDM_RC_CF_ILLEGAL_COMMAND      = 7,     /**< - BDM Interface did not recognize the command */
  //
  BDM_RC_UNKNOWN_TARGET          = 15,    /**< - Target not supported */
  BDM_RC_NO_TX_ROUTINE           = 16,    /**< - No Tx routine available at measured BDM communication speed */
  BDM_RC_NO_RX_ROUTINE           = 17,    /**< - No Rx routine available at measured BDM communication speed */
  BDM_RC_BDM_EN_FAILED           = 18,    /**< - Failed to enable BDM mode in target */
  BDM_RC_RESET_TIMEOUT_FALL      = 19,    /**< - RESET signal failed to fall */
  BDM_RC_BKGD_TIMEOUT            = 20,    /**< - BKGD signal failed to rise/fall */
  BDM_RC_SYNC_TIMEOUT            = 21,    /**< - No response to SYNC sequence */
  BDM_RC_UNKNOWN_SPEED           = 22,    /**< - Communication speed is not known or cannot be determined */
  BDM_RC_WRONG_PROGRAMMING_MODE  = 23,    /**< - Attempted Flash programming when in wrong mode (e.g. Vpp off) */
  BDM_RC_FLASH_PROGRAMING_BUSY   = 24,    /**< - Busy with last Flash programming command */
  BDM_RC_VDD_NOT_REMOVED         = 25,    /**< - Target Vdd failed to fall */
  BDM_RC_VDD_NOT_PRESENT         = 26,    /**< - Target Vdd not present/failed to rise */
  BDM_RC_VDD_WRONG_MODE          = 27,    /**< - Attempt to cycle target Vdd when not controlled by BDM interface */
  BDM_RC_CF_BUS_ERROR            = 28,    /**< - Illegal bus cycle on target (Coldfire) */
  BDM_RC_USB_ERROR               = 29,    /**< - Indicates USB transfer failed (returned by driver not BDM) */
  BDM_RC_ACK_TIMEOUT             = 30,    /**< - Indicates an expected ACK was missing */
  BDM_RC_FAILED_TRIM             = 31,    /**< - Trimming of target clock failed (out of clock range?). */
  BDM_RC_FEATURE_NOT_SUPPORTED   = 32,    /**< - Feature not supported by this version of hardware/firmware */
  BDM_RC_RESET_TIMEOUT_RISE      = 33,    /**< - RESET signal failed to rise */
} bdm_error_code_type_e;

/** Target Status bit masks for USBDM/OSBDM
  *     9       8       7       6       5        4       3       2       1       0
  * +-------+-------+-------+-------+--------+-------+-------+-------+-------+-------+
  * |      VPP      |     Power     |  Halt  | Communication | Reset | ResDet| Ackn  |
  * +-------+-------+-------+-------+--------+-------+-------+-------+-------+-------+
  */
typedef enum  {
  TBLCF_RESET_DETECTED_MASK = (1<<0),  /**< - TBLCF Target Detected mask */
  TBLCF_RST0_STATE_MASK     = (1<<1),  /**< - TBLCF Target RST0 State mask */
  USBDM_ACKN                = (1<<0),  /**< - Target supports BDM ACK */
  USBDM_RESET_DETECT        = (1<<1),  /**< - Target has been reset since status last polled */
  USBDM_RESET_STATE         = (1<<2),  /**< - Current state of target reset pin (RESET or RSTO) (active low!) */
  USBDM_NOT_CONNECTED       = (0<<3),  /**< - No connection with target */
  USBDM_SYNC_DONE           = (1<<3),  /**< - Target communication speed determined by BDM SYNC */
  USBDM_GUESS_DONE          = (2<<3),  /**< - Target communication speed guessed */
  USBDM_USER_DONE           = (3<<3),  /**< - Target communication speed specified by user */
  USBDM_COMM_MASK           = (3<<3),  /**< - Mask for communication state (S_NOT_CONNECTED, S_SYNC_DONE, S_GUESS_DONE or S_USER_DONE) */
  USBDM_HALT                = (1<<5),  /**< - Indicates target is halted (CF V2, V3 & V4) - buggy? */
  USBDM_POWER_NONE          = (0<<6),  /**< - Target power not present */
  USBDM_POWER_EXT           = (1<<6),  /**< - External target power present */
  USBDM_POWER_INT           = (2<<6),  /**< - Internal target power on */
  USBDM_POWER_ERR           = (3<<6),  /**< - Internal target power error - overcurrent or similar */
  USBDM_POWER_MASK          = (3<<6),  /**< - Mask for Power */
  USBDM_VPP_OFF             = (0<<8),  /**< - Vpp Off */
  USBDM_VPP_STANDBY         = (1<<8),  /**< - Vpp standby (Inverter on) */
  USBDM_VPP_ON              = (2<<8),  /**< - Vpp On */
  USBDM_VPP_ERR             = (3<<8),  /**< - Vpp Error - not used */
  USBDM_VPP_MASK            = (3<<8),  /**< - Mask for Vpp */
} status_bitmask_type_e;

/* if command fails, the device responds with command code CMD_FAILED */
/* if command succeeds, the device responds with the same command number followed by any results as appropriate */

/* JTAG commands */
#define CMD_JTAG_GOTORESET    80 /* no parameters, takes the TAP to TEST-LOGIC-RESET state, re-select the JTAG target to take TAP back to RUN-TEST/IDLE */
#define CMD_JTAG_GOTOSHIFT    81 /* parameters 8-bit path option; path option ==0 : go to SHIFT-DR, !=0 : go to SHIFT-IR (requires the tap to be in RUN-TEST/IDLE) */
#define CMD_JTAG_WRITE        82 /* parameters 8-bit exit option, 8-bit count of bits to shift in, and the data to be shifted in (shifted in LSB (last byte) first, unused bits (if any) are in the MSB (first) byte; exit option ==0 : stay in SHIFT-xx, !=0 : go to RUN-TEST/IDLE when finished */
#define CMD_JTAG_READ         83 /* parameters 8-bit exit option, 8-bit count of bits to shift out; exit option ==0 : stay in SHIFT-xx, !=0 : go to RUN-TEST/IDLE when finished, returns the data read out of the device (first bit in LSB of the last byte in the buffer) */

/* Comments:


*/

#endif /* _COMMANDS_H_ */