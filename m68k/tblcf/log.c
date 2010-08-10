/*
    Turbo BDM Light ColdFire
    Copyright (C) 2006  Daniel Malik

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

#include <stdio.h>
#include <string.h>

#include "log.h"

void tblcf_print_dump(unsigned char *data, unsigned int size) {
  static char buf[256];
  char *p = buf;
  unsigned int i=0;
  while(size--) {
    p += sprintf(p,"%02X ",*(data++));
    if (((++i)%32)==0) {
      tblcf_print("%s\n", buf);
      p = buf;
    }
    else if (((i)%8)==0) {
      p = strcat (p, " ");
    }
	}
  if (((i)%32)!=0) tblcf_print("%s\n", buf);
}
