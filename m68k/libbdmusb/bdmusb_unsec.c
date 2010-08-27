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
#include <unistd.h>

#include "tblcf/version.h"
#include "log_cmdline.h"
#include "tblcf/tblcf.h"

#include "bdmusb.h"

char device_name[128];
int device_no=0;

unsigned char unsec_instr[4];
unsigned int unsec_instruction_len;
unsigned char flash_clock_divisor;

/* returns number of parameters or a negative number in case of error */
char handleoptions(int argc,char *argv[]) {
	int i,notoption=0;
	for (i=1; i< argc;i++) {
		if (argv[i][0] == '/' || argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 'd':
				case 'D':
					/* USB device number */
					{
						strcpy (device_name, argv[i]+2);
						print_screen("Device #: %s\n",device_name);
						break;
					}
				case 'l':
				case 'L':
					/* -l<log file> */
					open_log(argv[i]+2);
					break;
				default:
					print_screen("Unknown option %s\n",argv[i]);
					return(-1);
			}
		} else {
			if (notoption==0) {
				unsigned long int unsec_instruction;
				sscanf(argv[i],"%lX",&unsec_instruction);
				print_screen("Unsecure instruction: 0x%lX\n",unsec_instruction);
				unsec_instr[0]=unsec_instruction&0xff;
				unsec_instr[1]=(unsec_instruction>>8)&0xff;
				unsec_instr[2]=(unsec_instruction>>16)&0xff;
				unsec_instr[3]=(unsec_instruction>>24)&0xff;
			} else if (notoption==1) {
				sscanf(argv[i],"%d",&unsec_instruction_len);
				print_screen("Instruction length: %d\n",unsec_instruction_len);
			} else if (notoption==2) {
				unsigned int flash_clock_div;
				sscanf(argv[i],"%X",&flash_clock_div);
				flash_clock_divisor = flash_clock_div&0xff;
				print_screen("Flash clock divisor: 0x%X\n",flash_clock_divisor);
			} else {
				print_screen("Too many parameters!\n");
			}
			notoption++;
		}
	}
	return(notoption);
}

void usage(void) {
	print_screen("\nUsage:\n\nTBLCF_UNSEC [<options>] <LR_INSTR> <LEN> <CLKDIV>\n\n");
	print_screen("<LR_INSTR>\tLOCKOUT_RECOVERY JTAG instruction in hexadecimal\n");
	print_screen("<LEN>\t\tLength of the unsecure instruction in decimal\n");
	print_screen("<CLKDIV>\tFlash clock divisor in hexadecimal\n\n");
	print_screen("Options:\n\n");
	print_screen("-D<n>\t\tSpecify which USB device to work with\n");
	print_screen("-L<log file>\tWrite messages into a log file\n\n");
}

int main(int argc, char *argv[]) {
	int i;
	unsigned char idcode[4]={0,0,0,1};	/* IDCODE instruction */
	unsigned char part_id[4];
	print_screen("Turbo BDM Light ColdFire Unsecure Utility ver %01X.%01X.\nCompiled on %s, %s.\n",TBLCF_UNSEC_VERSION>>4,TBLCF_UNSEC_VERSION&0x0f,__DATE__,__TIME__);
	print_screen("TBLCF DLL version: %02X\n",tblcf_version());
	/* handle user supplied options */
	i=handleoptions(argc,argv);
	if (i != 3) {	/* number of parameters is incorrect */
		print_screen("Number of parameters incorrect\n");
		usage();
		return(1);
	}
	print_screen("found %d Turbo BDM Light ColdFire device(s)\n",i=bdmusb_init());
	if (device_no>=i) {
		print_screen("Not enough devices connected to work with device #%d\n",device_no);
		return(1);
	}
	device_no = bdmusb_usb_open(device_name);
	tblcf_set_target_type(device_no, T_JTAG);	/* select JTAG target */
	tblcf_jtag_sel_shift(device_no, 1);		/* select instruction path */
	tblcf_jtag_write(device_no,
                   unsec_instruction_len,1,idcode+4-((unsec_instruction_len-1)>>3));	/* shift the IDCODE instruction in */
	tblcf_jtag_sel_shift(device_no, 0);		/* select data path */
	tblcf_jtag_read(device_no, 32,1,part_id);	/* get the ID */
	printf("Part ID: %02X%02X%02X%02X\n",part_id[0],part_id[1],part_id[2],part_id[3]);
	tblcf_jtag_sel_shift(device_no, 1);		/* select instruction path */
	tblcf_jtag_write(device_no, unsec_instruction_len,1,unsec_instr);	/* shift the LOCKOUT_RECOVERY instruction in */
	tblcf_jtag_sel_shift(device_no, 0);		/* select data path */
	tblcf_jtag_write(device_no, 7,1,&flash_clock_divisor);				/* shift the flash clk div in */
	tblcf_jtag_sel_reset(device_no);			/* go to TEST-LOGIC-RESET */
	usleep(300 * 1000);		/* wait to make sure the flash is mass erased before exiting */
	bdmusb_usb_close(device_no);
	printf("Done.\n");
	return(0);
}


