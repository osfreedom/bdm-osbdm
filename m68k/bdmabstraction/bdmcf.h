#ifndef LINUX_BDM_H
#define LINUX_BDM_H

#include <linux/ioctl.h>
/*
 * $Id: bdmcf.h,v 1.1 2003/12/29 22:18:49 codewiz Exp $
 *
 * Linux Device Driver for P&E Microcomputer Systems Coldfire Cable
 * (c) 1997 Rolf Fiedler
 * 
 * this header file is used to interface to a kernel driver and to 
 * a user-mode lpt driver (using #ifdef USER_MODE)
 * therefore it contains prototypes of usermode functions
 * 
 * based on code from and using (roughly) the same API as the
 * 
 * Linux Device Driver for Public Domain BDM Interface
 * based on the PD driver package by Scott Howard, Feb 93
 * ported to Linux by M.Schraut
 * tested for kernel version 1.2.4
 * (C) 1995 Technische Universitaet Muenchen, Lehrstuhl fuer Prozessrechner
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* the interface of the kernel mode driver as of June 10th, 1997
 * 
 * there are a number of ioctl's for all non-speed-critial bdm operations
 * these operations allow to reset, restart, stop, step and run the chip,
 * to exchange data with the bdm interface, and to set the clocking speed
 * and debug-level.
 * 
 * the download of code and the reading back of memory are considered
 * speed critical, due to the fact that download times should be minimized
 * therefore these two operations are supported by the BDM fill and dump
 * commands. these commands are issued automatically by read and write
 * system calls on the bdm driver
 */

#define BDM_MAJOR_NUMBER	30	

/* error codes */
#define BDM_FAULT_UNKNOWN  -610   /*Error-definitions*/
#define BDM_FAULT_POWER    -611   /* target has no power */
#define BDM_FAULT_CABLE    -612   /* target is not connected */
#define BDM_FAULT_RESPONSE -613   /* NOT Ready */
#define BDM_FAULT_RESET    -614   /* target is held in reset */
#define BDM_FAULT_PORT     -615   /* can not gain permission to access port */
#define BDM_FAULT_BERR     -616   /* access led to a bus error */
#define BDM_FAULT_NVC      -617   /* no valid command */

/* Debug Levels  */
#define BDM_DEBUG_NONE     0
#define BDM_DEBUG_SOME     1
#define BDM_DEBUG_ALL      2

/* supported ioctls */
#define BDM_IOCTL_TYPE      0xaa
enum BDM_IOCTLS {
        BDM_INIT,            /* no argument */
	BDM_DEINIT,          /* no argument */
	BDM_RESET_CHIP,      /* no argument */
	BDM_RESTART_CHIP,    /* no argument */
	BDM_STOP_CHIP,	     /* no argument */
	BDM_STEP_CHIP,	     /* no argument */
	BDM_RUN_CHIP,	     /* no argument */
	BDM_GET_STATUS,	     /* no argument */
	BDM_XCHG_DATA,       /* argument - int[], rw variable size */
	BDM_DEBUG_LEVEL      /* arg = level */
};

#define BDM_INIT_IOC            _IO(BDM_IOCTL_TYPE,BDM_INIT)
#define BDM_DEINIT_IOC          _IO(BDM_IOCTL_TYPE,BDM_DEINIT)
#define BDM_RESET_CHIP_IOC      _IO(BDM_IOCTL_TYPE,BDM_RESET_CHIP)
#define BDM_RESTART_CHIP_IOC    _IO(BDM_IOCTL_TYPE,BDM_RESTART_CHIP)
#define BDM_STOP_CHIP_IOC       _IO(BDM_IOCTL_TYPE,BDM_STOP_CHIP)
#define BDM_STEP_CHIP_IOC       _IO(BDM_IOCTL_TYPE,BDM_STEP_CHIP)
#define	BDM_RUN_CHIP_IOC        _IO(BDM_IOCTL_TYPE,BDM_RUN_CHIP)
#define BDM_GET_STATUS_IOC      _IO(BDM_IOCTL_TYPE,BDM_GET_STATUS)
#define BDM_XCHG_DATA_IOC(size) _IOWR(BDM_IOCTL_TYPE,BDM_XCHG_DATA,size)
#define BDM_DEBUG_LEVEL_IOC     _IO(BDM_IOCTL_TYPE,BDM_DEBUG_LEVEL)

/* return codes of get status (or'ed) */
#define BDM_NORETURN		0	/* no error, no ret value */
/* functional bits of ioctl BDM_GET_STATUS */
#define BDM_TARGETRESET 	(1<<0)	/* Target reset                      */
#define BDM_TARGETHALT 		(1<<1)	/* Target halt                       */
#define BDM_TARGETSTOPPED 	(1<<2)	/* Target (was already) stopped      */
#define BDM_TARGETPOWER 	(1<<3)	/* Power failed                      */
#define BDM_TARGETNC 		(1<<4)	/* Target not Connected              */
#define BDM_FROZEN		(1<<5)  /* Target (was running before)stopped*/

/* it's determined by hardware from now on */
/* command codes for bdm interface */
#define	BDM_RREG_CMD		0x2180
#define	BDM_WREG_CMD		0x2080
#define	BDM_READ_CMD		0x1900
#define	BDM_WRITE_CMD		0x1800
#define	BDM_DUMP_CMD		0x1d00
#define	BDM_FILL_CMD		0x1c00
#define	BDM_GO_CMD		0x0c00
#define	BDM_NOP_CMD		0x0000
/* coldfire doesn't support BDM_RSREG_CMD/BDM_WSREG_CMD */
/* coldfire doesn't support BDM_CALL_CMD/BDM_RTS_CMD */
/* added for coldfire */
#define BDM_RCREG_CMD           0x2980
#define BDM_WCREG_CMD           0x2880
#define BDM_RDMREG_CMD          0x2d80
#define BDM_WDMREG_CMD          0x2c80

/* RCREG/WCREG */
#define BDM_REG_CACR            0x002
#define BDM_REG_ACR0            0x004
#define BDM_REG_ACR1            0x005
#define BDM_REG_VBR             0x801
#define BDM_REG_SR              0x80e
#define BDM_REG_RPC             0x80f
#define BDM_REG_RAMBAR          0xc04
#define BDM_REG_MBAR            0xc0f

/* RDMREG/WDMREG */
#define BDM_REG_CSR    0x0   /* Configuration/Status */
#define BDM_REG_RSRVD1 0x1 
#define BDM_REG_RSRVD2 0x2
#define BDM_REG_RSRVD3 0x3
#define BDM_REG_RSRVD4 0x4
#define BDM_REG_RSRVD5 0x5
#define BDM_REG_AATR   0x6   /* Address ATtribute breakpoint Register */
#define BDM_REG_TDR    0x7   /* Trigger Definition Register */
#define BDM_REG_PBR    0x8   /* Pc Breakpoint Register */
#define BDM_REG_PBMR   0x9   /* Pc Breakpoint Mask Register */
#define BDM_REG_RSRVD6 0xa
#define BDM_REG_RSRVD7 0xb
#define BDM_REG_ABHR   0xc   /* Address Breakpoint H Register */
#define BDM_REG_ABLR   0xd   /* Address Breakpoint L Register */
#define BDM_REG_DBR    0xe   /* Data Breakpoint Register */
#define BDM_REG_DBMR   0xf   /* Data Breakpoint Mask Register */

/* system register for RREG/WREG */
#define	BDM_REG_D0			0x0
#define	BDM_REG_D1			0x1
#define	BDM_REG_D2			0x2
#define	BDM_REG_D3			0x3
#define	BDM_REG_D4			0x4
#define	BDM_REG_D5			0x5
#define	BDM_REG_D6			0x6
#define	BDM_REG_D7			0x7
#define	BDM_REG_A0			0x8
#define	BDM_REG_A1			0x9
#define	BDM_REG_A2			0xa
#define	BDM_REG_A3			0xb
#define	BDM_REG_A4			0xc
#define	BDM_REG_A5			0xd
#define	BDM_REG_A6			0xe
#define	BDM_REG_A7			0xf

/* op size for READ/WRITE */
#define	BDM_SIZE_BYTE		0x0000
#define	BDM_SIZE_WORD		0x0040
#define	BDM_SIZE_LONG		0x0080

/* coldfire has trace capability */
/* processor status while trace */
#define BDM_PST_CONTINUE    0x0
#define BDM_PST_BEGIN       0x1
#define BDM_PST_RESERVED    0x2
#define BDM_PST_USERMODE    0x3
#define BDM_PST_PULSE       0x4
#define BDM_PST_BRANCH      0x5
#define BDM_PST_RESERVED2   0x6
#define BDM_PST_RTE         0x7
#define BDM_PST_DDATA1      0x8
#define BDM_PST_DDATA2      0x9
#define BDM_PST_DDATA3      0xa
#define BDM_PST_DDATA4      0xb
#define BDM_PST_EXCEPTION   0xc
#define BDM_PST_EMULATOR    0xd
#define BDM_PST_STOPPED     0xe
#define BDM_PST_HALTED      0xf

/* responses from chip */
/* bdm messages */
#define BDM_VALID       0x00000 /* not a message, but data transfer */
#define BDM_COMPLETE    0x0ffff
#define BDM_NOTREADY    0x10000
#define BDM_BERR        0x10001
#define BDM_ILLEGAL     0x1ffff

/* debug module register definitions */
/* AATR */
#define BDM_AATR_RM      (1<<15)    /* Read/Write mask */
#define BDM_AATR_SZM     (3<<13)    /* SiZe Mask */
#define BDM_AATR_TTM     (3<<11)    /* Transfer Type Mask */
#define BDM_AATR_TMM     (7<<8)     /* Transfer Modifier Mask */
#define BDM_AATR_R       (1<<7)     /* Read/write */
#define BDM_AATR_SZ      (3<<5)     /* SiZe */
#define BDM_AATR_TT      (3<<3)     /* Tranfer Type */
#define BDM_AATR_TM      (7<<0)     /* Transfer Modifier */
/* TDR */
#define BDM_TDR_TRC      (3<<30)    /* Trigger Response Control */
#define BDM_TDR_TRC_DATA (0<<30)    /* Trigger Response Control */
#define BDM_TDR_TRC_HALT (1<<30)    /* Trigger Response Control */
#define BDM_TDR_TRC_DEBG (2<<30)    /* Trigger Response Control */
#define BDM_TDR_EBL2     (1<<29)    /* Enable Breakpoint Level */
#define BDM_TDR_EDLW2    (1<<28)    /* Enable Data BP for LongWord */
#define BDM_TDR_EDWL2    (1<<27)    /* Enable Data BP for Word (Upper) */
#define BDM_TDR_EDWU2    (1<<26)    /* Enable Data BP for Word (Lower) */
#define BDM_TDR_EDLL2    (1<<25)    /* Enable Data BP for byte (LL) */
#define BDM_TDR_EDLM2    (1<<24)    /* Enable Data BP for byte (LM) */
#define BDM_TDR_EDUM2    (1<<23)    /* Enable Data BP for byte (UM) */
#define BDM_TDR_EDUU2    (1<<22)    /* Enable Data BP for byte (UU) */
#define BDM_TDR_DI2      (1<<21)    /* Data breakpoint invert */
#define BDM_TDR_EAI2     (1<<20)    /* Enable Address breakpoint inverted */
#define BDM_TDR_EAR2     (1<<19)    /* Enable Address breakpoint Range */
#define BDM_TDR_EAL2     (1<<18)    /* Enable Address breakpoint Low */
#define BDM_TDR_EPC2     (1<<17)    /* Enable PC breakpoint */
#define BDM_TDR_PCI2     (1<<16)    /* PC breakpoint Invert */
#define BDM_TDR_EBL1     (1<<13)    /* Enable Breakpoint Level */
#define BDM_TDR_EDLW1    (1<<12)    /* Enable Data BP for LongWord */
#define BDM_TDR_EDWL1    (1<<11)    /* Enable Data BP for Word (Upper) */
#define BDM_TDR_EDWU1    (1<<10)    /* Enable Data BP for Word (Lower) */
#define BDM_TDR_EDLL1    (1<<9)     /* Enable Data BP for byte (LL) */
#define BDM_TDR_EDLM1    (1<<8)     /* Enable Data BP for byte (LM) */
#define BDM_TDR_EDUM1    (1<<7)     /* Enable Data BP for byte (UM) */
#define BDM_TDR_EDUU1    (1<<6)     /* Enable Data BP for byte (UU) */
#define BDM_TDR_DI1      (1<<5)     /* Data breakpoint invert */
#define BDM_TDR_EAI1     (1<<4)     /* Enable Address breakpoint inverted */
#define BDM_TDR_EAR1     (1<<3)     /* Enable Address breakpoint Range */
#define BDM_TDR_EAL1     (1<<2)     /* Enable Address breakpoint Low */
#define BDM_TDR_EPC1     (1<<1)     /* Enable PC breakpoint */
#define BDM_TDR_PCI1     (1<<0)     /* PC breakpoint Invert */
/* CSR */
#define BDM_CSR_STATUS   (15<<28)   /* breakpoint status */
#       define BDM_CSR_STAT_NOBP    0
#       define BDM_CSR_STAT_WAIT1   1
#       define BDM_CSR_STAT_TRIGG1  2
#       define BDM_CSR_STAT_WAIT2   5
#       define BDM_CSR_STAT_TRIGG2  6
#define BDM_CSR_FOF      (1<<27)    /* Fault-On-Fault */
#define BDM_CSR_TRG      (1<<26)    /* hardware breakpoint TRiGger */
#define BDM_CSR_HALT     (1<<25)    /* processor HALT */
#define BDM_CSR_BKPT     (1<<24)    /* BreaKPoinT assert */
#define BDM_CSR_IPW      (1<<16)    /* Inhibit Processor Writes to dbg reg. */
#define BDM_CSR_MAP      (1<<15)    /* MAP processor accesses in emu mode */
#define BDM_CSR_TRC      (1<<14)    /* emulation mode on TRaCe exception */
#define BDM_CSR_EMU      (1<<13)    /* force EMUlation mode */
#define BDM_CSR_DDC      (3<<11)    /* Debug Data Control */
#define BDM_CSR_UHE      (1<<10)    /* User Halt Enable */
#define BDM_CSR_BTB      (3<<8)     /* Branch Target Bytes */
#define BDM_CSR_NPL      (1<<6)     /* NonPipelined Mode */
#define BDM_CSR_IPI      (1<<5)     /* Ignore Pending Interrupts */
#define BDM_CSR_SSM      (1<<4)     /* Single Step Mode */

#ifdef USERMODE
int bdm_open(int minor, int flags);
int bdm_ioctl(int minor, unsigned int request, unsigned long arg);
int bdm_read(int minor, unsigned char *p, int count);
int bdm_write(int minor, const unsigned char *p, int count);
void bdm_close(int minor);
#endif

#endif
