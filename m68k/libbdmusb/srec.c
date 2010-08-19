/*
    Turbo BDM Light ColdFire
    Copyright (C) 2006  Daniel Malik

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

#include <stdio.h>
#include <string.h>

#include "log_cmdline.h"
#include "srec.h"
#include "tblcf_hwdesc.h"
#include "tblcf_bt.h"

/* converts BCD number to unsigned integer value */
unsigned int hex2dec(char *bcd) {
	unsigned int i,result;
	int charvalue;
	result=0;
	for (i=0;i<2;i++) {
		charvalue=*bcd-'0';
		if (charvalue>9) charvalue-='A'-'0'-10;
		if (charvalue>15) charvalue-='a'-'A';
		if ((charvalue>15) || (charvalue<0)) {
			printf("Hex2dec conversion error: %c\r\n",*bcd);
			return (0);
		}
		bcd++;
		result*=16;
		result+=charvalue;
	}
	return (result);
}

/* extract address and data from each line */
/* returns 0 on success and -1 on error */
/* checksum is not verified */
char s_line_process(char *line) {
	int i;
	unsigned int address;
	switch (line[1]) {
		case '0':
			/* S0 - header */
			for (i=0;i<((strlen(line)-8-2)/2);i++) line[i]=hex2dec(line+8+i*2);	/* convert the string to readable text */
			line[i]=0;	/* terminate the string */
			print_screen("S-rec: \"%s\"\r\n",line);
			return(0);
		case '1':
			/* S1 */
			address = 256*hex2dec(line+4) + hex2dec(line+6);
			if ((address>=TBLCF_FLASH_START)&&(address<=TBLCF_FLASH_END)) {
				/* application code */
				for (i=0;i<((strlen(line)-8-2)/2);i++) {
					if ((address+i)>TBLCF_FLASH_END) {
						print_screen("S-rec address (0x%04X) outside of expected range\r\n",address+i);
						return(-1);
					}
					function_descriptor.flash_data[address-TBLCF_FLASH_START+i]=hex2dec(line+8+i*2);
				}
				return(0);
			} else if ((address>=TBLCF_FLASH_BOOT_START)&&(address<=TBLCF_FLASH_BOOT_END)) {
				/* boot sector code */
				for (i=0;i<((strlen(line)-8-2)/2);i++) {
					if ((address+i)>TBLCF_FLASH_BOOT_END) {
						print_screen("S-rec address (0x%04X) outside of expected range\r\n",address+i);
						return(-1);
					}
					function_descriptor.boot_sector_data[address-TBLCF_FLASH_BOOT_START+i]=hex2dec(line+8+i*2);
				}
				return(0);
			}
			/* address outside of expected range */
			print_screen("S-rec address (0x%04X) outside of expected range\r\n",address);
			return(-1);
		case '9':
			/* S9 - end record */
		case '8':
			/* S8 - end record */
			return(0);
		default:
			print_screen("Unknown S-rec line: %s \r\n",line);
			return(-1);
	}
}

/* main S-rec routine which opens the file and reads it line by line */
/* returns 0 on success and !=0 on error */
char s_rec_process(void) {
	FILE *input;
	char line[MAX_LINE_LENGTH+1];
	input=fopen(function_descriptor.s_rec_filename,"r");
	if (input==NULL) {
		print_screen("Cannot open file \"%s\"\r\n",function_descriptor.s_rec_filename);
		return(-1);
	}
	while (!feof(input)) {
		if (fgets(line,MAX_LINE_LENGTH,input)==0) continue;	/* ignore blank lines */
		if (line[0]=='S') {
			if (s_line_process(line)) return(1);			/* error while processing the line? */
		}
	}
	fclose(input);
	return(0);
}

