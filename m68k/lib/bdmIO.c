/*
 * Motorola Background Debug Mode Library
 * Copyright (C) 1995  W. Eric Norum
 * Copyright (C) 1998  Chris Johns
 *
 * Based on:
 *  1. `A Background Debug Mode Driver Package for Motorola's
 *     16-bit and 32-Bit Microcontrollers', Scott Howard, Motorola
 *     Canada, 1993.
 *  2. `Linux device driver for public domain BDM Inteface',
 *     M. Schraut, Technische Universitaet Muenchen, Lehrstuhl
 *     fuer Prozessrechner, 1995.
 *
 * 31-11-1999 Chris Johns (ccj@acm.org)
 * Extended to support remote operation. See bdmRemote.c for details.
 *
 * Extended to support the ColdFire BDM interface using the P&E module
 * which comes with the EVB. Thanks to David Fiddes who has tested the
 * driver with the 5206 (5V), 5206e (3.3V) and 5307 devices.
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
 * Objective Design Systems
 *
 * ccj@acm.org
 *
 */

/*
 * If no remote/local directive is supplied, default to the
 * existing local only mode.
 */
#if !defined (BDM_DEVICE_REMOTE) && !defined (BDM_DEVICE_LOCAL)
#define BDM_DEVICE_LOCAL
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/*
 * This test should be done by autoconf. Please fix if you
 * know how.
 */
#if !defined (__WIN32__) || defined (__CYGWIN__)
#define HAVE_SYSLOG 1
#include <syslog.h>
#endif

#include "BDMlib.h"

#if defined (BDM_DEVICE_REMOTE)

#include "bdmRemote.h"

#endif

/*
 * Provide the driver print interface so any debug IO
 * does not end up on stdout if the library is embedded
 * in a server, ie bdmd.
 */

#define PRINTF bdmDebug

/*
 * If on Cygwin assume Windows.
 */

#if defined (__WIN32__) || defined (__CYGWIN__)

#include "../driver/win/win-bdm.c"

#else

/*
 * See if this is an ioperm build.
 */
#if defined (BDM_IOPERM)

#include "../driver/ioperm/ioperm.c"

#endif

#endif

/*
 * Limit of one BDM per process
 */
static int bdm_fd    = -1;
static int mustSwap  = 0;
static int cpu       = BDM_CPU32;
static int iface     = BDM_CPU32_PD;
#if defined (BDM_DEVICE_REMOTE)
static int bdmRemote = 0;
#endif

/*
 * Have to make global to allow the bdmBFD function have
 * access.
 */
const char *bdmIO_lastErrorString = "No BDM error (yet!)";

/*
 * Debugging
 */
static int debugFlag = 0;
#if HAVE_SYSLOG
static int debug_syslog = 0;
#endif
static const char *const sysregName[BDM_MAX_SYSREG] = {
  "RPC",  "PCC",    "SR",   "USP",
  "SSP",  "SFC",    "DFC",  "ATEMP",
  "FAR",  "VBR",    "CACR", "ACR0",
  "ACR1", "RAMBAR", "MBAR", "CSR",
  "AATR", "TDR",    "PBR",  "PBMR",
  "ABHR", "ABLR",   "DBR",  "DBMR",
};

static const char *const regName[] = {
  "D0", "D1", "D2", "D3",
  "D4", "D5", "D6", "D7",
  "A0", "A1", "A2", "A3",
  "A4", "A5", "A6", "A7",
};

void
bdmLogSyslog (void)
{
#if HAVE_SYSLOG
  debug_syslog = 1;
#endif
}

void
bdmDebug (const char *format, ...)
{
  va_list ap;

  va_start (ap, format);

  if (debugFlag) {
#if HAVE_SYSLOG
    if (debug_syslog) {
      vsyslog (LOG_INFO, format, ap);
    } else
#endif
    {
      fprintf (stderr, "BDM: ");
      vfprintf (stderr, format, ap);
    }
  }

  va_end (ap);
}

void
bdmSetDebugFlag (int flag)
{
  debugFlag = flag;
}

/*
 * Verify that device is open
 */
int
bdmCheck (void)
{
  if (bdm_fd < 0) {
    bdmIO_lastErrorString = "BDM not open";
    return 0;
  }
  return 1;
}

/*
 * Like strerror(), but knows about BDM driver errors, too.
 */
static const char *
bdmStrerror (int error_no)
{
  switch (error_no) {
    case BDM_FAULT_UNKNOWN:  return "Unknown BDM error";
    case BDM_FAULT_POWER:    return "No power to BDM adapter";
    case BDM_FAULT_CABLE:    return "BDM cable problem";
    case BDM_FAULT_RESPONSE: return "No response to BDM request";
    case BDM_FAULT_RESET:    return "Target is in RESET state";
    case BDM_FAULT_BERR:     return "Target Bus Error";
    case BDM_FAULT_NVC:      return "Invalid target command";
  }
#if defined (BDM_DEVICE_REMOTE)
  return bdmRemoteStrerror (error_no);
#else
  return strerror (error_no);
#endif
}

/*
 * Do an int-argument  BDM ioctl
 */
int
bdmIoctlInt (int code, int *var)
{
  if (!bdmCheck ())
    return -1;
#if defined (BDM_DEVICE_REMOTE)
  if (bdmRemote) {
    if (bdmRemoteIoctlInt (bdm_fd, code, var) < 0) {
      bdmIO_lastErrorString = bdmStrerror (errno);
      return -1;
    }
  }
  else {
#endif
#if defined (BDM_DEVICE_LOCAL)
    if (ioctl (bdm_fd, code, var) < 0) {
      bdmIO_lastErrorString = bdmStrerror (errno);
      return -1;
    }
#endif
#if defined (BDM_DEVICE_REMOTE)
  }
#endif
  return 0;
}

/*
 * Do a command (no-argument) BDM ioctl
 */
int
bdmIoctlCommand (int code)
{
  if (!bdmCheck ())
    return -1;
#if defined (BDM_DEVICE_REMOTE)
  if (bdmRemote) {
    if (bdmRemoteIoctlCommand (bdm_fd, code) < 0) {
      bdmIO_lastErrorString = bdmStrerror (errno);
      return -1;
    }
  }
  else {
#endif
#if defined (BDM_DEVICE_LOCAL)
    if (ioctl (bdm_fd, code, NULL) < 0) {
      bdmIO_lastErrorString = bdmStrerror (errno);
      return -1;
    }
#endif
#if defined (BDM_DEVICE_REMOTE)
  }
#endif
  return 0;
}

/*
 * Do a BDMioctl-argument BDM ioctl
 */
int
bdmIoctlIo (int code, struct BDMioctl *ioc)
{
  if (!bdmCheck ())
    return -1;
#if defined (BDM_DEVICE_REMOTE)
  if (bdmRemote) {
    if (bdmRemoteIoctlIo (bdm_fd, code, ioc) < 0) {
      bdmIO_lastErrorString = bdmStrerror (errno);
      return -1;
    }
  }
  else {
#endif
#if defined (BDM_DEVICE_LOCAL)
    if (ioctl (bdm_fd, code, ioc) < 0) {
      bdmIO_lastErrorString = bdmStrerror (errno);
      return -1;
    }
#endif
#if defined (BDM_DEVICE_REMOTE)
  }
#endif
  return 0;
}

/*
 * Do a BDM read
 */
int
bdmRead (unsigned char *cbuf, unsigned long nbytes)
{
  if (!bdmCheck ())
    return -1;
#if defined (BDM_DEVICE_REMOTE)
  if (bdmRemote) {
    if (bdmRemoteRead (bdm_fd, cbuf, nbytes) != nbytes) {
      bdmIO_lastErrorString = bdmStrerror (errno);
      return -1;
    }
  }
  else {
#endif
#if defined (BDM_DEVICE_LOCAL)
    if (read (bdm_fd, cbuf, nbytes) != nbytes) {
      bdmIO_lastErrorString = bdmStrerror (errno);
      return -1;
    }
#endif
#if defined (BDM_DEVICE_REMOTE)
  }
#endif
  return nbytes;
}

/*
 * Do a BDM write
 */
int
bdmWrite (unsigned char *cbuf, unsigned long nbytes)
{
  if (!bdmCheck ())
    return -1;
#if defined (BDM_DEVICE_REMOTE)
  if (bdmRemote) {
    if (bdmRemoteWrite (bdm_fd, cbuf, nbytes) != nbytes) {
      bdmIO_lastErrorString = bdmStrerror (errno);
      return -1;
    }
  }
  else {
#endif
#if defined (BDM_DEVICE_LOCAL)
    if (write (bdm_fd, cbuf, nbytes) != nbytes) {
      bdmIO_lastErrorString = bdmStrerror (errno);
      return -1;
    }
#endif
#if defined (BDM_DEVICE_REMOTE)
  }
#endif
  return nbytes;
}

/*
 * Read from the target
 */
static int
readTarget (int command, unsigned long address, unsigned long *lp)
{
  struct BDMioctl ioc;

  ioc.address = address;
  if (bdmIoctlIo (command, &ioc) < 0)
    return -1;
  *lp = ioc.value;
  return 0;
}

/*
 * Write to the target
 */
static int
writeTarget (int command, unsigned long address, unsigned long l)
{
  struct BDMioctl ioc;

  ioc.address = address;
  ioc.value = l;
  if (bdmIoctlIo (command, &ioc) < 0)
    return -1;
  return 0;
}

static int
remoteOpen (const char *name)
{
  int fd = -1;
#if defined (BDM_DEVICE_REMOTE)
  if (bdmRemoteName (name)) {
    bdmRemote = 1;
    if ((fd = bdmRemoteOpen (name)) < 0) {
      bdmIO_lastErrorString = bdmStrerror (errno);
      bdmRemote = 0;
    }
  }
#endif
  return fd;
}

/*
 * Return string describing most recent error
 */
const char *
bdmErrorString (void)
{
  return bdmIO_lastErrorString;
}

/*
 * Open the specified BDM device
 */
int
bdmOpen (const char *name)
{
#if defined (BDM_LIB_CHECKS_VERSION)
  unsigned int ver;
#endif
  union {
    char  c[4];
    long  l;
  } un;

  /*
   * Determine what byte-swapping we'll need to do
   */
  if (sizeof un.l != sizeof un.c) {
    bdmIO_lastErrorString = "Host machine sizeof (long) != sizeof (char[4])";
    return -1;
  }
  if ((sizeof (long) != 4) ||
      (sizeof (short) != 2) ||
      (sizeof (char) != 1)) {
    bdmIO_lastErrorString = "Host machine sizeof not appropriate";
    return -1;
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
    bdmIO_lastErrorString = "Host machine has peculiar byte ordering";
    return -1;
  }

  /*
   * Open the interface. Name could be an ip:port address
   * which results in tring to open a connection to the
   * the remote server.
   *
   * First we try to open a remote connection if remote is
   * supported. If this fails we attempt to open the driver.
   */

  if (bdm_fd >= 0) {
#if defined (BDM_DEVICE_REMOTE)
    if (bdmRemote)
      bdmRemoteClose (bdm_fd);
#endif
#if defined (BDM_DEVICE_LOCAL)
    else
      close (bdm_fd);
#endif
  }

  bdm_fd = -1;

  if ((bdm_fd = remoteOpen (name)) < 0) {
#if !defined (BDM_DEVICE_LOCAL)
    bdmIO_lastErrorString = bdmStrerror (2);
    return -1;
#endif
  }

#if defined (BDM_DEVICE_LOCAL)
  if (bdm_fd < 0) {
    if ((bdm_fd = open (name, O_RDWR)) < 0) {
      bdmIO_lastErrorString = bdmStrerror (errno);
      return -1;
    }
  }
#endif

#if defined (BDM_LIB_CHECKS_VERSION)
  /*
   * CCJ: This is better done by the user as they produce a better
   *      error message.
   */
  /*
   * Check the driver version.
   */
  if (bdmGetDrvVersion (&ver) < 0) {
    bdmIO_lastErrorString = bdmStrerror (errno);
    bdmClose ();
    return -1;
  }
  if ((ver & 0xff00) != (BDM_DRV_VERSION & 0xff00)) {
    bdmIO_lastErrorString = "invalid driver version";
    bdmClose ();
    return -1;
  }
#endif

  /*
   * Get the processor and interface type
   */
  if (bdmGetProcessor (&cpu) < 0) {
    bdmIO_lastErrorString = bdmStrerror (errno);
    bdmClose ();
    return -1;
  }
  if (bdmGetInterface (&iface) < 0) {
    bdmIO_lastErrorString = bdmStrerror (errno);
    bdmClose ();
    return -1;
  }

  return bdm_fd;
}

/*
 * Close the specified BDM device
 */
int
bdmClose (void)
{
  if (!bdmCheck ())
    return -1;
#if defined (BDM_DEVICE_REMOTE)
  if (bdmRemote) {
    bdmRemote = 0;
    if (bdmRemoteClose (bdm_fd) < 0) {
      bdm_fd = -1;
      return -1;
    }
  }
  else {
#endif
#if defined (BDM_DEVICE_LOCAL)
    if (close (bdm_fd) < 0) {
      bdm_fd = -1;
      bdmIO_lastErrorString = bdmStrerror (errno);
      return -1;
    }
#endif
#if defined (BDM_DEVICE_REMOTE)
  }
#endif
  bdm_fd = -1;
  return 0;
}

/*
 * Tell if interface is open
 */
int
bdmIsOpen (void)
{
  return (bdm_fd >= 0);
}

/*
 * Return the status of the BDM interface
 */
int
bdmStatus (void)
{
  int status;

  if (bdmIoctlInt (BDM_GET_STATUS, &status) < 0)
    return -1;
  PRINTF ("Status %#x\n", status);
  return status;
}

/*
 * Set the delay time
 */
int
bdmSetDelay (int delay)
{
  if (bdmIoctlInt (BDM_SPEED, &delay) < 0)
    return -1;
  PRINTF ("Set delay %d\n", delay);
  return 0;
}

/*
 * Set the driver debug flag
 */
int
bdmSetDriverDebugFlag (int debugFlag)
{
  if (bdmIoctlInt (BDM_DEBUG, &debugFlag) < 0)
    return -1;
  PRINTF ("Set driver debug flag %d\n", debugFlag);
  return 0;
}

/*
 * Get the Coldfire PST enable state.
 */
int
bdmColdfireGetPST (int *pst)
{
  if (bdmIoctlInt (BDM_GET_CF_PST, pst) < 0)
    return -1;
  PRINTF ("Get Coldfire PST state: %d\n", *pst);
  return 0;
}

/*
 * Set the Coldfire PST enable state.
 */
int
bdmColdfireSetPST (int pst)
{
  if (bdmIoctlInt (BDM_SET_CF_PST, &pst) < 0)
    return -1;
  PRINTF ("Set Coldfire PST state: %d\n", pst);
  return 0;
}

/*
 * Read a control register
 */
int
bdmReadControlRegister (int code, unsigned long *lp)
{
  unsigned long ltmp;

  if (readTarget (BDM_READ_CTLREG, code, &ltmp) < 0)
    return -1;
  PRINTF ("Read control register 0x%04x: %#8lx\n", code, ltmp);
  *lp = ltmp;
  return 0;
}

/*
 * Read a system register
 */
int
bdmReadSystemRegister (int code, unsigned long *lp)
{
  unsigned long ltmp;

  if (readTarget (BDM_READ_SYSREG, code, &ltmp) < 0)
    return -1;
  PRINTF ("Read system register %s: %#8lx\n", sysregName[code], ltmp);
  *lp = ltmp;
  return 0;
}

/*
 * Read a register
 */
int
bdmReadRegister (int code, unsigned long *lp)
{
  unsigned long ltmp;

  code &= 0xF;
  if (readTarget (BDM_READ_REG, code, &ltmp) < 0)
    return -1;
  PRINTF ("Read register %s: %#8lx\n", regName[code], ltmp);
  *lp = ltmp;
  return 0;
}

/*
 * Write a control register
 */
int
bdmWriteControlRegister (int code, unsigned long l)
{
  if (writeTarget (BDM_WRITE_CTLREG, code, l) < 0)
    return -1;
  PRINTF ("Write control register 0x%04x: %#8lx\n", code, l);
  return 0;
}

/*
 * Write a system register
 */
int
bdmWriteSystemRegister (int code, unsigned long l)
{
  if (writeTarget (BDM_WRITE_SYSREG, code, l) < 0)
    return -1;
  PRINTF ("Write system register %s: %#8lx\n", sysregName[code], l);
  return 0;
}

/*
 * Write a register
 */
int
bdmWriteRegister (int code, unsigned long l)
{
  if (writeTarget (BDM_WRITE_REG, code, l) < 0)
    return -1;
  PRINTF ("Write register %s: %#8lx\n", regName[code], l);
  return 0;
}

/*
 * Read a long word
 */
int
bdmReadLongWord (unsigned long address, unsigned long *lp)
{
  unsigned long ltmp;

  if (readTarget (BDM_READ_LONGWORD, address, &ltmp) < 0)
    return -1;
  PRINTF ("Read %#8.8lx @ %#8lx\n", ltmp, address);
  *lp = ltmp;
  return 0;
}

/*
 * Read a word
 */
int
bdmReadWord (unsigned long address, unsigned short *sp)
{
  unsigned long ltmp;

  if (readTarget (BDM_READ_WORD, address, &ltmp) < 0)
    return -1;
  *sp = ltmp;
  PRINTF ("Read %#4.4x @ %#8lx\n", (unsigned short)ltmp, address);
  return 0;
}

/*
 * Read a byte
 */
int
bdmReadByte (unsigned long address, unsigned char *cp)
{
  unsigned long ltmp;

  if (readTarget (BDM_READ_BYTE, address, &ltmp) < 0)
    return -1;
  *cp = ltmp;
  PRINTF ("Read %#2.2x @ %#8lx\n", (unsigned char)ltmp, address);
  return 0;
}

/*
 * Write a long word
 */
int
bdmWriteLongWord (unsigned long address, unsigned long l)
{
  PRINTF ("Write %#8.8lx @ %#8lx\n", l, address);
  return writeTarget (BDM_WRITE_LONGWORD, address, l);
}

/*
 * Write a word
 */
int
bdmWriteWord (unsigned long address, unsigned short s)
{
  PRINTF ("Write %#4.4x @ %#8lx\n", s, address);
  return writeTarget (BDM_WRITE_WORD, address, s);
}

/*
 * Write a byte
 */
int
bdmWriteByte (unsigned long address, unsigned char c)
{
  PRINTF ("Write %#2.2x @ %#8lx\n", c, address);
  return writeTarget (BDM_WRITE_BYTE, address, c);
}

/*
 * Read the Module Base Address Register
 */
int
bdmReadMBAR (unsigned long *lp)
{
  return bdmReadSystemRegister (BDM_REG_MBAR, lp);
}

/*
 * Write the Module Base Address Register
 */
int
bdmWriteMBAR (unsigned long l)
{
  return bdmWriteSystemRegister (BDM_REG_MBAR, l);
}

/*
 * Reset the target and disable BDM operation
 */
int
bdmRelease (void)
{
  PRINTF ("Release\n");
  return bdmIoctlCommand (BDM_RELEASE_CHIP);
}

/*
 * Reset the target, enable BDM operation, enter BDM
 */
int
bdmReset (void)
{
  PRINTF ("Reset\n");
  return bdmIoctlCommand (BDM_RESET_CHIP);
}

/*
 * Restart the chip.
 */
int
bdmRestart (void)
{
  PRINTF ("Restart\n");
  return bdmIoctlCommand (BDM_RESTART_CHIP);
}

/*
 * Restart target execution
 */
int
bdmGo (void)
{
  PRINTF ("Go\n");
  return bdmIoctlCommand (BDM_GO);
}

/*
 * Stop the target
 */
int
bdmStop (void)
{
  PRINTF ("Stop\n");
  return bdmIoctlCommand (BDM_STOP_CHIP);
}

/*
 * Single-step the target
 */
int
bdmStep (void)
{
  PRINTF ("Step\n");
  return bdmIoctlCommand (BDM_STEP_CHIP);
}

/*
 * Read target memory
 * `cbuf' is in target byte order
 */
int
bdmReadMemory (unsigned long address, unsigned char *cbuf, unsigned long nbytes)
{
  if (nbytes == 0)
    return 0;

  if (!bdmCheck ())
    return -1;

  /*
   * Transfer first part and set up target address pointer
   */
  if (((address & 0x3) == 0) && (nbytes >= 4)) {
    unsigned long l;

    if (bdmReadLongWord (address, &l) < 0)
      return -1;
    *cbuf++ = l >> 24;
    *cbuf++ = l >> 16;
    *cbuf++ = l >> 8;
    *cbuf++ = l;
    address += 4;
    nbytes -= 4;
  }
  else if (((address & 0x1) == 0) && (nbytes >= 2)) {
    unsigned short s;

    if (bdmReadWord (address, &s) < 0)
      return -1;
    *cbuf++ = s >> 8;
    *cbuf++ = s;
    address += 2;
    nbytes -= 2;
  }
  else {
    do {
      if (bdmReadByte (address, cbuf) < 0)
        return -1;
      cbuf++;
      address++;
      nbytes--;
    } while (nbytes && (address & 0x3));
  }
  if (nbytes == 0)
    return 0;
  if (bdmRead (cbuf, nbytes) < 0)
    return -1;
  if (mustSwap) {
    unsigned int i;
    char c;
    for (i = 0 ; i < (nbytes-1) ; i += 2) {
      c = cbuf[i];
      cbuf[i] = cbuf[i+1];
      cbuf[i+1] = c;
    }
  }
  PRINTF ("Read %d byte%s\n", nbytes, nbytes == 1 ? "" : "s");
  return 0;
}

/*
 * Write target memory
 * `cbuf' is in target byte order
 */
int
bdmWriteMemory (unsigned long address, unsigned char *cbuf, unsigned long nbytes)
{
  int ret;

  if (nbytes == 0)
    return 0;

  if (!bdmCheck ())
    return -1;

  /*
   * Transfer first part and set up target address pointer
   */
  if (((address & 0x3) == 0) && (nbytes >= 4)) {
    unsigned long l;

    l = (unsigned long)*cbuf++ << 24;
    l |= (unsigned long)*cbuf++ << 16;
    l |= (unsigned long)*cbuf++ << 8;
    l |= (unsigned long)*cbuf++;
    if (bdmWriteLongWord (address, l) < 0)
      return -1;
    address += 4;
    nbytes -= 4;
  }
  else if (((address & 0x1) == 0) && (nbytes >= 2)) {
    unsigned short s;

    s = (unsigned short)*cbuf++ << 8;
    s |= (unsigned short)*cbuf++;
    if (bdmWriteWord (address, s) < 0)
      return -1;
    address += 2;
    nbytes -= 2;
  }
  else {
    do {
      if (bdmWriteByte (address, *cbuf) < 0)
        return -1;
      cbuf++;
      address++;
      nbytes--;
    } while (nbytes && (address & 0x3));
  }
  if (nbytes == 0)
    return 0;
  if (mustSwap) {
    unsigned int i;
    char c;
    for (i = 0 ; i < (nbytes-1) ; i += 2) {
      c = cbuf[i];
      cbuf[i] = cbuf[i+1];
      cbuf[i+1] = c;
    }
  }
  ret = bdmWrite (cbuf, nbytes);
  if (mustSwap) {
    unsigned int i;
    char c;
    for (i = 0 ; i < (nbytes-1) ; i += 2) {
      c = cbuf[i];
      cbuf[i] = cbuf[i+1];
      cbuf[i+1] = c;
    }
  }
  if (ret < 0)
    return -1;
  PRINTF ("Wrote %d byte%s\n", nbytes, nbytes == 1 ? "" : "s");
  return 0;
}

/*
 * Get Driver version
 */
int
bdmGetDrvVersion (unsigned int *ver)
{
  if (bdmIoctlInt (BDM_GET_DRV_VER, ver) < 0)
    return -1;
  PRINTF ("Driver version: %x.%x\n", *ver >> 8, *ver & 0xff);
  return 0;
}

/*
 * Get Processor type
 */
int
bdmGetProcessor (int *processor)
{
  if (bdmIoctlInt (BDM_GET_CPU_TYPE, processor) < 0)
    return -1;
  PRINTF ("CPU type: %d\n", *processor);
  return 0;
}

/*
 * Get Interface type
 */
int
bdmGetInterface (int *iface)
{
  if (bdmIoctlInt (BDM_GET_IF_TYPE, iface) < 0)
    return -1;
  PRINTF ("Interface type: %d\n", *iface);
  return 0;
}
