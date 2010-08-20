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
static unsigned char usb_data[MAX_DATA_SIZE+2];
extern unsigned char usb_data[MAX_DATA_SIZE+2];

/* returns version of the DLL in BCD format */
unsigned char tblcf_version(void) {
	return(TBLCF_DLL_VERSION);
}

/* initialises USB and returns number of devices found */
unsigned char tblcf_init(void) {
	unsigned char i;
	bdmusb_init();
	//tblcf_usb_find_devices(TBLCF_PID);	/* look for devices on all USB busses */
	i=tblcf_usb_cnt();
	bdm_print("TBDML_INIT: Usb initialised, found %d device(s)\r\n",i);
	return(i);							/* count the devices found and return the number */
}

/* opens a device with given number name */
/* returns device number on success and -1 on error */
int tblcf_open(const char *device) {
	bdm_print("TBLCF_OPEN: Trying to open device %s\r\n", device);
	return(tblcf_usb_open(device));
}

/* closes currently open device */
void tblcf_close(int dev) {
	bdm_print("TBLCF_CLOSE: Trying to close the device\r\n");
	tblcf_usb_close(dev);
}

/* returns status of the last command: 0 on sucess and non-zero on failure */
unsigned char tblcf_get_last_sts_value(int dev) {
	usb_data[0]=1;	 /* get 1 byte */
	usb_data[1]=CMD_GET_LAST_STATUS;
	tblcf_usb_recv_ep0(dev, usb_data);
	bdm_print("TBLCF_GET_LAST_STATUS: Reported last command status 0x%02X\r\n",usb_data[0]);
  return usb_data[0];
}
/* returns status of the last command: 0 on sucess and non-zero on failure */
unsigned char tblcf_get_last_sts(int dev) {
  unsigned char status = tblcf_get_last_sts_value(dev);
	if ((status==CMD_FAILED)||(status==CMD_UNKNOWN)) return(1); else return(0);
}

/* requests bootloader execution on next power-up */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_request_boot(int dev) {
	usb_data[0]=1;			  	/* return 1 byte */
	usb_data[1]=CMD_SET_BOOT;
	usb_data[2]='B';
	usb_data[3]='O';
	usb_data[4]='O';
	usb_data[5]='T';
	tblcf_usb_recv_ep0(dev, usb_data);
	bdm_print("TBLCF_REQUEST_BOOT: Requested bootloader on next power-up (0x%02X)\r\n",usb_data[0]);
	return(!(usb_data[0]==CMD_SET_BOOT));
}

/* sets target MCU type */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_set_target_type(int dev, target_type_e target_type) {
	usb_data[0]=1;	 /* get 1 byte */
	usb_data[1]=CMD_SET_TARGET;
	usb_data[2]=target_type;
	tblcf_usb_recv_ep0(dev, usb_data);
	bdm_print("TBLCF_SET_TARGET_TYPE: Set target type 0x%02X (0x%02X)\r\n",usb_data[2],usb_data[0]);
	return(!(usb_data[0]==CMD_SET_TARGET));
}

/* resets the target to normal or BDM mode */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_target_reset(int dev, target_mode_e target_mode) {
	usb_data[0]=1;	 /* get 1 byte */
	usb_data[1]=CMD_RESET;
	usb_data[2]=target_mode;
	tblcf_usb_recv_ep0(dev, usb_data);
	bdm_print("TBLCF_TARGET_RESET: Target reset into mode 0x%02X (0x%02X)\r\n",usb_data[2],usb_data[0]);
	return(!(usb_data[0]==CMD_RESET));
}

/* fills user supplied structure with current state of the BDM communication channel */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_bdm_sts(int dev, bdmcf_status_t *bdmcf_status) {
	usb_data[0]=3;	 /* get 3 bytes */
	usb_data[1]=CMD_GET_STATUS;
	tblcf_usb_recv_ep0(dev, usb_data);
	if (usb_data[0]!=CMD_GET_STATUS) return(1);
	bdmcf_status->reset_state = ((usb_data[1]*256+usb_data[2])&RSTO_STATE_MASK)?RSTO_INACTIVE:RSTO_ACTIVE;
	bdmcf_status->reset_detection =
    ((usb_data[1]*256+usb_data[2])&RESET_DETECTED_MASK)?RESET_DETECTED:RESET_NOT_DETECTED;
	bdm_print("TBLCF_BDM_STATUS: Reported communication status 0x%04X (0x%02X)\r\n",
              (usb_data[1]*256+usb_data[2]),usb_data[0]);
	return(0);
}

/* brings the target into BDM mode */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_target_halt(int dev) {
	usb_data[0]=1;	 /* get 1 byte */
	usb_data[1]=CMD_HALT;
	tblcf_usb_recv_ep0(dev, usb_data);
	bdm_print("TBLCF_TARGET_HALT: (0x%02X)\r\n",usb_data[0]);
	return(!(usb_data[0]==CMD_HALT));
}

/* starts target execution from current PC address */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_target_go(int dev) {
	usb_data[0]=1;	 /* get 1 byte */
	usb_data[1]=CMD_GO;
	tblcf_usb_recv_ep0(dev, usb_data);
	bdm_print("TBLCF_TARGET_GO: (0x%02X)\r\n",usb_data[0]);
	return(!(usb_data[0]==CMD_GO));
}

/* steps over a single target instruction */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_target_step(int dev) {
	usb_data[0]=1;	 /* get 1 byte */
	usb_data[1]=CMD_STEP;
	tblcf_usb_recv_ep0(dev, usb_data);
	bdm_print("TBLCF_TARGET_STEP: (0x%02X)\r\n",usb_data[0]);
	return(!(usb_data[0]==CMD_STEP));
}

/* resynchronizes communication with the target (in case of noise, etc.) */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_resynchronize(int dev) {
	usb_data[0]=1;	 /* get 1 byte */
	usb_data[1]=CMD_RESYNCHRONIZE;
	tblcf_usb_recv_ep0(dev, usb_data);
	bdm_print("TBLCF_RESYNCHRONIZE: (0x%02X)\r\n",usb_data[0]);
	return(!(usb_data[0]==CMD_RESYNCHRONIZE));
}

/* asserts the TA signal for the specified time (in 10us ticks) */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_assert_ta(int dev, unsigned char duration_10us) {
	usb_data[0]=1;	 /* get 1 byte */
	usb_data[1]=CMD_ASSERT_TA;
	usb_data[2]=duration_10us;
	tblcf_usb_recv_ep0(dev, usb_data);
	bdm_print("TBLCF_ASSERT_TA: duration %d us (0x%02X)\r\n",(int)10*usb_data[2],usb_data[0]);
	return(!(usb_data[0]==CMD_ASSERT_TA));
}

/* reads control register at the specified address and writes its contents into the supplied buffer */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_read_creg(int dev, unsigned int address, unsigned long int * result) {
	usb_data[0]=5;	 /* get 5 bytes */
	usb_data[1]=CMD_READ_CREG;
	usb_data[2]=(address>>8)&0xff;	/* address in big endian */
	usb_data[3]=address&0xff;
	tblcf_usb_recv_ep0(dev, usb_data);
	*result = (((unsigned long int)usb_data[1])<<24)+(usb_data[2]<<16)+(usb_data[3]<<8)+usb_data[4];	/* result is in big endian */
	bdm_print("TBLCF_READ_CREG: Read control register from address 0x%04X, result: 0x%08lX (0x%02X)\r\n",address,*result,usb_data[0]);
	return(!(usb_data[0]==CMD_READ_CREG));
}

/* writes control register at the specified address */
void tblcf_write_creg(int dev, unsigned int address, unsigned long int value) {
	usb_data[0]=7;	 /* send 7 bytes */
	usb_data[1]=CMD_WRITE_CREG;
	usb_data[2]=(address>>8)&0xff;	/* address in big endian */
	usb_data[3]=address&0xff;
	usb_data[4]=(value>>24)&0xff;
	usb_data[5]=(value>>16)&0xff;
	usb_data[6]=(value>>8)&0xff;
	usb_data[7]=(value)&0xff;
	bdm_print("TBLCF_WRITE_CREG: Write control register at address 0x%04X with 0x%08lX\r\n",address,value);
	tblcf_usb_send_ep0(dev, usb_data);
}

/* reads the specified debug register and writes its contents into the supplied buffer */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_read_dreg(int dev, unsigned char dreg_index, unsigned long int * result) {
	usb_data[0]=5;	 /* get 5 bytes */
	usb_data[1]=CMD_READ_DREG;
	usb_data[2]=dreg_index;
	tblcf_usb_recv_ep0(dev, usb_data);
	*result = (((unsigned long int)usb_data[1])<<24)+(usb_data[2]<<16)+(usb_data[3]<<8)+usb_data[4];	/* result is in big endian */
	bdm_print("TBLCF_READ_DREG: Read debug register #0x%02X, result: 0x%08lX (0x%02X)\r\n",
              dreg_index,*result,usb_data[0]);
	return(!(usb_data[0]==CMD_READ_DREG));
}

/* writes specified debug register */
void tblcf_write_dreg(int dev, unsigned char dreg_index, unsigned long int value) {
	usb_data[0]=6;	 /* send 6 bytes */
	usb_data[1]=CMD_WRITE_DREG;
	usb_data[2]=dreg_index&0xff;
	usb_data[3]=(value>>24)&0xff;
	usb_data[4]=(value>>16)&0xff;
	usb_data[5]=(value>>8)&0xff;
	usb_data[6]=(value)&0xff;
	bdm_print("TBLCF_WRITE_DREG: Write debug register #0x%02X with 0x%08lX\r\n",dreg_index,value);
	tblcf_usb_send_ep0(dev, usb_data);
}

/* reads the specified register and writes its contents into the supplied buffer */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_read_reg(int dev, unsigned char reg_index, unsigned long int * result) {
	usb_data[0]=5;	 /* get 5 bytes */
	usb_data[1]=CMD_READ_REG;
	usb_data[2]=reg_index;
	tblcf_usb_recv_ep0(dev, usb_data);
	*result = (((unsigned long int)usb_data[1])<<24)+(usb_data[2]<<16)+(usb_data[3]<<8)+usb_data[4];	/* result is in big endian */
	bdm_print("TBLCF_READ_REG: Read register #0x%02X, result: 0x%08lX (0x%02X)\r\n",
              reg_index,*result,usb_data[0]);
	return(!(usb_data[0]==CMD_READ_REG));
}

/* writes specified register */
void tblcf_write_reg(int dev, unsigned char reg_index, unsigned long int value) {
	usb_data[0]=6;	 /* send 6 bytes */
	usb_data[1]=CMD_WRITE_REG;
	usb_data[2]=reg_index&0xff;
	usb_data[3]=(value>>24)&0xff;
	usb_data[4]=(value>>16)&0xff;
	usb_data[5]=(value>>8)&0xff;
	usb_data[6]=(value)&0xff;
	bdm_print("TBLCF_WRITE_REG: Write register #0x%02X with 0x%08lX\r\n",reg_index,value);
	tblcf_usb_send_ep0(dev, usb_data);
}

/* reads byte from the specified address */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_read_mem8(int dev, unsigned long int address, unsigned char * result) {
	usb_data[0]=2;	 /* get 2 bytes */
	usb_data[1]=CMD_READ_MEM8;
	usb_data[2]=(address>>24)&0xff;
	usb_data[3]=(address>>16)&0xff;
	usb_data[4]=(address>>8)&0xff;
	usb_data[5]=(address)&0xff;
	tblcf_usb_recv_ep0(dev, usb_data);
	*result = usb_data[1];
	bdm_print("TBLCF_READ_MEM8: Read byte from address 0x%08lX, result: 0x%02X (0x%02X)\r\n",
              address,*result,usb_data[0]);
	return(!(usb_data[0]==CMD_READ_MEM8));
}

/* reads word from the specified address */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_read_mem16(int dev, unsigned long int address, unsigned int * result) {
	usb_data[0]=3;	 /* get 3 bytes */
	usb_data[1]=CMD_READ_MEM16;
	usb_data[2]=(address>>24)&0xff;
	usb_data[3]=(address>>16)&0xff;
	usb_data[4]=(address>>8)&0xff;
	usb_data[5]=(address)&0xff;
	tblcf_usb_recv_ep0(dev, usb_data);
	*result = ((unsigned int)usb_data[1]<<8)+usb_data[2];
	bdm_print("TBLCF_READ_MEM16: Read word from address 0x%08lX, result: 0x%04X (0x%02X)\r\n",
              address,*result,usb_data[0]);
	return(!(usb_data[0]==CMD_READ_MEM16));
}

/* reads long word from the specified address */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_read_mem32(int dev, unsigned long int address, unsigned long int * result) {
	usb_data[0]=5;	 /* get 5 bytes */
	usb_data[1]=CMD_READ_MEM32;
	usb_data[2]=(address>>24)&0xff;
	usb_data[3]=(address>>16)&0xff;
	usb_data[4]=(address>>8)&0xff;
	usb_data[5]=(address)&0xff;
	tblcf_usb_recv_ep0(dev, usb_data);
	*result = (((unsigned long int)usb_data[1])<<24)+(usb_data[2]<<16)+(usb_data[3]<<8)+usb_data[4];
	bdm_print("TBLCF_READ_MEM32: Read long word from address 0x%08lX, result: 0x%08lX (0x%02X)\r\n",address,*result,usb_data[0]);
	return(!(usb_data[0]==CMD_READ_MEM32));
}

/* writes byte at the specified address */
void tblcf_write_mem8(int dev, unsigned long int address, unsigned char value) {
	usb_data[0]=6;	 /* send 6 bytes */
	usb_data[1]=CMD_WRITE_MEM8;
	usb_data[2]=(address>>24)&0xff;
	usb_data[3]=(address>>16)&0xff;
	usb_data[4]=(address>>8)&0xff;
	usb_data[5]=(address)&0xff;
	usb_data[6]=(value)&0xff;
	bdm_print("TBLCF_WRITE_MEM8: Write byte 0x%02X to address 0x%08lX\r\n",value,address);
	tblcf_usb_send_ep0(dev, usb_data);
}

/* writes word at the specified address */
void tblcf_write_mem16(int dev, unsigned long int address, unsigned int value) {
	usb_data[0]=7;	 /* send 7 bytes */
	usb_data[1]=CMD_WRITE_MEM16;
	usb_data[2]=(address>>24)&0xff;
	usb_data[3]=(address>>16)&0xff;
	usb_data[4]=(address>>8)&0xff;
	usb_data[5]=(address)&0xff;
	usb_data[6]=(value>>8)&0xff;
	usb_data[7]=(value)&0xff;
	bdm_print("TBLCF_WRITE_MEM16: Write word 0x%04X to address 0x%08lX\r\n",value,address);
	tblcf_usb_send_ep0(dev, usb_data);
}

/* writes long word at the specified address */
void tblcf_write_mem32(int dev, unsigned long int address, unsigned long int value) {
	usb_data[0]=9;	 /* send 9 bytes */
	usb_data[1]=CMD_WRITE_MEM32;
	usb_data[2]=(address>>24)&0xff;
	usb_data[3]=(address>>16)&0xff;
	usb_data[4]=(address>>8)&0xff;
	usb_data[5]=(address)&0xff;
	usb_data[6]=(value>>24)&0xff;
	usb_data[7]=(value>>16)&0xff;
	usb_data[8]=(value>>8)&0xff;
	usb_data[9]=(value)&0xff;
	bdm_print("TBLCF_WRITE_MEM32: Write long word 0x%08lX to address 0x%08lX\r\n",value,address);
	tblcf_usb_send_ep0(dev, usb_data);
}

/* reads the requested number of bytes from target memory from the supplied address and stores results into the user supplied buffer */
/* uses byte accesses only */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_read_block8(int dev, unsigned long int address,
                                unsigned long int bytecount, unsigned char *buffer) {
	int i;
	bdm_print("TBLCF_READ_BLOCK8: Read 0x%08lX byte(s) from address 0x%08lX:\r\n",bytecount,address);
	while (bytecount>=MAX_DATA_SIZE) {
		usb_data[0]=MAX_DATA_SIZE+1;	 /* receive block of maximum size */
		usb_data[1]=CMD_READ_MEMBLOCK8;
		usb_data[2]=(address>>24)&0xff;
		usb_data[3]=(address>>16)&0xff;
		usb_data[4]=(address>>8)&0xff;
		usb_data[5]=(address)&0xff;
		tblcf_usb_recv_ep0(dev, usb_data);
		bdm_print("TBLCF_READ_BLOCK8: Block read, size 0x%02X (0x%02X):\r\n",MAX_DATA_SIZE,usb_data[0]);
		bdm_print_dump(usb_data+1, MAX_DATA_SIZE);
		if (usb_data[0]!=CMD_READ_MEMBLOCK8) return(1);
		for (i=0;i<MAX_DATA_SIZE;i++) *(buffer++)=usb_data[1+i];	/* copy results */
		bytecount-=MAX_DATA_SIZE;
		address+=MAX_DATA_SIZE;
	}
	if (bytecount) {
		usb_data[0]=bytecount+1;
		usb_data[1]=CMD_READ_MEMBLOCK8;
		usb_data[2]=(address>>24)&0xff;
		usb_data[3]=(address>>16)&0xff;
		usb_data[4]=(address>>8)&0xff;
		usb_data[5]=(address)&0xff;
		tblcf_usb_recv_ep0(dev, usb_data);
		bdm_print("TBLCF_READ_BLOCK8: Block read, size 0x%02X (0x%02X):\r\n",bytecount,usb_data[0]);
		bdm_print_dump(usb_data+1, bytecount);
		if (usb_data[0]!=CMD_READ_MEMBLOCK8) return(1);
		for (i=0;i<bytecount;i++) *(buffer++)=usb_data[1+i];	/* copy results */
	}
	return(0);
}

/* reads the requested number of bytes from target memory from the supplied address and stores results into the user supplied buffer */
/* uses word accesses */
/* if the address is odd a byte read is performed to make it even before performing the word reads */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_read_block16(int dev, unsigned long int address,
                                 unsigned long int bytecount, unsigned char *buffer) {
	int i;
	bdm_print("TBLCF_READ_BLOCK16: Read 0x%08lX byte(s) from address 0x%08lX:\r\n",bytecount,address);
	if (address&0x01) {
		bdm_print("TBLCF_READ_BLOCK16: Address is odd, performing 1 byte read\r\n");
		if (tblcf_read_mem8(dev, address, buffer)) return(1);
		bytecount--;
		address++;
		buffer++;
	}
	while (bytecount>=(MAX_DATA_SIZE&0xfffe)) {
		usb_data[0]=(MAX_DATA_SIZE&0xfffe)+1;	 /* receive block of maximum size */
		usb_data[1]=CMD_READ_MEMBLOCK16;
		usb_data[2]=(address>>24)&0xff;
		usb_data[3]=(address>>16)&0xff;
		usb_data[4]=(address>>8)&0xff;
		usb_data[5]=(address)&0xff;
		tblcf_usb_recv_ep0(dev, usb_data);
		bdm_print("TBLCF_READ_BLOCK16: Block read, size 0x%02X (0x%02X):\r\n",MAX_DATA_SIZE&0xfffe,usb_data[0]);
		bdm_print_dump(usb_data+1, MAX_DATA_SIZE&0xfffe);
		for (i=0;i<(MAX_DATA_SIZE&0xfffe);i++) *(buffer++)=usb_data[1+i];	/* copy results */
		bytecount-=(MAX_DATA_SIZE&0xfffe);
		address+=(MAX_DATA_SIZE&0xfffe);
		if (usb_data[0]!=CMD_READ_MEMBLOCK16) return(1);

	}
	if (bytecount) {
		usb_data[0]=bytecount+1;
		if ((bytecount&0x01)==1) usb_data[0]++;		/* increase the number of bytes to return to a whole number of words */
		usb_data[1]=CMD_READ_MEMBLOCK16;
		usb_data[2]=(address>>24)&0xff;
		usb_data[3]=(address>>16)&0xff;
		usb_data[4]=(address>>8)&0xff;
		usb_data[5]=(address)&0xff;
		tblcf_usb_recv_ep0(dev, usb_data);
		bdm_print("TBLCF_READ_BLOCK16: Block read, size 0x%02X (0x%02X):\r\n",bytecount,usb_data[0]);
		bdm_print_dump(usb_data+1, bytecount);
		if (usb_data[0]!=CMD_READ_MEMBLOCK16) return(1);
		for (i=0;i<bytecount;i++) *(buffer++)=usb_data[1+i];	/* copy results */
	}
	return(0);
}

/* reads the requested number of bytes from target memory from the supplied address and stores results into the user supplied buffer */
/* uses long word accesses */
/* if the address is not aligned byte/word reads are performed to align it */
/* returns 0 on success and non-zero on failure */
unsigned char tblcf_read_block32(int dev, unsigned long int address,
                                 unsigned long int bytecount, unsigned char *buffer) {
	int i;
	bdm_print("TBLCF_READ_BLOCK32: Read 0x%08lX byte(s) from address 0x%08lX:\r\n",bytecount,address);
	if (address&0x01) {
		bdm_print("TBLCF_READ_BLOCK32: Address is odd, performing 1 byte read\r\n");
		if (tblcf_read_mem8(dev, address, buffer)) return(1);
		bytecount--;
		address++;
		buffer++;
	}
	if (address&0x02) {
		bdm_print("TBLCF_READ_BLOCK32: Address is not aligned, performing 1 word read\r\n");
		if (tblcf_read_mem16(dev, address, (unsigned int*) buffer)) return(1);
		bytecount-=2;
		address+=2;
		buffer+=2;
	}
	while (bytecount>=(MAX_DATA_SIZE&0xfffc)) {
		usb_data[0]=(MAX_DATA_SIZE&0xfffc)+1;	 /* receive block of maximum size */
		usb_data[1]=CMD_READ_MEMBLOCK32;
		usb_data[2]=(address>>24)&0xff;
		usb_data[3]=(address>>16)&0xff;
		usb_data[4]=(address>>8)&0xff;
		usb_data[5]=(address)&0xff;
		tblcf_usb_recv_ep0(dev, usb_data);
		bdm_print("TBLCF_READ_BLOCK32: Block read, size 0x%02X (0x%02X):\r\n",MAX_DATA_SIZE&0xfffc,usb_data[0]);
		bdm_print_dump(usb_data+1, MAX_DATA_SIZE&0xfffc);
		for (i=0;i<(MAX_DATA_SIZE&0xfffc);i++) *(buffer++)=usb_data[1+i];	/* copy results */
		bytecount-=(MAX_DATA_SIZE&0xfffc);
		address+=(MAX_DATA_SIZE&0xfffc);
		if (usb_data[0]!=CMD_READ_MEMBLOCK32) return(1);

	}
	if (bytecount) {
		usb_data[0]=bytecount+1;
		usb_data[0]+=((4-(bytecount&0x03))&0x03);	/* increase the number of bytes to make it a multiple of 4 */
		usb_data[1]=CMD_READ_MEMBLOCK32;
		usb_data[2]=(address>>24)&0xff;
		usb_data[3]=(address>>16)&0xff;
		usb_data[4]=(address>>8)&0xff;
		usb_data[5]=(address)&0xff;
		tblcf_usb_recv_ep0(dev, usb_data);
		bdm_print("TBLCF_READ_BLOCK32: Block read, size 0x%02X (0x%02X):\r\n",bytecount,usb_data[0]);
		bdm_print_dump(usb_data+1, bytecount);
		if (usb_data[0]!=CMD_READ_MEMBLOCK32) return(1);
		for (i=0;i<bytecount;i++) *(buffer++)=usb_data[1+i];	/* copy results */
	}
	return(0);
}

/* writes the requested number of bytes to target memory from the supplied address */
/* uses byte accesses only */
/* returns 0 on success and non-zero on failure (must be compiled with WRITE_BLOCK_CHECK, otherwise always returns 0) */
unsigned char tblcf_write_block8(int dev, unsigned long int address,
                                 unsigned long int bytecount, unsigned char *buffer) {
	int i;
	bdm_print("TBLCF_WRITE_BLOCK8: Write 0x%08lX byte(s) to address 0x%08lX:\r\n",bytecount,address);
	while (bytecount>=MAX_DATA_SIZE) {
		usb_data[0]=MAX_DATA_SIZE+5;	 /* write block of maximum size */
		usb_data[1]=CMD_WRITE_MEMBLOCK8;
		usb_data[2]=(address>>24)&0xff;
		usb_data[3]=(address>>16)&0xff;
		usb_data[4]=(address>>8)&0xff;
		usb_data[5]=(address)&0xff;
		for (i=0;i<MAX_DATA_SIZE;i++) usb_data[6+i]=*(buffer++);	/* copy data */
		bdm_print("TBLCF_WRITE_BLOCK8: Block write, size 0x%02X:\r\n",MAX_DATA_SIZE);
		bdm_print_dump(usb_data+6, MAX_DATA_SIZE);
		tblcf_usb_send_ep0(dev, usb_data);
		#ifdef WRITE_BLOCK_CHECK
			if (tblcf_get_last_sts(dev)) return(1);
		#endif
		bytecount-=MAX_DATA_SIZE;
		address+=MAX_DATA_SIZE;
	}
	if (bytecount) {
		usb_data[0]=bytecount+5;
		usb_data[1]=CMD_WRITE_MEMBLOCK8;
		usb_data[2]=(address>>24)&0xff;
		usb_data[3]=(address>>16)&0xff;
		usb_data[4]=(address>>8)&0xff;
		usb_data[5]=(address)&0xff;
		for (i=0;i<bytecount;i++) usb_data[6+i]=*(buffer++);	/* copy data */
		bdm_print("TBLCF_WRITE_BLOCK8: Block write, size 0x%02X:\r\n",bytecount);
		bdm_print_dump(usb_data+6, bytecount);
		tblcf_usb_send_ep0(dev, usb_data);
		#ifdef WRITE_BLOCK_CHECK
			if (tblcf_get_last_sts(dev)) return(1);
		#endif
	}
	return(0);
}

/* writes the requested number of bytes to target memory at the supplied address */
/* uses word accesses */
/* if the address is odd a byte write is performed to make it even before performing the word writes */
/* if there is an odd byte at the end of the block it is written using a byte write */
/* returns 0 on success and non-zero on failure (must be compiled with WRITE_BLOCK_CHECK, otherwise always returns 0) */
unsigned char tblcf_write_block16(int dev, unsigned long int address,
                                  unsigned long int bytecount, unsigned char *buffer) {
	int i;
	bdm_print("TBLCF_WRITE_BLOCK16: Write 0x%08lX byte(s) from address 0x%08lX:\r\n",bytecount,address);
	if (address&0x01) {
		bdm_print("TBLCF_WRITE_BLOCK16: Address is odd, performing 1 byte write\r\n");
		tblcf_write_mem8(dev, address, *buffer);
		#ifdef WRITE_BLOCK_CHECK
			if (tblcf_get_last_sts(dev)) return(1);
		#endif
		bytecount--;
		address++;
		buffer++;
	}
	while (bytecount>=(MAX_DATA_SIZE&0xfffe)) {
		usb_data[0]=(MAX_DATA_SIZE&0xfffe)+5;	 /* send block of maximum size */
		usb_data[1]=CMD_WRITE_MEMBLOCK16;
		usb_data[2]=(address>>24)&0xff;
		usb_data[3]=(address>>16)&0xff;
		usb_data[4]=(address>>8)&0xff;
		usb_data[5]=(address)&0xff;
		for (i=0;i<(MAX_DATA_SIZE&0xfffe);i++) usb_data[6+i]=*(buffer++);	/* copy data */
		bdm_print("TBLCF_WRITE_BLOCK16: Block write, size 0x%02X:\r\n",MAX_DATA_SIZE&0xfffe);
		bdm_print_dump(usb_data+6, MAX_DATA_SIZE&0xfffe);
		tblcf_usb_send_ep0(dev, usb_data);
		bytecount-=(MAX_DATA_SIZE&0xfffe);
		address+=(MAX_DATA_SIZE&0xfffe);
		#ifdef WRITE_BLOCK_CHECK
			if (tblcf_get_last_sts(dev)) return(1);
		#endif
	}
	if (bytecount>=2) {
		usb_data[0]=bytecount+5;
		if ((bytecount&0x01)==1) usb_data[0]--;		/* decrease the number of bytes to write a whole number of words */
		usb_data[1]=CMD_WRITE_MEMBLOCK16;
		usb_data[2]=(address>>24)&0xff;
		usb_data[3]=(address>>16)&0xff;
		usb_data[4]=(address>>8)&0xff;
		usb_data[5]=(address)&0xff;
		for (i=0;i<(bytecount&0xfffe);i++) usb_data[6+i]=*(buffer++);	/* copy data */
		bdm_print("TBLCF_WRITE_BLOCK16: Block write, size 0x%02X:\r\n",(bytecount&0xfffe));
		bdm_print_dump(usb_data+6, (bytecount&0xfffe));
		tblcf_usb_send_ep0(dev, usb_data);
		#ifdef WRITE_BLOCK_CHECK
			if (tblcf_get_last_sts(dev)) return(1);
		#endif
		address+=(bytecount&0xfffe);
		bytecount&=0x01;
	}
	if (bytecount) {	/* leftover byte */
		bdm_print("TBLCF_WRITE_BLOCK16: Misaligned byte at the end of the block, performing 1 byte write\r\n");
		tblcf_write_mem8(dev, address, *buffer);
		#ifdef WRITE_BLOCK_CHECK
			if (tblcf_get_last_sts(dev)) return(1);
		#endif
	}
	return(0);
}

/* writes the requested number of bytes to target memory at the supplied address */
/* uses long word accesses */
/* if the address is not aligned byte/word writes are performed */
/* if there are odd bytes at the end of the block they is written using byte/word writes */
/* returns 0 on success and non-zero on failure (must be compiled with WRITE_BLOCK_CHECK, otherwise always returns 0) */
unsigned char tblcf_write_block32(int dev, unsigned long int address,
                                  unsigned long int bytecount, unsigned char *buffer) {
	int i;
	bdm_print("TBLCF_WRITE_BLOCK32: Write 0x%08lX byte(s) at address 0x%08lX:\r\n",bytecount,address);
	if (address&0x01) {
		bdm_print("TBLCF_WRITE_BLOCK32: Address is odd, performing 1 byte write\r\n");
		tblcf_write_mem8(dev, address, *buffer);
		#ifdef WRITE_BLOCK_CHECK
			if (tblcf_get_last_sts(dev)) return(1);
		#endif
		bytecount--;
		address++;
		buffer++;
	}
	if (address&0x02) {
		bdm_print("TBLCF_WRITE_BLOCK32: Address is not aligned, performing 1 word write\r\n");
		tblcf_write_mem16(dev, address, (*(buffer+0))*256+*(buffer+1));
		#ifdef WRITE_BLOCK_CHECK
			if (tblcf_get_last_sts(dev)) return(1);
		#endif
		bytecount-=2;
		address+=2;
		buffer+=2;
	}
	while (bytecount>=(MAX_DATA_SIZE&0xfffc)) {
		usb_data[0]=(MAX_DATA_SIZE&0xfffc)+5;	 /* send block of maximum size */
		usb_data[1]=CMD_WRITE_MEMBLOCK32;
		usb_data[2]=(address>>24)&0xff;
		usb_data[3]=(address>>16)&0xff;
		usb_data[4]=(address>>8)&0xff;
		usb_data[5]=(address)&0xff;
		for (i=0;i<(MAX_DATA_SIZE&0xfffc);i++) usb_data[6+i]=*(buffer++);	/* copy data */
		bdm_print("TBLCF_WRITE_BLOCK32: Block write, size 0x%02X:\r\n",MAX_DATA_SIZE&0xfffc);
		bdm_print_dump(usb_data+6, MAX_DATA_SIZE&0xfffc);
		tblcf_usb_send_ep0(dev, usb_data);
		bytecount-=(MAX_DATA_SIZE&0xfffc);
		address+=(MAX_DATA_SIZE&0xfffc);
		#ifdef WRITE_BLOCK_CHECK
			if (tblcf_get_last_sts(dev)) return(1);
		#endif
	}
	if (bytecount>=4) {
		usb_data[0]=bytecount+5;
		usb_data[0]-=(bytecount&0x03);			/* decrease the number of bytes to write a whole number of long words */
		usb_data[1]=CMD_WRITE_MEMBLOCK32;
		usb_data[2]=(address>>24)&0xff;
		usb_data[3]=(address>>16)&0xff;
		usb_data[4]=(address>>8)&0xff;
		usb_data[5]=(address)&0xff;
		for (i=0;i<(bytecount&0xfffc);i++) usb_data[6+i]=*(buffer++);	/* copy data */
		bdm_print("TBLCF_WRITE_BLOCK32: Block write, size 0x%02X:\r\n",(bytecount&0xfffc));
		bdm_print_dump(usb_data+6, (bytecount&0xfffc));
		tblcf_usb_send_ep0(dev, usb_data);
		#ifdef WRITE_BLOCK_CHECK
			if (tblcf_get_last_sts(dev)) return(1);
		#endif
		address+=(bytecount&0xfffc);
		bytecount&=0x03;
	}
	if (bytecount>=2) {						/* leftover word */
		bdm_print("TBLCF_WRITE_BLOCK32: Misaligned word at the end of the block, performing 1 word write\r\n");
		tblcf_write_mem16(dev, address, (*(buffer+0))*256+*(buffer+1));
		#ifdef WRITE_BLOCK_CHECK
			if (tblcf_get_last_sts(dev)) return(1);
		#endif
		address+=2;
		bytecount-=2;
		buffer+=2;
	}
	if (bytecount) {						/* leftover byte */
		bdm_print("TBLCF_WRITE_BLOCK32: Misaligned byte at the end of the block, performing 1 byte write\r\n");
		tblcf_write_mem8(dev, address, *(buffer));
		#ifdef WRITE_BLOCK_CHECK
			if (tblcf_get_last_sts(dev)) return(1);
		#endif
	}
	return(0);
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
