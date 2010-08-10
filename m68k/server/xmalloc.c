/* xmalloc.c -- malloc with out of memory checking
   Copyright (C) 1990, 1991 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <config.h> 
#include <syslog.h> 
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
char *malloc ();
char *realloc ();
void free ();
void abort();
#endif

/* Allocate N bytes of memory dynamically, with error checking.  */

char *
xmalloc (n)
     unsigned n;
{
  char *p;

  if (n == 0)
    return 0;
  
  p = malloc (n);
  if (p == 0)
    {
      syslog (LOG_INFO, "xmalloc: request for %d bytes failed\n", n);
      abort ();
    }
  return p;
}

/* Change the size of an allocated block of memory P to N bytes,
   with error checking.
   If P is NULL, run xmalloc.
   If N is 0, run free and return NULL.  */

char *
xrealloc (p, n)
     char *p;
     unsigned n;
{
  if (p == 0)
    return xmalloc (n);
  if (n == 0)
    {
      free (p);
      return 0;
    }
  p = realloc (p, n);
  if (p == 0)
    {
      syslog (LOG_INFO, "xrealloc: request for %d bytes failed\n", n);
      abort ();
    }
  return p;
}

void
xfree (p)
     char *p;
{
  if (p != 0)
    free (p);
}
