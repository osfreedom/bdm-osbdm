/*
 * Motorola Background Debug Mode Driver
 * Copyright (C) 1995  W. Eric Norum
 * Copyright (C) 1998-2007  Chris Johns
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
 * chris@users.sourceforge.net
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
 *     System Register Modes                                            *
 ************************************************************************
 */
#define BDM_SYS_REG_MODE_MAPPED   (0)
#define BDM_SYS_REG_MODE_CONTROL  (1)
#define BDM_SYS_REG_MODE_DEBUG    (2)

/*
 ************************************************************************
 *     Common Functions                                                 *
 ************************************************************************
 */
static int bdm_invalidate_cache (struct BDM *self);
static int bdm_pc_read_check (struct BDM *self);

static int bdmDrvGetStatus (struct BDM *self);
static int bdmDrvInitHardware (struct BDM *self);
static int bdmDrvRestartChip (struct BDM *self);
static int bdmDrvReleaseChip (struct BDM *self);
static int bdmDrvResetChip (struct BDM *self);
static int bdmDrvStopChip (struct BDM *self);
static int bdmDrvSerialClock (struct BDM *self, unsigned short wval, int holdback);
static int bdmDrvGenerateBusError (struct BDM *self);
static int bdmDrvStepChip (struct BDM *self);
static int bdmDrvFillBuf (struct BDM *self, int count);
static int bdmDrvSendBuf (struct BDM *self, int count);
static int bdmDrvGo (struct BDM *self);
static int bdmDrvReadSystemRegister (struct BDM *self, struct BDMioctl *ioc, int mode);
static int bdmDrvReadProcessorRegister (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvReadLongWord (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvReadWord (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvReadByte (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvWriteSystemRegister (struct BDM *self, struct BDMioctl *ioc, int mode);
static int bdmDrvWriteProcessorRegister (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvWriteLongWord (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvWriteWord (struct BDM *self, struct BDMioctl *ioc);
static int bdmDrvWriteByte (struct BDM *self, struct BDMioctl *ioc);

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
#define BDM_FORCED_TA_CMD   0x0002  /* Coldfire version D and higher */

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
 * Does this host require swapping.
 */
static int mustSwap;

/*
 * Coldfire system register mapping. See bdm.h for the user values.
 *
 * Note, this is only for the common ones.
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

#ifndef BDM_USE_PARPORT
static inline unsigned char bdm_inb_data(struct BDM *descr)
{
	return inb(descr->dataPort);
}

static inline unsigned char bdm_inb_status(struct BDM *descr)
{
	return inb(descr->statusPort);
}

static inline unsigned char bdm_inb_control(struct BDM *descr)
{
	return inb(descr->controlPort);
}

static inline void bdm_outb_data(int data, struct BDM *descr)
{
	outb(data, descr->dataPort);
}

static inline void bdm_outb_control(int data, struct BDM *descr)
{
	outb(data, descr->controlPort);
}

static inline int bdm_claim_parport(struct BDM *descr)
{
	return os_claim_io_ports (descr->name, descr->portBase, 4);
}

static inline void bdm_release_parport(struct BDM *descr)
{
	os_release_io_ports (descr->portBase, 4);
}
#else
static inline unsigned char bdm_inb_data(struct BDM *descr)
{
	return parport_read_data(descr->port);
}

static inline unsigned char bdm_inb_status(struct BDM *descr)
{
	return parport_read_status(descr->port);
}

static inline unsigned char bdm_inb_control(struct BDM *descr)
{
	return parport_read_control(descr->port);
}

static inline void bdm_outb_data(int data, struct BDM *descr)
{
	parport_write_data(descr->port, data);
}

static inline void bdm_outb_control(int data, struct BDM *descr)
{
	parport_write_control(descr->port, data);
}

static int bdm_preempt(void *handle)
{
	return 1; /* 1=don't release parport */
}

static inline int bdm_claim_parport(struct BDM *descr)
{
	descr->pardev = parport_register_device(descr->port,
				"bdm", bdm_preempt, NULL, NULL, 0, NULL);
	return parport_claim(descr->pardev);
}

static inline void bdm_release_parport(struct BDM *descr)
{
	parport_release(descr->pardev);
	parport_unregister_device(descr->pardev);
}
#endif

/*
 ************************************************************************
 *     Generic Bit Bash Functions Decls                                  *
 ************************************************************************
 */

static int bdmBitBashSendCommandTillTargetReady (struct BDM *self,
                                                 unsigned short command);
static int bdmBitBashFillBuf (struct BDM *self, int count);
static int bdmBitBashSendBuf (struct BDM *self, int count);
static int bdmBitBashFetchWord (struct BDM *self, unsigned short *sp);
static int bdmBitBashReadProcessorRegister (struct BDM *self,
                                            struct BDMioctl *ioc);
static int bdmBitBashReadLongWord (struct BDM *self, struct BDMioctl *ioc);
static int bdmBitBashReadWord (struct BDM *self, struct BDMioctl *ioc);
static int bdmBitBashReadByte (struct BDM *self, struct BDMioctl *ioc);
static int bdmBitBashWriteProcessorRegister (struct BDM *self,
                                             struct BDMioctl *ioc);
static int bdmBitBashWriteLongWord (struct BDM *self,
                                    struct BDMioctl *ioc);
static int bdmBitBashWriteWord (struct BDM *self, struct BDMioctl *ioc);
static int bdmBitBashWriteByte (struct BDM *self, struct BDMioctl *ioc);


/*
 ************************************************************************
 *     CPU32 for PD/ICD interface support routines                      *
 ************************************************************************
 */
#if defined(BDM_DEVICE_IOPERM) || !defined(BDM_USERLAND_LIB)
#include "bdm-cpu32.c"
#endif

/*
 ************************************************************************
 *     Coldfire P&E support routines                                    *
 ************************************************************************
 */
#if defined(BDM_DEVICE_IOPERM) || !defined(BDM_USERLAND_LIB)
#include "bdm-cf-pe.c"
#endif

/*
 ************************************************************************
 *     Coldfire P&E support routines                                    *
 ************************************************************************
 */
#if defined(BDM_DEVICE_USB)
#include "bdm-tblcf.c"
#endif

/*
 ************************************************************************
 *     Genric Bit Bash Functions                                        *
 ************************************************************************
 */

/*
 * Loop till target I/O operation completes
 */
static int
bdmBitBashSendCommandTillTargetReady (struct BDM *self, unsigned short command)
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
bdmBitBashFillBuf (struct BDM *self, int count)
{
  unsigned short *sp = (unsigned short *)self->ioBuffer;
  int cmd;
  int err;

  if (self->debugFlag)
    PRINTF ("bdmBitBashFillBuf - count:%d\n", count);

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
        err = bdmBitBashSendCommandTillTargetReady (self, BDM_NOP_CMD);
        if (err)
          return err;
        *(unsigned char *)sp = self->readValue;
        return 0;

      case 2:
        err = bdmBitBashSendCommandTillTargetReady (self, BDM_NOP_CMD);
        if (err)
          return err;
        *sp = self->readValue;
        return 0;

      case 3:
        err = bdmBitBashSendCommandTillTargetReady (self, BDM_DUMP_CMD | BDM_SIZE_BYTE);
        if (err)
          return err;
        *sp++ = self->readValue;
        count -= 2;
        break;

      case 4:
        err = bdmBitBashSendCommandTillTargetReady (self, BDM_NOP_CMD);
        if (err)
          return err;
        *sp++ = self->readValue;
        err = bdmDrvSerialClock (self, BDM_NOP_CMD, 0);
        if (err)
          return err;
        *sp = self->readValue;
        return 0;

      case 5:
        err = bdmBitBashSendCommandTillTargetReady (self, BDM_NOP_CMD);
        if (err)
          return err;
        *sp++ = self->readValue;
        err = bdmBitBashSendCommandTillTargetReady (self, BDM_DUMP_CMD | BDM_SIZE_BYTE);
        if (err)
          return err;
        *sp++ = self->readValue;
        count -= 4;
        break;

      case 6:
      case 7:
        err = bdmBitBashSendCommandTillTargetReady (self, BDM_NOP_CMD);
        if (err)
          return err;
        *sp++ = self->readValue;
        err = bdmBitBashSendCommandTillTargetReady (self, BDM_DUMP_CMD | BDM_SIZE_WORD);
        if (err)
          return err;
        *sp++ = self->readValue;
        count -= 4;
        break;

      default:
        err = bdmBitBashSendCommandTillTargetReady (self, BDM_NOP_CMD);
        if (err)
          return err;
        *sp++ = self->readValue;
        err = bdmBitBashSendCommandTillTargetReady (self, BDM_DUMP_CMD | BDM_SIZE_LONG);
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
bdmBitBashSendBuf (struct BDM *self, int count)
{
  unsigned short *sp = (unsigned short *)self->ioBuffer;
  int cmd;
  int err;

  if (self->debugFlag)
    PRINTF ("bdmBitBashSendBuf - count:%d\n", count);

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
    err = bdmBitBashSendCommandTillTargetReady (self, cmd);
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
int
bdmBitBashFetchWord (struct BDM *self, unsigned short *sp)
{
  int err;

  err = bdmBitBashSendCommandTillTargetReady (self, BDM_NOP_CMD);
  if (err)
    return err;
  *sp = self->readValue;
  return 0;
}

/*
 * Read processor register
 */
static int
bdmBitBashReadProcessorRegister (struct BDM *self, struct BDMioctl *ioc)
{
  int err;
  unsigned short msw, lsw;

  if (((err = bdmDrvSerialClock (self, BDM_RREG_CMD | (ioc->address & 0xF), 0)) != 0) ||
      ((err = bdmBitBashFetchWord (self, &msw)) != 0) ||
      ((err = bdmBitBashFetchWord (self, &lsw)) != 0)) {
    if (self->debugFlag)
      PRINTF ("bdmBitBashReadProcessorRegister - reg:0x%02lx, failed err=%d\n",
              ioc->address & 0xF, err);
    return err;
  }

  ioc->value = (msw << 16) | lsw;

  if (self->debugFlag)
    PRINTF ("bdmBitBashReadProcessorRegister - reg:0x%02lx = 0x%08lx\n",
            ioc->address & 0xF, ioc->value);
  return 0;
}

/*
 * Read a long word from memory
 */
static int
bdmBitBashReadLongWord (struct BDM *self, struct BDMioctl *ioc)
{
  int err;
  unsigned short msw, lsw;

  if (((err = bdmDrvSerialClock (self, BDM_READ_CMD | BDM_SIZE_LONG, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address >> 16, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address, 0)) != 0) ||
      ((err = bdmBitBashFetchWord (self, &msw)) != 0) ||
      ((err = bdmBitBashFetchWord (self, &lsw)) != 0)) {
    if (self->debugFlag)
      PRINTF ("bdmBitBashReadLongWord : *0x%08lx failed, err=%d\n", ioc->address, err);
    return err;
  }

  ioc->value = (msw << 16) | lsw;

  if (self->debugFlag)
    PRINTF ("bdmBitBashReadLongWord : *0x%08lx = 0x%08lx\n", ioc->address, ioc->value);

  return 0;
}

/*
 * Read a word from memory
 */
static int
bdmBitBashReadWord (struct BDM *self, struct BDMioctl *ioc)
{
  int            err;
  unsigned short w;

  if (((err = bdmDrvSerialClock (self, BDM_READ_CMD | BDM_SIZE_WORD, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address >> 16, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address, 0)) != 0) ||
      ((err = bdmBitBashFetchWord (self, &w)) != 0)) {
    if (self->debugFlag)
      PRINTF ("bdmBitBashReadWord : *0x%08lx failed, err=%d\n", ioc->address, err);
    return err;
  }

  ioc->value = w;

  if (self->debugFlag)
    PRINTF ("bdmBitBashReadWord : *0x%08lx = 0x%04lx\n", ioc->address, (ioc->value & 0xffff));

  return 0;
}

/*
 * Read a byte from memory
 */
static int
bdmBitBashReadByte (struct BDM *self, struct BDMioctl *ioc)
{
  int            err;
  unsigned short w;

  if (((err = bdmDrvSerialClock (self, BDM_READ_CMD | BDM_SIZE_BYTE, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address >> 16, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address, 0)) != 0) ||
      ((err = bdmBitBashFetchWord (self, &w)) != 0)){
  if (self->debugFlag)
    PRINTF ("bdmBitBashReadByte : *0x%08lx failed, err=%d\n", ioc->address, err);

  return err;
  }

  ioc->value = w;

  if (self->debugFlag)
    PRINTF ("bdmBitBashReadByte : *0x%08lx = 0x%02lx\n", ioc->address, (ioc->value & 0xff));
  return 0;
}

/*
 * Write processor register
 */
static int
bdmBitBashWriteProcessorRegister (struct BDM *self, struct BDMioctl *ioc)
{
  int err;

  if (self->debugFlag)
    PRINTF ("bdmBitBashWriteProcessorRegister - reg:%ld, val:0x%08lx\n",
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
bdmBitBashWriteLongWord (struct BDM *self, struct BDMioctl *ioc)
{
  int err;
  unsigned short w;

  if (self->debugFlag)
    PRINTF ("bdmBitBashWriteLongWord : 0x%08lx = 0x%08lx\n", ioc->address, ioc->value);

  if (((err = bdmDrvSerialClock (self, BDM_WRITE_CMD | BDM_SIZE_LONG, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address >> 16, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->value >> 16, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->value, 0)) != 0) ||
      ((err = bdmBitBashFetchWord (self, &w)) != 0))
    return err;
  return 0;
}

/*
 * Write a word to memory
 */
static int
bdmBitBashWriteWord (struct BDM *self, struct BDMioctl *ioc)
{
  int err;
  unsigned short w;

  if (self->debugFlag)
    PRINTF ("bdmBitBashWriteWord : 0x%08lx = 0x%04lx\n", ioc->address, (ioc->value & 0xffff));

  if (((err = bdmDrvSerialClock (self, BDM_WRITE_CMD | BDM_SIZE_WORD, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address >> 16, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->value, 0)) != 0) ||
      ((err = bdmBitBashFetchWord (self, &w)) != 0))
    return err;
  return 0;
}

/*
 * Write a byte to memory
 */
static int
bdmBitBashWriteByte (struct BDM *self, struct BDMioctl *ioc)
{
  int err;
  unsigned short w;

  if (self->debugFlag)
    PRINTF ("bdmBitBashWriteByte : 0x%08lx = 0x%02lx\n", ioc->address, (ioc->value & 0xff));

  if (((err = bdmDrvSerialClock (self, BDM_WRITE_CMD | BDM_SIZE_BYTE, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address >> 16, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->address, 0)) != 0) ||
      ((err = bdmDrvSerialClock (self, ioc->value, 0)) != 0) ||
      ((err = bdmBitBashFetchWord (self, &w)) != 0))
    return err;
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
 * Fill I/O buffer with data from target
 */
static int
bdmDrvFillBuf (struct BDM *self, int count)
{
  int err = (self->fill_buf) (self, count);
  if (mustSwap)
  {
    char *p = self->ioBuffer;
    int  i;
    char c;
    for (i = 0; i < (count - 1); i += 2)
    {
      c = *p;
      *p = *(p + 1);
      p++;
      *p = c;
      p++;
    }
  }
  return err;
}

/*
 * Send contents of I/O buffer to target
 */
static int
bdmDrvSendBuf (struct BDM *self, int count)
{
  if (mustSwap)
  {
    char *p = self->ioBuffer;
    int  i;
    char c;
    for (i = 0; i < (count - 1); i += 2)
    {
      c = *p;
      *p = *(p + 1);
      p++;
      *p = c;
      p++;
    }
  }
  return (self->send_buf) (self, count);
}

/*
 * Read system register
 */
static int
bdmDrvReadSystemRegister (struct BDM *self, struct BDMioctl *ioc, int mode)
{
  return (self->read_sysreg) (self, ioc, mode);
}

/*
 * Read processor register
 */
static int
bdmDrvReadProcessorRegister (struct BDM *self, struct BDMioctl *ioc)
{
  return (self->read_proreg) (self, ioc);
}

/*
 * Read a long word from memory
 */
static int
bdmDrvReadLongWord (struct BDM *self, struct BDMioctl *ioc)
{
  return (self->read_long_word) (self, ioc);
}

/*
 * Read a word from memory
 */
static int
bdmDrvReadWord (struct BDM *self, struct BDMioctl *ioc)
{
  return (self->read_word) (self, ioc);
}

/*
 * Read a byte from memory
 */
static int
bdmDrvReadByte (struct BDM *self, struct BDMioctl *ioc)
{
  return (self->read_byte) (self, ioc);
}

/*
 * Write system register
 */
static int
bdmDrvWriteSystemRegister (struct BDM *self, struct BDMioctl *ioc, int mode)
{
  return (self->write_sysreg) (self, ioc, mode);
}

/*
 * Write processor register
 */
static int
bdmDrvWriteProcessorRegister (struct BDM *self, struct BDMioctl *ioc)
{
  return (self->write_proreg) (self, ioc);
}

/*
 * Write a long word to memory
 */
static int
bdmDrvWriteLongWord (struct BDM *self, struct BDMioctl *ioc)
{
  return (self->write_long_word) (self, ioc);
}

/*
 * Write a word to memory
 */
static int
bdmDrvWriteWord (struct BDM *self, struct BDMioctl *ioc)
{
  return (self->write_word) (self, ioc);
}

/*
 * Write a byte to memory
 */
static int
bdmDrvWriteByte (struct BDM *self, struct BDMioctl *ioc)
{
  return (self->write_byte) (self, ioc);
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
  if (self->bit_bash)
    return (self->bit_bash) (self, ioc->value >> 16, ioc->value & 0xffff);
  return -1;
}

#endif

/*
 * Invalidate the cache.
 */

/*
 ************************************************************************
 *      Driver Functions which are common to different PODs             *
 ************************************************************************
 */
static int
bdm_invalidate_cache (struct BDM *self)
{
  struct BDMioctl cacr_ioc;

  if (self->debugFlag > 1)
    PRINTF (" bdm_invalidate_cache\n");

  cacr_ioc.address = BDM_REG_CACR;

  if (self->read_sysreg (self, &cacr_ioc, BDM_SYS_REG_MODE_MAPPED) < 0)
      return BDM_TARGETNC;

  /*
   * Set the invalidate bit.
   */

  if (cacr_ioc.value) {
    if (self->cf_debug_ver == CF_REVISION_D)
      cacr_ioc.value |= 0x01040100;
    else
      cacr_ioc.value |= 0x01000100;

    if (self->debugFlag > 2)
      PRINTF (" bdm_invalidate_cache -- cacr:0x%08x\n", (int) cacr_ioc.value);

    if (self->write_sysreg(self, &cacr_ioc, BDM_SYS_REG_MODE_MAPPED) < 0)
      return BDM_TARGETNC;
  }

  return 0;
}

/*
 * PC read check. This is used to see if the processor has halted.
 */

static int
bdm_pc_read_check (struct BDM *self)
{
  struct BDMioctl pc_ioc;

  pc_ioc.address = BDM_REG_RPC;

  if (self->read_sysreg (self, &pc_ioc, BDM_SYS_REG_MODE_MAPPED) < 0)
      return BDM_TARGETNC;

  return 0;
}


/*
 ************************************************************************
 *      Driver Functions which are common to the different OS's         *
 ************************************************************************
 */

#ifndef BDM_STATIC
#define BDM_STATIC static
#endif

/*
 * Requires the OS to define `os_claim_io_ports' and
 * `os_release_io_ports'. If your OS does not support this,
 * do nothing and return an error code of 0.
 */

BDM_STATIC int
bdm_open (unsigned int minor)
{
  struct BDM *self;
  int status, err = 0;

  union {
    char     c[4];
    uint32_t l;
  } un;

  if (minor >= (sizeof bdm_device_info / sizeof bdm_device_info[0]))
    return ENODEV;

  /*
   * Determine what byte-swapping we'll need to do
   */
  if (sizeof un.l != sizeof un.c) {
    PRINTF ("bdm_open -- host machine sizeof (uint32_t) != sizeof (char[4])");
    return ENODEV;
  }
  if ((sizeof (uint32_t) != 4) ||
      (sizeof (short) != 2) ||
      (sizeof (char) != 1)) {
    PRINTF ("bdm_open -- host machine sizeof not appropriate");
    return ENODEV;
  }
  un.c[0] = 0x01;
  un.c[1] = 0x02;
  un.c[2] = 0x03;
  un.c[3] = 0x04;
  if (un.l == 0x01020304) {
    mustSwap = 0;
  }
  else if (un.l == 0x04030201) {
    mustSwap = 1;
  }
  else {
    PRINTF ("bdm_open -- host machine has peculiar byte ordering");
    return ENODEV;
  }

  self = &bdm_device_info[minor];

  if (self->debugFlag > 0)
    PRINTF ("bdm_open -- minor %d\n", minor);

  if (!self->exists)
    return ENODEV;
  if (self->isOpen)
    return EBUSY;

  /*
   * Ask the OS to try and claim the port.
   */

  err = bdm_claim_parport(self);

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
    bdm_release_parport (self);
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
BDM_STATIC int
bdm_close (unsigned int minor)
{
  struct BDM *self = &bdm_device_info[minor];

  if (self->isOpen) {
    bdmDrvReleaseChip (self);
    if (self->exists && self->portsAreMine)
      bdm_release_parport (self);
    self->portsAreMine = 0;
    self->isOpen = 0;
    os_unlock_module ();
  }

  if (self->debugFlag)
    PRINTF ("BDM released\n");

  return 0;
}

BDM_STATIC int
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
    case BDM_READ_CTLREG:
    case BDM_READ_DBREG:
    case BDM_READ_SYSREG:
    case BDM_READ_LONGWORD:
    case BDM_READ_WORD:
    case BDM_READ_BYTE:
    case BDM_WRITE_REG:
    case BDM_WRITE_CTLREG:
    case BDM_WRITE_DBREG:
    case BDM_WRITE_SYSREG:
    case BDM_WRITE_LONGWORD:
    case BDM_WRITE_WORD:
    case BDM_WRITE_BYTE:
      err = os_copy_in ((void*) &ioc, (void*) arg, sizeof ioc);
      break;

    case BDM_SPEED:
    case BDM_DEBUG:
    case BDM_SET_CF_PST:
      err = os_copy_in ((void*) &iarg, (void*) arg, sizeof iarg);
      if (self->debugFlag > 3)
        PRINTF ("BDMioctl cmd->iarg:0x%x\n", iarg);
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
      if (self->debugFlag)
        PRINTF ("debugFlag now %d\n", self->debugFlag);
      break;

    case BDM_RELEASE_CHIP:
      err = bdmDrvReleaseChip (self);
      break;

    case BDM_READ_CTLREG:
      err = bdmDrvReadSystemRegister (self, &ioc, BDM_SYS_REG_MODE_CONTROL);
      break;

    case BDM_READ_DBREG:
      err = bdmDrvReadSystemRegister (self, &ioc, BDM_SYS_REG_MODE_DEBUG);
      break;

    case BDM_READ_REG:
      err = bdmDrvReadProcessorRegister (self, &ioc);
      break;

    case BDM_READ_SYSREG:
      err = bdmDrvReadSystemRegister (self, &ioc, BDM_SYS_REG_MODE_MAPPED);
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

    case BDM_WRITE_CTLREG:
      err = bdmDrvWriteSystemRegister (self, &ioc, BDM_SYS_REG_MODE_CONTROL);
      break;

    case BDM_WRITE_DBREG:
      err = bdmDrvWriteSystemRegister (self, &ioc, BDM_SYS_REG_MODE_DEBUG);
      break;

    case BDM_WRITE_SYSREG:
      err = bdmDrvWriteSystemRegister (self, &ioc, BDM_SYS_REG_MODE_MAPPED);
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

    case BDM_GET_CF_PST:
      iarg = self->cf_use_pst;
      break;

    case BDM_SET_CF_PST:
      self->cf_use_pst = iarg;
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
    case BDM_READ_CTLREG:
    case BDM_READ_DBREG:
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
    case BDM_GET_CF_PST:
      err = os_copy_out ((void*) arg, (void*) &iarg, sizeof iarg);
  }

  return err;
}

BDM_STATIC int
bdm_read (unsigned int minor, unsigned char *buf, int count)
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

BDM_STATIC int
bdm_write (unsigned int minor, unsigned char *buf, int count)
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

struct BDM*
bdm_get_device_info (int minor)
{
  if (minor < BDM_NUM_OF_MINORS)
    return &bdm_device_info[minor];
  return NULL;
}

int
bdm_get_device_info_count (void)
{
  return BDM_NUM_OF_MINORS;
}
