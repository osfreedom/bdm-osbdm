/*******************************************************************
  TPUDB Project to Create Free Motorola 683xx TPU Debugger

  tpudb.c	- first test

  (C) Copyright 2000 by Pavel Pisa - Original Author
      e-mail:	pisa@cmp.felk.cvut.cz
      homepage:	http://cmp.felk.cvut.cz/~pisa
      work:	http://www.pikron.com/

  This package can be copied and modified under 
  GNU General Public License with all its conditions.
  See file GPL for details. Under this license nobody can
  distribute this work and any derived work without full source code
  for all modules compiled and linked into executable.
 
 *******************************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <bfd.h>
#include "bdm.h"
#include "bdmlib.h"

#define __val2mfld(mask,val) (((mask)&~((mask)<<1))*(val)&(mask))
#define __mfld2val(mask,val) (((val)&(mask))/((mask)&~((mask)<<1)))

#define val2mfld __val2mfld
#define mfld2val __mfld2val

#if 0
  #define TPU_WR16(adr,val) (*(volatile u_int16_t*)(adr)=(val))
  #define TPU_RD16(adr) (*(volatile u_int16_t*)(adr))
#else
  #define TPU_WR16(adr,val) \
  		({ \
  		  if(bdmlib_write_var(adr,BDM_SIZE_WORD,val)<0) \
  		  	goto mem_op_error; \
  		  val; \
  		})
  #define TPU_RD16(adr) \
  		({ u_int16_t temp_val; \
  		  if(bdmlib_read_var(adr,BDM_SIZE_WORD,&temp_val)<0) \
  		  	goto mem_op_error; \
  		  temp_val; \
  		})
#endif

char hashmark=0;			/* flag set by "set hash" */
int  bdm_autoreset=1;			/* automatic reset before load */
int  bdm_ttcu=0;			/* time to come up for init by rom */
char initname[255];			/* need reimplement !!!!!!!!! */
char bdm_dev_name[255]="/dev/bdm";	/* device name */

/* Numberring of TPU register */
/* First part are aliases for main CPU accesible registers  */
#define TPUREG_TSTMSRA	0x0	/* Master Shift Register A */
#define TPUREG_TSTMSRB	0x1	/* Master Shift Register B */
#define TPUREG_TSTSC	0x2	/* Test Module Shift Count */
#define    TPU_TSTSCA 0xff00	/*    Bits Shifted from TSTMSRA to Tested Module */
#define    TPU_TSTSCB 0x00ff	/*    Bits Shifted from Tested Module to TSTMSRB */
#define TPUREG_TSTRC	0x3	/* Test Module Repetition Count */
#define TPUREG_CREG	0x4	/* Test Submodule Control Register */
#define    TPU_TSTBUSY 0x8000	/*    Test Submodule Busy Status Bit */
#define    TPU_TMARM 0x4000	/*    Test Mode Armed Status Bit (latched /TSTME) */
#define    TPU_COMP  0x2000	/*    Compare Status Bit */
#define    TPU_IMBTST 0x1000	/*    Intermodule Bus Test (SCANA/SCANB) */
#define    TPU_CPUTR 0x0800	/*    Scan to CPU Test Register */
#define    TPU_QBIT  0x0400	/*    Quotient Bit at FREEZE/QUOT Pin */
#define    TPU_MUXEL 0x0200	/*    Multiplexer Select Bit for MSRB from Int/Ext */
#define    TPU_ACUT  0x0010	/*    Activate Circuit Under Test */
#define    TPU_SCONT 0x0008	/*    Start Continuous Operation */
#define    TPU_SSHOP 0x0004	/*    Start Shifting Operation */
#define    TPU_SATO  0x0002	/*    Start Automatic Test Operation */
#define    TPU_EMT   0x0001	/*    Enter test mode */
#define TPUREG_DREG	0x5	/* Distributed Register */
#define    TPU_WAIT  0x0700	/*    Wait Counter Preset 2-16 SCLK */
#define    TPU_MSRAHI 0x0e0	/*    MSRA High Bits 18-16 */
#define    TPU_MSRAC 0x0010	/*    Master Shift Register A Configuration */
#define    TPU_MSRBHI 0x00e	/*    MSRB High Bits 18-16 */
#define    TPU_MSRBC 0x0001	/*    Master Shift Register B Configuration */
#define TPUREG_MCR	0x6	/* TPU Module Configuration Register   */
#define    TPU_STOP  0x8000	/*    Low-Power Stop Mode Enable */
#define    TPU_TCR1P 0x6000	/*    Timer Count Register 1 Prescaler Control */
#define    TPU_TCR2P 0x1800	/*    Timer Count Register 2 Prescaler Control */
#define    TPU_EMU   0x0400	/*    Emulation Control */
#define    TPU_T2CG  0x0200	/*    TCR2 Clock/Gate Control */
#define    TPU_STF   0x0100	/*    Stop Flag */
#define    TPU_SUPV  0x0080	/*    Supervisor/Unrestricted */
#define    TPU_PSCK  0x0040	/*    Prescaler Clock */
#define    TPU_IARB  0x000f	/*    Interrupt Arbitration ID */
#define TPUREG_TCR	0x7	/* Test Configuration Register */
#define    TPU_INCAD 0x1000	/*    Increment Address in uPC at ACUTL */
#define    TPU_TCR1C 0x0800	/*    TCR1 Clock - TCR2 pin */
#define    TPU_ACUTR 0x0600	/*    Activate Circuit Under Test Response 1, 0 */
#define        TPU_ACUTR_NONE		0 /* None */
#define        TPU_ACUTR_STEPTPU	1 /* Run ONE TPU microcycle */
#define        TPU_ACUTR_SHEOT		2 /* Scheduler End of Time Slot */
#define    TPU_SOSEL 0x0070	/*    Scan-Out Select */
#define    TPU_SISEL 0x000e	/*    Scan-in Select */
#define        TPU_SxSEL_NONE		0 /* None */
#define        TPU_SxSEL_PC		1 /* uPC */
#define        TPU_SxSEL_IR		2 /* Microinstruction */
#define        TPU_SxSEL_BPLA		3 /* Branch PLA */
#define        TPU_SxSEL_PCBR		4 /* uPC Breakpoint */
#define        TPU_SxSEL_SPLA		5 /* Scheduler PLA */
#define        TPU_SxSEL_CHANBR		6 /* Channel Breakpoint */
#define    TPU_TMW   0x0001	/*    Test Memory Map */
#define TPUREG_DSCR	0x8	/* Development Support Control Register */
#define    TPU_HOT4  0x8000	/*    Hang on T4 */
#define    TPU_BLC   0x0400	/*    Branch Latch Control */
#define    TPU_CLKS  0x0200	/*    Stop Clocks (to TCRs) */
#define    TPU_FRZ   0x0180	/*    FREEZE Assertion Response */
#define    TPU_CCL   0x0040	/*    Channel Conditions Latch */
#define    TPU_BMSK  0x003F	/*    Break cond mask */
#define    TPU_BP    0x0020	/*    Break  mPC == mPC breakpoint register */
#define    TPU_BC    0x0010	/*    Break if CHAN start or set */
#define    TPU_BH    0x0008	/*    Break if host service latch set */
#define    TPU_BL    0x0004	/*    Break if link service latch set */
#define    TPU_BM    0x0002	/*    Break if MRL set at beginning*/
#define    TPU_BT    0x0001	/*    Break if TDL set at beginnibg */
#define TPUREG_DSSR	0x9	/* Development Support Status Register   */
#define    TPU_BKPT  0x0080	/*    Breakpoint Asserted Flag */
#define    TPU_PCBK  0x0040	/*    mPC Breakpoint Flag */
#define    TPU_CHBK  0x0020	/*    Channel Register Breakpoint Flag */
#define    TPU_SRBK  0x0010	/*    Service Request Breakpoint Flag */
#define    TPU_TPUF  0x0008	/*    TPU FREEZE Flag */
#define TPUREG_TICR	0xA	/* TPU Interrupt Configuration Register */
#define    TPU_CIRL  0x0700	/*    Channel Interrupt Request Level */
#define    TPU_CIBV  0x00f0	/*    Bits [7:4] of Channel Interrupt Base Vector */
#define TPUREG_CIER	0xB	/* Channel Interrupt Enable Register */
#define TPUREG_CFSR	0xC	/* Channel Function Select Register 0 */
#define TPUREG_HSQR	0xD	/* Host Sequence Register 0 */
#define TPUREG_HSRR	0xE	/* Host Service Request Register 0 */
#define TPUREG_CPR	0xF	/* Channel Priority Register 0 */
#define TPUREG_CISR	0x10	/* Channel Interrupt Status Register */
#define TPUREG_LR	0x11	/* Link Register */
#define TPUREG_SGLR	0x12	/* Service Grant Latch Register */
#define TPUREG_DCNR	0x13	/* Decoded Channel Number Register */
/* Registers not accesible from main CPU */
#define TPUREG_P	0x14	/* Parameter Register (16-bit) */
#define TPUREG_A	0x15	/* Accumulator (16-bit) */
#define TPUREG_DIOB	0x16	/* Data I/O Buffer (16-bit) */
#define TPUREG_SR	0x17	/* Shift Register (16-bit) */
#define TPUREG_ERT	0x18	/* Event Temporary Register (16-b.) */
#define TPUREG_CHAN	0x19	/* Channel Number (4-bit) */
#define TPUREG_DEC	0x1A	/* Decrementator Reg (4-bit) */
#define TPUREG_TCR2	0x1B	/* Timebase Register 2 (16-bit) */
#define TPUREG_TCR1	0x1C	/* Timebase Register 1 (16-bit) */
#define TPUREG_PC	0x1D	/* Microprogram Counter (9-bit) */
#define TPUREG_IR	0x1E	/*  */
#define TPUREG_BPLA	0x1F	/*  */
#define TPUREG_SPLA	0x20	/*  */
#define TPUREG_PCBR	0x21	/*  */
#define TPUREG_CHANBR	0x22	/*  */
/* Aditional CPU Accessible Registers */
#define TPUREG_DEBPAR	0x23	/* Parameter register at 0xFFFF00 for debugging */
#define TRAMREG_CR	0x24	/* TPURAM Configuration Register */
#define    TRAM_STOP  0x8000	/*    Low-Power Stop Mode Enable */
#define    TRAM_RASP  0x0080	/*    TPURAM Array Space */
#define TRAMREG_TST	0x25	/* TPURAM Test Register */
#define TRAMREG_BAR	0x26	/* TPURAM Base Address and Status Register */
#define    TRAM_ADDR  0xfff0	/*    TPURAM Array Base Address */
#define    TRAM_RAMDS 0x0001	/*    RAM Array Disable */

#define TPUREG_MAX	0x26

/* Descriptor of TPU register access */
typedef struct tpu_reg_des {
  int (*reg_rd)(struct tpu_reg_des *des, int indx);
  int (*reg_wr)(struct tpu_reg_des *des, int indx, int val);
  int adr;
  int adr1;
  int flags;
  int arr_len;		/* array size, 0 .. no array */
  int bfld_bits;	/* bitfield length, 0 .. no bitfields */
  int bfld_arr_len;	/* bitfield array */
}tpu_reg_des_t;

int tpu_mem_rd16(struct tpu_reg_des *des, int indx);
int tpu_mem_wr16(struct tpu_reg_des *des, int indx, int val);
int tpu_t1_rd(struct tpu_reg_des *des, int indx);
int tpu_t1_wr(struct tpu_reg_des *des, int indx, int val);
int tpu_t2_rd(struct tpu_reg_des *des, int indx);
int tpu_t2_wr(struct tpu_reg_des *des, int indx, int val);
int tpu_reg_rd(int reg, int indx);
int tpu_reg_wr(int reg, int indx, int val);
int tpu_reg_or(int reg, int indx, int val);
int tpu_reg_and(int reg, int indx, int val);

#define TPUREG_FL_SH4	1	/* shift value by 4 bits */
#define TPUREG_FL_PMOD	2	/* modification for reg P */
#define TPUREG_FL_DIOB	4	/* modification for reg DIOB */

#define TPUREG_INDX_BFLD  (0x1<<24)	/* index for bitfields */

/* Descriptors of TPU register access */
tpu_reg_des_t tpu_regs[]={
  /* CPU accessible */
  [TPUREG_TSTMSRA]= {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFA30,},
  [TPUREG_TSTMSRB]= {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFA32,},
  [TPUREG_TSTSC]  = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFA34,},
  [TPUREG_TSTRC]  = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFA36,},
  [TPUREG_CREG]   = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFA38,},
  [TPUREG_DREG]   = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFA3A,},
  [TPUREG_MCR]	  = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFE00,},
  [TPUREG_TCR]    = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFE02,},
  [TPUREG_DSCR]   = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFE04,},
  [TPUREG_DSSR]   = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFE06,},
  [TPUREG_TICR]   = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFE08,},
  [TPUREG_CIER]   = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFE0A,
  			arr_len:0,bfld_bits:1,bfld_arr_len:16,},
  [TPUREG_CFSR]   = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFE0C,
  			arr_len:4,bfld_bits:4,bfld_arr_len:16,},
  [TPUREG_HSQR]   = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFE14,
  			arr_len:2,bfld_bits:2,bfld_arr_len:16,},
  [TPUREG_HSRR]   = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFE18,
  			arr_len:2,bfld_bits:2,bfld_arr_len:16,},
  [TPUREG_CPR]    = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFE1C,
  			arr_len:2,bfld_bits:2,bfld_arr_len:16,},
  [TPUREG_CISR]   = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFE20,
  			arr_len:0,bfld_bits:1,bfld_arr_len:16,},
  [TPUREG_LR]     = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFE22,},
  [TPUREG_SGLR]   = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFE24,},
  [TPUREG_DCNR]   = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFE26,},
  /* Special TPU - group 1 */
  /* internal TPU registers, ounly access possible through forced TPU microengine instructions */
  /* adr .. instruction for reg read, adr1 .. instruction for reg write */
  [TPUREG_P]      = {reg_rd:tpu_t1_rd,reg_wr:tpu_t1_wr,adr:0x3fff,adr1:0x36ff,
  			flags:TPUREG_FL_PMOD,},
  [TPUREG_A]      = {reg_rd:tpu_t1_rd,reg_wr:tpu_t1_wr,adr:0x327f,adr1:0x361f},
  [TPUREG_DIOB]   = {reg_rd:tpu_t1_rd,reg_wr:tpu_t1_wr,adr:0,adr1:0,
  			flags:TPUREG_FL_DIOB,},
  [TPUREG_SR]     = {reg_rd:tpu_t1_rd,reg_wr:tpu_t1_wr,adr:0x347f,adr1:0x363f},
  [TPUREG_ERT]    = {reg_rd:tpu_t1_rd,reg_wr:tpu_t1_wr,adr:0x3c7f,adr1:0x365f},
  [TPUREG_CHAN]   = {reg_rd:tpu_t1_rd,reg_wr:tpu_t1_wr,adr:0x267f,adr1:0x373f,
  			flags:TPUREG_FL_SH4,},
  [TPUREG_DEC]    = {reg_rd:tpu_t1_rd,reg_wr:tpu_t1_wr,adr:0x247f,adr1:0x375f},
  [TPUREG_TCR2]   = {reg_rd:tpu_t1_rd,reg_wr:tpu_t1_wr,adr:0x387f,adr1:0x379f},
  [TPUREG_TCR1]   = {reg_rd:tpu_t1_rd,reg_wr:tpu_t1_wr,adr:0x3a7f,adr1:0x37bf},
  /* Special TPU - group 2 */
  /* access through test subsystem serial scan lines */
  /* adr .. number of register bits, adr1 .. TPU scan line selection */
  [TPUREG_PC]     = {reg_rd:tpu_t2_rd,reg_wr:tpu_t2_wr,adr: 9,adr1:TPU_SxSEL_PC},
  [TPUREG_IR]     = {reg_rd:NULL,reg_wr:NULL,adr:0,adr1:0},
  [TPUREG_BPLA]   = {reg_rd:tpu_t2_rd,reg_wr:tpu_t2_wr,adr:16,adr1:TPU_SxSEL_BPLA},
  [TPUREG_SPLA]   = {reg_rd:tpu_t2_rd,reg_wr:tpu_t2_wr,adr:15,adr1:TPU_SxSEL_SPLA},
  [TPUREG_PCBR]   = {reg_rd:tpu_t2_rd,reg_wr:tpu_t2_wr,adr: 9,adr1:TPU_SxSEL_PCBR},
  [TPUREG_CHANBR] = {reg_rd:tpu_t2_rd,reg_wr:tpu_t2_wr,adr: 4,adr1:TPU_SxSEL_CHANBR},
  /* One of parameter registers used for internal registers access */ /* was TPUREG_DEBUR */
  [TPUREG_DEBPAR] = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFF00,},
  [TRAMREG_CR]	  = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFB00,},
  [TRAMREG_TST]	  = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFB02,},
  [TRAMREG_BAR]	  = {reg_rd:tpu_mem_rd16,reg_wr:tpu_mem_wr16,adr:0xFFFB04,},
};

/**************************************************************************/
/* Helper function for test register access */

static int use_fetched_reg;
static u_int16_t fetched_dscr;
static u_int16_t fetched_creg;
static u_int16_t fetched_tcr;

static u_int16_t stored_diob;
static u_int16_t stored_pc;
static u_int16_t stored_debpar2;	/* was strored_debur2 */
static u_int32_t stored_ir;		/* was strored_x1 */
					/* was strored_x2 */

static
void test_shift_wait(unsigned creg)
{
  int cnt=0x8000;
  tpu_reg_wr(TPUREG_CREG,0,creg|TPU_SSHOP);	/* 4 */
  do{
    creg=tpu_reg_rd(TPUREG_CREG,0);
    if(!(creg&TPU_TSTBUSY)) return;		/* 0x8000 */
  }while(cnt--);
  fprintf(stderr,"test_shift_wait : stalled busy condition\n");
}

/* Write 32 Bit Instruction Register */
static		/* was test_shift_1 */
void test_shift_irwr(u_int32_t new_ir )
{
  u_int16_t dscr; /* [bp-6] */
  u_int16_t creg; /* si */
  u_int16_t tcr;  /* di */

  if (!use_fetched_reg){
    tpu_reg_wr(TPUREG_TSTMSRB,0,0);
    tpu_reg_wr(TPUREG_TSTRC,0,0);
    tpu_reg_wr(TPUREG_DREG,0,0);
    dscr=tpu_reg_rd(TPUREG_DSCR,0);
    creg=tpu_reg_rd(TPUREG_CREG,0);
    tcr=tpu_reg_rd(TPUREG_TCR,0);
  }else{
    creg=fetched_creg;
    tcr=fetched_tcr;
    dscr=fetched_dscr;
  }

  creg=(creg|TPU_IMBTST)&~TPU_MUXEL;		/*!!!!!!!!!!!*/
  tpu_reg_wr(TPUREG_CREG,0,creg);

  if(!(dscr&TPU_HOT4))  /* ???????? */
    tpu_reg_wr(TPUREG_DSCR,0,dscr|TPU_HOT4);

  tcr&=TPU_INCAD | TPU_TCR1C | TPU_TMW;		/* 0x1801 */
  tpu_reg_wr(TPUREG_TCR,0,tcr | val2mfld(TPU_ACUTR,TPU_ACUTR_STEPTPU) |
  				val2mfld(TPU_SISEL,TPU_SxSEL_IR)); /* 0x204 */
  tpu_reg_wr(TPUREG_TSTSC,0,0x1000);
  tpu_reg_wr(TPUREG_TSTMSRA,0,new_ir&0xffff);

  test_shift_wait(creg);		/* test shift */

  tpu_reg_wr(TPUREG_TCR,0,tcr | val2mfld(TPU_ACUTR,TPU_ACUTR_STEPTPU) |
  				val2mfld(TPU_SISEL,TPU_SxSEL_IR)); /* 0x204 */
  tpu_reg_wr(TPUREG_TSTSC,0,0x1000);
  tpu_reg_wr(TPUREG_TSTMSRA,0,(new_ir>>16)&0xffff);

  test_shift_wait(creg);		/* test shift */

  if(!(dscr&TPU_HOT4))  /* ???????? */
    tpu_reg_wr(TPUREG_DSCR,0,dscr);
}

/* Read 32 Bit Instruction Register */
static		/* was test_shift_2 */
u_int32_t test_shift_irrd(void)
{
  u_int16_t dscr; /* [bp-0a] */
  u_int16_t creg; /* si */
  u_int16_t tcr;  /* di */
  u_int16_t ir_lo,ir_hi;

  if (!use_fetched_reg){
    tpu_reg_wr(TPUREG_TSTMSRB,0,0);
    tpu_reg_wr(TPUREG_TSTRC,0,0);
    tpu_reg_wr(TPUREG_DREG,0,0);
    dscr=tpu_reg_rd(TPUREG_DSCR,0);
    creg=tpu_reg_rd(TPUREG_CREG,0);
    tcr=tpu_reg_rd(TPUREG_TCR,0);
  }else{
    creg=fetched_creg;
    tcr=fetched_tcr;
    dscr=fetched_dscr;
  }

  creg=(creg|TPU_IMBTST)&~TPU_MUXEL;		/*!!!!!!!!!!!*/
  tpu_reg_wr(TPUREG_CREG,0,creg);

  if(!(dscr&TPU_HOT4))  /* ???????? */
    tpu_reg_wr(TPUREG_DSCR,0,dscr|TPU_HOT4);

  tcr&=TPU_INCAD | TPU_TCR1C | TPU_TMW;		/* 0x1801 */
  tpu_reg_wr(TPUREG_TCR,0,tcr | val2mfld(TPU_ACUTR,TPU_ACUTR_STEPTPU) |
  				val2mfld(TPU_SOSEL,TPU_SxSEL_IR)); /* 0x220 */
  tpu_reg_wr(TPUREG_TSTSC,0,0x0010);

  test_shift_wait(creg);		/* test shift */

  ir_lo=tpu_reg_rd(TPUREG_TSTMSRB,0);
  tpu_reg_wr(TPUREG_TCR,0,tcr | val2mfld(TPU_ACUTR,TPU_ACUTR_STEPTPU) |
  				val2mfld(TPU_SOSEL,TPU_SxSEL_IR) |
				val2mfld(TPU_SISEL,TPU_SxSEL_IR)); /* 0x224 */
  tpu_reg_wr(TPUREG_TSTSC,0,0x1010);
  tpu_reg_wr(TPUREG_TSTMSRA,0,ir_lo);

  test_shift_wait(creg);		/* test shift */

  ir_hi=tpu_reg_rd(TPUREG_TSTMSRB,0);
  tpu_reg_wr(TPUREG_TCR,0,tcr | val2mfld(TPU_ACUTR,TPU_ACUTR_STEPTPU) |
				val2mfld(TPU_SISEL,TPU_SxSEL_IR)); /* 0x204 */
  tpu_reg_wr(TPUREG_TSTSC,0,0x1000);
  tpu_reg_wr(TPUREG_TSTMSRA,0,ir_hi);
  
  test_shift_wait(creg);		/* test shift */

  if(!(dscr&TPU_HOT4))  /* ???????? */
    tpu_reg_wr(TPUREG_DSCR,0,dscr);
    
  return ir_lo|((u_int32_t)ir_hi<<16);
}

static 		/* was test_shift_3 */
int test_shift_regrd(int shiftcnt ,int sxsel)
{
  u_int16_t dscr; /* [bp-8] */
  u_int16_t creg; /* si */
  u_int16_t tcr;  /* di */
  u_int16_t ret;

  if (!use_fetched_reg){
    tpu_reg_wr(TPUREG_TSTMSRB,0,0);
    tpu_reg_wr(TPUREG_TSTRC,0,0);
    tpu_reg_wr(TPUREG_DREG,0,0);
    dscr=tpu_reg_rd(TPUREG_DSCR,0);
    creg=tpu_reg_rd(TPUREG_CREG,0);
    tcr=tpu_reg_rd(TPUREG_TCR,0);
  }else{
    creg=fetched_creg;
    tcr=fetched_tcr;
    dscr=fetched_dscr;
  }

  creg=(creg|TPU_IMBTST)&~TPU_MUXEL;		/*!!!!!!!!!!!*/
  tpu_reg_wr(TPUREG_CREG,0,creg);

  if(!(dscr&TPU_HOT4))  /* ???????? */
    tpu_reg_wr(TPUREG_DSCR,0,dscr|TPU_HOT4);

  tcr&=TPU_INCAD | TPU_TCR1C | TPU_TMW;		/* 0x1801 */
  tpu_reg_wr(TPUREG_TCR,0,tcr | val2mfld(TPU_ACUTR,TPU_ACUTR_STEPTPU) |
				val2mfld(TPU_SOSEL,sxsel)); /* (sxsel<<4)|0x200 */
  tpu_reg_wr(TPUREG_TSTSC,0,shiftcnt);

  test_shift_wait(creg);		/* test shift */

  ret=tpu_reg_rd(TPUREG_TSTMSRB,0)>>(0x10-shiftcnt);
  tpu_reg_wr(TPUREG_TCR,0,tcr | val2mfld(TPU_ACUTR,TPU_ACUTR_STEPTPU) |
				val2mfld(TPU_SISEL,sxsel)); /* (sxsel<<1)|0x200 */
  tpu_reg_wr(TPUREG_TSTSC,0,shiftcnt<<8);
  tpu_reg_wr(TPUREG_TSTMSRA,0,ret);

  test_shift_wait(creg);		/* test shift */

  if(!(dscr&TPU_HOT4))  /* ???????? */
    tpu_reg_wr(TPUREG_DSCR,0,dscr);

  return ret;
}

static		/* was test_shift_4 */
int test_shift_regwr(int shiftcnt, int sxsel, int val)
{
  u_int16_t dscr; /* di */
  u_int16_t creg; /* si */
  u_int16_t tcr;  /* [bp-6] */

  if (!use_fetched_reg){
    tpu_reg_wr(TPUREG_TSTMSRB,0,0);
    tpu_reg_wr(TPUREG_TSTRC,0,0);
    tpu_reg_wr(TPUREG_DREG,0,0);
    dscr=tpu_reg_rd(TPUREG_DSCR,0);
    creg=tpu_reg_rd(TPUREG_CREG,0);
    tcr=tpu_reg_rd(TPUREG_TCR,0);
  }else{
    creg=fetched_creg;
    tcr=fetched_tcr;
    dscr=fetched_dscr;
  }

  if(!(dscr&TPU_HOT4))  /* ???????? */
    tpu_reg_wr(TPUREG_DSCR,0,dscr|TPU_HOT4);

  creg=(creg|TPU_IMBTST)&~TPU_MUXEL;		/*!!!!!!!!!!!*/
  tpu_reg_wr(TPUREG_CREG,0,creg);
  tcr&=TPU_INCAD | TPU_TCR1C | TPU_TMW;		/* 0x1801 */
  tpu_reg_wr(TPUREG_TCR,0,tcr | val2mfld(TPU_ACUTR,TPU_ACUTR_STEPTPU) |
				val2mfld(TPU_SISEL,sxsel)); /* (sxsel<<1)|0x200 */
  tpu_reg_wr(TPUREG_TSTSC,0,shiftcnt<<8);
  tpu_reg_wr(TPUREG_TSTMSRA,0,val);

  test_shift_wait(creg);		/* test shift */

  if(!(dscr&TPU_HOT4))  /* ???????? */
    tpu_reg_wr(TPUREG_DSCR,0,dscr);

  return 0;
}

static		/* was test_pha_2 */
void test_forced_inst(u_int32_t new_ir)
{
  test_shift_irwr(new_ir);
  tpu_reg_or(TPUREG_CREG,0,TPU_ACUT);
}

static		/* was test_pha_1 */
void test_store_ir(void)
{
  stored_debpar2=tpu_reg_rd(TPUREG_DEBPAR,0);
  stored_pc=tpu_reg_rd(TPUREG_PC,0);
  stored_ir=test_shift_irrd();
  test_forced_inst(0x3ffffc03);
  stored_diob=tpu_reg_rd(TPUREG_DEBPAR,0);
}

static		/* was test_pha_3 */
void test_restore_ir(void)
{
  tpu_reg_wr(TPUREG_DEBPAR,0,stored_diob);
  test_forced_inst(0x1ffffc03);
  tpu_reg_wr(TPUREG_DEBPAR,0,stored_debpar2);
  tpu_reg_wr(TPUREG_PC,0,stored_pc);
  test_shift_irwr(stored_ir);
}

/**************************************************************************/
/* Other supporting routines */

static u_int16_t test_dssrdscr_1;	/* 0 => stop, 1 =>run */
static u_int16_t test_dscr_arm;
static u_int16_t test_dssr_bkpt;

int test_init_1(void)
{
  u_int16_t creg; /* bp-2 */
  int cnt=0x1000;

  tpu_reg_wr(TPUREG_CREG,0,TPU_MUXEL|TPU_EMT);
  creg=tpu_reg_rd(TPUREG_CREG,0);
  do{
    creg=tpu_reg_rd(TPUREG_CREG,0);
  }while(!(creg&TPU_EMT)&&(--cnt));
  tpu_reg_wr(TPUREG_CIER,0,0);	/* disable interrupt generation */
  if(!cnt){
    fprintf(stderr,"test_init_1 : cannot enter test mode\n");
    return -1;
  }
  return 0;
}

int test_init_2(void)
{
  u_int16_t dssr; /* si */
  u_int16_t dscr; /* bx */
  dssr=tpu_reg_rd(TPUREG_DSSR,0);
  dscr=tpu_reg_rd(TPUREG_DSCR,0);
  if((dssr&TPU_BKPT)||(dscr&TPU_HOT4))
    test_dssrdscr_1=0;
  else
    test_dssrdscr_1=1;
  test_dscr_arm=dscr&TPU_BMSK;
  if (dssr&TPU_BKPT)
    test_dssr_bkpt=1;
  else
    test_dssr_bkpt=0;
  return 0;
}

int tpu_halt(void)
{
  int i;
  if(test_dssrdscr_1==0){
    /* already halted */
    /* return 0; */
  }
  tpu_reg_or(TPUREG_CREG,0,TPU_IMBTST);	/* stop microengine */
  tpu_reg_wr(TPUREG_DSCR,0,TPU_HOT4|TPU_CLKS|test_dscr_arm);
  test_dssrdscr_1=0;
  /* old_cpr = CPR0,CPR1 */
  /* CPR0,CPR1 = 0 */
  for(i=0;i<5;i++){
    if(tpu_reg_rd(TPUREG_DSSR,0)&TPU_BKPT)
      tpu_reg_and(TPUREG_DSSR,0,~TPU_BKPT);
    tpu_reg_or(TPUREG_CREG,0,TPU_IMBTST|TPU_ACUT);
  }
  /* CPR0,CPR1 = old_cpr */

  return 0;
}

#define TPU_UCODE_LEN 0x200

u_int32_t tpu_ucode_img[TPU_UCODE_LEN];

int tpu_ucode_read(int print)
{
  int adr;
  u_int32_t ir;
  adr=0;
  
  test_store_ir();  

  if(print)printf("reading microcode\n");
  tpu_reg_wr(TPUREG_PC,0,adr);
    for(;adr<TPU_UCODE_LEN;adr++){  
    test_shift_irwr(0xffffffff);
    tpu_reg_or(TPUREG_CREG,0,TPU_ACUT);
    ir=test_shift_irrd();
    tpu_ucode_img[adr]=ir;
    if(print)printf("  %03X: %08lX\n",adr,(unsigned long)ir);
  }

  test_restore_ir();
  return 1;
}


/**************************************************************************/
/* Register read and write routines */

/* read TPU register - method 1 */
int tpu_t1_rd(struct tpu_reg_des *des,  int indx)
{
  unsigned val;
  test_store_ir();
  if(des->flags&TPUREG_FL_DIOB){
    val=stored_diob;			/* DIOB read */
  }else{
    if(des->flags&TPUREG_FL_PMOD)
      test_forced_inst(0xf403|(des->adr<<16));	/* P read */
    else
      test_forced_inst(0xfc03|(des->adr<<16));	/* rest */
    /* read value from mem 0xFFFF00 */
    val=tpu_reg_rd(TPUREG_DEBPAR,0);
  }
  test_restore_ir();
  if(des->flags&TPUREG_FL_SH4)
    val>>=4;
  return val;
}

/* write TPU register - method 1 */
int tpu_t1_wr(struct tpu_reg_des *des,  int indx, int val)
{
  int ret;
  test_store_ir();
  if(des->flags&TPUREG_FL_SH4)
    val<<=4;
  ret=tpu_reg_wr(TPUREG_DEBPAR,0,val);
  test_forced_inst(0x1ffffc03);
  if(des->flags&TPUREG_FL_DIOB){
    stored_diob=val;			/* DIOB write */
  }else{
    test_forced_inst(0xffff|(des->adr1<<16));	/* rest */
  }
  test_restore_ir();
  return 0;
}

/* read TPU register - method 2 */
int tpu_t2_rd(struct tpu_reg_des *des,  int indx)
{
  return test_shift_regrd(des->adr,des->adr1);
}

/* write TPU register - method 2 */
int tpu_t2_wr(struct tpu_reg_des *des,  int indx, int val)
{
  return test_shift_regwr(des->adr,des->adr1,val);
}

/* read 16 bit variable from main CPU address space */
int tpu_mem_rd16(struct tpu_reg_des *des,  int indx)
{
  u_int16_t val;
  int adr=des->adr;
  if(indx){
    if(indx>=des->arr_len) return -1;
    adr+=2*indx;
  }
  val=TPU_RD16((caddr_t)adr);
  return val;

  mem_op_error:
    fprintf(stderr,"tpu_mem_rd16 : read from address %06x error\n",adr);
    fflush(NULL);
    return -1;
}

/* write 16 bit variable to main CPU address space */
int tpu_mem_wr16(struct tpu_reg_des *des,  int indx, int val)
{
  int adr=des->adr;
  if(indx){
    if(indx>=des->arr_len) return -1;
    adr+=2*indx;
  }
  TPU_WR16((caddr_t)adr,val);
  return 0;

  mem_op_error:
    fprintf(stderr,"tpu_mem_wr16 : write to address %06x error\n",adr);
    fflush(NULL);
    return -1;
}

/* Read TPU register */
int tpu_reg_rd(int reg, int indx)
{
  if((reg>TPUREG_MAX)||(tpu_regs[reg].reg_rd==NULL))
    return -1;
  return tpu_regs[reg].reg_rd(&tpu_regs[reg],indx);
}

/* Write TPU register */
int tpu_reg_wr(int reg, int indx, int val)
{
  if((reg>TPUREG_MAX)||(tpu_regs[reg].reg_wr==NULL))
    return -1;
  return tpu_regs[reg].reg_wr(&tpu_regs[reg],indx,val);
}

int tpu_reg_or(int reg, int indx, int val)
{
  int reg_val;
  reg_val=tpu_reg_rd(reg,indx);
  reg_val|=val;
  tpu_reg_wr(reg,indx,reg_val);
  return reg_val;
}

int tpu_reg_and(int reg, int indx, int val)
{
  int reg_val;
  reg_val=tpu_reg_rd(reg,indx);
  reg_val&=val;
  tpu_reg_wr(reg,indx,reg_val);
  return reg_val;
}

/**************************************************************************/

#define swap_l(x) (x>>24) | ((x>>8)&0xff00) | ((x<<8)&0xff0000) | ((x&0xff)<<24)

/* external TPU disassembler */
void DisInst (unsigned long i, FILE * fp);


int cpu_stat(void)
{
  int ret;
  u_int rpc;

  ret=bdmlib_getstatus();
  printf("MCU ");
  if(ret&BDM_TARGETRESET)   printf("Reset ");
  if(ret&BDM_TARGETSTOPPED) printf("Stopped ");
  if(ret&BDM_TARGETPOWER)   printf("NoPower ");
  if(ret&BDM_TARGETNC)      printf("NoConnect ");
  printf("\n");
  if(ret&BDM_TARGETSTOPPED) {
    if((ret=bdmlib_get_sys_reg(BDM_REG_RPC, &rpc))<0) return ret;
    printf("RPC=0x%06x\n",swap_l(rpc));
  }
  return 0;
}

int main(int argc, char *argv[])
{
  int ret;

  bdmlib_setdebug(1);
  if((ret=bdmlib_open(bdm_dev_name))<0)
    { printf("bdmlib_open : %s\n",bdmlib_geterror_str(ret)); return 1; };
  bdmlib_setioctl(BDM_SPEED,0);
  if (!(bdmlib_getstatus() & BDM_TARGETSTOPPED))
    ret=bdmlib_ioctl(BDM_STOP_CHIP);

  if(1){
    printf("We need to reset target for first run\n");
    if((ret=bdmlib_reset())<0){
      printf("Cannot reset target - exitting\n");
      exit(1);
    }        
  }
  
  if(cpu_stat()<0){
    printf("trying to reset !!!!!!!\n");
    ret=bdmlib_reset();
    cpu_stat();
  }

  bdmlib_set_sys_reg(BDM_REG_DFC, 5);
  bdmlib_set_sys_reg(BDM_REG_SFC, 5);

  ret=test_init_1();
  printf("test_init_1 returned %d\n",ret);
  ret=test_init_2();
  printf("test_init_2 returned %d\n",ret);
  printf("test_dssrdscr_1(run)=%d test_dscr_arm=0x%02x test_dssr_bkpt=%d\n",
         test_dssrdscr_1,test_dscr_arm,test_dssr_bkpt);

  fflush(NULL);

//  tpu_halt();

  printf("DCNR %04X\n",tpu_reg_rd(TPUREG_DCNR,0));
  printf("DSCR %04X\n",tpu_reg_rd(TPUREG_DSCR,0));

//  tpu_reg_or(TPUREG_CREG,0,TPU_IMBTST);	/* stop microengine */
//  tpu_reg_or(TPUREG_DSCR,0,0x8200);	/* ???????????? */
  
  printf("P    %04X DIOB %04X A    %04X SR   %04X DEC  %01X\n"
  	 "ERT  %04X TCR1 %04X TCR2 %04X PC   %03X  CHAN %01X\n",
          tpu_reg_rd(TPUREG_P,0),tpu_reg_rd(TPUREG_DIOB,0),
          tpu_reg_rd(TPUREG_A,0),tpu_reg_rd(TPUREG_SR,0),
          tpu_reg_rd(TPUREG_DEC,0),tpu_reg_rd(TPUREG_ERT,0),
          tpu_reg_rd(TPUREG_TCR1,0),tpu_reg_rd(TPUREG_TCR2,0),
          tpu_reg_rd(TPUREG_PC,0),tpu_reg_rd(TPUREG_CHAN,0));

  if(0){
    printf("TCR2 %04X\n",tpu_reg_rd(TPUREG_TCR2,0));
    printf("TCR1 %04X\n",tpu_reg_rd(TPUREG_TCR1,0));
    printf("TCR2 %04X\n",tpu_reg_rd(TPUREG_TCR2,0));
  
    tpu_reg_wr(TPUREG_P   ,0,0xABCD);tpu_reg_wr(TPUREG_DIOB,0,0xEF01);
    tpu_reg_wr(TPUREG_A   ,0,0x2345);tpu_reg_wr(TPUREG_SR  ,0,0x6789);
    tpu_reg_wr(TPUREG_DEC ,0,   0xA);tpu_reg_wr(TPUREG_ERT ,0,0xBCDE);
    tpu_reg_wr(TPUREG_TCR1,0,0xF012);tpu_reg_wr(TPUREG_TCR2,0,0x3456);
    tpu_reg_wr(TPUREG_PC  ,0, 0x789);tpu_reg_wr(TPUREG_CHAN,0,   0xB);
  }

  {
    int i;
    tpu_ucode_read(0);
    
    for(i=0;i<TPU_UCODE_LEN;i++){
      printf("%03x: %08x ",i,tpu_ucode_img[i]);
      DisInst (tpu_ucode_img[i],stdout);
    }
    
  }
  

  return 0;
}
