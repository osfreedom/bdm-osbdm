#ifndef Flash_Included_M
#define Flash_Included_M

/* @#Copyright:
 * Copyright (c) 1999, Rolf Fiedler.
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

/* File:	Flash.h
 * Purpose:	Algorithm for manipulating flash independent of access
 *              method.
 * Author:	Brett Wuth
 * Created:	2000-04-15
 *
 * Initials:
 *	BCW - Brett Wuth
 *		@#[ContactWuth:
 *		Phone: (403) 627-2460
 *		E-mail: support@castrov.cuug.ab.ca, wuth@acm.org]
 *
 * #@[MultiProject:
 * MultiProject@castrov.cuug.ab.ca Flash Flash.h
 * MultiProject@castrov.cuug.ab.ca CVSROOT=:ext:wuth@hulk.adomo.com:/home/cvs linux-development/bdm-fiedler debug/bdm_abstraction/Flash.h
 * MultiProject@castrov.cuug.ab.ca CVSROOT=:ext:wuth@hulk.adomo.com:/home/cvs linux-development/ThinClientFlashWrite Flash.h
 * 
 * This file is shared among multiple projects.  The normal way to so
 * is to create a separate project and have this project depend on it,
 * perhaps linking to a library.  But for whatever reason this project
 * requires the file to be included with it.
 *
 * To keep your copy of this file in sync with the other copies add
 * your e-mail address, the project name and file path to the list
 * above.  E-mail a copy of the file to the first address in the list
 * now and whenever there are changes.]
 * 
 * HISTORY:
 * $Log: Flash.h,v $
 * Revision 1.1  2003/06/03 15:42:04  codewiz
 * Import userland tools from bdm-fiedler
 *
 * Revision 1.4  2000/08/03 06:29:18  wuth
 * MultiProject Sync; Support Micron-style flash; Report flash model
 *
 * Revision 1.3  2000/07/25 13:51:09  wuth
 * Working sector erase.  Better error reports.
 *
 * Revision 1.2  2000/07/14 18:38:55  wuth
 * Flash error status; Fix sector erase support; Command line sector erase
 *
 * Revision 1.1  2000/04/20 04:56:23  wuth
 * GPL.  Abstract flash interface.
 *
 * @#[BasedOnTemplate: template.h version 2]
 */

#include <stdlib.h> /* size_t */

typedef enum FlashError_en {
  FlashErrorOkay_c, /* no error */
  FlashErrorReadAccess_c, /* can't read flash */
  FlashErrorWriteAccess_c, /* can't write to flash */

  FlashErrorTimeOut_c, /* flash's internal program timed out */
  FlashErrorEraseFail_c, /* flash's internal erase program failed */
  FlashErrorWriteFail_c, /* flash's internal write program failed */
  FlashErrorProtected_c, /* flash is protected from modification */

  /* flash produced contents that are unexpected */
  FlashErrorUnexpected_c,

  /* the specified width of the flash is invalid or not supported */
  FlashErrorWidth_c,

  /* the style of flash is not supported */
  FlashErrorStyle_c,

  FlashErrorCount_c /* number of Flash Errors */
} FlashError_t;


extern char const *FlashErrorDescriptionEnglish[FlashErrorCount_c];


typedef enum FlashStyle_en {
  FlashStyleUndefined_c,
  FlashStyleAMD_c,
  FlashStyleMicron_c,
  FlashStyleCount_c /* number of Flash Styles */
} FlashStyle_t;

extern char const *FlashStyleName[FlashStyleCount_c];


typedef unsigned short FlashID_t;


typedef enum FlashManufacturer_en {
  FlashManufacturerAMD_c = 0x01,
  FlashManufacturerFujitsu_c = 0x04,
  FlashManufacturerSGSThomson_c = 0x20,
  FlashManufacturerAtmel_c = 0x1F,
  FlashManufacturerMicron_c = 0x89,
  FlashManufacturerTexasInstruments_c = 0x97,
  FlashManufacturerHyundai_c = 0xAD,
  FlashManufacturerSST_c = 0xBF,
} FlashManufacturer_t;


char const *
FlashManufacturerName( FlashManufacturer_t Manufacturer );


typedef 
FlashError_t
(*FlashElementReadFunc_t)( void *UserData, 
			   unsigned long Location, 
			   unsigned int BytesWide, /* what type of read */
			   void /*out*/ *Element );

typedef
FlashError_t
(*FlashElementWriteFunc_t)( void *UserData,
			    unsigned long Location,
			    unsigned int BytesWide, /* what type of write */
			    void const *Element );


/* Parameters passed to flash routines.
 * This is object-based coding to reduce the proliferation
 * of arguments passed to each function.
 */
typedef struct Flash_s {
  /* Pointer to data which is used by generic Read and Write
   * functions.  The actual data is of a type specific to the
   * particular functions defined.  */
  void *UserData; 

  /* Generic function read a single element out of the array of
   * interlaced flash chips.  The size of an element is defined by
   * ChipWidthBytes.  */
  FlashElementReadFunc_t Read;

  /* Generic function read a single element out of the array of
   * interlaced flash chips.  The size of an element is defined by
   * ChipWidthBytes.  */
  FlashElementWriteFunc_t Write;

  /* The number of bytes written or read in a single access */
  unsigned int ChipWidthBytes;

  /* Whether the hardware accessing the flash is big-endian or
   * little-endian.  This is not necessarily the endianness of the CPU
   * on which this code is executing, because the generic Read and
   * Write functions may fetch the data through some other machine.
   * Such happens when an Intel-based computer manipulates the flash
   * through a BDM cable connected to a Motorola CPU.  The flash
   * provides status information on the least significant bits.  */
  int/*Bool*/ BigEndian;


  /* The number of flash chips of the same type which are interleaved.
   * For instance, if ChipWidthBytes == 2, one chip may provide bytes
   * 0, 1, 4, 5, 8, 9, etc.  And another chip provides bytes 2, 3, 6,
   * 7, 10, 11, etc.  */
  unsigned int NumParallelChips;

  /* The general type of the flash.  Different styles have radically
   * different algorithms for burning and erasing. */
  FlashStyle_t Style;
} Flash_t;


/* Initialiaze the Flash structure */
FlashError_t
FlashInit( Flash_t /*out*/ *Flash,
	   void *UserData,
	   FlashElementReadFunc_t Read,
	   FlashElementWriteFunc_t Write,
	   unsigned int ChipWidthBytes,
	   int/*Bool*/ BigEndian,
	   unsigned int NumParallelChips );


/* store into flash */
FlashError_t
FlashWrite( Flash_t const *Flash,
	    unsigned long Location, 
	    void const *Data,
	    size_t Size /*actual rounded up by ChipWidthBytes*/ );

/* erase the whole flash */
FlashError_t
FlashErase( Flash_t const *Flash );

/* erase one sector of the flash */
FlashError_t
FlashEraseSector( Flash_t const *Flash,
		  unsigned long OffsetInFlash /* sector identified by offset */ );

typedef struct FlashInfo_s {
  FlashID_t ID;
  unsigned long Size;
  unsigned long loend;  /* start of range of sectors all the same size */
  unsigned long hiend;  /* end of range of sectors all the same size */
  unsigned long sec_size;  /* size of sectors which are all the same size */
  char const *Model;
} FlashInfo_t;



/*sets Flash->Style */
FlashError_t
FlashProbe( Flash_t *Flash );


/* leaves flash in Array Read mode */
FlashError_t
FlashIDRead( Flash_t const *Flash,
	     FlashID_t /*out*/ *ID );


FlashError_t
FlashDetect( Flash_t const *Flash,
	     FlashInfo_t const * /*out*/ *FlashInfo );


#endif /* Flash_Included_M */
/*** Emacs configuration ***/
/* Local Variables: */
/* mode:C */
/* End: */
/*EOF*/
