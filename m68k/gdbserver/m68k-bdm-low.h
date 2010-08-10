/* BDM/m68k specific low level interface, for the remote server for GDB.
   Copyright (C) 2008 Free Software Foundation, Inc.
   Copyright (C) 2008 Chris Johns (chrisj@rtems.org).

   This file is part of M68K BDM.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   The code in the this file is based on the GDB server code in GDB and
   the code in M68K BDM GDB patch file called 'remote-m68k-bdm.c'.
   Based on:
    1. `A Background Debug Mode Driver Package for Motorola's
       16-bit and 32-Bit Microcontrollers', Scott Howard, Motorola
       Canada, 1993.
    2. `Linux device driver for public domain BDM Interface',
       M. Schraut, Technische Universitaet Muenchen, Lehrstuhl
       fuer Prozessrechner, 1995.                                      */

#if !defined (M68K_BDM_LOW_H)
#define M68K_BDM_LOW_H

#include "regdef.h"
#include "BDMlib.h"

/*
 * Mark a register as a control register using this define.
 */
#define BDM_REG_CONTROL_REG      (1 << 31)
#define BDM_REG_CONTROL_REG_MASK (~(1 << 31))
#define BDM_REG_CTRL(r)          (BDM_REG_CONTROL_REG | (r))

/*
 * Mark a register as a debug register using this define.
 */
#define BDM_REG_DEBUG_REG      (1 << 30)
#define BDM_REG_DEBUG_REG_MASK (~(1 << 30))
#define BDM_REG_DEBUG(r)       (BDM_REG_DEBUG_REG | (r))

/*
 * Mark a register as virtual or not present.
 */
#define BDM_REG_VIRTUAL_REG      (1 << 29)
#define BDM_REG_VIRTUAL_REG_MASK (~(1 << 29))
#define BDM_REG_VIRT(r)          (BDM_REG_VIRTUAL_REG | (r))

#define BDM_REG_MASK \
  (~(BDM_REG_CONTROL_REG | BDM_REG_DEBUG_REG | BDM_REG_VIRTUAL_REG))

/*
 * Types of registers.
 */
#define M68K_BDM_REG_TYPE_INT32         (0)
#define M68K_BDM_REG_TYPE_UINT32        (1)
#define M68K_BDM_REG_TYPE_VOID_DATA_PTR (2)
#define M68K_BDM_REG_TYPE_M68881_EXT    (3)

/*
 * The Coldfire VBR is write only.
 */
#define M68K_BDM_CF_VBR_FLAGS (REG_NON_CACHEABLE | REG_WRITE_ONLY)

/*
 * Hold a mapping from a register number to a register type
 * and device number type. The register number comes from the
 * register name.
 */
struct m68k_bdm_reg_mapping
{
  const char*  name;
  int          type;
  int          num;
  unsigned int code;
  unsigned int flags; 
};

/*
 * The number of registers.
 */
#define M68K_BDM_REG_NUMBER(_m) (sizeof (_m) / sizeof (struct m68k_bdm_reg_mapping))

#endif
