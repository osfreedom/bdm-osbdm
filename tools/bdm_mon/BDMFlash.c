/* @#Copyright (c) 2000, Brett Wuth.*/
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
 * Purpose:	Work with target flash through BDM connector.
 * Author:	Brett Wuth
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
 * Revision 1.6  2000/08/03 06:29:18  wuth
 * MultiProject Sync; Support Micron-style flash; Report flash model
 *
 * Revision 1.5  2000/07/25 13:51:09  wuth
 * Working sector erase.  Better error reports.
 *
 * Revision 1.4  2000/07/14 18:38:55  wuth
 * Flash error status; Fix sector erase support; Command line sector erase
 *
 * Revision 1.3  2000/04/20 04:56:23  wuth
 * GPL.  Abstract flash interface.
 *
 * Revision 1.2  2000/03/30 03:36:56  wuth
 * BDMFlash can now write beyond first 1 KiB.
 *
 * Revision 1.1  2000/03/28 20:28:50  wuth
 * Break out flash code into separate executable.  Make run under Chris Johns BDM driver.
 *
 * @#[BasedOnTemplate: template.c version 2]
 */

#include <BDMFlash.h>
#include <bdmops.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static
void
Usage( char const *ProgName )
{
  fprintf( stderr, 
	   "usage: %s DeviceFile FlashBaseAddress NumChips ByteWidth "
	   "[probe | detect | erase | SectorErase Offset | BlockErase Offset | write BinaryFile Offset | read BinaryFile Offset Length]\n",
	   ProgName );
  exit( EXIT_FAILURE );
}


static
void
Probe( int ArgC, char *ArgV[] )
{
  FlashError_t Error;
  FlashStyle_t Style;

  if (ArgC != 6)
    Usage( ArgV[0] );

  if ((Error = BDMFlashProbe( &Style )) != FlashErrorOkay_c)
    {
      fprintf( stderr, 
	       "Problem probing flash (code = %d, '%s').\n", 
	       (int) Error,
	       FlashErrorDescriptionEnglish[Error] );
      exit( EXIT_FAILURE );
    }

  printf( "Flash style is %s.\n", FlashStyleName[Style] );
}

static
void
Detect( int ArgC, char *ArgV[] )
{
  FlashError_t Error;
  FlashID_t ID;
  FlashInfo_t const *Info;

  if (ArgC != 6)
    Usage( ArgV[0] );

  Error = BDMFlashIDRead( &ID );
  if (Error != FlashErrorOkay_c)
    {
      fprintf( stderr, 
	       "Problem detecting flash (code = %d, '%s').\n", 
	       (int) Error,
	       FlashErrorDescriptionEnglish[Error] );
    }

  printf( "Flash ID = 0x%04x\n", ID );
  printf( "Manufacturer: %s\n", FlashManufacturerName( ID>>8 ) );

  if ((Error = BDMFlashDetect( &Info )) != FlashErrorOkay_c)
    {
      fprintf( stderr, 
	       "Problem detecting flash (code = %d, '%s').\n", 
	       (int) Error,
	       FlashErrorDescriptionEnglish[Error] );
      exit( EXIT_FAILURE );
    }

  if (Info == NULL)
    {
      printf( "Model not described.\n" );
    }
  else
    {
      printf( "Model: %s\n", Info->Model );
      printf( "Flash size is %lu.\n", Info->Size );
    }
}

static
void
Erase( int ArgC, char *ArgV[] )
{
  FlashError_t Error;

  if (ArgC != 6)
    Usage( ArgV[0] );

  if ((Error = BDMFlashErase()) != FlashErrorOkay_c)
    {
      fprintf( stderr, 
	       "Problem erasing flash (code = %d, '%s').\n", 
	       (int) Error,
	       FlashErrorDescriptionEnglish[Error] );
      exit( EXIT_FAILURE );
    }
}

static
void
SectorErase( int ArgC, char *ArgV[] )
{
  unsigned long Offset;
  FlashError_t Error;

  if (ArgC != 7)
    Usage( ArgV[0] );

  Offset = strtoul( ArgV[6], NULL, 0 );

  if ((Error = BDMFlashEraseSector( Offset )) != FlashErrorOkay_c)
    {
      fprintf( stderr, 
	       "Problem erasing flash sector at 0x%08lx; error code = %d, '%s'.\n", 
	       Offset,
	       (int) Error,
	       FlashErrorDescriptionEnglish[Error] );
      exit( EXIT_FAILURE );
    }
}

static
void
BlockErase( int ArgC, char *ArgV[] )
{
  unsigned long Offset;
  FlashError_t Error;

  if (ArgC != 7)
    Usage( ArgV[0] );

  Offset = strtoul( ArgV[6], NULL, 0 );

  if ((Error = BDMFlashEraseBlock( Offset )) != FlashErrorOkay_c)
    {
      fprintf( stderr, 
	       "Problem erasing flash block at 0x%08lx; error code = %d, '%s'.\n", 
	       Offset,
	       (int) Error,
	       FlashErrorDescriptionEnglish[Error] );
      exit( EXIT_FAILURE );
    }
}

static
void
Write( int ArgC, char *ArgV[] )
{
  FILE *InFile;
  char Buffer[1024];
  unsigned long Offset;

  if (ArgC != 8)
    Usage( ArgV[0] );

  InFile = fopen( ArgV[6], "r" );
  if (InFile == NULL)
    {
      fprintf( stderr, "Can't open %s\n", ArgV[6] );
      exit( EXIT_FAILURE );
    }

  Offset = strtoul( ArgV[7], NULL, 0 );

  for (;;)
    {
      FlashError_t Error;
      size_t NumRead;

      NumRead = fread( Buffer, 1, sizeof (Buffer), InFile );
      if (NumRead == 0)
	break;

      Error = BDMFlashWrite( Offset, Buffer, NumRead );
      if (Error != FlashErrorOkay_c)
	{
	  fprintf( stderr, 
		   "Problem writing flash.\n"
		   "While at offset 0x%08x attempting to write %lu bytes, error code %d, '%s'.\n",
		   Offset, 
		   (unsigned long) NumRead, 
		   (int) Error,
		   FlashErrorDescriptionEnglish[Error] );
	  exit( EXIT_FAILURE );
	}
      Offset += NumRead;
    }
}

static
void
Read( int BDMHandle, int ArgC, char *ArgV[] )
{
  FILE *OutFile;
  unsigned long Len;
  unsigned long ByteI;
  unsigned long Offset;

  if (ArgC != 9)
    Usage( ArgV[0] );

  Offset = strtoul( ArgV[7], NULL, 0 );
  Len = strtoul( ArgV[8], NULL, 0 );

  OutFile = fopen( ArgV[6], "w" );
  if (OutFile == NULL)
    {
      fprintf( stderr, "Can't open %s\n", ArgV[6] );
      exit( EXIT_FAILURE );
    }

  for (ByteI = 0; ByteI < Len; ByteI++)
    {
      int Byte;
      Byte = BDMTargetByteRead( BDMHandle, Offset+ByteI );
      if (Byte < 0)
	{
	  fprintf( stderr,
		   "Failed reading byte at offset %lu, code = %d\n", 
		   Offset+ByteI,
		   Byte );
	  exit( EXIT_FAILURE );
	}
      fputc( Byte, OutFile );
    }
}


int
main( int ArgC, char *ArgV[] )
{
  int BDMHandle;
  unsigned int Base;
  unsigned int Chips;
  unsigned int Bytes;
  unsigned long Offset;
  char *Operation;
  FlashError_t Error;

  if (ArgC < 6)
    Usage( ArgV[0] );

  if((BDMHandle = bdm_init(ArgV[1]))<0) 
      {
	  fprintf( stderr, 
		   "Problem opening bdm device %s, error code %d.\n", ArgV[1], BDMHandle );
	  return (EXIT_FAILURE);
      }

  Base = (unsigned int) strtoul( ArgV[2], NULL, 0 );
  Chips = (unsigned int) strtoul( ArgV[3], NULL, 0 );
  Bytes = (unsigned int) strtoul( ArgV[4], NULL, 0 );
  Operation = ArgV[5];

  Error = BDMFlashConfigSet( BDMHandle, Base, Chips, Bytes );
  if (Error != FlashErrorOkay_c)
    {
      fprintf( stderr, 
	       "Problem configuring flash; error code = %d '%s'.\n", 
	       (int) Error,
	       FlashErrorDescriptionEnglish[Error] );
    }

  if (strcasecmp( Operation, "erase" ) == 0)
    {
	Erase( ArgC, ArgV );
    }
  else if (strcasecmp( Operation, "SectorErase" ) == 0)
    {
	SectorErase( ArgC, ArgV );
    }
  else if (strcasecmp( Operation, "BlockErase" ) == 0)
    {
	BlockErase( ArgC, ArgV );
    }
  else if (strcasecmp( Operation, "write" ) == 0)
    {
	Write( ArgC, ArgV );
    }
  else if (strcasecmp( Operation, "read" ) == 0)
    {
	Read( BDMHandle, ArgC, ArgV );
    }
  else if (strcasecmp( Operation, "probe" ) == 0)
    {
	Probe( ArgC, ArgV );
    }
  else if (strcasecmp( Operation, "detect" ) == 0)
    {
	Detect( ArgC, ArgV );
    }
  else
    Usage( ArgV[0] );

  bdm_release( BDMHandle );

  return (EXIT_SUCCESS);
}


/*EOF*/
