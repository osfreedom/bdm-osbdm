/*
    Turbo BDM Light ColdFire - interface DLL
    Copyright (C) 2006 Daniel Malik

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

#include "log.h"
#include "version.h"
#include "tblcf.h"
#include "tblcf_usb.h"
#include "tblcf_hwdesc.h"
#include "commands.h"

#include "../bdmusb.h"

/* define symbol "LOG" during compilation to produce a log file tblcf_dll.log */

//static unsigned char usb_data[MAX_DATA_SIZE+2];
extern unsigned char usb_data[MAX_DATA_SIZE+2];

/* returns version of the DLL in BCD format */
unsigned char tblcf_version(void) {
	return(TBLCF_DLL_VERSION);
}

/* returns status of the last command: 0 on sucess and non-zero on failure */
unsigned char tblcf_get_last_sts(int dev) {
    unsigned char status = bdmusb_get_last_sts_value(dev);
    if (status != BDM_RC_OK)
	return 1;
    return BDM_RC_OK;
}

/* resynchronizes communication with the target (in case of noise, etc.) */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_resynchronize(int dev) {
	usb_data[0]=1;	 /* get 1 byte */
	usb_data[1]=CMD_TBLCF_TARGET_RESYNCHRONIZE;
	tblcf_usb_recv_ep0(dev, usb_data);
	bdm_print("TBLCF_RESYNCHRONIZE: (0x%02X)\r\n",usb_data[0]);
	return(!(usb_data[0]==CMD_TBLCF_TARGET_RESYNCHRONIZE));
}

/* asserts the TA signal for the specified time (in 10us ticks) */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_assert_ta(int dev, unsigned char duration_10us) {
	usb_data[0]=1;	 /* get 1 byte */
	usb_data[1]=CMD_TBLCF_TARGET_ASSERT_TA;
	usb_data[2]=duration_10us;
	tblcf_usb_recv_ep0(dev, usb_data);
	bdm_print("TBLCF_ASSERT_TA: duration %d us (0x%02X)\r\n",(int)10*usb_data[2],usb_data[0]);
	return(!(usb_data[0]==CMD_TBLCF_TARGET_ASSERT_TA));
}

/* JTAG - go from RUN-TEST/IDLE to SHIFT-DR or SHIFT-IR state */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_jtag_sel_shift(int dev, unsigned char mode) {
	usb_data[0]=1;	 /* get 1 byte */
	usb_data[1]=CMD_JTAG_GOTOSHIFT;
	usb_data[2]=mode;
	tblcf_usb_recv_ep0(dev, usb_data);
	bdm_print("TBLCF_JTAG_SEL_SHIFT: SHIFT-%cR\r\n",mode?'I':'D');
	return(!(usb_data[0]==CMD_JTAG_GOTOSHIFT));
}

/* JTAG - go from RUN-TEST/IDLE to TEST-LOGIC-RESET state */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_jtag_sel_reset(int dev) {
	usb_data[0]=1;	 /* get 1 byte */
	usb_data[1]=CMD_JTAG_GOTORESET;
	tblcf_usb_recv_ep0(dev, usb_data);
	bdm_print("TBLCF_JTAG_SEL_RESET\r\n");
	return(!(usb_data[0]==CMD_JTAG_GOTORESET));
}

/* JTAG - write data */
/* parameter exit: ==0 : stay in SHIFT-xx, !=0 : go to RUN-TEST/IDLE when done */
/* data: shifted in LSB (last byte) first, unused bits (if any) are in the MSB (first) byte */
void tblcf_jtag_write(int dev, unsigned char bit_count, unsigned char exit, unsigned char *buffer) {
	int i;
	usb_data[0]=(bit_count>>3)+((bit_count&0x07)!=0)+3;	 /* send all data bits & 3 more bytes */
	usb_data[1]=CMD_JTAG_WRITE;
	usb_data[2]=exit;
	usb_data[3]=bit_count;
	for (i=0;i<((bit_count>>3)+1);i++) usb_data[4+i]=*(buffer+i);	/* copy data */
	tblcf_usb_send_ep0(dev, usb_data);
	bdm_print("TBLCF_JTAG_WRITE: %d bits (%sexit to RUN-TEST/IDLE)\r\n",bit_count,exit?"":"NO ");
	bdm_print_dump(usb_data+4,(bit_count>>3)+((bit_count&0x07)!=0));
}

/* JTAG - read data */
/* parameter exit: ==0 : stay in SHIFT-xx, !=0 : go to RUN-TEST/IDLE when done */
/* data: shifted in LSB (last byte) first, unused bits (if any) are in the MSB (first) byte */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_jtag_read(int dev, unsigned char bit_count, unsigned char exit, unsigned char *buffer) {
	int i;
	usb_data[0]=(bit_count>>3)+((bit_count&0x07)!=0)+1;	 /* get all data bits & the status byte */
	usb_data[1]=CMD_JTAG_READ;
	usb_data[2]=exit;
	usb_data[3]=bit_count;
	tblcf_usb_recv_ep0(dev, usb_data);
	for (i=0;i<((bit_count>>3)+((bit_count&0x07)!=0));i++) *(buffer+i)=usb_data[1+i];	/* copy data */
	bdm_print("TBLCF_JTAG_READ: %d bits (%sexit to RUN-TEST/IDLE)\r\n",bit_count,exit?"":"NO ");
	bdm_print_dump(usb_data+1,(bit_count>>3)+((bit_count&0x07)!=0));
	return(!(usb_data[0]==CMD_JTAG_READ));
}

#if 0
BOOL LibMain(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved) {
	char path[MAX_PATH];
	char *charp;
	time_t time_now;
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
			tblcf_usb_init();			/* initialise USB */
			#ifdef LOG
				GetModuleFileName(NULL,path,sizeof(path));
				charp=strchr(path,'\\');
				if (charp!=NULL) {
					while (strchr(charp+1,'\\')) charp=strchr(charp+1,'\\');	/* find the last backslash */
					strcpy(charp+1,"tblcf_dll.log");
					log_file=fopen(path,"wb");
				}
				bdm_print("Log file path: %s\r\n",path);
				bdm_print("Turbo BDM Light DLL v%1d.%1d. Compiled on %s, %s.\r\n",
                    TBLCF_DLL_VERSION/16,TBLCF_DLL_VERSION&0x0f,__DATE__,__TIME__);
				time(&time_now);
				bdm_print("Log file created on: %s\r", ctime(&time_now));
			#endif
            break;
        case DLL_PROCESS_DETACH:
			#ifdef LOG
				time(&time_now);
				bdm_print("End of log file: %s\r", ctime(&time_now));
				fclose(log_file);
			#endif
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}
#endif
