/*
    Turbo BDM Light ColdFire
    Copyright (C) 2008 Chris Johns
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "BDMlib.h"

#include "bdmusb.h"
#include "tblcf/tblcf.h"
#include "tblcf/tblcf_usb.h"

int
main (int argc, char *argv[])
{
  extern char *optarg;
  extern int optind, optopt;
  int devs;
  int i;
  int c;
  int debug = 0;
  char name[128];

  printf ("BDMUSB Show\n\n");

  while ((c = getopt(argc, argv, "d:")) != -1)
  {
    switch (c)
    {
      case 'd':
        debug = strtoul (optarg, 0, 0);
        break;

      case '?':
        fprintf (stderr, "Unrecognized option: -%c\n", optopt);
        exit (1);
        break;
    }
  }

  bdmSetDebugFlag (debug);
  
  devs = bdmusb_init();
  printf ("Found %i device(s)\n", devs);

  for (i = 0; i < devs; i++)
  {
    bdmusb_dev_name (i, name, sizeof (name));
    printf (" %2d: %s\n", i + 1, name);
  }
  
  return 0;
}
