#ifndef LINUX_BDM_H
#define LINUX_BDM_H
/*
 * $Id: bdm.h,v 1.1 2003/06/04 01:31:32 ppisa Exp $
 *
 * Linux Device Driver for Public Domain BDM Interface
 * based on the PD driver package by Scott Howard, Feb 93
 * ported to Linux by M.Schraut
 * tested for kernel version 1.2.4
 * (C) 1995 Technische Universitaet Muenchen, Lehrstuhl fuer Prozessrechner

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 */

#define BDM_MAJOR_NUMBER	53

/* error codes */
#define BDM_FAULT_UNKNOWN  -610   /*Error-definitions*/
#define BDM_FAULT_POWER    -611 
#define BDM_FAULT_CABLE    -612
#define BDM_FAULT_RESPONSE -613   /*NOT Ready */
#define BDM_FAULT_RESET    -614
#define BDM_FAULT_PORT     -615
#define BDM_FAULT_BERR     -616
#define BDM_FAULT_NVC      -617   /*no valid command */

/* Debug Levels  */
#define BDM_DEBUG_NONE     0
#define BDM_DEBUG_SOME     1
#define BDM_DEBUG_ALL      2

/* supported ioctls */
#define BDM_INIT           0		/* no argument */
#define BDM_DEINIT         1		/* no argument */
#define BDM_RESET_CHIP     2		/* no argument */
#define BDM_RESTART_CHIP   3		/* no argument */
#define BDM_STOP_CHIP      4		/* no argument */
#define BDM_STEP_CHIP      5		/* no argument */
#define BDM_GET_STATUS     6		/* no argument */
#define BDM_SPEED          7		/* arg = speed */
#define	BDM_RELEASE_CHIP   8		/* no argument */
#define BDM_DEBUG_LEVEL    9		/* arg = level */
#define BDM_GET_VERSION   10		/* arg = &int  */
#define	BDM_SENSECABLE    11		/* arg =on/off */

#define BDM_NORETURN		0		/* no error, no ret value */
/* functional bits of ioctl BDM_GET_STATUS */
#define BDM_TARGETRESET 	(1<<0)	/* Target reset                      */
#define BDM_TARGETSTOPPED 	(1<<2)	/* Target (was already) stopped      */
#define BDM_TARGETPOWER 	(1<<3)	/* Power failed                      */
#define BDM_TARGETNC 		(1<<4)	/* Target not Connected              */

/* command codes for bdm interface */
#define	BDM_RREG_CMD		0x2180
#define	BDM_WREG_CMD		0x2080
#define	BDM_RSREG_CMD		0x2580
#define	BDM_WSREG_CMD		0x2480
#define	BDM_READ_CMD		0x1900
#define	BDM_WRITE_CMD		0x1800
#define	BDM_DUMP_CMD		0x1d00
#define	BDM_FILL_CMD		0x1c00
#define	BDM_GO_CMD			0x0c00
#define	BDM_CALL_CMD		0x0800
#define	BDM_RST_CMD			0x0400
#define	BDM_NOP_CMD			0x0000

/* system register for RSREG/WSREG */
#define	BDM_REG_RPC			0x0
#define	BDM_REG_PCC			0x1
#define	BDM_REG_SR			0xb
#define	BDM_REG_USP			0xc
#define	BDM_REG_SSP			0xd
#define	BDM_REG_SFC			0xe
#define	BDM_REG_DFC			0xf
#define	BDM_REG_ATEMP		0x8
#define	BDM_REG_FAR			0x9
#define	BDM_REG_VBR			0xa

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

#endif
