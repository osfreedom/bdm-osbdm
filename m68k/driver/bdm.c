/*
 * Motorola Background Debug Mode Driver
 * Copyright (C) 1995  W. Eric Norum
 * Copyright (C) 1998  Chris Johns
 *
 * Based on:
 *  1. `A Background Debug Mode Driver Package for Motorola's
 *     16- and 32-Bit Microcontrollers', Scott Howard, Motorola
 *     Canada, 1993.
 *  2. `Linux device driver for public domain BDM Interface',
 *     M. Schraut, Technische Universitaet Muenchen, Lehrstuhl
 *     fuer Prozessrechner, 1995.
 * 
 * Extended to support the ColdFire BDM interface using the P&E
 * module which comes with the EVB. Currently only tested with the
 * 5206 (5V) device.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * W. Eric Norum
 * Saskatchewan Accelerator Laboratory
 * University of Saskatchewan
 * 107 North Road
 * Saskatoon, Saskatchewan, CANADA
 * S7N 5C6
 * 
 * eric@skatter.usask.ca
 *
 * Coldfire support by:
 * Chris Johns
 * Objective Design Systems Pty Ltd,
 * http://www.cybertec.com.au/
 *
 * ccj@acm.org
 *
 * 19/1/1999 -- Greg Ungerer (gerg@moreton.com.au)
 *     Added support for more recent LINUX kernels 2.2.0+
 *
 * 22/10/1999 -- CCJ (ccj@acm.org)
 *     Remove the OS specific parts to allow for easier support for
 *     different OS's. This is based on the work done by
 *     David McCullough (davidm@stallion.oz.au) for the SCO port.
 *
 * 31/05/2000 - CCJ (ccj@acm.org)
 *     Merged in the ICD interface, and renamed the Eric interface
 *     to the PD interface.
 *
 * 01/09/2000 - ASA (aaganichev@hypercom.com)
 *     Fixed ICD and PD support basing on original driver code, fix ICD name
 *     misspelling.
 *
 * 14/11/2000 - James Housley <jim@thehousleys.net>
 *              Greg Tunnock (gtunnock@redfernnetworks.com)
 *     Port to FreeBSD.
 *
 * 02/02/2001 - KJO (vac4050@cae597.rsc.raytheon.com)
 *     Reduced time delays in some cpu32_icd_xxx() functions to improve
 *     performance of cpu32 ICD driver and speed up downloads.
 *
 * 12/12/2001 - Thomas Andrew (tandrews@mindspring.co.za)
 *     Change to stop getting "Invalid instruction (0x1FFFF) back from the
 *     ICD interface.
 */

/*
 * Declarations common to kernel and user code
 */
#include "bdm.h"

/*
 ************************************************************************
 *     BDM driver control structure                                     *
 ************************************************************************
 */

struct BDM {
  /*
   * Device name, processor and interface type
   */
  char name[20];
  int  processor;
  int  interface;

  /*
   * Device I/O ports
   */
  int  exists;
  int  portsAreMine;
  int  portBase;
  int  dataPort;
  int  statusPort;
  int  controlPort;

  /*
   * Control debugging messages
   */
  int  debugFlag;

  /*
   * Device is exclusive-use
   */
  int  isOpen;

  /*
   * Serial I/O delay timer
   */
  int  delayTimer;

  /*
   * I/O buffer
   */
  char         ioBuffer[512];
  unsigned int readValue;
  
  /*
   * Interface specific handlers
   */
  int (*get_status)(struct BDM *self);
  int (*init_hardware) (struct BDM *self);
  int (*serial_clock) (struct BDM *self, unsigned short wval, int holdback);
  int (*read_sysreg) (struct BDM *self, struct BDMioctl *ioc);
  int (*write_sysreg) (struct BDM *self, struct BDMioctl *ioc);
  int (*gen_bus_error) (struct BDM *self);
  int (*restart_chip) (struct BDM *self);
  int (*release_chip) (struct BDM *self);
  int (*reset_chip) (struct BDM *self);
  int (*stop_chip) (struct BDM *self);
  int (*run_chip) (struct BDM *self);
  int (*step_chip) (struct BDM *self);

  /*
   * Some system registers are write only so we need to shadow them
   */
  unsigned long shadow_sysreg[BDM_MAX_SYSREG];

  /*
   * Coldfire processors have different versions of the debug module.
   * These also require special operations to occur.
   */
  int           cf_debug_ver;
  int           cf_use_pst;
  
  /*
   * Need to get the status from the csr, how-ever the status bits are a
   * once read then clear so we need logic to store this state to know
   * the status.
   */
  int           cf_running;
  unsigned long cf_csr;

#ifdef BDM_BIT_BASH_PORT
  
  int (*bit_bash) (struct BDM *self, unsigned short mask, unsigned short value);

  /*
   * The current state of the bit bash bits. Easier than figuring out
   * if the bit should be on or off by reading the PC port. I am sure
   * this could be done, but I am not a PC parallel port expert.
   */

  unsigned short bit_bash_bits;

#endif
};

/*
 ************************************************************************
 *     Common Functions                                                 *
 ************************************************************************
 */

static int bdmDrvGetStatus (struct BDM *self);
static int bdmDrvInitHardware (struct BDM *self);
static int bdmDrvRestartChip (struct BDM *self);
static int bdmDrvReleaseChip (struct BDM *self);
static int bdmDrvResetChip (struct BDM *self);
static int bdmDrvStopChip (struct BDM *self);
static int bdmDrvSerialClock (struct BDM *self, unsigned short wval, int holdback);
static int bdmDrvStepChip (struct BDM *self);
static int bdmDrvSendCommandTillTargetReady (struct BDM *self,
                                          unsigned short command);
static int bdmDrvFillBuf (struct BDM *self, int count);
static int bdmDrvSendBuf (struct BDM *self, int count);
static int bdmDrvFetchWord (struct BDM *self, unsigned short *sp);
static int bdmDrvGo (struct BDM *self);
static int bdmDrvReadSystemRegister (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvReadProcessorRegister (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvReadLongWord (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvReadWord (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvReadByte (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvWriteSystemRegister (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvWriteProcessorRegister (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvWriteLongWord (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvWriteWord (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvWriteByte (struct BDM *self, struct BDMioctl *ioc);

/*
 ************************************************************************
 *     BDM Interfaces                                                   *
 ************************************************************************
 */

/*
 * PD (Eric's) CPU32 Interface
 *
 * Parallel port bit assignments
 *
 * Status register (bits 0-2 not used):
 * +---+---+---+---+---+---+---+---+
 * | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * +---+---+---+---+---+---+---+---+
 *   |   |   |   |   |
 *   |   |   |   |   +--- Target FREEZE line
 *   |   |   |   |     1 - Target is in background mode
 *   |   |   |   |     0 - Target is not background mode
 *   |   |   |   |
 *   |   |   |   +------- Not used
 *   |   |   |
 *   |   |   +----------- Serial data from target
 *   |   |  1 - `0' from target
 *   |   |  0 - `1' from target
 *   |   |
 *   |   +--------------- Target power
 *   |      1 - Target power is ON
 *   |      0 - Target power is OFF
 *   |
 *   +------------------- Target connected
 *    1 - Target is connected
 *    0 - Target is not connected
 *
 * Control register (bits 4-7 not used):
 * +---+---+---+---+
 * | 3 | 2 | 1 | 0 |
 * +---+---+---+---+
 *   |   |   |   |
 *   |   |   |   +--- Target BKPT* /DSCLK line
 *   |   |   |     Write 1 - Drive BKPT* /DSCLK line LOW
 *   |   |   |     Write 0 - Allow BKPT* /DSCLK line to go HIGH
 *   |   |   |   Allow flip-flop to control BKPT* /DSCLK line
 *   |   |   |
 *   |   |   +------- Target RESET* line
 *   |   |   Write 1 - Force RESET* LOW
 *   |   |   Write 0 - Allow monitoring of RESET*
 *   |   |       Read 1 - RESET* is LOW
 *   |   |       Read 0 - RESET* is HIGH
 *   |   |
 *   |   +----------- Serial data to target
 *   |  Write 0 - Send `0' to target
 *   |  Write 1 - Send `1' to target
 *   |
 *   +--------------- Control single-step flip-flop
 *      Write 1 - Clear flip-flop
 *    BKPT* /DSCLK is controlled by bit 0.
 *      Write 0 - Allow flip-flop operation
 *    Falling edge of IFETCH* /DSI clocks a `1'
 *    into the flip-flop and drive BKPT* /DSCLK
 *    LOW, causing a breakpoint.
 */
#define CPU32_PD_SR_CONNECTED        (0x80)
#define CPU32_PD_SR_POWERED          (0x40)
#define CPU32_PD_SR_DATA_BAR         (0x20)
#define CPU32_PD_SR_FROZEN           (0x08)

#define CPU32_PD_CR_NOT_SINGLESTEP   (0x08)
#define CPU32_PD_CR_DATA             (0x04)
#define CPU32_PD_CR_FORCE_RESET      (0x02)
#define CPU32_PD_CR_RESET_STATUS     (0x02)
#define CPU32_PD_CR_CLOCKBAR_BKPT    (0x01)

/*
 * ICD interface.
 *
 * Parallel port bit assignments
 *
 * Status register 
 * +---+---+---+---+---+---+---+---+
 * | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * +---+---+---+---+---+---+---+---+
 *   |   |   |   |   |
 *   |   |   |   |   +--- Not used
 *   |   |   |   +------- Not used
 *   |   |   +----------- Not used
 *   |   |
 *   |   +--------------- Target FREEZE line
 *   |                        1 - Target is in background mode
 *   |                        0 - Target is not background mode
 *   |
 *   +------------------- Serial data from target
 *                            1 - `0' from target
 *                            0 - `1' from target
 *        
 * Data register 
 * +---+---+---+---+---+---+---+---+
 * | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * +---+---+---+---+---+---+---+---+
 *   |   |   |   |   |   |   |   |
 *   |   |   |   |   |   |   |   +---  Serial data to target
 *   |   |   |   |   |   |   |            Write 1: Send 1 to target
 *   |   |   |   |   |   |   |            Write 0: Send 0 to target
 *   |   |   |   |   |   |   |            Signal gets to target, if OE is 1
 *   |   |   |   |   |   |   |            and target is in FREEZE mode
 *   |   |   |   |   |   |   |
 *   |   |   |   |   |   |   +-------  Clock 
 *   |   |   |   |   |   |                if target in freeze mode, then:
 *   |   |   |   |   |   |                Write 1: drive BKPT* /DSCLK 1
 *   |   |   |   |   |   |                Write 0: drive BKPT* /DSCLK 0
 *   |   |   |   |   |   |
 *   |   |   |   |   |   +-----------  BREAK
 *   |   |   |   |   |                    if target not in freeze mode, then:
 *   |   |   |   |   |                    Write 0: drive BKPT* /DSCLK 0
 *   |   |   |   |   |                    line determines single stepping
 *   |   |   |   |   |                    on leaving BGND mode:
 *   |   |   |   |   |                    Write 0: do single step
 *   |   |   |   |   |                    Write 1: continue normally
 *   |   |   |   |   |
 *   |   |   |   |   +---------------  RESET
 *   |   |   |   |                        Write 0: pull reset low
 *   |   |   |   |                        Write 1: release reset line
 *   |   |   |   |
 *   |   |   |   +--- OE
 *   |   |   |           Write 0 - DSI is tristated
 *   |   |   |           Write 1 - DSI pin is forced to level of serial data
 *   |   |   |
 *   |   |   +------- LED
 *   |   |               Write 1 - turn on LED
 *   |   |               Write 0 - turn off LED
 *   |   |
 *   |   +----------- ERROR
 *   |                   Write 0 - BERR output is tristated
 *   |                   Write 1 - BERR is pulled low
 *   |
 *   +--------------- spare
 */

#define CPU32_ICD_DSI        (1 << 0)    /* data shift input  Host->MCU        */
#define CPU32_ICD_DSCLK      (1 << 1)    /* data shift clock / breakpoint pin  */
#define CPU32_ICD_STEP_OUT   (1 << 2)    /* set low to force breakpoint        */
#define CPU32_ICD_RST_OUT    (1 << 3)    /* set low to force reset on MCU      */
#define CPU32_ICD_OE         (1 << 4)    /* set to a 1 to enable DSI           */
#define CPU32_ICD_FORCE_BERR (1 << 6)    /* set to a 1 to force BERR on target */
#define CPU32_ICD_FREEZE     (1 << 6)    /* */
#define CPU32_ICD_DSO        (1 << 7)    /* */

/*
 * Coldfire P&E Interface.
 *
 * Parallel port bit assignments 
 *
 * Data register
 * +---+---+---+---+---+---+---+---+
 * | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * +---+---+---+---+---+---+---+---+
 *   |   |   |   |   |   |   |   |
 *   |   |   |   |   |   |   |   +--- Serial data to target
 *   |   |   |   |   |   |   |    Write 0 - Send `0' to target
 *   |   |   |   |   |   |   |    Write 1 - Send `1' to target
 *   |   |   |   |   |   |   |
 *   |   |   |   |   |   |   +------- Serial Data clock
 *   |   |   |   |   |   |  0 = clock low
 *   |   |   |   |   |   |  1 = clock high
 *   |   |   |   |   |   |
 *   |   |   |   |   |   +----------- Target Breakpoint
 *   |   |   |   |   |      0 = brkp active (low)
 *   |   |   |   |   |      1 = brkp negated (high)
 *   |   |   |   |   |
 *   |   |   |   |   +--------------- Target Reset
 *   |   |   |   |    0 = reset active (low)
 *   |   |   |   |    1 = reset negated (target active)
 *   |   |   |   |
 *   |   |   |   +------------------- Not Used
 *   |   |   |
 *   |   |   +----------------------- Not used
 *   |   |
 *   |   +--------------------------- TEA
 *   | 
 *   +------------------------------- Not used
 *
 * Status register (bits 0-2 not used):
 * +---+---+---+---+---+---+---+---+
 * | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * +---+---+---+---+---+---+---+---+
 *   |   |   |   |   |
 *   |   |   |   |   +--- PST1
 *   |   |   |   |     1 - ???
 *   |   |   |   |     0 - ???
 *   |   |   |   |
 *   |   |   |   +------- Target power
 *   |   |   |    1 - Target power is ON
 *   |   |   |    0 - Target power is OFF
 *   |   |   |
 *   |   |   +----------- Not used
 *   |   |
 *   |   +--------------- Halted
 *   |      1 - Target is halted (PST0 | PST1 | PST1 | PST3)
 *   |      0 - Target is running
 *   |
 *   +------------------- Serial data from target (DSO)
 *    1 - `0' from target
 *    0 - `1' from target
 *
 * Control register (bits 4-7 not used):
 * +---+---+---+---+
 * | 3 | 2 | 1 | 0 |
 * +---+---+---+---+
 *   |   |   |   |
 *   |   |   |   +--- Target /PST0
 *   |   |   |     Write 1 - /PST0 line LOW
 *   |   |   |     Write 0 - /PST0 line to go HIGH
 *   |   |   |
 *   |   |   +------- Target PST2
 *   |   |   Write 1 - /PST2 line high
 *   |   |   Write 0 - /PST2 line low
 *   |   |
 *   |   +----------- Target /PST3
 *   |  Write 0 - /PST3 line LOW
 *   |  Write 1 - /PST3 line to go HIGH
 *   |
 *   +--------------- Not Used
 */

#define CF_PE_DR_DATA_IN    0x01
#define CF_PE_DR_CLOCK_HIGH 0x02
#define CF_PE_DR_BKPT       0x04
#define CF_PE_DR_RESET      0x08
#define CF_PE_DR_TEA        0x40

#define CF_PE_DR_MASK       (CF_PE_DR_DATA_IN | CF_PE_DR_CLOCK_HIGH \
                             | CF_PE_DR_BKPT | CF_PE_DR_RESET | CF_PE_DR_TEA)

#define CF_PE_MAKE_POS_DR(flags) ((flags) & (CF_PE_DR_DATA_IN | CF_PE_DR_CLOCK_HIGH))
#define CF_PE_MAKE_NEG_DR(flags) ((~(flags)) & (CF_PE_DR_BKPT | CF_PE_DR_RESET | CF_PE_DR_TEA))
#define CF_PE_MAKE_DR(flags)     ((CF_PE_MAKE_POS_DR(flags) | CF_PE_MAKE_NEG_DR(flags)) & \
                                  CF_PE_DR_MASK)

#define CF_PE_SR_PST1       0x08
#define CF_PE_SR_POWERED    0x10
#define CF_PE_SR_FROZEN     0x40  /* do not use this, require SIM setup */
#define CF_PE_SR_DATA_OUT   0x80

#define CF_PE_CR_NOT_PST0   0x01
#define CF_PE_CR_NOT_PST2   0x02
#define CF_PE_CR_NOT_PST3   0x04

/*
 * Common processor defines.
 */

/*
 * CPU Generated Messages.
 */

#define CGM_FAILURE         1       /* bit 16, s/c bit */
#define CGM_CMD_OK          0xffff
#define CGM_NOT_READY       0x0000
#define CGM_BUS_ERROR       0x0001
#define CGM_ILLEGAL_CMD     0xffff

/*
 * Command codes for BDM interface
 */

#define BDM_RREG_CMD        0x2180  /* CPU32, Coldfire */
#define BDM_WREG_CMD        0x2080  /* CPU32, Coldfire */
#define BDM_RSREG_CMD       0x2580  /* CPU32, Coldfire */
#define BDM_WSREG_CMD       0x2480  /* CPU32, Coldfire */
#define BDM_READ_CMD        0x1900  /* CPU32, Coldfire */
#define BDM_WRITE_CMD       0x1800  /* CPU32, Coldfire */
#define BDM_DUMP_CMD        0x1d00  /* CPU32, Coldfire */
#define BDM_FILL_CMD        0x1c00  /* CPU32, Coldfire */
#define BDM_GO_CMD          0x0c00  /* CPU32, Coldfire */
#define BDM_CALL_CMD        0x0800  /* CPU32 */
#define BDM_RST_CMD         0x0400  /* CPU32, Coldfire */
#define BDM_NOP_CMD         0x0000  /* CPU32, Coldfire */
#define BDM_RCREG_CMD       0x2980  /* Coldfire */
#define BDM_WCREG_CMD       0x2880  /* Coldfire */
#define BDM_RDMREG_CMD      0x2d80  /* Coldfire */
#define BDM_WDMREG_CMD      0x2c80  /* Coldfire */

/*
 * Operand size for BDM_READ_CMD/BDM_WRITE_CMD
 */

#define BDM_SIZE_BYTE       0x0000
#define BDM_SIZE_WORD       0x0040
#define BDM_SIZE_LONG       0x0080

/*
 * Address/Data field
 */

#define BDM_ADDR_NOT_DATA   0x0004

/*
 * Per-device data.
 * Allocate a struct for each port for each interface.
 */
static struct BDM bdm_device_info[BDM_NUM_OF_MINORS];

/*
 ************************************************************************
 *     CPU32 for PD/ICD interface support routines                      *
 ************************************************************************
 */

/*
 * CPU32 system register mapping. See bdm.h for the user values.
 */

static int cpu32_sysreg_map[BDM_REG_VBR + 1] =
{ 0x0,      /* BDM_REG_RPC   */
  0x1,      /* BDM_REG_PCC   */
  0xb,      /* BDM_REG_SR    */
  0xc,      /* BDM_REG_USP   */
  0xd,      /* BDM_REG_SSP   */
  0xe,      /* BDM_REG_SFC   */
  0xf,      /* BDM_REG_DFC   */
  0x8,      /* BDM_REG_ATEMP */
  0x9,      /* BDM_REG_FAR   */
  0xa       /* BDM_REG_VBR   */
};

/* need by cpu32_read_sysreg() */
static int cpu32_write_sysreg (struct BDM *self, struct BDMioctl *ioc);
static int cpu32_icd_stop_chip (struct BDM *self);

/*
 * Clock a word to/from the target
 */

static int
cpu32_serial_clock (struct BDM *self, unsigned short wval, int holdback)
{
  return (self->serial_clock) (self, wval, holdback);
}

/*
 * Get target status
 */
static int
cpu32_pd_get_status (struct BDM *self)
{
  unsigned char sr = inb (self->statusPort);
  int           ret;

  if (!(sr & CPU32_PD_SR_CONNECTED))
    ret = BDM_TARGETNC;
  else if (!(sr & CPU32_PD_SR_POWERED))
    ret = BDM_TARGETPOWER;
  else
    ret = (sr & CPU32_PD_SR_FROZEN ? BDM_TARGETSTOPPED : 0) |
      (inb (self->controlPort) & CPU32_PD_CR_RESET_STATUS ? BDM_TARGETRESET : 0);
  if (self->debugFlag > 1)
    PRINTF (" cpu32_pd_get_status -- Status Port:0x%02x  Status:0x%04x\n",
            sr, ret);
  return ret;
}

/*
 * Hardware initialization
 */
static int
cpu32_pd_init_hardware (struct BDM *self)
{
  int status;

  /*
   * Force breakpoint
   */
  outb (CPU32_PD_CR_NOT_SINGLESTEP | CPU32_PD_CR_CLOCKBAR_BKPT, self->controlPort);
  udelay (100);
  outb (CPU32_PD_CR_FORCE_RESET | CPU32_PD_CR_CLOCKBAR_BKPT, self->controlPort);
  bdm_sleep (HZ / 100);
  outb (CPU32_PD_CR_CLOCKBAR_BKPT, self->controlPort);
  udelay (10);
  outb (CPU32_PD_CR_NOT_SINGLESTEP | CPU32_PD_CR_CLOCKBAR_BKPT, self->controlPort);
  udelay (100);
  status = cpu32_pd_get_status (self);
  if (self->debugFlag)
    PRINTF (" cpu32_pd_init_hardware: status:0x%02x control port:0x%02x\n",
            status, inb (self->controlPort));
  return status;
}

/*
 * Clock a word to/from the target
 */
static int
cpu32_pd_serial_clock (struct BDM *self, unsigned short wval, int holdback)
{
  unsigned long shiftRegister;
  unsigned char dataBit;
  unsigned int  counter;
  unsigned int  status = cpu32_pd_get_status (self);

  if (status & BDM_TARGETRESET)
    return BDM_FAULT_RESET;
  if (status & BDM_TARGETNC)
    return BDM_FAULT_CABLE;
  if (status & BDM_TARGETPOWER)
    return BDM_FAULT_POWER;
  shiftRegister = wval;
  counter = 17 - holdback;
  while (counter--) {
    dataBit = ((shiftRegister & 0x10000) ? CPU32_PD_CR_DATA : 0);
    shiftRegister <<= 1;
    outb (dataBit | CPU32_PD_CR_NOT_SINGLESTEP | CPU32_PD_CR_CLOCKBAR_BKPT,
          self->controlPort);
    bdm_delay (self->delayTimer + 1);
    if ((inb (self->statusPort) & CPU32_PD_SR_DATA_BAR) == 0)
      shiftRegister |= 1;
    outb (dataBit | CPU32_PD_CR_NOT_SINGLESTEP, self->controlPort);
    bdm_delay ((self->delayTimer >> 1) + 1);
  }
  self->readValue = shiftRegister & 0x1FFFF;
  if (self->debugFlag)
    PRINTF (" cpu32_pd_serial_clock -- send 0x%05x  receive 0x%05x\n",
            wval, self->readValue);
  if (self->readValue & 0x10000) {
    if (self->readValue == 0x10001)
      return BDM_FAULT_BERR;
    else if (self->readValue != 0x10000)
      return BDM_FAULT_NVC;
  }
  return 0;
}

/*
 * Restart chip and stop on first instruction fetch
 */
static int
cpu32_pd_restart_chip (struct BDM *self)
{
  int check;

  if (self->debugFlag)
    PRINTF (" cpu32_pd_restart_chip\n");
  outb (CPU32_PD_CR_FORCE_RESET | CPU32_PD_CR_CLOCKBAR_BKPT, self->controlPort);
  udelay (10);
  outb (CPU32_PD_CR_CLOCKBAR_BKPT, self->controlPort);
  udelay (10);
  outb (CPU32_PD_CR_NOT_SINGLESTEP | CPU32_PD_CR_CLOCKBAR_BKPT, self->controlPort);
  for (check = 0 ; check < 1000 ; check++) {
    if (inb (self->statusPort) & CPU32_PD_SR_FROZEN)
      return 0;
  }
  return BDM_FAULT_RESPONSE;
}

/*
 * Restart chip and disable background debugging mode
 */
static int
cpu32_pd_release_chip (struct BDM *self)
{
  if (self->debugFlag)
    PRINTF (" cpu32_pd_release_chip\n");
  outb (CPU32_PD_CR_NOT_SINGLESTEP | CPU32_PD_CR_FORCE_RESET, self->controlPort);
  udelay (10);
  outb (CPU32_PD_CR_NOT_SINGLESTEP, self->controlPort);
  return 0;
}

/*
 * Restart chip, enable background debugging mode, halt on first fetch
 *
 * The software from the Motorola BBS tries to have the target
 * chip begin execution, but that doesn't work very reliably.
 * The RESETH* line rises rather slowly, so sometimes the BKPT* / DSCLK
 * would be seen low, and sometimes it wouldn't.
 */
static int
cpu32_pd_reset_chip (struct BDM *self)
{
  if (self->debugFlag)
    PRINTF (" cpu32_pd_reset_chip\n");
  outb (CPU32_PD_CR_CLOCKBAR_BKPT | CPU32_PD_CR_FORCE_RESET,
        self->controlPort);
  udelay (10);
  outb (CPU32_PD_CR_CLOCKBAR_BKPT, self->controlPort);
  udelay (10);
  outb (CPU32_PD_CR_CLOCKBAR_BKPT | CPU32_PD_CR_NOT_SINGLESTEP,
        self->controlPort);
  return 0;
}

/*
 * Force the target into background debugging mode
 */
static int
cpu32_pd_stop_chip (struct BDM *self)
{
  int check;

  if (self->debugFlag)
    PRINTF (" cpu32_pd_stop_chip\n");
  if (inb (self->statusPort) & CPU32_PD_SR_FROZEN)
    return 0;
  outb (CPU32_PD_CR_NOT_SINGLESTEP | CPU32_PD_CR_CLOCKBAR_BKPT,
        self->controlPort);
  for (check = 0 ; check < 1000 ; check++) {
    if (inb (self->statusPort) & CPU32_PD_SR_FROZEN)
      return 0;
  }
  return BDM_FAULT_RESPONSE;
}

/*
 * Make the target execute a single instruction and
 * reenter background debugging mode
 */
static int
cpu32_pd_step_chip (struct BDM *self)
{
  int           check;
  unsigned char dataBit;
  int           err;

  if (self->debugFlag)
    PRINTF (" cpu32_pd_step_chip\n");
  err = cpu32_serial_clock (self, BDM_GO_CMD, 1);
  if (err)
    return err;

  /*
   * Send the last bit of the command
   */
  dataBit = (BDM_GO_CMD & 0x1) ? CPU32_PD_CR_DATA : 0;
  outb (dataBit | CPU32_PD_CR_NOT_SINGLESTEP | CPU32_PD_CR_CLOCKBAR_BKPT,
        self->controlPort);
  bdm_delay (self->delayTimer + 1);

  /*
   * Enable single-step
   */
  outb (dataBit | CPU32_PD_CR_CLOCKBAR_BKPT, self->controlPort);
  bdm_delay (1);
  outb (dataBit, self->controlPort);

  /*
   * Wait for step to complete
   * The software from the Motorola BBS doesn't do this, but
   * omitting the `outb' operation leaves a race condition the
   * next time cpu32_serial_clock is called.
   *
   * The first output operation in bdmSerialClock sends
   * `dataBit | CPU32_CR_NOT_SINGLESTEP | CPU32_CR_CLOCKBAR_BKPT' to the
   * control port.  If the flip flop in the external circuit
   * clears before the `CPU32_CR_CLOCKBAR_BKPT' pin of the '132 goes
   * low, there is a narrow glitch on the BKPT* / DSCLK pin, which
   * clocks a garbage bit into the target chip.
   */
  for (check = 0 ; check < 1000 ; check++) {
    if (inb (self->statusPort) & CPU32_PD_SR_FROZEN) {
      outb (CPU32_PD_CR_CLOCKBAR_BKPT, self->controlPort);
      return 0;
    }
  }
  return BDM_FAULT_RESPONSE;
}

/*
 * Get target status
 */
static int
cpu32_icd_get_status (struct BDM *self)
{
  unsigned char sr = inb (self->statusPort);
  int           ret;

  ret = sr & CPU32_ICD_FREEZE ? BDM_TARGETSTOPPED : 0;
  if (self->debugFlag > 1)
    PRINTF (" cpu32_icd_get_status -- Status Port:0x%02x  Status:0x%04x\n",
            sr, ret);
  return ret;
}

/*
 * Hardware initialization
 */
static int
cpu32_icd_init_hardware (struct BDM *self)
{
  int status;

  /*
   * Force breakpoint
   */
  outb (CPU32_ICD_STEP_OUT | CPU32_ICD_DSCLK | CPU32_ICD_RST_OUT, self->dataPort);
  udelay (10);
  status = cpu32_icd_get_status (self);
  if (self->debugFlag)
    PRINTF (" cpu32_icd_init_hardware: status:0x%02x, data port:0x%02x\n",
            status, inb (self->dataPort));
  return status;
}

/*
 * Clock a word to/from the target
 */
static int
cpu32_icd_serial_clock (struct BDM *self, unsigned short wval, int holdback)
{
  unsigned long shiftRegister;
  unsigned char dataBit;
  unsigned int  counter;
  unsigned int  status = cpu32_icd_get_status (self);

  if (status & BDM_TARGETRESET)
    return BDM_FAULT_RESET;
  if (status & BDM_TARGETNC)
    return BDM_FAULT_CABLE;
  if (status & BDM_TARGETPOWER)
    return BDM_FAULT_POWER;
  if(!(status & BDM_TARGETSTOPPED)) {
    if (self->debugFlag)
      PRINTF (" cpu32_icd_serial_clock -- stop target first\n");
    if (cpu32_icd_stop_chip (self) == BDM_FAULT_RESPONSE) {
      if (self->debugFlag)
        PRINTF (" cpu32_icd_serial_clock -- can\'t stop it\n");
      return BDM_FAULT_RESPONSE;
    }
  }
  shiftRegister = wval;
  counter = 17 - holdback;
  while (counter--) {
    dataBit = ((shiftRegister & 0x10000) ? CPU32_ICD_DSI : 0);
    shiftRegister <<= 1;
    outb (dataBit | CPU32_ICD_RST_OUT | CPU32_ICD_OE | CPU32_ICD_STEP_OUT,
          self->dataPort);
    bdm_delay (self->delayTimer + 1);
    if ((inb (self->statusPort) & CPU32_ICD_DSO) == 0)
      shiftRegister |= 1;
    outb (dataBit | CPU32_ICD_RST_OUT | CPU32_ICD_OE | CPU32_ICD_STEP_OUT | CPU32_ICD_DSCLK,
          self->dataPort);
    bdm_delay ((self->delayTimer >> 1) + 1);
  }
  if (holdback == 0) {
    outb (CPU32_ICD_RST_OUT | CPU32_ICD_STEP_OUT | CPU32_ICD_DSCLK,
          self->dataPort);
    bdm_delay (self->delayTimer + 1);
  }
  outb (CPU32_ICD_RST_OUT | CPU32_ICD_STEP_OUT | CPU32_ICD_DSCLK,
        self->dataPort);
  self->readValue = shiftRegister & 0x1FFFF;
  if (self->debugFlag)
    PRINTF (" cpu32_icd_serial_clock -- send 0x%05x, receive 0x%05x\n",
            wval, self->readValue);
  if (self->readValue & 0x10000) {
    if (self->readValue == 0x10001)
      return BDM_FAULT_BERR;
    else if (self->readValue != 0x10000)
      return BDM_FAULT_NVC;
  }
  return 0;
}

/*
 * Force the target into background debugging mode
 */
static int
cpu32_icd_stop_chip (struct BDM *self)
{
  int check;
  int pass;
  int first_time = 1;
  
  if (self->debugFlag)
    PRINTF (" cpu32_icd_stop_chip\n");
  if (inb (self->statusPort) & CPU32_ICD_FREEZE)
    return 0;
  for (pass = 0; pass < 2; pass++) {
    if (first_time)
      outb (CPU32_ICD_DSCLK | CPU32_ICD_RST_OUT, self->dataPort);
    else
      outb (CPU32_ICD_DSCLK | CPU32_ICD_RST_OUT | CPU32_ICD_FORCE_BERR,
            self->dataPort);
    first_time = 0;
    for (check = 0 ; check < 1000 ; check++) {
      if (inb (self->statusPort) & CPU32_ICD_FREEZE) {
        outb (CPU32_ICD_DSCLK | CPU32_ICD_RST_OUT, self->dataPort);
        return 0;
      }
      bdm_delay (1);
    }
  }
  outb (CPU32_ICD_DSCLK | CPU32_ICD_RST_OUT, self->dataPort);
  return BDM_FAULT_RESPONSE;
}

/*
 * Restart chip and stop on first instruction fetch
 */
static int
cpu32_icd_restart_chip (struct BDM *self)
{
  if (self->debugFlag)
    PRINTF (" cpu32_icd_restart_chip\n");
  outb (CPU32_ICD_DSCLK, self->dataPort);
  udelay (1);
  return cpu32_icd_stop_chip (self);
}

/*
 * Restart chip and disable background debugging mode
 */
static int
cpu32_icd_release_chip (struct BDM *self)
{
  if (self->debugFlag)
    PRINTF (" cpu32_icd_release_chip\n");
  outb (CPU32_ICD_DSCLK | CPU32_ICD_STEP_OUT, self->dataPort);
  udelay (10);
  outb (CPU32_ICD_DSCLK | CPU32_ICD_RST_OUT | CPU32_ICD_STEP_OUT, self->dataPort);
  udelay (10);
  return 0;
}

/*
 * Restart chip, enable background debugging mode, halt on first fetch
 *
 * The software from the Motorola BBS tries to have the target
 * chip begin execution, but that doesn't work very reliably.
 * The RESETH* line rises rather slowly, so sometimes the BKPT* / DSCLK
 * would be seen low, and sometimes it wouldn't.
 */
static int
cpu32_icd_reset_chip (struct BDM *self)
{
  if (self->debugFlag)
    PRINTF (" cpu32_icd_reset_chip\n");

  /*
   * Assert RESET*, BKPT*, and BREAK*
   */
  outb (0, self->dataPort);
  udelay (100);
  
  /*
   * Deassert RESET (CPU must see BKPT* asserted at rising edge of RESET*)
   * Leaving BKPT* and BREAK* asserted gets us ready for first data txfer
   * as per Figure 7-8 in CPU32RM/AD
   */
  outb (CPU32_ICD_RST_OUT, self->dataPort);
  udelay (100);

  return 0;
}


/*
 * Make the target execute a single instruction and
 * reenter background debugging mode
 */
static int
cpu32_icd_step_chip (struct BDM *self)
{
  unsigned char dataBit;
  int           err;

  if (self->debugFlag)
    PRINTF (" cpu32_step_chip\n");
  err = cpu32_serial_clock (self, BDM_GO_CMD, 1);
  if (err)
    return err;

  /*
   * Send the last bit of the command
   */
  dataBit = (BDM_GO_CMD & 0x1) ? CPU32_ICD_DSI : 0;
  outb (dataBit | CPU32_ICD_OE | CPU32_ICD_STEP_OUT | CPU32_ICD_RST_OUT,
        self->dataPort);
  bdm_delay (self->delayTimer + 1);
  outb (dataBit | CPU32_ICD_OE | CPU32_ICD_RST_OUT, self->dataPort);
  bdm_delay (1);
  /* Raise CPU32_ICD_DSCLK before dropping CPU32_ICD_OEA */
  outb (CPU32_ICD_DSCLK | CPU32_ICD_OE | CPU32_ICD_RST_OUT, self->dataPort);
  bdm_delay (1);
  outb (CPU32_ICD_DSCLK | CPU32_ICD_RST_OUT, self->dataPort);

  return cpu32_icd_stop_chip (self);
}

/*
 * Read system register
 */

static int 
cpu32_read_sysreg (struct BDM *self, struct BDMioctl *ioc)
{
  int err, cmd;
  unsigned short msw, lsw;

  /*
   * CPU32 MBAR require sfc support, make it look like
   * a register.
   */
  if (ioc->address == BDM_REG_MBAR) {
    struct BDMioctl mbar_ioc;
    unsigned long sfc;

    mbar_ioc.address = BDM_REG_SFC;
    if ((err = cpu32_read_sysreg (self, &mbar_ioc)) < 0)
      return err;
    sfc = mbar_ioc.value;
    mbar_ioc.address = BDM_REG_SFC;
    mbar_ioc.value = 7;
    if ((err = cpu32_write_sysreg (self, &mbar_ioc)) < 0)
      return err;
    mbar_ioc.address = 0x3FF00;
    if ((err = bdmDrvReadLongWord (self, &mbar_ioc)) < 0)
      return err;
    ioc->value = mbar_ioc.value;
    mbar_ioc.address = BDM_REG_SFC;
    mbar_ioc.value = sfc;
    if ((err = cpu32_write_sysreg (self, &mbar_ioc)) < 0)
      return err;
    return 0;
  }
  
  if (ioc->address > BDM_REG_VBR)
    return BDM_FAULT_NVC;
  
  cmd = BDM_RSREG_CMD | cpu32_sysreg_map[ioc->address];
  if (((err = cpu32_serial_clock (self, cmd, 0)) != 0) ||
      ((err = bdmDrvFetchWord (self, &msw)) != 0) ||
      ((err = bdmDrvFetchWord (self, &lsw)) != 0))
    return err;
  ioc->value = (msw << 16) | lsw;
  return 0;
}

/*
 * Write system register
 */
static int
cpu32_write_sysreg (struct BDM *self, struct BDMioctl *ioc)
{
  int err, cmd;

  /*
   * CPU32 MBAR require dfc support, make it look like
   * a register.
   */
  if (ioc->address == BDM_REG_MBAR) {
    struct BDMioctl mbar_ioc;
    unsigned long dfc;

    mbar_ioc.address = BDM_REG_DFC;
    if ((err = cpu32_read_sysreg (self, &mbar_ioc)) < 0)
      return err;
    dfc = mbar_ioc.value;
    mbar_ioc.address = BDM_REG_DFC;
    mbar_ioc.value = 7;
    if ((err = cpu32_write_sysreg (self, &mbar_ioc)) < 0)
      return err;
    mbar_ioc.address = 0x3FF00;
    mbar_ioc.value = ioc->value;
    if ((err = bdmDrvWriteLongWord (self, &mbar_ioc)) < 0)
      return err;
    mbar_ioc.address = BDM_REG_DFC;
    mbar_ioc.value = dfc;
    if ((err = cpu32_write_sysreg (self, &mbar_ioc)) < 0)
      return err;
    return 0;
  }
  
  if (ioc->address > BDM_REG_VBR)
    return BDM_FAULT_NVC;
  cmd = BDM_WSREG_CMD | cpu32_sysreg_map[ioc->address];
  if (((err = cpu32_serial_clock (self, cmd, 0)) != 0) ||
      ((err = cpu32_serial_clock (self, ioc->value >> 16, 0)) != 0) ||
      ((err = cpu32_serial_clock (self, ioc->value, 0)) != 0))
    return err;
  return 0;
}

/*
 * Generate a bus error as the access has failed. This is
 * not supported on the CPU32.
 */

static int 
cpu32_gen_bus_error (struct BDM *self)
{
  return 0;
}

/*
 * Restart target execution
 */
static int
cpu32_run_chip (struct BDM *self)
{
  return cpu32_serial_clock (self, BDM_GO_CMD, 0);
}

#ifdef BDM_BIT_BASH_PORT

/*
 * Bit Bash the BDM port. No status checks. I assume you know what is happening at
 * a low level with the BDM hardware if you are using this interface.
 */
static int
cpu32_bit_bash (struct BDM *self, unsigned short mask, unsigned short bits)
{
  unsigned char ctrl_port = 0;
  
  if (self->debugFlag)
    PRINTF (" cpu32_bit_bash: mask=%04x, bits=%04x\n", mask, bits);

  self->bit_bash_bits &= ~mask;
  self->bit_bash_bits |= bits;
  
  if (self->bit_bash_bits & BDM_BB_RESET)
    ctrl_port |= CPU32_CR_FORCE_RESET;

  if ((self->bit_bash_bits & BDM_BB_BKPT) == 0)
    ctrl_port |= CPU32_CR_CLOCKBAR_BKPT;

  return 0;
}

#endif

/*
 * Initialise the BDM structure for a CPU32
 */
static int
cpu32_pd_init_self (struct BDM *self)
{
  int reg;
  
  self->processor = BDM_CPU32;
  self->interface = BDM_CPU32_ERIC;

  self->get_status    = cpu32_pd_get_status;
  self->init_hardware = cpu32_pd_init_hardware;
  self->serial_clock  = cpu32_pd_serial_clock;
  self->gen_bus_error = cpu32_gen_bus_error;
  self->read_sysreg   = cpu32_read_sysreg;
  self->write_sysreg  = cpu32_write_sysreg;
  self->restart_chip  = cpu32_pd_restart_chip;
  self->release_chip  = cpu32_pd_release_chip;
  self->reset_chip    = cpu32_pd_reset_chip;
  self->stop_chip     = cpu32_pd_stop_chip;
  self->run_chip      = cpu32_run_chip;
  self->step_chip     = cpu32_pd_step_chip;
  
#ifdef BDM_BIT_BASH_PORT
  self->bit_bash      = cpu32_bit_bash;
  self->bit_bash_bits = 0;
#endif
  
  for (reg = 0; reg < BDM_MAX_SYSREG; reg++)
    self->shadow_sysreg[reg] = 0;

  return 0;
}
  
static int
cpu32_icd_init_self (struct BDM *self)
{
  int reg;
  
  self->processor = BDM_CPU32;
  self->interface = BDM_CPU32_ICD;

  self->get_status    = cpu32_icd_get_status;
  self->init_hardware = cpu32_icd_init_hardware;
  self->serial_clock  = cpu32_icd_serial_clock;
  self->gen_bus_error = cpu32_gen_bus_error;
  self->read_sysreg   = cpu32_read_sysreg;
  self->write_sysreg  = cpu32_write_sysreg;
  self->restart_chip  = cpu32_icd_restart_chip;
  self->release_chip  = cpu32_icd_release_chip;
  self->reset_chip    = cpu32_icd_reset_chip;
  self->stop_chip     = cpu32_icd_stop_chip;
  self->run_chip      = cpu32_run_chip;
  self->step_chip     = cpu32_icd_step_chip;
  
#ifdef BDM_BIT_BASH_PORT
  self->bit_bash      = cpu32_bit_bash;
  self->bit_bash_bits = 0;
#endif
  
  for (reg = 0; reg < BDM_MAX_SYSREG; reg++)
    self->shadow_sysreg[reg] = 0;

  return 0;
}

/*
 ************************************************************************
 *     Coldfire P&E support routines                                    *
 ************************************************************************
 */

/*
 * Coldfire system register mapping. See bdm.h for the user values.
 *
 * For a RCREG 0x1 is invalid.
 */

static int cf_sysreg_map[BDM_REG_DBMR + 1] =
{ 0x80f,    /* BDM_REG_RPC      */
  0x1,      /* BDM_REG_PCC      */
  0x80e,    /* BDM_REG_SR       */
  0x1,      /* BDM_REG_USP      */
  0x1,      /* BDM_REG_SSP, use A7    */
  0x1,      /* BDM_REG_SFC      */
  0x1,      /* BDM_REG_DFC      */
  0x1,      /* BDM_REG_ATEMP    */
  0x1,      /* BDM_REG_FAR      */
  0x801,    /* BDM_REG_VBR      */
  0x2,      /* BDM_REG_CACR     */
  0x4,      /* BDM_REG_ACR0     */
  0x5,      /* BDM_REG_ACR1     */
  0xc04,    /* BDM_REG_RAMBAR   */
  0xc0f,    /* BDM_REG_MBAR     */
  0x0,      /* BDM_REG_CSR      */
  0x6,      /* BDM_REG_AATR     */
  0x7,      /* BDM_REG_TDR      */
  0x8,      /* BDM_REG_PBR      */
  0x9,      /* BDM_REG_PBMR     */
  0xc,      /* BDM_REG_ABHR     */
  0xd,      /* BDM_REG_ABLR     */
  0xe,      /* BDM_REG_DBR      */
  0xf       /* BDM_REG_DBMR     */
};

static int cf_pe_read_sysreg (struct BDM *self, struct BDMioctl *ioc);
static int cf_pe_write_sysreg (struct BDM *self, struct BDMioctl *ioc);

/*
 * Get direct target status, that is from the parallel port.
 */
static int
cf_pe_get_direct_status (struct BDM *self)
{
  unsigned char sr = inb (self->statusPort);
  int           ret;
  
  if (self->cf_use_pst) {
    unsigned char cr = 0, pst = 0;

    ret = 0;
    if (!(sr & CF_PE_SR_POWERED))
      ret = BDM_TARGETPOWER;
    else if ((inb (self->dataPort) & CF_PE_DR_RESET) == 0)
      ret = BDM_TARGETRESET;
    else if (sr & CF_PE_SR_FROZEN) {
      cr = inb(self->controlPort);

      pst = 0;
      if (sr & CF_PE_SR_PST1)
        pst |= 0x02;
      if ((cr & CF_PE_CR_NOT_PST0) == 0)
        pst |= 0x01;
      if ((cr & CF_PE_CR_NOT_PST2) == 0)
        pst |= 0x04;
      if ((cr & CF_PE_CR_NOT_PST3) == 0)
        pst |= 0x08;
  
      switch (pst) {
        case 0x00:                 /* continue execution */
        case 0x01:                 /* begin execution of an instruction */
        case 0x02:                 /* reserved */
        case 0x03:                 /* entry into user mode */
        case 0x04:                 /* begin execution of PULSE and WDDATA inst.  */
        case 0x05:                 /* begin execution of taken branch */
        case 0x06:                 /* reserved */
        case 0x07:                 /* begin execution of RTE instruction */
        case 0x08:                 /* begin 1-byte transfer on DDATA */
        case 0x09:                 /* begin 2-byte transfer on DDATA */
        case 0x0A:                 /* begin 3-byte transfer on DDATA */
        case 0x0B:                 /* begin 4-byte transfer on DDATA */
        case 0x0C:                 /* Execption processing */
        case 0x0D:                 /* emulator mode exception processing */
          ret = 0;
          break;
        case 0x0E:             /* processor stopped waiting for interrupt */
          ret = BDM_TARGETSTOPPED;
          break;
        case 0x0F:                 /* processor halted */
          ret = BDM_TARGETHALT;
          break;
        default:
          PRINTF ("PST is invalid (0x%x,sr=%x,cr=%x)\n", pst, sr, cr);
          break;
      }
    }
    if (self->debugFlag > 3)
      PRINTF (" cf_pe_get_direct_status -- Status:0x%x SR:%x CR:%x PST:%x\n",
              ret, sr, cr, pst);
  }
  else {
    if ((sr & CF_PE_SR_POWERED) == 0)
      ret = BDM_TARGETPOWER;
    else
      ret = (inb (self->dataPort) & CF_PE_DR_RESET ? 0 : BDM_TARGETRESET);

    if (self->debugFlag > 3)
      PRINTF (" cf_pe_get_direct_status -- Status:0x%x, sr:0x%x\n", ret, sr);
  }
  
  return ret;
}

/*
 * Invalidate the cache.
 */

static int
cf_pe_invalidate_cache (struct BDM *self)
{
  struct BDMioctl cacr_ioc;

  cacr_ioc.address = BDM_REG_CACR;

  if (cf_pe_read_sysreg (self, &cacr_ioc) < 0)
      return BDM_TARGETNC;

  /*
   * Set the invalidate bit.
   */

  if (cacr_ioc.value) {
    cacr_ioc.value |= 0x01000000;

    if (self->debugFlag > 2)
      PRINTF (" cf_pe_invalidate_cache -- cacr:0x%08x\n", (int) cacr_ioc.value);
  
    if (cf_pe_write_sysreg (self, &cacr_ioc) < 0)
      return BDM_TARGETNC;
  }
  
  return 0;
}

/*
 * PC read check. This is used to see if the processor has halted.
 */

static int
cf_pe_pc_read_check (struct BDM *self)
{
  struct BDMioctl pc_ioc;

  pc_ioc.address = BDM_REG_RPC;

  if (cf_pe_read_sysreg (self, &pc_ioc) < 0)
      return BDM_TARGETNC;

  return 0;
}

/*
 * Get target status
 */

static int
cf_pe_get_status (struct BDM *self)
{
  int           cf_last_running = self->cf_running;
  unsigned int  status = cf_pe_get_direct_status (self);
  unsigned long csr;
  int           ret;

  if (self->cf_use_pst) {
    return status;
  }
  
  if (status & (BDM_TARGETRESET | BDM_TARGETNC | BDM_TARGETPOWER)) {
    PRINTF (" cf_pe_get_status -- Status:0x%x\n", status);
    return status;
  }
  
  if (self->cf_running) {
    struct BDMioctl csr_ioc;

    csr_ioc.address = BDM_REG_CSR;

    if (cf_pe_read_sysreg (self, &csr_ioc) < 0)
      return BDM_TARGETNC;

    csr = csr_ioc.value;
  } else
    csr = self->cf_csr;

  ret = ((csr & 0x0e000000 ? BDM_TARGETHALT : 0) |
         (csr & 0x0f000000 ? BDM_TARGETSTOPPED : 0));

  /*
   * If we were running, and we have detected we have stopped
   * flush the cache.
   */
  if (cf_last_running && !self->cf_running)
    cf_pe_invalidate_cache (self);
  
  if (self->debugFlag > 2)
    PRINTF (" cf_pe_get_status -- Status:0x%x, csr:0x%08x, cf_csr:0x%08x\n",
            ret, (int) csr, (int) self->cf_csr);
  
  return ret;
}

/*
 * Hardware initialization.
 */
static int
cf_pe_init_hardware (struct BDM *self)
{
  int status;

  /*
   * Set the control port to this value. It makes the pins
   * high allowing them to be pulled down and therefore
   * used as inputs.
   * 
   * This value breaks on a DELL 500MHz Pentium III machine. The
   * only known work around is to add a second parallel port to the
   * machine.
   */

  outb (0x00, self->controlPort);
  
  /*
   * Force breakpoint
   */

  outb (CF_PE_MAKE_DR (0), self->dataPort);
  bdm_sleep (HZ / 4);
  outb (CF_PE_MAKE_DR (CF_PE_DR_RESET), self->dataPort);
  bdm_sleep (HZ / 2);
  outb (CF_PE_MAKE_DR (CF_PE_DR_RESET | CF_PE_DR_BKPT), self->dataPort);
  udelay (1000);
  outb (CF_PE_MAKE_DR (CF_PE_DR_BKPT), self->dataPort);
  bdm_sleep (HZ / 2);
  outb (CF_PE_MAKE_DR (0), self->dataPort);

  /*
   * We do not know which version of debug module we have so
   * default to 0 and do not use PST signals. This will cause the
   * CSR register to be read.
   */
   
  self->cf_running   = 1;
  self->cf_debug_ver = 0;
  self->cf_use_pst   = 0;
  
  status = cf_pe_get_status (self);
  
  if (self->debugFlag)
    PRINTF (" cf_pe_init_hardware: status:%d\n", status);

  /*
   * Process the result of the CSR read.
   */

  self->cf_debug_ver = (self->cf_csr >> 20) & 0x0f;

  if (self->cf_debug_ver)
    self->cf_use_pst = 1;

  if (self->debugFlag)
    PRINTF (" cf_ps_init_hardware: debug version is %d, PST %s\n",
            self->cf_debug_ver, self->cf_use_pst ? "enabled" : "disabled");
  
  return status;
}

/*
 * Clock a word to/from the target
 */
static int
cf_pe_serial_clock (struct BDM *self, unsigned short wval, int holdback)
{
  unsigned long shiftRegister;
  unsigned char dataBit;
  unsigned int  counter;
  unsigned int  status = cf_pe_get_direct_status (self);

  if (status & BDM_TARGETRESET)
    return BDM_FAULT_RESET;
  if (status & BDM_TARGETNC)
    return BDM_FAULT_CABLE;
  if (status & BDM_TARGETPOWER)
    return BDM_FAULT_POWER;
  
  shiftRegister = wval;
  counter       = 17 - holdback;
  
  while (counter--) {
    dataBit = ((shiftRegister & 0x10000) ? CF_PE_DR_DATA_IN : 0);

    shiftRegister <<= 1;
    
    outb (CF_PE_MAKE_DR (dataBit), self->dataPort);

    if (self->delayTimer)
      bdm_delay (self->delayTimer);

    outb (CF_PE_MAKE_DR (dataBit | CF_PE_DR_CLOCK_HIGH), self->dataPort);
    
    if (self->delayTimer)
      bdm_delay (self->delayTimer);

    outb (CF_PE_MAKE_DR (dataBit), self->dataPort);
    
    if (self->delayTimer)
      bdm_delay (self->delayTimer);

    if ((inb (self->statusPort) & CF_PE_SR_DATA_OUT) == 0)
      shiftRegister |= 1;    
  }

  outb (CF_PE_MAKE_DR (0), self->dataPort);
  
  self->readValue = shiftRegister & 0x1FFFF;
  
  if (self->debugFlag > 2)
    PRINTF (" cf_pe_serial_clock -- send 0x%x  receive 0x%x\n",
            wval, self->readValue);
  
  if (self->readValue & 0x10000) {
    if (self->readValue == 0x10001) {
      if (self->debugFlag > 1)
        PRINTF (" cf_pe_serial_clock -- failure BUS ERROR, send 0x%x receive 0x%x\n",
                wval, self->readValue);
      return BDM_FAULT_BERR;
    }
    else if (self->readValue != 0x10000) {
      if (self->debugFlag > 1)
        PRINTF (" cf_pe_serial_clock -- failure NVC, send 0x%x receive 0x%x\n",
                wval, self->readValue);
      return BDM_FAULT_NVC;
    }
  }
  return 0;
}

/*
 * Read system register
 */

static int 
cf_pe_read_sysreg (struct BDM *self, struct BDMioctl *ioc)
{
  int            err;
  unsigned short msw, lsw;

  if (ioc->address >= BDM_MAX_SYSREG)
    return BDM_FAULT_NVC;

  /*
   * For the Coldfire we need to select the type of command
   * to use.
   */
  if (ioc->address < BDM_REG_CSR) {
    if (((err = cf_pe_serial_clock (self, BDM_RCREG_CMD, 0)) != 0) ||
        ((err = cf_pe_serial_clock (self, 0, 0)) != 0) ||
        ((err = cf_pe_serial_clock (self, cf_sysreg_map[ioc->address], 0)) != 0) ||
        ((err = bdmDrvFetchWord (self, &msw)) != 0) ||
        ((err = bdmDrvFetchWord (self, &lsw)) != 0))
      return err;
  }
  else {
    int cmd = BDM_RDMREG_CMD | cf_sysreg_map[ioc->address];
    
    /*
     * Are the registers write only ?
     */
    if (ioc->address != BDM_REG_CSR) {
      ioc->value = self->shadow_sysreg[ioc->address];
      if (self->debugFlag > 1)
        PRINTF (" cf_pe_read_sysreg - Reg:0x%x is write only, 0x%08x\n",
              ioc->address, ioc->value);
      return 0;
    }
  
    if (((err = cf_pe_serial_clock (self, cmd, 0)) != 0) ||
        ((err = bdmDrvFetchWord (self, &msw)) != 0) ||
        ((err = bdmDrvFetchWord (self, &lsw)) != 0)) {
      PRINTF (" cf_pe_read_sysreg - Reg:0x%x failed with cmd 0x%02x, err = %d\n",
              ioc->address, cmd, err);
      return err;
    }
  }
  
  ioc->value = (msw << 16) | lsw;

  /*
   * Watch out here as this function is called from cf_pe_get_status.
   */
  
  if (ioc->address == BDM_REG_CSR) {
    if (self->cf_running) {
      if (ioc->value & 0x0f000000) {
        
       /*
         * We could false trigger for some reason such as bit
         * errors on the serial stream. To check if we have
         * actually halted we should read the program counter
         * register. If we can then we have halted, else
         * we can only assume we are still running.
         *
         * We check the PC read twice to insure we are ok.
         *
         * This solution is provided by Motorola, more specifically
         * Joe Circello, the chief ColdFire architect, and Sue Cozart
         * for providing access to Joe. Thanks. (CCJ 15-05-2000)
         */

        if ((cf_pe_pc_read_check (self) == 0) &&
            (cf_pe_pc_read_check (self) == 0)) {
          self->cf_csr     = ioc->value;
          self->cf_running = 0;
        
          /*
           * If we were running, and we have detected we have stopped
           * flush the cache.  This should probably be somewhere else
           * if we are using PST,  I am not sure yet : davidm
           */
          
          cf_pe_invalidate_cache (self);
        }
        else {
          /*
           * We have not halted as the PC is not accessable.
           */
          ioc->value &= ~0x0f000000;
        }
      }
    } else {
      self->cf_csr = ioc->value = (self->cf_csr & 0x0f000000) | ioc->value;
    }
  }

  if (ioc->address == BDM_REG_SR)
    ioc->value &= 0xffff;
  
  if (self->debugFlag > 1)
    PRINTF (" cf_pe_read_sysreg - Reg:0x%x is 0x%08x\n",
            ioc->address, ioc->value);
  
  return 0;
}

/*
 * Write system register
 */
static int
cf_pe_write_sysreg (struct BDM *self, struct BDMioctl *ioc)
{
  int err;

  if (ioc->address >= BDM_MAX_SYSREG)
    return BDM_FAULT_NVC;
  
  if (self->debugFlag)
    PRINTF (" cf_pe_write_sysreg - Reg:0x%x is 0x%x\n", ioc->address, ioc->value);
  
  /*
   * For the Coldfire we need to select the type of command
   * to use.
   */
  if (ioc->address < BDM_REG_CSR) {
    if (((err = cf_pe_serial_clock (self, BDM_WCREG_CMD, 0)) != 0) ||
        ((err = cf_pe_serial_clock (self, 0, 0)) != 0) ||
        ((err = cf_pe_serial_clock (self, cf_sysreg_map[ioc->address], 0)) != 0) ||
        ((err = cf_pe_serial_clock (self, ioc->value >> 16, 0)) != 0) ||
        ((err = cf_pe_serial_clock (self, ioc->value, 0)) != 0))
      return err;
  }
  else {
    int cmd = BDM_WDMREG_CMD | cf_sysreg_map[ioc->address];

    self->shadow_sysreg[ioc->address] = ioc->value;
    
    if (((err = cf_pe_serial_clock (self, cmd , 0)) != 0) ||
        ((err = cf_pe_serial_clock (self, ioc->value >> 16, 0)) != 0) ||
        ((err = cf_pe_serial_clock (self, ioc->value, 0)) != 0))
      return err;
  }
  
  return 0;
}

/*
 * Generate a bus error as the access has failed. This is
 * not supported on the CPU32.
 */

static int 
cf_pe_gen_bus_error (struct BDM *self)
{
  if (self->debugFlag)
    PRINTF (" cf_pe_gen_bus_error\n");
  
  outb (CF_PE_MAKE_DR (CF_PE_DR_TEA), self->dataPort);
  udelay (400);
  outb (CF_PE_MAKE_DR (0), self->dataPort);
  
  return BDM_FAULT_BERR;
}

/*
 * Restart chip and stop on first instruction fetch. Easy for the
 * Coldfire, keep BKPT asserted during the first 16 clocks after
 * reset is negated.
 */
static int
cf_pe_restart_chip (struct BDM *self)
{
  if (self->debugFlag)
    PRINTF (" cf_pe_restart_chip\n");
  
  outb (CF_PE_MAKE_DR (0), self->dataPort);
  udelay (2000);
  outb (CF_PE_MAKE_DR (CF_PE_DR_RESET), self->dataPort);
  bdm_sleep (HZ / 2);
  outb (CF_PE_MAKE_DR (CF_PE_DR_RESET | CF_PE_DR_BKPT), self->dataPort);
  udelay (1000);
  outb (CF_PE_MAKE_DR (CF_PE_DR_BKPT), self->dataPort);
  bdm_sleep (HZ / 2);
  outb (CF_PE_MAKE_DR (0), self->dataPort);
  
  self->cf_running = 1;
  
  if (cf_pe_get_status (self) & (BDM_TARGETHALT | BDM_TARGETSTOPPED))
    return 0;

  return BDM_FAULT_RESPONSE;
}

/*
 * Restart chip and disable background debugging mode. On
 * a Coldfire exit reset without the BKPT being asserted.
 */
static int
cf_pe_release_chip (struct BDM *self)
{
  if (self->debugFlag)
    PRINTF (" cf_pe_release_chip\n");
  outb (CF_PE_MAKE_DR (0), self->dataPort);
  udelay (2000);
  outb (CF_PE_MAKE_DR (CF_PE_DR_RESET), self->dataPort);
  bdm_sleep (HZ / 2);
  outb (CF_PE_MAKE_DR (0), self->dataPort);
  
  self->cf_running = 1;
  
  return 0;
}

/*
 * Restart chip, the coldfire is easier than the CPU32 to get into
 * BMD mode after reset. Just have the BKPT signal active when
 * reset is negated.
 */
static int
cf_pe_reset_chip (struct BDM *self)
{
  if (self->debugFlag)
    PRINTF (" cf_pe_reset_chip\n");

  outb (CF_PE_MAKE_DR (0), self->dataPort);
  udelay (2000);
  outb (CF_PE_MAKE_DR (CF_PE_DR_RESET), self->dataPort);
  bdm_sleep (HZ / 2);
  outb (CF_PE_MAKE_DR (CF_PE_DR_RESET | CF_PE_DR_BKPT), self->dataPort);
  udelay (2000);
  outb (CF_PE_MAKE_DR (CF_PE_DR_BKPT), self->dataPort);
  bdm_sleep (HZ / 2);
  outb (CF_PE_MAKE_DR (0), self->dataPort);

  self->cf_running = 1;
  
  return 0;
}

/*
 * Force the target into background debugging mode
 */
static int
cf_pe_stop_chip (struct BDM *self)
{
  int status, retries = 4;
  
  if (self->debugFlag)
    PRINTF (" cf_pe_stop_chip\n");

  while (retries--) {
    outb (CF_PE_MAKE_DR (CF_PE_DR_BKPT), self->dataPort);

    bdm_sleep (HZ / 50);
    status = cf_pe_get_status (self);
    
    outb (CF_PE_MAKE_DR (0), self->dataPort);

    if (cf_pe_get_status (self) & (BDM_TARGETHALT | BDM_TARGETSTOPPED))
      return 0;
  }
  
  return BDM_FAULT_RESPONSE;
}

/*
 * Restart target execution.
 */
static int
cf_pe_run_chip (struct BDM *self)
{
  struct BDMioctl csr_ioc;
  int err;

  if (self->debugFlag)
    PRINTF (" cf_pe_run_chip\n");

  /*
   * Flush the cache to insure all changed data is read by the
   * processor.
   */
  cf_pe_invalidate_cache (self);
  
  /*
   * Change the CSR:4 or the SSM bit to off then issue a go.
   */
  csr_ioc.address = BDM_REG_CSR;
  csr_ioc.value   = 0x00000000;

  if ((err = cf_pe_write_sysreg (self, &csr_ioc)) < 0)
    return err;

  /*
   * Now issue a GO command.
   */
  err = cf_pe_serial_clock (self, BDM_GO_CMD, 0);
  if (err) {
    PRINTF (" cf_pe_run_chip - GO cmd failed, err = %d\n", err);
    return err;
  }
  
  self->cf_running = 1;

  return 0;
}

/*
 * Make the target execute a single instruction and
 * reenter background debugging mode
 */
static int
cf_pe_step_chip (struct BDM *self)
{
  struct BDMioctl csr_ioc;
  int err;

  if (self->debugFlag)
    PRINTF (" cf_pe_step_chip\n");

  /*
   * Flush the cache to insure all changed data is read by the
   * processor.
   */
  cf_pe_invalidate_cache (self);
  
  /*
   * Change the CSR:4 or the SSM bit to on then issue a go.
   * Mask pending interrupt to stop stepping into an interrupt.
   */
  csr_ioc.address = BDM_REG_CSR;
  csr_ioc.value   = 0x00000030;

  if ((err = cf_pe_write_sysreg (self, &csr_ioc)) < 0)
    return err;

  /*
   * Now issue a GO command.
   */
  err = cf_pe_serial_clock (self, BDM_GO_CMD, 0);
  if (err) {
    PRINTF (" cf_pe_step_chip - GO cmd failed, err = %d\n", err);
    return err;
  }
  
  self->cf_running = 0;
  self->cf_csr     = 0x01000000;
    
  return 0;
}

#ifdef BDM_BIT_BASH_PORT

/*
 * Bit Bash the BDM port. No status checks. I assume you know what is happening at
 * a low level with the BDM hardware if you are using this interface.
 */
static int
cf_bit_bash (struct BDM *self, unsigned short mask, unsigned short bits)
{
  unsigned char data_port = 0;
  
  self->bit_bash_bits &= ~mask;
  self->bit_bash_bits |= bits;
  
  if (self->bit_bash_bits & BDM_BB_RESET)
    data_port |= CF_PE_DR_RESET;

  if (self->bit_bash_bits & BDM_BB_BKPT)
    data_port |= CF_PE_DR_BKPT;

  if (self->bit_bash_bits & BDM_BB_DATA_IN)
    data_port |= CF_PE_DR_DATA_IN;

  if (self->bit_bash_bits & BDM_BB_CLOCK)
    data_port |= CF_PE_DR_CLOCK_HIGH;

  outb (CF_PE_MAKE_DR (data_port), self->dataPort);

  if (self->debugFlag)
    PRINTF (" cf_bit_bash: mask=%04x, bits=%04x, data reg=%02x\n",
            mask, bits, CF_PE_MAKE_DR (data_port));

  return 0;
}

#endif

/*
 * Initialise the BDM structure for a Coldfire in the P&E interface
 */
static int
cf_pe_init_self (struct BDM *self)
{
  int reg;
  
  self->processor = BDM_COLDFIRE;
  self->interface = BDM_COLDFIRE_PE;

  self->get_status    = cf_pe_get_status;
  self->init_hardware = cf_pe_init_hardware;
  self->serial_clock  = cf_pe_serial_clock;
  self->gen_bus_error = cf_pe_gen_bus_error;
  self->read_sysreg   = cf_pe_read_sysreg;
  self->write_sysreg  = cf_pe_write_sysreg;
  self->restart_chip  = cf_pe_restart_chip;
  self->release_chip  = cf_pe_release_chip;
  self->reset_chip    = cf_pe_reset_chip;
  self->stop_chip     = cf_pe_stop_chip;
  self->run_chip      = cf_pe_run_chip;
  self->step_chip     = cf_pe_step_chip;

  for (reg = 0; reg < BDM_MAX_SYSREG; reg++)
    if (reg == BDM_REG_AATR)
      self->shadow_sysreg[reg] = 0x0005;
    else
      self->shadow_sysreg[reg] = 0;

#ifdef BDM_BIT_BASH_PORT
  
  self->bit_bash      = cf_bit_bash;
  self->bit_bash_bits = 0;

#endif

  self->cf_debug_ver = 0;
  self->cf_use_pst   = 0;
  self->cf_running   = 1;
  self->cf_csr       = 0;
  
  return 0;
}

/*
 ************************************************************************
 *     Common Functions                                                 *
 ************************************************************************
 */

/*
 * Get target status
 */
static int
bdmDrvGetStatus (struct BDM *self)
{
  return (self->get_status) (self);
}

/*
 * Hardware initialization
 */
static int
bdmDrvInitHardware (struct BDM *self)
{
  return (self->init_hardware) (self);
}

/*
 * Clock a word to/from the target
 */
static int
bdmDrvSerialClock (struct BDM *self, unsigned short wval, int holdback)
{
  return (self->serial_clock) (self, wval, holdback);
}

/*
 * Generate a bus error on the target.
 */
static int
bdmDrvGenerateBusError (struct BDM *self)
{
  return (self->gen_bus_error) (self);
}
  
/*
 * Restart chip and stop on the first instruction fetch
 */
static int
bdmDrvRestartChip (struct BDM *self)
{
  return (self->restart_chip) (self);
}
  
/*
 * Restart chip and disable background debugging mode
 */
static int
bdmDrvReleaseChip (struct BDM *self)
{
  return (self->release_chip) (self);
}

/*
 * Reset chip, enable background debugging mode, halt on first fetch.
 *
 */
static int
bdmDrvResetChip (struct BDM *self)
{
  return (self->reset_chip) (self);
}

/*
 * Force the target into background debugging mode
 */
static int
bdmDrvStopChip (struct BDM *self)
{
  return (self->stop_chip) (self);
}

/*
 * Restart target execution
 */
static int
bdmDrvGo (struct BDM *self)
{
  return (self->run_chip) (self);
}

/*
 * Make the target execute a single instruction and
 * reenter background debugging mode
 */
static int
bdmDrvStepChip (struct BDM *self)
{
  return (self->step_chip) (self);
}
  
/*
 * Loop till target I/O operation completes
 */
static int
bdmDrvSendCommandTillTargetReady (struct BDM *self, unsigned short command)
{
  int err;
  int timeout = 0;

  /*
   * If no response is returned hit the bus error line and
   * then wait for the bus error response.
   */
  for (;;) {
    err = bdmDrvSerialClock (self, command, 0);
    if (err)
      return err;
    if ((self->readValue & 0x10000) == 0)
      return 0;
    ++timeout;
    if (timeout == 5) {
      /*
       * Should get a bus error response back.
       */
      bdmDrvGenerateBusError (self);
    }
    if (timeout > 6) {
      return BDM_FAULT_RESPONSE;
    }
  }
}

/*
 * Fill I/O buffer with data from target
 */
static int
bdmDrvFillBuf (struct BDM *self, int count)
{
  unsigned short *sp = (unsigned short *)self->ioBuffer;
  int cmd;
  int err;

  if (self->debugFlag)
    PRINTF ("bdmDrvFillBuf - count:%d\n", count);

  if (count == 0)
    return 0;
  
  if (count >= 4)
    cmd = BDM_DUMP_CMD | BDM_SIZE_LONG;
  else if (count >= 2)
    cmd = BDM_DUMP_CMD | BDM_SIZE_WORD;
  else
    cmd = BDM_DUMP_CMD | BDM_SIZE_BYTE;
  
  err = bdmDrvSerialClock (self, cmd, 0);
  if (err)
    return err;

  for (;;) {
    switch (count) {
      case 1:
        err = bdmDrvSendCommandTillTargetReady (self, BDM_NOP_CMD);
        if (err)
          return err;
        *(unsigned char *)sp = self->readValue;
        return 0;

      case 2:
        err = bdmDrvSendCommandTillTargetReady (self, BDM_NOP_CMD);
        if (err)
          return err;
        *sp = self->readValue;
        return 0;

      case 3:
        err = bdmDrvSendCommandTillTargetReady (self, BDM_DUMP_CMD | BDM_SIZE_BYTE);
        if (err)
          return err;
        *sp++ = self->readValue;
        count -= 2;
        break;

      case 4:
        err = bdmDrvSendCommandTillTargetReady (self, BDM_NOP_CMD);
        if (err)
          return err;
        *sp++ = self->readValue;
        err = bdmDrvSerialClock (self, BDM_NOP_CMD, 0);
        if (err)
          return err;
        *sp = self->readValue;
        return 0;

      case 5:
        err = bdmDrvSendCommandTillTargetReady (self, BDM_NOP_CMD);
        if (err)
          return err;
        *sp++ = self->readValue;
        err = bdmDrvSendCommandTillTargetReady (self, BDM_DUMP_CMD | BDM_SIZE_BYTE);
        if (err)
          return err;
        *sp++ = self->readValue;
        count -= 4;
        break;

      case 6:
      case 7:
        err = bdmDrvSendCommandTillTargetReady (self, BDM_NOP_CMD);
        if (err)
          return err;
        *sp++ = self->readValue;
        err = bdmDrvSendCommandTillTargetReady (self, BDM_DUMP_CMD | BDM_SIZE_WORD);
        if (err)
          return err;
        *sp++ = self->readValue;
        count -= 4;
        break;

      default:
        err = bdmDrvSendCommandTillTargetReady (self, BDM_NOP_CMD);
        if (err)
          return err;
        *sp++ = self->readValue;
        err = bdmDrvSendCommandTillTargetReady (self, BDM_DUMP_CMD | BDM_SIZE_LONG);
        if (err)
          return err;
        *sp++ = self->readValue;
        count -= 4;
        break;
    }
  }
}

/*
 * Send contents of I/O buffer to target
 */
static int
bdmDrvSendBuf (struct BDM *self, int count)
{
  unsigned short *sp = (unsigned short *)self->ioBuffer;
  int cmd;
  int err;

  if (self->debugFlag)
    PRINTF ("bdmDrvSendBuf - count:%d\n", count);

  if (count == 0)
    return 0;
  
  for (;;) {
    if (count >= 4)
      cmd = BDM_FILL_CMD | BDM_SIZE_LONG;
    else if (count >= 2)
      cmd = BDM_FILL_CMD | BDM_SIZE_WORD;
    else if (count >= 1)
      cmd = BDM_FILL_CMD | BDM_SIZE_BYTE;
    else
      cmd = BDM_NOP_CMD;
    err = bdmDrvSendCommandTillTargetReady (self, cmd);
    if (err)
      return err;
    if (count >= 4) {
      err = bdmDrvSerialClock (self, *sp++, 0);
      if (err)
        return err;
      err = bdmDrvSerialClock (self, *sp++, 0);
      if (err)
        return err;
      count -= 4;
    }
    else if (count >= 2) {
      err = bdmDrvSerialClock (self, *sp++, 0);
      if (err)
        return err;
      count -= 2;
    }
    else if (count >= 1) {
      err = bdmDrvSerialClock (self, *(unsigned char *)sp, 0);
      if (err)
        return err;
      count -= 1;
    }
    else {
      return 0;
    }
  }
}

/*
 * Get a word from the target
 */
static int
bdmDrvFetchWord (struct BDM *self, unsigned short *sp)
{
  int err;

  err = bdmDrvSendCommandTillTargetReady (self, BDM_NOP_CMD);
  if (err)
    return err;
  *sp = self->readValue;
  return 0;
}

/*
 * Read system register
 */
static int
bdmDrvReadSystemRegister (struct BDM *self, struct BDMioctl *ioc)
{
  return (self->read_sysreg) (self, ioc);
}

/*
 * Read processor register
 */
static int
bdmDrvReadProcessorRegister (struct BDM *self, struct BDMioctl *ioc)
{
  int err;
  unsigned short msw, lsw;

  if (((err = bdmDrvSerialClock (self, BDM_RREG_CMD | (ioc->address & 0xF), 0)) != 0) ||
      ((err = bdmDrvFetchWord (self, &msw)) != 0) ||
      ((err = bdmDrvFetchWord (self, &lsw)) != 0)) {
    if (self->debugFlag)
      PRINTF ("bdmDrvReadProcessorRegister - reg:0x%02x, failed err=%d\n",
              ioc->address & 0xF, err);
    return err;
  }
  
  ioc->value = (msw << 16) | lsw;
  
  if (self->debugFlag)
    PRINTF ("bdmDrvReadProcessorRegister - reg:0x%02x = 0x%08x\n",
            ioc->address & 0xF, ioc->value);
  return 0;
}

/*
 * Read a long word from memory
 */
static int
bdmDrvReadLongWord (struct BDM *self, struct BDMioctl *ioc)
{
  int err;
  unsigned short msw, lsw;

  if (((err = bdmDrvSerialClock (self, BDM_READ_CMD | BDM_SIZE_LONG, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address >> 16, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address, 0)) != 0) ||
      ((err = bdmDrvFetchWord (self, &msw)) != 0) ||
      ((err = bdmDrvFetchWord (self, &lsw)) != 0)) {
    if (self->debugFlag)
      PRINTF ("bdmDrvReadLongWord : *0x%08x failed, err=%d\n", ioc->address, err);
    return err;
  }
  
  ioc->value = (msw << 16) | lsw;

  if (self->debugFlag)
    PRINTF ("bdmDrvReadLongWord : *0x%08x = 0x%08x\n", ioc->address, ioc->value);

  return 0;
}

/*
 * Read a word from memory
 */
static int
bdmDrvReadWord (struct BDM *self, struct BDMioctl *ioc)
{
  int            err;
  unsigned short w;

  if (((err = bdmDrvSerialClock (self, BDM_READ_CMD | BDM_SIZE_WORD, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address >> 16, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address, 0)) != 0) ||
      ((err = bdmDrvFetchWord (self, &w)) != 0)) {
    if (self->debugFlag)
      PRINTF ("bdmDrvReadWord : *0x%08x failed, err=%d\n", ioc->address, err);
    return err;
  }
  
  ioc->value = w;
  
  if (self->debugFlag)
    PRINTF ("bdmDrvReadWord : *0x%08x = 0x%04x\n", ioc->address, (ioc->value & 0xffff));

  return 0;
}

/*
 * Read a byte from memory
 */
static int
bdmDrvReadByte (struct BDM *self, struct BDMioctl *ioc)
{
  int            err;
  unsigned short w;

  if (((err = bdmDrvSerialClock (self, BDM_READ_CMD | BDM_SIZE_BYTE, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address >> 16, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address, 0)) != 0) ||
      ((err = bdmDrvFetchWord (self, &w)) != 0)){
  if (self->debugFlag)
    PRINTF ("bdmDrvReadByte : *0x%08x failed, err=%d\n", ioc->address, err);
    
  return err;
  }
  
  ioc->value = w;
  
  if (self->debugFlag)
    PRINTF ("bdmDrvReadByte : *0x%08x = 0x%02x\n", ioc->address, (ioc->value & 0xff));
  return 0;
}

/*
 * Write system register
 */
static int
bdmDrvWriteSystemRegister (struct BDM *self, struct BDMioctl *ioc)
{
  return (self->write_sysreg) (self, ioc);
}

/*
 * Write processor register
 */
static int
bdmDrvWriteProcessorRegister (struct BDM *self, struct BDMioctl *ioc)
{
  int err;

  if (self->debugFlag)
    PRINTF ("bdmDrvWriteProcessorRegister - reg:%d, val:0x%08x\n",
            ioc->address & 0xF, ioc->value);

  if (((err = bdmDrvSerialClock (self, BDM_WREG_CMD | (ioc->address & 0xF), 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->value >> 16, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->value, 0)) != 0))
    return err;
  return 0;
}

/*
 * Write a long word to memory
 */
static int
bdmDrvWriteLongWord (struct BDM *self, struct BDMioctl *ioc)
{
  int err;
  unsigned short w;

  if (self->debugFlag)
    PRINTF ("bdmDrvWriteLongWord : 0x%08x = 0x%08x\n", ioc->address, ioc->value);

  if (((err = bdmDrvSerialClock (self, BDM_WRITE_CMD | BDM_SIZE_LONG, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address >> 16, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->value >> 16, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->value, 0)) != 0) ||
      ((err = bdmDrvFetchWord (self, &w)) != 0))
    return err;
  return 0;
}

/*
 * Write a word to memory
 */
static int
bdmDrvWriteWord (struct BDM *self, struct BDMioctl *ioc)
{
  int err;
  unsigned short w;

  if (self->debugFlag)
    PRINTF ("bdmDrvWriteWord : 0x%08x = 0x%04x\n", ioc->address, (ioc->value & 0xffff));

  if (((err = bdmDrvSerialClock (self, BDM_WRITE_CMD | BDM_SIZE_WORD, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address >> 16, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->value, 0)) != 0) ||
      ((err = bdmDrvFetchWord (self, &w)) != 0))
    return err;
  return 0;
}

/*
 * Write a byte to memory
 */
static int
bdmDrvWriteByte (struct BDM *self, struct BDMioctl *ioc)
{
  int err;
  unsigned short w;

  if (self->debugFlag)
    PRINTF ("bdmDrvWriteByte : 0x%08x = 0x%02x\n", ioc->address, (ioc->value & 0xff));

  if (((err = bdmDrvSerialClock (self, BDM_WRITE_CMD | BDM_SIZE_BYTE, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address >> 16, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->value, 0)) != 0) ||
      ((err = bdmDrvFetchWord (self, &w)) != 0))
    return err;
  return 0;
}

#ifdef BDM_BIT_BASH_PORT

/*
 * Bit Bash the BDM signals. This is provided to allow low level
 * bit bashing support for getting new BDM or target hardware
 * working.
 */
static int
bdmDrvBitBash (struct BDM *self, struct BDMioctl *ioc)
{
  return (self->bit_bash) (self, ioc->value >> 16, ioc->value & 0xffff);
}

#endif

/*
 ************************************************************************
 *      Driver Functions which are common to the different OS's         *
 ************************************************************************
 */

/*
 * Requires the OS to define `os_claim_io_ports' and
 * `os_release_io_ports'. If your OS does not support this,
 * do nothing and return an error code of 0.
 */

static int
bdm_open (unsigned int minor)
{  
  struct BDM *self;
  int status, err = 0;

  if (minor >= (sizeof bdm_device_info / sizeof bdm_device_info[0]))
    return ENODEV;
  
  self = &bdm_device_info[minor];

  if (!self->exists)
    return ENODEV;
  if (self->isOpen)
    return EBUSY;

  /*
   * Ask the OS to try and claim the port.
   */

  err = os_claim_io_ports (self->name, self->portBase, 4);

  if (err)
    return err;
  
  self->portsAreMine = 1;
  
  if (self->debugFlag)
    PRINTF ("bdm_open -- %s using port 0x%x.\n", self->name, self->portBase);
  
  /*
   * Set up the driver.
   */

  status = bdmDrvInitHardware (self);
  
  if (status & BDM_TARGETNC)
    err = BDM_FAULT_CABLE;
  else if (status & BDM_TARGETPOWER)
    err = BDM_FAULT_POWER;

  if (err) {
    os_release_io_ports (self->portBase, 4);
    self->portsAreMine = 0;
  }
  else {
    os_lock_module ();
    self->isOpen = 1;
  }
  
  if (self->debugFlag)
    PRINTF ("BDMopen return %d, delayTimer %d\n", err, self->delayTimer);
  
  return err;
}

/*
 * The old Linux driver set the control port back to the value it
 * had when the device was opened.  This is good when the port is
 * being shared between the BDM interface and a printer, but has
 * the unfortunate side effect of freezing the target.  This makes
 * it inconvenient to use the debugger, or a downloader program,
 * since the target freezes as soon as the debugger, or downloader,
 * exits.
 */
static int
bdm_close (unsigned int minor)
{
  struct BDM *self = &bdm_device_info[minor];
  
  if (self->isOpen) {
    bdmDrvReleaseChip (self);
    if (self->exists && self->portsAreMine)
      os_release_io_ports (self->portBase, 4);
    self->portsAreMine = 0;
    self->isOpen = 0;
    os_unlock_module ();
  }

  if (self->debugFlag)
    PRINTF ("BDM released\n");

  return 0;
}

static int
bdm_ioctl (unsigned int minor, unsigned int cmd, unsigned long arg)
{
  struct BDM      *self = &bdm_device_info[minor];
  struct BDMioctl ioc;
  int             iarg;
  int             err = 0;

  /*
   * Pick up the argument
   */
  if (self->debugFlag > 3)
    PRINTF ("BDMioctl cmd:0x%x\n", cmd);
  
  switch (cmd) {
    case BDM_READ_REG:
    case BDM_READ_SYSREG:
    case BDM_READ_LONGWORD:
    case BDM_READ_WORD:
    case BDM_READ_BYTE:
    case BDM_WRITE_REG:
    case BDM_WRITE_SYSREG:
    case BDM_WRITE_LONGWORD:
    case BDM_WRITE_WORD:
    case BDM_WRITE_BYTE:
      err = os_copy_in ((void*) &ioc, (void*) arg, sizeof ioc);
      break;

    case BDM_SPEED:
    case BDM_DEBUG:      
      err = os_copy_in ((void*) &iarg, (void*) arg, sizeof iarg);
      break;
  }

  if (err)
    return err;

  /*
   * Handle the request
   */
  switch (cmd) {
    case BDM_INIT:
      iarg = bdmDrvInitHardware (self);
      if (iarg & BDM_TARGETNC)
        err = BDM_FAULT_CABLE;
      else if (iarg & BDM_TARGETPOWER)
        err = BDM_FAULT_POWER;
      break;

    case BDM_RESET_CHIP:
      err = bdmDrvResetChip (self);
      break;

    case BDM_RESTART_CHIP:
      err = bdmDrvRestartChip (self);
      break;

    case BDM_STOP_CHIP:
      err = bdmDrvStopChip (self);
      break;

    case BDM_STEP_CHIP:
      err = bdmDrvStepChip (self);
      break;

    case BDM_GO:
      err = bdmDrvGo (self);
      break;

    case BDM_GET_STATUS:
      iarg = bdmDrvGetStatus (self);
      break;

    case BDM_SPEED:
      self->delayTimer = iarg;
      if (self->debugFlag)
        PRINTF ("delayTimer now %d\n", self->delayTimer);
      break;

    case BDM_DEBUG:
      self->debugFlag = iarg;
      PRINTF ("debugFlag now %d\n", self->debugFlag);
      break;

    case BDM_RELEASE_CHIP:
      err = bdmDrvReleaseChip (self);
      break;

    case BDM_READ_SYSREG:
      err = bdmDrvReadSystemRegister (self, &ioc);
      break;

    case BDM_READ_REG:
      err = bdmDrvReadProcessorRegister (self, &ioc);
      break;

    case BDM_READ_LONGWORD:
      err = bdmDrvReadLongWord (self, &ioc);
      break;

    case BDM_READ_WORD:
      err = bdmDrvReadWord (self, &ioc);
      break;

    case BDM_READ_BYTE:
      err = bdmDrvReadByte (self, &ioc);
      break;

    case BDM_WRITE_SYSREG:
      err = bdmDrvWriteSystemRegister (self, &ioc);
      break;

    case BDM_WRITE_REG:
      err = bdmDrvWriteProcessorRegister (self, &ioc);
      break;

    case BDM_WRITE_LONGWORD:
      err = bdmDrvWriteLongWord (self, &ioc);
      break;

    case BDM_WRITE_WORD:
      err = bdmDrvWriteWord (self, &ioc);
      break;

    case BDM_WRITE_BYTE:
      err = bdmDrvWriteByte (self, &ioc);
      break;

    case BDM_GET_CPU_TYPE:
      iarg = self->processor;
      break;

    case BDM_GET_IF_TYPE:
      iarg = self->interface;
      break;

    case BDM_GET_DRV_VER:
      iarg = BDM_DRV_VERSION;
      break;

    default:
      err = EINVAL;
      break;
  }

  if (err)
    return err;

  /*
   * Return the result if no error was found.
   */
  switch (cmd) {
    case BDM_READ_REG:
    case BDM_READ_SYSREG:
    case BDM_READ_LONGWORD:
    case BDM_READ_WORD:
    case BDM_READ_BYTE:
      err = os_copy_out ((void*) arg, (void*) &ioc, sizeof ioc);
      break;

    case BDM_GET_STATUS:
    case BDM_GET_CPU_TYPE:
    case BDM_GET_IF_TYPE:
    case BDM_GET_DRV_VER:
      err = os_copy_out ((void*) arg, (void*) &iarg, sizeof iarg);
  }

  return err;
}

static int
bdm_read (unsigned int minor, char *buf, int count)
{
  struct BDM *self = &bdm_device_info[minor];
  int        nleft, ncopy;
  int        err;
  
  nleft = count;
  while (nleft) {
    if (nleft > sizeof self->ioBuffer)
      ncopy = sizeof self->ioBuffer;
    else
      ncopy = nleft;
    err = bdmDrvFillBuf (self, ncopy);
    if (err)
      return err;
    err = os_move_out (buf, self->ioBuffer, ncopy);
    if (err)
      return err;
    nleft -= ncopy;
#ifndef BUF_INCREMENTED_BY_MOVE_OUT
    buf += ncopy;
#endif
  }
  return 0;
}

static int
bdm_write (unsigned int minor, char *buf, int count)
{  
  struct BDM *self = &bdm_device_info[minor];
  int        nleft, ncopy;
  int        err;

  nleft = count;
  while (nleft) {
    if (nleft > sizeof self->ioBuffer)
      ncopy = sizeof self->ioBuffer;
    else
      ncopy = nleft;
    err = os_move_in (self->ioBuffer, (void*) buf, ncopy);
    if (err)
      return err;
    err = bdmDrvSendBuf (self, ncopy);
    if (err)
      return err;
    nleft -= ncopy;
#ifndef BUF_INCREMENTED_BY_MOVE_IN
    buf += ncopy;
#endif
  }
  return 0;
}
