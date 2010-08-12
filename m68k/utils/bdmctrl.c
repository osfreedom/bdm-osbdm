/* $Id: bdmctrl.c,v 1.24 2008/08/03 23:43:15 cjohns Exp $
 *
 * A utility to control bdm targets.
 *
 * 2003-10-12 jw  (jw@raven.inka.de)
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

#include <config.h>

#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include <elf-utils.h>

#include "BDMlib.h"
#include "flash_filter.h"

#define HAVE_FLASHLIB 1                 /* this should be provided by autoconf */

static void do_command (size_t argc, char **argv);

#ifndef STREQ
# define STREQ(a,b) (!strcmp((a),(b)))
#endif

#ifndef NUMOF
# define NUMOF(ary) (sizeof(ary)/sizeof(ary[0]))
#endif

static int verify;
static int verbosity = 1;
static int delay = 0;
static int debug_driver = 0;
static int fatal_errors = 0;
static char *progname;
static int cpu_type;
static time_t base_time;                /* when bdmctrl was started */

static unsigned int patcnt = 0;         /* number of test patterns */
static uint32_t *pattern = NULL;

typedef struct
{
  char *name;
  char *val;
} var_t;
static var_t *var = NULL;
static int num_vars = 0;

static int loaded_elf_cnt = 0;
static elf_handle *loaded_elfs = NULL;

static void
wait_here (int msecs)
{
#if defined (__MINGW32__)
  Sleep (msecs);
#else
  struct timeval tv;

  tv.tv_sec = msecs / 1000;
  tv.tv_usec = (msecs % 1000) * 1000;
  select (0, NULL, NULL, NULL, &tv);
#endif    
}

static void
clean_exit(int exit_value)
{
  int i;

  for (i = 0; i < loaded_elf_cnt; i++)
    elf_close (&loaded_elfs[i]);

  if (bdmIsOpen ()) {
    bdmSetDriverDebugFlag (0);
    bdmClose ();
  }

  exit (exit_value);
}

static void
fatal (char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  va_end (args);

  clean_exit (EXIT_FAILURE);
}

static void
warn (char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  va_end (args);

  if (fatal_errors)
    clean_exit (EXIT_FAILURE);
}

static void
set_var (const char *name, const char *val)
{
  int i;
  char *v;
  
  /* Set the variable in the flash system */
  
#if HAVE_FLASHLIB

  if(1)
  {
    char *e;
    uint32_t ival = strtoul(val, &e, 0);
    if (val != e) {
      flash_set_var (name, ival);
    }
  }
  
#endif

  if (!(v = strdup (val)))
    fatal ("Out of memory\n");

  for (i = 0; i < num_vars; i++) {
    if (STREQ (var[i].name, name)) {
      free (var[i].val);
      var[i].val = v;
      return;
    }
  }

  if (!(var = realloc (var, (num_vars + 1) * sizeof *var)) ||
      !(var[num_vars].name = strdup (name)))
    fatal ("Out of memory\n");

  var[num_vars++].val = v;
}

static char *
get_var (const char *name)
{
  int i;

  for (i = 0; i < num_vars; i++) {
    if (STREQ (var[i].name, name)) {
      return var[i].val;
    }
  }

  fatal ("Variable %s not defined\n", name);
  return 0;   /* remove warning */
}

/* Duplicate a string with (rudimentary) variable substitution.
 */
static char *
varstrdup (const char *instr, size_t argc, char **vars)
{
  int dlen = 0;
  size_t varnum = 0;
  char *dst, *str, *rstr, *s, *p;

  rstr = str = strdup (instr); /* make str writable */
  dst = malloc (10);           /* make sure we return a string */

  if (!str || !dst)
    return NULL;

  *dst = 0;

  while (*str) {
    s = str;
    if ((p = strchr (s, '$'))) {
      *p++ = 0;
      if (isdigit (*p)) {
        varnum = strtol (p, &str, 0);
        if (varnum < 1 || varnum >= argc)
          fatal ("argument $%d not available\n", varnum);
        p = vars[varnum];
      } else {
        p = get_var (p);
      }
    }
    while (s) {
      int slen = strlen (s);

      if (s == str)
        str += slen;
      if (!(dst = realloc (dst, dlen + slen + 1)))
        return NULL;
      strcpy (dst + dlen, s);
      dlen += slen;
      s = p;
      p = NULL;
    }
  }

  free (rstr);

  return dst;
}

/* build_argv() builds an argv-style array of pointers from an input line.
   It returns the number of arguments. The argv pointer and the pointers
   to the arguments can be passed to free(). glbl_argc/glbl_argv are used
   for variable substitution ($0, $1, $2, ...)
 */
static size_t
build_argv (const char *line, char **argv[],
            size_t glbl_argc, char *glbl_argv[])
{
  char *linedup, *p;
  size_t i, tnum = 0;

  /* Make sure realloc() will work properly
   */
  *argv = NULL;

  /* Modifying strings that we get passed is considered harmful. Therefore
     we make a copy to mess around with.
   */
  if (!(p = linedup = strdup (line)))
    fatal ("Out of memory\n");

  /* Split the line
   */
  while (1) {
    /* Allocate space for pointer to new argument. While we're at it,
       we allocate space for trailing NULL pointer, too.
     */
    if (!(*argv = realloc (*argv, (tnum + 2) * sizeof (**argv))))
      fatal ("Out of memory\n");

    /* Skip leading whitespace. Terminate at end of line.
     */
    while (*p && strchr (" \t", *p))
      p++;
    if (!*p)
      break;

    /* Store argument.
     */
    (*argv)[tnum++] = p;

    /* Search delimiter and terminate the argument.
     */
    while (*p && !strchr (" \t", *p))
      p++;
    if (*p)
      *p++ = 0;
  };

  /* Substitute variables ind convert pointers to something that can be
     passed to free()
   */
  for (i = 0; i < tnum; i++) {
    if (!((*argv)[i] = varstrdup ((*argv)[i], glbl_argc, glbl_argv)))
      fatal ("Out of memory\n");
  }
  free (linedup);

  (*argv)[tnum] = NULL;

  return tnum;
}

/* Now, we define some helper functions. We don't want to mess around with
   special cases like byte/word/long accesses or system/non-system registers
   all the time. In addition, we can move error checks into those helper
   functions. This greatly simplifies the real working code.
 */

/* mapping register-name => register-number
 */
typedef struct
{
  char *regname;
  int regnum;
  int issystem;
} regname_t;

#define REGNAM(nam, issystem) { #nam, BDM_REG_##nam, issystem }

regname_t regnames[] = {
  REGNAM(A0, 0),   REGNAM(A1, 0),     REGNAM(A2, 0),   REGNAM(A3, 0),
  REGNAM(A4, 0),   REGNAM(A5, 0),     REGNAM(A6, 0),   REGNAM(A7, 0),
  REGNAM(D0, 0),   REGNAM(D1, 0),     REGNAM(D2, 0),   REGNAM(D3, 0),
  REGNAM(D4, 0),   REGNAM(D5, 0),     REGNAM(D6, 0),   REGNAM(D7, 0),
  REGNAM(RPC, 1),  REGNAM(PCC, 1),    REGNAM(SR, 1),   REGNAM(USP, 1),
  REGNAM(SSP, 1),  REGNAM(SFC, 1),    REGNAM(DFC, 1),  REGNAM(ATEMP, 1),
  REGNAM(FAR, 1),  REGNAM(VBR, 1),    REGNAM(CACR, 1), REGNAM(ACR0, 1),
  REGNAM(ACR1, 1), REGNAM(RAMBAR, 1), REGNAM(MBAR, 1), REGNAM(CSR, 1),
  REGNAM(AATR, 1), REGNAM(TDR, 1),    REGNAM(PBR, 1),  REGNAM(PBMR, 1),
  REGNAM(ABHR, 1), REGNAM(ABLR, 1),   REGNAM(DBR, 1),  REGNAM(DBMR, 1),
};

#undef REGNAM

/* comparison function for regname_t
 */
static int
cmpreg (const void *ap, const void *bp)
{
  return strcmp (((const regname_t *) ap)->regname,
                 ((const regname_t *) bp)->regname);
}

/* Find a register by its name. If found, provide the nuber of the register
   and whether it is a system-register or not.
*/
static int
find_register (const char *regname, int *regnum, int *issystem)
{
  regname_t *found, key;
  const char *s = regname;
  char name[20];
  char *d = name;

  if (*s == '%')
    s++;

  /* make uppercase copy of the name
   */
  while (d - name < ((int) sizeof (name)) && (*d++ = toupper (*s++))) ;
  name[sizeof (name) - 1] = 0;

  key.regname = name;
  if ((found = bsearch (&key, regnames, NUMOF(regnames),
                        sizeof (regnames[0]), cmpreg))) {
    *regnum = found->regnum;
    *issystem = found->issystem;
    return 1;
  }

  return 0;
}

/* Read from a register by name
 */
static void
read_register (const char *regname, uint32_t * val)
{
  int issystem;
  int regnum;
  unsigned long rval;

  if (!find_register (regname, &regnum, &issystem)) {
    warn ("No such register \"%s\"\n", regname);
    return;
  }

  if (issystem) {
    if (bdmReadSystemRegister (regnum, &rval) >= 0) {
      *val = (uint32_t) rval;
      return;
    }
  } else {
    if (bdmReadRegister (regnum, &rval) >= 0) {
      *val = (uint32_t) rval;
      return;
    }
  }

  fatal ("Can't read register \"%s\": %s\n", regname, bdmErrorString ());
}

/* Write into a register by name. FIXME: should use a size parameter to
   distinguish between 8/16/32 bit writes.
 */
static void
write_register (const char *regname, uint32_t val)
{
  int issystem;
  int regnum;

  if (!find_register (regname, &regnum, &issystem)) {
    warn ("No such register \"%s\"\n", regname);
    return;
  }

  if (issystem) {
    if (bdmWriteSystemRegister (regnum, val) >= 0)
      return;
  } else {
    if (bdmWriteRegister (regnum, val) >= 0)
      return;
  }

  fatal ("Can't write 0x%08x into register \"%s\": %s\n",
         val, regname, bdmErrorString());
}

/* read values of one, two or four bytes into a long
 */
static int
read_value (uint32_t adr, uint32_t * val, char size)
{
  int ret = -1;
  unsigned char char_val;
  unsigned short short_val;
  unsigned long long_val;

  switch (size) {
    case '1':
    case 1:
    case 'b':
    case 'B':
      ret = bdmReadByte (adr, &char_val);
      *val = char_val;
      size = 1;
      break;
    case '2':
    case 2:
    case 'w':
    case 'W':
      ret = bdmReadWord (adr, &short_val);
      *val = short_val;
      size = 2;
      break;
    case '4':
    case 4:
    case 'l':
    case 'L':
      ret = bdmReadLongWord (adr, &long_val);
      *val = long_val;
      size = 4;
      break;
    default:
      fatal ("Unknown size spezifier '%c'\n", size);
  }

  if (ret < 0)
    fatal ("read_value (0x%08x,xxx,%d): %s\n", adr, size, bdmErrorString ());

  return size;
}

/* write values of one, two or four bytes
 */
static int
write_value (uint32_t adr, uint32_t val, char size)
{
  int ret = -1;

  switch (size) {
    case '1':
    case 1:
    case 'b':
    case 'B':
      ret = bdmWriteByte (adr, val);
      size = 1;
      break;
    case '2':
    case 2:
    case 'w':
    case 'W':
      ret = bdmWriteWord (adr, val);
      size = 2;
      break;
    case '4':
    case 4:
    case 'l':
    case 'L':
      ret = bdmWriteLongWord (adr, val);
      size = 4;
      break;
    default:
      fatal ("Unknown size specifier '%c'\n", size);
  }

  if (ret < 0)
    fatal ("write_value(0x%08x,0x%x,%d): %s\n",
           adr, val, size, bdmErrorString());

  return size;
}

#if !HAVE_FLASHLIB

/* Write a buffer to the target.
 */
uint32_t
write_memory (uint32_t adr, unsigned char *buf, uint32_t cnt)
{
  if (bdmWriteMemory (adr, buf, cnt) < 0)
    fatal ("\nCan not download 0x%08x bytes to addr 0x%08lx: %s\n",
           cnt, adr, bdmErrorString());
  return cnt;
}

int
flash_plugin (uint32_t adr, uint32_t len, char *argv[])
{
  fatal ("No flash support available\n");
  return 0;
}

int
flash_register (uint32_t adr)
{
  fatal ("No flash support available\n");
  return 0;
}

int
flash_erase (uint32_t adr, int32_t sec_offs)
{
  fatal ("No flash support available\n");
  return 0;
}

int
flash_blank_chk (uint32_t adr, int32_t sec_adr)
{
  fatal ("No flash support available\n");
  return 0;
}

#endif

/* Read a buffer from the target.
 */
static void
read_memory (uint32_t adr, unsigned char *buf, uint32_t cnt)
{
  if (bdmReadMemory (adr, buf, cnt) < 0)
    fatal ("\nCan not upload 0x%08x bytes from addr 0x%08lx: %s\n",
           cnt, adr, bdmErrorString());
}

/* Evaluate (rudimentary) a string. For now, no real evaluation is done.
   Only numbers, registers and symbols can be distinguished.
 */
static uint32_t
eval_string (char *val)
{
  uint32_t v;
  int i;

  if (isdigit (*val))
    return strtoul (val, NULL, 0);

  if (*val == '%') {
    read_register (val, &v);
    return v;
  }

  /* search opened bfd's for the symbol. The bfd's are searched in reverse
     order, so loading a new bfd will override symbols with same name from
     bfd's that were loaded before.
   */
  for (i = 0; i < loaded_elf_cnt; i++) {
    elf_handle *elf = &loaded_elfs[loaded_elf_cnt - i - 1];
    GElf_Sym sym;

    if (elf_get_symbol (elf, val, &sym))
      return sym.st_value;
  }

  fatal ("Can't find symbol \"%s\"\n", val);
  return 0; /* omit compiler warning */
}

/* check one register
 */
static void
check_one_register (char *regname)
{
  unsigned int i;
  uint32_t reg_value;
  uint32_t orig_value;

  read_register (regname, &orig_value);  /* read original value */

  for (i = 0; i < patcnt; i++) {
    write_register (regname, pattern[i]);
    read_register (regname, &reg_value);
    if (reg_value != pattern[i])
      warn ("register %s written 0x%08x, read back 0x%08x\n");
  }

  write_register (regname, orig_value);  /* restore original value */

  if (verbosity)
    printf(" %s", regname);
}

/* load a bfd section into target
 */

static int
load_section (elf_handle * elf, GElf_Phdr * phdr, GElf_Shdr * shdr, 
              const char *sname, int sindex)
{
  if ((shdr->sh_type == SHT_PROGBITS) &&
      (shdr->sh_flags & (SHF_WRITE | SHF_ALLOC))) {

    unsigned char rbuf[1 * 1024];
    uint32_t off = 0;
    unsigned char *data;
    uint32_t size = 0;
    uint32_t dfc = 0;
    uint32_t paddr = (uint32_t) shdr->sh_addr;
    
    if(phdr) {
      paddr = phdr->p_paddr + (shdr->sh_addr - phdr->p_vaddr);
    }

    if (cpu_type == BDM_CPU32) {
      read_register ("dfc", &dfc);
      write_register ("dfc", (shdr->sh_flags & SHF_EXECINSTR) ? 6 : 5);
    }

    if (verbosity) {
      printf (" fl:%15s 0x%08lx..0x%08lx (0x%08lx): 0x%08lx:   ",
              sname, (long unsigned int) paddr,
              (long unsigned int) (paddr + shdr->sh_size),
              (long unsigned int) shdr->sh_size,
              (long unsigned int) 0);
      fflush (stdout);
    }

    data = elf_get_section_data (elf, sindex, &size);
    if (size && !data) {
      printf (" cannot load data for section: %s", sname);
      return 0;
    }

    for (off = 0; off < shdr->sh_size; off += sizeof (rbuf)) {
      unsigned int cnt = shdr->sh_size - off;
      int ret;

      if (cnt > sizeof (rbuf))
        cnt = sizeof (rbuf);

      if ((ret = write_memory (paddr + off, data + off, cnt)) != cnt) {
        if (verbosity)
          printf ("\b\bFAIL\n");
        warn ("%swrite_memory(0x%x, xxx, 0x%x)==%d failed\n",
              verbosity ? "" : "\n", paddr + off, cnt, ret);
        return 0;
      }

      if (verify) {
        read_memory (paddr + off, rbuf, cnt);
        if (memcmp (data + off, rbuf, cnt)) {
          if (verbosity)
            printf ("\b\bFAIL\n");
          warn ("%sRead back contents from 0x%08lx don't match\n",
                verbosity ? "" : "\n", paddr + off);
          return 0;
        }
      }

      if (verbosity) {
        printf ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b0x%08x: OK", off + cnt);
        fflush (stdout);
      }
    }

    if (verbosity && off >= shdr->sh_size)
      printf ("\n");

    if (cpu_type == BDM_CPU32) {
      write_register ("dfc", dfc);
    }
  }

  return 1;
}

/* invert portions of a longword and check whether the correct portions
   were actually inverted.
 */
static void
check_alignment (uint32_t adr, char size)
{
  unsigned i;
  uint32_t readback;
  uint32_t mask = 0x0ff;
  uint32_t off = 0;
  uint32_t pat = pattern[0];

  switch (size) {
    case 1:
    case '1':
    case 'B':
    case 'b':
      off = 3;
      mask = 0x0ff;
      break;
    case 2:
    case '2':
    case 'W':
    case 'w':
      off = 2;
      mask = 0x0ffff;
      break;
    case 4:
    case '4':
    case 'L':
    case 'l':
      off = 0;
      mask = 0x0ffffffff;
      break;
    default:
      fatal ("Unknown size specifier %c\n", size);
  }

  /* write pattern
   */
  write_value (adr, pat, 4);

  for (i = 0; i < 4; i += size) {
    pat ^= (mask << (i * 8));

    write_value (adr + off - i, (pat >> (i * 8)) & mask, size);

    read_value (adr, &readback, 4);

    if (readback != pat)
      warn (" Readback failed at 0x%08lx expect 0x%08lx read 0x%08lx\n",
            adr, pat, readback);
  }
}

/* Split a line by whitespace, do variable substitutions and execute the
   command
 */
static void
exec_line (const char *line, size_t glbl_argc, char **glbl_argv)
{
  int i, ac;
  char **av;

  ac = build_argv (line, &av, glbl_argc - 1, glbl_argv + 1);

  if (ac) {
    do_command (ac, av);

    for (i = 0; i < ac; i++)
      free (av[i]);
    free (av);
  }
}

/* Here we're done with the helper functions. Define the real command
   functions now
 */

/* Register test patterns for the "check-mem" and "check-register" commands.
   if argv==NULL, argc random patterns are generated.
 */
static void
cmd_patterns (size_t argc, char *argv[])
{
  unsigned int i;

  if (argc == 3 && argv && STREQ (argv[1], "random")) {
    warn ("command \"patterns random CNT\" deprecated."
          " Use \"random-patterns CNT\" instead.\n");
    argc = strtoul (argv[2], NULL, 0);
    argv = 0;
  }

  if (!(pattern = realloc (pattern, argc * sizeof (*pattern))))
    fatal ("Out of memory\n");

  if (argv) {
    argc--;
    for (i = 0; i < argc; i++) {
      pattern[i] = eval_string (argv[i + 1]);
    }
  } else {
    for (i = 0; i < 2 * argc; i++) {
      ((unsigned short *) pattern)[i] = (rand() >> 3) & 0x0ffff;
    }
  }

  patcnt = argc;

  if (verbosity) {
    printf ("\n");
    for (i = 0; i < patcnt; i++) {
      printf (" pattern %d: 0x%08lx\n", i, (long unsigned int) pattern[i]);
    }
  }
}

/* Generate random test patterns for register/memory checks. We use only
   bits 3..18 of the generated numbers since the lower bits are less random
   and bit 31 would always be zero on a typical host.
 */
static void
cmd_patterns_rnd (size_t argc, char *argv[])
{
  cmd_patterns (strtoul(argv[1], NULL, 0), NULL);
}

/* check registers
 */
static void
cmd_check_reg (size_t argc, char **argv)
{
  unsigned int i;

  if (!patcnt)
    cmd_patterns (37, NULL);             /* generate prime number of test patterns */

  if (verbosity)
    printf ("\n");

  for (i = 1; i < argc; i++)
    check_one_register (argv[i]);

  if (verbosity)
    printf (" OK\n");
}

/* dump register
 */
static void
cmd_dump_reg (size_t argc, char **argv)
{
  unsigned int i;
  uint32_t reg_value;

  if (verbosity)
    printf ("\n");

  for (i = 1; i < argc; i++) {
    read_register (argv[i], &reg_value);
    printf (" %7s: 0x%08lx\n", argv[i], (long unsigned int) reg_value);
  }

  if (verbosity)
    printf(" OK\n");
}

/* write a value to a specific address
 */
static void
cmd_write (size_t argc, char **argv)
{
  char *dst;
  uint32_t val;

  dst = argv[1];
  val = eval_string (argv[2]);

  if (verbosity) {
    printf ("0x%08lx to %s", (long unsigned int) val, dst);
    fflush (stdout);
  }

  if (*dst == '%') {
    write_register (dst, val);
  } else {
    write_value (eval_string (dst), val, argv[3][0]);
  }

  if (verbosity)
    printf (" OK\n");
}

/* write a value to a control register
 */
static void
cmd_write_ctrl (size_t argc, char **argv)
{
  uint32_t val;
  int dst;

  dst = (int) eval_string (argv[1]);
  val = eval_string (argv[2]);

  if (verbosity) {
    printf ("0x%08lx to ctrl-reg 0x%04x", (long unsigned int) val, dst);
    fflush (stdout);
  }

  if (bdmWriteControlRegister (dst, val) < 0)
    fatal ("Can't write 0x%08x into control-register 0x%04x: %s\n",
           val, dst, bdmErrorString());

  if (verbosity)
    printf (" OK\n");
}

/* dump a memory region
 */
static void
cmd_dump_mem (size_t argc, char **argv)
{
  uint32_t i;
  uint32_t src;
  uint32_t val;
  uint32_t len;
  int rlen;
  FILE *file = stdout;

  src = eval_string (argv[1]);
  len = eval_string (argv[2]);

  if (argc == 5 && !(file = fopen (argv[4], "w")))
    fatal ("Can't open \"%s\":%s\n", argv[4], strerror(errno));

  for (i = 0; i < len; i += rlen) {
    if (!(i % 16))
      fprintf (file, "\n 0x%08lx: ", (long unsigned int) (src + i));
    rlen = read_value (src + i, &val, argv[3][0]);
    fprintf (file, " 0x%0*lx", 2 * rlen, (long unsigned int) val);
    fflush (file);
  }

  if (file != stdout)
    fclose (file);

  printf ("\nOK\n");
}

/* check a memory region
 */
static void
cmd_check_mem (size_t argc, char **argv)
{
  uint32_t base;
  uint32_t size;
  uint32_t off;
  uint32_t *wbuf;
  uint32_t *rbuf;
  unsigned char *sav;

  if (!patcnt)
    cmd_patterns (37, NULL);             /* generate prime number of test patterns */

  base = eval_string (argv[1]);
  size = eval_string (argv[2]);

  sav = malloc (size + 4);
  wbuf = malloc (size + 4);
  rbuf = malloc (size + 4);

  if (!sav || !wbuf || !rbuf)
    fatal ("Out of memory\n");

  /* Fill buffer with a known pattern.
   */
  for (off = 0; off < size / 4; off++) {
    wbuf[off] = pattern[off % patcnt];
  }

  read_memory (base, sav, size);                     /* save original contents */
  write_memory (base, (unsigned char *) wbuf, size); /* write pattern */
  read_memory (base, (unsigned char *) rbuf, size);  /* read it back */

  if (memcmp (wbuf, rbuf, size))
    warn (" Readback failed at 0x%08lx\n", base);

  for (off = 0; off < size; off += 4) {
    check_alignment (base + off, 'b');
    check_alignment (base + off, 'w');
  }

  write_memory (base, sav, size);        /* restore original contents */

  free (sav);
  free (wbuf);
  free (rbuf);

  if (verbosity)
    printf ("OK\n");
}

/* load binary into target
 */
static void
cmd_load (size_t argc, char **argv)
{
  elf_handle elf;

  if (verbosity)
    printf ("\n");

  verify = 0;
  if (STREQ (argv[1], "-v")) {
    verify = 1;
    argv++;
  }

  if (argc < 2)
    fatal ("Wrong number of arguments\n");

  elf_handle_init (&elf);

  if (!elf_open (argv[1], &elf, printf)) {
    warn ("can not open %s: %s\n", argv[1], strerror (errno));
    return;
  }

  /* FIXME: loading a file multiple times should not open it again. Only
     the search order for the symbols should be adjusted. The question is
     how to distinguish foo/bar from foo/baz/../bar. On unix, dev/inode
     could be used for this. But how to do it on windows?
   */
  if (!(loaded_elfs = realloc (loaded_elfs,
                               (loaded_elf_cnt + 1) * sizeof (elf_handle))))
    fatal ("Out of memory\n");

  loaded_elfs[loaded_elf_cnt++] = elf;

  elf_map_over_sections (&elf, load_section, argv[2]);

  write_register ("rpc", elf.ehdr.e_entry);

  if (verbosity)
    printf ("PC set to default entry address: 0x%08lx\n",
            (long unsigned int) elf.ehdr.e_entry);
}

/* execute code withhin target
 */
static void
cmd_execute (size_t argc, char **argv)
{
  uint32_t addr;

  if (argc >= 2)
    write_register ("rpc", eval_string (argv[1]));

  read_register ("rpc", &addr);

  if (verbosity)
    printf ("Run at 0x%08lx\n", (long unsigned int) addr);

  if (bdmGo () < 0)
    fatal ("Can not run the taget at 0x%08lx\n", addr);
}

/* single step target
 */
static void
cmd_step (size_t argc, char **argv)
{
  uint32_t addr;

  if (argc >= 2)
    write_register ("rpc", eval_string (argv[1]));

  read_register("rpc", &addr);

  if (verbosity)
    printf("Step at 0x%08lx\n", (long unsigned int) addr);

  if (bdmStep () < 0)
    fatal ("Can not step the taget at 0x%08lx\n", addr);
}

/* set a variable
 */
static void
cmd_set (size_t argc, char *argv[])
{
  char *val;
  set_var (argv[1], argv[2]);

  val = get_var (argv[1]);
  printf ("%s = %s ", argv[1], (val) ? val : "null");
  
  if (verbosity)
    printf ("OK\n");
}

/* read a line from stdin and assign arguments to variables
 */
static void
cmd_read (size_t argc, char *argv[])
{
  int i, ac;
  char *p, **av, buf[1024];

  if (!fgets (buf, 1020, stdin))
    fatal ("Can't read stdin\n");
  buf[1020] = 0;
  if ((p = strchr (buf, '\n')))
    *p = 0;

  ac = build_argv (buf, &av, 0, NULL);
  if (ac) {
    for (i = 1; i < argc; i++) {
      if (i > ac)
        fatal ("Not enough arguments specified\n");
      set_var (argv[i], av[i - 1]);
      free (av[i - 1]);
    }
    free (av);
  }

  if (verbosity)
    printf ("OK\n");
}

/* exit bdmctrl
 */
static void
cmd_exit (size_t argc, char *argv[])
{
  if (verbosity)
    printf ("OK\n");
  clean_exit (EXIT_SUCCESS);
}

/* reset the target
 */
static void
cmd_reset (size_t argc, char *argv[])
{
  if (bdmReset () < 0)
    fatal ("bdmReset (): %s\n", bdmErrorString ());

  if (verbosity)
    printf ("OK\n");
}

/* Wait until target is stopped or hlated
 */
static void
cmd_wait (size_t argc, char **argv)
{
  while (!((bdmStatus ()) & (BDM_TARGETSTOPPED | BDM_TARGETHALT)))
    wait_here (250);

  if (verbosity)
    printf ("OK\n");
}

/* print out seconds since start
 */
static void
cmd_time (size_t argc, char **argv)
{
  printf (" %ld seconds\n", time(NULL) - base_time);
}

/* sleep for a while
 */
static void
cmd_sleep (size_t argc, char **argv)
{
  wait_here (strtoul (argv[1], NULL, 0));
  if (verbosity)
    printf ("OK\n");
}

/* output a line of text
 */
static void
cmd_echo (size_t argc, char **argv)
{
  int i;

  for (i = 1; i < argc; i++)
    printf ("%s%s", i > 1 ? " " : "", argv[i]);
  printf ("\n");
}

/* autodetect flash hardware
 */
static void
cmd_flash (size_t argc, char **argv)
{
  char name[1024];
  uint32_t adr = strtoul (argv[1], NULL, 0);

  if (flash_register (name, adr, (argc > 2) ? argv[2] : NULL)) {
    if (verbosity)
      printf ("%s\n", name);
  } else {
    warn ("Could not detect flash hardware on 0x%x\n", adr);
  }
}

/* load target flash driver
 */
static void
cmd_flashplug (size_t argc, char **argv)
{
  uint32_t adr = strtoul (argv[1], NULL, 0);
  uint32_t len = strtoul (argv[2], NULL, 0);

  flash_plugin (verbosity ? printf : NULL, adr, len, argv + 3);
  if (verbosity)
    printf ("\n");
}

/* erase flash hardware
 */
static void
cmd_erase (size_t argc, char **argv)
{
  int ret;
  uint32_t adr = strtoul (argv[1], NULL, 0);

  if (STREQ (argv[2], "wait")) {
    warn ("command \"erase BASE wait\" deprecated."
          " Use \"erase-wait BASE\" instead.\n");
    ret = flash_erase_wait (adr);
  } else {
    ret = flash_erase (adr, strtol(argv[2], NULL, 0));
  }

  if (ret) {
    if (verbosity)
      printf ("OK\n");
  } else {
    warn ("FAIL\n");
  }
}

/* blank check flash hardware
 */
static void
cmd_blank_chk (size_t argc, char **argv)
{
  int ret;
  uint32_t adr = strtoul (argv[1], NULL, 0);

  ret = flash_blank_chk (adr, strtol (argv[2], NULL, 0));

  if (ret) {
    if (verbosity)
      printf ("OK\n");
  } else {
    warn ("FAIL\n");
  }
}

/* wait for flash hardware to finish erase operation
 */
static void
cmd_erase_wait(size_t argc, char **argv)
{
  if (flash_erase_wait (strtoul (argv[1], NULL, 0))) {
    if (verbosity)
      printf ("OK\n");
  } else {
    warn ("FAIL\n");
  }
}

/* read commands from a file and execute them
 */
static void
cmd_source (size_t argc, char **argv)
{
  char buf[1024];
  FILE *file = stdin;

  if (argc >= 2 && !(file = fopen (argv[1], "r")))
    fatal ("%s: %s: %s\n", progname, argv[1], strerror (errno));

  while (fgets (buf, 1020, file)) {
    char *p;

    buf[1020] = 0;
    if ((p = strchr (buf, '\n')))
      *p = 0;
    if ((p = strchr (buf, '#')))
      *p = 0;
    exec_line (buf, argc, argv);
  }

  fclose(file);
}

/* Open BDM device
 */
static void
cmd_open (size_t argc, char **argv)
{
  /* open BDM port and set basic options
   */
  if (bdmOpen (argv[1]) < 0)
    fatal ("bdmOpen (\"%s\"): %s\n", argv[1], bdmErrorString ());

  if (debug_driver) {
    bdmSetDriverDebugFlag (1);
    bdmSetDebugFlag (1);  
  }
  if (delay)
    bdmSetDelay (delay);

  /* print information we can retrieve from driver
   */
  if (verbosity) {
    unsigned int driver_version;
    int iface;

    if (bdmGetDrvVersion (&driver_version) < 0)
      fatal("bdmGetDrvVersion (): %s\n", bdmErrorString ());

    printf ("BDM Driver Version: %x.%d\n",
            driver_version >> 8, driver_version & 0xff);

    if (bdmGetProcessor (&cpu_type) < 0)
      fatal("bdmGetProcessor (): %s\n", bdmErrorString ());

    switch (cpu_type) {
      case BDM_CPU32:
        printf ("Processor: CPU32\n");
        break;
      case BDM_COLDFIRE:
        printf ("Processor: Coldfire\n");
        break;
      default:
        fatal ("Unsupported processor type %d!\n", cpu_type);
    }

    if (bdmGetInterface (&iface) < 0)
      fatal("bdmGetInterface (): %s", bdmErrorString ());

    switch (iface) {
      case BDM_COLDFIRE_PE:
        printf ("Interface: P&E Coldfire\n");
        break;
      case BDM_COLDFIRE_TBLCF:
        printf ("Interface: TBLCF USB Coldfire\n");
        break;
      case BDM_CPU32_PD:
        printf ("Interface: Eric's CPU32\n");
        break;
      case BDM_CPU32_ICD:
        printf ("Interface: ICD (P&E) CPU32\n");
        break;
      default:
        fatal ("Unknown or unsupported interface type %d!\n", iface);
    }
  }
}

/* Known commands and their implementations.
 */

/* *INDENT-OFF* */ 

static struct command_s {
  char *name;
  char *args;
  int need_device;
  size_t min, max; /* number of arguments (inclusive command by itself) */
  void (*func)(size_t, char**);
  char *helptext;
} command[] = {
  { "open",            "DEV",                 0, 2,       2, cmd_open,
    "Open the bdm device DEV\n"
  },
  { "reset",           "",                    1, 1,       1, cmd_reset,
    "Reset the target.\n"
  },
  { "check-register",  "REG [REG ...]",       1, 2, INT_MAX, cmd_check_reg,
    "The specified registers are tested with random testpatterns.  After\n"
    "the test, the original values of the registers are restored.\n"
  },
  { "dump-register",   "REG [REG ...]",       1, 2, INT_MAX, cmd_dump_reg,
    "The contents of the specified registers are dumped to stdout.\n"
  },
  { "check-mem",       "ADR SIZ",             1, 3,       3, cmd_check_mem,
    "The specified memory region is checked with a random testpattern.  In\n"
    "addition, an alingnment test is done.  After the test the original\n"
    "memory contents are restored.\n"
    "ADR and SIZ can be a number, a register or a symbol.\n"
  },
  { "dump-mem",        "ADR SIZ WIDTH [FN]",  1, 4,       5, cmd_dump_mem,
    "Dump memory contents to stdout.  The WIDTH argument specifies whether\n"
    "bytes, words or longwords are dumped.\n"
    "ADR and SIZ can be a number, a register or a symbol.\n"
    "WIDTH can be '1', 'b', 'B', '2', 'w', 'W', '4', 'l' or 'L'.\n"
    "if FN is specified, the contents are dumped to the named file.\n"
  },
  { "write",           "DST VAL WIDTH",       1, 4,       4, cmd_write,
    "Write a VAL with WIDTH to destination DST.  VAL and DST can be an\n"
    "absolute memory address, a register or a symbol.\n"
    "DST and VAL can be a number, a register or a symbol.\n"
    "WIDTH can be '1', 'b', 'B', '2', 'w', 'W', '4', 'l' or 'L'.\n"
  },
  { "write-ctrl",      "DST VAL",             1, 3,       3, cmd_write_ctrl,
    "Write a VAL to destination control register DST.\n"
  },    
  { "load",            "[-v] FN [SEC ...]",   1, 2, INT_MAX, cmd_load,
    "Load object file FN into the target.  Only the specified sections are\n"
    "loaded.  When no sections are specified, only sections with the\n"
    "SEC_LOAD flag are loaded.  If FN has an entry address specified, %rpc\n"
    "is set to this address.  With the -v flag, the written contents are\n"
    "read back and verified.\n"
    "After the load, the symbols from the loaded file are known to the\n"
    "commands which can deal with symbols.\n"
    "Please note that s-record and intel-hex don't have section names.  In\n"
    "this cases BFD assigns '.sec1', '.sec2' and so forth as section\n"
    "names.\n"
  },
  { "execute",         "[ADR]",               1, 1,       2, cmd_execute,
    "Run the target at ADR.  If ADR is omitted, the target will run\n"
    "from %rpc.  In this case you probably want to make sure that you have\n"
    "loaded a file with an entry address definition.\n"
    "ADR can be a number, a register or a symbol.\n"
  },
  { "step",            "[ADR]",               1, 1,       2, cmd_step,
    "Step the target at ADR.  If ADR is omitted, the target will\n"
    "step on %rpc.  In this case you probably want to make sure that you\n"
    "have loaded a file with an entry address definition.\n"
    "ADR can be a number, a register or a symbol.\n"
  },
  { "set",             "VAR VAL",             0, 3,       3, cmd_set,
    "Define variable VAR and set its value to VAL.\n"
  },
  { "read",            "[VAR ...]",           0, 1, INT_MAX, cmd_read,
    "Read a line from stdin, split it into arguments and assign the\n"
    "arguments to specified variables.\n"
  },
  { "sleep",           "MSEC",                0, 2,       2, cmd_sleep,
    "Sleep for MSEC milli-seconds.\n"
  },
  { "wait",            "",                    1, 1,       1, cmd_wait,
    "Wait until target is halted/stopped.\n"
  },
  { "time",            "",                    0, 1,       1, cmd_time,
    "Print seconds since bdmctrl was started.\n"
  },
  { "echo",            "[ARG ...]",           0, 1, INT_MAX, cmd_echo,
    "Print a line of text."
  },
  { "exit",            "",                    0, 1,       1, cmd_exit,
    "Exit bdmctrl immediately.\n"
  },
  { "source",          "[FN [ARG ...]]",      0, 1, INT_MAX, cmd_source,
    "Execute commands from file FN.  Withhin FN, the variables $1, $2, $3\n"
    "(and so on) are replaced by the specified arguments.  After all the\n"
    "commands from FN are executed, control returns to the original\n"
    "position, so recursive execution is possible. When FN is omitted,\n"
    "commands are read from stdin.  Please note that it doesn't make much\n"
    "sense to source stdin more than one time.\n"
  },
  { "patterns",        "PAT [PAT ...]",       0, 2, INT_MAX, cmd_patterns,
    "The provided argument numbers are taken as test patterns for the\n"
    "'check-register' and 'check-mem' commands.  The patterns are\n"
    "32 bits wide.  On startup, bdmctrl generates 37 random testpatterns.\n"
    "In general, a prime number of random patterns is best to _detect_\n"
    "errors.  Therefore 37 randoms are generated at startup, so you don't\n"
    "need to do this yourself.\n"
    "Once an error is detected, you might want to define your own patterns\n"
    "in order to locate and understand why the check-XXX command fails.\n"
  },
  { "random-patterns", "CNT",                 0, 2,       2,cmd_patterns_rnd,
    "Generate CNT random test patterns.  See description of 'patterns'\n"
    "command for more information.\n"
  },
  { "flash-plugin",    "ADR LEN FN [FN ...]", 0, 3, INT_MAX, cmd_flashplug,
    "Load and register target-assisted flash driver plugin(s) from\n"
    "file(s) FN.\n"
    "ADR and LEN define a memory region on the target that can be used as\n"
    "temporary memory to download driver and flash contents.\n"
  },
  { "flash",           "ADR [DRIVER]",        1, 2,       3, cmd_flash,
    "Autodetect flash chip(s) on ADR.  Currently only 29Fxxx and 49Fxxx\n"
    "types of chips are supported.\n"
    "Note that there is no dedicated command for the actual flash\n"
    "operation.  The usual memory write command 'load' will automatically\n"
    "call the correct driver for registered memory areas.\n"
    "Optionally pass driver as driver_magic of the known device at ADR.\n"
  },
  { "erase",           "BASE OFF",            1, 2,       3, cmd_erase,
    "Submit erase command to sector with OFF on flash chip at BASE.\n"
    "OFF is sector offset relative to the base of the chip.\n"
    "OFF==-1 indicates erase the whole chip.  For 29Fxxx and 49Fxxx types\n"
    "of chips multiple erase commands can be issued simultanously before\n"
    "the erase-wait command is issued.\n"
  },
  { "erase-wait",      "BASE",                1, 2,       2, cmd_erase_wait,
    "Wait for the flash chip on BASE to finish the issued erase operation.\n"
  },
  { "blank-chk",       "BASE OFF",            1, 2,       3, cmd_blank_chk,
    "perform a blank check on the chip at BASE.\n"
    "OFF is sector offset relative to the base of the chip.\n"
    "OFF==-1 indicates erase the whole chip.  Otherwise indicates page.\n"
  },
};

/* *INDENT-ON* */ 

/* search a command and execute it
 */
static void
do_command(size_t argc, char **argv)
{
  unsigned int i;

  for (i = 0; i < NUMOF (command); i++) {
    if (STREQ (argv[0], command[i].name) && command[i].func) {
      if (argc < command[i].min || argc > command[i].max)
        fatal ("Wrong number of arguments (%d) to \"%s\"\n", argc, argv[0]);
      if (command[i].need_device && !bdmIsOpen ())
        fatal ("Device must be specified before \"%s\" can be used\n",
               argv[0]);
      if (verbosity)
        printf ("%s: ", argv[0]);
      fflush (stdout);
      command[i].func (argc, argv);
      return;
    }
  }

  fatal ("%s: unknown command\n", argv[0]);
}

static void
usage (char *progname, char *fmt, ...)
{
  unsigned int i;

  va_list args;

  if (fmt) {
    va_start (args, fmt);
    vfprintf (stderr, fmt, args);
    va_end (args);
  }

  fprintf (stderr,
           "Usage: %s [options] [<script> [arguments [...]]]\n"
           " where options are one ore more of:\n"
           "   -h <cmd>   Get additional description for command <cmd>.\n"
           "   -d <0/1>   Output driver debug.\n"
           "   -v <level> Choose verbosity level (default=1).\n"
           "   -D <delay> Delay count for BDM clock generation (default=0).\n"
           "   -c <cmd>   Split <cmd> into args and execute resulting command.\n"
           "   -f         Turn warnings into fatal errors.\n"
           "\n" " available commands are:\n", progname);

  for (i = 0; i < NUMOF (command); i++) {
    fprintf (stderr, "   %s %s\n", command[i].name, command[i].args);
  }

  exit (EXIT_FAILURE);
}

static void
help_command (char *progname, char *cmd)
{
  unsigned int i;

  for (i = 0; i < NUMOF (command); i++) {
    if (STREQ (cmd, command[i].name)) {
      fprintf (stderr, "%s %s\n\n%s",
               command[i].name, command[i].args, command[i].helptext);
      exit (EXIT_SUCCESS);
    }
  }

  usage (progname, "unknown command '%s'.\n", cmd);
}

int
main (int argc, char *argv[])
{
  int opt, need_stdin = 1;

  progname = argv[0];

  srand (base_time = time(NULL));
  qsort (regnames, NUMOF (regnames), sizeof (regnames[0]), cmpreg);

  /* parse options
   */
  while ((opt = getopt (argc, argv, "fd:D:v:h:c:")) >= 0) {
    switch (opt) {
      case 'h':
        help_command (progname, optarg);
        break;
      case 'd':
        debug_driver = strtol (optarg, NULL, 10);
        break;
      case 'D':
        delay = strtol (optarg, NULL, 10);
        break;
      case 'v':
        verbosity = strtol (optarg, NULL, 10);
        break;
      case 'c':
        exec_line (optarg, 1, argv);
        need_stdin = 0;
        break;
      case 'f':
        fatal_errors = 1;
        break;
      default:
        usage (progname, NULL);
    }
  }

  if (optind < argc) {
    /* Non-option arguments are present, run "source" command on them
     */
    optind--;     /* fake argv[0] which normally would be "source" command */
    cmd_source (argc - optind, argv + optind);
  } else {
    /* No more args present. Read commands from stdin unless at least
       one command was already executed.
     */
    if (need_stdin) {
      char *av[] = { "source", NULL };
      cmd_source (1, av);
    }
  }

  clean_exit (EXIT_SUCCESS);
  exit (0);                              /* omit compiler warning */
}
