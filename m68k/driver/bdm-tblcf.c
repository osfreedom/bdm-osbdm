/*
 * Motorola Background Debug Mode Driver
 * Copyright (C) 2008  Chris Johns
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
 * Coldfire TBLCF USB support by:
 * Chris Johns
 * cjohns@users.sourceforge.net
 */

#include <tblcf.h>

static int tblcf_read_sysreg (struct BDM *self,
                              struct BDMioctl *ioc, int mode);
static int tblcf_write_sysreg (struct BDM *self,
                               struct BDMioctl *ioc, int mode);
static int tblcf_reset_chip (struct BDM *self);
static int tblcf_stop_chip (struct BDM *self);

/*
 * Get target status
 */

static int
tblcf_get_status (struct BDM *self)
{
  int            cf_last_running = self->cf_running;
  bdmcf_status_t bdmcf_status;
  int            status = 0;
  unsigned long  csr;
  int            ret;

  /*
   * TBLCF does not report target power.
   */

  if (tblcf_bdm_sts (self->usbDev, &bdmcf_status))
    status = BDM_TARGETNC;
  else {
    if (bdmcf_status.reset_detection == RESET_DETECTED)
      status |= BDM_TARGETRESET;
  }
  
  if (status & (BDM_TARGETRESET | BDM_TARGETNC | BDM_TARGETPOWER)) {
    PRINTF (" tblcf_get_status -- Status:0x%x\n", status);
    return status;
  }

  /*
   * The actual hard work is done when reading the system register. Check this
   * function for the issues in reading the CSR register.
   */
  if (self->cf_running) {
    struct BDMioctl csr_ioc;

    csr_ioc.address = BDM_REG_CSR;

    if (tblcf_read_sysreg (self, &csr_ioc, BDM_SYS_REG_MODE_MAPPED) < 0)
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
    bdm_invalidate_cache (self);
  
  if (self->debugFlag > 2)
    PRINTF (" tblcf_get_status -- Status:0x%x, csr:0x%08x, cf_csr:0x%08x\n",
            ret, (int) csr, (int) self->cf_csr);
  
  return ret;
}

/*
 * Hardware initialization. The open.
 */
static int
tblcf_init_hardware (struct BDM *self)
{
  int status;

  /*
   * Swap the swap state as the USB pod will swap.
   *
   * The swap of the swap will work on Intel devices but I am
   * not sure of other platforms.
   */
  mustSwap = mustSwap ? 0 : 1;

  /*
   * We do not know which version of debug module we have so
   * default to 0 and do not use PST signals. This will cause the
   * CSR register to be read.
   */

  self->cf_running   = 1;
  self->cf_debug_ver = 0;
  self->cf_use_pst   = 0;
  self->address      = 0;

  tblcf_reset_chip (self);
  
  status = tblcf_get_status (self);

  if (self->debugFlag)
    PRINTF (" tblcf_init_hardware: status:%d\n", status);

  /*
   * Process the result of the CSR read.
   */

  self->cf_debug_ver = (self->cf_csr >> 20) & 0x0f;

  if (self->debugFlag)
    PRINTF (" tblcf_init_hardware: debug version is %d\n", self->cf_debug_ver);
  
  return status;
}

static int
tblcf_serial_clock (struct BDM *self, unsigned short wval, int holdback)
{
  PRINTF (" tblcf_serial_clock --  should never be called\n");
  return 0;
}

/*
 * Read system register
 */

static int 
tblcf_read_sysreg (struct BDM *self, struct BDMioctl *ioc, int mode)
{
  int cmd;

  if (self->debugFlag > 2)
    PRINTF (" tblcf_read_sysreg + Reg(%d):0x%x\n", mode, ioc->address);
  
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
        PRINTF (" tblcf_read_sysreg - Reg(%d):0x%x is not mapped; ignored\n",
                mode, ioc->address);
      return 0;
    }

    if (tblcf_read_creg (self->usbDev, cmd, &ioc->value)) {
      PRINTF (" tblcf_read_sysreg - Reg:0x%x failed with cmd 0x%02x\n",
              ioc->address, cmd);
      return BDM_FAULT_RESPONSE;
    }
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
            PRINTF (" tblcf_read_sysreg - remapped to Reg:0x%x\n", ioc->address);
          break;
        }
      }
    }
    else
      cmd = cf_sysreg_map[ioc->address];

    if (cmd == -1) {
      ioc->value = 0;
      if (self->debugFlag)
        PRINTF (" tblcf_read_sysreg - Reg(%d):0x%x is not mapped; ignored\n",
                mode, ioc->address);
      return 0;
    }

    /*
     * Are the registers write only ?
     */
    if ((mode == BDM_SYS_REG_MODE_MAPPED) && (ioc->address != BDM_REG_CSR)) {
      ioc->value = self->shadow_sysreg[ioc->address];
      if (self->debugFlag > 1)
        PRINTF (" tblcf_read_sysreg - Reg:0x%x is write only, 0x%08x\n",
                ioc->address, ioc->value);
      return 0;
    }

    if (tblcf_read_dreg (self->usbDev, cmd, &ioc->value)) {
      PRINTF (" tblcf_read_sysreg - Reg:0x%x failed with cmd 0x%02x\n",
              ioc->address, cmd);
      return BDM_FAULT_RESPONSE;
    }
  }
  
  /*
   * Watch out here as this function is called from tblcf_get_status.
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

        if ((bdm_pc_read_check (self) == 0) &&
            (bdm_pc_read_check (self) == 0)) {
          self->cf_csr     = ioc->value;
          self->cf_running = 0;
        
          /*
           * If we were running, and we have detected we have stopped
           * flush the cache.  This should probably be somewhere else
           * if we are using PST,  I am not sure yet : davidm
           */
          
          bdm_invalidate_cache (self);
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
    PRINTF (" tblcf_read_sysreg - Reg(%d):0x%x is 0x%08x\n",
            mode, ioc->address, ioc->value);
  
  return 0;
}

/*
 * Write system register
 */
static int
tblcf_write_sysreg (struct BDM *self, struct BDMioctl *ioc, int mode)
{
  int cmd;
  
  if ((mode == BDM_SYS_REG_MODE_MAPPED) && (ioc->address >= BDM_MAX_SYSREG))
    return BDM_FAULT_NVC;

  if (self->debugFlag)
    PRINTF (" tblcf_write_sysreg - Reg(%d):0x%x is 0x%x\n",
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
      if (self->debugFlag)
        PRINTF (" tblcf_write_sysreg - Reg(%d):0x%x is not mapped; ignored\n",
                mode, ioc->address);
      return 0;
    }

    tblcf_write_creg (self->usbDev, cmd, ioc->value);
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

    if (mode == BDM_SYS_REG_MODE_MAPPED)
      self->shadow_sysreg[ioc->address] = ioc->value;

    tblcf_write_dreg (self->usbDev, cmd, ioc->value);
  }

  if (tblcf_get_last_sts (self->usbDev)) {
    if (self->debugFlag)
      PRINTF (" tblcf_write_sysreg - Reg(%d):0x%x write failed\n",
              mode, ioc->address);
    return BDM_FAULT_RESPONSE;
  }
    
  return 0;
}

/*
 * Generate a bus error as the access has failed. This is
 * not supported on the CPU32.
 */

static int 
tblcf_gen_bus_error (struct BDM *self)
{
  if (self->debugFlag)
    PRINTF (" tblcf_gen_bus_error\n");
  
  tblcf_assert_ta (self->usbDev, 100);
  
  return BDM_FAULT_FORCED_TA;
}

/*
 * Restart chip and stop on first instruction fetch. Easy for the
 * Coldfire, keep BKPT asserted during the first 16 clocks after
 * reset is negated.
 */
static int
tblcf_restart_chip (struct BDM *self)
{
  if (self->debugFlag)
    PRINTF (" tblcf_restart_chip\n");

  tblcf_reset_chip (self);
  
  self->cf_running = 1;
  
  if (tblcf_get_status (self) & (BDM_TARGETHALT | BDM_TARGETSTOPPED))
    return 0;

  return BDM_FAULT_RESPONSE;
}

/*
 * Restart chip and disable background debugging mode. On
 * a Coldfire exit reset without the BKPT being asserted.
 */
static int
tblcf_release_chip (struct BDM *self)
{
  if (self->debugFlag)
    PRINTF (" tblcf_release_chip\n");
  
  self->cf_running = 1;

  tblcf_target_go (self->usbDev);
  tblcf_close(self->usbDev);
  
  return 0;
}

/*
 * Restart chip, the coldfire is easier than the CPU32 to get into
 * BMD mode after reset. Just have the BKPT signal active when
 * reset is negated.
 */
static int
tblcf_reset_chip (struct BDM *self)
{
  if (self->debugFlag)
    PRINTF (" tblcf_reset_chip\n");

  self->cf_running = 1;
  
  if (tblcf_target_reset (self->usbDev, BDM_MODE)) {
    if (self->debugFlag)
      PRINTF (" tblcf_reset_chip: error\n");
    return BDM_FAULT_RESPONSE;
  }

  self->address    = 0;
  self->cf_running = 1;
  
  return 0;
}

/*
 * Force the target into background debugging mode
 */
static int
tblcf_stop_chip (struct BDM *self)
{
  int status, retries = 4;
  
  if (self->debugFlag)
    PRINTF (" tblcf_stop_chip\n");

  while (retries--) {
    if (tblcf_target_halt (self->usbDev)) {
      if (self->debugFlag)
        PRINTF (" tblcf_stop_chip: usb error\n");
    }

    bdm_sleep (10);
    status = tblcf_get_status (self);
    
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
tblcf_run_chip (struct BDM *self)
{
  struct BDMioctl sreg_ioc;
  struct BDMioctl pc_ioc;
  int err;

  if (self->debugFlag)
    PRINTF (" tblcf_run_chip\n");

  /*
   * Flush the cache to insure all changed data is read by the
   * processor.
   */
  bdm_invalidate_cache (self);
  
  /*
   * Change the CSR:4 or the SSM bit to off then issue a go.
   */
  sreg_ioc.address = BDM_REG_CSR;
  sreg_ioc.value   = 0x00000000;
  if ((err = tblcf_write_sysreg (self, &sreg_ioc, BDM_SYS_REG_MODE_MAPPED)) < 0)
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
    if ((err = tblcf_read_sysreg (self, &sreg_ioc,
                                  BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    sreg_ioc.value &= ~0x0700;
    sreg_ioc.value |= self->cf_sr_mask_cache;
    if ((err = tblcf_write_sysreg (self, &sreg_ioc,
                                   BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    self->cf_sr_masked = 0;

    /*
     * Read the PC value and write it back out
     */
    pc_ioc.address = BDM_REG_RPC;
    if ((err = tblcf_read_sysreg (self, &pc_ioc,
                                  BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    if ((err = tblcf_write_sysreg (self, &pc_ioc,
                                   BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
  }

  /*
   * Now issue a GO command.
   */
  if (tblcf_target_go (self->usbDev)) {
    PRINTF (" tblcf_run_chip - GO cmd failed, err = %d\n", err);
    return BDM_FAULT_RESPONSE;
  }
  
  self->cf_running = 1;

  return 0;
}

/*
 * Make the target execute a single instruction and
 * reenter background debugging mode
 */
static int
tblcf_step_chip (struct BDM *self)
{
  struct BDMioctl sreg_ioc;
  struct BDMioctl csr_ioc;
  struct BDMioctl pc_ioc;
  int err;

  if (self->debugFlag)
    PRINTF (" tblcf_step_chip\n");

  /*
   * Flush the cache to insure all changed data is read by the
   * processor.
   */
  bdm_invalidate_cache (self);
  
  /*
   * Change the CSR:5 or the IPI bit to on. The SSM bit is handled
   * by the pod.
   */
  sreg_ioc.address = BDM_REG_CSR;
  sreg_ioc.value   = 1 << 5;
  if ((err = tblcf_write_sysreg (self, &sreg_ioc, BDM_SYS_REG_MODE_MAPPED)) < 0)
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
    if ((err = tblcf_read_sysreg (self, &sreg_ioc,
                                  BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    self->cf_sr_mask_cache = sreg_ioc.value & 0x0700;
    sreg_ioc.value |= 0x0700;
    if ((err = tblcf_write_sysreg (self, &sreg_ioc,
                                   BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    self->cf_sr_masked = 1;

    /*
     * Read the PC value and write it back out
     */
    pc_ioc.address = BDM_REG_RPC;
    if ((err=tblcf_read_sysreg (self, &pc_ioc,
                                BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
    if ((err=tblcf_write_sysreg (self, &pc_ioc,
                                 BDM_SYS_REG_MODE_MAPPED)) < 0)
      return err;
  }

  /*
   * Now issue a GO command.
   */
  if (tblcf_target_step (self->usbDev)) {
    PRINTF (" tblcf_step_chip - GO cmd failed\n");
    return BDM_FAULT_RESPONSE;
  }
  
  self->cf_running = 0;
  self->cf_csr     = 0x01000000;
    
  return 0;
}

/*
 * Fill I/O buffer with data from target
 */
int
tblcf_fill_buf (struct BDM *self, int count)
{
  if (self->debugFlag)
    PRINTF ("tblcf_fill_buf - count:%d\n", count);
  
  if (count == 0)
    return 0;

  /*
   * We need to swap the data if it has already been swapped.
   */
  
  if (tblcf_read_block32 (self->usbDev, self->address, count, self->ioBuffer)) {
    tblcf_gen_bus_error (self);
    if (self->debugFlag)
      PRINTF ("tblcf_fill_buf - failed\n");
    return BDM_FAULT_RESPONSE;
  }

  self->address += count;
  
  return 0;
}

/*
 * Send contents of I/O buffer to target
 */
int
tblcf_send_buf (struct BDM *self, int count)
{
  if (self->debugFlag)
    PRINTF ("tblcf_send_buf - count:%d\n", count);

  if (count == 0)
    return 0;
  
  if (tblcf_write_block32 (self->usbDev, self->address, count, self->ioBuffer)) {
    tblcf_gen_bus_error (self);
    if (self->debugFlag)
      PRINTF ("tblcf_send_buf - failed\n");
    return BDM_FAULT_RESPONSE;
  }

  self->address += count;
  
  return 0;
}

/*
 * Read processor register
 */
static int
tblcf_read_preg (struct BDM *self, struct BDMioctl *ioc)
{
  if (tblcf_read_reg (self->usbDev, ioc->address & 0xF, &ioc->value)) {
    tblcf_gen_bus_error (self);
    if (self->debugFlag)
      PRINTF ("tblcf_read_preg - reg:0x%02x, failed\n", ioc->address & 0xF);
    return BDM_FAULT_RESPONSE;
  }
  
  if (self->debugFlag)
    PRINTF ("tblcf_read_preg - reg:0x%02x = 0x%08x\n",
            ioc->address & 0xF, ioc->value);
  return 0;
}

/*
 * Read a long word from memory
 */
static int
tblcf_read_long_word (struct BDM *self, struct BDMioctl *ioc)
{
  unsigned long int l;
  
  if (tblcf_read_mem32 (self->usbDev, ioc->address, &l)) {
    tblcf_gen_bus_error (self);
    if (self->debugFlag)
      PRINTF ("tblcf_read_long_word : *0x%08x failed\n", ioc->address);
    return BDM_FAULT_RESPONSE;
  }
  
  ioc->value = l;
  
  self->address = ioc->address + 4;
  
  if (self->debugFlag)
    PRINTF ("tblcf_read_long_word : *0x%08x = 0x%08x\n", ioc->address, ioc->value);

  return 0;
}

/*
 * Read a word from memory
 */
static int
tblcf_read_word (struct BDM *self, struct BDMioctl *ioc)
{
  unsigned int w;

  if (tblcf_read_mem16 (self->usbDev, ioc->address, &w)) {
    tblcf_gen_bus_error (self);
    if (self->debugFlag)
      PRINTF ("tblcf_read_word : *0x%08x failed\n", ioc->address);
    return BDM_FAULT_RESPONSE;
  }

  ioc->value = w;
  
  self->address = ioc->address + 2;
  
  if (self->debugFlag)
    PRINTF ("tblcf_read_word : *0x%08x = 0x%04x\n", ioc->address, ioc->value);

  return 0;
}

/*
 * Read a byte from memory
 */
static int
tblcf_read_byte (struct BDM *self, struct BDMioctl *ioc)
{
  unsigned char b;

  if (tblcf_read_mem8 (self->usbDev, ioc->address, &b)) {
    tblcf_gen_bus_error (self);
    if (self->debugFlag)
      PRINTF ("tblcf_read_byte : *0x%08x failed\n", ioc->address);
    return BDM_FAULT_RESPONSE;
  }
  
  ioc->value = b;

  self->address = ioc->address + 1;

  if (self->debugFlag)
    PRINTF ("tblcf_read_byte : *0x%08x = 0x%02x\n", ioc->address, ioc->value);
  return 0;
}

/*
 * Write processor register
 */
static int
tblcf_write_preg (struct BDM *self, struct BDMioctl *ioc)
{
  if (self->debugFlag)
    PRINTF ("tblcf_write_preg - reg:%d, val:0x%08x\n",
            ioc->address & 0xF, ioc->value);

  tblcf_write_reg (self->usbDev, ioc->address & 0xF, ioc->value);

  return 0;
}

/*
 * Write a long word to memory
 */
static int
tblcf_write_long_word (struct BDM *self, struct BDMioctl *ioc)
{
  if (self->debugFlag)
    PRINTF ("tblcf_write_long_word : 0x%08x = 0x%08x\n", ioc->address, ioc->value);

  self->address = ioc->address + 4;
  
  tblcf_write_mem32 (self->usbDev, ioc->address, ioc->value);

  if (tblcf_get_last_sts (self->usbDev)) {
    tblcf_gen_bus_error (self);
    if (self->debugFlag)
      PRINTF ("tblcf_write_long_word : *0x%08x failed\n", ioc->address);
    return BDM_FAULT_RESPONSE;
  }
    
  return 0;
}

/*
 * Write a word to memory
 */
static int
tblcf_write_word (struct BDM *self, struct BDMioctl *ioc)
{
  if (self->debugFlag)
    PRINTF ("tblcf_write_word : 0x%08x = 0x%04x\n", ioc->address, (ioc->value & 0xffff));

  self->address = ioc->address + 2;

  tblcf_write_mem16 (self->usbDev, ioc->address, ioc->value);

  if (tblcf_get_last_sts (self->usbDev)) {
    tblcf_gen_bus_error (self);
    if (self->debugFlag)
      PRINTF ("tblcf_write_word : *0x%08x failed\n", ioc->address);
    return BDM_FAULT_RESPONSE;
  }

  return 0;
}

/*
 * Write a byte to memory
 */
static int
tblcf_write_byte (struct BDM *self, struct BDMioctl *ioc)
{
  if (self->debugFlag)
    PRINTF ("tblcf_write_byte : 0x%08x = 0x%02x\n", ioc->address, (ioc->value & 0xff));

  self->address = ioc->address + 1;

  tblcf_write_mem8 (self->usbDev, ioc->address, ioc->value);

  if (tblcf_get_last_sts (self->usbDev)) {
    tblcf_gen_bus_error (self);
    if (self->debugFlag)
      PRINTF ("tblcf_write_byte : *0x%08x failed\n", ioc->address);
    return BDM_FAULT_RESPONSE;
  }

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
  return 0;
}

#endif

/*
 * Initialise the BDM structure for a Coldfire in the P&E interface
 */
int
bdm_tblcf_init_self (struct BDM *self)
{
  int reg;
  
  self->processor = BDM_COLDFIRE;
  self->interface = BDM_COLDFIRE_TBLCF;

  self->get_status      = tblcf_get_status;
  self->init_hardware   = tblcf_init_hardware;
  self->serial_clock    = tblcf_serial_clock;
  self->gen_bus_error   = tblcf_gen_bus_error;
  self->restart_chip    = tblcf_restart_chip;
  self->release_chip    = tblcf_release_chip;
  self->reset_chip      = tblcf_reset_chip;
  self->stop_chip       = tblcf_stop_chip;
  self->run_chip        = tblcf_run_chip;
  self->step_chip       = tblcf_step_chip;
  self->fill_buf        = tblcf_fill_buf;
  self->send_buf        = tblcf_send_buf;
  self->read_sysreg     = tblcf_read_sysreg;
  self->read_proreg     = tblcf_read_preg;
  self->read_long_word  = tblcf_read_long_word;
  self->read_word       = tblcf_read_word;
  self->read_byte       = tblcf_read_byte;
  self->write_sysreg    = tblcf_write_sysreg;
  self->write_proreg    = tblcf_write_preg;
  self->write_long_word = tblcf_write_long_word;
  self->write_word      = tblcf_write_word;
  self->write_byte      = tblcf_write_byte;

  for (reg = 0; reg < BDM_MAX_SYSREG; reg++)
    if (reg == BDM_REG_AATR)
      self->shadow_sysreg[reg] = 0x0005;
    else
      self->shadow_sysreg[reg] = 0;

  self->cf_debug_ver = 0;
  self->cf_use_pst   = 1;
  self->cf_running   = 1;
  self->cf_csr       = 0;
  
  self->cf_sr_masked = 0;
  
  return 0;
}
