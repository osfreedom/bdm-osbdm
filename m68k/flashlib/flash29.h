/* $Id: flash29.h,v 1.2 2008/07/31 01:53:44 cjohns Exp $
 *
 * Header for 29Fxxx and 49Fxxx flash driver.
 *
 * 2003-12-28 Josef Wolf (jw@raven.inka.de)
 *
 * Portions of this program which I authored may be used for any purpose
 * so long as this notice is left intact. 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* This function will be called by flash_filter and is responsible to register
   the driver
*/
void init_flash29(int num);

/* following #define's should probably be set by configure
 */

/* Setting this to 0 produces more generic (and compact) code which is able
   to program all bus widths at the expense of performance. Setting this
   to 1 approximately doubles the programming speed on a 20MHz 68332 with
   Am29F400 in 16-bit-mode.
*/
#define FLASH_OPTIMIZE_FOR_SPEED 0

/* When FLASH_OPTIMIZE_FOR_SPEED==1, selecting only required bus widths can
   reduce size of generated code
*/
#define FLASH_BUS_WIDTH1 1
#define FLASH_BUS_WIDTH2 1
#define FLASH_BUS_WIDTH4 1

#if FLASH_OPTIMIZE_FOR_SPEED
# if ( !FLASH_BUS_WIDTH1 && !FLASH_BUS_WIDTH2 && !FLASH_BUS_WIDTH4 )
#  error At least one bus width must be defined when FLASH_OPTIMIZE_FOR_SPEED!=0
# endif
#endif
