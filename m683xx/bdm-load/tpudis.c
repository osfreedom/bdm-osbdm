/*******************************************************************

  TPUDB Project to Create Free Motorola 683xx TPU Debugger

  tpudis.c	- TPU microcode disassembler

  (C) Copyright 2000 by Wouter Vlothuizen - Original Author
      e-mail:	wouter@vlothuizen.demon.nl

  This package can be copied and modified under 
  GNU General Public License with all its conditions.
  See file GPL for details. Under this license nobody can
  distribute this work and any derived work without full source code
  for all modules compiled and linked into executable.
 
 *******************************************************************/


#include <stdio.h>
#define TRUE 1
#define FALSE 0

static FILE *f;
static unsigned long inst;

void
Error (char *s)
{
  fprintf (stderr, "\ntpudis: %s\n", s);
  exit (1);
}


#define put(x) s=x
#define I(h,l) ( (inst>>(l)) & (((1<<((h)+1-(l))))-1) )

void
T1ABS (void)
{
  char *s=NULL;

  switch (I (28, 25))
    {
    case 0:
      put ("plow");
      break;
    case 1:
      put ("phi");
      break;
    case 2:
      put ("dec");
      break;
    case 3:
      put ("chan");
      break;
    case 4:
      put ("#0 special");
      break;
    case 5:
      put ("{Illegal T1ABS: 5}");
      break;
    case 6:
      put ("{Illegal T1ABS: 6}");
      break;
    case 7:
      put ("#0");
      break;
    case 8:
      put ("p");
      break;
    case 9:
      put ("a");
      break;
    case 10:
      put ("sr");
      break;
    case 11:
      put ("diob");
      break;
    case 12:
      put ("tcr1");
      break;
    case 13:
      put ("tcr2");
      break;
    case 14:
      put ("ert");
      break;
    case 15:
      put ("#0");
      break;
    default:
      Error ("T1ABS decoding error");
    }
  fprintf (f, "%s + ", s);
}


void
T3ABD (void)
{
  char *s=NULL;

  switch (inst >> 21 & 0xF)
    {
    case 0:
      put ("a");
      break;
    case 1:
      put ("sr");
      break;
    case 2:
      put ("ert");
      break;
    case 3:
      put ("diob");
      break;
    case 4:
      put ("phi");
      break;
    case 5:
      put ("{Illegal T3ABD: 5}");
      break;
    case 6:
      put ("plow");
      break;
    case 7:
      put ("p");
      break;
    case 8:
      put ("link");
      break;
    case 9:
      put ("chan");
      break;
    case 10:
      put ("dec");
      break;
    case 11:
      put ("dec&chan");
      break;
    case 12:
      put ("tcr1");
      break;
    case 13:
      put ("tcr2");
      break;
    case 14:
      put ("{Illegal T3ABD: 14}");
      break;
    case 15:
      put ("Nil");
      break;
    default:
      Error ("T3ABD decoding error");
    }
  fprintf (f, "AU %s :=", s);
}


void
SHF (void)
{
  char *s=NULL;

  switch (inst >> 19 & 3)
    {
    case 0:
      s = "<<";
      break;
    case 1:
      s = ">>";
      break;
    case 2:
      s = "R>";
      break;
    case 3:
      s = "";
      break;
    default:
      Error ("SHF decoding error");
    }
  fprintf (f, "%s ", s);
}


void
T1BBSetc (void)
{
  char *s=NULL;

  switch (inst >> 14 & 7)
    {
    case 0:
      s = "p";
      break;
    case 1:
      s = "a";
      break;
    case 2:
      s = "sr";
      break;
    case 3:
      s = "diob";
      break;
    case 7:
      s = "#0";
      break;
    case 4:
    case 5:
    case 6:
      s = "{Illegal T1BBS}";
      break;
    default:
      Error ("T1BBS decoding error");
    }
  fprintf (f, "%s%s%s ",
	   inst & (1L << 12) ? "" : "!", s, inst & (1L << 13) ? "" : " + 1");
}


void
SRCCCL (void)
{
  if (~inst & (1L << 18))
    fprintf (f, ", ensr");
  if (~inst & (1L << 17))
    fprintf (f, ", cc");
}


void
T1BBI (void)
{
  fprintf (f, "#$%2lx", inst >> 9 & 0xFF);
}


void
DECEND (void)
{
  char *s=NULL;

  switch (inst & 3)
    {
    case 0:
      s = "DEC_RTS";
      break;
    case 1:
      s = "DEC_RPT";
      break;
    case 2:
      s = "end";
      break;
    case 3:
      return;
    default:
      Error ("DECEND decoding error");
    }
  fprintf (f, "%s", s);
}


void
IOMetc (int rwPos)
{
  char *fm=NULL;
  int aid=0;

  switch (inst >> 9 & 7)
    {
    case 0:
      fm = "RAM p %s @prm%d ; ";
      aid = inst >> 2 & 7;
      break;
    case 1:
      fm = "RAM p %s by_diob ; ";
      break;
    case 2:
      fm = "RAM p %s @$%x ; ";
      aid = inst >> 2 & 0x7F;
      break;
    case 4:
      fm = "RAM diob %s @prm%d ; ";
      aid = inst >> 2 & 7;
      break;
    case 5:
      fm = "RAM diob %s by_diob ; ";
      break;
    case 6:
      fm = "RAM diob %s @$%x ; ";
      aid = inst >> 2 & 0x7F;
      break;
    case 3:
    case 7:
      return;
    default:
      Error ("IOM decoding error");

    }
  fprintf (f, fm, inst & (1L << rwPos) ? "->" : "<-", aid);
}


void
CJCetc (void)
{
  char *s=NULL;

  if (I (29, 16) == 0x3FFF)
    return;

  switch (inst >> 26 & 0xF)
    {
    case 0:
      s = "LESS_THAN";
      break;
    case 1:
      s = "LOW_SAME";
      break;
    case 2:
      s = "V";
      break;
    case 3:
      s = "N";
      break;
    case 4:
      s = "C";
      break;
    case 5:
      s = "Z";
      break;
    case 6:
      s = "cflg1";
      break;
    case 7:
      s = "cflg0";
      break;
    case 8:
      s = "TDL";
      break;
    case 9:
      s = "MRL";
      break;
    case 10:
      s = "LSL";
      break;
    case 11:
      s = "SEQ1";
      break;
    case 12:
      s = "SEQ0";
      break;
    case 13:
      s = "PSL";
      break;
    case 14:
      s = "{Illegal branch}";
      break;
    case 15:
      s = "false";
      break;
    default:
      Error ("CJC decoding error");
    }
  fprintf (f, "if %s = %s then goto $%lx%s; ",
	   s,
	   inst & (1L << 8) ? "true" : "false",
	   inst >> 16 & 0x1FF, inst & (1L << 25) ? "" : ", flsh");
}


void
NMAetc (void)
{
  char *s=NULL;

  switch (inst >> 26 & 3)
    {
    case 0:
      s = "jmp";
      break;
    case 1:
      s = "jsr";
      break;
    case 2:
      s = "rts";
      break;
    case 3:
      return;
    default:
      Error ("NMA decoding error");
    }
  fprintf (f, "%s%s $%lx ; ", s, inst & (1L << 25) ? "" : " ,flsh",
	   inst >> 16 & 0x1FF);
}

void
TBS (void)
{
  char *s=NULL;

  switch (I (15, 12))
    {
    case 0:
      s = "in_mtcr1_ctcr1";
      break;
    case 1:
      s = "in_mtcr2_ctcr1";
      break;
    case 2:
      s = "in_mtcr1_ctcr2";
      break;
    case 3:
      s = "in_mtcr2_ctcr2";
      break;
    case 4:
      s = "out_mtcr1_ctcr1";
      break;
    case 5:
      s = "out_mtcr2_ctcr1";
      break;
    case 6:
      s = "out_mtcr1_ctcr2";
      break;
    case 7:
      s = "out_mtcr2_ctcr2";
      break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      return;
    default:
      Error ("TBS decoding error");
    }
  fprintf (f, "tbs:=%s ", s);
}

void
ERWTDLMRL (void)
{
  if (~inst & (1L << 29))
    fprintf (f, "write_mer ");
  if (~inst & (1L << 18))
    fprintf (f, "neg_tdl ");
  if (~inst & (1L << 17))
    fprintf (f, "neg_mrl ");
}

void
PSC (void)
{
  char *s=NULL;

  switch (I (7, 6))
    {
    case 0:
      s = "PAC";
      break;
    case 1:
      s = "high";
      break;
    case 2:
      s = "low";
      break;
    case 3:
      return;
    default:
      Error ("PSC decoding error");
    }
  fprintf (f, "pin:=%s ", s);
}

void
PAC (void)
{
  char *s=NULL;

  switch (I (11, 9))
    {
    case 0:
      s = "off";
      break;
    case 1:
      s = "high";
      break;
    case 2:
      s = "low";
      break;
    case 3:
      s = "h+l";
      break;
    case 4:
    case 5:
    case 6:
    case 7:
      return;
    default:
      Error ("PAC decoding error");
    }
  fprintf (f, "pac:=%s ", s);
}

void
FLC (int pos)			/* at position 5 or 15 */
{
  char *s=NULL;

  fprintf (f, "chan ");
  switch (I (pos, pos - 2))
    {
    case 0:
      s = "clr cflg0";
      break;
    case 1:
      s = "set cflg0";
      break;
    case 2:
      s = "clr cflg1";
      break;
    case 3:
      s = "set cflg1";
      break;
    case 4:
    case 5:
    case 6:
    case 7:
      return;
    default:
      Error ("FLC decoding error");
    }
  fprintf (f, "%s ", s);
}

void
LSL (int pos)			/* at position 8 or 12 */
{
  if (~inst & (1L << pos))
    fprintf (f, "neg_lsr ");
}

void
CIR (void)
{
  if (~inst & (1L << 2))
    fprintf (f, "pir ");
}

void
CCM (void)
{
  if (~inst & (1L << 2))
    fprintf (f, "by_p ");
}

void
MTD (void)
{
  char *s=NULL;

  switch (I (1, 0))
    {
    case 0:
      s = "en";
      break;
    case 1:
      s = "ds";
      break;
    case 2:
    case 3:
      return;
    default:
      Error ("MTD decoding error");
    }
  fprintf (f, "%ssr ", s);
}

void
sep (void)
{
  fprintf (f, "; ");
}

void
DisInst (unsigned long i, FILE * fp)
{
  f = fp;
  inst = i;

  if (inst != -1)
    switch (inst >> 30 & 3)	/* type of instruction */
      {
      case 0:			/* format 1 */
	T3ABD ();
	SHF ();
	T1ABS ();
	T1BBSetc ();
	SRCCCL ();
	sep ();
	IOMetc (29);
	DECEND ();
	break;
      case 1:			/* format 2 */
	T3ABD ();
	SHF ();
	T1ABS ();
	T1BBSetc ();
	sep ();
	FLC (5);
	ERWTDLMRL ();
	PAC ();
	LSL (8);
	PSC ();
	CIR ();
	sep ();
	DECEND ();
	break;
      case 2:			/* format 3 */
	CJCetc ();
	FLC (5);
	TBS ();
	PAC ();
	PSC ();
	CCM ();
	MTD ();
	break;
      case 3:
	if (inst & (1L << 29))	/* format 5 */
	  {
	    T3ABD ();
	    SHF ();
	    T1ABS ();
	    T1BBI ();
	    SRCCCL ();
	    sep ();
	    FLC (5);
	    LSL (8);
	    CIR ();
	    sep ();
	  }
	else			/* format 4 */
	  {
	    NMAetc ();
	    FLC (15);
	    LSL (12);
	    sep ();
	    IOMetc (28);
	  }
	DECEND ();
	break;
      default:
	Error ("format decoding error");
      }
  fprintf (f, "\n");
}

#if 0
void
EntryPoint (unsigned int ep)
{
  int pc;

  pc = ep & 0x1FF;

  if (ep >> 9 & 3 == 0)
    {
      fprintf (f, "Error in EntryPoint format\n");
    }
  else
    {
      fprintf (f, "PreLoad=%d  ME=%d  PPD=%4s  PC=$%03x\n",
	       ep >> 13 & 7,
	       ep & (1L << 12) ? 1 : 0, ep & (1L << 11) ? "DIOB" : "P", pc);
      epCnt[pc]++;
    }
}

#endif
