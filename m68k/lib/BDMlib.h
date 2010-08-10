/*
 * Motorola Background Debug Mode Remote Library
 * Copyright (C) 1998  Chris Johns
 *
 * Chris Johns
 * Cybertec Pty Ltd
 *
 * cjohns@users.sourceforge.net
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
/*
 * Routines to control a CPU32(+) or Coldfire target
 */

#ifndef _BDM_LIB_H_
#define _BDM_LIB_H_

#if __cplusplus
extern "C"
{
#endif

#include <bdm.h>

/*
 * Configuration file name.
 */
#define M68K_BDM_INIT_FILE ".m68kbdminit"

/*
 * All (int) routines return -1 on error, >=0 otherwise.
 */
void bdmReadConfiguration();
int  bdmCheck (void);
int  bdmOpen (const char *name);
int  bdmClose (void);
int  bdmIsOpen (void);
int  bdmSetDelay (int delay);
int  bdmSetDriverDebugFlag (int flag);
void bdmSetDebugFlag (int flag);
void bdmLogSyslog (void);

int         bdmStatus (void);
const char *bdmErrorString (void);

/*
 * Return a value in the host byte order. Useful for printing on
 * the host.
 */
unsigned long bdmHostByteOrder (unsigned long value);

/*
 * The following routines return, or are passed, data with byte
 * order (big-endian or little-endian) of the machine on which
 * the routines are running.
 *
 * Note, the control registers are the values documented in the
 *       devices data sheet.
 */
int bdmReadControlRegister (int code, unsigned long *lp);
int bdmReadDebugRegister (int code, unsigned long *lp);
int bdmReadSystemRegister (int code, unsigned long *lp);
int bdmReadRegister (int code, unsigned long *lp);
int bdmReadMBAR (unsigned long *lp);

int bdmWriteControlRegister (int code, unsigned long l);
int bdmWriteDebugRegister (int code, unsigned long l);
int bdmWriteSystemRegister (int code, unsigned long l);
int bdmWriteRegister (int code, unsigned long l);
int bdmWriteMBAR (unsigned long l);

int bdmReadLongWord (unsigned long address, unsigned long *lp);
int bdmReadWord (unsigned long address, unsigned short *sp);
int bdmReadByte (unsigned long address, unsigned char *cp);

int bdmWriteLongWord (unsigned long address, unsigned long l);
int bdmWriteWord (unsigned long address, unsigned short s);
int bdmWriteByte (unsigned long address, unsigned char c);

/*
 * The following routines report the driver version, processor and
 * interface type
 */
int bdmGetDrvVersion (unsigned int *ver);
int bdmGetProcessor (int *processor);
int bdmGetInterface (int *iface);

/*
 * Get and set the PST signal state. 1 = enabled, 0 = disabled.
 */

int bdmColdfireGetPST (int *pst);
int bdmColdfireSetPST (int pst);

/*
 * The following routines control execution of the target machine
 */
int bdmReset (void);
int bdmRestart (void);
int bdmRelease (void);
int bdmGo (void);
int bdmStop (void);
int bdmStep (void);

/*
 * In the bdmReadMemory and bdmWriteMemory routines, cbuf is the address
 * of a buffer whose contents are in M68k (i.e. big-endian) byte order.
 */
int bdmReadMemory (unsigned long address, unsigned char *cbuf, unsigned long nbytes);
int bdmWriteMemory (unsigned long address, unsigned char *cbuf, unsigned long nbytes);

/*
 * The following routines are low-level and are used to implement a
 * client/interface.
 */
int bdmIoctlInt (int code, int *var);
int bdmIoctlCommand (int code);
int bdmIoctlIo (int code, struct BDMioctl *ioc);
int bdmRead (unsigned char *cbuf, unsigned long nbytes);
int bdmWrite (unsigned char *cbuf, unsigned long nbytes);

/*
 * BDM Configuration file support.
 */
void bdmReadConfiguration ();
const char* bdmConfigSkipWhiteSpace (const char* text);
const char* bdmConfigGet (const char* label, const char* last);

/*
 * BDM debug channel. Applications may use this to have a common
 * debug trace environment.
 */
void bdmInfo (const char *format, ...);
void bdmPrint (const char *format, ...);

#if __cplusplus
}
#endif

#endif
