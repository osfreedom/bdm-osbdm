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

/* File:	BDMFlash.c
 * Purpose:	Flash Driver through BDM
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
 * $Log: BDMFlash.c,v $
 * Revision 1.2  2003/07/04 22:33:01  codewiz
 * Applied SST block-erase patch.
 *
 * Revision 1.1  2003/06/03 15:42:04  codewiz
 * Import userland tools from bdm-fiedler
 *
 * Revision 1.5  2000/08/03 06:29:18  wuth
 * MultiProject Sync; Support Micron-style flash; Report flash model
 *
 * Revision 1.4  2000/07/25 13:51:09  wuth
 * Working sector erase.  Better error reports.
 *
 * Revision 1.3  2000/07/14 18:38:55  wuth
 * Flash error status; Fix sector erase support; Command line sector erase
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

#include <BDMFlash.h>

#include <assert.h>
#include <BDMTargetAddress.h> 
#include <Debug.h>
#include <Flash.h>
#include <netinet/in.h> /* ntohl, ntohl */
#include <stdio.h>
#include <stdlib.h> /* size_t */

typedef struct BDMFlash_s
{
  int Device; /* file descriptor of BDM device */
  unsigned long Base; /* start of flash in BDM's address space */
} BDMFlash_t;


static
FlashError_t
BDMFlashByteRead( void *UserData, unsigned long Location, unsigned char /*out*/ *Byte )
{
  BDMFlash_t *BDMFlash = (BDMFlash_t *)UserData;
  int Val;

  if ((Val = BDMTargetByteRead( BDMFlash->Device, BDMFlash->Base + Location )) < 0)
      return (FlashErrorReadAccess_c);

  *Byte = (unsigned char) Val;
  return (FlashErrorOkay_c);
}


static
FlashError_t
BDMFlashWordRead( void *UserData, unsigned long Location, unsigned short /*out*/ *Word )
{
  BDMFlash_t *BDMFlash = (BDMFlash_t *)UserData;
  int Val;

  if ((Val = BDMTargetWordRead( BDMFlash->Device, BDMFlash->Base + Location )) < 0)
      return (FlashErrorReadAccess_c);

  *Word = (unsigned short) Val;
  return (FlashErrorOkay_c);
}


#if 0 /*BDMTargetLongRead not yet implemented*/
static
FlashError_t
BDMFlashLongRead( void *UserData, unsigned long Location, unsigned long /*out*/ *Long )
{
  BDMFlash_t *BDMFlash = (BDMFlash_t *)UserData;
  long Val;

  if ((Val = BDMTargetLongRead( BDMFlash->Device, BDMFlash->Base + Location )) < 0)
      return (FlashErrorRead_c);

  *Long = hlton( (unsigned long) Val );
  return (FlashErrorOkay_c);
}
#endif


static
FlashError_t
BDMFlashByteWrite( void *UserData, unsigned long Location, unsigned char Byte )
{
  BDMFlash_t *BDMFlash = (BDMFlash_t *)UserData;

  if (BDMTargetByteWrite( BDMFlash->Device, BDMFlash->Base + Location, Byte ) != 0)
      return (FlashErrorWriteAccess_c);

  return (FlashErrorOkay_c);
}



static
FlashError_t
BDMFlashWordWrite( void *UserData, unsigned long Location, unsigned short Word )
{
  BDMFlash_t *BDMFlash = (BDMFlash_t *)UserData;

  if (BDMTargetWordWrite( BDMFlash->Device, BDMFlash->Base + Location, Word ) != 0)
      return (FlashErrorWriteAccess_c);
  return (FlashErrorOkay_c);
}



static
FlashError_t
BDMFlashLongWrite( void *UserData, unsigned long Location, unsigned long Long )
{
  BDMFlash_t *BDMFlash = (BDMFlash_t *)UserData;

  if (BDMTargetLongWrite( BDMFlash->Device, BDMFlash->Base + Location, Long ) != 0)
      return (FlashErrorWriteAccess_c);
  return (FlashErrorOkay_c);
}


static
FlashError_t
BDMFlashElementRead( void *UserData, 
		     unsigned long Location, 
		     unsigned int BytesWide, /* what type of read */
		     void /*out*/ *Element )
{
  FlashError_t Error;

  switch (BytesWide)
    {
    case 1:
      Error = BDMFlashByteRead( UserData, Location, Element );
      break;
    case 2:
      Error = BDMFlashWordRead( UserData, Location, Element );

      /* BDM functions give us the number in host-order, but we want
       * to pass it up in network order because that's the byte order
       * that the data was originally in as seen under 68K
       * architecture */
      *(unsigned short *)Element = htons( *(unsigned short *)Element );
      break;
    case 4: /* not yet implemented */
    default:
      return (FlashErrorWidth_c);
    }
  return (Error);
}

static
FlashError_t
BDMFlashElementWrite( void *UserData,
		      unsigned long Location,
		      unsigned int BytesWide, /* what type of write */
		      void const *Element )
{
  FlashError_t Error;

  switch (BytesWide)
    {
    case 1:
      Error = BDMFlashByteWrite( UserData, Location, *(unsigned char *)Element );
      break;
    case 2:
      /* We want the value to be written in the same sequence as we
       * receive it.  The writer (the ColdFire/BDM core) uses network
       * order.  Therefore the byte order we're receiving it is also
       * network order.  The BDM functions take values in host order.
       * Therefore we need to convert from network order to host
       * order.  */
      Error = BDMFlashWordWrite( UserData, Location, ntohs( *(unsigned short *)Element ) );
      break;
    case 4:
      Error = BDMFlashLongWrite( UserData, Location, ntohl( *(unsigned long *)Element ) );
      break;
    default:
      return (FlashErrorWidth_c);
    }
  return (Error);
}



static Flash_t Flash;
static BDMFlash_t BDMFlash;

/* configure for arrangement of flash */
FlashError_t
BDMFlashConfigSet( int fd,
		   unsigned int Base, 
		   unsigned int Chips, 
		   unsigned int Bytes )
{
  FlashError_t Error;

  PRINTD( "Base = %u, Chips = %u, Bytes = %u\n", Base, Chips, Bytes );
  BDMFlash.Device = fd;
  BDMFlash.Base = Base;

  Error = FlashInit( &Flash,
		     &BDMFlash,
		     BDMFlashElementRead,
		     BDMFlashElementWrite,
		     Bytes,
		     1, /* BDM in Big-Endian */
		     Chips );
  return (Error);
}




/* store into flash */
FlashError_t
BDMFlashWrite( unsigned int addr, unsigned char *mem, size_t count)
{
  return (FlashWrite( &Flash,
		      addr, 
		      mem, 
		      count ));
}

/* erase flash */
FlashError_t
BDMFlashErase( void )
{
  return (FlashErase( &Flash ));
}


FlashError_t
BDMFlashProbe( FlashStyle_t /*out*/ *Style )
{
  *Style = Flash.Style;
  return (FlashErrorOkay_c);
}



/* erase flash, addr is base of a sector of a byte-wide flash */
FlashError_t
BDMFlashEraseSector( unsigned int addr )
{
  return (FlashEraseSector( &Flash,
			    addr /* base of sector of byte-wide flash */ ));
}

/* erase flash, addr is base of a block of a byte-wide flash */
FlashError_t
BDMFlashEraseBlock( unsigned int addr )
{
  return (FlashEraseBlock( &Flash,
			    addr /* base of sector of byte-wide flash */ ));
}

FlashError_t
BDMFlashIDRead( FlashID_t /*out*/ *ID )
{
  return (FlashIDRead( &Flash, ID ));
}

FlashError_t
BDMFlashDetect( FlashInfo_t const * /*out*/ *FlashInfo )
{
  return (FlashDetect( &Flash, FlashInfo ));
}
    


/*EOF*/
