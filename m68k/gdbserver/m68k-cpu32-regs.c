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
 * The standard CPU32 core.
 */

struct m68k_bdm_reg_mapping m68k_bdm_cpu32_reg_map[] = {
  { "d0",  M68K_BDM_REG_TYPE_INT32,         0,  BDM_REG_D0, 0 },
  { "d1",  M68K_BDM_REG_TYPE_INT32,         1,  BDM_REG_D1, 0 },
  { "d2",  M68K_BDM_REG_TYPE_INT32,         2,  BDM_REG_D2, 0 },
  { "d3",  M68K_BDM_REG_TYPE_INT32,         3,  BDM_REG_D3, 0 },
  { "d4",  M68K_BDM_REG_TYPE_INT32,         4,  BDM_REG_D4, 0 },
  { "d5",  M68K_BDM_REG_TYPE_INT32,         5,  BDM_REG_D5, 0 },
  { "d6",  M68K_BDM_REG_TYPE_INT32,         6,  BDM_REG_D6, 0 },
  { "d7",  M68K_BDM_REG_TYPE_INT32,         7,  BDM_REG_D7, 0 },
  { "a0",  M68K_BDM_REG_TYPE_VOID_DATA_PTR, 8,  BDM_REG_A0, 0 },
  { "a1",  M68K_BDM_REG_TYPE_VOID_DATA_PTR, 9,  BDM_REG_A1, 0 },
  { "a2",  M68K_BDM_REG_TYPE_VOID_DATA_PTR, 10, BDM_REG_A2, 0 },
  { "a3",  M68K_BDM_REG_TYPE_VOID_DATA_PTR, 11, BDM_REG_A3, 0 },
  { "a4",  M68K_BDM_REG_TYPE_VOID_DATA_PTR, 12, BDM_REG_A4, 0 },
  { "a5",  M68K_BDM_REG_TYPE_VOID_DATA_PTR, 13, BDM_REG_A5, 0 },
  { "fp",  M68K_BDM_REG_TYPE_VOID_DATA_PTR, 14, BDM_REG_A6, 0 },
  { "sp",  M68K_BDM_REG_TYPE_VOID_DATA_PTR, 15, BDM_REG_A7, 0 },
  { "ps",  M68K_BDM_REG_TYPE_INT32,         16, BDM_REG_SR, 0 },
  { "pc",  M68K_BDM_REG_TYPE_INT32,         17, BDM_REG_RPC, 0 },
  { "vbr", M68K_BDM_REG_TYPE_VOID_DATA_PTR, 18, BDM_REG_VBR, REG_NON_CACHEABLE },
  { "pcc", M68K_BDM_REG_TYPE_INT32,         19, BDM_REG_PCC, 0 },
  { "usp", M68K_BDM_REG_TYPE_VOID_DATA_PTR, 20, BDM_REG_USP, 0 },
  { "ssp", M68K_BDM_REG_TYPE_VOID_DATA_PTR, 21, BDM_REG_SSP, 0 },
  { "sfc", M68K_BDM_REG_TYPE_INT32,         22, BDM_REG_SFC, REG_NON_CACHEABLE },
  { "dfc", M68K_BDM_REG_TYPE_INT32,         23, BDM_REG_DFC, REG_NON_CACHEABLE }
};

const int m68k_bdm_cpu32_reg_map_size = M68K_BDM_REG_NUMBER (m68k_bdm_cpu32_reg_map);
