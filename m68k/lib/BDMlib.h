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
 * All (int) routines return -1 on error, >=0 otherwise.
 */
int  bdmOpen (const char *name);
int  bdmClose (void);
int  bdmIsOpen (void);
int  bdmSetDelay (int delay);
int  bdmSetDriverDebugFlag (int flag);
void bdmSetDebugFlag (int flag);

int         bdmStatus (void);
const char *bdmErrorString (void);

/*
 * The following routines return, or are passed, data with byte
 * order (big-endian or little-endian) of the machine on which
 * the routines are running.
 */
int bdmReadSystemRegister (int code, unsigned long *lp);
int bdmReadRegister (int code, unsigned long *lp);
int bdmReadMBAR (unsigned long *lp);

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

int bdmLoadExecutable (const char *name);

/*
 * The following routines are low-level and are used to implement a 
 * client/interface.
 */
int bdmIoctlInt (int code, int *var);
int bdmIoctlCommand (int code);
int bdmIoctlIo (int code, struct BDMioctl *ioc);
int bdmRead (unsigned char *cbuf, unsigned long nbytes);
int bdmWrite (unsigned char *cbuf, unsigned long nbytes);

#if __cplusplus
}
#endif

#endif
