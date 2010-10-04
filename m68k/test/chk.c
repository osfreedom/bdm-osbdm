/*
 * Check for the BDM driver. Tests parts of the driver.
 */

#include <inttypes.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <BDMlib.h>

#if defined (__WIN32__)
#define sleep(_s) Sleep((_s) * 1000000)
#endif

static const char *const sysreg_name[BDM_MAX_SYSREG] = {
  "RPC",  "PCC",    "SR",   "USP",
  "SSP",  "SFC",    "DFC",  "ATEMP",
  "FAR",  "VBR",    "CACR", "ACR0",
  "ACR1", "RAMBAR", "MBAR", "CSR",
  "AATR", "TDR",    "PBR",  "PBMR",
  "ABHR", "ABLR",   "DBR",  "DBMR",
};

static int stop_on_error = 1;
static int stop_quite = 0;
static int force_reset_on_fail = 0;
static int bdm_revision;

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

#define SRAMBAR 0x20000000

void
cleanExit (int exit_code)
{
  if (bdmIsOpen ()) {
    bdmSetDriverDebugFlag (0);
    bdmClose ();
  }
  exit (exit_code);
}
  
void
showError (char *msg)
{
  if (!stop_quite)
    printf ("%s failed: %s\n", msg, bdmErrorString ());
  if (stop_on_error)
    cleanExit (1);
}

void
showLong (unsigned long address)
{
  unsigned long l;

  printf ("%.8lX: ", address);
  if (bdmReadLongWord (address, &l) < 0)
    showError ("Read long");
  printf ("%.8lX\n", l);
}

void
showRegs (int cpu)
{
  unsigned long a, d, r;
  int i, s;

  for (i = 0 ; i < 8 ; i++) {
    if ((bdmReadRegister (i, &d) < 0) || (bdmReadRegister (i+8, &a) < 0))
      showError ("Read register");
    
    printf ("A%d: %.8lX   D%d: %.8lX\n", i, a, i, d);
  }

  for (i = 0 ; i < BDM_MAX_SYSREG; i++) {
    if (cpu == BDM_CPU32) {
      switch (i) {
        default:
          s = i;
          break;
    
        case BDM_REG_CACR:
        case BDM_REG_ACR0:
        case BDM_REG_ACR1:
        case BDM_REG_RAMBAR:
        case BDM_REG_CSR:
        case BDM_REG_AATR:
        case BDM_REG_TDR:
        case BDM_REG_PBR:
        case BDM_REG_PBMR:
        case BDM_REG_ABHR:
        case BDM_REG_ABLR:
        case BDM_REG_DBR:
        case BDM_REG_DBMR:
          s =  BDM_MAX_SYSREG;
          break;
      }
    }
    else {
      switch (i) {
        default:
          s = i;
          break;
    
        case BDM_REG_PCC:
        case BDM_REG_USP:
        case BDM_REG_SSP:
        case BDM_REG_SFC:
        case BDM_REG_DFC:
        case BDM_REG_ATEMP:
        case BDM_REG_FAR:
          s =  BDM_MAX_SYSREG;
          break;
      }
    }
    
    if (s <  BDM_MAX_SYSREG) {
      if (bdmReadSystemRegister (s, &r) < 0)
        showError ("Read system register");
      printf ("   %8s:%.8lX\n", sysreg_name[i], r);
    }
  }
}

void
cpu32Execute (unsigned long chkpc)
{
  unsigned long pc;
  
  printf ("Run to %#8lx\n", chkpc);
  
  if ((bdmWriteSystemRegister (BDM_REG_SFC, 5) < 0) ||
      (bdmWriteSystemRegister (BDM_REG_DFC, 5) < 0))
    showError ("Write SFC/DFC");
  
  for (;;) {
    if (bdmReadSystemRegister (BDM_REG_RPC, &pc) < 0)
      showError ("Read PC");
    if (pc == chkpc)
      break;
    if (bdmStep () < 0)
      showError ("Step");
  }
}

void
do_reset (int cpu)
{
  unsigned long csr;
  const char *cputype;
  
  /*
   * Reset the target
   */
  if (bdmReset () < 0)
    showError ("Reset");

  /*
   * Get the target status
   */
  if (cpu == BDM_COLDFIRE) {
    if (bdmReadSystemRegister (BDM_REG_CSR, &csr) < 0)
      showError ("Reading CSR");
    
    if ((csr & 0x01000000) == 0) {
      printf ("CSR break not set, target failed to break, CSR = 0x%08lx\n", csr);
      cleanExit (1);
    }
    printf ("CSR break set, target stopped.\n");
    bdm_revision = (csr >> 20) & 0x0f;
    switch (bdm_revision) {
      case 0: cputype = "5206(e)"; break;
      case 1: cputype = "5307"; break;
      case 2: cputype = "5407"; break;
      case 3: cputype = "547x/548x"; break;
      default: cputype = "post-548x";
    }
    printf ("Debug module version is %d, (%s)\n", bdm_revision, cputype);
  }
}

struct postTestRegs {
	int reg;
	int value;
};

struct postTestRegs expectedResults[]={
	{BDM_REG_A0,0xa0a0a0a0},
	{BDM_REG_A1,0xa1a1a1a1},
	{BDM_REG_A2,0xa2a2a2a2},
	{BDM_REG_A3,0xa3a3a3a3},
	{BDM_REG_A4,0xa4a4a4a4},
	{BDM_REG_A5,0xa5a5a5a5},
	{BDM_REG_A6,0xa6a6a6a6},
	{BDM_REG_A7,0xa7a7a7a7},
	{BDM_REG_D0,0xd0d0d0d0},
	{BDM_REG_D1,0xd1d1d1d1},
	{BDM_REG_D2,0xd2d2d2d2},
	{BDM_REG_D3,0xd3d3d3d3},
	{BDM_REG_D4,0xd4d4d4d4},
	{BDM_REG_D5,0xd5d5d5d5},
	{BDM_REG_D6,0xd6d6d6d6},
	{BDM_REG_D7,0xd7d7d7d7},
};


void checkRegValues()
{
	int i;
	for (i=0;i<sizeof(expectedResults)/sizeof(struct postTestRegs); ++i)
	{
		unsigned long value=0;
		bdmReadRegister(expectedResults[i].reg,&value);
		if ( expectedResults[i].value != value )
			printf("Failed setting register %c%i: %" PRIx32 " != %" PRIxMAX "\n", (i<7?'A':'D'),
             i%8, expectedResults[i].value, value);
	}
}

void
coldfireExecute ()
{
  unsigned char wbuf[512];
  unsigned char rbuf[512];
  unsigned long csr, pc;
  int status;
  int code_len;
  int i, b;
  
  const char *code =
    "46fc 2700"		// "movew #0x2700,%sr\n"	| enter supervisor mode
    "2e7c 2000 0000"	// "movel #0x20000000,%sp\n"	| setup the stack
    "207c a0a0 a0a0"	// "movel #0xa0a0a0a0,%a0\n"
    "227c a1a1 a1a1"	// "movel #0xa1a1a1a1,%a1\n"
    "247c a2a2 a2a2"	// "movel #0xa2a2a2a2,%a2\n"
    "267c a3a3 a3a3"	// "movel #0xa3a3a3a3,%a3\n"
    "287c a4a4 a4a4"	// "movel #0xa4a4a4a4,%a4\n"
    "2a7c a5a5 a5a5"	// "movel #0xa5a5a5a5,%a5\n"
    "2c7c a6a6 a6a6"	// "movel #0xa6a6a6a6,%a6\n"
    "2e7c a7a7 a7a7"	// "movel #0xa7a7a7a7,%a7\n"
    "203c d0d0 d0d0"	// "movel #0xd0d0d0d0,%d0\n"
    "223c d1d1 d1d1"	// "movel #0xd1d1d1d1,%d1\n"
    "243c d2d2 d2d2"	// "movel #0xd2d2d2d2,%d2\n"
    "263c d3d3 d3d3"	// "movel #0xd3d3d3d3,%d3\n"
    "283c d4d4 d4d4"	// "movel #0xd4d4d4d4,%d4\n"
    "2a3c d5d5 d5d5"	// "movel #0xd5d5d5d5,%d5\n"
    "2c3c d6d6 d6d6"	// "movel #0xd6d6d6d6,%d6\n"
    "2e3c d7d7 d7d7"	// "movel #0xd7d7d7d7,%d7\n"
    "4e71"		// "nop\n"
    "4e71"		// "nop\n"
    "4e71"		// "nop\n"
    "4e71"		// "nop\n"
    "4e71"		// "nop\n"
    "4e71"		// "nop\n"
    "4e71"		// "nop\n"
    "4e71"		// "nop\n"
    "4e71"		// "nop\n"
    "4e71"		// "nop\n"
    "4e71"		// "nop\n"
    "4e71"		// "nop\n"
    "4e71"		// "nop\n"
    "4e71"		// "nop\n"
    "4ac8"		// "halt\n"
    ;

  printf ("Coldfire execution test, loading code to SRAM.\n");
  
  /*
   * Convert the code to binary to load. The code is
   * like this as it is easy to cut and paste in.
   */

  code_len = strlen (code);

  if (code_len >= 512) {
    printf("Too much code for the buffer.\n");
    cleanExit (1);
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
  
  if (bdmWriteSystemRegister (BDM_REG_RAMBAR, SRAMBAR | 1) < 0)
    showError ("Writing RAMBAR");
  
  if (bdmWriteMemory (SRAMBAR, wbuf, b) < 0)
    showError ("Writing SRAM buffer");
    
  if (bdmReadMemory (SRAMBAR, rbuf, b) < 0)
    showError ("Reading SRAM buffer");

  if (bdmWriteSystemRegister (BDM_REG_RPC, SRAMBAR) < 0)
    showError ("Writing PC");
  if (memcmp(wbuf,rbuf,b))
  {
    showError("Buffer mismatch");
  }

  if (bdm_revision >= 2 /* 5407, 547x & 548x need 0x81 for execution */
  && bdmWriteSystemRegister (BDM_REG_RAMBAR, SRAMBAR | 0x81) < 0)
    showError ("Changing RAMBAR");

  printf ("Stepping code.\n");
  
  for (i = 0; i < 18; i++) {
    if (bdmReadSystemRegister (BDM_REG_CSR, &csr) < 0)
      showError ("Reading CSR");
    if (bdmReadSystemRegister (BDM_REG_RPC, &pc) < 0)
      showError ("Reading PC");

    printf ("Stepping, pc is 0x%08lx, csr = 0x%08lx\n", pc, csr);

    if ((pc & 0xfffff000) != SRAMBAR) 
    {
      printf ("WARNING: PC is not in the SRAM, something is wrong.\n");
    }
  
    if (bdmStep () < 0)
      showError ("Step");
  }
  
  showRegs (BDM_COLDFIRE);
  checkRegValues();

  if (bdmGo () < 0)
    showError ("Go");

  sleep (1);

  if (bdmReadSystemRegister (BDM_REG_CSR, &csr) < 0)
    showError ("Reading CSR");

  if ((csr & 0x02000000) == 0) {
    printf ("CSR halt not set, target failed to halt, CSR = 0x%08lx\n", csr);
    cleanExit (1);
  }

  printf ("CSR halt set, target halted.\n");
  
  /*
   * For the Coldfire the target must be halted. This tests the csr caching.
   */
  status = bdmStatus ();
  printf ("Target status: 0x%x -- %s, %s, %s, %s, %s.\n", status,
          status & BDM_TARGETRESET   ? "RESET" : "NOT RESET", 
          status & BDM_TARGETHALT    ? "HALTED" : "NOT HALTED", 
          status & BDM_TARGETSTOPPED ? "STOPPED" : "NOT STOPPED", 
          status & BDM_TARGETPOWER   ? "POWER OFF" : "POWER ON", 
          status & BDM_TARGETNC      ? "NOT CONNECTED" : "CONNECTED");
  
  if (bdm_revision >= 2 /* restore data access on 5407, 547x & 548x */
  && bdmWriteSystemRegister (BDM_REG_RAMBAR, SRAMBAR | 0x1) < 0)
    showError ("Restoring RAMBAR");
}

void
coldfireSramVerify (int loops)
{
#define SRAM_BYTE_SIZE (1024) /* (1024 * 2) change for 5206e */
#define SRAM_BUF_SIZE  (SRAM_BYTE_SIZE / sizeof(unsigned long))
  
  unsigned long buf[SRAM_BUF_SIZE];
  unsigned int  test;
  unsigned int  i;
  int           sram_ok;
  int           loop = 0;

  printf ("Read/Write SRAM Test, %d loops\n", loops);
  
  if (bdmWriteSystemRegister (BDM_REG_RAMBAR, SRAMBAR | 1) < 0)
    showError ("Writing RAMBAR");
  
  while (loop < loops) {
    loop++;
    printf (" %5i : ", loop);
    for (test = 0; test < sizeof(test_pattern) / sizeof (test_pattern[0]); test++) {
      for (i = 0; i < SRAM_BUF_SIZE; i++) {
        buf[i] = test_pattern[test];
      }

      sram_ok = 1;
      
      if (bdmWriteMemory (SRAMBAR, (unsigned char*) buf, SRAM_BYTE_SIZE) < 0) {
        if (stop_quite)
          printf ("W");
        else
          showError ("Writing SRAM buffer");
      }

      if (bdmReadMemory (SRAMBAR, (unsigned char*) buf, SRAM_BYTE_SIZE) < 0) {
        if (stop_quite)
          printf ("R");
        else
          showError ("Reading SRAM buffer");
      }

      for (i = 0; i < SRAM_BUF_SIZE; i++) {
        if (buf[i] != test_pattern[test]) {
          sram_ok = 0;
          if (stop_quite)
            printf ("R");
          else {
            printf ("  addr=%08" PRIxMAX ",  write/read %08lx %08lx\n",
                    SRAMBAR + (i * sizeof(unsigned long)), test_pattern[test], buf[i]);
            showError ("Verifing SRAM");
          }
        }
      }

      if (sram_ok)
        printf (".");

      fflush(stdout);
    }
    printf ("\n");
  }
}

void
checkRegisters (int cpu, int loops)
{
  int           reg;
  unsigned long reg_value;
  unsigned int  i;
  int           reg_chk_loop_failed = 0;
  int           reg_chk_failed = 0;
  int           loop = 0;
  
  while (loop < loops) {
    loop++;
    if (reg_chk_loop_failed && force_reset_on_fail)
      do_reset (cpu);
    reg_chk_loop_failed = 0;
    printf ("Register test, %4d of %4d : \n", loop, loops);
    for (reg = BDM_REG_D0; reg <= BDM_REG_A7; reg++) {
      printf ("   %c%02d : ",
	      reg < BDM_REG_A0 ? 'D' : 'A',
	      reg < BDM_REG_A0 ? reg : reg-BDM_REG_A0);
      for (i = 0; i < sizeof(test_pattern) / sizeof (test_pattern[0]); i++) {
        if (bdmWriteRegister (reg, test_pattern[i]) < 0) {
          reg_chk_failed = reg_chk_loop_failed = 1;
          if (stop_quite)
            printf ("W");
          else {
            printf ("\n");
            showError ("Write Register");
          }
          continue;
        }
        if (bdmReadRegister (reg, &reg_value) < 0) {
          reg_chk_failed = reg_chk_loop_failed = 1;
          if (stop_quite)
            printf ("R");
          else {
            printf ("\n");
            showError ("Read Register");
          }
          continue;
        }
        if (reg_value != test_pattern[i]) {
          reg_chk_failed = reg_chk_loop_failed = 1;
          if (stop_quite)
            printf ("X");
          else {
            printf ("Write 0x%08lx, read 0x%08lx\n", test_pattern[i], reg_value);
            showError ("Verify register");
          }
        }
        else
          printf (".");
      }
      printf ("\n");
    }    
  }

  if (reg_chk_failed) {
    stop_on_error = 1;
    stop_quite = 0;
    showError("Reg Check Failed");
  }
}

#define ALIGN_MEM_SIZE (256)

int
verifyAlignment (int cpu)
{
  unsigned long  addr;
  unsigned char  byte;
  unsigned short word;
  unsigned long  lword;
  
  /*
   * Check the values using a byte read.
   */
  printf (" reading bytes :");
  for (addr = 0; addr < ALIGN_MEM_SIZE; addr += sizeof (byte)) {
    if (bdmReadByte (addr + SRAMBAR, &byte) < 0) {
      if (stop_quite)
        printf ("W");
      else {
        printf ("\n");
        printf ("Read byte at 0x%08lx\n", addr + SRAMBAR);
        showError ("Reading with bytes");
      }
      return -1;
    } else if (byte != addr) {
      if (stop_quite)
        printf ("b");
      else {
        printf ("\n");
        printf ("Read byte match addr=0x%08lx, read=0x%02x, wanted=0x%02x\n",
                addr + SRAMBAR, byte, (unsigned char) addr);
        showError ("Bytes read does not match");
      }
      return -1;
    }
    else {
      if ((addr % 32) == 0)
        printf ("\n");
      printf (".");
    }
  }
  /*
   * Check the values using a word read.
   */
  printf ("\n reading words :");
  for (addr = 0; addr < ALIGN_MEM_SIZE; addr += sizeof (word) ) {
    if (bdmReadWord (addr + SRAMBAR, &word) < 0) {
      if (stop_quite)
        printf ("w");
      else {
        printf ("\n");
        printf ("Read words (16bits) at 0x%08lx\n", addr + SRAMBAR);
        showError ("Reading with words");
      }
      return -1;
    } else if (word != ((addr << 8) | (addr + 1))) {
      if (stop_quite)
        printf ("l");
      else {
        printf ("\n");
        printf ("Read word (16bits) match addr=0x%08lx, read=0x%04x, wanted=0x%04x\n",
                addr + SRAMBAR, word, (unsigned short) ((addr << 8) | (addr + 1)));
        showError ("Words (16bits) does not match");
      }
      return -1;
    }
    else {
      if ((addr % 32) == 0)
        printf ("\n");
      printf (".");
    }
  }
  /*
   * Check the values using a long read.
   */
  printf ("\n reading long words :");
  for (addr = 0; addr < ALIGN_MEM_SIZE; addr += sizeof (lword) ) {
    if (bdmReadLongWord (addr + SRAMBAR, &lword) < 0) {
      if (stop_quite)
        printf ("W");
      else {
        printf ("\n");
        printf ("Read long words (32bits) at 0x%08lx\n", addr + SRAMBAR);
        showError ("Reading with long words");
      }
      return -1;
    } else if(lword !=
              ((addr << 24) | ((addr + 1) << 16) | ((addr + 2) << 8) | (addr + 3))) {
      if (stop_quite)
        printf ("W");
      else {
        printf ("\n");
        printf ("Read long word (32bits) match addr=0x%08lx"
                ", read=0x%08lx, wanted=0x%08lx\n",
                addr + SRAMBAR, lword,
                ((addr << 24) | ((addr + 1) << 16) | ((addr + 2) << 8) | (addr + 3)));
        showError ("Long Words (32bits) does not match");
      }
      return -1;
    }
    else {
      if ((addr % 32) == 0)
        printf ("\n");
      printf (".");
    }
  }
  return 0;
}

void
checkAlignment (int cpu, int loops)
{
  unsigned long addr;
  int           loop_failed = 0;
  int           failed = 0;
  int           loop = 0;
  unsigned char buf[ALIGN_MEM_SIZE];
  
  printf ("Alignment SRAM Test, %d loops\n", loops);
  
  if (bdmWriteSystemRegister (BDM_REG_RAMBAR, SRAMBAR | 1) < 0)
    showError ("Writing RAMBAR");
  
  for (addr = 0; addr < 256; addr++)
    buf[addr] = addr;
  
  while (loop < loops) {
    loop++;
    if (loop_failed && force_reset_on_fail) {
      do_reset (cpu);
      if (bdmWriteSystemRegister (BDM_REG_RAMBAR, SRAMBAR | 1) < 0)
        showError ("Writing RAMBAR");
    }
    loop_failed = 0;
    /*
     * Fill the memory with a byte count.
     */    
    printf ("Byte Write alignment write, %4d of %4d : ", loop, loops);
    for (addr = 0; addr < 256; addr++) {
      if (bdmWriteByte (addr + SRAMBAR, addr) < 0) {
          failed = loop_failed = 1;
          if (stop_quite)
            printf ("B");
          else {
            printf ("\n");
            printf ("Write byte 0x%02x to 0x%08lx\n", (unsigned char) addr, addr + SRAMBAR);
            showError ("Write byte pattern");
          }
          continue;
      }
      else {
        if ((addr % 32) == 0)
          printf ("\n");
        printf (".");
      }
    }
    printf ("\n");
    if (verifyAlignment (cpu) < 0) {
      failed = loop_failed = 1;
      if (!stop_quite) {
        printf ("\n");
        showError ("Verify byte write");
      }
    }
    /*
     * Fill the memory with a byte count.
     */    
    printf ("\nWord (16bits) Write alignment verify, %4d of %4d : ", loop, loops);
    for (addr = 0; addr < 256; addr += 2) {
      if (bdmWriteWord (addr + SRAMBAR, (addr << 8) | (addr + 1)) < 0) {
          failed = loop_failed = 1;
          if (stop_quite)
            printf ("W");
          else {
            printf ("\n");
            printf ("Write word (16bits) 0x%04x to 0x%08lx\n",
                    (unsigned short) addr, addr + SRAMBAR);
            showError ("Write word (16bits) pattern");
          }
          continue;
      }
      else {
        if ((addr % 32) == 0)
          printf ("\n");
        printf (".");
      }
    }
    printf ("\n");
    if (verifyAlignment (cpu) < 0) {
      failed = loop_failed = 1;
      if (!stop_quite) {
        printf ("\n");
        showError ("Verify word (16bits) write");
      }
    }
    /*
     * Fill the memory with a byte count.
     */    
    printf ("\nLong Word (32bits) Write alignment verify, %4d of %4d : ", loop, loops);
    for (addr = 0; addr < 256; addr += 4) {
      if (bdmWriteLongWord (addr + SRAMBAR,
                            ((addr << 24) | ((addr + 1) << 16) |
                             ((addr + 2) << 8) | (addr + 3)))) {
          failed = loop_failed = 1;
          if (stop_quite)
            printf ("L");
          else {
            printf ("\n");
            printf ("Write long word (32bits) 0x%08lx to 0x%08lx\n", addr, addr + SRAMBAR);
            showError ("Write long word (32bits) pattern");
          }
          continue;
      }
      else {
        if ((addr % 32) == 0)
          printf ("\n");
        printf (".");
      }
    }
    printf ("\n");
    if (verifyAlignment (cpu) < 0) {
      failed = loop_failed = 1;
      if (!stop_quite) {
        printf ("\n");
        showError ("Verify long word (32bits) write");
      }
    }
    printf ("\nBlock Write alignment verify, %4d of %4d : \n", loop, loops);
    if (bdmWriteMemory (SRAMBAR, (unsigned char*) buf, ALIGN_MEM_SIZE) < 0) {
      if (stop_quite)
        printf ("K");
      else
        showError ("Writing block buffer");
    }
    if (verifyAlignment (cpu) < 0) {
      failed = loop_failed = 1;
      if (!stop_quite) {
        printf ("\n");
        showError ("Verify long word (32bits) write");
      }
    }
  }

  if (failed) {
    stop_on_error = 1;
    stop_quite = 0;
    showError("Alignment Check Failed");
  }
  printf ("\n");
}

void
Usage()
{
  printf("chk -d [level] -p [pc] -r [loops] -s [loops] -C -Q -R [device]\n"
   " where :\n"
   "    -d [level]   : enable driver debug output\n"
   "    -p [pc]      : address to run to for the CPU32 target\n"
   "    -r [loops]   : number or register check loops\n"
   "    -s [loops]   : number or SRAM check loops on the Coldfire\n"
   "    -a [loops]   : number or alignment check loops\n"
   "    -D [delay]   : delay count for the clock generation\n"
   "    -C           : continue on an error\n"
   "    -Q           : be quiet on errors\n"
   "    -R           : reset on a register check fail\n"
   "    [device]     : the bdm device, eg /dev/bdmcf0\n");
  exit(0);
}

int
main (int argc, char **argv)
{
  char          *dev = NULL;
  int           arg;
  int           status;
  int           verbose = 0;
  int           delay_counter = 0;
  int           options_stop_on_error = 1;
  int           options_stop_quite = 0;
  unsigned int  ver;
  int           cpu;
  int           iface;
  unsigned long chkpc = 0;
  int           reg_chk_loops = 1;
  int           sram_chk_loops = 1;
  int           align_chk_loops = 1;

  if (argc <= 1) {
    Usage();
  }

  printf ("BDM Check for Coldfire processors.\n");
  
  for (arg = 1; arg < argc; arg++) {
    if (argv[arg][0] != '-') {
      if (dev) {
        printf(" device name specified more then once");
        exit(1);
      }
      dev = argv[arg];
    }
    else {
      switch (argv[arg][1]) {
        case 'd':
          arg++;
          if (arg == argc) {
            printf (" -d option requires a parameter");
            exit(0);
          }
          verbose = strtoul (argv[arg], NULL, 0);
          break;

        case 'D':
          arg++;
          if (arg == argc) {
            printf (" -D option requires a parameter");
            exit(0);
          }
          delay_counter = strtoul (argv[arg], NULL, 0);
          break;

        case 'p':
          arg++;
          if (arg == argc) {
            printf (" -p option requires a parameter");
            exit(0);
          }
          chkpc = strtoul (argv[arg], NULL, 0);
          break;

        case 'r':
          arg++;
            if (arg == argc) {
              printf (" -r option requires a parameter");
              exit(0);
            }
          reg_chk_loops = strtoul (argv[arg], NULL, 0);
          break;

        case 's':
          arg++;
            if (arg == argc) {
              printf (" -s option requires a parameter");
              exit(0);
            }
          sram_chk_loops = strtoul (argv[arg], NULL, 0);
          break;
          
        case 'a':
          arg++;
            if (arg == argc) {
              printf (" -a option requires a parameter");
              exit(0);
            }
          align_chk_loops = strtoul (argv[arg], NULL, 0);
          break;

        case 'C':
          options_stop_on_error = 0;
          break;
          
        case 'Q':
          options_stop_quite = 1;
          break;

        case 'R':
          force_reset_on_fail = 1;
          break;
          
        default:
          printf(" unknown option!");
    
        case '?':
        case 'h':
          Usage();
          break;
      }
    }
  }

  if (!dev)
  {
    printf(" ERROR: no device set, check your options as some require parameters.\n");
    exit (1);
  }

  printf ("Device: %s\n", dev);
  
  /*
   * Open the BDM interface driver
   */
  if (bdmOpen (dev) < 0)
    showError ("Open");

  if (verbose)
    bdmSetDriverDebugFlag (verbose);

  if (delay_counter)
    bdmSetDelay (delay_counter);

  /*
   * Get the driver version
   */
  if (bdmGetDrvVersion (&ver) < 0)
    showError ("GetDrvVersion");

  printf ("Driver Ver : %i.%i\n", ver >> 8, ver & 0xff);

  /*
   * Get the processor
   */
  if (bdmGetProcessor (&cpu) < 0)
    showError ("GetProcessor");

  switch (cpu) {
    case BDM_CPU32:
      printf ("Processor  : CPU32\n");
      break;
    case BDM_COLDFIRE:
      printf ("Processor  : Coldfire\n");
      break;
    default:
      printf ("unknown processor type!\n");
      cleanExit (1);
      break;
  }

  /*
   * Get the interface
   */
  if (bdmGetInterface (&iface) < 0)
    showError ("GetInterface");

  switch (iface) {
    case BDM_CPU32_ERIC:
      printf ("Interface  : Eric's CPU32\n");
      break;
    case BDM_COLDFIRE_TBLCF:
      printf ("Interface: TBLCF USB Coldfire\n");
      break;
    case BDM_COLDFIRE:
      printf ("Interface  : P&E Coldfire\n");
      break;
    case BDM_CPU32_ICD:
      printf ("Interface  : ICD (P&E) CPU32\n");
      printf ("  use the cpu32_check program for this interface.\n");
      cleanExit (1);
      break;
    default:
      printf ("unknown interface type!\n");
      cleanExit (1);
      break;
  }

  do_reset (cpu);
  
  stop_on_error = options_stop_on_error;
  stop_quite = options_stop_quite;
  
  /*
   * For the Coldfire the target must be stopped. This tests the csr caching.
   */
  status = bdmStatus ();
  printf ("Target status: 0x%x -- %s, %s, %s, %s, %s.\n", status,
          status & BDM_TARGETRESET   ? "RESET" : "NOT RESET", 
          status & BDM_TARGETHALT    ? "HALTED" : "NOT HALTED", 
          status & BDM_TARGETSTOPPED ? "STOPPED" : "NOT STOPPED", 
          status & BDM_TARGETPOWER   ? "POWER OFF" : "POWER ON", 
          status & BDM_TARGETNC      ? "NOT CONNECTED" : "CONNECTED");

  if (reg_chk_loops)
    checkRegisters (cpu, reg_chk_loops);
  
  if ((cpu == BDM_COLDFIRE) && sram_chk_loops)
    coldfireSramVerify (sram_chk_loops);

  
  if (align_chk_loops)
    checkAlignment (cpu, align_chk_loops);
  
  /*
   * Verify that target register can be written and read
   */
  if (cpu == BDM_CPU32) {
    cpu32Execute (chkpc);
  }
  else {
    coldfireExecute ();
  }
  
  showRegs (cpu);
  showLong (0x0FC00000);
  showLong (0x01000000);
  showLong (0x01001000);
  showLong (0x01001050);
  showLong (0x01001054);
  if (bdmGo () < 0)
    showError ("Go");

  cleanExit (0);

  return 0;
}
