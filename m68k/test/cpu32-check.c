/* $Id: cpu32-check.c,v 1.6 2007/11/03 05:31:22 cjohns Exp $
0        1         2         3         4         5         6         7
123456789012345678901234567890123456789012345678901234567890123456789012345678
 * 
 * [cpu32_check.c]
 *
 * A basic checkout utility for a CPU32+ target using the gdb-bdm
 * BDM driver.  Based on the 'chk' utility that is included with
 * gdb-bdm.
 *
 * 12/29/00 KJO (vac4050@cae597.rsc.raytheon.com)
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
 *
 */

/*
 * This notice applies to test_data_bus(), test_address_bus() and
 * test_memory():
 *
 *  Copyright (c) 1998 by Michael Barr.  This software is placed into
 *  the public domain and may be used for any purpose.  However, this
 *  notice must not be changed or removed and no warranty is either
 *  expressed or implied by its publication or distribution.
 */
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <BDMlib.h>
#include <string.h>

#if defined (__WIN32__)
#define sleep(_s) Sleep((_s) * 1000000)
#endif

static int stop_on_error = 1;
static int stop_quiet = 0;
static int options_verbose = 0;

const unsigned long test_pattern[10 * 4] = {
  0x00000000, 0xffffffff, 0xaaaaaaaa, 0x55555555,
  0x12345678, 0x87654321, 0x12344321, 0x43211234,
  0x00000001, 0x00000002, 0x00000004, 0x00000008,
  0x00000010, 0x00000020, 0x00000040, 0x00000080,
  0x00000100, 0x00000200, 0x00000400, 0x00000800,
  0x00001000, 0x00002000, 0x00004000, 0x00008000,
  0x00010000, 0x00020000, 0x00040000, 0x00080000,
  0x00100000, 0x00200000, 0x00400000, 0x00800000,
  0x01000000, 0x02000000, 0x04000000, 0x08000000,
  0x10000000, 0x20000000, 0x40000000, 0x80000000 
};

/*
 * Some 68360 internal registers.
 */
#define DPRAM_BASE				0x0e000000L
#define REG_BASE					(DPRAM_BASE + 0x1000)
#define __MBAR						(DPRAM_BASE | 0x0101)
#define MCR_ADDR					REG_BASE + 0x0000
#define CLKOCR_ADDR				REG_BASE + 0x000c
#define PLLCR_ADDR				REG_BASE + 0x0010
#define CDVCR_ADDR				REG_BASE + 0x0014
#define PEPAR_ADDR				REG_BASE + 0x0016
#define SYPCR_ADDR				REG_BASE + 0x0022
#define GMR_ADDR					REG_BASE + 0x0040
#define MSTAT_ADDR				REG_BASE + 0x0044
#define BR0_ADDR					REG_BASE + 0x0050
#define OR0_ADDR					REG_BASE + 0x0054
#define BR1_ADDR					REG_BASE + 0x0060
#define OR1_ADDR					REG_BASE + 0x0064
#define BR2_ADDR					REG_BASE + 0x0070
#define OR2_ADDR					REG_BASE + 0x0074
#define BR3_ADDR					REG_BASE + 0x0080
#define OR3_ADDR					REG_BASE + 0x0084
#define BR4_ADDR					REG_BASE + 0x0090
#define OR4_ADDR					REG_BASE + 0x0094
#define BR5_ADDR					REG_BASE + 0x00a0
#define OR5_ADDR					REG_BASE + 0x00a4
#define BR6_ADDR					REG_BASE + 0x00b0
#define OR6_ADDR					REG_BASE + 0x00b4
#define BR7_ADDR					REG_BASE + 0x00c0
#define OR7_ADDR					REG_BASE + 0x00c4
#define ICCR_ADDR					REG_BASE + 0x0500

#define INTERNAL_RAM				1
#define EXTERNAL_RAM				0
#define MAX_CODE_LEN				512

#define DPRAM_SIZE				0x100

/*
 * Target-specific external memory parameters
 */
#define EXTERNAL_MEM_BASE		0x0
#define EXTERNAL_MEM_SIZE		0x04000000

void
clean_exit(int exit_code)
{
  if (bdmIsOpen()) {
    bdmSetDriverDebugFlag(0);
    bdmClose();
  }
  exit(exit_code);
}
  
void
show_error(char *msg)
{
  if (!stop_quiet)
    printf("%s failed: %s\n", msg, bdmErrorString());
  if (stop_on_error)
    clean_exit(1);
}

void
show_long(unsigned long address)
{
  unsigned long l;

  printf("%.8lX: ", address);
  if (bdmReadLongWord(address, &l) < 0)
    show_error("Read long");
  printf("%.8lX\n", l);
}

void
show_registers(void)
{
	unsigned long a, d, r;
	int i;

	printf("\nRegisters:\n");
	printf("A0-7:");
	for (i = 0 ; i < 8 ; i++) {
		if (bdmReadRegister(i + 8, &a) < 0)
			show_error("Read address register");
		printf("%.8lX ", a);
	}

	printf("\nD0-7:");
	for (i = 0 ; i < 8 ; i++) {
		if (bdmReadRegister(i, &d) < 0)
			show_error("Read address register");
		printf("%.8lX ", d);
	}

	printf("\n");

	if (bdmReadSystemRegister(BDM_REG_RPC, &r) < 0)
		show_error("Read RPC system register");
	printf(" RPC:%.8lX", r);

	if (bdmReadSystemRegister(BDM_REG_PCC, &r) < 0)
		show_error("Read PCC system register");
	printf("  PCC:%.8lX", r);

	if (bdmReadSystemRegister(BDM_REG_SR, &r) < 0)
		show_error("Read SR system register");
	printf("  SR :%.8lX", r);

	if (bdmReadSystemRegister(BDM_REG_USP, &r) < 0)
		show_error("Read USP system register");
	printf("  USP:%.8lX\n", r);

	if (bdmReadSystemRegister(BDM_REG_SSP, &r) < 0)
		show_error("Read SSP system register");
	printf(" SSP:%.8lX", r);

	if (bdmReadSystemRegister(BDM_REG_SFC, &r) < 0)
		show_error("Read SFC system register");
	printf("  SFC:%.8lX", r);

	if (bdmReadSystemRegister(BDM_REG_DFC, &r) < 0)
		show_error("Read DFC system register");
	printf("  DFC:%.8lX", r);

	if (bdmReadSystemRegister(BDM_REG_FAR, &r) < 0)
		show_error("Read FAR system register");
	printf("  FAR:%.8lX\n", r);

	if (bdmReadSystemRegister(BDM_REG_VBR, &r) < 0)
		show_error("Read VBR system register");
	printf(" VBR:%.8lX", r);

	if (bdmReadSystemRegister(BDM_REG_ATEMP, &r) < 0)
		show_error("Read ATEMP system register");
	printf("  ATEMP:%.8lX", r);

	if (bdmReadSystemRegister(BDM_REG_MBAR, &r) < 0)
		show_error("Read MBAR system register");
	printf("  MBAR:%.8lX\n\n", r);
}

void
do_reset(void)
{
  if (bdmReset() < 0)
    show_error("Reset");
}

void
execute(int memory_type, int loops)
{
	void initialize_system(void);

	unsigned char wbuf[MAX_CODE_LEN];
	unsigned char rbuf[MAX_CODE_LEN];
	unsigned long pc, csr;
	unsigned long address;
	int status;
	int code_len;
	int i, b, loop = 0;
	int verify_error = 0;
  
	const char *code =
	"46FC 2700"
	"2E7C 0E00 0000"
	"207C A0A0 A0A0"
	"227C A1A1 A1A1"
	"247C A2A2 A2A2"
	"267C A3A3 A3A3"
	"287C A4A4 A4A4"
	"2A7C A5A5 A5A5"
	"2C7C A6A6 A6A6"
	"2E7C A7A7 A7A7"
	"203C D0D0 D0D0"
	"223C D1D1 D1D1"
	"243C D2D2 D2D2"
	"263C D3D3 D3D3"
	"283C D4D4 D4D4"
	"2A3C D5D5 D5D5"
	"2C3C D6D6 D6D6"
	"2E3C D7D7 D7D7"
	"4E71"
	"6000 FFFC";

  printf("\nCPU32 %s code execution test: iteration %d of %d\n",
			memory_type == INTERNAL_RAM ? "internal": "external",
			loop, loops);
  
  /*
   * Convert the code to binary to load. The code is
   * like this as it is easy to cut and paste in.
   */
  code_len = strlen(code);

  if (code_len >= MAX_CODE_LEN) {
    printf("Too much code for the buffer.\n");
    clean_exit(1);
  }

  b = 0;
  
  for (i = 0; i < code_len; i++) {
    if (code[i] > ' ') {
      if (code[i] >= '0' && code[i] <= '9')
        wbuf[b] = code[i] - '0';
      else if (code[i] >= 'a' && code[i] <= 'f')
        wbuf[b] = code[i] - 'a' + 10;
      
      i++;
      
      if (code[i] >= '0' && code[i] <= '9')
        wbuf[b] = (wbuf[b] << 4) | (code[i] - '0');
      else if (code[i] >= 'a' && code[i] <= 'f')
        wbuf[b] = (wbuf[b] << 4) | (code[i] - 'a' + 10);

      b++;
    }
  }
  
	initialize_system();

	if (memory_type == INTERNAL_RAM) {
		address = DPRAM_BASE;
		printf("Loading code into DPRAM, base address: 0x%.8lX\n", address);
	}
	else {
		address = 0x0;
		printf("Loading code into external RAM, base address: 0x%.8lX\n",
				  address);
	}

  if (bdmWriteMemory(address, wbuf, b) < 0)
    show_error("Writing buffer to RAM");
    
  if (bdmReadMemory(address, rbuf, b) < 0)
    show_error("Reading buffer from RAM");

	for (i = 0; i < b; i++) {
		if (wbuf[i] != rbuf[i]) {
			printf("Read buffer and write buffer do not agree!\n");
			verify_error = 1;

			if (stop_on_error)
				clean_exit(1);
		}
	}

	if (!verify_error)
		printf("Code readback successful\n");
			
  if (bdmWriteSystemRegister(BDM_REG_RPC, address) < 0)
    show_error("Writing PC");

  printf("Stepping code.\n");
  
  for (i = 0; i < 20; i++) {
    if (bdmReadSystemRegister(BDM_REG_CSR, &csr) < 0)
      show_error("Reading CSR");
    if (bdmReadSystemRegister(BDM_REG_RPC, &pc) < 0)
      show_error("Reading PC");

    printf("Stepping, pc: 0x%08lx", pc);

    if ((pc & 0xfffff000) != address) {
      printf("  WARNING: PC is not in correct address range\n");
    }
	 else {
		printf("\n");
	 }
  
    if (bdmStep() < 0)
      show_error("Step");
  }
  
  show_registers();

	printf("Letting target run...\n");

  if (bdmGo() < 0)
    show_error("Go");

  sleep(1);

  status = bdmStatus();
  printf("Target status: 0x%x -- %s, %s, %s, %s, %s.\n", status,
          status & BDM_TARGETRESET   ? "RESET" : "NOT RESET", 
          status & BDM_TARGETHALT    ? "HALTED" : "NOT HALTED", 
          status & BDM_TARGETSTOPPED ? "STOPPED" : "NOT STOPPED", 
          status & BDM_TARGETPOWER   ? "POWER OFF" : "POWER ON", 
          status & BDM_TARGETNC      ? "NOT CONNECTED" : "CONNECTED");

	printf("Stopping target...\n");

	if (bdmStop() < 0) {
		show_error("Stopping target");
	}
	else {
		status = bdmStatus();
		printf("Target status: 0x%x -- %s, %s, %s, %s, %s.\n", status,
				 status & BDM_TARGETRESET   ? "RESET" : "NOT RESET",
				 status & BDM_TARGETHALT    ? "HALTED" : "NOT HALTED",
				 status & BDM_TARGETSTOPPED ? "STOPPED" : "NOT STOPPED",
				 status & BDM_TARGETPOWER   ? "POWER OFF" : "POWER ON",
				 status & BDM_TARGETNC      ? "NOT CONNECTED" : "CONNECTED");

		show_registers();
	}
}


void
verify_internal_ram(int loops)
{
#define SRAM_BYTE_SIZE (256)
#define SRAM_BUF_SIZE  (SRAM_BYTE_SIZE / sizeof(unsigned long))

	void initialize_system(void);
 
	unsigned long buf[SRAM_BUF_SIZE];
	unsigned int  test;
	unsigned int  i;
	int           sram_ok;
	int           loop = 0;

	printf("Internal DPAM Test, %d loops\n", loops);
	initialize_system();
 
	while (loop < loops) {
		loop++;
    	printf(" %5i : ", loop);
    	for (test = 0; test < sizeof(test_pattern)/sizeof(test_pattern[0]); 
			  test++) {
      	for (i = 0; i < SRAM_BUF_SIZE; i++) {
        		buf[i] = test_pattern[test];
      	}

      sram_ok = 1;
      
      if (bdmWriteMemory(DPRAM_BASE, (unsigned char*) buf, SRAM_BYTE_SIZE) < 0) {
        if (stop_quiet)
          printf("W");
        else
          show_error("Writing SRAM buffer");
      }

      if (bdmReadMemory(DPRAM_BASE, (unsigned char*) buf, SRAM_BYTE_SIZE) < 0) {
        if(stop_quiet)
          printf("R");
        else
          show_error("Reading SRAM buffer");
      }

      for (i = 0; i < SRAM_BUF_SIZE; i++) {
        if (buf[i] != test_pattern[test]) {
          sram_ok = 0;
          if(stop_quiet)
            printf("R");
          else {
            printf("  addr=%08lx,  write/read %08lx %08lx\n",
                    DPRAM_BASE + (i * sizeof(unsigned long)), 
                    test_pattern[test], buf[i]);
            show_error("Verifing SRAM");
          }
        }
      }

      if(sram_ok)
        printf(".");

      fflush(stdout);
    }
    printf("\n");
  }
}

void
check_registers(int loops)
{
  int           reg;
  unsigned long reg_value;
  int           reg_check_loop_failed = 0;
  int           reg_check_failed = 0;
  int           loop = 0;
  unsigned int  i;
  
  while (loop < loops) {
    loop++;
    reg_check_loop_failed = 0;
    printf("Register test, %4d of %4d : \n", loop, loops);
    for (reg = BDM_REG_D0; reg <= BDM_REG_A7; reg++) {
      printf("   %c%02d : ", reg < BDM_REG_A0 ? 'D' : 'A', reg);
      for (i = 0; i < sizeof(test_pattern) / sizeof(test_pattern[0]); i++) {
        if (bdmWriteRegister(reg, test_pattern[i]) < 0) {
          reg_check_failed = reg_check_loop_failed = 1;
          if(stop_quiet)
            printf("W");
          else {
            printf("\n");
            show_error("Write Register");
          }
          continue;
        }
        if (bdmReadRegister(reg, &reg_value) < 0) {
          reg_check_failed = reg_check_loop_failed = 1;
          if(stop_quiet)
            printf("R");
          else {
            printf("\n");
            show_error("Read Register");
          }
          continue;
        }
        if (reg_value != test_pattern[i]) {
          reg_check_failed = reg_check_loop_failed = 1;
          if(stop_quiet)
            printf("X");
          else {
            printf("Write 0x%08lx, read 0x%08lx\n",test_pattern[i],reg_value);
            show_error("Verify register");
          }
        }
        else
          printf(".");
      }
      printf("\n");
    }    
  }

  if (reg_check_failed) {
    show_error("Register Check");
  }
}

#define ALIGN_MEM_SIZE (256)

int
verify_alignment(void)
{
  unsigned long  addr;
  unsigned char  byte;
  unsigned short word;
  unsigned long  lword;
  
  /*
   * Check the values using a byte read.
   */
  printf("Reading bytes :");
  for (addr = 0; addr < ALIGN_MEM_SIZE; addr += sizeof(byte)) {
    if (bdmReadByte(addr + DPRAM_BASE, &byte) < 0) {
      if(stop_quiet)
        printf("W");
      else {
        printf("\n");
        printf("Read byte at 0x%08lx\n", addr + DPRAM_BASE);
        show_error("Reading with bytes");
      }
      return -1;
    } else if (byte != addr) {
      if(stop_quiet)
        printf("b");
      else {
        printf("\n");
        printf("Read byte match addr=0x%08lx, read=0x%02x, wanted=0x%02x\n",
                addr + DPRAM_BASE, byte, (unsigned char)addr);
        show_error("Bytes read does not match");
      }
      return -1;
    }
    else {
      if ((addr % 32) == 0)
        printf("\n");
      printf(".");
    }
  }

  /*
   * Check the values using a word read.
   */
  printf("\n Reading words :");
  for (addr = 0; addr < ALIGN_MEM_SIZE; addr += sizeof(word) ) {
    if (bdmReadWord(addr + DPRAM_BASE, &word) < 0) {
      if(stop_quiet)
        printf("w");
      else {
        printf("\n");
        printf("Read words at 0x%08lx\n", addr + DPRAM_BASE);
        show_error("Reading with words");
      }
      return -1;
    } else if (word != ((addr << 8) | (addr + 1))) {
      if(stop_quiet)
        printf("l");
      else {
        printf("\n");
        printf("Read word match addr=0x%08lx, read=0x%04x, wanted=0x%04x\n",
                addr + DPRAM_BASE, word,
					(unsigned short)((addr << 8) | (addr + 1)));
        show_error("Words do not match");
      }
      return -1;
    }
    else {
      if ((addr % 32) == 0)
        printf("\n");
      printf(".");
    }
  }

  /*
   * Check the values using a long read.
   */
  printf("\n Reading long words :");
  for (addr = 0; addr < ALIGN_MEM_SIZE; addr += sizeof(lword) ) {
    if (bdmReadLongWord(addr + DPRAM_BASE, &lword) < 0) {
      if(stop_quiet)
        printf("W");
      else {
        printf("\n");
        printf("Read long words at 0x%08lx\n", addr + DPRAM_BASE);
        show_error("Reading with long words");
      }
      return -1;
    } else if(lword !=
        ((addr << 24) | ((addr + 1) << 16) | ((addr + 2) << 8) | (addr + 3))) {
      if(stop_quiet)
        printf("W");
      else {
        printf("\n");
        printf("Read long word match addr=0x%08lx"
                ", read=0x%08lx, wanted=0x%08lx\n",
                addr + DPRAM_BASE, lword,
        			 ((addr << 24) | ((addr + 1) << 16) | 
					 ((addr + 2) << 8) | (addr + 3)));
        show_error("Long words do not match");
      }
      return -1;
    }
    else {
      if ((addr % 32) == 0)
        printf("\n");
      printf(".");
    }
  }
  return 0;
}


void
check_alignment(int loops)
{
	unsigned long addr;
	int           loop_failed = 0;
	int           failed = 0;
	int           loop = 0;
	unsigned char buf[ALIGN_MEM_SIZE];
  
	void initialize_system(void);

	printf("\nAlignment Test, %d loop%s\n", loops, loops == 1 ? "" : "s");
  
	initialize_system();
 
	for (addr = 0; addr < 256; addr++)
		buf[addr] = addr;
  
	while (loop < loops) {
		loop++;
		loop_failed = 0;

    /*
     * Fill the memory with a byte count.
     */    
    printf("Byte write alignment verify, %4d of %4d : \n", loop, loops);
    for (addr = 0; addr < 256; addr++) {
      if (bdmWriteByte(addr + DPRAM_BASE, addr) < 0) {
          failed = loop_failed = 1;
          if(stop_quiet)
            printf("B");
          else {
            printf("\n");
            printf("Write byte 0x%02x to 0x%08lx\n", 
					(unsigned char)addr, addr + DPRAM_BASE);
            show_error("Write byte pattern");
          }
          continue;
      }
      else {
        if ((addr % 32) == 0)
          printf("\n");
        printf(".");
      }
    }
    printf("\n");
    if (verify_alignment() < 0) {
      failed = loop_failed = 1;
      if(!stop_quiet) {
        printf("\n");
        show_error("Verify byte write");
      }
    }

    /*
     * Fill the memory with a byte count.
     */    
    printf("\nWord write alignment verify, %4d of %4d : \n", 
            loop, loops);
    for (addr = 0; addr < 256; addr += 2) {
      if (bdmWriteWord(addr + DPRAM_BASE, (addr << 8) | (addr + 1)) < 0) {
          failed = loop_failed = 1;
          if(stop_quiet)
            printf("W");
          else {
            printf("\n");
            printf("Write word 0x%04x to 0x%08lx\n",
                    (unsigned short) addr, addr + DPRAM_BASE);
            show_error("Write word pattern");
          }
          continue;
      }
      else {
        if ((addr % 32) == 0)
          printf("\n");
        printf(".");
      }
    }
    printf("\n");
    if (verify_alignment() < 0) {
      failed = loop_failed = 1;
      if(!stop_quiet) {
        printf("\n");
        show_error("Verify word write");
      }
    }

    /*
     * Fill the memory with a byte count.
     */    
    printf("\nLong word write alignment verify, %4d of %4d : \n",
            loop, loops);
    for (addr = 0; addr < 256; addr += 4) {
      if (bdmWriteLongWord(addr + DPRAM_BASE,
                            ((addr << 24) | ((addr + 1) << 16) |
                             ((addr + 2) << 8) | (addr + 3)))) {
          failed = loop_failed = 1;
          if(stop_quiet)
            printf("L");
          else {
            printf("\n");
            printf("Write long word 0x%08lx to 0x%08lx\n", 
                    addr, addr + DPRAM_BASE);
            show_error("Write long word pattern");
          }
          continue;
      }
      else {
        if ((addr % 32) == 0)
          printf("\n");
        printf(".");
      }
    }
    printf("\n");
    if (verify_alignment() < 0) {
      failed = loop_failed = 1;
      if(!stop_quiet) {
        printf("\n");
        show_error("Verify long word write");
      }
    }
    printf("\nBlock write alignment verify, %4d of %4d : \n", loop, loops);
    if (bdmWriteMemory(DPRAM_BASE, (unsigned char*) buf, ALIGN_MEM_SIZE) < 0) {
      if(stop_quiet)
        printf("K\n");
      else
        show_error("Writing block buffer");
    }
    if (verify_alignment() < 0) {
      failed = loop_failed = 1;
      if(!stop_quiet) {
        printf("\n");
        show_error("Verify long word write");
      }
    }
  }

  if (failed) {
    show_error("Alignment check");
  }
  printf("\n");
}


/*
 * Test the data bus wiring in a memory region by performing a walking 1's 
 * test at a fixed address within that region.  The address (and hence 
 * the memory region) is selected by the caller.
 * 
 * Returns:     0 if the test succeeds.  
 *              -1 if the test fails.
 */
int
test_data_bus(unsigned long base_addr)
{
	unsigned long readback;
	unsigned long pattern;
	int error = 0;

	printf("  Test data bus at address 0x%.8lX\n", base_addr);

	for (pattern = 1; pattern != 0; pattern <<= 1) {
		if (bdmWriteLongWord(base_addr, pattern) < 0)
			show_error("  Write pattern in test_data_bus()");
		/*
		 * Immediate read back is okay for this test
		 */
		if (bdmReadLongWord(base_addr, &readback) < 0)
			show_error("    Readback in test_data_bus()");

		if (readback!= pattern) {
			printf("    Readback failed; expected 0x%.8lX readback 0x%.8lX\n",
					 pattern, readback);
			error = -1;

			if (stop_on_error)
				clean_exit(1);
		}
	}
	return (error);
}


/*
 * Test the address bus wiring in a memory region by performing a walking 1's
 * test on the relevant bits of the address and checking for aliasing. This 
 * test will find single-bit address failures such as stuck -high, stuck-low,
 * and shorted pins.  The base address and size of the region are selected by
 * the caller.
 *
 * Notes:  For best results, the selected base address should have enough 
 * LSB 0's to guarantee single address bit changes.  For example, to test a 
 * 64-Kbyte region, select a base address on a 64-Kbyte boundary.  Also, 
 * select the region size as a power-of-two if at all possible.

 *
 * Returns:  0 if the test succeeds.  
 *           A non-zero result is the first address at which an aliasing 
 * problem was uncovered.  By examining the contents of memory, it may be i
 * possible to gather additional information about the problem.
 */
int
test_addr_bus(unsigned long base_addr, unsigned long num_words)
{
	unsigned long addr_mask = (num_words - 1);
	unsigned long offset;
	unsigned long test_offset;
	unsigned char readback;
	int error = 0;

	unsigned char pattern = 0xAA;
	unsigned char anti_pattern = 0x55;

	printf("  Test address bus: 0x%.8lX words at base address 0x%.8lX\n",
			 num_words, base_addr);

	/*
	 * Write the default pattern at each of the power-of-two offsets.
	 */
	for (offset = 1; (offset & addr_mask) != 0; offset <<= 1) {
		if (bdmWriteByte(base_addr + offset, pattern) < 0)
			show_error("    Write default pattern");
	}

	/* 
	 * Check for address bits stuck high.
	 */
	printf("    Test for address bits stuck high\n");

	test_offset = 0;

	if (bdmWriteByte(base_addr + test_offset, anti_pattern) < 0)
		show_error("      anti_pattern write");

	for (offset = 1; (offset & addr_mask) != 0; offset <<= 1) {
		if (bdmReadByte(base_addr + offset, &readback) < 0)
			show_error("    Readback");

		if (readback != pattern) {
			printf("      Readback error at 0x%.8lX;"
					 " expected 0x%.2X read 0x%.2X\n",
					 (base_addr + offset), pattern, readback);
			error = -1;

			if (stop_on_error)
				clean_exit(1);
		}
	}

	if (bdmWriteByte(base_addr + test_offset, pattern) < 0)
		show_error("      Write pattern");

	/*
	 * Check for address bits stuck low or shorted.
	 */
	printf("    Test for address bits stuck low or shorted\n");

	for (test_offset = 1; (test_offset & addr_mask) != 0; test_offset <<= 1) {
		if (bdmWriteByte(base_addr + test_offset, anti_pattern) < 0)
			show_error("      anti_pattern write");
		
		for (offset = 1; (offset & addr_mask) != 0; offset <<= 1) {
			if (bdmReadByte(base_addr + offset, &readback) < 0)
	         show_error("      Readback");

			if ((readback != pattern) && (offset != test_offset)) {
				printf("      Fail at offset 0x%.8lX test_offset 0x%.8lX;"
						 " expected 0x%.2X read 0x%.2X\n",
						  (base_addr + offset), (base_addr + test_offset), 
						  pattern, readback);
				error = -1;

				if (stop_on_error)
					clean_exit(1);
			}
		}

		if (bdmWriteByte(base_addr + test_offset, pattern) < 0)
         show_error("      Pattern write");
	}
	return (error);
}


/*
 * Test the integrity of a physical memory device by performing an 
 * increment/decrement test over the entire region.  In the process every 
 * storage bit in the device is tested as a zero and a one.  The base address
 * and the size of the region are selected by the caller.
 *
 * Returns: 0 if the test succeeds.  Also, in that case, the entire memory 
 * region will be filled with zeros.
 *
 * A non-zero result is the first address at which an incorrect value was 
 * read back.  By examining the contents of memory, it may be possible to 
 * gather additional information about the problem.
 */
int
test_memory(unsigned long base_addr, unsigned long num_words)
{
	unsigned long offset;
	unsigned long pattern;
	unsigned long anti_pattern;
	unsigned long readback;
	int error = 0;

	printf("  Memory test: 0x%.8lX words at base address 0x%.8lX\n",
			 num_words, base_addr);

	/*
	 * Fill memory with a known pattern.
	 */
	printf("    Fill memory with known pattern\n");

	for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
		if (bdmWriteLongWord((base_addr + offset) * 4, pattern) < 0)
			show_error("    Write pattern");
	}

	/*
	 * Check each location and invert it for the second pass.
	 */
	printf("    Check each address and invert data for second pass\n");

	for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
		if (bdmReadLongWord((base_addr + offset) * 4, &readback) < 0)
			show_error("    Readback");

		if (options_verbose)
			printf("Readback 0x%.8lX at address 0x%.8lX\n", 
					 readback, (base_addr + offset) * 4);

		if (readback != pattern) {
			printf("    Readback failed at 0x%.8lX;"
					 " expected 0x%.8lX read 0x%.8lX\n",
					 (base_addr + offset) * 4, pattern, readback);
			error = -1;

			if (stop_on_error)
				clean_exit(1);
		}

		anti_pattern = ~pattern;

		if (bdmWriteLongWord((base_addr + offset) * 4, anti_pattern) < 0)
			show_error("    anti_pattern write");
	}

	/*
	 * Check each location for the inverted pattern and zero it.
	 */
	printf("  Checking each address for anti_pattern and zeroing data\n");

	for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
		  anti_pattern = ~pattern;

		if (bdmReadLongWord((base_addr + offset) * 4, &readback) < 0)
			show_error("    Read of readback");

		if (readback != anti_pattern) {
			printf("    Readback error at 0x%.8lX:"
					 " expected 0x%.8lX read 0x%.8lX\n",
					 (base_addr + offset) * 4, anti_pattern, readback);
			error = -1;

			if (stop_on_error)
				clean_exit(1);
		}

		if (bdmWriteLongWord((base_addr + offset) * 4, 0) < 0)
			show_error("    Write to zero data");
	}
	return (error);
}

void
verify_internal_mem(int loops)
{
	void initialize_system();
	int i;

	initialize_system();

	for (i = 1; i <= loops; i++) {
      printf("\nTesting internal memory: iteration %d of %d\n",
             i, loops);
      test_data_bus(DPRAM_BASE);
      test_addr_bus(DPRAM_BASE, DPRAM_SIZE);
      test_memory(DPRAM_BASE, DPRAM_SIZE);
   }

}

/*
 * Perform a memory test on external main memory.
 */
void
verify_external_mem(int loops, unsigned int addr_bus_size, unsigned int size)
{
	void initialize_system();
	int i;
	int databus_errors = 0;
	int addrbus_errors = 0;
	int memory_errors = 0;

	initialize_system();
	
	for (i = 1; i <= loops; i++) {
		printf("\nTesting external memory: iteration %d of %d\n",
				 i, loops);
		if (test_data_bus(EXTERNAL_MEM_BASE) < 0) 
			databus_errors++;

		if (test_addr_bus(EXTERNAL_MEM_BASE, addr_bus_size) < 0)
			addrbus_errors++;

		if (test_memory(EXTERNAL_MEM_BASE, size) < 0)
			memory_errors++; 
	}
		printf("Data Bus Errors = %d\n", databus_errors);
		printf("Address Bus Errors = %d\n", addrbus_errors);
		printf("Memory Errors = %d\n", memory_errors);
}

/*
 * Perform any target specific initialization required to check
 * main (external to the processor) memory.
 */
void
initialize_system(void)
{
	unsigned long r;
	unsigned short rword;

	printf("Initializing target\n");
	do_reset();

	if (bdmWriteMBAR(__MBAR) < 0)
		show_error("MBAR write");

	if (bdmReadMBAR(&r) < 0)
		show_error("MBAR read");

	if (options_verbose)
		printf("Readback MBAR = 0x%.8lX\n", r);

	if (r != __MBAR)
		show_error("MBAR readback");

	if (bdmWriteSystemRegister(BDM_REG_DFC, 0x5) < 0)
		show_error("Write 0x5 to DFC");

	if (bdmWriteSystemRegister(BDM_REG_SFC, 0x5) < 0)
		show_error("Write 0x5 to SFC");

	if (bdmWriteByte(CLKOCR_ADDR, 0x8f) < 0)
		show_error("Write to CLKOCR");

	if(bdmWriteWord(PLLCR_ADDR, 0xd000) < 0)
		show_error("Write to PLLCR");

	if (bdmWriteWord(CDVCR_ADDR, 0x8000) < 0)
		show_error("Write to CDVCR");

	if (bdmWriteByte(SYPCR_ADDR, 0x7f) < 0)
		show_error("Write to SYPCR");

	if (bdmWriteLongWord(MCR_ADDR, 0x4c7f) < 0)
		show_error("Write to MCR");

	if (bdmWriteWord(PEPAR_ADDR, 0x0080) < 0)
		show_error("Write to PEPAR");

	if (bdmReadWord(PEPAR_ADDR, &rword) < 0)
		show_error("Read of PEPAR");

	if ((rword && 0x20) == 0) /* PEPAR is not written */
		printf("PWW in PEPAR is not set!");

	/*if (bdmWriteLongWord(GMR_ADDR, 0x16e801A0) < 0) */
	if (bdmWriteLongWord(GMR_ADDR, 0x179801A0) < 0)
		show_error("Write to GMR");

	if (bdmWriteLongWord(BR0_ADDR, 0x04000001) < 0)
		show_error("Write to BR0");

	/* if (bdmWriteLongWord(OR0_ADDR, 0xfc000000) < 0) */
	if (bdmWriteLongWord(OR0_ADDR, 0x84000000) < 0)
		show_error("Write to OR0");

	if (bdmWriteLongWord(BR1_ADDR, 0x00000001) < 0)
		show_error("Write to BR1");

	if (bdmWriteLongWord(OR1_ADDR, 0x02000001) < 0)
		show_error("Write to OR1");
}


void
usage()
{
  printf("cpu32-check -d [level] -v -r [loops] -m [loops] -M [loops] -a [loops] -E [loops] -e [loops] -C -Q [device]\n"
   " where :\n"
   "    -d [level]   : Enable driver debug output\n"
   "    -v           : Enable verbose output\n"
   "    -r [loops]   : Number of register check loops\n"
   "    -m [loops]   : Number of internal RAM check loops\n"
   "    -M [loops]   : Number of external RAM check loops\n"
   "    -a [loops]   : Number of alignment check loops\n"
   "    -e [loops]   : Number of internal RAM execute loops\n"
   "    -E [loops]   : Number of external RAM execute loops\n"
   "    -D [delay]   : Delay count for BDM clock generation\n"
   "    -C           : Continue on an error\n"
   "    -Q           : Be quiet on errors\n"
   "    [device]     : The bdm device, eg /dev/cpu32icd0\n");

  exit(0);
}

int
main(int argc, char **argv)
{
  char          *dev = NULL;
  int           arg;
  int           driver_debug = 0;
  int           delay_counter = 0;
  int           options_stop_on_error = 1;
  int           options_stop_quiet = 0;
  unsigned int  driver_version;
  int           cpu_type;
  int           iface;
  int           reg_check_loops = 1;
  int           align_check_loops = 1;
  int           external_mem_check_loops = 1;
  int           internal_mem_check_loops = 1;
  int           external_ram_execute_loops = 1;
  int           internal_ram_execute_loops = 1;

  if (argc <= 1) {
    usage();
  }

  for (arg = 1; arg < argc; arg++) {
    if (argv[arg][0] != '-') {
      if (dev) {
        printf(" Device name specified more than once");
        exit(1);
      }
      dev = argv[arg];
    }
    else {
      switch (argv[arg][1]) {
        case 'd':
          arg++;
          if (arg == argc) {
            printf(" -d option requires a parameter");
            exit(0);
          }
          driver_debug = strtoul(argv[arg], NULL, 0);
          break;

        case 'v':
          options_verbose = 1;
          break;

        case 'D':
          arg++;
          if (arg == argc) {
            printf(" -D option requires a parameter");
            exit(0);
          }
          delay_counter = strtoul(argv[arg], NULL, 0);
          break;

        case 'r':
          arg++;
            if (arg == argc) {
              printf(" -r option requires a parameter");
              exit(0);
            }
          reg_check_loops = strtoul(argv[arg], NULL, 0);
          break;

        case 'm':
          arg++;
            if (arg == argc) {
              printf(" -m option requires a parameter");
              exit(0);
            }
          internal_mem_check_loops = strtoul(argv[arg], NULL, 0);
          break;

        case 'M':
          arg++;
            if (arg == argc) {
              printf(" -M option requires a parameter");
              exit(0);
            }
          external_mem_check_loops = strtoul(argv[arg], NULL, 0);
          break;
          
        case 'e':
          arg++;
            if (arg == argc) {
              printf(" -e option requires a parameter");
              exit(0);
            }
          internal_ram_execute_loops = strtoul(argv[arg], NULL, 0);
          break;
          
        case 'E':
          arg++;
            if (arg == argc) {
              printf(" -E option requires a parameter");
              exit(0);
            }
          external_ram_execute_loops = strtoul(argv[arg], NULL, 0);
          break;
          
        case 'a':
          arg++;
            if (arg == argc) {
              printf(" -a option requires a parameter");
              exit(0);
            }
          align_check_loops = strtoul(argv[arg], NULL, 0);
          break;

        case 'C':
          options_stop_on_error = 0;
          break;
          
        case 'Q':
          options_stop_quiet = 1;
          break;

        default:
          printf(" Unknown option!");
    
        case '?':
        case 'h':
          usage();
          break;
      }
    }
  }

  if (!dev)
  {
    printf(" ERROR: No BDM device set.\n");
    exit(1);
  }

  if (bdmOpen(dev) < 0)
    show_error("Open");

  if (driver_debug)
    bdmSetDriverDebugFlag(driver_debug);

  if (delay_counter)
    bdmSetDelay(delay_counter);

  if (bdmGetDrvVersion(&driver_version) < 0)
    show_error("GetDrvVersion");

  printf("BDM Driver Version : %x.%x\n", 
         driver_version >> 8, driver_version & 0xff);

  if (bdmGetProcessor(&cpu_type) < 0)
    show_error("GetProcessor");

  switch(cpu_type) {
    case BDM_CPU32:
      printf("Processor  : CPU32\n");
      break;
    default:
      printf("Unsupported processor type!\n");
      clean_exit(1);
      break;
  }

  if (bdmGetInterface(&iface) < 0)
    show_error("GetInterface");

  switch(iface) {
    case BDM_CPU32_ERIC:
      printf("Interface  : Eric's CPU32\n");
      break;
    case BDM_CPU32_ICD:
      printf("Interface  : ICD (P&E) CPU32\n");
      break;
    default:
      printf("Unknown or unsupported interface type!\n");
      clean_exit(1);
      break;
  }

	stop_on_error = options_stop_on_error;
	stop_quiet = options_stop_quiet;
  
	if (reg_check_loops)
		check_registers(reg_check_loops);
  
	if (internal_mem_check_loops)
		verify_internal_mem(internal_mem_check_loops);

	/*
	 * Only test the whole memory in the address test; the others take too long.
	 */
	if (external_mem_check_loops)
		verify_external_mem(external_mem_check_loops, EXTERNAL_MEM_SIZE, 0x400);
  
	if (align_check_loops)
		check_alignment(align_check_loops);
 	
	if (internal_ram_execute_loops) 
		execute(INTERNAL_RAM, internal_ram_execute_loops);

	if (external_ram_execute_loops)
		execute(EXTERNAL_RAM, external_ram_execute_loops);
  
	clean_exit(0);
	return 0;
}
