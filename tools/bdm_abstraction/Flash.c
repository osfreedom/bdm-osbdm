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

/* File:	Flash.c
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
 *      JUS - Jens Ulrik Skakkebaek
 *              jskakkebaek@adomo.com
 *
 * #@[MultiProject:
 * MultiProject@castrov.cuug.ab.ca Flash Flash.c
 * MultiProject@castrov.cuug.ab.ca CVSROOT=:ext:wuth@hulk.adomo.com:/home/cvs linux-development/bdm-fiedler debug/bdm_abstraction/Flash.c
 * MultiProject@castrov.cuug.ab.ca CVSROOT=:ext:wuth@hulk.adomo.com:/home/cvs linux-development/ThinClientFlashWrite Flash.c
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
 * $Log: Flash.c,v $
 * Revision 1.1  2003/06/03 15:42:04  codewiz
 * Import userland tools from bdm-fiedler
 *
 * Revision 1.7  2000/08/03 06:29:18  wuth
 * MultiProject Sync; Support Micron-style flash; Report flash model
 *
 * Revision 1.6  2000/07/26 02:02:57  wuth
 * Inclued CHeaderFileValue script; MultiProject sync on Flash.*
 *
 * Revision 1.5  2000/07/25 15:45:23  wuth
 * Fix bug that limited sector erase to first sector
 *
 * Revision 1.4  2000/07/25 13:51:09  wuth
 * Working sector erase.  Better error reports.
 *
 * Error types.  Errors reported.  FlashOperationCompletedWait catches bad writes.  Debug code.
 * 
 * Revision 1.3  2000/07/14 18:38:55  wuth
 * Flash error status; Fix sector erase support; Command line sector erase
 *
 * Revision 1.2  2000/06/13 19:45:11  wuth
 * Delay needed for new board.
 *
 * Revision 1.2  2000/06/13 19:32:10  wuth
 * Delay needed by new board revision
 *
 * Revision 1.1  2000/04/20 05:08:37  wuth
 * ThinClientFlashWrite writes fast.
 *
 * Based on revision Wuth4 of bdm-fiedler/debug/bdm_abstraction/BDMFlash.c.
 * Based on revision Wuth1 of bdm-fiedler/debug/bdm_abstraction/bdmops.c.
 * @#[BasedOnTemplate: template.c version 2]
 */

#include <Flash.h>

#include <assert.h>

/* #define DEBUG */


char const *FlashErrorDescriptionEnglish[FlashErrorCount_c] = {
  "no error",
  "can't read flash",
  "can't write to flash",
  "flash's internal program timed out",
  "flash's internal erase program failed",
  "flash's internal write program failed",
  "flash is protected from modification",
  "flash produced contents that are unexpected",
  "the specified width of the flash is invalid or not supported",
  "the style of flash is not supported",
};


char const *
FlashManufacturerName( FlashManufacturer_t Manufacturer )
{
  switch (Manufacturer)
    {
    case FlashManufacturerAMD_c:
      return ("AMD");

    case FlashManufacturerFujitsu_c:
      return ("Fujitsu");

    case FlashManufacturerSGSThomson_c:
      return ("SGS Thomson");

    case FlashManufacturerAtmel_c:
      return ("Atmel");

    case FlashManufacturerMicron_c:
      return ("Micron");

    case FlashManufacturerTexasInstruments_c:
      return ("Texas Instruments");

    case FlashManufacturerHyundai_c:
      return ("Hyundai");

    case FlashManufacturerSST_c:
      return ("SST");

    default:
      return ("Unknown");
    }
}

#define FlashIDMake_M( Manufacturer, Device ) ((FlashID_t)((Manufacturer)<<8 | (Device)))


#define ChipWidthBytesMax_M (4) /* Maximum number of bytes wide supported */

static unsigned long const ULongFFFFFFFF = 0xFFFFFFFF;


/*******************************************************************************************
 * AMD-style specific code
 */

typedef enum FlashStyleAMDCommand_en {
  FlashStyleAMDCommandInvalidReserved_c = 0x00,
  FlashStyleAMDCommandChipEraseConfirm_c = 0x10,
  FlashStyleAMDCommandReserved_c = 0x20,
  FlashStyleAMDCommandBlockEraseResumeConfirm_c = 0x30,
  FlashStyleAMDCommandSetUpErase_c = 0x80,
  FlashStyleAMDCommandReadSignatureBlockStatus_c = 0x90,
  FlashStyleAMDCommandProgram_c = 0xA0,
  FlashStyleAMDCommandEraseSuspend_c = 0xB0,
  FlashStyleAMDCommandReadArrayReset_c = 0xF0,
} FlashStyleAMDCommand_t;



typedef unsigned char FlashStatus_t;

/***************************************************************/

#ifdef DEBUG
/* Number of times the status has been checked */
unsigned long DebugFlashStatusI = 0;

/* Number of times to check the status before we give up.
 * 0 means forever.
 */
unsigned long DebugFlashStatusCheckIterationsMax = 0;

/* Amount of history to keep */
#define DebugFlashStatusHistoryMax_M (3000)

/* Log of each status check result during loop. */
unsigned char DebugFlashStatusHistory[DebugFlashStatusHistoryMax_M][ChipWidthBytesMax_M];
FlashError_t DebugFlashStatusErrorHistory[DebugFlashStatusHistoryMax_M];

void
DebugBreakPoint( void )
{
  /* CPUHalt(); */
}
#else
#define DebugBreakPoint() ((void)0)
#endif


static
void
ElementFillPattern( unsigned char /*out*/ Element[],
		    unsigned int ChipWidthBytes /* Bytes wide each chip */,
		    unsigned char Pattern )
{
  unsigned int i;
  for (i = 0; i < ChipWidthBytes; i++)
    Element[i] = Pattern;
}


/* Write the same pattern across all the bits of the ChipWidthBytes
 * bus.
 */
static
FlashError_t
WritePattern( Flash_t const *Flash,
	      unsigned long Address,
	      unsigned char Pattern )
{
  unsigned char Element[ChipWidthBytesMax_M];
  ElementFillPattern( Element, Flash->ChipWidthBytes, Pattern );
  return (Flash->Write( Flash->UserData, Address, Flash->ChipWidthBytes, Element ));
}


/* Flash can be in either byte-wide or word-wide mode.  Except when
 * reading the array data, the information it gives us is only on the
 * low byte, because that's the common denominator between the two
 * modes.  
 */
static
FlashError_t
ReadLowByte( Flash_t const *Flash,
	     unsigned long Location,
	     unsigned char /*out*/ *Byte )
{
  FlashError_t Error;
  unsigned char Element[ChipWidthBytesMax_M];

  Error = Flash->Read( Flash->UserData, Location, Flash->ChipWidthBytes, Element );
  if (Error != FlashErrorOkay_c) return (Error);

  /* The status bits are in the low-order bits of the Element.
   * But which byte is low-order?  If we're dealing with reads on
   * an M68K family processor the low-order byte is the last one.
   * If we're running on an Intel but accessing the flash through
   * BDM, the reads are still being done by a M68K processor, so
   * the low order byte is the last one.  
   */
  if (Flash->BigEndian)
    *Byte = Element[Flash->ChipWidthBytes-1];
  else
    *Byte = Element[0];

  return (FlashErrorOkay_c);
}


/***************************************************************/
/* Bits in the flash Operation Status register */

/* The opposite of the stored data value while the flash is 
 * running its embedded program.
 */
#define FlashStyleAMDStatusNotDataPollingMask_M       (1<<7)

/* Toggles with each read while the flash is 
 * running its embedded program.
 */
#define FlashStyleAMDStatusToggleBitMask_M            (1<<6)

/* 1 if the flash's embedded algorithm has failed. */
#define FlashStyleAMDStatusExceededTimingLimitsMask_M (1<<5)

/* 1 if the flash's sector erase timer has closed. */
#define FlashStyleAMDStatusSectorEraseTimerMask_M     (1<<3)

/* Distinguishes Embedded Erase Algorithm and Erase suspend. */
#define FlashStyleAMDStatusToggleBit2Mask_M           (1<<2)

/* The address we provide addresses a particular byte in flash.  But
 * the address we provide is not necessarily the address that is
 * seen by the flash.  
 *
 * In byte-wide mode, the flash receives all the address bits we
 * provide.
 *
 * In word-wide mode, the lowest bit is stripped off, and the flash
 * only sees remaining the higher bits.
 *
 * Most flash manufacturers label the address bits based on word
 * wide mode.  So flash pin A0 represents 2^1, flash pin A1
 * represents 2^2, etc.
 *
 * In byte-wide mode, a flash pin called A-1 (as in "address bit
 * negative one") represents 2^0.
 *
 * Some commands sent to the flash require a special pattern on the
 * address pins.  That pattern is the same regardless of whether in
 * word-mode or byte-mode.  Address bit 0 (2^0) should be 0 in
 * byte-mode.  In word-mode address bit 0 is irrelevant to the
 * flash.  It still has to be 0 when we do the write, as we don't
 * want to end up physically doing two writes -- one on either side
 * of the word boundary.
 *
 * All of this is a bit confused in the spec sheets because the
 * manufacturers provide the pattern but don't indicate if it's an
 * address from the program's perspective, a pattern on flash bits
 * A0 to A(n), or a pattern on flash bits A-1 to A(n).
 *
 * ST's M29W160BB.pdf indicates an unlock pattern of 555/AA, 2AA/55
 * which is A0 to A(n).  AMD's Am29LV800BB.pdf (also publication
 * #20375, Rev A Amendment/0, November 1995) indicates an unlock
 * pattern of AAAA/AA, 5555/55 for byte-mode (which is A-1 to A(n)),
 * and 5555/AA, 2AAA/55 (which is A0 to A(n)).
 *
 * FIXME: need to mask off high order bits based on size of flash
 * to make sure we're still addressing the flash.
 */
#define FlashStyleAMDCommandAddress1_M (0x5555<<1)
#define FlashStyleAMDCommandAddress2_M (0x2AAA<<1)


/* Issue the sequence to unlock the command mode */
FlashError_t
FlashStyleAMDCommandUnlock( Flash_t const *Flash,
			    unsigned int ParallelChipI )
{
  FlashError_t Error;

  /* offset of parallel chip from first chip */
  unsigned long ChipOffset = ParallelChipI*Flash->ChipWidthBytes;

  Error = WritePattern( Flash,
			FlashStyleAMDCommandAddress1_M*Flash->NumParallelChips+ChipOffset, 
			0xAA );
  if (Error != FlashErrorOkay_c) return (Error);

  Error = WritePattern( Flash,
			FlashStyleAMDCommandAddress2_M*Flash->NumParallelChips+ChipOffset, 
			0x55 );
  if (Error != FlashErrorOkay_c) return (Error);

  return (FlashErrorOkay_c);
}



FlashError_t
FlashStyleAMDCommand( Flash_t const *Flash,
		      unsigned int ParallelChipI,
		      FlashStyleAMDCommand_t Command )
{
  FlashError_t Error;

  /* offset of parallel chip from first chip */
  unsigned long ChipOffset = ParallelChipI*Flash->ChipWidthBytes;

  Error = FlashStyleAMDCommandUnlock( Flash,
				      ParallelChipI );
  if (Error != FlashErrorOkay_c) return (Error);

  Error = WritePattern( Flash,
			FlashStyleAMDCommandAddress1_M*Flash->NumParallelChips+ChipOffset, 
			Command );
  if (Error != FlashErrorOkay_c) return (Error);
    
  return (FlashErrorOkay_c);
}


static
FlashError_t
FlashStyleAMDOperationCompletedWait( Flash_t const *Flash,
				     unsigned long Location, 
				     void const *ExpectedData )
{
  FlashStatus_t StatusPrev;
  FlashStatus_t StatusCurr;
  FlashError_t Error;
  int/*BOOL*/ LastCheck = 0;

  StatusPrev = *(FlashStatus_t *)ExpectedData;

#ifdef DEBUG
  DebugFlashStatusI = 0;
#endif
  for (;;)
    {
      unsigned char Element[ChipWidthBytesMax_M];

      if (Flash->ChipWidthBytes > ChipWidthBytesMax_M)
	return (FlashErrorWidth_c);

      Error = Flash->Read( Flash->UserData, Location, Flash->ChipWidthBytes, Element );

#ifdef DEBUG
      if (DebugFlashStatusI < DebugFlashStyleAMDHistoryMax_M)
	{
	  memcpy( DebugFlashStatusHistory[DebugFlashStatusI], Element, sizeof (Element) );
	  DebugFlashStatusErrorHistory[DebugFlashStatusI] = Error;
	}
      DebugFlashStatusI++;
      if (DebugFlashStatusCheckIterationsMax 
	  && DebugFlashStatusI > DebugFlashStatusCheckIterationsMax)
	DebugBreakPoint();
#endif

      /* The status bits are in the low-order bits of the Element.
       * But which byte is low-order?  If we're dealing with reads on
       * an M68K family processor the low-order byte is the last one.
       * If we're running on an Intel but accessing the flash through
       * BDM, the reads are still being done by a M68K processor, so
       * the low order byte is the last one.  
       */
      if (Flash->BigEndian)
	StatusCurr = Element[Flash->ChipWidthBytes-1];
      else
	StatusCurr = Element[0];

      if (Error != FlashErrorOkay_c) 
	return (Error);

      if (memcmp( ExpectedData, Element, Flash->ChipWidthBytes ) == 0)
	return (FlashErrorOkay_c);

      if (StatusCurr == StatusPrev)
	{
	  /* The toggle bit has stopped so the operation is complete */

	  /* Check one last time to see if the final data has shown
	   * up.
	   *
	   * You may think this step is unnecessary since the DQ6
	   * settles on the final, expected data.  Indeed, testing
	   * with the AMD29LV160D indicates that at least some chips
	   * are implemented that way.
	   *
	   * But the spec states that the flash can give two
	   * unchanging toggle bits followed by the final data.  E.g.
	   * you could have two 1's followed by a final 0.  See
	   * M29W800AB.pdf spec sheet (March 200), page 9, "Toggle Bit
	   * DQ6".  
	   */
	  Error = Flash->Read( Flash->UserData, Location, Flash->ChipWidthBytes, Element );
	  if (Error != FlashErrorOkay_c) return (Error);
	  if (memcmp( ExpectedData, Element, Flash->ChipWidthBytes ) == 0)
	    return (FlashErrorOkay_c);

#ifdef DEBUG
	  /* fill up the buffer to see if would have changed later */
	  for (; DebugFlashStatusI < DebugFlashStatusHistoryMax_M; DebugFlashStatusI++)
	    {
	      Error = Flash->Read( Flash->UserData, Location, Flash->ChipWidthBytes, Element );
	      memcpy( DebugFlashStatusHistory[DebugFlashStatusI], Element, sizeof (Element) );
	      DebugFlashStatusErrorHistory[DebugFlashStatusI] = Error;
	    }
	  DebugBreakPoint();
#endif
	  return (FlashErrorUnexpected_c);
	}

      if (LastCheck)
	{
	  /* It was a Timeout */
	  DebugBreakPoint();
	  return (FlashErrorTimeOut_c);
	}

      if (StatusCurr & FlashStyleAMDStatusExceededTimingLimitsMask_M)
	{
	  /* Possibly time-out, but possibly operation was completing
	   * but bits were not all settling at same time.
	   */
	  LastCheck = 1;
	}

      StatusPrev = StatusCurr;
    }
}



/* store into flash */
FlashError_t
FlashStyleAMDWrite( Flash_t const *Flash,
		    unsigned long Location, 
		    void const *Data,
		    size_t Size /*actual rounded up by ChipWidthBytes*/ )
{
  FlashError_t Error;
  size_t i;
  unsigned int ParallelWidthBytes = Flash->NumParallelChips * Flash->ChipWidthBytes;
    
  /* write block to flash */
  for (i=0; i < Size;) {
    unsigned long ParallelChipI = (Location%ParallelWidthBytes)/Flash->ChipWidthBytes;
    
    /* Enable the chip to write one element. */
    Error = FlashStyleAMDCommand( Flash,
				  ParallelChipI,
				  FlashStyleAMDCommandProgram_c );
    if (Error != FlashErrorOkay_c) return (Error);

    /* Write one element */
    Error = Flash->Write( Flash->UserData, Location, Flash->ChipWidthBytes, Data );
    if (Error != FlashErrorOkay_c)
      return (Error);


#if 0 /* Apparently no longer needed with Word Read of Status */
    /* Add a slight delay before polling.  This is required by the AMD
     * flash chip.  Experimentally determined by JUS.  
     *
     * FIXME: The documented requirements for this delay should be
     * found.  The delay should be created using a function call that
     * guarantees a minimum delay.
     */
    {
      int volatile DelayI; 
      for (DelayI=0; DelayI<2; DelayI++)
	;
    } 
#endif

#ifdef DEBUG
    DebugFlashStatusCheckIterationsMax = DebugFlashStatusHistoryMax_M;
#endif


    /* Wait for the write to complete */
    Error = FlashStyleAMDOperationCompletedWait( Flash, Location, Data );
    if (Error != FlashErrorOkay_c)
      return (Error);
    
    /* Next element */
    Location += Flash->ChipWidthBytes;
    Data += Flash->ChipWidthBytes;
    i += Flash->ChipWidthBytes;
  }
  return (FlashErrorOkay_c);
}


FlashError_t
FlashStyleAMDEraseSector( Flash_t const *Flash,
			  unsigned long OffsetInFlash /* sector identified by offset */ )
{
  unsigned long ParallelChipI;
  FlashError_t Error;

  /* erase sector in each parallel flash */
  for (ParallelChipI=0; ParallelChipI < Flash->NumParallelChips; ParallelChipI++) 
    {
      Error = FlashStyleAMDCommand( Flash,
				    ParallelChipI,
				    FlashStyleAMDCommandSetUpErase_c );
      if (Error != FlashErrorOkay_c) return (Error);

      Error = FlashStyleAMDCommandUnlock( Flash,
					  ParallelChipI );
      if (Error != FlashErrorOkay_c) return (Error);

      Error = WritePattern( Flash,
			    OffsetInFlash+ParallelChipI*Flash->NumParallelChips,
			    FlashStyleAMDCommandBlockEraseResumeConfirm_c );
      if (Error != FlashErrorOkay_c) return (Error);
    }

  /* wait for each parallel flash to complete */
  for (ParallelChipI=0; ParallelChipI < Flash->NumParallelChips; ParallelChipI++) 
    {
#ifdef DEBUG
      DebugFlashStatusCheckIterationsMax = 0;
#endif

      Error = FlashStyleAMDOperationCompletedWait( Flash,
						   OffsetInFlash+Flash->ChipWidthBytes*ParallelChipI,
						   /* Expect all bits to be erased */
						   &ULongFFFFFFFF );
      if (Error != FlashErrorOkay_c)
	return (Error);
    }
  return (FlashErrorOkay_c);
}


/* erase the whole flash */
FlashError_t
FlashStyleAMDErase( Flash_t const *Flash )
{
  unsigned long ParallelChipI;
  FlashError_t Error;

  /* erase each parallel flash */
  for (ParallelChipI=0; ParallelChipI < Flash->NumParallelChips; ParallelChipI++) 
    {
      Error = FlashStyleAMDCommand( Flash,
				    ParallelChipI,
				    FlashStyleAMDCommandSetUpErase_c );
      if (Error != FlashErrorOkay_c) return (Error);

      Error = FlashStyleAMDCommand( Flash,
				    ParallelChipI,
				    FlashStyleAMDCommandChipEraseConfirm_c );
      if (Error != FlashErrorOkay_c) return (Error);
    }

  /* wait for each parallel flash to complete */
  for (ParallelChipI=0; ParallelChipI < Flash->NumParallelChips; ParallelChipI++) 
    {
#ifdef DEBUG
      DebugFlashStatusCheckIterationsMax = 0;
#endif
      Error = FlashStyleAMDOperationCompletedWait( Flash,
						   Flash->ChipWidthBytes*ParallelChipI,
						   /* Expect bits to be all erased */
						   &ULongFFFFFFFF );
      if (Flash != FlashErrorOkay_c)
	return (Error);
    }
  return (FlashErrorOkay_c);
}


/* Check whether chip will respond to Micron-Style manipulation.
 * If found, leave chip in Array Read Mode.
 */
FlashError_t
FlashStyleAMDProbe( Flash_t const *Flash,
		    int/*BOOL out*/ *Found )
{
  unsigned int ParallelWidthBytes = Flash->NumParallelChips * Flash->ChipWidthBytes;
  FlashError_t Error;
  unsigned char Store[ChipWidthBytesMax_M];
  unsigned char Compare[ChipWidthBytesMax_M];
  unsigned char Manufacturer[ChipWidthBytesMax_M];
  unsigned char Device[ChipWidthBytesMax_M];
  unsigned char BlockStatus[ChipWidthBytesMax_M];
  unsigned char Command[ChipWidthBytesMax_M];

  /* Number of times the values read match the command that was written */
  unsigned int CommandMatches = 0;

  /* (supposedly) put chip into signature/block status read mode */
  Error = FlashStyleAMDCommand( Flash,
				0,
				FlashStyleAMDCommandReadSignatureBlockStatus_c );
  if (Error != FlashErrorOkay_c) return (Error);

  
  /* validate that we're in signature/block read mode.
   * Data should be same regardless of A2
   */
  Error = Flash->Read( Flash->UserData, 0, Flash->ChipWidthBytes, Manufacturer );
  if (Error != FlashErrorOkay_c) return (Error);

  ElementFillPattern( Command, Flash->ChipWidthBytes, FlashStyleAMDCommandReadSignatureBlockStatus_c );
  if (memcmp( Manufacturer, Command, Flash->ChipWidthBytes ) == 0)
    CommandMatches++;

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*1, Flash->ChipWidthBytes, Device );
  if (Error != FlashErrorOkay_c) return (Error);

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*2, Flash->ChipWidthBytes, BlockStatus );
  if (Error != FlashErrorOkay_c) return (Error);

#if 0 /*ATMEL chips don't provide the Manufacturer, etc. at multiple addresses*/
  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*4, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);

  if (memcmp( Manufacturer, Compare, Flash->ChipWidthBytes ) != 0)
    {
      /* Manufacturer changed value.
       * We're not in identification signature/block read mode.
       * This isn't an AMD-style chip.
       */
      *Found = 0/*FALSE*/;
      return (FlashErrorOkay_c);
    }

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*5, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);

  if (memcmp( Device, Compare, Flash->ChipWidthBytes ) != 0)
    {
      /* Device changed value.
       * We're not in identification signature/block read mode.
       * This isn't an AMD-style chip.
       */
      *Found = 0/*FALSE*/;
      return (FlashErrorOkay_c);
    }

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*6, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);

  if (memcmp( BlockStatus, Compare, Flash->ChipWidthBytes ) != 0)
    {
      /* Block Status changed value.
       * We're not in identification signature/block read mode.
       * This isn't an AMD-style chip.
       */
      *Found = 0/*FALSE*/;
      return (FlashErrorOkay_c);
    }
#endif

  /* (supposedly) put chip into array read mode */
  Error = FlashStyleAMDCommand( Flash,
				0,
				FlashStyleAMDCommandReadArrayReset_c );
  if (Error != FlashErrorOkay_c) return (Error);

  /* validate that we're in read mode -- data shouldn't change */
  Error = Flash->Read( Flash->UserData, 0, Flash->ChipWidthBytes, Store );
  if (Error != FlashErrorOkay_c) return (Error);

  ElementFillPattern( Command, Flash->ChipWidthBytes, FlashStyleAMDCommandReadArrayReset_c );
  if (memcmp( Store, Command, Flash->ChipWidthBytes ) == 0)
    CommandMatches++;

  if (CommandMatches == 2)
    {
      /* Data tracks the values written.
       * Probably no chip at all, or RAM instead.
       */
      *Found = 0/*FALSE*/;
      return (FlashErrorOkay_c);
    }

  Error = Flash->Read( Flash->UserData, 0, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);

  if (memcmp( Store, Compare, Flash->ChipWidthBytes ) != 0)
    {
      /* Element changed value.
       * We're not in array read mode.
       * This isn't an AMD-style chip.
       */
      *Found = 0/*FALSE*/;
      return (FlashErrorOkay_c);
    }

  /* If data is different from identification signature/block read
   * mode, accept as proof that were changing modes and that this is a
   * AMD-style chip.  
   */

  if (memcmp( Manufacturer, Compare, Flash->ChipWidthBytes ) != 0) goto Valid;

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*1, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);
  if (memcmp( Device, Compare, Flash->ChipWidthBytes ) != 0) goto Valid;

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*2, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);
  if (memcmp( BlockStatus, Compare, Flash->ChipWidthBytes ) != 0) goto Valid;

#if 0
  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*4, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);
  if (memcmp( Manufacturer, Compare, Flash->ChipWidthBytes ) != 0) goto Valid;

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*5, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);
  if (memcmp( Device, Compare, Flash->ChipWidthBytes ) != 0) goto Valid;

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*6, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);
  if (memcmp( BlockStatus, Compare, Flash->ChipWidthBytes ) != 0) goto Valid;
#endif

  /* If we get here either this isn't an AMD-style flash or the first
   * portion of the array is identical to the Manufacturer ID, Device
   * ID, and Block Status.  (unlikely).  
   */

  *Found = 0/*FALSE*/;
  return (FlashErrorOkay_c);

 Valid:
  /* This is indeed an AMD-style flash */
  *Found = 1/*TRUE*/;
  if (Error != FlashErrorOkay_c) return (Error);
  return (FlashErrorOkay_c);
}


FlashError_t
FlashStyleAMDIDRead( Flash_t const *Flash,
		     FlashID_t /*out*/ *ID )
{
  /* Read the first parallel flash chip.  Assume that each of them are
   * identical. 
   */
  unsigned int ParallelWidthBytes = Flash->NumParallelChips * Flash->ChipWidthBytes;
  FlashError_t Error;
  unsigned char Manufacturer;
  unsigned char Device;
  
  Error = FlashStyleAMDCommand( Flash,
				0,
				FlashStyleAMDCommandReadSignatureBlockStatus_c );
  if (Error != FlashErrorOkay_c) return (Error);

  Error = ReadLowByte( Flash,
		       ParallelWidthBytes*0, 
		       &Manufacturer );
  if (Error != FlashErrorOkay_c) return (Error);

  Error = ReadLowByte( Flash,
		       ParallelWidthBytes*1,
		       &Device );
  if (Error != FlashErrorOkay_c) return (Error);

  /* Issue the read/reset command sequence: */
  Error = FlashStyleAMDCommand( Flash,
				0,
				FlashStyleAMDCommandReadArrayReset_c );
  if (Error != FlashErrorOkay_c) return (Error);

  *ID = FlashIDMake_M( Manufacturer, Device );

  return (FlashErrorOkay_c);
}


/*******************************************************************************************
 * Micron-style specific code
 */

typedef enum FlashStyleMicronCommand_en {
  FlashStyleMicronCommandReserved_c = 0x00,
  FlashStyleMicronCommandReadArray_c = 0xFF,
  FlashStyleMicronCommandIdentifyDevice_c = 0x90,
  FlashStyleMicronCommandReadStatusRegister_c = 0x70,
  FlashStyleMicronCommandClearStatusRegister_c = 0x50,
  FlashStyleMicronCommandEraseSetup_c = 0x20,
  FlashStyleMicronCommandEraseConfirmResume_c = 0xD0,
  FlashStyleMicronCommandWriteSetup_c = 0x40,
  FlashStyleMicronCommandAlternateWriteSetup_c = 0x10,
  FlashStyleMicronCommandEraseSuspend_c = 0xB0,
} FlashStyleMicronCommand_t;


#define FlashStyleMicronISMStatusMask_M          (1<<7)
#define FlashStyleMicronEraseSuspendStatusMask_M (1<<6)
#define FlashStyleMicronEraseStatusMask_M        (1<<5)
#define FlashStyleMicronWriteStatusMask_M        (1<<4)
#define FlashStyleMicronVPPStatusMask_M          (1<<3)


/* give command to *ONE* of the parallel chips */
FlashError_t
FlashStyleMicronCommand( Flash_t const *Flash,
			 unsigned long OffsetInFlash,
			 unsigned char Command )
{
  FlashError_t Error;

  Error = WritePattern( Flash,
			OffsetInFlash,
			Command );
  return (Error);
}


/* give command to *EACH* parallel chip */
FlashError_t
FlashStyleMicronParallelCommand( Flash_t const *Flash,
				 unsigned long OffsetInFlash,
				 unsigned char Command )
{
  FlashError_t Error;
  unsigned long ChipOffset;  /* offset of parallel chip from first chip */
  unsigned int ParallelWidthBytes = Flash->NumParallelChips * Flash->ChipWidthBytes;

  for (ChipOffset=0; ChipOffset < ParallelWidthBytes; ChipOffset += Flash->ChipWidthBytes) 
    {
      Error = FlashStyleMicronCommand( Flash,
				       OffsetInFlash+ChipOffset,
				       Command );
      if (Error != FlashErrorOkay_c) return (Error);
    }
  return (FlashErrorOkay_c);
}



/* Wait for an operation to complete on a Micron-style flash.
 * Assumes flash is in Status Read mode.
 * Reports and clears any error.
 * Leaves flash in Array Read mode.
 */
static
FlashError_t
FlashStyleMicronOperationCompletedWait( Flash_t const *Flash,
					unsigned long Location, 
					void const *ExpectedData )
{
  FlashError_t Error;
  FlashError_t StatusError;
  unsigned char Status;
  unsigned char Element[ChipWidthBytesMax_M];
  

  if (Flash->ChipWidthBytes > ChipWidthBytesMax_M)
    return (FlashErrorWidth_c);

  /* Wait for the flash to indicate it's ready */
  for (;;)
    {
      Error = ReadLowByte( Flash, Location, &Status );
      if (Error != FlashErrorOkay_c) 
	return (Error);
      
      if (Status & FlashStyleMicronISMStatusMask_M)
	break;
    }

  /* decode the status error flags */
  StatusError = FlashErrorOkay_c;

  if (Status & FlashStyleMicronEraseStatusMask_M)
    StatusError = FlashErrorEraseFail_c;

  if (Status & FlashStyleMicronWriteStatusMask_M)
    StatusError = FlashErrorWriteFail_c;

  if (Status & FlashStyleMicronVPPStatusMask_M)
    StatusError = FlashErrorProtected_c;

  /* Clear any error */
  Error = FlashStyleMicronCommand( Flash,
				   Location,
				   FlashStyleMicronCommandClearStatusRegister_c );
  if (Error != FlashErrorOkay_c) return (Error);

  /* Leave in Array Read mode */
  Error = FlashStyleMicronCommand( Flash,
				   Location,
				   FlashStyleMicronCommandReadArray_c );
  if (Error != FlashErrorOkay_c) return (Error);

  if (StatusError != FlashErrorOkay_c) return (StatusError);

  /* verify that element now has the value we expected */
  Error = Flash->Read( Flash->UserData, Location, Flash->ChipWidthBytes, Element );
  if (Error != FlashErrorOkay_c) return (Error);
  if (memcmp( ExpectedData, Element, Flash->ChipWidthBytes ) != 0)
    return (FlashErrorUnexpected_c);

  return (FlashErrorOkay_c);
}



/* store into flash */
FlashError_t
FlashStyleMicronWrite( Flash_t const *Flash,
		       unsigned long Location, 
		       void const *Data,
		       size_t Size /*actual rounded up by ChipWidthBytes*/ )
{
  FlashError_t Error;
  size_t i;
  unsigned int ParallelWidthBytes = Flash->NumParallelChips * Flash->ChipWidthBytes;
    
  /* write block to flash */
  for (i=0; i < Size;) 
    {
    unsigned long ParallelChipI = (Location%ParallelWidthBytes)/Flash->ChipWidthBytes;
    
    /* Enable the chip to write one element. */
    Error = FlashStyleMicronCommand( Flash,
				     ParallelChipI*Flash->ChipWidthBytes,
				     FlashStyleMicronCommandWriteSetup_c );
    if (Error != FlashErrorOkay_c) return (Error);

    /* Write one element */
    Error = Flash->Write( Flash->UserData, Location, Flash->ChipWidthBytes, Data );
    if (Error != FlashErrorOkay_c) return (Error);


#ifdef DEBUG
    DebugFlashStatusCheckIterationsMax = DebugFlashStatusHistoryMax_M;
#endif


    /* Wait for the write to complete */
    Error = FlashStyleMicronOperationCompletedWait( Flash, Location, Data );
    if (Error != FlashErrorOkay_c)
      return (Error);
    
    /* Next element */
    Location += Flash->ChipWidthBytes;
    Data += Flash->ChipWidthBytes;
    i += Flash->ChipWidthBytes;
  }
  return (FlashErrorOkay_c);
}


FlashError_t
FlashStyleMicronEraseSector( Flash_t const *Flash,
			     unsigned long OffsetInFlash /* sector identified by offset */ )
{
  unsigned long ParallelChipI;
  FlashError_t Error;

  /* erase sector in each parallel flash */
  for (ParallelChipI=0; ParallelChipI < Flash->NumParallelChips; ParallelChipI++) 
    {
      Error = FlashStyleMicronCommand( Flash,
				       OffsetInFlash+ParallelChipI*Flash->ChipWidthBytes,
				       FlashStyleMicronCommandEraseSetup_c );
      if (Error != FlashErrorOkay_c) return (Error);

      Error = FlashStyleMicronCommand( Flash,
				       OffsetInFlash+ParallelChipI*Flash->ChipWidthBytes,
				       FlashStyleMicronCommandEraseConfirmResume_c );
      if (Error != FlashErrorOkay_c) return (Error);
    }

  /* wait for each parallel flash to complete */
  for (ParallelChipI=0; ParallelChipI < Flash->NumParallelChips; ParallelChipI++) 
    {
#ifdef DEBUG
      DebugFlashStatusCheckIterationsMax = 0;
#endif

      Error = FlashStyleMicronOperationCompletedWait( Flash,
						      OffsetInFlash+ParallelChipI*Flash->ChipWidthBytes,
						      /* Expect all bits to be erased */
						      &ULongFFFFFFFF );
      if (Error != FlashErrorOkay_c) return (Error);
    }
  return (FlashErrorOkay_c);
}


/* erase the whole flash */
FlashError_t
FlashStyleMicronErase( Flash_t const *Flash )
{
  FlashInfo_t const *Info;
  FlashError_t Error;
  unsigned long Size;
  unsigned long Location;
  unsigned char Element[ChipWidthBytesMax_M];

  /* Micron-style only supports erasing one sector at a time.
   * Rather than knowing where each sector lies, scan the entire flash
   * and erase any sector that isn't already erased.
   */

  /* Find out how big the flash is */
  Error = FlashDetect( Flash, &Info );
  if (Error != FlashErrorOkay_c) return (Error);
  Size = Info->Size * Flash->NumParallelChips;

  /* scan for unerased data */
  for (Location = 0; Location < Size; Location += Flash->ChipWidthBytes)
    {
      Error = Flash->Read( Flash->UserData, 
			   Location, 
			   Flash->ChipWidthBytes, 
			   Element );
      if (Error != FlashErrorOkay_c) return (Error);

      if (memcmp( Element, &ULongFFFFFFFF, Flash->ChipWidthBytes ) != 0)
	{
	  Error = FlashStyleMicronEraseSector( Flash, Location );
	  if (Error != FlashErrorOkay_c) return (Error);
	}
    }

  return (FlashErrorOkay_c);
}


/* Check whether chip will respond to Micron-Style manipulation.
 * If found, leave chip in Array Read Mode.
 */
FlashError_t
FlashStyleMicronProbe( Flash_t const *Flash,
		       int/*BOOL out*/ *Found )
{
  unsigned int ParallelWidthBytes = Flash->NumParallelChips * Flash->ChipWidthBytes;
  FlashError_t Error;
  unsigned char Store[ChipWidthBytesMax_M];
  unsigned char Compare[ChipWidthBytesMax_M];
  unsigned char Manufacturer[ChipWidthBytesMax_M];
  unsigned char Device[ChipWidthBytesMax_M];
  unsigned char Command[ChipWidthBytesMax_M];

  /* Number of times the values read match the command that was written */
  unsigned int CommandMatches = 0;

  /* (supposedly) put chip into identification read mode */
  Error = FlashStyleMicronCommand( Flash,
				   0,
				   FlashStyleMicronCommandIdentifyDevice_c );
  if (Error != FlashErrorOkay_c) return (Error);

  
  /* validate that we're in identifacion mode.
   * Data should be same regardless of A1
   */
  Error = Flash->Read( Flash->UserData, 0, Flash->ChipWidthBytes, Manufacturer );
  if (Error != FlashErrorOkay_c) return (Error);

  ElementFillPattern( Command, Flash->ChipWidthBytes, FlashStyleMicronCommandIdentifyDevice_c );
  if (memcmp( Manufacturer, Command, Flash->ChipWidthBytes ) == 0)
    CommandMatches++;

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*1, Flash->ChipWidthBytes, Device );
  if (Error != FlashErrorOkay_c) return (Error);

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*2, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);

  if (memcmp( Manufacturer, Compare, Flash->ChipWidthBytes ) != 0)
    {
      /* Manufacturer changed value.
       * We're not in identification read mode.
       * This isn't a Micron-Style chip.
       */
      *Found = 0/*FALSE*/;
      return (FlashErrorOkay_c);
    }

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*3, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);

  if (memcmp( Device, Compare, Flash->ChipWidthBytes ) != 0)
    {
      /* Device changed value.
       * We're not in identification read mode.
       * This isn't a Micron-Style chip.
       */
      *Found = 0/*FALSE*/;
      return (FlashErrorOkay_c);
    }

  /* (supposedly) put chip into array read mode */
  Error = FlashStyleMicronCommand( Flash,
				   0,
				   FlashStyleMicronCommandReadArray_c );
  if (Error != FlashErrorOkay_c) return (Error);

  /* validate that we're in read mode -- data shouldn't change */
  Error = Flash->Read( Flash->UserData, 0, Flash->ChipWidthBytes, Store );
  if (Error != FlashErrorOkay_c) return (Error);

  ElementFillPattern( Command, Flash->ChipWidthBytes, FlashStyleMicronCommandReadArray_c );
  if (memcmp( Store, Command, Flash->ChipWidthBytes ) == 0)
    CommandMatches++;
  if (CommandMatches == 2)
    {
      /* Data tracks the values written.
       * Probably no chip at all, or RAM instead.
       */
      *Found = 0/*FALSE*/;
      return (FlashErrorOkay_c);
    }

  Error = Flash->Read( Flash->UserData, 0, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);

  if (memcmp( Store, Compare, Flash->ChipWidthBytes ) != 0)
    {
      /* Element changed value.
       * We're not in array read mode.
       * This isn't a Micron-Style chip.
       */
      *Found = 0/*FALSE*/;
      return (FlashErrorOkay_c);
    }

  /* If data is different from identication mode, accept as proof that
   * were changing modes and that this is a Micron-Style chip.  
   */

  if (memcmp( Manufacturer, Compare, Flash->ChipWidthBytes ) != 0) goto Valid;

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*1, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);
  if (memcmp( Device, Compare, Flash->ChipWidthBytes ) != 0) goto Valid;

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*2, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);
  if (memcmp( Manufacturer, Compare, Flash->ChipWidthBytes ) != 0) goto Valid;

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*3, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);
  if (memcmp( Device, Compare, Flash->ChipWidthBytes ) != 0) goto Valid;

  /* (supposedly) put chip into status read mode */
  Error = FlashStyleMicronCommand( Flash,
				   0,
				   FlashStyleMicronCommandReadStatusRegister_c );
  if (Error != FlashErrorOkay_c) return (Error);

  
  /* Check for different value than in Identifaction Read mode */
  Error = Flash->Read( Flash->UserData, 0, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);
  if (memcmp( Manufacturer, Compare, Flash->ChipWidthBytes ) != 0) goto Valid;

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*1, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);
  if (memcmp( Device, Compare, Flash->ChipWidthBytes ) != 0) goto Valid;

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*2, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);
  if (memcmp( Manufacturer, Compare, Flash->ChipWidthBytes ) != 0) goto Valid;

  Error = Flash->Read( Flash->UserData, ParallelWidthBytes*3, Flash->ChipWidthBytes, Compare );
  if (Error != FlashErrorOkay_c) return (Error);
  if (memcmp( Device, Compare, Flash->ChipWidthBytes ) != 0) goto Valid;

  /* If we get here either this isn't a Micron-Style flash or
   * the Manufacturer ID, Device ID, Status, and the first portion
   * of the array are all identical (very unlikely).
   */

  *Found = 0/*FALSE*/;
  return (FlashErrorOkay_c);

 Valid:
  /* This is indeed a Micron-Style flash */
  *Found = 1/*TRUE*/;
  
  /* place all the parallel chips in Array Read mode */
  Error = FlashStyleMicronParallelCommand( Flash,
					   0,
					   FlashStyleMicronCommandReadArray_c );
  if (Error != FlashErrorOkay_c) return (Error);
  return (FlashErrorOkay_c);
}

FlashError_t
FlashStyleMicronIDRead( Flash_t const *Flash,
			FlashID_t /*out*/ *ID )
{
  /* Read the first parallel flash chip.  Assume that each of them are
   * identical. 
   */
  unsigned int ParallelWidthBytes = Flash->NumParallelChips * Flash->ChipWidthBytes;
  FlashError_t Error;
  unsigned char Manufacturer;
  unsigned char Device;
  
  Error = FlashStyleMicronCommand( Flash,
				   0,
				   FlashStyleMicronCommandIdentifyDevice_c );
  if (Error != FlashErrorOkay_c) return (Error);

  Error = ReadLowByte( Flash,
		       ParallelWidthBytes*0, 
		       &Manufacturer );
  if (Error != FlashErrorOkay_c) return (Error);

  Error = ReadLowByte( Flash,
		       ParallelWidthBytes*1,
		       &Device );
  if (Error != FlashErrorOkay_c) return (Error);

  /* put each chip into array read mode */
  Error = FlashStyleMicronParallelCommand( Flash,
					   0,
					   FlashStyleMicronCommandReadArray_c );
  if (Error != FlashErrorOkay_c) return (Error);

  *ID = FlashIDMake_M( Manufacturer, Device );

  return (FlashErrorOkay_c);
}


/*******************************************************************************************
 * generic code
 */





/* store into flash */
FlashError_t
FlashWrite( Flash_t const *Flash,
	    unsigned long Location, 
	    void const *Data,
	    size_t Size /*actual rounded up by ChipWidthBytes*/ )
{
  switch (Flash->Style)
    {
    default:
    case FlashStyleUndefined_c:
      return (FlashErrorStyle_c);
      break;
    case FlashStyleAMD_c:
      return (FlashStyleAMDWrite( Flash,
				  Location,
				  Data,
				  Size ));
      break;
    case FlashStyleMicron_c:
      return (FlashStyleMicronWrite( Flash,
				     Location,
				     Data,
				     Size ));
      break;
    }
}


/* erase one sector of the flash */
FlashError_t
FlashEraseSector( Flash_t const *Flash,
		  unsigned long OffsetInFlash /* sector identified by offset */ )
{
  switch (Flash->Style)
    {
    default:
    case FlashStyleUndefined_c:
      return (FlashErrorStyle_c);
      break;
    case FlashStyleAMD_c:
      return (FlashStyleAMDEraseSector( Flash,
					OffsetInFlash ));
      break;
    case FlashStyleMicron_c:
      return (FlashStyleMicronEraseSector( Flash,
					   OffsetInFlash ));
      break;
    }
}




/* erase the whole flash */
FlashError_t
FlashErase( Flash_t const *Flash )
{
  switch (Flash->Style)
    {
    default:
    case FlashStyleUndefined_c:
      return (FlashErrorStyle_c);
      break;
    case FlashStyleAMD_c:
      return (FlashStyleAMDErase( Flash ));
      break;
    case FlashStyleMicron_c:
      return (FlashStyleMicronErase( Flash ));
      break;
    }
}


char const *FlashStyleName[FlashStyleCount_c] = {
  "Undefined",
  "AMD",
  "Micron",
};

/*sets Parameters->Style */
FlashError_t
FlashProbe( Flash_t *Flash )
{
  FlashError_t Error;
  int/*BOOL*/ Found;

  /* Test first for Micron-style.
   * It's more sensitive to abuse because it doesn't have a command unlock
   * process.  Also the test is more rigourous than for AMD-style.
   */
  Error = FlashStyleMicronProbe( Flash, &Found );
  if (Error != FlashErrorOkay_c) return (Error);
  if (Found)
    {
      Flash->Style = FlashStyleMicron_c;
      return (FlashErrorOkay_c);
    }

  Error = FlashStyleAMDProbe( Flash, &Found );
  if (Error != FlashErrorOkay_c) return (Error);
  if (Found)
    {
      Flash->Style = FlashStyleAMD_c;
      return (FlashErrorOkay_c);
    }

  Flash->Style = FlashStyleUndefined_c;
  return (FlashErrorOkay_c);
}

/* leaves flash in Array Read mode */
FlashError_t
FlashIDRead( Flash_t const *Flash,
	     FlashID_t /*out*/ *ID )
{
  switch (Flash->Style)
    {
    default:
    case FlashStyleUndefined_c:
      return (FlashErrorStyle_c);
      break;
    case FlashStyleAMD_c:
      return (FlashStyleAMDIDRead( Flash,
				   ID ));
      break;
    case FlashStyleMicron_c:
      return (FlashStyleMicronIDRead( Flash,
				      ID ));
      break;
    }
}


static FlashInfo_t const flash_table[]= {
  /*              Manufacturer                         Device  Size      Low      High    SectorSize Model*/
  {FlashIDMake_M( FlashManufacturerAMD_c,              0x20 ),  0x40000,  0x8000,  0x40000,  0x8000, "29F010"},
  {FlashIDMake_M( FlashManufacturerAMD_c,              0xA4 ), 0x100000, 0x20000, 0x100000, 0x20000, "29F040"},
  {FlashIDMake_M( FlashManufacturerAMD_c,              0xD5 ), 0x200000, 0x40000, 0x200000, 0x40000, "29F080"},
  {FlashIDMake_M( FlashManufacturerAMD_c,              0xBA ),  0x80000, 0x10000,  0x80000, 0x10000, "Am29LV400BB"},
  {FlashIDMake_M( FlashManufacturerAMD_c,              0xB9 ),  0x80000, 0x00000,  0x70000, 0x10000, "Am29LV400BT"},
  {FlashIDMake_M( FlashManufacturerAMD_c,              0x5B ), 0x100000, 0x10000, 0x100000, 0x10000, "Am29LV800BB"},
  {FlashIDMake_M( FlashManufacturerAMD_c,              0xDA ), 0x100000, 0x00000, 0x0F0000, 0x10000, "Am29LV800BT"},
  {FlashIDMake_M( FlashManufacturerAMD_c,              0x49 ), 0x200000, 0x10000, 0x200000, 0x10000, "Am29LV160DB"},
  {FlashIDMake_M( FlashManufacturerAMD_c,              0xC4 ), 0x200000, 0x00000, 0x1F0000, 0x10000, "Am29LV160DT"},
  {FlashIDMake_M( FlashManufacturerAtmel_c,            0xD5 ),  0x40000,  0x8000,  0x40000,  0x8000, "29C010"},
  {FlashIDMake_M( FlashManufacturerAtmel_c,            0xDA ),  0x80000, 0x10000,  0x80000, 0x10000, "29C020"}, 
  {FlashIDMake_M( FlashManufacturerAtmel_c,            0x5B ), 0x100000, 0x20000, 0x100000, 0x20000, "29C040"},
  {FlashIDMake_M( FlashManufacturerAtmel_c,            0xC0 ), 0x200000, 0x00000, 0x1E0000, 0x10000, "AT49BV16x4"},
  {FlashIDMake_M( FlashManufacturerAtmel_c,            0xC2 ), 0x200000, 0x20000, 0x200000, 0x10000, "AT49BV16x4T"},

  {FlashIDMake_M( FlashManufacturerFujitsu_c,          0xA4 ), 0x100000, 0x20000, 0x100000, 0x20000, "29F040"},
  {FlashIDMake_M( FlashManufacturerFujitsu_c,          0xD5 ), 0x200000, 0x40000, 0x200000, 0x40000, "29F080"},
  {FlashIDMake_M( FlashManufacturerSST_c,              0x07 ),  0x40000,  0x8000,  0x40000,  0x8000, "29EE010"},
  {FlashIDMake_M( FlashManufacturerSGSThomson_c,       0xE2 ), 0x100000, 0x20000, 0x100000, 0x20000, "29F040"},
  {FlashIDMake_M( FlashManufacturerSGSThomson_c,       0xEE ),  0x80000, 0x00000,  0x70000, 0x10000, "M29W400AT"},
  {FlashIDMake_M( FlashManufacturerSGSThomson_c,       0xEF ),  0x80000, 0x10000,  0x80000, 0x10000, "M29W400AB"},
  {FlashIDMake_M( FlashManufacturerSGSThomson_c,       0xD7 ), 0x100000, 0x00000, 0x0F0000, 0x10000, "M29W800AT"},
  {FlashIDMake_M( FlashManufacturerSGSThomson_c,       0x5B ), 0x100000, 0x10000, 0x100000, 0x10000, "M29W800AB"},
  {FlashIDMake_M( FlashManufacturerSGSThomson_c,       0xC4 ), 0x200000, 0x00000, 0x1F0000, 0x10000, "M29W160AT"},
  {FlashIDMake_M( FlashManufacturerSGSThomson_c,       0x49 ), 0x200000, 0x10000, 0x200000, 0x10000, "M29W160AB"},
  {FlashIDMake_M( FlashManufacturerHyundai_c,          0x40 ), 0x100000, 0x20000, 0x100000, 0x20000, "HY29F040"},
  {FlashIDMake_M( FlashManufacturerTexasInstruments_c, 0x94 ), 0x100000, 0x20000, 0x100000, 0x20000, "TMS29F040"},
  {FlashIDMake_M( FlashManufacturerMicron_c,           0x70 ),  0x80000, 0x00000,  0x60000, 0x20000, "MTF004B3/400B3xx-xxT"},
  {FlashIDMake_M( FlashManufacturerMicron_c,           0x71 ),  0x80000, 0x20000,  0x80000, 0x20000, "MTF004B3/400B3xx-xxB"},
  {FlashIDMake_M( FlashManufacturerMicron_c,           0x9C ), 0x100000, 0x00000,  0xE0000, 0x20000, "MTF008B3/800B3xx-xxT"},
  {FlashIDMake_M( FlashManufacturerMicron_c,           0x9D ), 0x100000, 0x20000, 0x100000, 0x20000, "MTF008B3/800B3xx-xxB"},
  
  {0,0,0,0}
};

FlashError_t
FlashDetect( Flash_t const *Flash,
	     FlashInfo_t const * /*out*/ *FlashInfo /*NULL if not known*/ )
{
  FlashID_t ID;
  FlashError_t Error;
  int i;

  Error = FlashIDRead( Flash, &ID );
  if (Error != FlashErrorOkay_c) return (Error);

  *FlashInfo = NULL;  
  for (i=0; flash_table[i].ID != 0x0000; i++) 
    {
    if (flash_table[i].ID == ID) 
      {
	*FlashInfo = &flash_table[i];
      }
    }    
  return (FlashErrorOkay_c);
}


/* Initialiaze the Flash structure */
FlashError_t
FlashInit( Flash_t /*out*/ *Flash,
	   void *UserData,
	   FlashElementReadFunc_t Read,
	   FlashElementWriteFunc_t Write,
	   unsigned int ChipWidthBytes,
	   int/*Bool*/ BigEndian,
	   unsigned int NumParallelChips )
{
  FlashError_t Error;

  Flash->UserData = UserData;
  Flash->Read = Read;
  Flash->Write = Write;
  Flash->ChipWidthBytes = ChipWidthBytes;
  Flash->BigEndian = BigEndian;
  Flash->NumParallelChips = NumParallelChips;

  Error = FlashProbe( Flash ); /* set Style */
  return (Error);
}



/*EOF*/
