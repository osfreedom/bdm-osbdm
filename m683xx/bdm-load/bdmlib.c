/*
 * $Id: bdmlib.c,v 1.4 2004/12/05 13:20:09 ppisa Exp $
 *
 * Remote debugging interface for 683xx via Background Debug Mode
 * needs a driver, which controls the BDM interface.
 * written by G.Magin
 * (C) 1996 Technische Universitaet Muenchen, Lehrstuhl fuer Prozessrechner
 * (C) 1997 G. Magin
 *
 * Modified by Pavel Pisa  pisa@cmp.felk.cvut.cz 1998

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

/*
 * NOTE:
 * This file is assumed to be runnable only on Linux/i386 because of
 * HW-restrictions: the driver is currently only available on i386-Linux 
 * So byte-swapping and alignment is handled directly, violating the 
 * GNU-coding standards. However, the "dirty spots" are restricted in this 
 * file. The accompanying application backend file (e.g. remote-bdm.c for 
 * gdb) is not affected by this restriction.
 *
 * If anybody wants to port to e.g. Sparc, byte-sex has to be handled in
 * a more general way.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <ansidecl.h>

/* #define	SUPPORT_RAMINIT 0 */	/* force ram_init to be loaded */
#define RAMINIT_FILENAME	"ram_init"
#define	RESETINIT_FILENAME	"cpu32init"
#define	END_MACRO	0
#define	BEGIN_MACRO	1
#define	NO_SUFFIX_MACRO	2
#define	BDMLIB_REQUIRED_DRIVER_VERSION	2
	/*
	 * version 1 (cannot be accessed by ioctl): release 3/95
	 * version 2 (includes SENSE ioctl + ICD): release 7/96
	 */

#ifdef BDMLIB_FORGDB
#include "defs.h"
#endif

#ifndef PROTO
/* this is missing in current ansidecl.h - remove it later   gm */
#if defined (__STDC__) || defined (_AIX) || (defined (__mips) && defined (_SYSTYPE_SVR4))

#define PROTO(type, name, arglist)  type name arglist
#define PARAMS(paramlist)       paramlist
#define ANSI_PROTOTYPES         1

#else		  /* Not ANSI C.  */

#define PROTO(type, name, arglist) type name ()
#define PARAMS(paramlist)       ()

#endif		  /* ANSI C.  */
#endif

#include <bfd.h>
#include "bdmlib.h"
#include "bdm.h"


static int bdm_fd;

/* default delay for interface */
#define	BDM_DEFAULT_DELAY	75
static int bdm_delay=-1;

#define	BDM_DODPRINTF		(1<<0)
#define	BDM_GOTEXCEPTION	(1<<1)
#define	BDM_DEBUG_NAME	"bdm-dbg.log"
static int bdm_flags = 0;
static int mbar_used = 0;
static u_long mbar_default_val = 0;
extern char hashmark;
extern int bdm_autoreset;
extern int bdm_ttcu;


static void dbprintf(const char *format, ...);

#if !defined BDMLIB_FORGDB
/* 
 * this is borrowed from gdb utils.c
 */
static void
error(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}

static void
fprintf_filtered(FILE *fp, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(fp, format, args);
	fprintf(fp, "\n");
	va_end(args);
}

#define printf_filtered printf
#define xmalloc malloc
#define	gdb_stderr	stderr
#endif

void 
bdmlib_setdebug(int switch_on)
{
	if (switch_on)
		bdm_flags |= BDM_DODPRINTF;
	else
		bdm_flags &= ~BDM_DODPRINTF;
	dbprintf("Setting Debug to %d\n", switch_on);
}

int 
bdmlib_querydebug(void)
{
	return (bdm_flags & BDM_DODPRINTF);
}

/* startofcleanup */
int 
bdmlib_isopen(void)
{
	return (bdm_fd != 0);
}

/* filter to put out bdm specific status messages */
char *
bdmlib_getstatus_str(bdmstatus status)
{
int st;
	static char put_buffer[128];

	*put_buffer = '\0';
	st = (int) status;
	if (st < 0) {
		return bdmlib_geterror_str(status);
	}
	if (status & BDM_TARGETNC) {
		strcat(put_buffer, "NotConnected ");
		return put_buffer;
	}
	strcat(put_buffer, "Connected ");
	if (status & BDM_TARGETPOWER) {
		strcat(put_buffer, "PowerFail ");
		return put_buffer;
	}
	strcat(put_buffer, "PowerOK ");
	if (status & BDM_TARGETSTOPPED)
		strcat(put_buffer, "Frozen ");
	else
		strcat(put_buffer, "Running ");
	if (status & BDM_TARGETRESET)
		strcat(put_buffer, "Reset ");
	return put_buffer;
}

static struct _err_messages {
	int err_num;
	char *err_msg;
}

err_messages[] = {
	{ BDM_FAULT_UNKNOWN, "bdm driver recognized unknown fault" } ,
	{ BDM_FAULT_POWER, "target power failed" } ,
	{ BDM_FAULT_CABLE, "cable disconnected" } ,
	{ BDM_FAULT_RESPONSE, "no response from target via bdm" } ,
	{ BDM_FAULT_RESET, "target got a reset" } ,
	{ BDM_FAULT_PORT, "wrong bdm port" } ,
	{ BDM_FAULT_BERR, "bus error occured on access via bdm" } ,
	{ BDM_FAULT_NVC, "bdm internal error: no valid command to bdm" } ,
	{ BDM_NO_ERROR, "No error" } ,
	{ BDM_ERR_NOT_OPEN, "bdm device is not open" } ,
	{ BDM_ERR_ILL_IOCTL, "ioctl code does not match library ioctl type" } ,
	{ BDM_ERR_WRITE_FAIL, "write to processor failed" } ,
	{ BDM_ERR_READ_FAIL, "read from processor failed" } ,
	{ BDM_ERR_ILL_SIZE, "illegal variable size" } ,
	{ BDM_ERR_OPEN, "open error" } ,
	{ BDM_ERR_LOAD, "error on loading binary file" } ,
	{ BDM_ERR_MACROFILE, "error on loading macro" } ,
	{ BDM_ERR_SECTION, "error on loading section" } ,
	{ BDM_ERR_VERSION, "driver version conflict" } ,
};

static int err_msg_len = sizeof(err_messages) / sizeof(struct _err_messages);

/* filter to pick out bdm specific error messages */
char *
bdmlib_geterror_str(int err)
{
	int i;
	static char put_buffer[128];

	for (i = 0; i < err_msg_len; i++) {
		if (err == err_messages[i].err_num)
			return err_messages[i].err_msg;
	}
	strncpy(put_buffer, strerror(-err), 128); 
	return put_buffer;
}

#define do_case(x) case x: ret = #x; break;

static char * 
bdmlib_getioctlname(int ioctl)
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
		default:
			ret = "Unknown ioctl";
	}
	return ret;
}

int 
bdmlib_setioctl(u_int code, u_int val)
{
	if (bdmlib_isopen()) {
		switch (code) {
		  case BDM_SENSECABLE:
		  case BDM_DEBUG_LEVEL:
		  	break;
		  case BDM_SPEED:
		  	bdm_delay = val; break;
		  default:
			return BDM_ERR_ILL_IOCTL;
		}
		return ioctl(bdm_fd, code, (u_long) val);
	} else {
		return BDM_ERR_NOT_OPEN;
	}
}

int 
bdmlib_ioctl(u_int code)
{
int ret;
	if (bdmlib_isopen()) {
		switch (code) {
		  case BDM_INIT:
		  case BDM_RESET_CHIP:
		  case BDM_STOP_CHIP:
		  case BDM_RESTART_CHIP:
		  case BDM_RELEASE_CHIP:
		  case BDM_STEP_CHIP:

			  if ((ret = ioctl(bdm_fd, code, (u_long) NULL)) == -1) {
			  	dbprintf("bdmlib_ioctl %s failed; error %d %s\n",
					bdmlib_getioctlname(code), errno, 
					bdmlib_geterror_str(errno));
			  	return -errno;
			  } else {
			  	return BDM_NO_ERROR;
			  }
			  break;
		  default:
			  return BDM_ERR_ILL_IOCTL;
		}
	} else {
		return BDM_ERR_NOT_OPEN;
	}
}

/*
* get status of interface
*/
bdmstatus
bdmlib_getstatus(void)
{
	if (bdmlib_isopen()) {
		return ioctl(bdm_fd, BDM_GET_STATUS, NULL);
	} else {
		return BDM_ERR_NOT_OPEN;
	}
}

int
bdmlib_go(void)
{
	u_short buf;
	int ret;

	dbprintf("bdmlib_go\n");

#if MORE_DEBUGGING
	bdmlib_showpc();
#endif /* MORE_DEBUGGING */

	if (bdmlib_isopen()) {
		buf = BDM_GO_CMD;
		ret = write(bdm_fd, &buf, 2);
		if ((ret < 0) || (ret != 2)) {
			dbprintf("bdm_go: write_error %d\n", errno);
			return -errno;
		}
	} else {
		return BDM_ERR_NOT_OPEN;
	}
	return BDM_NO_ERROR;
}

int
bdmlib_set_mbar(u_long mbar_val)
{
  int ret;
  bdmlib_set_sys_reg(BDM_REG_DFC, 7);
  bdmlib_set_sys_reg(BDM_REG_SFC, 7);
  ret = bdmlib_write_var((caddr_t)0x3ff00, BDM_SIZE_LONG, mbar_val);
  if(ret >= 0){
    mbar_used = 1;
    mbar_default_val = mbar_val;
  }
  bdmlib_set_sys_reg(BDM_REG_DFC, 5);
  bdmlib_set_sys_reg(BDM_REG_SFC, 5);
  return ret;
}

int
bdmlib_reset(void)
{
#if SUPPORT_RAMINIT
	u_long dummy;
#endif 
	int ret;

	dbprintf("bdmlib_reset ttcu %d\n", bdm_ttcu);
	if ((ret = bdmlib_ioctl(BDM_RESTART_CHIP))) {
		return ret;
	}

#if MORE_DEBUGGING
	printf("bdmlib_reset: RESTART_CHIP replies %d %s\n",
		   ret, bdmlib_geterror_str(ret));
	printf("status: %s\n", bdmlib_getstatus_str(bdmlib_getstatus()));
#endif	/* MORE_DEBUGGING  */

	/*
	 * it cannot break anything to set right SFC and DFC
	 * adress space for memory accesses
	 */

	bdmlib_set_sys_reg(BDM_REG_DFC, 5);
	bdmlib_set_sys_reg(BDM_REG_SFC, 5);
	
	/*
	 * The 68360 targets requires setup of MBAR to enable access
	 * to integrated modules and system configuration registers
	 */
	 
	if(mbar_used) {
	  bdmlib_set_mbar(mbar_default_val);
	}

	/*
	 * in case we have a monitor in place, we might want to let him do
	 * the basic setup, let it come to the prompt...
	 */
	if (bdm_ttcu) {
		if ((ret = bdmlib_go())) 	/* let the monitor come up.... */
			return ret;		
		usleep(bdm_ttcu);
		ret = bdmlib_ioctl(BDM_STOP_CHIP);
	}

#if SUPPORT_RAMINIT
	if ((ret = bdmlib_do_load_binary(RAMINIT_FILENAME, &dummy)) < 0) {
		fprintf_filtered(gdb_stderr, "Warning: %s for file `%s'\n",
			bdmlib_geterror_str(ret),
			RAMINIT_FILENAME);
	}
#endif /* SUPPORT_RAMINIT */

	bdmlib_do_load_macro(RESETINIT_FILENAME, NO_SUFFIX_MACRO);
	fprintf(stdout, "\r"); fflush(stdout);
	return ret;
}

#define swaps(x) \
((u_short)((((u_short)(x) & 0x00ff) << 8) | \
	(((u_short)(x) & 0xff00) >> 8)))

#define	swapl(x) \
((u_int)((((u_int)(x) & 0x000000ffU) << 24) | \
	(((u_int)(x) & 0x0000ff00U) << 8) | \
	(((u_int)(x) & 0x00ff0000U) >> 8) | \
	(((u_int)(x) & 0xff000000U) >> 24)))

static u_short *
bdmlib_conv_short_to_buf(u_short * buf, u_short val, int endianness)
{
	if (endianness) {
		*buf++ = swaps(val);
	} else {
		*buf++ = val;
	}
	return buf;
}

/* conv long in host format (little endian) to buf (target format = big e) */
static u_short *
bdmlib_conv_long_to_buf(u_short * buf, u_long val, int endianness)
{
	if (endianness) {
		*buf++ = swaps(val & 0xffff);
		*buf++ = swaps(val >> 16);
	} else {
		*buf++ = val >> 16;
		*buf++ = val & 0xffff;
	}
	return buf;
}

/* buf representation to a single short */
static u_short *
bdmlib_conv_buf_to_short(u_short * buf, u_short * val, int endianness)
{
	if (endianness) {
		*val = *buf++;
	} else {
		*val = swaps(*buf);
		buf++;
	}
	return buf;
}

/* buf representation to a single long */
static u_short *
bdmlib_conv_buf_to_long(u_short * buf, u_long * val, int endianness)
{
	if (endianness) {
		*val = (buf[0] << 16) | buf[1];
	} else {
		*val = (swaps(buf[1]) << 16) | swaps(buf[0]);
	}
	buf += 2;
	return buf;
}

/*
* put a char stream of 4 bytes into a long;
* endianness=0: interprete char as little end
* else interprete char as big endian
*/
static u_char *
bdmlib_conv_char_to_long(u_char * buf, u_long * val, int endianness)
{
	int i;
	union {
		u_long l;
		u_char c[4];
	} u;

	if (endianness) {
		for (i = 3; i >= 0; i--)
			u.c[i] = *buf++;
	} else {
		for (i = 0; i < 4; i++)
			u.c[i] = *buf++;
	}
	*val = u.l;
	return buf;
}

/* put a char stream of 2 bytes into a short */
static u_char *
bdmlib_conv_char_to_short(u_char * buf, u_short * val, int endianness)
{
	int i;
	union {
		u_short s;
		u_char c[2];
	} u;

	if (endianness) {
		for (i = 1; i >= 0; i--)
			u.c[i] = *buf++;
	} else {
		for (i = 0; i < 2; i++)
			u.c[i] = *buf++;
	}
	*val = u.s;
	return buf;
}

/* put a long to a char stream */
static u_char *
bdmlib_conv_long_to_char(u_char * buf, u_long val, int endianness)
{
	int i;
	union {
		u_long l;
		u_char c[4];
	} u;

	u.l = val;
	if (endianness) {
		for (i = 3; i >= 0; i--)
			*buf++ = u.c[i];
	} else {
		for (i = 0; i < 4; i++)
			*buf++ = u.c[i];
	}
	return buf;
}

/* put a short to a char stream */
static u_char *
bdmlib_conv_short_to_char(u_char * buf, u_short val, int endianness)
{
	int i;
	union {
		u_short s;
		u_char c[2];
	} u;

	u.s = val;
	if (endianness) {
		for (i = 1; i >= 0; i--)
			*buf++ = u.c[i];
	} else {
		for (i = 0; i < 2; i++)
			*buf++ = u.c[i];
	}
	return buf;
}

int
bdmlib_get_sys_reg(u_int reg, u_int * ret_val)
{
	u_short send;
	u_short recv[2];
	int ret;

	send = BDM_RSREG_CMD | (reg & 0xf);
	if (!bdmlib_isopen())
		return BDM_ERR_NOT_OPEN;
	ret = write(bdm_fd, &send, 2);
	if (ret == 2) {
		ret = read(bdm_fd, recv, 4);
	} else {
		dbprintf("bdmlib_get_sys_reg error on send: errno %d\n", errno);
		return -errno;
	}
	bdmlib_conv_buf_to_long(recv, (u_long *) ret_val, 0);
	return (ret == 4) ? BDM_NO_ERROR : -errno;
}

int
bdmlib_set_sys_reg(u_int reg, u_int cont)
{
	u_short send[3];
	int ret;

	if (!bdmlib_isopen())
		return BDM_ERR_NOT_OPEN;
	send[0] = BDM_WSREG_CMD | (reg & 0xf);
	bdmlib_conv_long_to_buf(&send[1], cont, 0);
	if ((ret = write(bdm_fd, send, 6)) != 6) {
		dbprintf("bdmlib_set_sys_reg error: reg %#x cont %#x error %d\n",
				reg, cont, errno);
		return -errno;
	}
	return BDM_NO_ERROR;
}

int
bdmlib_get_reg(u_int reg, u_int * ret_val)
{
	u_short send;
	u_short recv[2];
	int ret;

	if (!bdmlib_isopen())
		return BDM_ERR_NOT_OPEN;
	send = BDM_RREG_CMD | (reg & 0xf);
	ret = write(bdm_fd, &send, 2);
	if (ret == 2) {
		ret = read(bdm_fd, recv, 4);
	} else {
		dbprintf("bdmlib_get_reg: error on send ret %d\n", errno);
		return -errno;
	}
	/* get target format, as conversion will be done in higher levels */
	bdmlib_conv_buf_to_long(recv, (u_long *) ret_val, 0);
	return (ret == 4) ? BDM_NO_ERROR : -errno;
}

int
bdmlib_set_reg(u_int reg, u_int cont)
{
	u_short send[3];
	int ret;

	if (!bdmlib_isopen())
		return BDM_ERR_NOT_OPEN;
	send[0] = BDM_WREG_CMD | (reg & 0xf);
	bdmlib_conv_long_to_buf(&send[1], cont, 0);
	if ((ret = write(bdm_fd, send, 6)) != 6) {
		dbprintf("bdmlib_set_reg error: reg %#x to cont %#x error %#d\n",
				reg, cont, errno);
		return -errno;
	}
	return BDM_NO_ERROR;
}

int
bdmlib_write_var(caddr_t adr, u_short size, u_int val)
{
	u_short buf[6], *b_ptr = buf;
	int w_buf_len, r_buf_len;
	int written;

	dbprintf("bdmlib_write_var: addr %#x cont %#x\n", adr, val);
	if (!bdmlib_isopen())
		return BDM_ERR_NOT_OPEN;
	size &= 0x00c0;
	*b_ptr++ = (BDM_WRITE_CMD | size);
	/* no need for byte swapping, as adr is already a valid long; so use 0 */
	b_ptr = bdmlib_conv_long_to_buf(b_ptr, (u_long) adr, 0);
	switch (size) {
	  case BDM_SIZE_LONG:
		  b_ptr = bdmlib_conv_long_to_buf(b_ptr, val, 0);
		  w_buf_len = 5;
		  break;
	  case BDM_SIZE_WORD:
		  b_ptr = bdmlib_conv_short_to_buf(b_ptr, val, 0);
		  w_buf_len = 4;
		  break;
	  case BDM_SIZE_BYTE:
		  *b_ptr++ = (u_short) (val & 0xff);
		  w_buf_len = 4;
		  break;
	  default:
		  w_buf_len = 0;
		  dbprintf("error! bdmlib_write_var: unknown size %#x\n", size);
		  return BDM_ERR_ILL_SIZE;
	}
	if ((written = write(bdm_fd, buf, w_buf_len * 2)) != w_buf_len * 2) {
		dbprintf("error! bdmlib_write_var: write returns %d errno %d\n",
			written, errno);
		written = errno;
	}
	if ((r_buf_len = read(bdm_fd, buf, 2)) != 2) {
		dbprintf(
"bdmlib_write_var: verify read return val expected 2 is %d errno %d\n",
				r_buf_len, errno);
		return -errno;
	}
	if (written == (w_buf_len*2)) {
		return BDM_NO_ERROR;
	} else {
		return -written;
	}
}



#if 1

/* slower but reliable method for writting, main solved problem is
   waiting for ready after memory access achieved by read 
   in bdm_write_var */ 

int
bdmlib_write_block(caddr_t in_adr, u_int size, u_char * bl_ptr)
{
	u_short buf[8];
	int fills, got_size = 0;
	u_long ul;
	u_short us;
	u_char uc;
	u_int first_acc;

	dbprintf("bdmlib_write_block size %#x to adr %#x ", size, in_adr);
	first_acc = 4 - ((u_long) in_adr & 0x3);
	if (size < first_acc) {
		switch (size) {
			case 0: return 0;
			case 1: first_acc = 1; break;
			case 2:
			case 3: first_acc = first_acc&1? 1: 2; break;
		}
	}
	switch (first_acc) {
	  case 4:
		  bl_ptr = bdmlib_conv_char_to_long(bl_ptr, &ul, 1);
		  if (bdmlib_write_var(in_adr, BDM_SIZE_LONG,  ul) < 0) {
			  return got_size;
		  }
		  got_size += 4;
		  break;
	  case 3:
		  uc = *bl_ptr++;
		  if (bdmlib_write_var(in_adr, BDM_SIZE_BYTE,  uc) < 0) {
			  return got_size;
		  }
		  in_adr += 1;
		  got_size += 1;
		  /* fall through to 'word' */
	  case 2:
		  bl_ptr = bdmlib_conv_char_to_short(bl_ptr, &us, 1);
		  if (bdmlib_write_var(in_adr, BDM_SIZE_WORD,  us) < 0) {
			  return got_size;
		  }
		  got_size += 2;
		  break;
	  case 1:
		  uc = *bl_ptr++;
		  if (bdmlib_write_var(in_adr, BDM_SIZE_BYTE, uc) < 0) {
			  return got_size;
		  }
		  got_size += 1;
		  break;
	  default: ;
		  /* cannot happen */
	}


	buf[0] = BDM_FILL_CMD | BDM_SIZE_LONG;
	fills = (size - got_size) / 4;
	while (fills--) {
		bl_ptr = bdmlib_conv_char_to_long(bl_ptr, &ul, 1);
		bdmlib_conv_long_to_buf(&buf[1], ul, 0);
		if (write(bdm_fd, &buf[0], 6) !=6) {
			return got_size;
		}
		if (read(bdm_fd, &buf[1], 2) != 2) {
			return got_size;
		}
		got_size += 4;
	}

	if (size - got_size >= 2) {
		buf[0] = (BDM_FILL_CMD | BDM_SIZE_WORD);
		bl_ptr = bdmlib_conv_char_to_short(bl_ptr, &us, 1);
		bdmlib_conv_short_to_buf(&buf[1], us, 0);		  
		if(write(bdm_fd, &buf[0], 4)!=4) return got_size;
		if (read(bdm_fd, &buf[0], 2) != 2) return got_size;
		got_size += 2;
	}
	if (size - got_size) {
		buf[0] = (BDM_FILL_CMD | BDM_SIZE_BYTE);
		buf[1] = *bl_ptr++;
		if(write(bdm_fd, &buf[0], 4)!=4) return got_size;
		if (read(bdm_fd, &buf[0], 2) != 2) return got_size;
		got_size += 1;
	}
	if (size - got_size) {	  /* cannot happen */
		error("internal error: bdmlib_write_block - cannot happen");		  
	}
	
	return got_size;
}

#else

int
bdmlib_write_block(caddr_t in_adr, u_int size, u_char * block)
{
	u_short *buf, *buf_ptr;
	int buf_len = 0;			/* valid len of buf to writecmd in shorts */
	u_char *bl_ptr;
	u_int adr;
	int fills, put_size;
	u_long ul;
	u_short us;
	u_int first_acc;

	dbprintf("bdmlib_write_block size %#x to adr %#x\n", size, in_adr);
	if (!bdmlib_isopen())
		return BDM_ERR_NOT_OPEN;
	first_acc = 4 - ((u_long) in_adr & 0x3);
	if (size < first_acc) {
		switch (size) {
			case 0: return 0;
			case 1: first_acc = 1; break;
			case 2:
			case 3: first_acc = first_acc&1? 1: 2; break;
		}
	}

	put_size = size;
	adr = (u_int) in_adr;
	bl_ptr = block;
	buf_ptr = buf = xmalloc((size / 4 + 5) * 6);

	/*
	 * per 4 bytes 3 shorts for fill-long-cmd
	 * + 3 short for initial write-long-cmd + addr
	 *  = 1 * 3 shorts
	 * + worst case both a write-byte and write-short on begin & end
	 *  = 4 * 3 shorts
	 */

	switch (first_acc) {
	  case 4:
		  *buf_ptr++ = (BDM_WRITE_CMD | BDM_SIZE_LONG);
		  *buf_ptr++ = (adr & 0xffff0000) >> 16;	/* h-Address */
		  *buf_ptr++ = (adr & 0x0000ffff);	/* l-Address */
		  bl_ptr = bdmlib_conv_char_to_long(bl_ptr, &ul, 1);
		  buf_ptr = bdmlib_conv_long_to_buf(buf_ptr, ul, 0);
		  adr += 4;
		  size -= 4;
		  buf_len = 5;
		  break;
	  case 3:
		  *buf_ptr++ = (BDM_WRITE_CMD | BDM_SIZE_BYTE);
		  *buf_ptr++ = (adr & 0xffff0000) >> 16;	/* h-Address */
		  *buf_ptr++ = (adr & 0x0000ffff);	/* l-Address */
		  *buf_ptr++ = *bl_ptr++;
		  adr += 1;
		  size -= 1;
		  buf_len = 4;
		  /* fall through to 'word' */
	  case 2:
		  *buf_ptr++ = (BDM_WRITE_CMD | BDM_SIZE_WORD);
		  *buf_ptr++ = (adr & 0xffff0000) >> 16;	/* h-Address */
		  *buf_ptr++ = (adr & 0x0000ffff);	/* l-Address */
		  bl_ptr = bdmlib_conv_char_to_short(bl_ptr, &us, 1);
		  buf_ptr = bdmlib_conv_short_to_buf(buf_ptr, us, 0);
		  adr += 2;
		  size -= 2;
		  buf_len += 4;
		  break;
	  case 1:
		  *buf_ptr++ = (BDM_WRITE_CMD | BDM_SIZE_BYTE);
		  *buf_ptr++ = (adr & 0xffff0000) >> 16;	/* h-Address */
		  *buf_ptr++ = (adr & 0x0000ffff);	/* l-Address */
		  *buf_ptr++ = *bl_ptr++;
		  adr += 1;
		  size -= 1;
		  buf_len = 4;
		  break;
	  default:
		  /* cannot happen */
	}
	fills = size / 4;
	size -= fills * 4;
	buf_len += (fills * 3);
	while (fills--) {
		*buf_ptr++ = (BDM_FILL_CMD | BDM_SIZE_LONG);
		bl_ptr = bdmlib_conv_char_to_long(bl_ptr, &ul, 1);
		buf_ptr = bdmlib_conv_long_to_buf(buf_ptr, ul, 0);
	}

	if(size>=2){
		*buf_ptr++ = (BDM_FILL_CMD | BDM_SIZE_WORD);
		bl_ptr = bdmlib_conv_char_to_short(bl_ptr, &us, 1);
		buf_ptr = bdmlib_conv_short_to_buf(buf_ptr, us, 0);
		buf_len += 2;
		size -= 2;
	}
	if(size>=1){
		*buf_ptr++ = (BDM_FILL_CMD | BDM_SIZE_BYTE);
		*buf_ptr++ = *bl_ptr;
		buf_len += 2;
		size--;
	}
	if(size){
		error("internal error: bdmlib_write_block: size=%d!=0",size);
	}

	/* now we have a whole buf we can send in one chunk to 'write' */

	buf_len *= 2;				/* now buflen in bytes */
	if ((size = write(bdm_fd, buf, buf_len)) != buf_len) {
		dbprintf("error! bdmlib_write_block: write %d returns %d errno %d %s\n",
				buf_len, size, errno, bdmlib_geterror_str(-errno));
		put_size = 0;
	}
	/*
 * FIXME: needs more persistence on trying to write; analyze reason
 * for not writing, and if possible, try again with rest of the buf
 * give up on detected bus-err
 */

	free(buf);
	return put_size;
}


#endif

/* return format in *val is target byte ordering */
int
bdmlib_read_var(caddr_t adr, u_short size, void *val)
{
	u_short buf[6], *b_ptr = buf;
	int count;

	/* u_char *cptr; u_short *sptr; u_long *lptr;	*/

	dbprintf("bdmlib_read_var size %#x from adr %#x ", size, adr);
	if (!bdmlib_isopen())
		return BDM_ERR_NOT_OPEN;
	size &= 0x00c0;
	*b_ptr++ = (BDM_READ_CMD | size);
	/* no need for byte swapping, as adr is already a valid long; so use 0 */
	b_ptr = bdmlib_conv_long_to_buf(b_ptr, (u_long) adr, 0);
	if ((count = write(bdm_fd, buf, 6)) != 6) {
		dbprintf("bdmlib_read_var error: write returns %d\n", count);
		return BDM_ERR_WRITE_FAIL;
	}
	switch (size) {
	  case BDM_SIZE_LONG:
		  if ((count = read(bdm_fd, buf, 4)) == 4) {
			  *(u_long *) val = (buf[0] << 16) | buf[1];
		  }
		  break;
	  case BDM_SIZE_WORD:
		  if ((count = read(bdm_fd, buf, 2)) == 2) {
			  *(u_short *) val = (u_short) buf[0];
			  /* sptr = (u_short*) val; *sptr = (u_short) buf[0];	*/
		  }
		  break;
	  case BDM_SIZE_BYTE:
		  if ((count = read(bdm_fd, buf, 2)) == 2) {
			  count = 1;
			  *(u_char *) val = (u_char) (buf[0] & 0xff);
			  /* cptr = (u_char*)val; *cptr = (u_char) (buf[0] & 0xff); */
		  }
		  break;
	  default:
		  dbprintf("\n\terror! bdmlib_read_var: unknown size %#x\n", size);
	}
	if (count < 0) {
		dbprintf("\nerror %d (%s)\n", errno, bdmlib_geterror_str(errno));
		return -errno;
	}
	/* val = (u_int*) ((u_long)val & ~3); 	*/
	dbprintf(" cont %#x\n", *(u_long *) ((long) val & ~3));
	return count;				/* _must_ return count */
}

int
bdmlib_read_block(caddr_t in_adr, u_int size, u_char * bl_ptr)
{
	u_short buf[8];
	u_short dump_cmd;
	int dumps, got_size = 0;
	u_long ul;
	u_short us;
	u_char uc;
	u_int first_acc;

	dbprintf("bdmlib_read_block size %#x from adr %#x ", size, in_adr);
	first_acc = 4 - ((u_long) in_adr & 0x3);
	if (size < first_acc) {
		switch (size) {
			case 0: return 0;
			case 1: first_acc = 1; break;
			case 2:
			case 3: first_acc = first_acc&1? 1: 2; break;
		}
	}
	switch (first_acc) {
	  case 4:
		  if (bdmlib_read_var(in_adr, BDM_SIZE_LONG, &ul) != 4) {
			  return got_size;
		  }
		  bl_ptr = bdmlib_conv_long_to_char(bl_ptr, ul, 1);
		  got_size += 4;
		  break;
	  case 3:
		  if (bdmlib_read_var(in_adr, BDM_SIZE_BYTE, &uc) != 1) {
			  return got_size;
		  }
		  *bl_ptr++ = uc;
		  got_size += 1;
		  in_adr += 1;
		  /* fall through to 'word' */
	  case 2:
		  if (bdmlib_read_var(in_adr, BDM_SIZE_WORD, &us) != 2) {
			  return got_size;
		  }
		  bl_ptr = bdmlib_conv_short_to_char(bl_ptr, us, 1);
		  got_size += 2;
		  break;
	  case 1:
		  if (bdmlib_read_var(in_adr, BDM_SIZE_BYTE, &uc) != 1) {
			  return got_size;
		  }
		  *bl_ptr++ = uc;
		  got_size += 1;
		  break;
	  default: ;
		  /* cannot happen */
	}


	dump_cmd = BDM_DUMP_CMD | BDM_SIZE_LONG;
	dumps = (size - got_size) / 4;
	while (dumps--) {
		write(bdm_fd, &dump_cmd, 2);
		if (read(bdm_fd, &buf, 4) != 4) {
			return got_size;
		}
		bdmlib_conv_buf_to_long(buf, &ul, 1);
		bl_ptr = bdmlib_conv_long_to_char(bl_ptr, ul, 1);
		got_size += 4;
	}

	if (size - got_size >= 2) {
		dump_cmd = BDM_DUMP_CMD | BDM_SIZE_WORD;
		write(bdm_fd, &dump_cmd, 2);
		if (read(bdm_fd, &buf, 2) != 2) {
			return got_size;
		}
		bdmlib_conv_buf_to_short(buf, &us, 1);
		bl_ptr = bdmlib_conv_short_to_char(bl_ptr, us, 1);
		got_size += 2;
	}
	if (size - got_size) {
		dump_cmd = BDM_DUMP_CMD | BDM_SIZE_BYTE;
		write(bdm_fd, &dump_cmd, 2);
		if (read(bdm_fd, &buf, 2) != 2) {
			return got_size;
		}
		*bl_ptr++ = *buf & 0xff;
		got_size += 1;
	}
	if (size - got_size) {	  /* cannot happen */
		error("internal error: bdmlib_read_block - cannot happen");		  
	}
	
	return got_size;
}

void
bdmlib_propeller(u_long addr, FILE * fp)
{
static char *str = "\\|/-";
static int index;
	
	if (!hashmark) return;
	fprintf(fp, "%c 0x%08lx\b\b\b\b\b\b\b\b\b\b\b\b", str[index++], addr);
	fflush(fp);
	index %= 4;
}

/*
* Open a connection the target via bdm
* name is the devicename of bdm and the filename to be used
* used for communication.
*/
int
bdmlib_open(char *name)
{
int ret;
int version;

	if (bdmlib_isopen()) {
		dbprintf("bdmlib_open: tried to open twice\n");
		return BDM_ERR_OPEN;
	}
	if ((bdm_fd = open(name, O_RDWR)) < 0) {
		dbprintf(
"bdmlib_open: Warning trouble on opening %s: reply %d errno %d\n", 
			name, bdm_fd, errno);
		fprintf(stderr, "Warning: trouble on opening %s: %s\n", name,
			bdmlib_geterror_str(-errno));
		bdm_fd = 0;				/* mark unused */
		return BDM_FAULT_PORT;
	}
	bdmlib_ioctl(BDM_INIT);
	if (bdm_delay>=0) {
		/* delay has been set before */
		bdmlib_setioctl(BDM_SPEED, bdm_delay);
	} else {
		bdmlib_setioctl(BDM_SPEED, BDM_DEFAULT_DELAY);
	}

	if (((ret = ioctl(bdm_fd, BDM_GET_VERSION, (u_long) &version)) < 0) ||
		(version < BDMLIB_REQUIRED_DRIVER_VERSION)) {
			if (ret < 0) version = 1;
			fprintf(stderr, "Error:\tBDM device driver version conflict.\n");
			fprintf(stderr, 
"\tyou need at least version %d, currently installed is version %d.\n", 
				BDMLIB_REQUIRED_DRIVER_VERSION, version);
			fprintf(stderr, "\tAborting operation\n");
			close(bdm_fd);
			return BDM_ERR_VERSION;
	}
	return BDM_NO_ERROR;
}

int
bdmlib_close(quitting)
int quitting;
{
	dbprintf("bdmlib_close: quitting %d\n", quitting);

	if (quitting) {
		bdmlib_reset();
		bdmlib_ioctl(BDM_RELEASE_CHIP);
	}
	close(bdm_fd);
	bdm_fd = 0;
	return BDM_NO_ERROR;
}

#define swap_l(x) (x>>24) | ((x>>8)&0xff00) | ((x<<8)&0xff0000) | ((x&0xff)<<24)

void
bdmlib_showpc(void)
{
	u_int sr, usp, ssp, vbr;
	u_int pcc, rpc, a7;

	bdmlib_get_sys_reg(BDM_REG_SR, &sr);
	bdmlib_get_sys_reg(BDM_REG_USP, &usp);
	bdmlib_get_reg(BDM_REG_A7, &a7);
	bdmlib_get_sys_reg(BDM_REG_SSP, &ssp);
	printf(" SR: %9x USP: %9x  A7: %9x SSP: %9x\n",
		   swap_l(sr), swap_l(usp), swap_l(a7), swap_l(ssp));

	bdmlib_get_sys_reg(BDM_REG_PCC, &pcc);
	bdmlib_get_sys_reg(BDM_REG_RPC, &rpc);
	bdmlib_get_sys_reg(BDM_REG_VBR, &vbr);
	printf("PCC: %9x RPC: %9x VBR: %9x\n",
			swap_l(pcc), swap_l(rpc), swap_l(vbr));
}

#define	LINE_LEN	256
static char *
get_line(FILE * f, int *line_nr)
{
	static char line_buf[LINE_LEN];

	while (!feof(f)) {
		fgets(line_buf, LINE_LEN, f);
		(*line_nr)++;
		if (line_buf[0] == '#')
			continue;
		if (line_buf[0] == '\n')
			continue;
		if (line_buf[0] == '\0')
			continue;
		return line_buf;
	}
	return NULL;
}

/* 
 * retval is just if we found the macro file, no errors!
 */
int 
bdmlib_do_load_macro(char *file_name, int which_suffix)
{
	char m_name[256];
	FILE *m_file;
	char *lptr;
	char cmd;
	int line_nr, ret, size;
	bfd_vma addr1, addr2;
	short size_tag;
	u_char *buf;
	int errorcount = 0;
	char * begin_suffix = ".bdmmb", *end_suffix = ".bdmme", *no_suffix = "";
	char * suffix;


	dbprintf("bdmlib_do_load_macro %s suffix %d\n", file_name, which_suffix);
	strcpy(m_name, file_name);
	if ((lptr = rindex(m_name, '.')) != 0) {
		*lptr = '\0';
	}
	switch (which_suffix) {
		case BEGIN_MACRO: suffix = begin_suffix; break;
		case END_MACRO: suffix = end_suffix; break;
		case NO_SUFFIX_MACRO: suffix = no_suffix; break;
		default: suffix = no_suffix;
	}
	strcat(m_name, suffix);
	if ((m_file = fopen(m_name, "r")) == NULL) {
		dbprintf("\tno macro file found\n");
		return BDM_ERR_MACROFILE;	/* no macro file available, quit silently */
	}
	if (!(bdmlib_getstatus() & BDM_TARGETSTOPPED)) {
		dbprintf("\twarning: chip needs stopping\n");
		bdmlib_ioctl(BDM_STOP_CHIP);
	}
	line_nr = 0;
	while ((lptr = get_line(m_file, &line_nr))) {
		cmd = *lptr++;
		addr1 = strtoul (lptr, &lptr, 0);
		addr2 = strtoul (lptr, &lptr, 0);
		size = strtol (lptr, &lptr, 0);
		bdmlib_propeller(addr1, stdout);
		switch (toupper(cmd)) {
		  case 'W':
			  dbprintf("\twrite to addr %#x cont %#x size %d\n",
					  addr1, addr2, size);
			  switch (size) {
				case 1:
					size_tag = BDM_SIZE_BYTE;
					size = 8;
					break;
				case 2:
					size_tag = BDM_SIZE_WORD;
					size = 8;
					break;
				case 4:
					size_tag = BDM_SIZE_LONG;
					size = 10;
					break;
				default:
					error("\
Error in processing macro %s line %d 'W' command:\n\t\
size must be either 1,2,4 bytes, '%d' is not allowed",
						  m_name, line_nr, size);
					continue;
			  }
			  if ((ret = bdmlib_write_var((caddr_t) addr1, size_tag, (u_int) addr2)) < 0) {
				  errorcount++;
				  dbprintf("\
Error in processing macro %s line %d 'W' command:\n\t\
bdm_write size mismatch: send %d returned %d errno %d",
						  m_name, line_nr, size, ret, errno);
			  }
			  break;
		  case 'C':
			  dbprintf("\tcopy from %#x to %#x size %d\n",
					  addr1, addr2, size);
			  buf = xmalloc(size);
			  if ((ret = bdmlib_read_block((caddr_t) addr1, size, buf)) != size) {
				  errorcount++;
				  error("\
Error in processing macro %s line %d 'C' command:\n\t\
bdm_copy size mismatch on read from %#x: wanted %d got %d",
						m_name, line_nr, addr1, size, ret);
			  } else {
				  {
					  int i;

					  for (i = 0; i < size; i++) {
						  if (!(i % 16))
							  printf("\n0x%08X: ", (unsigned int) addr1 + i);
						  printf("0x%02X ", buf[i]);
					  }
					  printf("\n");
				  }
				  if ((ret = bdmlib_write_block((caddr_t) addr2, size, buf)) != size) {
					  errorcount++;
					  error("\
Error in processing macro %s line %d 'C' command:\n\t\
bdm_copy size mismatch on write to %#x: wanted %d got %d",
							m_name, line_nr, addr2, size, ret);
				  }
			  }
			  free(buf);
			  break;
		  case 'Z':
			  dbprintf("\tzero from %#x with %#x size %#x\n",
					  addr1, addr2 & 0xff, size);
			  buf = xmalloc(size);
			  memset(buf, addr2, size);
			  if ((ret = bdmlib_write_block((caddr_t) addr1, size, buf)) != size) {
				  errorcount++;
				  error("\
Error in processing macro %s line %d 'Z' command:\n\t\
bdm_set size mismatch on write to %#x: wanted %d got %d",
						m_name, line_nr, addr1, size, ret);

			  }
			  free(buf);
			  break;
		  case 'M':
			  dbprintf("\tMBAR setup to %#x\n", addr1);
			  if ((ret = bdmlib_set_mbar((u_long) addr1))) {
				  errorcount++;
				  error("\
Error in processing macro %s line %d 'M' command:\n\t\
MBAR set returned %d",
						m_name, line_nr, ret);

			  }
			  break;
#if DEBUGGING_MACROS
		  case 'T':
			  {
				  int i;

				  buf = xmalloc(size);
				  for (i = 0; i < size; i++)
					  buf[i] = i;
				  bdmlib_write_block(addr1, size, buf);
				  free(buf);
			  }
			  break;
#endif		  /* DEBUGGING_MACROS */
		  default:
			  dbprintf("\tcmd %c unknown\n", cmd);
		}
	}
	if (hashmark) {
		printf("            \b\b\b\b\b\b\b\b\b\b\b\b");
		fflush(stdout);
	}
	if(m_file) fclose(m_file);
	dbprintf("\tfinished macro. errorcount %d\n", errorcount);
	return 0;	
		/*
		 * dont give back errorcount, errors will show up on binary
		 * download also.
		 */
}

bdmlib_bfilt_t * bdmlib_bfilt=NULL;

int
bdmlib_wrb_filt(bdmlib_bfilt_t * filt, caddr_t in_adr,
		u_int size, u_char * bl_ptr)
{
	int ret, part_ret;
	u_int part_size;	
	if (!size) return 0;

	while (filt) {
		if((in_adr<=filt->end_adr)&&
		   (in_adr+size-1>=filt->begin_adr)) {
			ret = 0;
			if(in_adr<filt->begin_adr) {
				part_size=filt->begin_adr-in_adr;
				part_ret=bdmlib_wrb_filt(filt->next,in_adr,part_size,bl_ptr);
				ret+=part_ret;
				if(part_ret!=part_size) return ret;
				in_adr+=part_size;
				bl_ptr+=part_size;
				size-=part_size;
			}
			part_size=filt->end_adr-in_adr+1;
			if (part_size>=size) {
				part_size=size; 
				size=0;
			} else size-=part_size;
			part_ret=filt->wrb_filt(filt,in_adr,part_size,bl_ptr);
			ret+=part_ret;
			if(part_ret!=part_size) {
				dbprintf("write error on filt write, address 0x%lx size %d acknowledged %d errno %d\n",
						(long)in_adr, part_size, part_ret, errno);
				return ret;
			}
			in_adr+=part_size;
			bl_ptr+=part_size;
			if(!size) return ret;
			ret+=bdmlib_wrb_filt(filt->next,in_adr,size,bl_ptr);
			return ret;
		}
		filt=filt->next;
	}
	/* regular memory block write */
	if ((ret = bdmlib_write_block(in_adr, size, bl_ptr)) != size) {
		dbprintf("write error on block write, written %d acknowledged %d errno %d\n",
				size, ret, errno);
		return ret;
	}
	return ret;
}

int bdmlib_load_use_lma=0;
static int load_section_error;

static void
bdmlib_load_section(bfd * abfd, sec_ptr sec, PTR ignore)
{
	u_long addr;
	u_long dfc;
	u_long load_bytes;
	file_ptr offset;
	int ret, cnt;
	char cbuf[512];

	dbprintf("bdmlib_load_section:\n\tsection %s index %d\n",
			sec->name, sec->index);
	dbprintf("\tflags %#x raw_size 0x%08x cooked_size 0x%08x\n",
			sec->flags, sec->_raw_size, sec->_cooked_size);
	dbprintf("\tvma %#x lma %#x output_offset %#x\n",
			sec->vma, sec->lma, sec->output_offset);
	if ((load_section_error < 0) ||
		((bfd_get_section_flags(abfd, sec) & SEC_LOAD) == 0)) {
		return;
	}
	if (bfd_get_section_flags(abfd, sec) & SEC_CODE)
		dfc = 0x6;				/* Supervisor program space */
	else
		dfc = 0x5;				/* Supervisor data space */
	if ((ret = bdmlib_set_sys_reg(BDM_REG_DFC, dfc)) < 0) {
		load_section_error = ret;
		return;
	}
	if(!bdmlib_load_use_lma)
		addr = bfd_section_vma(abfd, sec);
	else
		addr = bfd_section_lma(abfd, sec);

	/* there used to be bfd_get_section_size_before_reloc() */
	if ((load_bytes = bfd_section_size(abfd,sec)) == 0)
		return;
	offset = 0;
	while (load_bytes) {
		if (load_bytes > sizeof(cbuf)) {
			cnt = sizeof(cbuf);
		} else {
			cnt = load_bytes;
		}
		if (!bfd_get_section_contents(abfd, sec, cbuf, offset, cnt)) {
			dbprintf("read error section %s\n", sec->name);
			load_section_error = BDM_ERR_LOAD;
			return;
		}
		if ((ret = bdmlib_wrb_filt(bdmlib_bfilt,(caddr_t) addr, cnt, cbuf)) != cnt) {
			dbprintf("write error on block write, written %d acknowledged %d errno %d\n",
					cnt, ret, errno);
			load_section_error = BDM_ERR_LOAD;
			return;
		}

		load_bytes -= cnt;
		addr += cnt;
		offset += cnt;
		bdmlib_propeller(addr, stdout);
	}
}

static int bfd_initialized;
static bfd * prepare_binary(char *file_name)
{
	bfd *abfd;

	if (!bfd_initialized) {
		bfd_init();
		bfd_initialized = 1;
	}
	abfd = bfd_openr(file_name, 0);
	if (!abfd) {
		dbprintf("Unable to open file %s\n", file_name);
		return NULL;
	}
	if (bfd_check_format(abfd, bfd_object) == 0) {
		dbprintf("File %s is not an object file\n", file_name);
		bfd_close(abfd);
		return NULL;
	}
	return abfd;
}

/*
 * load a binary file, returns error codes
 */
int
bdmlib_do_load_binary(char *file_name, char *entry_name, u_long *entry_pt)
{
	bfd *abfd;
	int ret;
	u_long entry_addr = 0;

	if ((abfd = prepare_binary(file_name)) == NULL) 
		return BDM_ERR_OPEN;

	if (entry_name)
	    while (isspace(*entry_name))
		entry_name++;

	if (entry_name && isdigit(*entry_name)) {
	    entry_addr = strtoul (entry_name, NULL, 0);
	} else if (entry_name) {
	    long i, symcnt;
	    asymbol **symtab;
	    if ((i=bfd_get_symtab_upper_bound(abfd))<=0 || !(symtab=malloc(i)))
		return BDM_ERR_OPEN;
	    if ((symcnt = bfd_canonicalize_symtab (abfd, symtab))<0)
		return BDM_ERR_OPEN;

	    for (i=0; i<symcnt; i++) {
		if (!strcmp (entry_name, symtab[i]->name)) {
		    entry_addr = symtab[i]->section->vma + symtab[i]->value;
		    break;
		}
	    }
	} else {
	  entry_addr = bfd_get_start_address(abfd);
	}
	if(entry_pt) *entry_pt = entry_addr;

	load_section_error = BDM_NO_ERROR;
	if (!(bdmlib_getstatus() & BDM_TARGETSTOPPED) && 
		 ((ret = bdmlib_ioctl(BDM_STOP_CHIP)))) {
		dbprintf("bdmlib_do_load_binary: %d %s\n", 
			errno, bdmlib_geterror_str(-errno));
		return -errno;
	}
	bfd_map_over_sections(abfd, bdmlib_load_section, NULL);
	if (hashmark) {
		printf(" .          \b\b\b\b\b\b\b\b\b\b\b\b");
		fflush(stdout);
	}
	bfd_close(abfd);
	return load_section_error;
}

int
bdmlib_do_load_binary_section(char *file_name, char *section_name)
{
	bfd *abfd;
	asection *sec;
	int ret;

	if ((abfd = prepare_binary(file_name)) == NULL) 
		return BDM_ERR_OPEN;

	if ((sec = bfd_get_section_by_name(abfd, section_name)) == NULL)
		return BDM_ERR_SECTION;

	load_section_error = BDM_NO_ERROR;
	if (!(bdmlib_getstatus() & BDM_TARGETSTOPPED) && 
		 ((ret = bdmlib_ioctl(BDM_STOP_CHIP)))) {
		dbprintf("bdmlib_do_load_binary: %d %s\n", 
			errno, bdmlib_geterror_str(-errno));
		return -errno;
	}

	if (hashmark) {
		printf("  "); fflush(stdout);
	}
	bdmlib_load_section(abfd, sec, NULL);
	if (hashmark) {
		printf("\r         \r");
		fflush(stdout);
	}
	bfd_close(abfd);
	return load_section_error;
}

/*
 * Load a file.
 * This is supposed to be coff here, but bfd should handle other formats...
 *
 * use a macro for target specific manipulations, eg. setting
 * chipselect, switching memory banks, etc.
 */

int
bdmlib_load(char *file, char *entry_name, u_long * entry_pt)
{
	int ret = 0;
	u_int sfc, dfc;
	int we_have_macro_files = 0;
#if SUPPORT_RAMINIT
	u_long dummy;
#endif 

	if (!bdmlib_isopen())
		return BDM_ERR_NOT_OPEN;
	dbprintf("bdmlib_load: program name %s\n", file);

	if (bdm_autoreset) bdmlib_reset();
	if (!(bdmlib_getstatus() & BDM_TARGETSTOPPED))
		if ((ret = bdmlib_ioctl(BDM_STOP_CHIP))) {
			fprintf_filtered(gdb_stderr, "%s", bdmlib_geterror_str(ret));
			error("Download failed\n");
			return ret;	/* this is for non-gdb applications */
		}

	bdmlib_set_sys_reg(BDM_REG_DFC, 5);
	if ((ret = bdmlib_set_sys_reg(BDM_REG_SFC, 5)) < 0) {
		return ret;
	}
	bdmlib_get_sys_reg(BDM_REG_DFC, &dfc);
	bdmlib_get_sys_reg(BDM_REG_SFC, &sfc);
	dbprintf("\tDFC %x SFC %x\n", dfc, sfc);

	we_have_macro_files = !bdmlib_do_load_macro(file, BEGIN_MACRO);

#if SUPPORT_RAMINIT
	if ((ret = bdmlib_do_load_binary(RAMINIT_FILENAME, &dummy)) < 0) {
		error("ram_init load error.\n");
	}
#endif /* SUPPORT_RAMINIT */

	if ((ret = bdmlib_do_load_binary(file, entry_name, entry_pt)) < 0) {
		error("Download failed.");
		return ret;
	}
	if (we_have_macro_files)
		bdmlib_do_load_macro(file, END_MACRO);
	return BDM_NO_ERROR;

}

static FILE *debug_fp;

static int
open_debug(void)
{
	if ((debug_fp = fopen(BDM_DEBUG_NAME, "w")) == NULL) {
		perror("bdm-dbprintf");
		fprintf_filtered(gdb_stderr, "bdm: opening debug %s\n",
			BDM_DEBUG_NAME);
		return -1;
	}
	return 0;
}

/* debug printf for Log-file */
void 
dbprintf(const char *format, ...)
{
	va_list  ap;

	if (bdm_flags & BDM_DODPRINTF) {
		if ((!debug_fp) && open_debug()) return;
		va_start(ap, format);
		vfprintf(debug_fp, format, ap); fflush(debug_fp);
		va_end(ap);
	} else if (debug_fp) {
		fclose(debug_fp);
		debug_fp = NULL;
	}
}

void
bdmlib_log(const char *format, ...)
{

	va_list ap;

/* 
 * this would normally call dbprintf(); however it seems not possible
 * to do nested varargs calls....
 */

	if (bdm_flags & BDM_DODPRINTF) {
		if ((!debug_fp) && open_debug()) return;
		va_start(ap, format);
		fprintf(debug_fp, "%s: ", 
#if defined BDMLIB_FORGDB
	"GDB"
#elif defined BDMLIB_FORFLT
	"FLT"
#else
	"???"
#endif
		);
		vfprintf(debug_fp, format, ap); fflush(debug_fp);
		va_end(ap);
	} else if (debug_fp) {
		fclose(debug_fp);
		debug_fp = NULL;
	}
}
