#ifndef BDMDriver_Included_M
#define BDMDriver_Included_M

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

/* File:	BDMDriver.h
 * Purpose:	Control which Linux BDM driver is used.
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
 * $Log: BDMDriver.h,v $
 * Revision 1.1  2003/12/29 22:18:49  codewiz
 * Move tools/bdm_abstraction to m68k/bdmabstraction and autoconfiscate.
 *
 * Revision 1.1  2003/06/03 15:42:03  codewiz
 * Import userland tools from bdm-fiedler
 *
 * Revision 1.2  2000/04/20 04:56:22  wuth
 * GPL.  Abstract flash interface.
 *
 * Revision 1.1  2000/03/28 20:24:41  wuth
 * Break out flash code into separate executable.  Make run under Chris Johns BDM driver.
 *
 * @#[BasedOnTemplate: template.h version 2]
 */

#define BDMDriverFiedler_M (1)
#define BDMDriverJohns_M   (2)

#define BDMDriverVersion_M BDMDriverJohns_M

#if BDMDriverVersion_M == BDMDriverFiedler_M
#include <bdmcf.h>
#define _COMPILING_
#  include <bdmops.h>
#undef _COMPILING_
#elif BDMDriverVersion_M == BDMDriverJohns_M
#include <BDMlib.h>
#else
# error "BDMDriverVersion_M not set to known value"
#endif

#endif /* BDMDriver_Included_M */
/*** Emacs configuration ***/
/* Local Variables: */
/* mode:C */
/* End: */
/*EOF*/
