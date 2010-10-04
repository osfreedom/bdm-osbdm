/* $Id:
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

/*
 * To use the flashintelp30 algorithm:
 *
 * NOTE: You'll need to setup any chip selects etc.
 *
 * NOTE: No need to use an erase wait command for batch erases.  if doing
 * a sector erase you'll need to do an erase followed by an erase wait!  
 *
 * NOTE: Flash addresses need to be 2 byte aligned!
 *
 */

/* This function will be called by flash_filter and is responsible to register
   the driver
*/
void init_flashintelp30(int num);
