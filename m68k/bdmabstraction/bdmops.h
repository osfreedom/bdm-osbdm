#ifndef _BDMOPS_H_
#define _BDMOPS_H_

/*
 * bdm abstraction for coldfire processors
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

typedef enum CF_REGS {
        /* user registers */
        REG_D0,
	REG_D1,
	REG_D2,
	REG_D3,
	REG_D4,
	REG_D5,
	REG_D6,
	REG_D7,
	REG_A0,
	REG_A1,
	REG_A2,
	REG_A3,
	REG_A4,
	REG_A5,
	REG_A6,
	REG_A7,
	MAX_USER_REGS=REG_A7,
	/* RCREG/WCREG */
	REG_CACR,
	REG_ACR0,
	REG_ACR1,
	REG_VBR,
	REG_SR,
	REG_RPC,
	REG_RAMBAR,
	REG_MBAR,
	MAX_CONFIG_REGS=REG_MBAR,
	/* RDMREG/WDMREG */
	REG_CSR,
	REG_AATR,
	REG_TDR,
	REG_PBR,
	REG_PBMR,
	REG_ABHR,
	REG_ABLR,
	REG_DBR,
	REG_DBMR,
	MAX_DEBUG_REGS=REG_DBMR,
	MAX_REGS
} CF_REGS;

#ifdef REG_NAMES
struct reg_names {
    CF_REGS id;
    const char name[8];
} reg_names[]={
        /* user registers */
        { REG_D0, "D0"},
	{ REG_D1, "D1"},
	{ REG_D2, "D2"},
	{ REG_D3, "D3"},
	{ REG_D4, "D4"},
	{ REG_D5, "D5"},
	{ REG_D6, "D6"},
	{ REG_D7, "D7"},
	{ REG_A0, "A0"},
	{ REG_A1, "A1"},
	{ REG_A2, "A2"},
	{ REG_A3, "A3"},
	{ REG_A4, "A4"},
	{ REG_A5, "A5"},
	{ REG_A6, "A6"},
	{ REG_A7, "A7"},
	/* RCREG/WCREG */
	{ REG_CACR, "CACR"},
	{ REG_ACR0, "ACR0"},
	{ REG_ACR1, "ACR1"},
	{ REG_VBR, "VBR"},
	{ REG_SR, "SR"},
	{ REG_RPC, "RPC"},
	{ REG_RAMBAR, "RAMBAR"},
	{ REG_MBAR, "MBAR"},
	/* RDMREG/WDMREG */
	{ REG_CSR, "CSR"},
	{ REG_AATR, "AATR"},
	{ REG_TDR, "TDR"},
	{ REG_PBR, "PBR"},
	{ REG_PBMR, "PBMR"},
	{ REG_ABHR, "ABHR"},
	{ REG_ABLR, "ABLR"},
	{ REG_DBR, "DBR"},
	{ REG_DBMR, "DBMR"},
        { 0xffff, ""}
};
#endif

/* debug module register definitions */
/* AATR */
#define AATR_RM      (1<<15)    /* Read/Write mask */
#define AATR_SZM     (3<<13)    /* SiZe Mask */
#define AATR_TTM     (3<<11)    /* Transfer Type Mask */
#define AATR_TMM     (7<<8)     /* Transfer Modifier Mask */
#define AATR_R       (1<<7)     /* Read/write */
#define AATR_SZ      (3<<5)     /* SiZe */
#define AATR_TT      (3<<3)     /* Tranfer Type */
#define AATR_TM      (7<<0)     /* Transfer Modifier */
/* TDR */
#define TDR_TRC      (3<<30)    /* Trigger Response Control */
#define TDR_EBL2     (1<<29)    /* Enable Breakpoint Level */
#define TDR_EDLW2    (1<<28)    /* Enable Data BP for LongWord */
#define TDR_EDWL2    (1<<27)    /* Enable Data BP for Word (Upper) */
#define TDR_EDWU2    (1<<26)    /* Enable Data BP for Word (Lower) */
#define TDR_EDLL2    (1<<25)    /* Enable Data BP for byte (LL) */
#define TDR_EDLM2    (1<<24)    /* Enable Data BP for byte (LM) */
#define TDR_EDUM2    (1<<23)    /* Enable Data BP for byte (UM) */
#define TDR_EDUU2    (1<<22)    /* Enable Data BP for byte (UU) */
#define TDR_DI2      (1<<21)    /* Data breakpoint invert */
#define TDR_EAI2     (1<<20)    /* Enable Address breakpoint inverted */
#define TDR_EAR2     (1<<19)    /* Enable Address breakpoint Range */
#define TDR_EAL2     (1<<18)    /* Enable Address breakpoint Low */
#define TDR_EPC2     (1<<17)    /* Enable PC breakpoint */
#define TDR_PCI2     (1<<16)    /* PC breakpoint Invert */
#define TDR_EBL1     (1<<13)    /* Enable Breakpoint Level */
#define TDR_EDLW1    (1<<12)    /* Enable Data BP for LongWord */
#define TDR_EDWL1    (1<<11)    /* Enable Data BP for Word (Upper) */
#define TDR_EDWU1    (1<<10)    /* Enable Data BP for Word (Lower) */
#define TDR_EDLL1    (1<<9)     /* Enable Data BP for byte (LL) */
#define TDR_EDLM1    (1<<8)     /* Enable Data BP for byte (LM) */
#define TDR_EDUM1    (1<<7)     /* Enable Data BP for byte (UM) */
#define TDR_EDUU1    (1<<6)     /* Enable Data BP for byte (UU) */
#define TDR_DI1      (1<<5)     /* Data breakpoint invert */
#define TDR_EAI1     (1<<4)     /* Enable Address breakpoint inverted */
#define TDR_EAR1     (1<<3)     /* Enable Address breakpoint Range */
#define TDR_EAL1     (1<<2)     /* Enable Address breakpoint Low */
#define TDR_EPC1     (1<<1)     /* Enable PC breakpoint */
#define TDR_PCI1     (1<<0)     /* PC breakpoint Invert */
/* CSR */
#define CSR_STATUS   (15<<28)   /* breakpoint status */
#       define CSR_STAT_NOBP    0
#       define CSR_STAT_WAIT1   1
#       define CSR_STAT_TRIG1   2
#       define CSR_STAT_WAIT2   5
#       define CSR_STAT_TRIG2   6
#define CSR_FOF      (1<<27)    /* Fault-On-Fault */
#define CSR_TRG      (1<<26)    /* hardware breakpoint TRiGger */
#define CSR_HALT     (1<<25)    /* processor HALT */
#define CSR_BKPT     (1<<24)    /* BreaKPoinT assert */
#define CSR_IPW      (1<<16)    /* Inhibit Processor Writes to dbg reg. */
#define CSR_MAP      (1<<15)    /* MAP processor accesses in emu mode */
#define CSR_TRC      (1<<14)    /* emulation mode on TRaCe exception */
#define CSR_EMU      (1<<13)    /* force EMUlation mode */
#define CSR_DDC      (3<<11)    /* Debug Data Control */
#define CSR_UHE      (1<<10)    /* User Halt Enable */
#define CSR_BTB      (3<<8)     /* Branch Target Bytes */
#define CSR_NPL      (1<<6)     /* NonPipelined Mode */
#define CSR_IPI      (1<<5)     /* Ignore Pending Interrupts */
#define CSR_SSM      (1<<4)     /* Single Step Mode */

/* 
 * this is an abstraction layer to have a general target programming interface
 * so the programmer doesn't have to bother with motorola's bdm commands.
 * 
 * applications only include bdmops.h and call the functions below
 */

/* open bdm-driver */
int /* <0 if error, BDMHandle otherwise */
bdm_init(const char *path);
/* close driver */
void bdm_release(int port);
/* bring chip to running state */
void bdm_run(void);
/* bring chip to stopped state */
void bdm_stop(void);
/* step chip on instruction */
void bdm_step(void);
/* reset chip and hold in reset state */
void bdm_reset(void);
/* restart from reset to running state */
void bdm_restart(void);
/* read a register from the chip */
int bdm_read_reg(CF_REGS which, unsigned long *value);
/* write a register in the chip */
int bdm_write_reg(CF_REGS which, unsigned long *value);
/* read a block of memory, alignment doesn't matter anymore */
int bdm_read_mem(unsigned int addr, unsigned char *mem, int count);
/* write a block of memory, alignment doesn't matter anymore */
int bdm_write_mem(unsigned int addr, unsigned char *mem, int count);
/* set program counter break point */
int bdm_set_pc_bp(unsigned long addr, unsigned long mask);
int bdm_clear_pc_bp(void);
/* set program counter break point */
int bdm_set_addr_bp(unsigned long from_addr, unsigned long to_addr);
int bdm_clear_addr_bp(void);
/* set program counter break point */
int bdm_set_data_bp(unsigned long value, unsigned long mask);
int bdm_clear_daat_bp(void);
/* bdm query status */
void bdm_clear_status(void);
int bdm_query_status(void);
#define BDM_STAT_BKPT      (1<<0)
#define BDM_STAT_HALT      (1<<1)
#define BDM_STAT_TRG       (1<<2)
#define BDM_STAT_FOF       (1<<3)
#define BDM_STAT_SSM       (1<<4)
#define BDM_STAT_WAIT1     (1<<5)
#define BDM_STAT_TRIG1     (1<<6)
#define BDM_STAT_WAIT2     (1<<7)
#define BDM_STAT_TRIG2     (1<<8)
#define BDM_STAT_RESET     (1<<9)
#define BDM_STAT_NOPWR     (1<<10)
#define BDM_STAT_PSTHALT   (1<<11)
#define BDM_STAT_NC        (1<<12)
/* bdm wait for stopped target */
int bdm_wait(void);

#endif
