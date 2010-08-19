/* $Id: flash29.c,v 1.7 2009/05/16 06:21:22 cjohns Exp $
 *
 * Driver for 29Fxxx and 49Fxxx flash chips.
 *
 * 2003-12-28 Josef Wolf (jw@raven.inka.de)
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

/* This piece of code arose from ad-hoc code-snippets which I threw into
   discussions on the bdm-devel mailing list and in private mails to Pavel
   Pisa. I have put those pieces together, removed compiler-errors, rewrote
   them several times and optimized somewhat.

  This code supports:

  - 29Fxxx and 49Fxxx types of flash chips.
  - Host-only, target-assisted and target-only operation modes.
  - Bus widths of 1, 2 and 4 bytes.
  - Chip widths of 1, 2 and 4 bytes.
  - Autodetection of known chips on arbitrary bus/chip width combinations.
  - Fast bypass unlock programming mode on chips that support it.
  - Unaligned programming.
*/

/* Todo:
   CSI command set? What is this?
   CFI specification http://www.amd.com/products/nvd/overview/cfi.html
*/

#include "flash29.h"
#include "flash_filter.h"

#if HOST_FLASHING
# include <inttypes.h>
# include <stdio.h>
# include <BDMlib.h>
#endif

/* This defines details for the flash algorithm.
 */
typedef struct
{
  uint32_t cmd_unlock1;
  uint32_t cmd_unlock2;
  uint32_t cmd_reset;
  uint32_t cmd_autoselect;
  uint32_t cmd_program;
  uint32_t cmd_erase;
  uint32_t cmd_chiperase;
  uint32_t cmd_secterase;
  uint32_t cmd_bypass;
  uint32_t cmd_resbypass1;
  uint32_t cmd_resbypass2;

  uint32_t timeout_mask;
  uint32_t adr_unlock1;
  uint32_t adr_unlock2;
} alg_info_t;

typedef struct
{
  const char *name;
  const uint32_t manufacturer, device_id;
  const uint32_t size;                  /* in bytes */
  const alg_info_t *alg_info;
} chip_t;

typedef struct
{
  const alg_info_t *alg_info;           /* algorithm information */
  uint32_t adr;                         /* base adress of the chip */
  uint32_t size;                        /* in bytes */
  uint32_t chip_width;                  /* 1->8bit, 2->16bit, 4->32bit */
  uint32_t bus_width;                   /* 1->8bit, 2->16bit, 4->32bit */
  uint32_t wait_adr;                    /* address to check when waiting for erase */

  /* From here on, everything is redundant. The only reason to maintain this
     redundant information is optimization. */
  uint32_t reg1;                        /* address of unlock register 1 */
  uint32_t reg2;                        /* address of unlock register 2 */

  /* This information is placed at the end of the struct because
     download_struct() don't need to fiddle around with it. */
  void (*wr_func) (uint32_t, uint32_t); /* write function */
   uint32_t(*rd_func) (uint32_t);       /* read function */
  uint32_t bus_mask;                    /* Mask to get bus_width lowest bytes */
  uint32_t chip_mask;                   /* Mask to get chip_width lowest bytes */
  uint32_t shift;                       /* shift for multiplication by bus_width */
} chiptype_t;

static const alg_info_t alg_29_std = {  /* standard 29fxx chips */
  0xaaaaaaaa,                           /* unlock 1 command */
  0x55555555,                           /* unlock 2 command */
  0xf0f0f0f0,                           /* res */
  0x90909090,                           /* asel */
  0xa0a0a0a0,                           /* prog */
  0x80808080,                           /* erase */
  0x10101010,                           /* chiperase */
  0x30303030,                           /* secterase */
  0x00000000,                           /* no unlock bypass */
  0x90909090,                           /* unlock bypass res1 */
  0x00000000,                           /* unlock bypass res2 */
  0x20,                                 /* timeout bitmask */
  0x555,                                /* register 1 */
  0x2aa,                                /* register 5 */
};

static const alg_info_t alg_29_unl = {  /* 29fxx with unlock bypass mode */
  0xaaaaaaaa,                           /* unlock 1 command */
  0x55555555,                           /* unlock 2 command */
  0xf0f0f0f0,                           /* res */
  0x90909090,                           /* asel */
  0xa0a0a0a0,                           /* prog */
  0x80808080,                           /* erase */
  0x10101010,                           /* chiperase */
  0x30303030,                           /* secterase */
  0x20202020,                           /* unlock bypass */
  0x90909090,                           /* unlock bypass res1 */
  0x00000000,                           /* unlock bypass res2 */
  0x20,                                 /* timeout bitmask */
  0x555,                                /* register 1 */
  0x2aa,                                /* register 5 */
};

static const alg_info_t alg_49 = {      /* 49fxx and 39fxx chips */
  0xaaaaaaaa,                           /* unlock 1 command */
  0x55555555,                           /* unlock 2 command */
  0xf0f0f0f0,                           /* res */
  0x90909090,                           /* asel */
  0xa0a0a0a0,                           /* prog */
  0x80808080,                           /* erase */
  0x10101010,                           /* chiperase */
  0x30303030,                           /* secterase *//* Block erase ? */
  0x00000000,                           /* no unlock bypass */
  0x90909090,                           /* unlock bypass res1 */
  0x00000000,                           /* unlock bypass res2 */
  0x00,                                 /* no timeout support */
  0x5555,                               /* register 1 */
  0x2aaa,                               /* register 5 */
};

/* Here come the known chips.
 */
static const chip_t chips[] = {
  {"Am29LV001BT", 0x01, 0xed, 0x20000, &alg_29_unl},    /* AMD */
  {"Am29LV001BB", 0x01, 0x6d, 0x20000, &alg_29_unl},    /* AMD */
  {"Am29LV002T", 0x01, 0x40, 0x40000, &alg_29_std},     /* AMD */
  {"Am29LV002B", 0x01, 0xc2, 0x40000, &alg_29_std},     /* AMD */
  {"Am29LV004T", 0x01, 0xb5, 0x80000, &alg_29_std},     /* AMD */
  {"Am29LV004B", 0x01, 0xb6, 0x80000, &alg_29_std},     /* AMD */
  {"Am29LV008BT", 0x01, 0x3e, 0x100000, &alg_29_unl},   /* AMD */
  {"Am29LV008BB", 0x01, 0x37, 0x100000, &alg_29_unl},   /* AMD */
  {"Am29LV017B", 0x01, 0xc8, 0x200000, &alg_29_unl},    /* AMD */

  {"Am29F010B", 0x01, 0x20, 0x20000, &alg_29_std},      /* AMD */

  {"At49F040", 0x1f, 0x13, 0x80000, &alg_49},   /* Atmel */

  {"Am29F040B", 0x01, 0xa4, 0x80000, &alg_29_std},      /* AMD */
  {"Am29F400BT", 0x01, 0x2223, 0x80000, &alg_29_std},   /* AMD */
  {"Am29F400BB", 0x01, 0x22ab, 0x80000, &alg_29_std},   /* AMD */
  {"MBM29F400TC", 0x04, 0x2223, 0x80000, &alg_29_std},  /* Fujitsu */
  {"MBM29F400BC", 0x04, 0x22ab, 0x80000, &alg_29_std},  /* Fujitsu */
  {"M29F400BB", 0x20, 0x22d6, 0x80000, &alg_29_unl},    /* ST */
  {"M29F400BB", 0x20, 0x22d5, 0x80000, &alg_29_unl},    /* ST */

  {"Am29F080B", 0x01, 0xd5, 0x100000, &alg_29_std},     /* AMD */
  {"Am29F800BT", 0x01, 0x22d6, 0x100000, &alg_29_std},  /* AMD */
  {"Am29F800BB", 0x01, 0x2258, 0x100000, &alg_29_std},  /* AMD */

  {"Am29PL160C", 0x01, 0x2245, 0x200000, &alg_29_unl},  /* AMD */
  {"Am29LV160B", 0x01, 0x2249, 0x200000, &alg_29_unl},  /* AMD */
  {"M29LV160B", 0x20, 0x2249, 0x200000, &alg_29_unl},   /* ST */
  {"HY29LV160B", 0xad, 0x2249, 0x200000, &alg_29_unl},  /* HYRIX */
  {"MX29LV160B", 0xc2, 0x2245, 0x200000, &alg_29_unl},  /* MACRONIX */
  {"TC58FVB160A", 0x98, 0x0043, 0x200000, &alg_29_unl}, /* TOSHIBA */

  {"Am29LV320B", 0x01, 0x22f9, 0x400000, &alg_29_unl},  /* AMD */
  {"HY29LV320B", 0xad, 0x227d, 0x400000, &alg_29_unl},  /* HYRIX */
  {"MX29LV320B", 0xc2, 0x22a8, 0x400000, &alg_29_unl},  /* MACRONIX */
  {"TC58FVM5B2A", 0x98, 0x0055, 0x400000, &alg_29_unl}, /* TOSHIBA */
  {"TC58FVM5B3A", 0x98, 0x0050, 0x400000, &alg_29_unl}, /* TOSHIBA */

  {"Am29LV640MB", 0x01, 0x227e, 0x800000, &alg_29_unl}, /* AMD */
  {"M29W320ET", 0x20, 0x2256, 0x400000, &alg_29_unl},     /* ST */
  {"M29W640DB", 0x20, 0x22df, 0x800000, &alg_29_unl},   /* ST */
  {"M29(D)W128G", 0x20, 0x227e, 0x1000000, &alg_29_unl},     /* ST */
  {"MBM29DLV640E", 0x04, 0x227e, 0x800000, &alg_29_std},        /* FUJITSU */
  {"MX29LV640MB", 0xc2, 0x22cb, 0x800000, &alg_29_unl}, /* MACRONIX */
  {"TC58FVM6B2A", 0x98, 0x0058, 0x800000, &alg_29_unl}, /* TOSHIBA */

  {"Sst39VF1601", 0xbf, 0x234b, 0x200000, &alg_49},     /* SST */
  {"Sst39VF1602", 0xbf, 0x234a, 0x200000, &alg_49},     /* SST */
  {"Sst39VF3201", 0xbf, 0x235b, 0x400000, &alg_49},     /* SST */
  {"Sst39VF3202", 0xbf, 0x235a, 0x400000, &alg_49},     /* SST */
  {"Sst39VF6401", 0xbf, 0x236b, 0x800000, &alg_49},     /* SST */
  {"Sst39VF6402", 0xbf, 0x236a, 0x800000, &alg_49},     /* SST */
};

/* define the actual access functions to the flash. There are three orthogonal
   aspects for the access functions:

   1. Every bus_width needs its own set of functions because we need to access
   the chips with bus_width width. This is to avoid the access be split up
   into several accesses, which could abort a started command.

   2. Bus accesses need two functions: one for reading and one for writing.

   3. In addition, we need a second set of functions for host-assisted access.
*/

#if HOST_FLASHING

# define DEFINE_READ_FUNC(funcname,vartype,bdmfunc) \
    static uint32_t funcname (uint32_t adr) { \
      vartype val; \
      if (bdmfunc (adr, &val) < 0) \
        fprintf (stderr, #bdmfunc "(0x%08" PRIx32 ",xxx): %s\n", \
		    adr, bdmErrorString()); \
      return (uint32_t) val; \
    }

# define DEFINE_WRITE_FUNC(funcname,vartype,bdmfunc) \
    static void funcname (uint32_t adr, uint32_t val) { \
      if (bdmfunc (adr, val) < 0) \
        fprintf (stderr, #bdmfunc "(0x%08" PRIx32 ",0x%08" PRIx32 "): %s\n", \
                 adr, val, bdmErrorString()); \
    }

#else

# define DEFINE_READ_FUNC(funcname,vartype,bdmfunc) \
    static uint32_t funcname (uint32_t adr) { \
      return *(volatile vartype *)adr; \
    }

# define DEFINE_WRITE_FUNC(funcname,vartype,bdmfunc) \
    static void funcname (uint32_t adr,uint32_t val) { \
      *(volatile vartype *)adr = val; \
    }

#endif

DEFINE_READ_FUNC(chip_rd_char, unsigned char, bdmReadByte)
DEFINE_READ_FUNC(chip_rd_word, unsigned short, bdmReadWord)
DEFINE_READ_FUNC(chip_rd_long, unsigned long, bdmReadLongWord)

DEFINE_WRITE_FUNC(chip_wr_char, uint8_t, bdmWriteByte)
DEFINE_WRITE_FUNC(chip_wr_word, uint16_t, bdmWriteWord)
DEFINE_WRITE_FUNC(chip_wr_long, uint32_t, bdmWriteLongWord)

/* We need a unique symbol to associate a (target-based) plugin with its
   (host-based) driver.
*/
static char driver_magic[] = "flash29";

/* Populate the the chiptype structure with bus_width specific information.
   This information is redundant. It is maintained here only for
   optimizations.
 */
static void 
set_chip_access(chiptype_t * ct, uint32_t bus_width)
{
  ct->bus_width = bus_width;
  switch (bus_width) {
    case 1:
      ct->shift = 0;
      ct->bus_mask = 0x0ff;
      ct->wr_func = chip_wr_char;
      ct->rd_func = chip_rd_char;
      break;
    case 2:
      ct->shift = 1;
      ct->bus_mask = 0x0ffff;
      ct->wr_func = chip_wr_word;
      ct->rd_func = chip_rd_word;
      break;
    case 4:
      ct->shift = 2;
      ct->bus_mask = 0x0ffffffff;
      ct->wr_func = chip_wr_long;
      ct->rd_func = chip_rd_long;
      break;
  }
  ct->chip_mask = (0x100 << ((ct->chip_width - 1) << 3)) - 1;
}

/* Wait for the chip to complete an erase/program command. Pavel Pisa mentioned
   that there's probably a racing condition in this code, so we will need to
   take a closer look on this one.
 */
static int
wait_chip(chiptype_t * ct, uint32_t adr, uint32_t expect)
{
  uint32_t i;
  uint32_t rval;
  uint32_t bus_mask = ct->bus_mask;
  uint32_t chip_mask = ct->chip_mask;
  uint32_t tout_mask = ct->alg_info->timeout_mask;

  uint32_t(*rd_func) (uint32_t) = ct->rd_func;
  uint32_t last_val = (rd_func(adr) & bus_mask);

  while ((rval = (rd_func(adr) & bus_mask)) != (expect & bus_mask)) {

    /* We don't want to loop forever when we accidentally try to program an
       EPROM. So we expect at least one toggle-bit to change. */
    if (last_val == rval)
      return 0;

    /* loop over chips */
    for (i = 0; i < ct->bus_width; i += ct->chip_width) {

      /* shift to get bits of this chip onto the lowest bits. */
      uint32_t shift = i << 3;
      uint32_t shifted_chip_mask = chip_mask << shift;

      /* check whether chip i signalled timeout. */
      if ((rval >> shift) & tout_mask)
        return 0;

      /* check whether chip i returned expected data. */
      if (!(((rval ^ expect) & shifted_chip_mask))) {
        /* Yes, we're not interested in results from this chip anymore. */
        bus_mask &= ~shifted_chip_mask;
      }
    }
    last_val = rval;
  }

  return 1;
}

#if HOST_FLASHING
static char *
prog_entry(void)
{
  return "flash29_prog";
}
#endif

/* The actual programming function
 */
static uint32_t
flash29_prog(void *chip_descr,
             uint32_t pos, unsigned char *data, uint32_t cnt)
{
  uint32_t i, align;
  void (*wr_func) (uint32_t, uint32_t);
  uint32_t val = 0;

  chiptype_t *ct = chip_descr;
  uint32_t reg1 = ct->reg1;
  uint32_t reg2 = ct->reg2;
  uint32_t siz = ct->bus_width;
  const alg_info_t *alg_info = ct->alg_info;
  uint32_t cmd_bypass = alg_info->cmd_bypass;
  uint32_t cmd_reset = alg_info->cmd_reset;
  uint32_t cmd_prog = alg_info->cmd_program;
  uint32_t cmd_unlock1 = alg_info->cmd_unlock1;
  uint32_t cmd_unlock2 = alg_info->cmd_unlock2;

  set_chip_access(ct, siz);
  
  wr_func = ct->wr_func;

  /* handle unaligned programming */
  align = pos & ((1 << (siz - 1)) - 1);;

  pos &= ~align;

  for (i = 0; i < align; i++) {
    val <<= 8;
    val |= chip_rd_char(pos + i);
  }

  if (cmd_bypass) {
    wr_func(reg1, cmd_reset);
    wr_func(reg1, cmd_unlock1);
    wr_func(reg2, cmd_unlock2);
    wr_func(reg1, cmd_bypass);
  }
  
  i = 0;
#if FLASH_OPTIMIZE_FOR_SPEED
  switch (siz) {
# if FLASH_BUS_WIDTH1
    case 1:
      for (; i < cnt; i++) {
        if (!cmd_bypass) {
          chip_wr_char(reg1, cmd_reset);
          chip_wr_char(reg1, cmd_unlock1);
          chip_wr_char(reg2, cmd_unlock2);
        }
        chip_wr_char(reg1, cmd_prog);

        val = *data++;
        chip_wr_char(pos, val);
        if (!wait_chip(ct, pos, val))
          break;                        /* error out */

        pos++;
      }
# endif
# if FLASH_BUS_WIDTH2
    case 2:
      if (align)
        goto W1;
      while (i < cnt) {
        val = (val << 8) | *data++;
      W1:val =
          (val << 8) | (i++ <
                        cnt ? *data++ : chip_rd_char(pos + 1));

        if (!cmd_bypass) {
          chip_wr_word(reg1, cmd_reset);
          chip_wr_word(reg1, cmd_unlock1);
          chip_wr_word(reg2, cmd_unlock2);
        }
        chip_wr_word(reg1, cmd_prog);
        chip_wr_word(pos, val);
        if (!wait_chip(ct, pos, val))
          break;                        /* error out */

        pos += 2;
        i++;
        val = 0;
      }
      break;
# endif
# if FLASH_BUS_WIDTH4
    case 4:
      switch (align) {
        case 1:
          goto L1;
        case 2:
          goto L2;
        case 3:
          goto L3;
      }
      while (i < cnt) {
        val = (val << 8) | *data++;
      L1:val =
          (val << 8) | (i++ <
                        cnt ? *data++ : chip_rd_char(pos + 1));
      L2:val =
          (val << 8) | (i++ <
                        cnt ? *data++ : chip_rd_char(pos + 2));
      L3:val =
          (val << 8) | (i++ <
                        cnt ? *data++ : chip_rd_char(pos + 3));
        if (!cmd_bypass) {
          chip_wr_long(reg1, cmd_reset);
          chip_wr_long(reg1, cmd_unlock1);
          chip_wr_long(reg2, cmd_unlock2);
        }
        chip_wr_long(reg1, cmd_prog);
        chip_wr_long(pos, val);
        if (!wait_chip(ct, pos, val))
          break;                        /* error out */

        pos += 4;
        i++;
        val = 0;
      }
      break;
# endif
  }
#else
  while (i < cnt) {
    uint32_t j;

    for (j = 0; j < siz - align; j++) {
      val <<= 8;
      val |= i + j < cnt ? *data++ : chip_rd_char(pos + j);
    }
    if (!cmd_bypass) {
      wr_func(reg1, cmd_reset);
      wr_func(reg1, cmd_unlock1);
      wr_func(reg2, cmd_unlock2);
    }
    wr_func(reg1, cmd_prog);
    wr_func(pos, val);
    
    if (!wait_chip(ct, pos, val))
      break;                            /* error out */

    pos += siz;
    i += siz;

    val = 0;
    align = 0;
  }
#endif

  if (cmd_bypass) {
    wr_func(reg1, ct->alg_info->cmd_resbypass1);
    wr_func(reg2, ct->alg_info->cmd_resbypass2);
  }

  return i > cnt ? cnt : i;
}

/* Initiate erase operation. Sector address is relative to chip-base.
   With sector address==-1, the whole chip is erased. The erase operation
   can be called several times before flash_29_erase_wait() is called for
   simultanous erasure of multiple sectors.
 */
static void
flash29_erase(void *chip_descr, int32_t sector_address)
{
  chiptype_t *ct = chip_descr;
  uint32_t reg1 = ct->reg1;
  uint32_t reg2 = ct->reg2;
  const alg_info_t *alg_info = ct->alg_info;
  void (*wr_func) (uint32_t, uint32_t) = ct->wr_func;

  wr_func(reg1, alg_info->cmd_reset);
  wr_func(reg1, alg_info->cmd_unlock1);
  wr_func(reg2, alg_info->cmd_unlock2);
  wr_func(reg1, alg_info->cmd_erase);
  wr_func(reg1, alg_info->cmd_unlock1);
  wr_func(reg2, alg_info->cmd_unlock2);

  if (sector_address >= 0 && sector_address < ct->size) {
    ct->wait_adr = ct->adr + (sector_address << ct->shift);
    wr_func(ct->wait_adr, alg_info->cmd_secterase);
  } else {
    ct->wait_adr = ct->adr;
    wr_func(reg1, alg_info->cmd_chiperase);
  }
}

/* wait for queued erasing operations to finish
 */
static int
flash29_erase_wait(void *chip_descr)
{
  chiptype_t *ct = chip_descr;

  return wait_chip(ct, ct->wait_adr, ct->bus_mask);
}

#if HOST_FLASHING

/* read the chip ID
 */
static uint32_t
read_id(chiptype_t * ct, int adr)
{
  unsigned int ret;
  uint32_t reg1 = ct->reg1;
  uint32_t reg2 = ct->reg2;
  const alg_info_t *alg_info = ct->alg_info;
  void (*wr_func) (uint32_t, uint32_t) = ct->wr_func;

  wr_func(reg1, alg_info->cmd_reset);
  wr_func(reg1, alg_info->cmd_unlock1);
  wr_func(reg2, alg_info->cmd_unlock2);
  wr_func(reg1, alg_info->cmd_autoselect);

  ret = ct->rd_func(ct->adr + (adr << ct->shift));

  wr_func(reg1, alg_info->cmd_reset);

  return ret;
}

/* autodetect a 29Fxxx type chip
 */
static uint32_t
flash29_search_chip(void *chip_descr, char *description, uint32_t pos)
{
  chiptype_t *ct = chip_descr;
  int i, j, bw, cw;
  uint32_t m, d;
  uint32_t exp;
  const alg_info_t *alg_info;
  const chip_t *chip;

  ct->adr = pos;

  for (bw = 4; bw; bw >>= 1) {
    for (cw = bw; cw; cw >>= 1) {
      ct->chip_width = cw;

      set_chip_access(ct, bw);

      for (i = 0; i < NUMOF(chips); i++) {
        chip = &chips[i];
        ct->size = chip->size * (bw / cw);
        ct->alg_info = alg_info = chip->alg_info;
        ct->reg1 = pos + ((alg_info->adr_unlock1) << (ct->shift));
        ct->reg2 = pos + ((alg_info->adr_unlock2) << (ct->shift));

        if ((m = read_id(ct, 0)) != chip->manufacturer)
          continue;

        exp = 0;
        for (j = 0; j < bw; j += cw) {
          exp <<= ((cw - 1) << 3);
          exp |= (chip->device_id) & (ct->chip_mask);
        }

        if ((d = read_id(ct, 1)) != exp)
          continue;

        /* check if we are just reading ram */
        if (m == ct->rd_func(pos) && d == ct->rd_func(pos + (1 << ct->shift)))
          return 0;

        if (description) {
          sprintf(description, "%10s @ 0x%08" PRIx32 "..0x%08" PRIx32 " "
                  "bw:%d cw:%d manuf:0x%02" PRIx32 " device:0x%04" PRIx32 " size:0x%08" PRIx32,
                  chip->name, pos, pos + ct->size, bw, cw, m, d, ct->size);
        }

        return ct->size;
      }
    }
  }
  return 0;
}

# define CHIP_WR(base,str,field,val) \
   chip_wr_long((base)+(((unsigned char*)&((str)->field)) - \
		        ((unsigned char*)str)), \
		(val));

/* FIXME: this function assumes that data sizes and alignment requirements
   on target and host are identical.
*/
static int
download_struct(void *chip_descr, uint32_t adr)
{
  chiptype_t *ct = chip_descr;
  const alg_info_t *alg = ct->alg_info;

  uint32_t alg_adr = adr + sizeof(*ct);

  CHIP_WR(adr, ct, alg_info, alg_adr);
  CHIP_WR(adr, ct, adr, ct->adr);
  CHIP_WR(adr, ct, size, ct->size);
  CHIP_WR(adr, ct, chip_width, ct->chip_width);
  CHIP_WR(adr, ct, bus_width, ct->bus_width);
  CHIP_WR(adr, ct, reg1, ct->reg1);
  CHIP_WR(adr, ct, reg2, ct->reg2);

  CHIP_WR(alg_adr, alg, cmd_unlock1, alg->cmd_unlock1);
  CHIP_WR(alg_adr, alg, cmd_unlock2, alg->cmd_unlock2);
  CHIP_WR(alg_adr, alg, cmd_reset, alg->cmd_reset);
  CHIP_WR(alg_adr, alg, cmd_program, alg->cmd_program);
  CHIP_WR(alg_adr, alg, cmd_bypass, alg->cmd_bypass);
  CHIP_WR(alg_adr, alg, cmd_resbypass1, alg->cmd_resbypass1);
  CHIP_WR(alg_adr, alg, cmd_resbypass2, alg->cmd_resbypass2);
  CHIP_WR(alg_adr, alg, timeout_mask, alg->timeout_mask);
  
  return alg_adr + sizeof(*alg);
}

void
init_flash29(int num)
{
  register_algorithm(num, driver_magic, sizeof(chiptype_t),
                     download_struct,
                     flash29_search_chip,
                     flash29_erase, 0, flash29_erase_wait, flash29_prog,
                     prog_entry);
}

#else

void
init_flash29(int num)
{
  register_algorithm(num, driver_magic, 0, 0, 0, 
                     flash29_erase, 0, flash29_erase_wait, flash29_prog, 0);
}

#endif
