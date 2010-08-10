/* @#Copyright:
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

/* File:	BDMTargetAddress.c 
 * Purpose:	Abstract manipulation of target address space through BDM.
 * Author:	Rolf Fiedler, Brett Wuth
 * Created:	2000-03-27
 *
 * Initials:
 *	BCW - Brett Wuth
 *		@#[ContactWuth:
 *		Phone: +1 403 627-2460
 *		E-mail: support@castrov.cuug.ab.ca, wuth@acm.org]
 *
 * HISTORY:
 * $Log: BDMTargetAddress.c,v $
 * Revision 1.3  2008/06/16 00:01:21  cjohns
 * 2008-06-08  Chris Johns <cjohns@users.sourceforge.net>
 *
 * 	* libelf/COPYING.LIB, libelf/ChangeLog, libelf/INSTALL,
 * 	libelf/MANIFEST, libelf/Makefile.in, libelf/README,
 * 	libelf/VERSION, libelf/acconfig.h, libelf/aclocal.m4,
 * 	libelf/config.guess, libelf/config.h.in, libelf/config.sub,
 * 	libelf/configure, libelf/configure.in, libelf/install-sh,
 * 	libelf/libelf.pc.in, libelf/mkinstalldirs, libelf/stamp-h.in,
 * 	libelf/lib/32.fsize.c, libelf/lib/32.getehdr.c,
 * 	libelf/lib/32.getphdr.c, libelf/lib/32.getshdr.c,
 * 	libelf/lib/32.newehdr.c, libelf/lib/32.newphdr.c,
 * 	libelf/lib/32.xlatetof.c, libelf/lib/64.xlatetof.c,
 * 	libelf/lib/Makefile.in, libelf/lib/Makefile.w32,
 * 	libelf/lib/assert.c, libelf/lib/begin.c, libelf/lib/build.bat,
 * 	libelf/lib/byteswap.h, libelf/lib/checksum.c, libelf/lib/cntl.c,
 * 	libelf/lib/config.h.w32, libelf/lib/cook.c, libelf/lib/data.c,
 * 	libelf/lib/elf_repl.h, libelf/lib/end.c, libelf/lib/errmsg.c,
 * 	libelf/lib/errno.c, libelf/lib/errors.h, libelf/lib/ext_types.h,
 * 	libelf/lib/fill.c, libelf/lib/flag.c, libelf/lib/gelf.h,
 * 	libelf/lib/gelfehdr.c, libelf/lib/gelfphdr.c,
 * 	libelf/lib/gelfshdr.c, libelf/lib/gelftrans.c,
 * 	libelf/lib/getarhdr.c, libelf/lib/getarsym.c,
 * 	libelf/lib/getbase.c, libelf/lib/getdata.c, libelf/lib/getident.c,
 * 	libelf/lib/getscn.c, libelf/lib/hash.c, libelf/lib/input.c,
 * 	libelf/lib/kind.c, libelf/lib/libelf.def, libelf/lib/libelf.h,
 * 	libelf/lib/memset.c, libelf/lib/ndxscn.c, libelf/lib/newdata.c,
 * 	libelf/lib/newscn.c, libelf/lib/next.c, libelf/lib/nextscn.c,
 * 	libelf/lib/nlist.c, libelf/lib/nlist.h, libelf/lib/opt.delscn.c,
 * 	libelf/lib/private.h, libelf/lib/rand.c, libelf/lib/rawdata.c,
 * 	libelf/lib/rawfile.c, libelf/lib/strptr.c, libelf/lib/swap64.c,
 * 	libelf/lib/sys_elf.h.in, libelf/lib/sys_elf.h.w32,
 * 	libelf/lib/update.c, libelf/lib/verdef.h,
 * 	libelf/lib/verdef_32_tof.c, libelf/lib/verdef_32_tom.c,
 * 	libelf/lib/verdef_64_tof.c, libelf/lib/verdef_64_tom.c,
 * 	libelf/lib/verneed.h, libelf/lib/version.c, libelf/lib/x.elfext.c,
 * 	libelf/lib/x.movscn.c, libelf/lib/x.remscn.c,
 * 	libelf/po/Makefile.in, libelf/po/de.gmo, libelf/po/de.msg,
 * 	libelf/po/de.po, libelf/po/gmo2msg.c, libelf/po/libelf.pot,
 * 	libelf/po/stamp-po: Merge libelf into the BDM package.
 *
 * 	* configure.ac, utils/Makefile.am, utils/bdmctrl.c,
 * 	flashlib/Makefile.am, flashlib/bdmfilt.c, flashlib/bdmfilt.h,
 * 	flashlib/bdmflash.c, flashlib/bdmflash.h, flashlib/flash29.c,
 * 	flashlib/flash_filter.c, flashlib/flash_filter.h: Remove all BFD
 * 	references and change to ELF file support.
 *
 * 	* flashlib/elf-utils.c, flashlib/elf-utils.h: New.
 *
 * 	* driver/bdm.h, driver/bdm-tblcf.c: Add the TBLCF interface
 * 	number.
 *
 * 	* bdmabstraction/BDMTargetAddress.c: Add a long write call.
 *
 * 	* gdbserver/Makefile.am: Fix the XML to C regen rule.
 *
 * 2008-06-08  Matthew Riek <matthew.riek@ibiscomputer.com.au>
 *
 * 	* flashlib/flashcfm.c, flashlib/flashcfm.h,
 * 	flashlib/compile_flashcfm, utils/mcf52235.test.
 *
 * Revision 1.2  2005/10/24 01:37:25  cjohns
 * Fixed includes for building in Windows with MinGW.
 *
 * Revision 1.1  2003/12/29 22:18:49  codewiz
 * Move tools/bdm_abstraction to m68k/bdmabstraction and autoconfiscate.
 *
 * Revision 1.1  2003/06/03 15:42:04  codewiz
 * Import userland tools from bdm-fiedler
 *
 * Revision 1.4  2000/09/19 00:28:29  wuth
 * cleanly use Fiedler's bdm driver; bdm_mon detects flash errors
 *
 * Revision 1.3  2000/07/25 13:51:09  wuth
 * Working sector erase.  Better error reports.
 *
 * Revision 1.2  2000/04/20 04:56:23  wuth
 * GPL.  Abstract flash interface.
 *
 * Revision 1.1  2000/03/28 20:24:41  wuth
 * Break out flash code into separate executable.  Make run under Chris Johns BDM driver.
 *
 * Based on revision Wuth1 of bdm-fiedler/debug/bdm_abstraction/bdmops.c.
 * @#[BasedOnTemplate: template.c version 2]
 */

#include <BDMTargetAddress.h>

#include <assert.h>
#include <BDMDriver.h>
#include <Debug.h>
#include <stdio.h>

#define PRINTDTRACE()	PRINTD( __FILE__ "(%d)\n", __LINE__ )

/* Write byte into target address space through BDM */
int /* 0 if success, or error code */
BDMTargetByteWrite( int fd, unsigned int addr, unsigned char byte )
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    unsigned long word[5];
    unsigned long poll;
    int ret;
    
    word[0]=BDM_WRITE_CMD | BDM_SIZE_BYTE;
    word[1]=(addr>>16);
    word[2]=addr & 0xffff;
    word[3]=byte;
    word[4]=BDM_NOP_CMD;
    ret=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(word), (unsigned long)&word);
    if(ret<0)
      {
	PRINTDTRACE();
	return ret;
      }
    if(word[4]==BDM_BERR) {
	/* get BDM_NOT_READY after BERR */
	unsigned long poll;
	poll = BDM_NOP_CMD;
	bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	PRINTDTRACE();
	return BDM_FAULT_BERR;
    }
    if(word[4]==BDM_ILLEGAL)
      {
	PRINTDTRACE();
	return BDM_FAULT_NVC;
      }
    poll=word[4];
    while(poll==BDM_NOTREADY) {
	poll = BDM_NOP_CMD;
	ret=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	if(ret<0)
	  {
	    PRINTDTRACE();
	    return ret;
	  }
	if(poll==BDM_BERR) {
	    /* get BDM_NOT_READY after BERR */
	    unsigned long poll;
	    poll = BDM_NOP_CMD;
	    bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	    PRINTDTRACE();
	    return BDM_FAULT_BERR;
	}
	if(poll==BDM_ILLEGAL)
	  {
	    PRINTDTRACE();
	    return BDM_FAULT_NVC;
	  }
    }
    return 0;
#else
    int Ret = bdmWriteByte( addr, byte );
    if (Ret)
      PRINTDTRACE();
    return (Ret);
#endif
}

/* Write word into target address space through BDM */
int /* 0 if success, or error code */
BDMTargetWordWrite( int fd, unsigned int addr, unsigned short word )
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    unsigned long words[5];
    unsigned long poll;
    int ret;
    
    words[0]=BDM_WRITE_CMD | BDM_SIZE_WORD;
    words[1]=(addr>>16);
    words[2]=addr & 0xffff;
    words[3]=word;
    words[4]=BDM_NOP_CMD;
    ret=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(words), (unsigned long)&words);
    if(ret<0)
      {
	PRINTDTRACE();
	return ret;	    
      }
    if(words[4]==BDM_BERR) {
	/* get BDM_NOT_READY after BERR */
	unsigned long poll;
	poll = BDM_NOP_CMD;
	bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	PRINTDTRACE();
	return BDM_FAULT_BERR;
    }
    if(words[4]==BDM_ILLEGAL)
      {
	PRINTDTRACE();
	return BDM_FAULT_NVC;
      }
    poll=words[4];
    while(poll==BDM_NOTREADY) {
	poll = BDM_NOP_CMD;
	ret=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	if(ret<0)
	  {
	    PRINTDTRACE();
	    return ret;
	  }
	if(poll==BDM_BERR) {
	    /* get BDM_NOT_READY after BERR */
	    unsigned long poll;
	    poll = BDM_NOP_CMD;
	    bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	    PRINTDTRACE();
	    return BDM_FAULT_BERR;
	}
	if(poll==BDM_ILLEGAL)
	  {
	    PRINTDTRACE();
	    return BDM_FAULT_NVC;
	  }
    }
    return 0;
#else
    int Ret = bdmWriteWord( addr, word );
    if (Ret)
      PRINTDTRACE();
    return (Ret);
#endif
}

/* Write long into target address space through BDM */
int /* 0 if success, or error code */
BDMTargetLongWrite( int fd, unsigned int addr, unsigned long Long )
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    assert( 0 ); /* not implemented */
#else
    int Ret = bdmWriteLongWord( addr, Long );
    if (Ret)
      PRINTDTRACE();
    return (Ret);  
#endif
    return (-1);
}

/* Read byte into target address space through BDM */
int /* <0 if error, else byte */
BDMTargetByteRead( int fd, unsigned int addr )
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    unsigned long word[4];
    unsigned long poll;
    int ret;
    
    word[0]=BDM_READ_CMD | BDM_SIZE_BYTE;
    word[1]=(addr>>16) & 0xffff;
    word[2]=addr & 0xffff;
    word[3]=BDM_NOP_CMD;
    ret=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(word), (unsigned long)&word);
    if(ret<0)
      {
	PRINTDTRACE();
	return ret;	    
      }
    if(word[3]==BDM_BERR) {
	/* get BDM_NOT_READY after BERR */
	unsigned long poll;
	poll = BDM_NOP_CMD;
	bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	PRINTDTRACE();
	return BDM_FAULT_BERR;
    }
    if(word[3]==BDM_ILLEGAL)
      {
	PRINTDTRACE();
	return BDM_FAULT_NVC;
      }
    poll=word[3];
    while(poll==BDM_NOTREADY) {
	poll = BDM_NOP_CMD;
	ret=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	if(ret<0)
	  {
	    PRINTDTRACE();
	    return ret;
	  }
	if(poll==BDM_BERR) {
	    /* get BDM_NOT_READY after BERR */
	    unsigned long poll;
	    poll = BDM_NOP_CMD;
	    bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	    PRINTDTRACE();
	    return BDM_FAULT_BERR;
	}
	if(poll==BDM_ILLEGAL)
	  {
	    PRINTDTRACE();
	    return BDM_FAULT_NVC;
	  }
    }
    return poll & 0xff;
#else
    unsigned char Byte;

    if (bdmReadByte( addr, &Byte ) != 0)
      {
	PRINTDTRACE();
	return (-1);
      }
    return Byte;
#endif
}

/* Read word into target address space through BDM */
int /* <0 if error, else word */
BDMTargetWordRead( int fd, unsigned int addr )
{
#if BDMDriverVersion_M == BDMDriverFiedler_M
    unsigned long words[4];
    unsigned long poll;
    int ret;
    
    words[0]=BDM_READ_CMD | BDM_SIZE_WORD;
    words[1]=(addr>>16) & 0xffff;
    words[2]=addr & 0xffff;
    words[3]=BDM_NOP_CMD;
    ret=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(words), (unsigned long)&words);
    if(ret<0)
      {
	PRINTDTRACE();
	return ret;	    
      }
    if(words[3]==BDM_BERR) {
	/* get BDM_NOT_READY after BERR */
	unsigned long poll;
	poll = BDM_NOP_CMD;
	bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	PRINTDTRACE();
	return BDM_FAULT_BERR;
    }
    if(words[3]==BDM_ILLEGAL)
      {
	PRINTDTRACE();
	return BDM_FAULT_NVC;
      }
    poll=words[3];
    while(poll==BDM_NOTREADY) {
	poll = BDM_NOP_CMD;
	ret=bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	if(ret<0)
	  {
	    PRINTDTRACE();
	    return ret;
	  }
	if(poll==BDM_BERR) {
	    /* get BDM_NOT_READY after BERR */
	    unsigned long poll;
	    poll = BDM_NOP_CMD;
	    bdm_ioctl(fd, BDM_XCHG_DATA_IOC(poll), (unsigned long)&poll);
	    PRINTDTRACE();
	    return BDM_FAULT_BERR;
	}
	if(poll==BDM_ILLEGAL)
	  {
	    PRINTDTRACE();
	    return BDM_FAULT_NVC;
	  }
    }
    return poll & 0xffff;
#else
    unsigned short Word;

    if (bdmReadWord( addr, &Word ) != 0)
      {
	PRINTDTRACE();
	return (-1);
      }
    return (Word);
#endif
}


/*EOF*/
