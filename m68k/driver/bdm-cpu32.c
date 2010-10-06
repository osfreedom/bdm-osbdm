/*
 * Motorola Background Debug Mode Driver
 * Copyright (C) 1995  W. Eric Norum
 * Copyright (C) 1998-2008  Chris Johns
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
 * chris@contemporary.net.au
 *
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
static int cpu32_write_sysreg (struct BDM *self, struct BDMioctl *ioc, int mode);
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

  if (self->debugFlag)
    PRINTF (" cpu32_icd_stop_chip: ");
  /* if FREEZE is already high, we're stopped and we're done here */
  if (inb (self->statusPort) & CPU32_ICD_FREEZE) {
    if (self->debugFlag)
      PRINTF ("already stopped\n");
    return 0;
  }

  /* try multiple times... */
  for (pass = 0; pass < 14; pass++) {

    /* even times, simply assert DSCLK and RESET */
    if (pass%2 == 0) {
      outb (CPU32_ICD_DSCLK | CPU32_ICD_RST_OUT, self->dataPort);
    }
    /* odd times, yank BERR as well, in case the target is wedged */
    else {
      outb (CPU32_ICD_DSCLK | CPU32_ICD_RST_OUT | CPU32_ICD_FORCE_BERR,
            self->dataPort);
    }

    /* now hang around and wait for the freeze line to come up
     * XXX we're depending on a nop loop for timing?  arrrgh!
     */
    for (check = 0 ; check < (1000 + ((pass+1)%2) * 9000) ; check++) {
      if (inb (self->statusPort) & CPU32_ICD_FREEZE) {
        /* if freeze line is high we're OK
         * XXX let reset go too?
         */
        if (self->debugFlag)
          PRINTF("stopped after %d bdm_delays\n", check);
        outb (CPU32_ICD_RST_OUT, self->dataPort);
        return 0;
      }
      bdm_delay (10);
    }

  }
  /* we've failed... */
  outb (CPU32_ICD_RST_OUT, self->dataPort);
  if (self->debugFlag)
    PRINTF("failed!\n");
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
cpu32_read_sysreg (struct BDM *self, struct BDMioctl *ioc, int mode)
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
    if ((err = cpu32_read_sysreg (self, &mbar_ioc,
                                  BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    sfc = mbar_ioc.value;
    mbar_ioc.address = BDM_REG_SFC;
    mbar_ioc.value = 7;
    if ((err = cpu32_write_sysreg (self, &mbar_ioc,
                                   BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    mbar_ioc.address = 0x3FF00;
    if ((err = bdmDrvReadLongWord (self, &mbar_ioc)) < 0)
      return err;
    ioc->value = mbar_ioc.value;
    mbar_ioc.address = BDM_REG_SFC;
    mbar_ioc.value = sfc;
    if ((err = cpu32_write_sysreg (self, &mbar_ioc,
                                   BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    return 0;
  }
  
  if (ioc->address > BDM_REG_VBR)
    return BDM_FAULT_NVC;

  if (mode != BDM_SYS_REG_MODE_MAPPED)
    cmd = ioc->address & 0xffff;
  else
    cmd = cpu32_sysreg_map[ioc->address];
  
  if (cmd == -1) {
    ioc->value = 0;
    if (self->debugFlag)
      PRINTF (" cpu32_read_sysreg - Reg(%d):0x%lx is not mapped; ignored\n",
              mode, ioc->address);
    return 0;
  }

  cmd |= BDM_RSREG_CMD;
  
  if (((err = cpu32_serial_clock (self, cmd, 0)) != 0) ||
      ((err = bdmBitBashFetchWord (self, &msw)) != 0) ||
      ((err = bdmBitBashFetchWord (self, &lsw)) != 0))
    return err;
  ioc->value = (msw << 16) | lsw;
  return 0;
}

/*
 * Write system register
 */
static int
cpu32_write_sysreg (struct BDM *self, struct BDMioctl *ioc, int mode)
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
    if ((err = cpu32_read_sysreg (self, &mbar_ioc,
                                  BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    dfc = mbar_ioc.value;
    mbar_ioc.address = BDM_REG_DFC;
    mbar_ioc.value = 7;
    if ((err = cpu32_write_sysreg (self, &mbar_ioc,
                                   BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    mbar_ioc.address = 0x3FF00;
    mbar_ioc.value = ioc->value;
    if ((err = bdmDrvWriteLongWord (self, &mbar_ioc)) < 0)
      return err;
    mbar_ioc.address = BDM_REG_DFC;
    mbar_ioc.value = dfc;
    if ((err = cpu32_write_sysreg (self, &mbar_ioc,
                                   BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    return 0;
  }
  
  if (ioc->address > BDM_REG_VBR)
    return BDM_FAULT_NVC;
  
  if (mode != BDM_SYS_REG_MODE_MAPPED)
    cmd = ioc->address & 0xffff;
  else
    cmd = cpu32_sysreg_map[ioc->address];
  
  cmd = BDM_WSREG_CMD;
  
  if (((err = cpu32_serial_clock (self, cmd, 0)) != 0) ||
      ((err = cpu32_serial_clock (self, ioc->value >> 16, 0)) != 0) ||
      ((err = cpu32_serial_clock (self, ioc->value, 0)) != 0))
    return err;
  return 0;
}

/*
 * Generate a bus error for the ICD interface
 */

static int 
cpu32_icd_gen_bus_error (struct BDM *self)
{
  if (self->debugFlag)
    PRINTF(" cpu32_icd_gen_bus_error\n");

  outb (CPU32_ICD_FORCE_BERR | CPU32_ICD_RST_OUT, self->dataPort);
  udelay (400);
  outb (CPU32_ICD_RST_OUT, self->dataPort);

  return BDM_FAULT_BERR;
}

/*
 * Generate a bus error as the access has failed. This is
 * not supported on the CPU32 with PD interface.
 * (the 7-chip PD interface generates it automatically in hardware
 */
static int
cpu32_gen_bus_error (struct BDM *self)
{
  if (self->debugFlag > 1)
    PRINTF(" cpu32_gen_bus_error\n");
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
int
bdm_cpu32_pd_init_self (struct BDM *self)
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
  
int
bdm_cpu32_icd_init_self (struct BDM *self)
{
  int reg;
  
  self->processor = BDM_CPU32;
  self->interface = BDM_CPU32_ICD;

  self->get_status      = cpu32_icd_get_status;
  self->init_hardware   = cpu32_icd_init_hardware;
  self->serial_clock    = cpu32_icd_serial_clock;
  self->gen_bus_error   = cpu32_icd_gen_bus_error;
  self->restart_chip    = cpu32_icd_restart_chip;
  self->release_chip    = cpu32_icd_release_chip;
  self->reset_chip      = cpu32_icd_reset_chip;
  self->stop_chip       = cpu32_icd_stop_chip;
  self->run_chip        = cpu32_run_chip;
  self->step_chip       = cpu32_icd_step_chip;
  self->fill_buf        = bdmBitBashFillBuf;
  self->send_buf        = bdmBitBashSendBuf;
  self->read_sysreg     = cpu32_read_sysreg;
  self->read_proreg     = bdmBitBashReadProcessorRegister;
  self->read_long_word  = bdmBitBashReadLongWord;
  self->read_word       = bdmBitBashReadWord;
  self->read_byte       = bdmBitBashReadByte;
  self->write_sysreg    = cpu32_write_sysreg;
  self->write_proreg    = bdmBitBashWriteProcessorRegister;
  self->write_long_word = bdmBitBashWriteLongWord;
  self->write_word      = bdmBitBashWriteWord;
  self->write_byte      = bdmBitBashWriteByte;
  
#ifdef BDM_BIT_BASH_PORT
  self->bit_bash      = cpu32_bit_bash;
  self->bit_bash_bits = 0;
#endif
  
  for (reg = 0; reg < BDM_MAX_SYSREG; reg++)
    self->shadow_sysreg[reg] = 0;

  return 0;
}

