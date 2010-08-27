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

#include "tblcf/tblcf_hwdesc.h"
#include "bdmusb-hwdesc.h"
#include "bdmusb.h"
#include "tblcf/tblcf_usb.h"
#include "tblcf/version.h"
#include "bdmusb_bt.h"
#include "log_cmdline.h"
#include "srec.h"
#include "commands.h"

/*
Changes:
	1.0		Initial coding

*/

function_descriptor_t function_descriptor;	/* this structure describes what the program should do */

/* returns number of parameters or a negative number in case of error */
char handleoptions(int argc,char *argv[]) {
	int i,notoption=0;
	for (i=1; i< argc;i++) {
		if (argv[i][0] == '/' || argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 'b':
				case 'B':
					/* replace boot sector */
					function_descriptor.replace_boot=1;
					break;
				case 'd':
				case 'D':
					/* USB device number */
					{
						strcpy (function_descriptor.device_name, argv[i]+2);
						print_screen("Device #: %s\n",function_descriptor.device_name);
						break;
					}
				case 'U':
				case 'u':
					/* request firmware upgrade */
					function_descriptor.request_upgrade=1;
					break;
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
			if (notoption==0) { /* the first parameter which is not an option is the S-record file */
				strncpy(function_descriptor.s_rec_filename,argv[i],MAX_PATH);
			}
			if (notoption>=1) {
				print_screen("Too many parameters!\n");
			}
			notoption++;
		}
	}
	return(notoption);
}

void usage(void) {
	print_screen("\nUsage:\n\nTBLCF_BT <S-record file> [<options>]\n\n");
	print_screen("Options:\n\n");
	print_screen("-B\t\tReplace BOOT sector\n");
	print_screen("-D<n>\t\tSpecify which USB device to work with\n");
	print_screen("-L<log file>\tWrite messages into a log file\n");
	print_screen("-U\t\tRequest Firmware upgrade\n");
	print_screen("\n");
}

int main(int argc, char *argv[]) {
	int i;
	unsigned int start,end;	/* indexes into the array delimiting block of data to be programmed into flash */
	print_screen("Turbo BDM Light ColdFire Bootloader ver %01X.%01X. Compiled on %s, %s.\n",TBLCF_BOOTLOADER_VERSION>>4,TBLCF_BOOTLOADER_VERSION&0x0f,__DATE__,__TIME__);
	/* initialise function_descriptor */
	function_descriptor.replace_boot=0;
	function_descriptor.device_no=0;
	function_descriptor.request_upgrade=0;
	for (i=0;i<sizeof(function_descriptor.boot_sector_data);i++) function_descriptor.boot_sector_data[i]=0xff;
	for (i=0;i<sizeof(function_descriptor.flash_data);i++) function_descriptor.flash_data[i]=0xff;
	/* now handle user supplied options */
	i=handleoptions(argc,argv);
	if ((i != 1)&&(function_descriptor.request_upgrade==0)) {	/* number of parameters is incorrect */
		print_screen("Number of parameters incorrect or other error\n");
		usage();
		return(1);
	}
	bdmusb_init();
	/* request firmware upgrade on next power up (if required by the user) */
	if (function_descriptor.request_upgrade) {
		unsigned char data[6];
		bdmusb_find_supported_devices();
		print_screen("found %d Turbo BDM Light ColdFire device(s)\n",i=tblcf_usb_cnt());
		if (function_descriptor.device_no>=i) {
			print_screen("Not enough devices connected to work with device #%d\n",
                   function_descriptor.device_no);
			return(1);
		}
		function_descriptor.device_no = bdmusb_usb_open(function_descriptor.device_name);
		/* request bootloader action op next power-up */
		if ((i)||(bdmusb_request_boot())) {
			print_screen("USB communication problem or CMD_SET_BOOT command failure\n");
			return(1);
		}
		print_screen("HC08JB16 ICP will execute on next power-up. Disconnect and reconnect the device.\n");
		bdmusb_usb_close(function_descriptor.device_no);
		return(0);
	}
	if (s_rec_process()) return(1);	/* process the S-record file, exit in case of error */
	/* now open communication with the device */
	//tblcf_usb_find_devices(JB16ICP_PID);
	print_screen("found %d HC08JB16 ICP device(s)\n",i=tblcf_usb_cnt());
	if (function_descriptor.device_no>=i) {
		print_screen("Not enough devices connected to work with device #%d\n",
                 function_descriptor.device_no);
		return(1);
	}
	function_descriptor.device_no = bdmusb_usb_open(function_descriptor.device_name);
	/* boot sector reprogramming (if requested by user) */
	i=icp_verify(function_descriptor.device_no,
               function_descriptor.boot_sector_data,
               TBLCF_FLASH_BOOT_START, TBLCF_FLASH_BOOT_END-TBLCF_FLASH_BOOT_START+1);
	if (i<0) {
		print_screen("USB communication problem\n");
		bdmusb_usb_close(function_descriptor.device_no);
		return(1);
	}
	if (function_descriptor.replace_boot) {
		/* user wants to overwrite the boot sector, first check, whether it is really needed */
		if (i) {
			print_screen("Boot sector contents different, performing mass erase\n");
			/* erase flash of the device */
			i=icp_mass_erase(function_descriptor.device_no);
			if (i) {
				print_screen("USB communication problem or mass erase failure\n");
				bdmusb_usb_close(function_descriptor.device_no);
				return(1);
			}
			print_screen("Mass erase done, programming boot sector\n");
			/* program the boot sector contents */
			i=icp_program(function_descriptor.device_no,
                    function_descriptor.boot_sector_data,
                    TBLCF_FLASH_BOOT_START, TBLCF_FLASH_BOOT_END-TBLCF_FLASH_BOOT_START+1);
			if (i) {
				print_screen("USB communication problem or programming failure\n");
				bdmusb_usb_close(function_descriptor.device_no);
				return(1);
			}
			print_screen("Programming done, verifying boot sector\n");
			/* verify that the contents is now correct */
			i=icp_verify(function_descriptor.device_no,
                   function_descriptor.boot_sector_data,
                   TBLCF_FLASH_BOOT_START, TBLCF_FLASH_BOOT_END-TBLCF_FLASH_BOOT_START+1);
			if (i) {
				print_screen("USB communication problem or verification failure\n");
				bdmusb_usb_close(function_descriptor.device_no);
				return(1);
			}
			print_screen("Verification done, boot sector OK. You can start breathing again.\n");
		} else {
			print_screen("Boot sector contents identical, -B option ignored\n");
		}
	} else {
		/* user does not want to reprogram the boot sector */
		if (i) {
			print_screen("Boot sector contents differs from S-record, update required\n");
			bdmusb_usb_close(function_descriptor.device_no);
			return(1);
		}
	}
	/* flash programming */
	for (i=TBLCF_FLASH_START;i<TBLCF_FLASH_END;i+=ICP_FLASH_BLOCK_SIZE) {
		/* erase all flash blocks one by one */
		print_screen("Erasing block at address: 0x%04X\r",i);
		if (icp_block_erase(function_descriptor.device_no, i)) {
			print_screen("\nUSB communication problem or block erase failure\n");
			bdmusb_usb_close(function_descriptor.device_no);
			return(1);
		}
	}
	print_screen("\n");
	/* set the boot_state variable to erased state (0xFF 0xFF) before programming */
	function_descriptor.flash_data[TBLCF_BOOT_STATE-TBLCF_FLASH_START]=TBLCF_BOOT_STATE_BLANK>>8;
	function_descriptor.flash_data[TBLCF_BOOT_STATE-TBLCF_FLASH_START+1]=TBLCF_BOOT_STATE_BLANK&&0xff;
	/* now do the actual programming */
	/* the algorithm below will split the flash into several sections if there is at least 8 unprogrammed bytes beetwen two sections */
	/* for TBLCF firmware there will typically be only 2 sections: code of the application at the beginning of flash and secondary vectors at the end */
	end=TBLCF_FLASH_START;
	while (1) {
		/* look for first byte in non-erased state */
		start=end;
		while ((function_descriptor.flash_data[start-TBLCF_FLASH_START]==0xff) &&
           (start<=TBLCF_FLASH_END)) start++;	/* find first byte != 0xff */
		if (start>TBLCF_FLASH_END) break;	/* end of array */
		/* now look for block of 8 bytes in erased state */
		end=start;
		do {
			while ((function_descriptor.flash_data[end-TBLCF_FLASH_START]!=0xff) &&
             (end<TBLCF_FLASH_END)) end++;		/* find first byte == 0xff */
			if (end<TBLCF_FLASH_END) {
				i=1;
				while ((function_descriptor.flash_data[end+i-TBLCF_FLASH_START]==0xff) &&
               ((end+i)<TBLCF_FLASH_END)&&(i<8)) i++;	/* keep going for 7 more bytes (max) */
				if (i<8) end+=i;
			}
		} while ((i<8)&&(end<TBLCF_FLASH_END));
		if (end<TBLCF_FLASH_END) end--;	/* go back 1 byte (it is blank anyway) */
		/* program the block */
		print_screen("Programming block from 0x%04X to 0x%04X\n",start,end);
		i=icp_program(function_descriptor.device_no,
                  function_descriptor.flash_data+(start-TBLCF_FLASH_START),
                  start, end-start+1);
		if (i) {
			print_screen("USB communication problem or programming failure\n");
			bdmusb_usb_close(function_descriptor.device_no);
			return(1);
		}
		/* verify the block */
		print_screen("Verifying block from 0x%04X to 0x%04X ",start,end);
		i=icp_verify(function_descriptor.device_no,
                 function_descriptor.flash_data+(start-TBLCF_FLASH_START),
                 start, end-start+1);
		if (i) {
			print_screen("\nUSB communication problem or verification failure\n");
			bdmusb_usb_close(function_descriptor.device_no);
			return(1);
		}
		print_screen("- OK\n",start,end);
		end++;	/* increment end again */
	}
	/* all flash programmed and verified, program the boot_state to 0xFF 0x01 to enable the application */
	print_screen("All flash programmed and verified, enabling the application\n");
	function_descriptor.flash_data[TBLCF_BOOT_STATE-TBLCF_FLASH_START]=TBLCF_BOOT_STATE_APPLOK>>8;
	function_descriptor.flash_data[TBLCF_BOOT_STATE-TBLCF_FLASH_START+1]=TBLCF_BOOT_STATE_APPLOK&&0xff;
	i=icp_program(function_descriptor.device_no,
                function_descriptor.flash_data+(TBLCF_BOOT_STATE-TBLCF_FLASH_START),
                TBLCF_BOOT_STATE, 2);	/* bootloader_state is 2 bytes long */
	if (i) {
		print_screen("USB communication problem or programming failure\n");
		bdmusb_usb_close(function_descriptor.device_no);
		return(1);
	}
	/* now verify that the bootloader state is programmed correctly */
	i=icp_verify(function_descriptor.device_no,
               function_descriptor.flash_data+(TBLCF_BOOT_STATE-TBLCF_FLASH_START),
               TBLCF_BOOT_STATE, 2);
	if (i) {
		print_screen("USB communication problem or bootloader state verification failure\n");
		bdmusb_usb_close(function_descriptor.device_no);
		return(1);
	}
	print_screen("Flash programming complete, disconnect & reconnect the device\n");
	bdmusb_usb_close(function_descriptor.device_no);
	return(0);
}


