/* 
 * $Id: bdm.c,v 1.5 2006/01/07 22:47:50 ppisa Exp $
 *
 * Linux Device Driver BDM Interface
 * based on the PD driver package by Scott Howard, Feb 93
 * PD version ported to Linux by M.Schraut, Feb 95
 * enhancements from W. Eric Norum (eric@skatter.usask.ca), who did 
 *   a Next-Step-Port in Jun 95
 * tested for kernel version 1.3.57
 * (C) 1995, 1996 Technische Universitaet Muenchen, L. f. Prozessrechner
 * (C) 1997 Gunter Magin, Muenchen
 *
 * Modified by Pavel Pisa  pisa@cmp.felk.cvut.cz 1997 - 2003
 * tested for kernel version 2.2.x and 2.4.x with ICD and PD cables
 * new k_compat.h for kernels up to 2.4.0
 *
 * 2003/04/27 modified by Juergen Eder <Juergen.Eder@gmx.de>:
 *            Compatible with parport driver
 *            define in makefile:  -DWITH_PARPORT_SUPPORT
 *

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
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

 */

#undef REALLY_SLOW_IO

#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,60)
#include <linux/devfs_fs_kernel.h>
#else /* 2.5.60 */
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif /* MODVERSIONS */
#endif /* 2.5.60 */
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)  // fixme ?
  #include <linux/slab.h> /* instead of malloc.h */
#else
  #include <linux/malloc.h>
#endif
#include <linux/sys.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/delay.h>
#ifdef WITH_PARPORT_SUPPORT
#include <linux/parport.h>
#endif
#include <asm/segment.h>
#include <asm/system.h>
#include <asm/io.h>
#include "bdm.h"
#include "k_compat.h"

#if (LINUX_VERSION_CODE >= VERSION(2,4,0))
  #define BDM_WITH_DEVFS    
  #include <linux/miscdevice.h>
  kc_devfs_handle_t bdm_devfs_handle=NULL;
#endif

#define BDM_DEVFS_DIR_NAME "m683xx-bdm"

MODULE_SUPPORTED_DEVICE("m683xx-bdm");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Motorola m683xx BDM debugging interface");

#define	BDM_DRIVER_VERSION	4

#if !defined PD_INTERFACE && !defined ICD_INTERFACE
#error	You have to decide for the PD interface or the ICD interface
#error	Check the Makefile and uncomment the right line
#endif

#if !defined ICD_INTERFACE && defined EFIICDISP
#error  It makes no sense to enable ISP programming support in a driver not
#error  for ICD compatible devices
#error  Check the Makefile and either disable EFIICDISP or enable ICD_INTERFACE
#endif

#define BYTECOUNT_FOR_BEING_NICE 64
				/* no of bytes to transfer before giving
				 * other tasks opportunity being scheduled */
#define BDM_WAITCNT    0xffff	/* no of loops to wait for response   */
#define BITS           15	/* shift 16bit into 32 bit by 15      */
				/* -.1.Bit always 0 (Statusbit)       */

/* declaration of local variables */
#define BDM_DLY_DEFAULT	30

#define	BDM_LONG_DLY(d) (2*d/3 == 0) ? 1 : (2*d/3)
#define BDM_HALF_DLY(d) (d/2 == 0) ? 1 : (d/2)
#define BDM_SHORT_DLY(d) (d/3 == 0) ? 1 : (d/3)
#define BDM_MIN_DLY(d)  (d/30 == 0) ? 1 : (d/30)
static int bdm_norm_dly  = BDM_DLY_DEFAULT;
static int bdm_long_dly  = BDM_LONG_DLY(BDM_DLY_DEFAULT);
static int bdm_half_dly  = BDM_HALF_DLY(BDM_DLY_DEFAULT);
static int bdm_short_dly = BDM_SHORT_DLY(BDM_DLY_DEFAULT);
static int bdm_min_dly   = BDM_MIN_DLY(BDM_DLY_DEFAULT);
static int bdm_reset_dly = 0;
static int bdm_no_ser_dly_mode = 0;	/* mode for maximal speed */
#define SER_CLK_DSO_WAIT 0x100		/* in fast mode, delay only before */
					/* explicit serial transfers */

static unsigned int bdm_debug_level = BDM_DEBUG_NONE;
static int bdm_version = BDM_DRIVER_VERSION;
static int bdm_sense = 1;		/* do sense power & connected */

typedef unsigned int status_bitset;

#define BDM_DESCR_MAGIC 0x42444daa

typedef struct _bdm_descr_t bdm_descr_t;
struct _bdm_descr_t {
	unsigned int	magic;
  #ifndef WITH_PARPORT_SUPPORT
	unsigned int 	port_adr;
  #else /* WITH_PARPORT_SUPPORT */
	unsigned int 	parport_nr;
  #endif /* WITH_PARPORT_SUPPORT */
	unsigned int 	minor;
	status_bitset 	(*init) (bdm_descr_t *);
	int			 	(*deinit) (bdm_descr_t *);
	status_bitset 	(*getstatus) (bdm_descr_t *);
	int 			(*restart_chip) (bdm_descr_t *);
	int 			(*release_chip) (bdm_descr_t *);
	int 			(*stop_chip) (bdm_descr_t *);
	int 			(*step_chip) (bdm_descr_t *);
	int 			(*reset_chip) (bdm_descr_t *);
	int				(*ser_clk) (bdm_descr_t *, short, int);
	int				(*set_led) (bdm_descr_t *, int);
	status_bitset	(*resynchro) (bdm_descr_t *);
	char			*name;
	unsigned int	old_ctl;	/* save old control port settings     */
	unsigned int	shadow_ctl;	/* shadow reg. of control port for read */
};

/* generic helper functions */

#ifndef WITH_PARPORT_SUPPORT

#define PORT_ADDRESSE_LP1 0x378   /* LPT1 */
#define PORT_ADDRESSE_LP2 0x278   /* LPT2 */
#define PORT_ADDRESSE_LP3 0x3bc   /* LPT3 */

#define	BDM_DATA(d)	(d->port_adr)
#define	BDM_STAT(d)	(d->port_adr + 1)
#define	BDM_CTRL(d)	(d->port_adr + 2)

static unsigned char 
bdm_in_ctl(bdm_descr_t * descr)
{                      
	return inb(BDM_CTRL(descr));
}                                                       

static void 
bdm_out_ctl(int data, bdm_descr_t * descr)
{                                               
	outb(data,BDM_CTRL(descr));
	descr->shadow_ctl=data;
}                                                       

static unsigned char 
bdm_in_st(bdm_descr_t * descr)
{                                               
	return inb(BDM_STAT(descr));
}                                                       

static void
bdm_out_data(int data, bdm_descr_t * descr)
{                                               
	outb(data,BDM_DATA(descr));
}                                                       

static unsigned char
bdm_in_data(bdm_descr_t * descr)
{                                               
	return inb(BDM_DATA(descr));
}                                                       

#else /* WITH_PARPORT_SUPPORT */

#define PORT_ADDRESSE_LP1 0
#define PORT_ADDRESSE_LP2 1
#define PORT_ADDRESSE_LP3 2

#define MAX_BDM_PORTS 3
static struct pardevice *bdm_parports[MAX_BDM_PORTS];

static struct pardevice *
bdm_get_parport(bdm_descr_t * descr)
{
	if((unsigned)descr->parport_nr>=MAX_BDM_PORTS)
		return NULL;
	return bdm_parports[descr->parport_nr];
}

/*static int
bdm_get_parport_num(bdm_descr_t * descr)
{
	return descr->parport_nr;
}*/

static void 
bdm_out_ctl(int data, bdm_descr_t * descr)
{                                               
	struct pardevice *p;
	p = bdm_get_parport(descr);
	if(!p)
		return;
  #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	parport_write_control(p->port,data & ~0x20);
	if(data & 0x20)
		parport_data_reverse(p->port);
	else
		parport_data_forward(p->port);
  #else
	parport_write_control(p->port,data);
  #endif
	descr->shadow_ctl=data;
}                                                       

static unsigned char 
bdm_in_ctl(bdm_descr_t * descr)
{                      
	struct pardevice *p;
	p = bdm_get_parport(descr);
	if(!p)
		return 0;
  #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	/* There is no 'parport_read_control()' function -> use my own variable */
	return descr->shadow_ctl;
  #else
	return parport_read_control(p->port);
  #endif
}                                                       

static unsigned char 
bdm_in_st(bdm_descr_t * descr)
{                                               
	struct pardevice *p;
	p = bdm_get_parport(descr);
	if(!p)
		return 0;
	return parport_read_status(p->port);
}                                                       

static void
bdm_out_data(int data, bdm_descr_t * descr)
{                                               
	struct pardevice *p;
	p = bdm_get_parport(descr);
	if(!p)
		return;
	parport_write_data(p->port,data);
}                                                       

static unsigned char
bdm_in_data(bdm_descr_t * descr)
{                                               
	struct pardevice *p;
	p = bdm_get_parport(descr);
	if(!p)
		return 0;
	return parport_read_data(p->port);
}                                                       

#endif /* WITH_PARPORT_SUPPORT */

/* slow down host for not overrunning target */
static void
bdm_delay(int counter)
{

	if (counter < (1000000/HZ)) {
		while (counter > 1000) {
			udelay(1000); counter -= 1000;
		}
		udelay(counter);
	} else {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(counter/(1000000/HZ));
	}
#if NEVER
	while (counter--) {
		asm volatile ("nop");
	}
#endif /* NEVER */
}


static void
bdm_error(int bdm_type)
{
	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		switch (bdm_type) {
		  case -BDM_FAULT_UNKNOWN:
			  printk("Unknown Error - check speed\n");
			  break;
		  case -BDM_FAULT_POWER:
			  printk("Power failed on Target MCU\n");
			  break;
		  case -BDM_FAULT_CABLE:
			  printk("Cable disconnected on Target MCU\n");
			  break;
		  case -BDM_FAULT_RESPONSE:
			  printk("No response from Target MCU\n");
			  break;
		  case -BDM_FAULT_RESET:
			  printk("Can t clock Target MCU while on Reset\n");
			  break;
		  case -BDM_FAULT_PORT:
			  printk("Wrong Port\n");
			  break;
		  case -BDM_FAULT_BERR:
			  printk("Buserror\n");
			  break;
		}
	}
}

#define	do_case(x) case x: ret = #x; break;

static char * 
bdm_get_ioctl_name(int ioctl)
{
char *ret;
	switch (ioctl) {
		do_case(BDM_INIT);
		do_case(BDM_DEINIT);
		do_case(BDM_RESET_CHIP);
		do_case(BDM_RESTART_CHIP);
		do_case(BDM_STOP_CHIP);
		do_case(BDM_STEP_CHIP);
		do_case(BDM_GET_STATUS);
		do_case(BDM_SPEED);
		do_case(BDM_RELEASE_CHIP);
		do_case(BDM_DEBUG_LEVEL);
		do_case(BDM_GET_VERSION);
		do_case(BDM_SENSECABLE);
		do_case(BDM_RESPW);
#ifdef EFIICDISP
		do_case(BDM_ISPSET);
		do_case(BDM_ISPGET);
#endif 
		default:
			ret = "Unknown ioctl";
	}
	return ret;
}


#ifdef PD_INTERFACE

/*
 * Parallel port bit assignments for the PD interface (minor-numbers 0-2)
 *
 * Status register (bits 0-2 not used):
 * +---+---+---+---+---+---+---+---+
 * | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * +---+---+---+---+---+---+---+---+
 *   |   |   |   |   |
 *   |   |   |   |   +--- Target FREEZE line
 *   |   |   |   |           1 - Target is in background mode
 *   |   |   |   |           0 - Target is not background mode
 *   |   |   |   |
 *   |   |   |   +------- Not used
 *   |   |   |
 *   |   |   +----------- Serial data from target
 *   |   |                    1 - `0' from target
 *   |   |                    0 - `1' from target
 *   |   |
 *   |   +--------------- Target power
 *   |                        1 - Target power is ON
 *   |                        0 - Target power is OFF
 *   |
 *   +------------------- Target connected
 *                            1 - Target is connected
 *                            0 - Target is not connected
 *
 * Control register (bits 4-7 not used):
 * +---+---+---+---+
 * | 3 | 2 | 1 | 0 |
 * +---+---+---+---+
 *   |   |   |   |
 *   |   |   |   +--- Target BKPT* /DSCLK line
 *   |   |   |           Write 1 - Drive BKPT* /DSCLK line LOW
 *   |   |   |           Write 0 - Drive BKPT* /DSCLK line HIGH
 *   |   |   |
 *   |   |   +------- Target RESET* line
 *   |   |               Write 1 - Force RESET* LOW
 *   |   |               Write 0 - Allow monitoring of RESET*
 *   |   |                         Read 1 - RESET* is LOW
 *   |   |                         Read 0 - RESET* is HIGH
 *   |   |
 *   |   +----------- Serial data to target
 *   |                    Write 0 - Send `0' to target
 *   |                    Write 1 - Send `1' to target
 *   |
 *   +--------------- Control single-step flip-flop
 *                        Write 1 - Clear flip-flop
 *                                  BKPT* /DSCLK is controlled by bit 0.
 *                        Write 0 - Allow flip-flop operation
 *                                  Falling edge of IFETCH* /DSI clocks a `1'
 *                                  into the flip-flop and drive BKPT* /DSCLK
 *                                  LOW, causing a breakpoint.
 */

#define BDMPD_DSCLK      1		/* data shift clock / breakpoint pin  */
#define BDMPD_RST_OUT    2		/* set high to force reset on MCU     */
#define BDMPD_DSI        4		/* data shift input  Host->MCU        */
#define BDMPD_STEP_OUT   8		/* set low to gate IFETCH onto BKPT   */

#define BDMPD_FREEZE     8		/* FREEZE asserted when MCU stopped   */
#define BDMPD_DSO        0x20	/* data shift output MCU-.Host        */
#define BDMPD_PWR_DWN    0x40	/* power down - low when Vcc failed   */
#define BDMPD_NC         0x80	/* not connected - low when unplugged */


static status_bitset
bdm_pd_init(bdm_descr_t *descr)
{
	status_bitset status;
	bdm_out_ctl(BDMPD_STEP_OUT | BDMPD_DSCLK, descr);	
										/* force breakpoint         */
	bdm_delay(bdm_min_dly);
	status = descr->getstatus(descr);		/* eg connected, power etc. */
	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_pd_init minor %d status %d\n", 
				descr->minor, status);
	}
	return status;
}

static int
bdm_pd_deinit(bdm_descr_t * descr)
{
#if LP_PORT_RESTORE
	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_deinit: restoring %#x\n", descr->old_ctl);
	}
	bdm_out_ctl(descr->old_ctl, descr);	
			/* restore old control port settings */
#endif /* LP_PORT_RESTORE */
	return BDM_FAULT_NOERROR;
}

/* read status byte from statusport and translate into bitset */
static status_bitset
bdm_pd_getstatus(bdm_descr_t *descr)
{
	unsigned char temp = bdm_in_st(descr);

	if (bdm_debug_level >= BDM_DEBUG_ALL) {
		printk("bdm_pd_getstatus minor=%d res=%#x\n", descr->minor, temp);	 
	}
	if (bdm_sense) {
		if (!(temp & BDMPD_NC))
			return BDM_TARGETNC;
		if (!(temp & BDMPD_PWR_DWN))
			return BDM_TARGETPOWER;
	}
	return (temp & BDMPD_FREEZE ? BDM_TARGETSTOPPED : 0) |
	(bdm_in_ctl(descr) & BDMPD_RST_OUT ? BDM_TARGETRESET : 0);
}

/* this routine is last resort one, it is able resyncronize with MCU,
   when some spike on DSCLK has disturbed MCU */
static status_bitset
bdm_pd_resynchro(bdm_descr_t *descr)
{
	int i;
	descr->ser_clk(descr, BDM_NOP_CMD, 0); /* send NOP command */
	descr->ser_clk(descr, BDM_NOP_CMD, 0); /* send NOP command */
	descr->ser_clk(descr, BDM_NOP_CMD, 0); /* send NOP command */
	i=17;	/* maximal number disturb in bit shift */
	while(i--)
	{
		if((bdm_in_st(descr) & BDMPD_DSO))
			return BDM_FAULT_NOERROR;	/* DSO becomes 0 => OK */
		bdm_out_ctl( BDMPD_STEP_OUT | BDMPD_DSCLK, descr); /* DSCLK=0 */
		bdm_delay(bdm_norm_dly);
		bdm_out_ctl( BDMPD_STEP_OUT , descr);	/* DSCLK=1 */
		bdm_delay(bdm_norm_dly);
	}
	return -BDM_FAULT_UNKNOWN;
}

/* restart chip and stop on first execution fetch */
static int
bdm_pd_restart_chip(bdm_descr_t * descr)
{
	unsigned bdm_Loop_Count = 0;

	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_pd_restart_chip minor=%d\n", descr->minor);
	}

	bdm_out_ctl(BDMPD_DSCLK, descr);
	bdm_out_ctl(BDMPD_RST_OUT | BDMPD_DSCLK, descr);
	bdm_delay(bdm_norm_dly+bdm_reset_dly);
	bdm_out_ctl(BDMPD_DSCLK, descr);
	bdm_delay(bdm_norm_dly);
	bdm_out_ctl(BDMPD_STEP_OUT | BDMPD_DSCLK, descr);
	for (bdm_Loop_Count = 0xffff; bdm_Loop_Count; bdm_Loop_Count--)
		if (bdm_in_st(descr) & BDMPD_FREEZE)
			break;
	if (!bdm_Loop_Count) {
		bdm_error(-BDM_FAULT_RESPONSE);
		return -BDM_FAULT_RESPONSE;
	} else {
		return BDM_FAULT_NOERROR;
	}

}


/* stop running target */
static int
bdm_pd_stop_chip(bdm_descr_t * descr)
{
	unsigned int bdm_ctr;
	int return_value;

	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_pd_stop_chip minor=%d\n", descr->minor);
	}

	if (bdm_in_st(descr) & BDMPD_FREEZE) {
		if (bdm_debug_level >= BDM_DEBUG_SOME) {
			printk("bdm_pd_stop_chip: already frozen..\n");
		};
		/* ensures no spike on BKPT/DSCLK in bdm_ser_clk */
		bdm_out_ctl(BDMPD_DSCLK, descr);
		bdm_delay(bdm_short_dly);
		bdm_out_ctl(BDMPD_DSCLK | BDMPD_STEP_OUT, descr);
		return BDM_FAULT_NOERROR;	/* target was already halted */
	}

	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_pd_stop_chip: was not frozen..\n");
	}
	return_value = -BDM_FAULT_UNKNOWN;

	/* bdm_out_ctl(BDMPD_DSCLK | BDMPD_STEP_OUT, descr); */
	bdm_out_ctl(BDMPD_DSCLK*0, descr);

	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_pd_stop_chip: armed stop\n");
	}
	for (bdm_ctr = BDM_WAITCNT; bdm_ctr; bdm_ctr--) {
		if ((bdm_in_st(descr)) & BDMPD_FREEZE) { 
			return_value = BDM_FAULT_NOERROR;	/* target is now halted      */
			break;
		}
		bdm_delay(bdm_min_dly);
	}

	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_pd_stop_chip: after wait-for-freeze: cntr = %d\n",
				BDM_WAITCNT-bdm_ctr);
	}
	bdm_out_ctl(BDMPD_DSCLK, descr); 
	bdm_delay(bdm_norm_dly);
	bdm_out_ctl(BDMPD_DSCLK | BDMPD_STEP_OUT, descr);

	if (!bdm_ctr) {
		bdm_error(-BDM_FAULT_RESPONSE);
		return -BDM_FAULT_RESPONSE;
	}
    #ifdef BDM_TRY_RESYNCHRO
	descr->resynchro(descr); /* tries to find correct 17 bit boundary */
    #endif /* BDM_TRY_RESYNCHRO */
	return return_value;
}



/* single stepping target mcu */
static int
bdm_pd_step_chip(bdm_descr_t * descr)
{
	int tester;
	unsigned short DataOut = (BDM_GO_CMD & 1 ? BDMPD_DSI : 0);

	status_bitset bdm_stat = descr->getstatus(descr);

	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_pd_step_chip minor=%d\n", descr->minor);
	}

	if (bdm_stat & BDM_TARGETRESET) {
		bdm_error(-BDM_FAULT_RESET);
		return -BDM_FAULT_RESET;
	}
	if (bdm_stat & BDM_TARGETNC) {
		bdm_error(-BDM_FAULT_CABLE);
		return -BDM_FAULT_CABLE;
	}
	if (bdm_stat & BDM_TARGETPOWER) {
		bdm_error(-BDM_FAULT_POWER);
		return -BDM_FAULT_POWER;
	}
	tester = descr->ser_clk(descr, BDM_GO_CMD, 1);
	switch (tester) {
		case -BDM_FAULT_RESET:
		case -BDM_FAULT_CABLE:
		case -BDM_FAULT_POWER:
			return tester;
	}
	if (bdm_debug_level >= BDM_DEBUG_ALL) {
		printk("stepchip sent: %x got : %x\n", 
			BDM_GO_CMD, (unsigned int) tester);
		printk("last answerbit : %x\n", !(bdm_in_st(descr) & BDMPD_DSO));
	}
	/*
	 * this is the place the inb() should take place, but on step_chip, we
	 * don't care for the reply of CPU32, coming in when sending the cmd
	 */
	bdm_out_ctl(BDMPD_DSCLK | BDMPD_STEP_OUT | DataOut, descr);
	bdm_delay(bdm_norm_dly);
	bdm_out_ctl(BDMPD_DSCLK | DataOut, descr);
	bdm_delay(bdm_long_dly);	
		/* leave more time to avoid a glitch on DSCLK*/
	bdm_out_ctl(DataOut, descr);
	/* FREEZE goes low after */
	/* 10 CPU32 clock cycles, it is about 700ns @ 16 MHz */
	bdm_delay(bdm_long_dly);
  #ifdef BDM_TRY_RESYNCHRO
	descr->stop_chip(descr);
	return descr->resynchro(descr);
  #else  /* BDM_TRY_RESYNCHRO */
	return descr->stop_chip(descr);
  #endif /* BDM_TRY_RESYNCHRO */
}

/* reset target chip without asserting bkpt/!dsclk */
static int
bdm_pd_release_chip(bdm_descr_t * descr)
{
	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_pd_release_chip minor=%d\n", descr->minor);
	}

	bdm_out_ctl(BDMPD_DSCLK | BDMPD_STEP_OUT, descr);
	bdm_delay(bdm_short_dly);
	bdm_out_ctl(BDMPD_DSCLK | BDMPD_RST_OUT | BDMPD_STEP_OUT, descr);
	bdm_delay(bdm_norm_dly);
	bdm_out_ctl(BDMPD_STEP_OUT, descr);
	bdm_delay(bdm_short_dly);

	return BDM_FAULT_NOERROR;
}

/*
 * Restart chip, enable background debugging mode, halt on first fetch
 *
 * The software from the Motorola BBS tries to have the target
 * chip begin execution, but that doesn't work very reliably.
 * The RESETH* line rises rather slowly, so sometimes the BKPT* / DSCLK
 * would be seen low, and sometimes it wouldn't.
 */
static int
bdm_pd_reset_chip(bdm_descr_t * descr)
{
	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_pd_reset_chip minor=%d\n", descr->minor);
	}

#if NEVER
	/* this is the original Howard code */
	bdm_out_ctl(BDMPD_RST_OUT | BDMPD_STEP_OUT, descr);
	bdm_delay(bdm_norm_dly);
	bdm_out_ctl(BDMPD_STEP_OUT, descr);
#endif /* NEVER */


	/* this is suggested by Eric Norum */
	bdm_out_ctl(BDMPD_DSCLK | BDMPD_RST_OUT, descr);
	bdm_delay(bdm_norm_dly+bdm_reset_dly);
	bdm_out_ctl(BDMPD_DSCLK, descr);
	bdm_delay(bdm_norm_dly+bdm_reset_dly);
	bdm_out_ctl(BDMPD_DSCLK | BDMPD_STEP_OUT, descr);
	return BDM_FAULT_NOERROR;
}


/* serial software protokoll for bdm-pd-interface */
static int
bdm_pd_ser_clk(bdm_descr_t * descr, short bdm_value, int delay_bits)
{
	unsigned int ShiftRegister;
	unsigned char DataOut;
	unsigned counter;
	unsigned bdm_stat = descr->getstatus(descr);

	ShiftRegister = bdm_value;
	ShiftRegister <<= BITS;
	if (bdm_stat & BDM_TARGETRESET) {	/*error checking */
		bdm_error(-BDM_FAULT_RESET);
		return -BDM_FAULT_RESET;
	}
	if (bdm_stat & BDM_TARGETNC) {
		bdm_error(-BDM_FAULT_CABLE);
		return -BDM_FAULT_CABLE;
	}
	if (bdm_stat & BDM_TARGETPOWER) {
		bdm_error(-BDM_FAULT_POWER);
		return -BDM_FAULT_POWER;
	}
	if(!(bdm_stat & BDM_TARGETSTOPPED))
	{ 	if (bdm_debug_level >= BDM_DEBUG_SOME)
			printk("bdm_ser_clk stop target first minor=%d\n", descr->minor);
		if(descr->stop_chip(descr)!=BDM_FAULT_NOERROR)
		{	printk("bdm_ser_clk can't stop it minor=%d\n", descr->minor);
			return -BDM_FAULT_RESPONSE;
		}
	}

	/*
	 * We can get there with DSCLK high or low
	 * but next code is OK for both cases
	 * first ensures DSCLK low => it frozes transmit shift
	 * register against CPU32 updates until 17 bits are sent
	 * and then clocks transmit shift register by DSCLK high
	 */

	if (bdm_debug_level >= BDM_DEBUG_ALL) {
		printk("bdm_pd_ser_clk: writing %5x\n", bdm_value);
	}
	if(bdm_no_ser_dly_mode && (delay_bits&SER_CLK_DSO_WAIT))
	{	/* Wait for DSO to become 0 to indicate ready
		 * according CPU32UM, DSO is updated as long as
		 * DSCLK is high
		 */
		int wait_cnt;
		for(wait_cnt=0;wait_cnt<100;wait_cnt++)
		{	if (bdm_in_st(descr) & BDMPD_DSO)
				break;	/* DSO becomed zerro already */
			bdm_delay(bdm_min_dly);
		}
		if(wait_cnt && (bdm_debug_level >= BDM_DEBUG_SOME))
			printk("bdm_pd_ser_clk: ready after %d cycles\n",
				wait_cnt);
	}

	counter = 32 - BITS - (delay_bits&0xff);
	while (counter--) {
		DataOut = ((ShiftRegister & 0x80000000) ? BDMPD_DSI : 0);
		/*if 1 then dsi=1*/
		ShiftRegister <<= 1;	/*shift one bit left*/
		bdm_out_ctl(DataOut | BDMPD_STEP_OUT | BDMPD_DSCLK, descr);
		if(!bdm_no_ser_dly_mode)
			bdm_delay(bdm_half_dly);
		if (!(bdm_in_st(descr) & BDMPD_DSO)) 
 			ShiftRegister |= 1;	/*put 1 on 0.bit */
		bdm_out_ctl(DataOut | BDMPD_STEP_OUT, descr);
		if(!bdm_no_ser_dly_mode)
			bdm_delay(bdm_norm_dly);
	}
	if (bdm_debug_level >= BDM_DEBUG_ALL) {
		printk("bdm_pd_ser_clk: read %5x bits %d\n", 
			ShiftRegister, delay_bits);
	}
#ifdef FIXME
	if (!(delay_bits&0xff) && (ShiftRegister & 0x10000)) switch (ShiftRegister) {
		case 0x1ffff:	/* no valid command return */
			return -BDM_FAULT_NVC;
		case 0x10001:
			return -BDM_FAULT_BERR;
		case 0x10000:
			return BDM_NOTE_NOTREADY;
		default: ;
	}
#endif /* FIXME */
	return ShiftRegister;
}

/* switch LED if available */
static int
bdm_pd_setled(bdm_descr_t * descr, int switchstate)
{
	return 0;
}

#endif

#ifdef ICD_INTERFACE
/*
 * Parallel port bit assignments for the ICD interface (minor-numbers 4-6)
 *
 * Status register 
 * +---+---+---+---+---+---+---+---+
 * | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * +---+---+---+---+---+---+---+---+
 *   |   |   |   |   |
 *   |   |   |   |   +--- ICD detection
 *   |   |   |   |            must follow LED level
 *   |   |   |   |            
 *   |   |   |   +------- Power Detection
 *   |   |   |                1 - Power available
 *   |   |   |                0 - no Power
 *   |   |   |
 *   |   |   +----------- Jumper State
 *   |   |                    1 - Jumper closed
 *   |   |                    0 - Jumper open
 *   |   |
 *   |   +--------------- Target FREEZE line
 *   |                        1 - Target is in background mode
 *   |                        0 - Target is not background mode
 *   |
 *   +------------------- Serial data from target
 *                            1 - `0' from target
 *                            0 - `1' from target
 *        
 * Data register 
 * +---+---+---+---+---+---+---+---+
 * | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * +---+---+---+---+---+---+---+---+
 *   |   |   |   |   |   |   |   |
 *   |   |   |   |   |   |   |   +---  Serial data to target
 *   |   |   |   |   |   |   |            Write 1: Send 1 to target
 *   |   |   |   |   |   |   |            Write 0: Send 0 to target
 *   |   |   |   |   |   |   |            Signal gets to target, if OE is 1
 *   |   |   |   |   |   |   |            and target is in FREEZE mode
 *   |   |   |   |   |   |   |
 *   |   |   |   |   |   |   +-------  Clock 
 *   |   |   |   |   |   |                if target in freeze mode, then:
 *   |   |   |   |   |   |                Write 1: drive BKPT* /DSCLK 1
 *   |   |   |   |   |   |                Write 0: drive BKPT* /DSCLK 0
 *   |   |   |   |   |   |
 *   |   |   |   |   |   +-----------  BREAK
 *   |   |   |   |   |                    if target not in freeze mode, then:
 *   |   |   |   |   |                    Write 0: drive BKPT* /DSCLK 0
 *   |   |   |   |   |                    line determines single stepping
 *   |   |   |   |   |                    on leaving BGND mode:
 *   |   |   |   |   |                    Write 0: do single step
 *   |   |   |   |   |                    Write 1: continue normally
 *   |   |   |   |   |
 *   |   |   |   |   +---------------  RESET
 *   |   |   |   |                        Write 0: pull reset low
 *   |   |   |   |                        Write 1: release reset line
 *   |   |   |   |
 *   |   |   |   +--- OE
 *   |   |   |           Write 0 - DSI is tristated
 *   |   |   |           Write 1 - DSI pin is forced to level of serial data
 *   |   |   |
 *   |   |   +------- LED (not implemented everywhere..)
 *   |   |               Write 1 - turn on LED
 *   |   |               Write 0 - turn off LED
 *   |   |
 *   |   +----------- ERROR
 *   |                   Write 0 - BERR output is tristated
 *   |                   Write 1 - BERR is pulled low
 *   |
 *   +--------------- spare
 */

/* data register bit definitions */
#define BDMICD_DSI        0x01	/* data shift input  Host->MCU        */
#define BDMICD_DSCLK      0x02	/* data shift clock / breakpoint pin  */
#define BDMICD_STEP_OUT   0x04	/* set low to force breakpoint        */
#define BDMICD_RST_OUT    0x08	/* set low to force reset on MCU      */
#define BDMICD_OE         0x10	/* set to a 1 to enable DSI           */
#define BDMICD_LED        0x20	/* set to 1 to turn on LED */
#define BDMICD_FORCE_BERR 0x40	/* set to a 1 to force BERR on target */

/* status register bit definitions */
#define BDMICD_DSO        0x80	/* serial data from target */
#define BDMICD_FREEZE     0x40  /* 1 if target is frozen */
#define BDMICD_JMPR       0x20  /* 1 if jumper is set */
#define BDMICD_PWR        0x10  /* 1 if power available */
#define BDMICD_ICDDETECT  0x08  /* must follow LED */

/* control register bit definitions */
#define BDMICD_EFIDET     0x04  /* setting to 1 will pull LED feedback low */
#define BDMICD_DCR_DIR    0x20  /*  0 -> drive data buffers, 1 -> data port tristate */

static int bdm_icd_led_status;

/*
 * Jumper seems to be responsible for SW generated breakpoints according to
 * P&E's user manual
 *
 * we assume a Rev. A cable, so ignore jumper setting
 *
 * Power detection is instable: depends heavily on PP internal
 * pull-ups/downs to get low level when there is no power
 *
 */


static status_bitset
bdm_icd_init(bdm_descr_t * descr)
{
	status_bitset status;
	unsigned char temp1, temp2;

	bdm_icd_led_status = BDMICD_LED;

	/* ensure data outputs are driven for BPP port configuration */
	bdm_out_ctl(BDMICD_DCR_DIR*0,descr);

	/* perform icd check */
	bdm_out_data(BDMICD_STEP_OUT | BDMICD_DSCLK | BDMICD_RST_OUT, descr);	
	bdm_delay(bdm_min_dly);
	temp1 = bdm_in_st(descr);
	bdm_out_data(BDMICD_STEP_OUT | BDMICD_DSCLK | BDMICD_RST_OUT | 
	    bdm_icd_led_status, descr);	
	bdm_delay(bdm_min_dly);
	temp2 = bdm_in_st(descr);
	if ((temp1 ^ temp2) & BDMICD_ICDDETECT) {
		printk("bdm_icd_init: detected ICD interface\n");
	}

	/* force breakpoint         */
	bdm_delay(bdm_min_dly);
	status = descr->getstatus(descr);
	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_icd_init minor %d status %d\n", 
				descr->minor, status);
	}
	return status;
}

static int
bdm_icd_deinit(bdm_descr_t * descr)
{
#if LP_PORT_RESTORE
	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_icd_deinit: restoring %#x\n", descr->bdm_old_ctl);
	}
	bdm_out_ctl(descr->bdm_old_ctl, descr);	
			/* restore old control port settings */
#endif /* LP_PORT_RESTORE */
	return BDM_FAULT_NOERROR;
}

/* read status byte from statusport and translate into bitset */
static status_bitset
bdm_icd_getstatus(bdm_descr_t * descr)
{
	unsigned char temp = bdm_in_st(descr);

	if (bdm_debug_level > BDM_DEBUG_SOME) {
		printk("bdm_icd_getstatus minor=%d res=%#x\n", descr->minor, temp);	 
	}
	if (bdm_sense) {
		if (!(temp & BDMICD_PWR))
			return BDM_TARGETPOWER;
	}
	return (temp & BDMICD_FREEZE ? BDM_TARGETSTOPPED : 0);
}

/* this routine is last resort one, it is able resyncronize with MCU,
   when some spike on DSCLK has disturbed MCU */
static status_bitset
bdm_icd_resynchro(bdm_descr_t *descr)
{
	int i;
	descr->ser_clk(descr, BDM_NOP_CMD, 0); /* send NOP command */
	descr->ser_clk(descr, BDM_NOP_CMD, 0); /* send NOP command */
	descr->ser_clk(descr, BDM_NOP_CMD, 0); /* send NOP command */
	i=17;	/* maximal number disturb in bit shift */
	while(i--)
	{
		if((bdm_in_st(descr) & BDMICD_DSO))	{
			if (bdm_debug_level >= BDM_DEBUG_SOME) {
				printk("bdm_icd_resynchro minor=%d shift=%d\n",
					descr->minor,16-i);
			}			
			return BDM_FAULT_NOERROR; /* DSO becomes 0 => OK */
		}
		bdm_out_data(BDMICD_RST_OUT | BDMICD_OE | BDMICD_STEP_OUT,
			descr);		/* DSCLK=0 */
		bdm_delay(bdm_norm_dly);
		bdm_out_data(BDMICD_RST_OUT | BDMICD_OE | BDMICD_STEP_OUT |
			BDMICD_DSCLK, descr);	/* DSCLK=1 */
		bdm_delay(bdm_norm_dly);
	}
	if (bdm_debug_level >= BDM_DEBUG_SOME)
		printk("bdm_icd_resynchro failed\n");
	return -BDM_FAULT_UNKNOWN;
}

/* restart chip and stop on first execution fetch */
static int
bdm_icd_restart_chip(bdm_descr_t * descr)
{
	if (bdm_debug_level >= BDM_DEBUG_SOME) {	
		printk("bdm_icd_restart_chip minor=%d\n", descr->minor);
	}

   	bdm_out_data(BDMICD_DSCLK, descr);
	bdm_delay(bdm_norm_dly+bdm_reset_dly);
	return descr->stop_chip(descr);
}


/* stop running target */
static int
bdm_icd_stop_chip(bdm_descr_t * descr)
{
	unsigned int bdm_ctr;
	int return_value;

	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_icd_stop_chip minor=%d\n", descr->minor);
	}

	if ((bdm_in_st(descr)) & BDMICD_FREEZE)
	{ /* already frozen, ensure no spikes on BKPT/DSCLK in bdm_ser_clk  */
		bdm_out_data(BDMICD_RST_OUT, descr);
		bdm_delay(bdm_short_dly);
		bdm_out_data(BDMICD_STEP_OUT | BDMICD_RST_OUT, descr);
		return BDM_FAULT_NOERROR;
	}

	return_value = -BDM_FAULT_UNKNOWN;

	bdm_delay(bdm_min_dly+100);
#if 1	/* stop imediately */
	bdm_out_data(BDMICD_RST_OUT, descr);
#else	/* stop after this instruction  */
	bdm_out_data(BDMICD_DSCLK | BDMICD_RST_OUT, descr);
#endif
	bdm_delay(bdm_min_dly+300);



	for (bdm_ctr = BDM_WAITCNT; bdm_ctr; bdm_ctr--) {
		if ((bdm_in_st(descr)) & BDMICD_FREEZE) { 
			return_value = BDM_FAULT_NOERROR;	/* target is now halted      */
			break;
		}
		bdm_delay(bdm_min_dly);
	}

#ifdef DEB_TIM
printk("bdm_icd_stop_chip: having waited for FREEZE %d loops\n", 
	BDM_WAITCNT-bdm_ctr);
#endif

	if (!bdm_ctr) {
#if 1
		bdm_out_data(BDMICD_RST_OUT | BDMICD_FORCE_BERR, descr);
#else
		bdm_out_data(BDMICD_DSCLK | BDMICD_RST_OUT | BDMICD_FORCE_BERR, descr);
#endif		
 		bdm_delay(bdm_min_dly);
 		for (bdm_ctr = BDM_WAITCNT; bdm_ctr; bdm_ctr--) {
 			if ((bdm_in_st(descr)) & BDMICD_FREEZE) {
 				return_value = BDM_FAULT_NOERROR;	/* target is now halted      */
 				break;
 			}
 			bdm_delay(bdm_min_dly);
 		}
#ifdef DEB_TIM
printk("bdm_icd_stop_chip: berr has been forced; wait time was: %d\n", 
	BDM_WAITCNT-bdm_ctr);
#endif
 	}

#if 1
	bdm_out_data(BDMICD_RST_OUT, descr);
	bdm_delay(bdm_short_dly);
	bdm_out_data(BDMICD_RST_OUT | BDMICD_STEP_OUT, descr);
#else
	bdm_out_data(BDMICD_DSCLK | BDMICD_RST_OUT | BDMICD_STEP_OUT, descr);
#endif

#ifdef DEB_TIM
printk("bdm_icd_stop_chip: forcing DSCLK RST STEP\n");
#endif

	if (!bdm_ctr) {
		bdm_error(-BDM_FAULT_RESPONSE);
		return -BDM_FAULT_RESPONSE;
	}
#ifdef BDM_TRY_RESYNCHRO
	descr->resynchro(descr); /* tries to find correct 17 bit boundary */
#endif /* BDM_TRY_RESYNCHRO */
	return return_value;
}



/* single stepping target mcu */
static int
bdm_icd_step_chip(bdm_descr_t * descr)
{
	int tester;
	unsigned short DataOut = (BDM_GO_CMD & 1 ? BDMICD_DSI : 0);

	status_bitset bdm_stat = descr->getstatus(descr);

	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_icd_step_chip minor=%d\n", descr->minor);
	}

	/* first two conditions will not be detected on ICD... */
	if (bdm_stat & BDM_TARGETRESET) {
		bdm_error(-BDM_FAULT_RESET);
		return -BDM_FAULT_RESET;
	}
	if (bdm_stat & BDM_TARGETNC) {
		bdm_error(-BDM_FAULT_CABLE);
		return -BDM_FAULT_CABLE;
	}
	if (bdm_stat & BDM_TARGETPOWER) {
		bdm_error(-BDM_FAULT_POWER);
		return -BDM_FAULT_POWER;
	}
	tester = descr->ser_clk(descr, BDM_GO_CMD, 1);
	switch (tester) {
		case -BDM_FAULT_RESET:
		case -BDM_FAULT_CABLE:
		case -BDM_FAULT_POWER:
			return tester;
	}
	if (bdm_debug_level >= BDM_DEBUG_ALL) {
		printk("stepchip sent: %x got : %x\n", 
			BDM_GO_CMD, (unsigned int) tester);
		printk("last answerbit : %x\n", !(bdm_in_st(descr) & BDMICD_DSO));
	}
	bdm_out_data(BDMICD_OE | BDMICD_STEP_OUT | DataOut | BDMICD_RST_OUT, descr);
	bdm_delay(bdm_norm_dly);
	bdm_out_data(BDMICD_OE | DataOut | BDMICD_RST_OUT, descr);
	bdm_delay(bdm_min_dly);
	bdm_out_data(BDMICD_DSCLK | DataOut | BDMICD_RST_OUT, descr);

#ifdef BDM_TRY_RESYNCHRO
	descr->stop_chip(descr);
	return descr->resynchro(descr);
#else  /* BDM_TRY_RESYNCHRO */
	return descr->stop_chip(descr);
#endif /* BDM_TRY_RESYNCHRO */
}

/* reset target chip without asserting freeze */
static int
bdm_icd_release_chip(bdm_descr_t * descr)
{
	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_icd_release_chip minor=%d\n", descr->minor);
	}

	bdm_out_data(BDMICD_DSCLK | BDMICD_STEP_OUT, descr);
	bdm_delay(bdm_norm_dly);
	bdm_out_data(BDMICD_DSCLK | BDMICD_RST_OUT | BDMICD_STEP_OUT, descr);
	bdm_delay(bdm_short_dly);

	return BDM_FAULT_NOERROR;
}

/*
 * Restart chip, enable background debugging mode, halt on first fetch
 *
 * The software from the Motorola BBS tries to have the target
 * chip begin execution, but that doesn't work very reliably.
 * The RESETH* line rises rather slowly, so sometimes the BKPT* / DSCLK
 * would be seen low, and sometimes it wouldn't.
 */
static int
bdm_icd_reset_chip(bdm_descr_t * descr)
{
	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_icd_reset_chip minor=%d\n", descr->minor);
	}

	/* this is according to Scott Howard */
    bdm_out_data(BDMICD_DSCLK | BDMICD_STEP_OUT, descr);
	bdm_delay(bdm_norm_dly+bdm_reset_dly);
	bdm_out_data(BDMICD_DSCLK | BDMICD_STEP_OUT | BDMICD_RST_OUT, descr);
	bdm_delay(bdm_norm_dly+bdm_reset_dly);

	/*
	 *Tom Hoover's Error or Optimization? has to be checked
	 * FIXME... with scope; kick out if unnecessary.
	 */
	bdm_out_data(BDMICD_DSCLK, descr);
	bdm_delay(bdm_norm_dly+1000);
	/*
	descr->stop_chip(descr);
	 */
	return BDM_FAULT_NOERROR;
}

/* serial software protokoll for bdm-icd-interface */
static int
bdm_icd_ser_clk(bdm_descr_t * descr, short bdm_value, int delay_bits)
{
	unsigned int ShiftRegister;
	unsigned char DataOut;
	unsigned counter;
	unsigned bdm_stat = descr->getstatus(descr);

	if (bdm_debug_level >= BDM_DEBUG_ALL) {
		printk("icd_ser_clk: in-val %x delay %d\n",
			(unsigned short) bdm_value, delay_bits);
	}

	ShiftRegister = bdm_value;
	ShiftRegister <<= BITS;
	/* first two conditions will not be detected on ICD... */
	if (bdm_stat & BDM_TARGETRESET) {	/*error checking */
		bdm_error(-BDM_FAULT_RESET);
		return -BDM_FAULT_RESET;
	}
	if (bdm_stat & BDM_TARGETNC) {
		bdm_error(-BDM_FAULT_CABLE);
		return -BDM_FAULT_CABLE;
	}
	if (bdm_stat & BDM_TARGETPOWER) {
		bdm_error(-BDM_FAULT_POWER);
		return -BDM_FAULT_POWER;
	}
	if(!(bdm_stat & BDM_TARGETSTOPPED))
	{ 	if (bdm_debug_level >= BDM_DEBUG_SOME)
			printk("bdm_ser_clk stop target first minor=%d\n", descr->minor);
		if(descr->stop_chip(descr)==-BDM_FAULT_RESPONSE)
		{	printk("bdm_ser_clk can't stop it minor=%d\n", descr->minor);
			return -BDM_FAULT_RESPONSE;
		}
	}
	if(bdm_no_ser_dly_mode && (delay_bits&SER_CLK_DSO_WAIT))
	{	/* Wait for DSO to become 0 to indicate ready
		 * according CPU32UM, DSO is updated as long as
		 * DSCLK is high
		 */
		int wait_cnt;
		for(wait_cnt=0;wait_cnt<100;wait_cnt++)
		{	if (bdm_in_st(descr) & BDMICD_DSO)
				break;	/* DSO becomed zerro already */
			bdm_delay(bdm_min_dly);
		}
		if(wait_cnt && (bdm_debug_level >= BDM_DEBUG_SOME))
			printk("bdm_icd_ser_clk: ready after %d cycles\n",
				wait_cnt);
	}

	counter = 32 - BITS - (delay_bits&0xff);
	if (bdm_debug_level >= BDM_DEBUG_ALL) {
		printk("bdm_icd_ser_clk: writing %x\n", bdm_value);
	}
	while (counter--) {
		DataOut = ((ShiftRegister & 0x80000000) ? BDMICD_DSI : 0);
		/*if 1 then DSI=1*/
		bdm_out_data(DataOut | BDMICD_RST_OUT | BDMICD_OE | BDMICD_STEP_OUT | 
			BDMICD_LED, descr);		/* DSCLK=0 */
		if(!bdm_no_ser_dly_mode)
			bdm_delay(bdm_norm_dly);

		ShiftRegister <<= 1;	/*shift one bit left*/
		if (!(bdm_in_st(descr) & BDMICD_DSO))
			ShiftRegister |= 1;	/*put 1 on 0.bit */
		bdm_out_data(DataOut | BDMICD_RST_OUT | BDMICD_OE | BDMICD_STEP_OUT |
			BDMICD_DSCLK | BDMICD_LED, descr); /* DSCLK=1 */
		if(!bdm_no_ser_dly_mode)
			bdm_delay(bdm_half_dly);
	}
	if ((delay_bits&0xff) == 0) {
		bdm_out_data(BDMICD_DSCLK | BDMICD_STEP_OUT | BDMICD_RST_OUT, 
			descr);
		bdm_delay(bdm_min_dly);
	}
	if (bdm_debug_level >= BDM_DEBUG_ALL) {
		printk("icd_ser_clk: out-val %x delay %d\n", 
			(unsigned short) ShiftRegister, delay_bits);
	}


#ifdef FIXME
	if (!(delay_bits&0xff) && (ShiftRegister & 0x10000)) switch (ShiftRegister) {
		case 0x1ffff:	/* no valid command return */
			return -BDM_FAULT_NVC;
		case 0x10001:
			return -BDM_FAULT_BERR;
		case 0x10000:
			return BDM_NOTE_NOTREADY;
		default: ;
	}
#endif /* FIXME */
	return ShiftRegister;
}

/* switch LED if available */
static int
bdm_icd_setled(bdm_descr_t * descr, int switchstate)
{
	bdm_icd_led_status = (switchstate ? BDMICD_LED : 0);
	bdm_out_data(BDMICD_STEP_OUT | BDMICD_DSCLK | BDMICD_RST_OUT |
		bdm_icd_led_status, descr);	
	return BDM_FAULT_NOERROR;
}

#endif

static int bdm_minor_index[] = {
#ifdef PD_INTERFACE
	0,	1,	2,
#else
	-1,	-1,	-1,
#endif
	-1,
#ifdef ICD_INTERFACE
	3,	4,	5,
#else
	-1,	-1,	-1,
#endif
	-1,
#ifdef AS_INTERFACE
	-1,	-1,	-1,
#else
	-1,	-1,	-1,
#endif
	-1
};
const int bdm_minor_index_size = 
	sizeof(bdm_minor_index)/sizeof(*bdm_minor_index);

#define	PD_DESCRIPTOR(adr,minor,name) \
		BDM_DESCR_MAGIC, adr, minor, \
		bdm_pd_init, bdm_pd_deinit, bdm_pd_getstatus,\
		bdm_pd_restart_chip, bdm_pd_release_chip, bdm_pd_stop_chip,\
		bdm_pd_step_chip, bdm_pd_reset_chip, bdm_pd_ser_clk,\
		bdm_pd_setled, bdm_pd_resynchro , name

#define	ICD_DESCRIPTOR(adr,minor,name) \
		BDM_DESCR_MAGIC, adr, minor, \
		bdm_icd_init, bdm_icd_deinit, bdm_icd_getstatus,\
		bdm_icd_restart_chip, bdm_icd_release_chip, bdm_icd_stop_chip,\
		bdm_icd_step_chip, bdm_icd_reset_chip, bdm_icd_ser_clk,\
		bdm_icd_setled, bdm_icd_resynchro , name

#define	EMPTY_DESCRIPTOR(adr,minor) \
		0, adr, minor, \
		NULL, NULL, NULL, \
		NULL, NULL, NULL, \
		NULL, NULL, NULL, \
		NULL, NULL, NULL

static bdm_descr_t bdm_driver_descriptors[] = {
#ifdef PD_INTERFACE
	{ PD_DESCRIPTOR(PORT_ADDRESSE_LP1,0,"pd0") },
	{ PD_DESCRIPTOR(PORT_ADDRESSE_LP2,1,"pd1") },
	{ PD_DESCRIPTOR(PORT_ADDRESSE_LP3,2,"pd2") },
#else
	{ EMPTY_DESCRIPTOR(0,0) },
	{ EMPTY_DESCRIPTOR(0,1) },
	{ EMPTY_DESCRIPTOR(0,2) },
#endif
#ifdef ICD_INTERFACE
	{ ICD_DESCRIPTOR(PORT_ADDRESSE_LP1,4,"icd0") },
	{ ICD_DESCRIPTOR(PORT_ADDRESSE_LP2,5,"icd1") },
	{ ICD_DESCRIPTOR(PORT_ADDRESSE_LP3,6,"icd2") },
#else
	{ EMPTY_DESCRIPTOR(0,4) },
	{ EMPTY_DESCRIPTOR(0,5) },
	{ EMPTY_DESCRIPTOR(0,6) },
#endif
	{ EMPTY_DESCRIPTOR(0,8) },
	{ EMPTY_DESCRIPTOR(0,9) },
	{ EMPTY_DESCRIPTOR(0,10) }
};

/* preparation macros to get minor descriptor */

#define	check_descriptor(rdev, descr) \
{\
	unsigned int minor; int descr_indx;\
	if ((minor = kc_dev2minor(rdev)) > bdm_minor_index_size) {\
		return -ENODEV;\
	} \
	if ((descr_indx = bdm_minor_index[minor]) < 0) {\
		return -ENODEV;\
	}\
	descr = &bdm_driver_descriptors[descr_indx];\
	if (descr->minor != minor) {\
		printk("Inconsistency: descr->minor %d, minor %d\n", descr->minor,\
			minor);\
		return -ENODEV;\
	}\
}

#define	minor2descriptor(minor) \
	(&bdm_driver_descriptors[bdm_minor_index[(minor)]])

static bdm_descr_t *bdm_file2descr(const struct file *file)
{
	bdm_descr_t *descr = (bdm_descr_t *)(file->private_data);
	if(descr==NULL){
		printk(KERN_CRIT "BDM no descriptor!!!\n");
		return NULL;
	}
	if(descr->magic!=BDM_DESCR_MAGIC){
		printk(KERN_CRIT "BDM wrong MAGIC number!!!\n");
		return NULL;
	}
	return descr;
}

static int
bdm_open(struct inode *inode, struct file *file)
{
	status_bitset bdm_okay;
	bdm_descr_t *descr;
	int val;
  #ifdef WITH_PARPORT_SUPPORT
	static struct pardevice *parport;
  #endif /* WITH_PARPORT_SUPPORT */
  

	check_descriptor(inode->i_rdev, descr);

  #ifndef WITH_PARPORT_SUPPORT
 	if (check_region(BDM_DATA(descr), 3)) {
		printk("bdm_open: address space in use (conflict with PARPORT ?)...\n");
 		return -EBUSY;
		request_region(BDM_DATA(descr), 3, "bdm");
	}
  #else /* WITH_PARPORT_SUPPORT */
	parport=bdm_get_parport(descr);
	if(!parport)
		return -ENODEV;
	if(parport_claim(parport)) {
		printk("bdm_open: no access to PARPORT device");
		return -EBUSY;
	}
  #endif /* WITH_PARPORT_SUPPORT */
	/* stupid test if required port is really available */
	val=bdm_in_data(descr);
	val&=~0x21;
	bdm_out_data(val, descr);
	if (bdm_in_data(descr) != val) {
		printk("bdm_open: physical device not detected...\n");
	  #ifdef WITH_PARPORT_SUPPORT
		parport_release(bdm_get_parport(descr));
	  #else
		release_region(BDM_DATA(descr), 3);
	  #endif
		return -ENODEV;
	}

	kc_MOD_INC_USE_COUNT;
	
	file->private_data=descr;

	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_open\n");
	}

	bdm_okay = descr->init(descr);

	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		if (!(bdm_okay & BDM_TARGETNC)) {
			printk("BDM detected !!\n");
		} else {
			printk("Target not connected ! Error %d \n", bdm_okay);
		}
	}

	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_open successful\n");
	}

	return BDM_FAULT_NOERROR;
}

static CLOSERET 
bdm_release(struct inode *inode, struct file *file)
{
	bdm_descr_t *descr = bdm_file2descr(file);
	if(!descr) return -ENODEV;

	descr->deinit(descr);
	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_release\n");
	}

	file->private_data=NULL;
	kc_MOD_DEC_USE_COUNT;
#ifdef WITH_PARPORT_SUPPORT
	parport_release(bdm_get_parport(descr));
#else
	release_region(BDM_DATA(descr), 3);
#endif
	return (CLOSERET)0;
}

static int
bdm_ioctl(struct inode *inode, struct file *file, 
		unsigned int cmd, unsigned long arg)
{
	unsigned retval = BDM_FAULT_NOERROR;
	status_bitset status; 
	bdm_descr_t *descr = bdm_file2descr(file);
	if(!descr) return -ENODEV;

	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_ioctl minor=%d cmd=%d (%s)  arg=0x%lx\n", 
			descr->minor, cmd, bdm_get_ioctl_name(cmd), arg);
	}
	switch (cmd) {
	  case BDM_INIT:
		status = descr->init(descr);
		if (status & BDM_TARGETNC) 
			retval = -BDM_FAULT_CABLE;
		else if (status & BDM_TARGETPOWER) 
			retval = -BDM_FAULT_POWER;
		break;
	  case BDM_DEINIT:
		retval = descr->deinit(descr);
		break;
	  case BDM_RESET_CHIP:
		retval = descr->reset_chip(descr);		
		/* hardware reset on MCU - running state */
		break;
	  case BDM_RESTART_CHIP:	
	  	/* reset target and stops execution on first instruction fetch */
		retval = descr->restart_chip(descr);
		break;
	  case BDM_STOP_CHIP:		
	  	/* stop running target (FREEZE) */
		retval = descr->stop_chip(descr);
		break;
	  case BDM_STEP_CHIP:
		/* one step on target */
		descr->step_chip(descr);	
		retval = BDM_FAULT_NOERROR;
		break;
	  case BDM_GET_STATUS:
		retval = descr->getstatus(descr);
		break;
	  case BDM_SPEED:			
		/* Change speed value */
		if ((int) arg <= 0) {
			if(bdm_debug_level >= BDM_DEBUG_SOME)
				printk("bdm_ioctl BDM_SPEED: no_ser_dly_mode=1\n");
			bdm_no_ser_dly_mode = 1;
			bdm_norm_dly = 1;
		} else {
			bdm_no_ser_dly_mode = 0;
			bdm_norm_dly = (int) arg;
		}
		bdm_long_dly  = BDM_LONG_DLY(bdm_norm_dly);
		bdm_half_dly  = BDM_HALF_DLY(bdm_norm_dly);
		bdm_short_dly = BDM_SHORT_DLY(bdm_norm_dly);
		bdm_min_dly   = BDM_MIN_DLY(bdm_norm_dly);
		if (bdm_debug_level >= BDM_DEBUG_SOME) {
			printk("bdm_ioctl SPEED: "
				"norm %d long %d half %d short %d min %d\n", 
				bdm_norm_dly, bdm_long_dly, bdm_half_dly, bdm_short_dly,
				bdm_min_dly);
		}
		break;
	  case BDM_RELEASE_CHIP:	
		/* reset without bdm, so quit bdm mode */
		retval = descr->release_chip(descr);
		break;
	  case BDM_DEBUG_LEVEL:
		/* change debug level 0,1,2 */
		if (arg < 3) {
			bdm_debug_level = (unsigned int) arg;	
		} else {
			retval = -EINVAL;
		}
		break;
	  case BDM_GET_VERSION:
		/* read counter and return it to *arg */
		if (verify_area(VERIFY_WRITE, (int *) arg, sizeof(int))) {
			retval = -EINVAL;
		} else {
			kc_put_user_long(bdm_version, arg);
		}
		break;
	  case BDM_SENSECABLE:
		bdm_sense = arg;
		if (bdm_debug_level >= BDM_DEBUG_SOME) {
			printk("bdm_ioctl: setting bdm_sense to %d\n", bdm_sense);
		}
	    break;
	  case BDM_RESPW:
		/* reset additional delay */
		bdm_reset_dly = (unsigned int) arg;	
		if (bdm_debug_level >= BDM_DEBUG_SOME) {
			printk("bdm_ioctl: setting reset_pw to %d\n", bdm_reset_dly);
		}
		break;
	  case BDM_SETLED:
		retval = descr->set_led(descr,arg);
	    break;
#ifdef EFIICDISP

#define BDMICD_ispSCLK   1		/* ctrl-Register; clock data to isp   */
#define BDMICD_ispSDI    8		/* ctrl-Register; data line to isp    */
#define BDMICD_ispMODE   0x80	/* data-Register; mode for ips        */
#define BDMICD_ispSDO    0x20	/* stat-Register; data line from isp  */
	  case BDM_ISPSET:
	  	{
			static char ispcmd=0, ispdata=0;
			switch(arg & 0xf) {
				case BDM_ispSDI:
/* #define ISPVERBOSE 1  */
					if (bdm_debug_level >= BDM_DEBUG_ALL) {
						printk("ispSDI ");
					}
					if (arg & BDM_isp_LEVEL) {	/* neg. output */
						ispcmd &= ~BDMICD_ispSDI;
						if (bdm_debug_level >= BDM_DEBUG_ALL) {
							printk("1\n");
						}
					} else {
						ispcmd |= BDMICD_ispSDI;
						if (bdm_debug_level >= BDM_DEBUG_ALL) {
							printk("0\n");
						}
					}
					bdm_out_ctl(ispcmd, descr);
					break;
				case BDM_ispSCLK:
					if (bdm_debug_level >= BDM_DEBUG_ALL) {
						printk("ispSCLK ");
					}
					if (arg & BDM_isp_LEVEL) {	/* neg. output */
						ispcmd &= ~BDMICD_ispSCLK;
						if (bdm_debug_level >= BDM_DEBUG_ALL) {
							printk("1\n");
						}
					} else {
						ispcmd |= BDMICD_ispSCLK;
						if (bdm_debug_level >= BDM_DEBUG_ALL) {
							printk("0\n");
						}
					}
					bdm_out_ctl(ispcmd, descr);
					break;
				case BDM_ispMODE:
					if (bdm_debug_level >= BDM_DEBUG_ALL) {
						printk("ispMODE ");
					}
					if (arg & BDM_isp_LEVEL) {
						ispdata |= BDMICD_ispMODE;
						if (bdm_debug_level >= BDM_DEBUG_ALL) {
							printk("1\n");
						}
					} else {
						ispdata &= ~BDMICD_ispMODE;
						if (bdm_debug_level >= BDM_DEBUG_ALL) {
							printk("0\n");
						}
					}
					bdm_out_data(ispdata, descr);
					break;
				case BDM_ispISP:
					/* ignore */
					break;
			}
			udelay(100);	
				/* this is necessary to mimic the bdm_delay factor for isp */
			break;
		}
	  case BDM_ISPGET:
		if (bdm_in_st(descr) & BDMICD_ispSDO) {
			retval = 1;
		} else {
			retval = 0;
		}
		if (bdm_debug_level >= BDM_DEBUG_ALL) {
			printk("ispSDO %d\n", retval);
		}
	  	break;
#endif
	  default:
		retval = -EINVAL;
	}
	return retval;
}



static RWRET
bdm_write(WRITE_PARAMETERS)
{
	short bdm_word;
	int ret;
	int written = 0;
	bdm_descr_t *descr = bdm_file2descr(file);
	if(!descr) return -ENODEV;

	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_write minor %d len %d\n", descr->minor, (int)count);	 
	}
	if (verify_area(VERIFY_READ, buf, count))
		return -EINVAL;

	while (count > 0) {
		bdm_word = kc_get_user_word(buf);
		ret = descr->ser_clk(descr, bdm_word, 0);
		if (bdm_debug_level >= BDM_DEBUG_ALL) {
			printk("bdm_write sent:  %x received: %x\n",
				bdm_word, (unsigned int) ret);
		}
		if (written) {
			switch (ret) {
				case 0x1ffff:	/* no valid command return */
					return -BDM_FAULT_NVC;
				case 0x10001:	/* Buserror */
					return -BDM_FAULT_BERR;
				case -BDM_FAULT_RESET:
				case -BDM_FAULT_CABLE:
				case -BDM_FAULT_POWER:
					return ret;
			}
		}
		count -= 2;
		buf += 2;
		written += 2;
		if (!(written % BYTECOUNT_FOR_BEING_NICE)) {
			schedule();
		}
	}
	return written;
}

static RWRET
bdm_read(READ_PARAMETERS)
{
	short bdm_word = BDM_NOP_CMD, value;
	int ret;
	int read = 0, timeout = 0;
	bdm_descr_t *descr = bdm_file2descr(file);
	if(!descr) return -ENODEV;

	if (bdm_debug_level >= BDM_DEBUG_SOME) {
		printk("bdm_read minor=%d len=%d\n", descr->minor, (int)count);	 
	}
	if (verify_area(VERIFY_WRITE, buf, count))
		return -EINVAL;

	while (count > 0) {
		ret = descr->ser_clk(descr, bdm_word, SER_CLK_DSO_WAIT);

		if (bdm_debug_level >= BDM_DEBUG_ALL) {
			printk("\n read sent:  %x received: %x\n",
				bdm_word, (unsigned int) ret);
		}
		switch (ret) {
			case 0x10001:	/* test if BERR*/
				return -BDM_FAULT_BERR;
			case 0x1ffff:	/* no valid command return */
				return -BDM_FAULT_NVC;
			case 0x10000:
				timeout++;
				if (timeout - read / 2 > 4) {
					/* 
					 * we assume the FAULT_RESPONSE error, if number of 
					 * failed attempts is by 4 higher than number of
					 * successful attempts
					 */
					return -BDM_FAULT_RESPONSE;
				}
				break;
			case -BDM_FAULT_RESET:
			case -BDM_FAULT_CABLE:
			case -BDM_FAULT_POWER:
				return ret;
			default:
				value = ret;
				kc_put_user_word(value, buf);
				buf += 2;
				count -= 2;
				read += 2;
				if (!(read % BYTECOUNT_FOR_BEING_NICE)) {
					schedule();
				}
		}

	}
	return read;
}

/* static struct file_operations bdm_fops */
KC_CHRDEV_FOPS_BEG(bdm_fops)
	KC_FOPS_LSEEK(NULL)		/* lseek              */
	read:bdm_read,			/* read                */
	write:bdm_write,		/* write               */
	readdir:NULL,			/* readdir             */
	poll:NULL,			/* poll/select         */
	ioctl:bdm_ioctl,		/* ioctl               */
	mmap:NULL,			/* mmap                */
	open:bdm_open,			/* open                */
	KC_FOPS_FLUSH(NULL)		/* flush               */
	KC_FOPS_RELEASE(bdm_release)	/* release             */
	fsync:NULL,			/* fsync               */
	fasync:NULL,			/* fasync              */
	NULL,				/* check_media_change  */
	NULL,				/* revalidate          */
KC_CHRDEV_FOPS_END;

struct kc_class *bdm_class;

#ifdef WITH_PARPORT_SUPPORT
static int 
bdm_preempt(void *handle)
{
  return 1; // 1=don't release parport
}
#endif


int
init_module(void)
{
	int i;
#ifdef WITH_PARPORT_SUPPORT
	int num;
	struct parport *port;
	int num_offset=0;
#endif

	printk("BDM init_module\n   %s\n   %s\n   %s\n",
		   "$RCSfile: bdm.c,v $", "$Revision: 1.5 $", "$Date: 2006/01/07 22:47:50 $");
		   /*"$Id: bdm.c,v 1.5 2006/01/07 22:47:50 ppisa Exp $", */
	printk("   Version %s\n   Compiled at %s %s\n",
#ifdef PD_INTERFACE
		   "PD "
#endif
#ifdef ICD_INTERFACE
		   "ICD " 
#ifdef EFIICDISP
           "+ISP"
#endif
		   " " 
#endif
		   "", __DATE__, __TIME__);

#ifdef WITH_PARPORT_SUPPORT
	printk("BDM init_module: compiled with PARPORT support\n");
  	for(num=0; num < MAX_BDM_PORTS; num++)
  	{
		port=NULL;
		do{
			if(num+num_offset >= PARPORT_MAX) break;
			port=parport_find_number(num+num_offset);
			if(port) break;
			num_offset++;
		}while(1);
		if(!port) break;
		bdm_parports[num] = parport_register_device(port, "bdm", bdm_preempt, NULL, NULL,0,NULL);
		if (!bdm_parports[num]){
			printk("BDM init_module: Can't register device no.: %d",num);
		}
		parport_put_port(port);
	}
#endif

#ifndef BDM_WITH_DEVFS
	if (register_chrdev(BDM_MAJOR_NUMBER, "bdm", &bdm_fops)) {
		printk("Unable to get major number for BDM driver\n");
		return -EIO;
	}
#else /* BDM_WITH_DEVFS */
	if (devfs_register_chrdev(BDM_MAJOR_NUMBER, "bdm", &bdm_fops)) {
		printk("Unable to get major number for BDM driver\n");
		return -EIO;
	}
  /* I hate next conditional compilation, but k_compat.h can not handle
     latest changes to devfs done by Adam J. Richter <adam@yggdrasil.com>. */
  #if (LINUX_VERSION_CODE >= VERSION(2,5,60)) 
	devfs_mk_dir (BDM_DEVFS_DIR_NAME);
	bdm_class = kc_class_create(THIS_MODULE, BDM_DEVFS_DIR_NAME);
	for(i=0;i<bdm_minor_index_size;i++){
          /*char dev_name[32];*/
	  if(bdm_minor_index[i]<0) continue;
	  if(!minor2descriptor(i)->name) continue;
	  /*sprintf(dev_name, "%s/%s", BDM_DEVFS_DIR_NAME, minor2descriptor(i)->name);*/
	  devfs_mk_cdev(MKDEV(BDM_MAJOR_NUMBER, i), S_IFCHR | S_IRUGO | S_IWUGO, 
	  	"%s/%s", BDM_DEVFS_DIR_NAME, minor2descriptor(i)->name);
	  kc_class_device_create(bdm_class, NULL, MKDEV(BDM_MAJOR_NUMBER, i), NULL, minor2descriptor(i)->name);
	}
  #else /* < 2.5.60 */
	bdm_devfs_handle = devfs_mk_dir (NULL, BDM_DEVFS_DIR_NAME, NULL);
	for(i=0;i<bdm_minor_index_size;i++){
	  if(bdm_minor_index[i]<0) continue;
	  if(!minor2descriptor(i)->name) continue;
	  devfs_register(bdm_devfs_handle, minor2descriptor(i)->name,
		DEVFS_FL_DEFAULT, BDM_MAJOR_NUMBER, i,
		S_IFCHR | S_IRUGO | S_IWUGO ,
		&bdm_fops, NULL);
	}
  #endif /* 2.5.60 */
#endif /* BDM_WITH_DEVFS */
	printk("BDM driver succesfully registered !!\n");
	return 0;
}

void
cleanup_module(void)
{
#ifdef WITH_PARPORT_SUPPORT
	int num;
#endif

#ifdef WITH_PARPORT_SUPPORT
	printk("BDM init_module: compiled with PARPORT support\n");
  	for(num=0; num < MAX_BDM_PORTS; num++)
  	{	
		if (bdm_parports[num])
    			parport_unregister_device(bdm_parports[num]);
		bdm_parports[num] = NULL;
  	}
#endif
#ifdef BDM_WITH_DEVFS
  /* I hate next conditional compilation, but k_compat.h can not handle
     latest changes to devfs done by Adam J. Richter <adam@yggdrasil.com>. */
  #if (LINUX_VERSION_CODE >= VERSION(2,5,60))
        {
	  int i;
	  for(i=0;i<bdm_minor_index_size;i++){
	    if(bdm_minor_index[i]<0) continue;
	    if(!minor2descriptor(i)->name) continue;
	    kc_class_device_destroy(bdm_class, MKDEV(BDM_MAJOR_NUMBER, i));
	    devfs_remove("%s/%s", BDM_DEVFS_DIR_NAME, minor2descriptor(i)->name);
          }
	}
	devfs_remove(BDM_DEVFS_DIR_NAME);
	kc_class_destroy(bdm_class);
  #else /* < 2.5.60 */
	if(bdm_devfs_handle)
		devfs_unregister (bdm_devfs_handle);
  #endif /* < 2.5.60 */
	if (devfs_unregister_chrdev(BDM_MAJOR_NUMBER, "bdm") != 0)
#else /* BDM_WITH_DEVFS */
	if (unregister_chrdev(BDM_MAJOR_NUMBER, "bdm") != 0)
#endif /* BDM_WITH_DEVFS */
		printk("BDM cleanup: Unregister failed\n");
	else
		printk("BDM cleanup: Unregister  O.K.\n");
}
