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
 * To use the flashcfm algorithm:
 *
 * set the following flash varialbes (flash_set_var)
 *
 * varname            (default)       note
 * IPSBAR             (0x40000000)
 * MCF_CFM_CFMCLKD    (0x1D0002)      ipsbar offset    
 * MCF_CFM_CFMUSTAT   (0x1D0020)      ipsbar offset
 * MCF_CFM_CFMCMD     (0x1D0024)      ipsbar offset
 * FLASHBAR_REG       (0x0C04)
 * FLASH_SIZE         (256 * 1024)
 * FLASH_BACKDOOR     (0x04000000)    ipsbar offset
 *
 * NOTE: The target and flash clock must be setup to the correct flashing
 * frequency prior to invoking this flash algorithm
 *
 * NOTE: Flash detection simply checks flashbar, so flashbar must be configured
 * prior to calling flash_register
 *
 * NOTE: Flash addresses and write memory sizes need to be 4 byte aligned!
 *
 */

/* This function will be called by flash_filter and is responsible to register
   the driver
*/
void init_flashcfm(int num);
