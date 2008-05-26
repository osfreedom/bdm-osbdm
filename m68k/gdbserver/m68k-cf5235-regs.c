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


#include "m68k-bdm-low.h"

/*
 * 5235 Coldfire Register set.
 */

struct m68k_bdm_reg_mapping m68k_bdm_cf5235_reg_map[] = {
  { "d0",       M68K_BDM_REG_TYPE_INT32,         0,  BDM_REG_D0, 0 },
  { "d1",       M68K_BDM_REG_TYPE_INT32,         1,  BDM_REG_D1, 0 },
  { "d2",       M68K_BDM_REG_TYPE_INT32,         2,  BDM_REG_D2, 0 },
  { "d3",       M68K_BDM_REG_TYPE_INT32,         3,  BDM_REG_D3, 0 },
  { "d4",       M68K_BDM_REG_TYPE_INT32,         4,  BDM_REG_D4, 0 },
  { "d5",       M68K_BDM_REG_TYPE_INT32,         5,  BDM_REG_D5, 0 },
  { "d6",       M68K_BDM_REG_TYPE_INT32,         6,  BDM_REG_D6, 0 },
  { "d7",       M68K_BDM_REG_TYPE_INT32,         7,  BDM_REG_D7, 0 },
  { "a0",       M68K_BDM_REG_TYPE_VOID_DATA_PTR, 8,  BDM_REG_A0, 0 },
  { "a1",       M68K_BDM_REG_TYPE_VOID_DATA_PTR, 9,  BDM_REG_A1, 0 },
  { "a2",       M68K_BDM_REG_TYPE_VOID_DATA_PTR, 10, BDM_REG_A2, 0 },
  { "a3",       M68K_BDM_REG_TYPE_VOID_DATA_PTR, 11, BDM_REG_A3, 0 },
  { "a4",       M68K_BDM_REG_TYPE_VOID_DATA_PTR, 12, BDM_REG_A4, 0 },
  { "a5",       M68K_BDM_REG_TYPE_VOID_DATA_PTR, 13, BDM_REG_A5, 0 },
  { "fp",       M68K_BDM_REG_TYPE_VOID_DATA_PTR, 14, BDM_REG_A6, 0 },
  { "sp",       M68K_BDM_REG_TYPE_VOID_DATA_PTR, 15, BDM_REG_A7, 0 },
  { "ps",       M68K_BDM_REG_TYPE_INT32,         16, BDM_REG_SR, 0 },
  { "pc",       M68K_BDM_REG_TYPE_INT32,         17, BDM_REG_RPC, 0 },
  { "vbr",      M68K_BDM_REG_TYPE_VOID_DATA_PTR, 18, BDM_REG_CTRL (0x801), M68K_BDM_CF_VBR_FLAGS },
  { "cacr",     M68K_BDM_REG_TYPE_INT32,         19, BDM_REG_CTRL (0x002), REG_NON_CACHEABLE },
  { "acr0",     M68K_BDM_REG_TYPE_INT32,         20, BDM_REG_CTRL (0x004), REG_NON_CACHEABLE },
  { "acr1",     M68K_BDM_REG_TYPE_INT32,         21, BDM_REG_CTRL (0x005), REG_NON_CACHEABLE },
  { "rambar",   M68K_BDM_REG_TYPE_INT32,         22, BDM_REG_CTRL (0xc05), REG_NON_CACHEABLE },
  { "othera7",  M68K_BDM_REG_TYPE_INT32,         23, BDM_REG_CTRL (0x800), REG_NON_CACHEABLE },
  { "csr",      M68K_BDM_REG_TYPE_INT32,         24, BDM_REG_DEBUG (0x0),  REG_NOT_ACCESSABLE },
  { "xcsr",     M68K_BDM_REG_TYPE_INT32,         25, BDM_REG_DEBUG (0x1),  REG_NOT_ACCESSABLE },
  { "aatr",     M68K_BDM_REG_TYPE_INT32,         26, BDM_REG_DEBUG (0x6),  REG_NOT_ACCESSABLE },
  { "tdr",      M68K_BDM_REG_TYPE_INT32,         27, BDM_REG_DEBUG (0x7),  REG_NOT_ACCESSABLE },
  { "pbr",      M68K_BDM_REG_TYPE_INT32,         28, BDM_REG_DEBUG (0x8),  REG_NOT_ACCESSABLE },
  { "pbmr",     M68K_BDM_REG_TYPE_INT32,         29, BDM_REG_DEBUG (0x9),  REG_NOT_ACCESSABLE },
  { "abhr",     M68K_BDM_REG_TYPE_INT32,         30, BDM_REG_DEBUG (0xc),  REG_NOT_ACCESSABLE },
  { "ablr",     M68K_BDM_REG_TYPE_INT32,         31, BDM_REG_DEBUG (0xd),  REG_NOT_ACCESSABLE },
  { "dbr",      M68K_BDM_REG_TYPE_INT32,         32, BDM_REG_DEBUG (0xe),  REG_NOT_ACCESSABLE },
  { "dbmr",     M68K_BDM_REG_TYPE_INT32,         33, BDM_REG_DEBUG (0xf),  REG_NOT_ACCESSABLE},
  { "macsr",    M68K_BDM_REG_TYPE_INT32,         34, BDM_REG_CTRL (0x804), REG_NON_CACHEABLE },
  { "mask",     M68K_BDM_REG_TYPE_INT32,         35, BDM_REG_CTRL (0x805), REG_NON_CACHEABLE },
  { "acc0",     M68K_BDM_REG_TYPE_INT32,         36, BDM_REG_CTRL (0x806), REG_NON_CACHEABLE },
  { "acc1",     M68K_BDM_REG_TYPE_INT32,         37, BDM_REG_CTRL (0x809), REG_NON_CACHEABLE },
  { "acc2",     M68K_BDM_REG_TYPE_INT32,         38, BDM_REG_CTRL (0x80a), REG_NON_CACHEABLE },
  { "acc3",     M68K_BDM_REG_TYPE_INT32,         39, BDM_REG_CTRL (0x80b), REG_NON_CACHEABLE },
  { "accext01", M68K_BDM_REG_TYPE_INT32,         40, BDM_REG_CTRL (0x807), REG_NON_CACHEABLE },
  { "accext32", M68K_BDM_REG_TYPE_INT32,         41, BDM_REG_CTRL (0x808), REG_NON_CACHEABLE }
};

const int m68k_bdm_cf5235_reg_map_size = M68K_BDM_REG_NUMBER (m68k_bdm_cf5235_reg_map);
