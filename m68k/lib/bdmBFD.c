/*
 * Motorola Background Debug Mode Library
 * Copyright (C) 1995  W. Eric Norum
 * Copyright (C) 1998  Chris Johns
 *
 * Based on:
 *  1. `A Background Debug Mode Driver Package for Motorola's
 *     16- and 32-Bit Microcontrollers', Scott Howard, Motorola
 *     Canada, 1993.
 *  2. `Linux device driver for public domain BDM Inteface',
 *     M. Schraut, Technische Universitaet Muenchen, Lehrstuhl
 *     fuer Prozessrechner, 1995.
 *
 * Extended to support the ColdFire BDM interface using the P&E module
 * which comes with the EVB. Thanks to David Fiddes who has tested the
 * driver with the 5206 (5V), 5206e (3.3V) and 5307 devices.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * W. Eric Norum
 * Saskatchewan Accelerator Laboratory
 * University of Saskatchewan
 * 107 North Road
 * Saskatoon, Saskatchewan, CANADA
 * S7N 5C6
 * 
 * eric@skatter.usask.ca
 *
 * Coldfire support by:
 * Chris Johns
 * Objective Design Systems
 * 35 Cairo Street
 * Cammeray, Sydney, 2062, Australia
 *
 * ccj@acm.org
 *
 */

#include "BDMlib.h"
#include "bfd.h"
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/*
 * Limit of one BDM per process
 */
extern const char *bdmIO_lastErrorString;

#define dbprintf bdmDebug

void
bdmDebug (const char *format, ...);
int
bdmCheck (void);

/*
 ***********************
 * Downloader routines *
 ***********************
 */
static int downLoaderReturn;
static int downLoaderFirst;
static unsigned long downLoadBaseAddress, downLoadStartAddress;

static void
downloadSection (bfd *abfd, sec_ptr sec, PTR ignore)
{
  unsigned long dfc;
  unsigned long address;
  unsigned long nleft;
  int count;
  file_ptr offset;
  char cbuf[1024];

  /*
   * See if the section needs loading
   */
  dbprintf ("Section flags:%#x", bfd_get_section_flags (abfd, sec));
  if ((downLoaderReturn < 0) ||
      ((bfd_get_section_flags (abfd, sec) & SEC_HAS_CONTENTS) == 0) ||
      ((bfd_get_section_flags (abfd, sec) & SEC_LOAD) == 0))
    return;
  address = bfd_section_lma (abfd, sec);
  dbprintf ("Section address:%#x", address);
  if (downLoaderFirst && (bfd_get_section_flags (abfd, sec) & SEC_CODE)) {
    downLoadStartAddress = bfd_get_start_address (abfd);
    downLoadBaseAddress = address;
    downLoaderFirst = 0;
    dbprintf ("Start address:%#x   Base address:%#x", downLoadStartAddress, downLoadBaseAddress);
  }

  /*
   * Set the appropriate destination address space
   */
  if (bfd_get_section_flags (abfd, sec) & SEC_CODE)
    dfc = 0x6;  /* Supervisor program space */
  else
    dfc = 0x5;  /* Supervisor data space */
  if (bdmWriteSystemRegister (BDM_REG_DFC, dfc) < 0) {
    downLoaderReturn = -1;
    return;
  }

  /*
   * Load the section in `sizeof cbuf` chunks
   */
  nleft = bfd_get_section_size_before_reloc (sec);
  if (nleft == 0)
    return;
  offset = 0;
  while (nleft) {
    if (nleft > sizeof cbuf)
      count = sizeof cbuf;
    else
      count = nleft;
    if (!bfd_get_section_contents (abfd, sec, cbuf, offset, count)) {
      bdmIO_lastErrorString = "Error reading section contents";
      downLoaderReturn = -1;
      return;
    }
    if (bdmWriteMemory (address, cbuf, count) < 0) {
      downLoaderReturn = -1;
      return;
    }
    address += count;
    offset += count;
    nleft -= count;
  }
}

/*
 * Load an executable image into the target
 */
int
bdmLoadExecutable (const char *name)
{
  bfd *abfd;
  unsigned long dfc;
  unsigned long l;

  /*
   * Make sure target is there
   */
  if (!bdmCheck ())
    return -1;

  /*
   * Open and verify the file
   */
  bfd_init ();
  abfd = bfd_openr (name, "default");
  if (abfd == NULL) {
    bdmIO_lastErrorString = bfd_errmsg (bfd_get_error());
    return -1;
  }
  if (!bfd_check_format (abfd, bfd_object)) {
    bdmIO_lastErrorString = "Not an object file";
    return -1;
  }

  /*
   * Save the destination function code register
   */
  if (bdmReadSystemRegister (BDM_REG_DFC, &dfc) < 0)
    return -1;

  /*
   * Load each section of the executable file
   */
  downLoaderFirst = 1;
  downLoaderReturn = 0;
  bfd_map_over_sections (abfd, downloadSection, NULL);
  if (downLoaderReturn < 0)
    return -1;

  /*
   * Set up stack pointer and program counter
   */
  if ((bdmReadLongWord (downLoadBaseAddress, &l) < 0)
   || (bdmWriteSystemRegister (BDM_REG_SSP, l) < 0)
   || (bdmReadLongWord (downLoadBaseAddress+4, &l) < 0)
   || (bdmWriteSystemRegister (BDM_REG_RPC, l) < 0)
   || (bdmWriteSystemRegister (BDM_REG_VBR, downLoadBaseAddress) < 0)
   || (bdmWriteSystemRegister (BDM_REG_DFC, dfc) < 0))
    return -1;
  return 0;
}
