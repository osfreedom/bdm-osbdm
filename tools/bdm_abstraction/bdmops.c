/* @#Copyright 
 * Copyright (c) 1997, Rolf Fiedler.
 * Copyright (c) 1999-2000, Brett Wuth.
 */
/* @#License: 
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

/* File:	bdmops.c (BDM Operations)
 * Purpose:	
 * Author:	Rolf Fiedler
 * Created:	
 *
 * Initials:
 *	BCW - Brett Wuth
 *		@#[ContactWuth:
 *		Phone: +1 403 627-2460
 *		E-mail: support@castrov.cuug.ab.ca, wuth@acm.org]
 *
 * this is an abstraction layer to have a general target programming interface
 * so the programmer doesn't have to bother with motorola's bdm commands.
 * 
 * applications only include bdmops.h and call the functions below
 *
 * HISTORY:
 * $Log: bdmops.c,v $
 * Revision 1.1  2003/06/03 15:42:04  codewiz
 * Import userland tools from bdm-fiedler
 *
 * Revision 1.6  2000/09/19 00:28:29  wuth
 * cleanly use Fiedler's bdm driver; bdm_mon detects flash errors
 *
 * Revision 1.5  2000/07/25 13:51:09  wuth
 * Working sector erase.  Better error reports.
 *
 * Revision 1.4  2000/04/20 04:56:23  wuth
 * GPL.  Abstract flash interface.
 *
 * Revision 1.3  2000/03/28 20:24:41  wuth
 * Break out flash code into separate executable.  Make run under Chris Johns BDM driver.
 *
 * Revision 1.2  1999/07/05 22:09:50  wuth
 * Abort if can't sync BDM.  Work with Am29F800 flash.
 * @#[BasedOnTemplate: template.c version 2]
 */

#define  _COMPILING_
# include "bdmops.h"
#undef   _COMPILING_

#include <assert.h>
#include <BDMDriver.h>
#include <Debug.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * query status
 */
static unsigned long target_status=0;

#if BDMDriverVersion_M == BDMDriverFiedler_M
static int fd;

static void bdm_update_status(unsigned long x)
{
    x &= CSR_FOF | CSR_TRG | CSR_HALT | CSR_BKPT; /* these bits ain't sticky */
    target_status |= x;
}

static int bdm_read_status(void)
{
    unsigned long x;
    
    if(bdm_read_reg(REG_CSR, &x)) return -1;
    bdm_update_status(x);
    return 0;
}
#endif



/*
 * general file handling
 */
/* open bdm-driver */
int /* <0 if error, BDMHandle otherwise */
bdm_init(const char *path)
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    unsigned long tdr, csr;
    
#ifdef USERMODE
    fd=path[strlen(path)-1]-'0'; /* this is so ugly! works with /dev/bdm[0-9]*/
    if(fd>3 || fd<0) return -1;
    fd=bdm_open(fd, O_RDWR);
    if(fd<0) return fd;
#else
    fd=bdm_open(path, O_RDWR);
    if(fd<0) return fd;
#endif    
    bdm_clear_status();
    tdr = BDM_TDR_TRC_HALT; /* trigger response is halt */
    if(bdm_write_reg(REG_TDR, &tdr)) return -1;
    csr = BDM_CSR_UHE;  /* user halt enable */
    if(bdm_write_reg(REG_CSR, &csr)) return -1;
    return fd;
#else
    int fd=bdmOpen( path );
    return (fd);
#endif
}

/* close driver */
void bdm_release(int port)
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    bdm_close(fd);
#else
    bdmClose();
#endif
}

/* 
 * bdm control functions
 */
/* bring chip to running state */
void bdm_run(void)
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    bdm_ioctl(fd, BDM_RUN_CHIP_IOC, 0);
    bdm_clear_status();
#else
    /* not implemented */
#endif
}
	
/* bring chip to stopped state */
void bdm_stop(void)    
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    bdm_ioctl(fd, BDM_STOP_CHIP_IOC, 0);
    bdm_read_status();
#else
    /* not implemented */
#endif
}
    
/* step chip on instruction */
void bdm_step(void)    
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    bdm_ioctl(fd, BDM_STEP_CHIP_IOC, 0);
    bdm_read_status();
#else
    /* not implemented */
#endif
}
    
/* reset chip and hold in reset state */
void bdm_reset(void)    
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    bdm_ioctl(fd, BDM_RESET_CHIP_IOC, 0);
    bdm_clear_status();
    bdm_read_status();
#else
    /* not implemented */
#endif
}
    
/* restart from reset to running state */
void bdm_restart(void)    
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    bdm_ioctl(fd, BDM_RESTART_CHIP_IOC, 0);
    bdm_read_status();
#else
    /* not implemented */
#endif
}

/*
 * data transfer to/from target
 */
int regs_log2phys(CF_REGS which)
{
    int i;
    struct regs {
	int logical;
	int physical;
    } regs[]= {
	{REG_D0,     BDM_REG_D0},
	{REG_D1,     BDM_REG_D1},
	{REG_D2,     BDM_REG_D2},
	{REG_D3,     BDM_REG_D3},
	{REG_D4,     BDM_REG_D4},
	{REG_D5,     BDM_REG_D5},
	{REG_D6,     BDM_REG_D6},
	{REG_D7,     BDM_REG_D7},
	{REG_A0,     BDM_REG_A0},
	{REG_A1,     BDM_REG_A1},
	{REG_A2,     BDM_REG_A2},
	{REG_A3,     BDM_REG_A3},
	{REG_A4,     BDM_REG_A4},
	{REG_A5,     BDM_REG_A5},
	{REG_A6,     BDM_REG_A6},
	{REG_A7,     BDM_REG_A7},
	/* debug registers */
	{REG_CSR,    BDM_REG_CSR},
	{REG_AATR,   BDM_REG_AATR},
	{REG_TDR,    BDM_REG_TDR},
	{REG_PBR,    BDM_REG_PBR},
	{REG_PBMR,   BDM_REG_PBMR},
	{REG_ABHR,   BDM_REG_ABHR},
	{REG_ABLR,   BDM_REG_ABLR},
	{REG_DBR,    BDM_REG_DBR},
	{REG_DBMR,   BDM_REG_DBMR},
	/* configuration registers */
	{REG_CACR,   BDM_REG_CACR},
	{REG_ACR0,   BDM_REG_ACR0},
	{REG_ACR1,   BDM_REG_ACR1},
	{REG_VBR,    BDM_REG_VBR},
	{REG_SR,     BDM_REG_SR},
	{REG_RPC,    BDM_REG_RPC},
	{REG_RAMBAR, BDM_REG_RAMBAR},
	{REG_MBAR,   BDM_REG_MBAR},
	{0xffff,0}
    };
    for(i=0; regs[i].logical!=0xffff; i++) {
	if(regs[i].logical == which) return regs[i].physical;
    }
    return -1;  
}


/* read a register from the chip */
int bdm_read_reg(CF_REGS which, unsigned long *value)
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    int i;
    
    if(which <= MAX_USER_REGS) {
	unsigned long word[3];
	which=regs_log2phys(which);
	word[0]=BDM_RREG_CMD | which;
	word[1]=BDM_NOP_CMD;
	word[2]=BDM_NOP_CMD;
	i=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(word), (unsigned long)&word);
	if(i<0)
	    return i;
	if(word[1]==BDM_BERR) {
	    /* get BDM_NOT_READY after BERR */
	    unsigned long poll;
	    poll = BDM_NOP_CMD;
	    bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	    return -1;
	}
	*value = ((word[1] & 0xffff) << 16) | (word[2] & 0xffff);
	return 0;
    }
    if(which <= MAX_CONFIG_REGS) {
	unsigned long word[5];
	which=regs_log2phys(which);
	word[0]=BDM_RCREG_CMD;
	word[1]=0;
	word[2]=which;
	word[3]=BDM_NOP_CMD;
	word[4]=BDM_NOP_CMD;
	i=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(word), (unsigned long)&word);
	if(i<0)
	    return i;	    
	if(word[4]==BDM_BERR) {
	    /* get BDM_NOT_READY after BERR */
	    unsigned long poll;
	    poll = BDM_NOP_CMD;
	    bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	    return -1;
	}
	word[0]=word[4];
	while(word[0]==BDM_NOTREADY) {
	    unsigned long poll = BDM_NOP_CMD;
	    i=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	    if(i<0)
		return i;
	    if(poll==BDM_BERR) {
		/* get BDM_NOT_READY after BERR */
		unsigned long poll;
		poll = BDM_NOP_CMD;
		bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
		return -1;
	    }
	}
	*value = ((word[3] & 0xffff) << 16) | (word[4] & 0xffff);
	return 0;
    }
    if(which <= MAX_DEBUG_REGS) {
	unsigned long word[3];
	which=regs_log2phys(which);
	word[0]=BDM_RDMREG_CMD | which;
	word[1]=BDM_NOP_CMD;
	word[2]=BDM_NOP_CMD;
	i=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(word), (unsigned long)&word);
	if(i<0)
	    return i;
	if(word[1]==BDM_ILLEGAL)
	    return -1;
	*value = ((word[1] & 0xffff) << 16) | (word[2] & 0xffff);
	if(which == REG_CSR) {
	    bdm_update_status(*value);
	}
	return 0;
    }
    return -1;
#else
    /* not implemented */
    return -1;
#endif
}

/* write a register in the chip */
int bdm_write_reg(CF_REGS which, unsigned long *value)
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    int i;
    
    if(which <= MAX_USER_REGS) {
	unsigned long word[4];
	which=regs_log2phys(which);
	word[0]=BDM_WREG_CMD | which;
	word[1]=(*value >> 16) & 0xffff;
	word[2]=*value & 0xffff;
	word[3]=BDM_NOP_CMD;
	i=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(word), (unsigned long)&word);
	if(i<0)
	    return i;
	if(word[1]==BDM_BERR) {
	    /* get BDM_NOT_READY after BERR */
	    unsigned long poll;
	    poll = BDM_NOP_CMD;
	    bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	    return -1;
	}
	return 0;
    }
    if(which <= MAX_CONFIG_REGS) {
	unsigned long word[6];
	which=regs_log2phys(which);
	word[0]=BDM_WCREG_CMD;
	word[1]=0;
	word[2]=which;
	word[3]=(*value >> 16) & 0xffff;
	word[4]=*value & 0xffff;
	word[5]=BDM_NOP_CMD;
	i=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(word), (unsigned long)&word);
	if(i<0)
	    return i;	    
	if(word[5]==BDM_BERR) {
	    /* get BDM_NOT_READY after BERR */
	    unsigned long poll;
	    poll = BDM_NOP_CMD;
	    bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	    return -1;
	}
	word[0]=word[5];
	while(word[0]==BDM_NOTREADY) {
	    unsigned long poll = BDM_NOP_CMD;
	    i=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	    if(i<0)
		return i;
	    if(poll==BDM_BERR) {
		/* get BDM_NOT_READY after BERR */
		unsigned long poll;
		poll = BDM_NOP_CMD;
		bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
		return -1;
	    }
	}
	return 0;
    }
    if(which <= MAX_DEBUG_REGS) {
	unsigned long word[4];
	which=regs_log2phys(which);
	word[0]=BDM_WDMREG_CMD | which;
	word[1]=(*value >> 16) & 0xffff;
	word[2]=*value & 0xffff;
	word[3]=BDM_NOP_CMD;
	i=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(word), (unsigned long)&word);
	if(i<0)
	    return i;
	if(word[1]==BDM_ILLEGAL)
	    return -1;
	return 0;
    }
    return -1;
#else
    /* not implemented */
    return -1;
#endif
}

/* read a block of memory, alignment doesn't matter anymore */
int bdm_read_mem(unsigned int addr, unsigned char *mem, int count)
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    unsigned long word[4], poll;
    int read_bytes=0, ret;
    int i;
    
    if(!count) return 0;
    
    /* align to long word boundary from target's point of view */
    i=4-(addr&3);
    for(; i>0 && count>0; i--) {
	word[0]=BDM_READ_CMD | BDM_SIZE_BYTE;
	word[1]=(addr>>16) & 0xffff;
	word[2]=addr & 0xffff;
	word[3]=BDM_NOP_CMD;
	ret=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(word), (unsigned long)&word);
	if(ret<0)
	    return ret;	    
	if(word[3]==BDM_BERR) {
	    /* get BDM_NOT_READY after BERR */
	    unsigned long poll;
	    poll = BDM_NOP_CMD;
	    bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	    return BDM_FAULT_BERR;
	}
	if(word[3]==BDM_ILLEGAL)
	    return BDM_FAULT_NVC;
	poll=word[3];
	while(poll==BDM_NOTREADY) {
	    poll = BDM_NOP_CMD;
	    ret=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	    if(ret<0)
		return ret;
	    if(poll==BDM_BERR) {
		/* get BDM_NOT_READY after BERR */
		unsigned long poll;
		poll = BDM_NOP_CMD;
		bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
		return BDM_FAULT_BERR;
	    }
	    if(poll==BDM_ILLEGAL)
		return BDM_FAULT_NVC;
	}
	read_bytes++;
	count--;
	addr++;
	*mem++=(unsigned char)(0xff & poll);
    }
    if(count>4) {
	ret = bdm_read(fd, mem, count & ~3);
    }
    if(ret < 0) return ret;
    read_bytes+=ret;
    count-=ret;
    addr+=ret;
    mem+=ret;
    
    while(count>0) {
	word[0]=BDM_READ_CMD | BDM_SIZE_BYTE;
	word[1]=(addr>>16) & 0xffff;
	word[2]=addr & 0xffff;
	word[3]=BDM_NOP_CMD;
	ret=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(word), (unsigned long)&word);
	if(ret<0)
	    return ret;	    
	if(word[3]==BDM_BERR) {
	    /* get BDM_NOT_READY after BERR */
	    unsigned long poll;
	    poll = BDM_NOP_CMD;
	    bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	    return BDM_FAULT_BERR;
	}
	if(word[3]==BDM_ILLEGAL)
	    return BDM_FAULT_NVC;
	poll=word[3];
	while(poll==BDM_NOTREADY) {
	    poll = BDM_NOP_CMD;
	    ret=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	    if(ret<0)
		return ret;
	    if(poll==BDM_BERR) {
		/* get BDM_NOT_READY after BERR */
		unsigned long poll;
		poll = BDM_NOP_CMD;
		bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
		return BDM_FAULT_BERR;
	    }
	    if(poll==BDM_ILLEGAL)
		return BDM_FAULT_NVC;
	}
	read_bytes++;
	count--;
	addr++;
	*mem++=(unsigned char)(0xff & poll);
    }
    return read_bytes;
#else
    /* not implemented */
    return -1;
#endif
}

#if BDMDriverVersion_M == BDMDriverFiedler_M
static int perform_byte_write(int fd, unsigned int addr, unsigned char *mem)
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    unsigned long word[5];
    int ret;
    
    word[0]=BDM_WRITE_CMD | BDM_SIZE_BYTE;
    word[1]=(addr>>16) & 0xffff;
    word[2]=addr & 0xffff;
    word[3]=*mem;
    word[4]=BDM_NOP_CMD;
    ret=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(word), (unsigned long)&word);
    if(ret<0) {
	PRINTD("error in bdm_ioctl (0x%08x): %d\n", addr, ret);
	return ret;
    }
    if(word[4]==BDM_BERR) {
	/* get BDM_NOT_READY after BERR */
	PRINTD("bus error in bdm_ioctl (0x%08x): %d\n", addr, ret);
	return BDM_FAULT_BERR;
    }
    if(word[4]==BDM_ILLEGAL) {
	/* get BDM_NOT_READY after ILLEGAL */
	PRINTD("illegal cmd in bdm_ioctl (0x%08x): %d\n", addr, ret);
	return BDM_FAULT_NVC;
    }
    while(1) {
	word[4] = BDM_NOP_CMD;
	ret=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(word[4]), (unsigned long)&word[4]);
	if(ret<0) {
	    PRINTD("error in bdm_ioctl poll (0x%08x): %d\n", addr, ret);
	    return ret;
	}
	switch(word[4]) {
	  case BDM_BERR:
	    /* get BDM_NOT_READY after BERR */
	    PRINTD("bus error in bdm_ioctl poll (0x%08x): %d\n", addr, ret);
	    return BDM_FAULT_BERR;
	  case BDM_ILLEGAL:
	    PRINTD("illegal cmd in bdm_ioctl poll (0x%08x): %d\n", addr, ret);
	    return BDM_FAULT_NVC;
	  case BDM_COMPLETE:
	    return 0; /* we've done it ! */
	  case BDM_NOTREADY:
	    break;
	  default:
	    PRINTD("undefined reponse from target (0x%08x): %d\n", addr, ret);
	    return BDM_FAULT_BERR;
	}
    }
#else
    /* not implemented */
    return -1;
#endif
}
#endif

/* write a block of memory, alignment doesn't matter anymore */
int bdm_write_mem(unsigned int addr, unsigned char *mem, int count)
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    int written=0, ret;
    int i;
    
    if(!count) return 0;
    
    PRINTD("entering write_mem...");
    /* align to long word boundary from target's point of view */
    i=4-(addr&3); /* perform at least one write to set address */
    for(; i && count; i--) {
	PRINTD("alignment loop and AATR setup: %d\n", i);
	ret=perform_byte_write(fd, addr, mem);
	if(ret<0) return ret;
	mem++;
	written++;
	count--;
	addr++;
    }
    if(count>4) {
	ret = bdm_write(fd, mem, count & ~3);
    }
    if(ret < 0) {
	PRINTD("error in bdm_write: %d\n", ret);
	return ret;
    }
    written+=ret;
    count-=ret;
    addr+=ret;
    mem+=ret;
    PRINTD("write syscall wrote %d (%x) bytes.\n", ret, ret);
    
    while(count>0) {
	ret=perform_byte_write(fd, addr, mem);
	if(ret<0) return ret;
	mem++;
	written++;
	count--;
	addr++;
    }
    PRINTD("bdm_write_mem finished, %d bytes written\n", written);	
    return written;
#else
    /* not implemented */
    return -1;
#endif
}

void bdm_clear_status(void)
{
    target_status = 0;
}

int bdm_query_status(void)
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    unsigned long x, y /* , z */;
    
    y=0;
    x = bdm_ioctl(fd, BDM_GET_STATUS_IOC, 0);
    if(x & BDM_TARGETRESET) y |= BDM_STAT_RESET;
    if(x & BDM_TARGETHALT) y |= BDM_STAT_PSTHALT;
    if(x & BDM_TARGETSTOPPED);        
    if(x & BDM_TARGETPOWER) return BDM_STAT_NOPWR;
    if(x & BDM_TARGETNC) return BDM_STAT_NC; 
    
    if(bdm_read_reg(REG_CSR, &x)) return -1;
    printf("CSR=%08lx\n", x);
/*    z = x & 0x10; USER HALT ENABLE */
/*    y = x >> 28;  it's better to translate */
    x |= target_status;
    if(x & CSR_BKPT) y |= BDM_STAT_BKPT;
    if(x & CSR_HALT) y |= BDM_STAT_HALT;
    if(x & CSR_TRG) y |= BDM_STAT_TRG;
    if(x & CSR_FOF) y |= BDM_STAT_FOF;
    if(x & CSR_SSM) y |= BDM_STAT_SSM;
    switch(x>>28) {
     case CSR_STAT_WAIT1: 
	y |= BDM_STAT_WAIT1;
	break;
     case CSR_STAT_TRIG1:
	y |= BDM_STAT_TRIG1;
	break;
     case CSR_STAT_WAIT2:
	y |= BDM_STAT_WAIT2;
	break;
     case CSR_STAT_TRIG2: 
	y |= BDM_STAT_TRIG2;
	break;
    }
    return y;
#else
    /* not implemented */
    return -1;
#endif
}

/*
 * wait for stopped target
 */
int bdm_wait(void)
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    unsigned long x;
    
    while(1) {
	usleep(100000);
	x = bdm_ioctl(fd, BDM_GET_STATUS_IOC, 0);
	if(x & BDM_TARGETRESET) return BDM_STAT_RESET;
	if(x & BDM_TARGETHALT) return BDM_STAT_PSTHALT;
	if(x & BDM_TARGETSTOPPED) return BDM_STAT_HALT;        
	if(x & BDM_TARGETPOWER) return BDM_STAT_NOPWR;
	if(x & BDM_TARGETNC) return BDM_STAT_NC;
    }
#else
    /* not implemented */
    return -1;
#endif
}


static unsigned long tdr = 0; /* shadow trigger definition register */
/*
 * breakpoint control
 */
int bdm_set_pc_bp(unsigned long addr, unsigned long mask)
{
    bdm_stop();
    if(bdm_write_reg(REG_PBR, &addr)) return -1;
    if(bdm_write_reg(REG_PBMR, &mask)) return -1;
    tdr = (1 << 30) | (~(3<<30) & tdr); /* trigger response is halt */
    tdr |=  1 << 29;  /* enable second level trigger breakpoints */
    tdr |=  1 << 13;  /* enable first level trigger breakpoints */
    tdr |=  1 << 1;   /* enable PC breakpoint */
    tdr &=  ~1;       /* clear PC breakpoint invert */
    if(bdm_write_reg(REG_TDR, &tdr)) return -1;
    return 0;
}

int bdm_clear_pc_bp(void)
{
    bdm_stop();
    tdr &=  ~(1 << 1);   /* disable PC breakpoint */
    if(bdm_write_reg(REG_TDR, &tdr)) return -1;
    return 0;
}

int bdm_set_addr_bp(unsigned long from_addr, unsigned long to_addr)
{
    unsigned long tmp=0xff05;
    
    bdm_stop();
    if(bdm_write_reg(REG_ABLR, &from_addr)) return -1;
    if(bdm_write_reg(REG_ABHR, &to_addr)) return -1;
    if(bdm_write_reg(REG_AATR, &tmp)) return -1; /* match everything */
    tdr = (1 << 30) | (~(3<<30) & tdr); /* trigger response is halt */
    tdr &=  ~(1 << 29);  /* disable second level trigger breakpoints */
    tdr |=  1 << 13;  /* enable first level trigger breakpoints */
    tdr = (tdr & ~(7<<2)) | (2 << 2);
                      /* enable address range breakpoint */
    if(bdm_write_reg(REG_TDR, &tdr)) return -1;
    return 0;
    
}

int bdm_clear_addr_bp(void)
{
    bdm_stop();
    tdr = (tdr & ~(7<<2)) | (0 << 2);
                      /* disable address range breakpoint */
    if(bdm_write_reg(REG_TDR, &tdr)) return -1;
    return 0;
}

int bdm_set_data_bp(unsigned long value, unsigned long mask)
{
    bdm_stop();
    bdm_write_reg(REG_DBR, &value);
    bdm_write_reg(REG_DBMR, &mask);
    tdr = (1 << 30) | (~(3<<30) & tdr); /* trigger response is halt */
    tdr &=  ~(1 << 29);  /* disable second level trigger breakpoints */
    tdr |=  1 << 13;  /* enable first level trigger breakpoints */
    tdr = (tdr & ~(0xff<<5)) | (0x80 << 5);
                      /* enable data breakpoint for long word */
    if(bdm_write_reg(REG_TDR, &tdr)) return -1;
    return 0;
}

int bdm_clear_data_bp(void)
{
    bdm_stop();
    tdr = (tdr & ~(0xff<<5)) | (0x00 << 5);
                      /* disable data breakpoint */
    if(bdm_write_reg(REG_TDR, &tdr)) return -1;
    return 0;
}
