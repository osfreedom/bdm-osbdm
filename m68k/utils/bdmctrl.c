/* $Id: bdmctrl.c,v 1.8 2003/11/26 20:34:31 joewolf Exp $
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

# include <ctype.h>
# include <limits.h>
# include <errno.h>
# include <sys/types.h>
# include <stdarg.h>
# include <stdlib.h>
# include <stdio.h>
# include <unistd.h>
# include <string.h>
# include <time.h>

# include <bfd.h>

# include "BDMlib.h"

static void do_command (size_t argc, char **argv);

# ifndef STREQ
# define STREQ(a,b) (!strcmp((a),(b)))
# endif

# ifndef NUMOF
# define NUMOF(ary) (sizeof(ary)/sizeof(ary[0]))
# endif

static int verify;
static int verbosity=1;
static int fatal_errors=0;
static char *progname;
static int cpu_type;
static time_t base_time; /* when bdmctrl was started */

static unsigned int patcnt=0; /* number of test patterns */
static unsigned long *pattern=NULL;

typedef struct {
    char *name;
    char *val;
} var_t;
static var_t *var=NULL;
static int num_vars=0;


static int bfd_cnt=0;
static bfd **bfd_ptr=NULL;

/* define list of known architectures/subarchitectures
 */

typedef struct {
    enum bfd_architecture arch;
    int machcnt;
    const unsigned long *mach;
} machlist_t;

typedef struct {
    int cpu;
    int listcnt;
    const machlist_t *machlist;
} arch_t;

static const unsigned long mach_unknown[] = { 0 };
static const unsigned long mach_m68k[] = {
    bfd_mach_m68000, bfd_mach_m68008, bfd_mach_m68010, bfd_mach_m68020,
    bfd_mach_m68030, bfd_mach_m68040, bfd_mach_m68060, bfd_mach_cpu32,
};
static const unsigned long mach_cf[] = {
    bfd_mach_mcf5200, bfd_mach_mcf5206e, bfd_mach_mcf5307, bfd_mach_mcf5407,
};

static const machlist_t machlist_cpu32[] = {
/*    { bfd_arch_unknown, NUMOF(mach_unknown), mach_unknown }, */
    { bfd_arch_m68k,    NUMOF(mach_m68k),    mach_m68k },
};

static const machlist_t machlist_coldfire[] = {
/*    { bfd_arch_unknown, NUMOF(mach_unknown), mach_unknown }, */
    { bfd_arch_m68k,    NUMOF(mach_cf),      mach_cf },
};

static arch_t arch[] = {
    { BDM_CPU32,    NUMOF(machlist_cpu32),    machlist_cpu32 },
    { BDM_COLDFIRE, NUMOF(machlist_coldfire), machlist_coldfire },
};



static void usage(char *progname, char *fmt, ...)
{
    va_list args;

    if (fmt) {
	va_start (args, fmt);
	vfprintf (stderr, fmt, args);
	va_end (args);
    }

    fprintf(stderr,
	    "Usage: %s [options] <device> [<command> [arguments [...]]]\n"
	    " where options are one ore more of:\n"
	    "   -d <level> (default=0) Choose driver debug level.\n"
	    "   -v <level> (default=1) Choose verbosity level.\n"
	    "   -D <delay> (default=0) Delay count for BDM clock generation.\n"
	    "   -f                     turn warnings into fatal errors\n",
	    progname);
    exit(EXIT_FAILURE);
}

static void clean_exit (int exit_value)
{
    int i;

    for (i=0; i<bfd_cnt; i++)
	bfd_close (bfd_ptr[i]);

    if (bdmIsOpen()) {
	bdmSetDriverDebugFlag(0);
	bdmClose();
    }

    exit (exit_value);
}

static void fatal (char *fmt, ...)
{
    va_list args;

    va_start (args, fmt);
    vfprintf (stderr, fmt, args);
    va_end (args);

    clean_exit (EXIT_FAILURE);
}

static void warn (char *fmt, ...)
{
    va_list args;

    va_start (args, fmt);
    vfprintf (stderr, fmt, args);
    va_end (args);

    if (fatal_errors) clean_exit (EXIT_FAILURE);
}

static void set_var (const char *name, const char *val)
{
    int i;
    char *v;

    if (!(v=strdup (val))) fatal ("Out of memory\n");

    for (i=0; i<num_vars; i++) {
	if (STREQ (var[i].name, name)) {
	    free (var[i].val);
	    var[i].val = v;
	    return;
	}
    }

    if (!(var = realloc (var, (num_vars+1) * sizeof *var)) ||
	!(var[num_vars].name = strdup (name)))
	fatal ("Out of memory\n");

    var[num_vars++].val = v;
}

static char *get_var (const char *name)
{
    int i;

    for (i=0; i<num_vars; i++) {
	if (STREQ (var[i].name, name)) {
	    return var[i].val;
	}
    }

    fatal ("Variable %s note defined\n", name);
}

/* Duplicate a string with (rudimentary) variable substitution.
 */
static char *varstrdup (const char *instr, size_t argc, char **vars)
{
    int dlen=0;
    size_t varnum=0;
    char *dst, *str, *rstr, *s, *p;

    rstr = str = strdup (instr); /* make str writable */
    dst = malloc (10);           /* make sure we return a string */

    if (!str || !dst) return NULL;

    *dst=0;

    while (*str) {
	s = str;
	if ((p=strchr (s, '$'))) {
	    *p++ = 0;
	    if (isdigit (*p)) {
		varnum = strtol (p, &str, 0);
		if (varnum<1 || varnum>=argc)
		    fatal ("argument $%d not available\n", varnum);
		p = vars[varnum];
	    } else {
		p = get_var (p);
	    }
	}
	while (s) {
	    int slen = strlen (s);
	    if (s==str) str += slen;
	    if (!(dst = realloc (dst, dlen + slen + 1))) return NULL;
	    strcpy (dst+dlen, s);
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
static size_t build_argv (const char *line, char **argv[],
			  size_t glbl_argc, char *glbl_argv[])
{
    char *linedup, *p;
    size_t i, tnum=0;

    /* Make sure realloc() will work properly
     */
    *argv=NULL;

    /* Modifying strings that we get passed is considered harmful. Therefore
       we make a copy to mess around with.
    */
    if (!(p=linedup=strdup(line))) fatal ("Out of memory\n");

    /* Split the line
     */
    while (1) {
	/* Allocate space for pointer to new argument. While we're at it,
	   we allocate space for trailing NULL pointer, too.
	 */
	if (!(*argv=realloc(*argv, (tnum+2) * sizeof(**argv))))
	    fatal ("Out of memory\n");

	/* Skip leading whitespace. Terminate at end of line.
	 */
	while (*p && strchr (" \t", *p))
	    p++;
	if (!*p) break;

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
    for (i=0; i<tnum; i++) {
	if (!((*argv)[i]=varstrdup((*argv)[i], glbl_argc, glbl_argv)))
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
typedef struct {
    char *regname;
    int regnum;
    int issystem;
} regname_t;

# define REGNAM(nam, issystem) { #nam, BDM_REG_##nam, issystem }

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

# undef REGNAM

/* comparison function for regname_t
 */
static int cmpreg (const void *ap, const void *bp)
{
    return strcmp (((const regname_t *)ap)->regname,
		   ((const regname_t *)bp)->regname);
}

/* Find a register by its name. If found, provide the nuber of the register
   and whether it is a system-register or not.
*/
static int find_register (const char *regname, int *regnum, int *issystem)
{
    regname_t *found, key;
    const char *s=regname;
    char name[20];
    char *d=name;

    if (*s == '%') s++;

    /* make uppercase copy of the name
     */
    while (d-name<((int)sizeof(name)) && (*d++=toupper(*s++)))
	;
    name[sizeof(name)-1] = 0;

    key.regname = name;
    if ((found=bsearch (&key, regnames, NUMOF(regnames),
			sizeof(regnames[0]), cmpreg))) {
	*regnum   = found->regnum;
	*issystem = found->issystem;
	return 1;
    }

    return 0;
}

/* Read from a register by name
 */
static void read_register (const char *regname, unsigned long *val)
{
    int issystem;
    int regnum;

    if (!find_register (regname, &regnum, &issystem))
	fatal ("No such register \"%s\"\n", regname);

    if (issystem) {
	if (bdmReadSystemRegister(regnum, val) >= 0) return;
    } else {
	if (bdmReadRegister(regnum, val) >= 0) return;
    }

    fatal ("Can't read register \"%s\": %s\n", regname, bdmErrorString());
}

/* Write into a register by name. FIXME: should use a size parameter to
   distinguish between 8/16/32 bit writes.
 */
static void write_register (const char *regname, unsigned long val)
{
    int issystem;
    int regnum;

    if (!find_register (regname, &regnum, &issystem))
	fatal ("No such register \"%s\"\n", regname);

    if (issystem) {
	if (bdmWriteSystemRegister(regnum, val) >= 0) return;
    } else {
	if (bdmWriteRegister(regnum, val) >= 0) return;
    }

    fatal ("Can't write 0x%08x into register \"%s\": %s\n",
	   val, regname, bdmErrorString());
}

/* read values of one, two or four bytes into a long
 */
static int read_value (unsigned long adr, unsigned long *val, char size)
{
    int ret=-1;
    unsigned char char_val;
    unsigned short short_val;

    switch (size) {
	case '1': case  1: case 'b': case 'B':
	    ret = bdmReadByte     (adr, &char_val);
	    *val = char_val;
	    size = 1;
	    break;
	case '2': case  2: case 'w': case 'W':
	    ret = bdmReadWord     (adr, &short_val);
	    *val = short_val;
	    size = 2;
	    break;
	case '4': case  4: case 'l': case 'L':
	    ret = bdmReadLongWord (adr, val);
	    size = 4;
	    break;
	default: fatal ("Unknown size spezifier '%c'\n", size);
    }

    if (ret<0)
	fatal ("read_value(0x%08x,xxx,%d): %s\n", adr, size, bdmErrorString());

    return size;
}

/* write values of one, two or four bytes
 */
static int write_value (unsigned long adr, unsigned long val, char size)
{
    int ret=-1;

    switch (size) {
	case '1': case  1: case 'b': case 'B':
	    ret = bdmWriteByte     (adr, val);
	    size = 1;
	    break;
	case '2': case  2: case 'w': case 'W':
	    ret = bdmWriteWord     (adr, val);
	    size = 2;
	    break;
	case '4': case  4: case 'l': case 'L':
	    ret = bdmWriteLongWord (adr, val);
	    size = 4;
	    break;
	default: fatal ("Unknown size spezifier '%c'\n", size);
    }

    if (ret<0)
	fatal ("write_value(0x%08x,0x%x,%d): %s\n",
	       adr, val, size, bdmErrorString());

    return size;
}

/* Write a buffer to the target.
 */
unsigned long write_memory (unsigned long adr, unsigned char *buf,
			    unsigned long cnt)
{
    if (bdmWriteMemory (adr, buf, cnt) < 0)
	fatal ("\nCan not download 0x%08x bytes to addr 0x%08lx: %s\n",
	       cnt, adr, bdmErrorString());
    return cnt;
}

/* Read a buffer from the target.
 */
static void read_memory (unsigned long adr, unsigned char *buf,
			 unsigned long cnt)
{
    if (bdmReadMemory (adr, buf, cnt) < 0)
	fatal ("\nCan not upload 0x%08x bytes from addr 0x%08lx: %s\n",
	       cnt, adr, bdmErrorString());
}

/* Evaluate (rudimentary) a string. For now, no real evaluation is done.
   Only numbers, registers and symbols can be distinguished.
 */
static unsigned long eval_string (char *val)
{
    long i, j, sc, symcnt;
    unsigned long v;
    asymbol **symtab=NULL;
    bfd *abfd;

    if (isdigit (*val)) return strtoul (val, NULL, 0);

    if (*val=='%') {
	read_register (val, &v);
	return v;
    }

    /* FIXME: should use bfd_perror() bfd_errmsg()
     */
    /* search opened bfd's for the symbol. The bfd's are searched in reverse
       order, so loading a new bfd will override symbols with same name from
       bfd's that were loaded before.
     */
    for (i=0; i<bfd_cnt; i++) {
	abfd = bfd_ptr[bfd_cnt-i-1];

	if ((sc=bfd_get_symtab_upper_bound(abfd))<=0 || !(symtab=malloc(sc))) {
	    warn ("Can't get symbol table for \"%s\"\n", abfd);
	    continue;
	}

	if ((symcnt = bfd_canonicalize_symtab (abfd, symtab))<0)
	    warn ("Can't canonicalize symbol table in \"%s\"\n", abfd);
	
	for (j=0; j<symcnt; j++) {
	    if (STREQ (val, symtab[j]->name)) {
		return symtab[j]->section->vma + symtab[j]->value;
	    }
	}
	free (symtab);
    }

    fatal ("Can't find symbol \"%s\"\n", val);
    return 0; /* omit compiler warning */
}

/* check one register
 */
static void check_one_register (char *regname)
{
    unsigned int i;
    unsigned long reg_value;
    unsigned long orig_value;

    read_register (regname, &orig_value); /* read original value */

    for (i=0; i<patcnt; i++) {
	write_register (regname, pattern[i]);
	read_register (regname, &reg_value);
	if (reg_value != pattern[i])
	    warn ("register %s written 0x%08x, read back 0x%08x\n");
    }

    write_register (regname, orig_value); /* restore original value */

    if (verbosity) printf (" %s", regname);
}

/* load a bfd section into target
 */
static void load_section (bfd *abfd, sec_ptr sec, PTR section_names)
{
    flagword flags;
    char **sec_names = section_names;
    unsigned long off=0;
    unsigned long addr;
    unsigned long length;
    unsigned long dfc;
    unsigned char buf[1024], rbuf[1024];

    /* FIXME: should use bfd_perror() bfd_errmsg()
     */
    flags  = bfd_get_section_flags (abfd, sec);
    addr   = bfd_section_lma (abfd, sec);
    length = bfd_get_section_size_before_reloc (sec);

    if (sec_names[0])
	for (off=0; sec_names[off]; off++)
	    if (STREQ (sec_names[off], sec->name))
		break;

    if (verbosity) {
	printf (" fl:0x%08x %10s 0x%08lx..0x%08lx (0x%08lx) 0x%08x:   ",
		flags, sec->name, addr, addr+length, length, off);
	fflush (stdout);
    }

    if ((flags & SEC_LOAD) && (flags & SEC_HAS_CONTENTS) && (!off || sec_names[off])) {
	read_register ("dfc", &dfc);
	write_register ("dfc", flags & SEC_CODE ? 6 : 5);

	for (off=0; off<length; off+=sizeof(buf)) {
	    unsigned int cnt = length-off;

	    if (cnt > sizeof(buf)) cnt = sizeof(buf);

	    if (!bfd_get_section_contents (abfd, sec, buf, off, cnt)) {
		warn ("\nCan not read contents for section %s\n", sec->name);
		continue;
	    }

	    write_memory (addr+off, buf, cnt);

	    if (verify) {
		read_memory (addr+off, rbuf, cnt);
		if (memcmp (buf, rbuf, cnt))
		    warn ("\nRead back contents from 0x%08lx don't match\n",
			   addr+off);
	    }

	    if (verbosity) {
		printf ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b0x%08x: OK", off+cnt);
		fflush (stdout);
	    }
	}

	write_register ("dfc", dfc);

	if (verbosity) printf ("\b\bOK\n");
    } else {
	if (verbosity) printf ("\b\bSkip\n");
    }
}

/* invert portions of a longword and check whether the correct portions
   were actually inverted.
 */ 
static void check_alignment (unsigned long adr, char size)
{
    unsigned i;
    unsigned long readback;
    unsigned long mask=0x0ff;
    unsigned long off=0;
    unsigned long pat=pattern[0];

    switch (size) {
	case 1: case '1': case 'B': case 'b': off=3; mask=0x0ff;       break;
	case 2: case '2': case 'W': case 'w': off=2; mask=0x0ffff;     break;
	case 4: case '4': case 'L': case 'l': off=0; mask=0x0ffffffff; break;
	default: fatal ("Unknown size specifier %c\n", size);
    }

    /* write pattern
     */
    write_value(adr, pat, 4);

    for (i=0; i<4; i+=size) {
	pat ^= (mask << (i*8));

	write_value (adr+off-i, (pat >> (i*8)) & mask, size);

	read_value (adr, &readback, 4);

	if (readback != pat)
	    warn (" Readback failed at 0x%08lx expect 0x%08lx read 0x%08lx\n",
		  adr, pat, readback);
    }
}

/* Here we're done with the helper functions. Define the real command
   functions now
 */

/* check registers
 */
static void cmd_check_register (size_t argc, char **argv)
{
    unsigned int i;

    if (verbosity) printf ("\n");

    for (i=1; i<argc; i++)
	check_one_register (argv[i]);

    if (verbosity) printf (" OK\n");
}

/* dump register
 */
static void cmd_dump_register (size_t argc, char **argv)
{
    unsigned int i;
    unsigned long reg_value;

    if (verbosity) printf ("\n");

    for (i=1; i<argc; i++) {
	read_register (argv[i], &reg_value);
	printf (" %7s: 0x%08lx\n", argv[i], reg_value);
    }

    if (verbosity) printf (" OK\n");
}

/* write a value to a specific address
 */
static void cmd_write (size_t argc, char **argv)
{
    char *dst;
    unsigned long val;

    dst = argv[1];
    val = eval_string (argv[2]);

    if (verbosity) {
	printf ("0x%08lx to %s", val, dst);
	fflush (stdout);
    }

    if (*dst=='%') {
	write_register (dst, val);
    } else {
	write_value (eval_string(dst), val, argv[3][0]);
    }

    if (verbosity) printf (" OK\n");
}

/* dump a memory region
 */
static void cmd_dump_mem (size_t argc, char **argv)
{
    unsigned long i;
    unsigned long src;
    unsigned long val;
    unsigned long len;
    int rlen;
    FILE *file=stdout;

    src = eval_string (argv[1]);
    len = eval_string (argv[2]);

    if (argc==5 && !(file = fopen (argv[4], "w")))
	fatal ("Can't open \"%s\":%s\n", argv[4], strerror(errno));

    for (i=0; i<len; i+=rlen) {
	if (!(i%16)) fprintf (file, "\n 0x%08lx: ", src+i);
	rlen = read_value (src+i, &val, argv[3][0]);
	fprintf (file, " 0x%0*lx", 2*rlen, val);
	fflush (file);
    }

    if (file != stdout) fclose (file);

    printf ("\nOK\n");
}

/* check a memory region
 */
static void cmd_check_mem (size_t argc, char **argv)
{
    unsigned long base;
    unsigned long size;
    unsigned long off;
    unsigned long *wbuf;
    unsigned long *rbuf;
    unsigned char *sav;

    base = eval_string (argv[1]);
    size = eval_string (argv[2]);

    sav  = malloc (size+4);
    wbuf = malloc (size+4);
    rbuf = malloc (size+4);
    
    if (!sav || !wbuf || !rbuf) fatal ("Out of memory\n");

    /* Fill buffer with a known pattern.
     */
    for (off=0; off<size/4; off++) {
	wbuf[off] = pattern[off % patcnt];
    }

    read_memory (base, sav, size);           /* save original contents */
    write_memory (base, (unsigned char *)wbuf, size); /* write pattern */
    read_memory (base, (unsigned char *)rbuf, size);  /* read it back */

    if (memcmp (wbuf, rbuf, size))
	warn (" Readback failed at 0x%08lx\n", base);

    for (off=0; off<size; off+=4) {
	check_alignment (base+off, 'b');
	check_alignment (base+off, 'w');
    }

    write_memory (base, sav, size); /* restore original contents */

    free (sav);
    free (wbuf);
    free (rbuf);

    if (verbosity) printf ("OK\n");
}

/* load binary into target
 */
static void cmd_load (size_t argc, char **argv)
{
    int c, a, m;
    bfd *abfd;
    static int need_init=1;
    unsigned long entry_addr=0;
    enum bfd_architecture cur_arch;
    unsigned long cur_mach;

    if (verbosity) printf ("\n");

    verify = 0;
    if (STREQ (argv[1], "-v")) {
	verify=1;
	argv++;
    }

    if (argc < 2) fatal ("Wrong number of arguments\n");

    if (need_init) {
	bfd_init ();
	need_init = 0;
    }

    if (!(abfd = bfd_openr (argv[1], NULL))) {
	warn ("BFD can not open %s: %s\n", argv[1], strerror (errno));
	return;
    }

    /* FIXME: loading a file multiple times should not open it again. Only
       the search order for the symbols should be adjusted. The question is
       how to distinguish foo/bar from foo/baz/../bar. On unix, dev/inode
       could be used for this. But how to do it on windows?
     */
    if (!(bfd_ptr=realloc(bfd_ptr, (bfd_cnt+1)*sizeof(*bfd_ptr))))
	fatal ("Out of memory\n");
    bfd_ptr[bfd_cnt++] = abfd;

    if (!bfd_check_format (abfd, bfd_object)) {
	warn ("%s is not an object file, skip it\n", argv[1]);
    }

    cur_arch = bfd_get_arch (abfd);
    cur_mach = bfd_get_mach (abfd);

    /* check whether we can find known CPU/arch/mach combination
     */
    for (c=0; c<NUMOF(arch); c++) {
	if (arch[c].cpu != cpu_type) continue;
	for (a=0; a<arch[c].listcnt; a++) {
	    if (arch[c].machlist[a].arch != cur_arch) continue;
	    for (m=0; m<arch[c].machlist[a].machcnt; m++) {
		if (arch[c].machlist[a].mach[m] != cur_mach) continue;
		break;
	    }
	    break;
	}
	break;
    }

    if (c >= NUMOF(arch) ||
	a >= arch[c].listcnt ||
	m >= arch[c].machlist[a].machcnt)
    {
	warn ("%s: Architecture (%s) don't match CPU %d\n",
	      argv[1], bfd_printable_name (abfd), cpu_type);
    }

    bfd_map_over_sections (abfd, load_section, argv+2);

    write_register ("rpc", entry_addr=bfd_get_start_address (abfd));

    if (verbosity) printf ("default entry address: 0x%08lx\n", entry_addr);
}

/* execute code withhin target
 */
static void cmd_execute (size_t argc, char **argv)
{
    unsigned long addr;

    if (argc>=2) write_register ("rpc", eval_string (argv[1]));

    read_register ("rpc", &addr);

    if (verbosity) printf ("Run at 0x%08lx\n", addr);

    if (bdmGo () < 0) fatal ("Can not run the taget at 0x%08lx\n", addr);
}

/* Generate random test patterns for register/memory checks. We use only
   bits 3..18 of the generated numbers since the lower bits are less random
   and bit 31 would always be zero on a typical host.
 */
static void cmd_patterns (size_t argc, char *argv[])
{
    unsigned int i;

    if (argc==3 && STREQ (argv[1], "random")) {
	argc = strtoul (argv[2], NULL, 0);
	argv = 0;
    }

    if (!(pattern = realloc (pattern, argc*sizeof(*pattern))))
	fatal ("Out of memory\n");

    if (argv) {
	argc--;
	for (i=0; i<argc; i++) {
	    pattern[i] = eval_string (argv[i+1]);
	}
    } else {
	for (i=0; i<2*argc; i++) {
	    ((unsigned short*)pattern)[i] = (rand() >> 3) & 0x0ffff;
	}
    }

    patcnt = argc;

    if (verbosity) {
	printf ("\n");
	for (i=0; i<patcnt; i++) {
	    printf (" pattern %d: 0x%08lx\n", i, pattern[i]);
	}
    }
}

/* set a variable
 */
static void cmd_set (size_t argc, char *argv[])
{
    set_var (argv[1], argv[2]);

    if (verbosity) printf ("OK\n");
}

/* read a line from stdin and assign arguments to variables
 */
static void cmd_read (size_t argc, char *argv[])
{
    int i, ac;
    char *p, **av, buf[1024];

    if (!fgets (buf, 1020, stdin)) fatal ("Can't read stdin\n");
    buf[1020]=0;
    if ((p=strchr(buf, '\n'))) *p=0;

    ac = build_argv (buf, &av, 0, NULL);
    if (ac) {
	for (i=1; i<argc; i++) {
	    if (i>ac) fatal ("Not enough arguments specified\n");
	    set_var (argv[i], av[i-1]);
	    free (av[i-1]);
	}
	free (av);
    }


    if (verbosity) printf ("OK\n");
}

/* exit bdmctrl
 */
static void cmd_exit (size_t argc, char *argv[])
{
    if (verbosity) printf ("OK\n");
    clean_exit (EXIT_SUCCESS);
}

/* reset the target
 */
static void cmd_reset (size_t argc, char *argv[])
{
    if (bdmReset() < 0) fatal ("bdmReset(): %s\n", bdmErrorString());

    if (verbosity) printf ("OK\n");
}

/* Wait until target is stopped or hlated
 */
static void cmd_wait (size_t argc, char **argv)
{
    while (!((bdmStatus ()) & (BDM_TARGETSTOPPED|BDM_TARGETHALT))) {
	sleep (1);
    }

    if (verbosity) printf ("OK\n");
}

/* print out seconds since start
 */
static void cmd_time (size_t argc, char **argv)
{
    printf (" %ld seconds\n", time(NULL) - base_time);
}

/* sleep for a while
 */
static void cmd_sleep (size_t argc, char **argv)
{
    sleep (strtoul (argv[1], NULL, 0));
}

/* output a line of text
 */
static void cmd_echo (size_t argc, char **argv)
{
    int i;

    for (i=1; i<argc; i++)
	printf ("%s%s", i>1 ? " " : "", argv[i]);
    printf ("\n");
}

/* read commands from a file and execute them
 */
static void cmd_source (size_t argc, char **argv)
{
    char buf[1024];
    FILE *file=stdin;

    if (argc>=2 && !(file=fopen(argv[1], "r")))
	fatal ("%s: %s: %s\n", progname, argv[1], strerror(errno));

    while (fgets (buf, 1020, file)) {
	unsigned int i, ac;
	char *p, **av;
	buf[1020]=0;
	if ((p=strchr(buf, '\n'))) *p=0;
	if ((p=strchr(buf, '#'))) *p=0;
	ac = build_argv (buf, &av, argc-1, argv+1);

	if (ac) {
	    do_command (ac, av);

	    for (i=0; i<ac; i++)
		free (av[i]);
	    free (av);
	}
    }

    fclose (file);
}

/* Known commands and their implementations.
 */
static struct command_s {
    char *name;
    size_t min, max; /* number of arguments (inclusive command by itself) */
    void (*func)(size_t, char**);
} command[] = {
    { "reset",          1,       1, cmd_reset },
    { "check-register", 2, INT_MAX, cmd_check_register },
    { "dump-register",  2, INT_MAX, cmd_dump_register },
    { "check-mem",      3,       3, cmd_check_mem },
    { "dump-mem",       4,       5, cmd_dump_mem },
    { "write",          4,       4, cmd_write },
    { "load",           2, INT_MAX, cmd_load },
    { "execute",        1,       2, cmd_execute },
    { "set",            3,       3, cmd_set },
    { "read",           1, INT_MAX, cmd_read },
    { "sleep",          2,       2, cmd_sleep },
    { "wait",           1,       1, cmd_wait },
    { "time",           1,       1, cmd_time },
    { "echo",           1, INT_MAX, cmd_echo },
    { "exit",           1,       1, cmd_exit },
    { "source",         1, INT_MAX, cmd_source },
    { "patterns",       2, INT_MAX, cmd_patterns },
};

/* search a command and execute it
 */
static void do_command (size_t argc, char **argv)
{
    unsigned int i;

    for (i=0; i<NUMOF(command); i++) {
	if (STREQ (argv[0], command[i].name) && command[i].func) {
	    if (argc<command[i].min || argc>command[i].max)
		fatal ("Wrong number of arguments to \"%s\"\n", argv[0]);
	    if (verbosity) printf ("%s: ", argv[0]);
	    fflush (stdout);
	    command[i].func (argc, argv);
	    return;
	}
    }

    fatal ("%s: unknwn command\n", argv[0]);
}

int main (int argc, char *argv[])
{
    int opt;
    char *dev;
    int delay=0;
    int debug_driver=0;

    progname = argv[0];

    /* parse options
     */
    while ((opt=getopt (argc, argv, "fd:D:v:")) >= 0) {
	switch (opt) {
	    case 'd': debug_driver = strtol (optarg, NULL, 10); break;
	    case 'D': delay        = strtol (optarg, NULL, 10); break;
	    case 'v': verbosity    = strtol (optarg, NULL, 10); break;
	    case 'f': fatal_errors = 1; break;
	    default:  usage (progname, NULL);
	}
    }
    if (argc<=optind) usage (progname, "missing mandatory option\n");

    /* get device name
     */
    dev=argv[optind++];

    /* open BDM port and set basic options
     */
    if (bdmOpen(dev)<0)
	fatal ("bdmOpen(\"%s\"): %s\n", dev, bdmErrorString());

    if (debug_driver)              bdmSetDriverDebugFlag(debug_driver);
    if (delay)                     bdmSetDelay(delay);

    srand (base_time = time (NULL));
    cmd_patterns (37, NULL);  /* generate a prime number of test patterns */
    qsort (regnames, NUMOF(regnames), sizeof(regnames[0]), cmpreg);

    /* print information we can retrieve from driver
     */
    if (verbosity) {
	unsigned int driver_version;
	int iface;

	if (bdmGetDrvVersion(&driver_version) < 0)
	    fatal ("bdmGetDrvVersion(): %s\n", bdmErrorString());

	printf("BDM Driver Version: %x.%x\n", 
	       driver_version >> 8, driver_version & 0xff);

	if (bdmGetProcessor(&cpu_type) < 0)
	    fatal ("bdmGetProcessor(): %s\n", bdmErrorString());

	switch(cpu_type) {
	    case BDM_CPU32:    printf("Processor: CPU32\n");    break;
	    case BDM_COLDFIRE: printf("Processor: Coldfire\n"); break;
	    default:
		fatal ("Unsupported processor type %d!\n", cpu_type);
	}

	if (bdmGetInterface(&iface) < 0)
	    fatal ("bdmGetInterface(): %s", bdmErrorString());

	switch(iface) {
	    case BDM_COLDFIRE_PE: printf("Interface: P&E Coldfire\n");   break;
	    case BDM_CPU32_PD:   printf("Interface: Eric's CPU32\n");    break;
	    case BDM_CPU32_ICD:  printf("Interface: ICD (P&E) CPU32\n"); break;
	    default:
		fatal ("Unknown or unsupported interface type %d!\n", iface);
	}
    }

    /* do the real work now
     */
    if (optind<argc) {
	do_command (argc-optind, argv+optind);
    } else {
	char *av[]={"source", NULL};
	cmd_source (1, av);
    }

    clean_exit (EXIT_SUCCESS);
    exit (0);      /* omit compiler warning */
}
