#ifndef BDMTargetAddress_Included_M
#define BDMTargetAddress_Included_M

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

/* File:	BDMTargetAddress.h
 * Purpose:	Abstract manipulation of target address space through BDM.
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
 * $Log: BDMTargetAddress.h,v $
 * Revision 1.1  2003/06/03 15:42:04  codewiz
 * Import userland tools from bdm-fiedler
 *
 * Revision 1.2  2000/04/20 04:56:23  wuth
 * GPL.  Abstract flash interface.
 *
 * Revision 1.1  2000/03/28 20:24:41  wuth
 * Break out flash code into separate executable.  Make run under Chris Johns BDM driver.
 *
 * @#[BasedOnTemplate: template.h version 2]
 */

/* Write byte into target address space through BDM */
int /* 0 if success, or error code */
BDMTargetByteWrite( int fd, unsigned int addr, unsigned char byte );

/* Write word into target address space through BDM */
int /* 0 if success, or error code */
BDMTargetWordWrite( int fd, unsigned int addr, unsigned short word );

/* Write long into target address space through BDM */
int /* 0 if success, or error code */
BDMTargetLongWrite( int fd, unsigned int addr, unsigned long Long );

/* Read byte into target address space through BDM */
int /* 0 if success, or error code */
BDMTargetByteRead( int fd, unsigned int addr );

/* Read word into target address space through BDM */
int /* 0 if success, or error code */
BDMTargetWordRead( int fd, unsigned int addr );


#endif /* BDMTargetAddress_Included_M */
/*** Emacs configuration ***/
/* Local Variables: */
/* mode:C */
/* End: */
/*EOF*/
