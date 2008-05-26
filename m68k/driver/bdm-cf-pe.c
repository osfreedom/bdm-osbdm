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
 */

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
 * Coldfire system register mapping. See bdm.h for the user values.
 *
 * For a RCREG 0x1 is invalid.
 */

static int cf_sysreg_map[BDM_REG_DBMR + 1] =
{ 0x80f,    /* BDM_REG_RPC      */
  -1,       /* BDM_REG_PCC      */
  0x80e,    /* BDM_REG_SR       */
  -1,       /* BDM_REG_USP      */
  -1,       /* BDM_REG_SSP, use A7    */
  -1,       /* BDM_REG_SFC      */
  -1,       /* BDM_REG_DFC      */
  -1,       /* BDM_REG_ATEMP    */
  -1,       /* BDM_REG_FAR      */
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

static int cf_pe_read_sysreg (struct BDM *self, struct BDMioctl *ioc, int mode);
static int cf_pe_write_sysreg (struct BDM *self, struct BDMioctl *ioc, int mode);
static int cf_pe_reset_chip (struct BDM *self);

#define CF_REVISION_A (0)
#define CF_REVISION_B (1)
#define CF_REVISION_C (2)
#define CF_REVISION_D (3)

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

  if (cf_pe_read_sysreg (self, &cacr_ioc, BDM_SYS_REG_MODE_MAPPED) < 0)
      return BDM_TARGETNC;

  /*
   * Set the invalidate bit.
   */

  if (cacr_ioc.value) {
    /* CCJ: Add version C as I think the version D was wrong. */
    if ((self->cf_debug_ver == CF_REVISION_C) ||
        (self->cf_debug_ver == CF_REVISION_D))
      cacr_ioc.value |= 0x01040100;
    else
      cacr_ioc.value |= 0x01000100;

    if (self->debugFlag > 2)
      PRINTF (" cf_pe_invalidate_cache -- cacr:0x%08x\n", (int) cacr_ioc.value);
  
    if (cf_pe_write_sysreg (self, &cacr_ioc, BDM_SYS_REG_MODE_MAPPED) < 0)
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

  if (cf_pe_read_sysreg (self, &pc_ioc, BDM_SYS_REG_MODE_MAPPED) < 0)
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

    if (cf_pe_read_sysreg (self, &csr_ioc, BDM_SYS_REG_MODE_MAPPED) < 0)
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
  cf_pe_reset_chip (self);

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
   * By default we want to use PST lines.
   */
  
  self->cf_use_pst = 1;

  /*
   * Process the result of the CSR read.
   */

  self->cf_debug_ver = (self->cf_csr >> 20) & 0x0f;

  if (self->debugFlag)
    PRINTF (" cf_ps_init_hardware: debug version is %d, PST %s\n",
            self->cf_debug_ver, self->cf_use_pst ? "enabled" : "disabled");
  
  return status;
}

/*
 * Clock a word to/from the target
 */
static void
cf_pe_serial_clocker (struct BDM *self, unsigned short wval, int holdback)
{
  unsigned long shiftRegister;
  unsigned char dataBit;
  unsigned int  counter;

  shiftRegister = wval;
  counter       = 17 - holdback;
  
  while (counter--) {
    dataBit = ((shiftRegister & 0x10000) ? CF_PE_DR_DATA_IN : 0);

    shiftRegister <<= 1;
    
    outb (CF_PE_MAKE_DR (dataBit), self->dataPort);

    if (self->delayTimer)
      bdm_delay (self->delayTimer << 1);

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
}

static int
cf_pe_serial_clock (struct BDM *self, unsigned short wval, int holdback)
{
  unsigned int status = cf_pe_get_direct_status (self);

  if (status & BDM_TARGETRESET)
    return BDM_FAULT_RESET;
  if (status & BDM_TARGETNC)
    return BDM_FAULT_CABLE;
  if (status & BDM_TARGETPOWER)
    return BDM_FAULT_POWER;
  
  cf_pe_serial_clocker (self, wval, holdback);
  
  if (self->readValue & 0x10000) {
    if (self->readValue == 0x10001) {
      if (self->debugFlag > 1)
        PRINTF (" cf_pe_serial_clock -- failure BUS ERROR, send 0x%x receive 0x%x\n",
                wval, self->readValue);
      return BDM_FAULT_BERR;
    }
    else if (self->readValue != 0x10000) {
      int i;
      if (self->debugFlag > 1)
        PRINTF (" cf_pe_serial_clock -- failure NVC, send 0x%x receive 0x%x\n",
                wval, self->readValue);
      /*
       * Force TA on a Coldfire if supported. We cannot do much else if
       * the Coldfire does not support the Forced TA command.
       */
      if (self->cf_debug_ver >= CF_REVISION_D) {
        if (self->debugFlag)
          PRINTF (" cf_pe_serial_clock --  forced ta\n");
        cf_pe_serial_clocker (self, BDM_FORCED_TA_CMD, holdback);
        cf_pe_serial_clocker (self, BDM_NOP_CMD, 0);
        cf_pe_serial_clocker (self, BDM_NOP_CMD, 0);
        return BDM_FAULT_FORCED_TA;
      }
      return BDM_FAULT_NVC;
      for (i = 0; i < 4; i++)
        cf_pe_serial_clocker (self, BDM_NOP_CMD, 0);
#if EXPERIMENTAL_RECOVER
      for (i = 17; i > 0; i--)
        if ((self->readValue & (1 << 16)) == 0)
          break;
      if (i && (i < 17))
        cf_pe_serial_clocker (self, BDM_NOP_CMD, 17 - i);
#endif
    }
  }
  return 0;
}

/*
 * Read system register
 */

static int 
cf_pe_read_sysreg (struct BDM *self, struct BDMioctl *ioc, int mode)
{
  int            err;
  unsigned short msw, lsw;
  int            cmd;

  if (self->debugFlag > 2)
    PRINTF (" cf_pe_read_sysreg + Reg(%d):0x%x\n", mode, ioc->address);
  
  if ((mode == BDM_SYS_REG_MODE_MAPPED) && (ioc->address >= BDM_MAX_SYSREG))
    return BDM_FAULT_NVC;

  /*
   * For the Coldfire we need to select the type of command
   * to use.
   */
  if ((mode == BDM_SYS_REG_MODE_CONTROL) ||
      ((mode == BDM_SYS_REG_MODE_MAPPED) && (ioc->address < BDM_REG_CSR))) {
    if (mode == BDM_SYS_REG_MODE_CONTROL)
      cmd = ioc->address & 0xffff;
    else
      cmd = cf_sysreg_map[ioc->address];

    if (cmd == -1) {
      ioc->value = 0;
      if (self->debugFlag)
        PRINTF (" cf_pe_read_sysreg - Reg(%d):0x%x is not mapped; ignored\n",
                mode, ioc->address);
      return 0;
    }

    if (((err = cf_pe_serial_clock (self, BDM_RCREG_CMD, 0)) != 0) ||
        ((err = cf_pe_serial_clock (self, 0, 0)) != 0) ||
        ((err = cf_pe_serial_clock (self, cmd, 0)) != 0) ||
        ((err = bdmBitBashFetchWord (self, &msw)) != 0) ||
        ((err = bdmBitBashFetchWord (self, &lsw)) != 0))
      return err;
  }
  else {
    if (mode == BDM_SYS_REG_MODE_DEBUG) {
      int r;
      cmd = ioc->address & 0xffff;
      /*
       * See if the register is actually mapped and therefore write only.
       */
      for (r = BDM_REG_CSR; r < BDM_MAX_SYSREG; r++) {
        if (cf_sysreg_map[r] == (ioc->address & 0xffff)) {
          mode = BDM_SYS_REG_MODE_MAPPED;
          ioc->address = r;
          if (self->debugFlag > 2)
            PRINTF (" cf_pe_read_sysreg - remapped to Reg:0x%x\n", ioc->address);
          break;
        }
      }
    }
    else
      cmd = cf_sysreg_map[ioc->address];

    if (cmd == -1) {
      ioc->value = 0;
      if (self->debugFlag)
        PRINTF (" cf_pe_read_sysreg - Reg(%d):0x%x is not mapped; ignored\n",
                mode, ioc->address);
      return 0;
    }

    cmd |= BDM_RDMREG_CMD;

    /*
     * Are the registers write only ?
     */
    if ((mode == BDM_SYS_REG_MODE_MAPPED) && (ioc->address != BDM_REG_CSR)) {
      ioc->value = self->shadow_sysreg[ioc->address];
      if (self->debugFlag > 1)
        PRINTF (" cf_pe_read_sysreg - Reg:0x%x is write only, 0x%08x\n",
                ioc->address, ioc->value);
      return 0;
    }
  
    if (((err = cf_pe_serial_clock (self, cmd, 0)) != 0) ||
        ((err = bdmBitBashFetchWord (self, &msw)) != 0) ||
        ((err = bdmBitBashFetchWord (self, &lsw)) != 0)) {
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
    PRINTF (" cf_pe_read_sysreg - Reg(%d):0x%x is 0x%08x\n",
            mode, ioc->address, ioc->value);
  
  return 0;
}

/*
 * Write system register
 */
static int
cf_pe_write_sysreg (struct BDM *self, struct BDMioctl *ioc, int mode)
{
  int err;
  int cmd;
  
  if ((mode == BDM_SYS_REG_MODE_MAPPED) && (ioc->address >= BDM_MAX_SYSREG))
    return BDM_FAULT_NVC;
  
  if (self->debugFlag)
    PRINTF (" cf_pe_write_sysreg - Reg(%d):0x%x is 0x%x\n",
            mode, ioc->address, ioc->value);
  
  /*
   * For the Coldfire we need to select the type of command
   * to use.
   */
  if ((mode == BDM_SYS_REG_MODE_CONTROL) ||
      ((mode == BDM_SYS_REG_MODE_MAPPED) && (ioc->address < BDM_REG_CSR))) {
    if (mode == BDM_SYS_REG_MODE_CONTROL)
      cmd = ioc->address & 0xffff;
    else
      cmd = cf_sysreg_map[ioc->address];

    if (cmd == -1) {
      ioc->value = 0;
      if (self->debugFlag)
        PRINTF (" cf_pe_write_sysreg - Reg(%d):0x%x is not mapped; ignored\n",
                mode, ioc->address);
      return 0;
    }

    if (((err = cf_pe_serial_clock (self, BDM_WCREG_CMD, 0)) != 0) ||
        ((err = cf_pe_serial_clock (self, 0, 0)) != 0) ||
        ((err = cf_pe_serial_clock (self, cmd, 0)) != 0) ||
        ((err = cf_pe_serial_clock (self, ioc->value >> 16, 0)) != 0) ||
        ((err = cf_pe_serial_clock (self, ioc->value, 0)) != 0))
      return err;
  }
  else {
    if (mode == BDM_SYS_REG_MODE_DEBUG)
      cmd = ioc->address & 0xffff;
    else
      cmd = cf_sysreg_map[ioc->address];
    
    if (cmd == -1) {
      if (self->debugFlag)
        PRINTF (" tblcf_write_sysreg - Reg(%d):0x%x is not mapped; ignored\n",
                mode, ioc->address);
      return 0;
    }

    cmd |= BDM_WDMREG_CMD;

    if (mode == BDM_SYS_REG_MODE_MAPPED)
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
  if (self->cf_debug_ver < CF_REVISION_D) {
    if (self->debugFlag)
      PRINTF (" cf_pe_gen_bus_error\n");
  
    outb (CF_PE_MAKE_DR (CF_PE_DR_TEA), self->dataPort);
    udelay (400);
    outb (CF_PE_MAKE_DR (0), self->dataPort);
  
    return BDM_FAULT_BERR;
  } else {
    int err;
    if (self->debugFlag)
      PRINTF (" cf_pe_gen_bus_error - forced ta\n");
    if (((err = cf_pe_serial_clock (self, BDM_FORCED_TA_CMD, 0)) != 0) ||
        ((err = cf_pe_serial_clock (self, BDM_NOP_CMD, 0)) != 0))
      return err;
    return BDM_FAULT_FORCED_TA;
  }
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

  cf_pe_reset_chip (self);
  
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
  bdm_sleep (250);
  outb (CF_PE_MAKE_DR (CF_PE_DR_RESET), self->dataPort);
  bdm_sleep (500);
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
  bdm_sleep (100);
//  outb (CF_PE_MAKE_DR (CF_PE_DR_RESET), self->dataPort);
//  bdm_sleep (100);
  outb (CF_PE_MAKE_DR (CF_PE_DR_RESET | CF_PE_DR_BKPT), self->dataPort);
  bdm_sleep (500);
  outb (CF_PE_MAKE_DR (CF_PE_DR_BKPT), self->dataPort);
  bdm_sleep (1500);
  outb (CF_PE_MAKE_DR (0), self->dataPort);
  bdm_sleep (500);

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
  
    bdm_sleep (10);
    status = cf_pe_get_status (self);
    
    outb (CF_PE_MAKE_DR (0), self->dataPort);
    
    if (status & (BDM_TARGETHALT | BDM_TARGETSTOPPED)) {
      return 0;
    }
  }
  
  return BDM_FAULT_RESPONSE;
}

/*
 * Restart target execution.
 */
static int
cf_pe_run_chip (struct BDM *self)
{
  struct BDMioctl sreg_ioc;
  struct BDMioctl pc_ioc;
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
  sreg_ioc.address = BDM_REG_CSR;
  sreg_ioc.value   = 0x00000000;
  if ((err = cf_pe_write_sysreg (self, &sreg_ioc, BDM_SYS_REG_MODE_MAPPED)) < 0)
    return err;

  /*
   * Revision D hardware does not have a bit to mask interrupts so
   * we save the SR if we step and need to restore it now.
   *
   * The SR needs to be read back and the mask bits set as the
   * other SR bits may have changed.
   *   
   * It appears that the V4 core has some pipelining in effect that
   * causes instruction fetches to already be cached.  This could be
   * problem in certain instances for stepping.  To resolve this,
   * we read the PC value, and then write it right back out.
   */

  if ((self->cf_debug_ver == CF_REVISION_D) && self->cf_sr_masked) {
    sreg_ioc.address = BDM_REG_SR;
    if ((err = cf_pe_read_sysreg (self, &sreg_ioc,
                                  BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    sreg_ioc.value &= ~0x0700;
    sreg_ioc.value |= self->cf_sr_mask_cache;
    if ((err = cf_pe_write_sysreg (self, &sreg_ioc,
                                   BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    self->cf_sr_masked = 0;

    /*
     * Read the PC value and write it back out
     */
    pc_ioc.address = BDM_REG_RPC;
    if ((err = cf_pe_read_sysreg (self, &pc_ioc,
                                  BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    if ((err = cf_pe_write_sysreg (self, &pc_ioc,
                                   BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
  }

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
  struct BDMioctl sreg_ioc;
  struct BDMioctl csr_ioc;
  struct BDMioctl pc_ioc;
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

  if ((err = cf_pe_write_sysreg (self, &csr_ioc, BDM_SYS_REG_MODE_MAPPED)) < 0)
    return err;

  /*
   * Revision D hardware does not have a bit to mask interrupts so
   * we save the SR when we step and then restore when we go
   *
   * The SR needs to be read back and the mask bits set as the
   * other SR bits may have changed.
   *
   * It appears that the V4 core has some pipelining in effect that
   * causes instruction fetches to already be cached. This could be
   * problem in certain instances for stepping. To resolve this,
   * we read the PC value, and then write it right back out.
   */
  if ((self->cf_debug_ver == CF_REVISION_D) && !self->cf_sr_masked) {
    sreg_ioc.address = BDM_REG_SR;
    if ((err = cf_pe_read_sysreg (self, &sreg_ioc,
                                  BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    self->cf_sr_mask_cache = sreg_ioc.value & 0x0700;
    sreg_ioc.value |= 0x0700;
    if ((err = cf_pe_write_sysreg (self, &sreg_ioc,
                                   BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    self->cf_sr_masked = 1;

    /*
     * Read the PC value and write it back out
     */
    pc_ioc.address = BDM_REG_RPC;
    if ((err=cf_pe_read_sysreg (self, &pc_ioc,
                                BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    if ((err=cf_pe_write_sysreg (self, &pc_ioc,
                                 BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
  }

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

  self->get_status      = cf_pe_get_status;
  self->init_hardware   = cf_pe_init_hardware;
  self->serial_clock    = cf_pe_serial_clock;
  self->gen_bus_error   = cf_pe_gen_bus_error;
  self->restart_chip    = cf_pe_restart_chip;
  self->release_chip    = cf_pe_release_chip;
  self->reset_chip      = cf_pe_reset_chip;
  self->stop_chip       = cf_pe_stop_chip;
  self->run_chip        = cf_pe_run_chip;
  self->step_chip       = cf_pe_step_chip;
  self->fill_buf        = bdmBitBashFillBuf;
  self->send_buf        = bdmBitBashSendBuf;
  self->read_sysreg     = cf_pe_read_sysreg;
  self->read_proreg     = bdmBitBashReadProcessorRegister;
  self->read_long_word  = bdmBitBashReadLongWord;
  self->read_word       = bdmBitBashReadWord;
  self->read_byte       = bdmBitBashReadByte;
  self->write_sysreg    = cf_pe_write_sysreg;
  self->write_proreg    = bdmBitBashWriteProcessorRegister;
  self->write_long_word = bdmBitBashWriteLongWord;
  self->write_word      = bdmBitBashWriteWord;
  self->write_byte      = bdmBitBashWriteByte;

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
  self->cf_use_pst   = 1;
  self->cf_running   = 1;
  self->cf_csr       = 0;
  
  self->cf_sr_masked = 0;
  
  return 0;
}

