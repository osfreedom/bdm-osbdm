#ifndef BDMFlash_Included_M
#define BDMFlash_Included_M

/* @#Copyright (c) 2000, Brett Wuth. */
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

/* File:	BDMFlash.h
 * Purpose:	Flash Driver through BDM
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
 * $Log: BDMFlash.h,v $
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
 * @#[BasedOnTemplate: template.h version 2]
 */

#include <Flash.h>
#include <stdlib.h> /* size_t */

/* configure for arrangement of flash */
FlashError_t
BDMFlashConfigSet( int fd,
		   unsigned int Base, 
		   unsigned int Chips, 
		   unsigned int Bytes );

/* store into flash */
FlashError_t
BDMFlashWrite( unsigned int addr, unsigned char *mem, size_t count);

/* erase flash */
FlashError_t
BDMFlashErase( void );

FlashError_t
BDMFlashProbe( FlashStyle_t /*out*/ *Style );

/* erase flash, addr is base of a sector of a byte-wide flash */
FlashError_t
BDMFlashEraseSector( unsigned int addr );

/* erase flash, addr is base of a block of a byte-wide flash */
FlashError_t
BDMFlashEraseBlock( unsigned int addr );

FlashError_t
BDMFlashIDRead( FlashID_t /*out*/ *ID );


FlashError_t
BDMFlashDetect( FlashInfo_t const * /*out*/ *FlashInfo );

#endif /* BDMFlash_Included_M */
/*** Emacs configuration ***/
/* Local Variables: */
/* mode:C */
/* End: */
/*EOF*/
