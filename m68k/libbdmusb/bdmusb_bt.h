/*
    Turbo BDM Light ColdFire
    Copyright (C) 2006  Daniel Malik

    Changed to support the BDM project.
    Chris Johns (cjohns@user.sourgeforge.net)
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef TBLCF_BT_H
#define TBLCF_BT_H

#ifndef MAX_PATH
#define MAX_PATH 512
#endif

typedef struct {
	/* filename of the S-record file */
	char s_rec_filename[MAX_PATH];
  /* 0=do not replace boot secotr, 1= mass erase part and replace the boot
   * secotor */
	char replace_boot;
	/* data to be programmed into the boot sector */
	unsigned char boot_sector_data[TBLCF_FLASH_BOOT_END-TBLCF_FLASH_BOOT_START+1];
  /* data to be programmed into the application flash */
	unsigned char flash_data[TBLCF_FLASH_END-TBLCF_FLASH_START+1];
  char device_name[128];
	unsigned char device_no;
	char request_upgrade;
} function_descriptor_t;

extern function_descriptor_t function_descriptor;

#endif
