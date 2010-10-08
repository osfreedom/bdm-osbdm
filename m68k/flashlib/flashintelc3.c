/* $Id:
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

#include "flashintelc3.h"
#include "flash_filter.h"
#include <stdint.h>

#if HOST_FLASHING
# include <inttypes.h>
# include <stdio.h>
# include <BDMlib.h>
# include <string.h>
#endif


typedef struct
{
  const char *name;
  const uint32_t manufacturer, device_id;
  const uint32_t size; /* in bytes */
  const uint32_t num_sectors;
  const uint32_t top;
} chip_t;

/*
 * Warning! do not change this struct without changing the download_struct
 * function.
 */
typedef struct
{
  uint32_t flash_address;
  uint32_t flash_size;
  uint32_t num_sectors;
  uint32_t top;
} chiptype_t;

static const chip_t chips[] = {
  {"28F800C3T", 0x0089, 0x88C0, 0x100000,  23, 1},    /* INTEL */
  {"28F800C3B", 0x0089, 0x88C1, 0x100000,  23, 0},    /* INTEL */
  {"28F160C3T", 0x0089, 0x88C2, 0x200000,  39, 1},    /* INTEL */
  {"28F160C3B", 0x0089, 0x88C3, 0x200000,  39, 0},    /* INTEL */
  {"28F320C3T", 0x0089, 0x88C4, 0x400000,  71, 1},    /* INTEL */
  {"28F320C3B", 0x0089, 0x88C5, 0x400000,  71, 0},    /* INTEL */
  {"28F640C3T", 0x0089, 0x88CC, 0x800000, 135, 1},    /* INTEL */
  {"28F640C3B", 0x0089, 0x88CD, 0x800000, 135, 0},    /* INTEL */
};

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

static inline uint16_t
swap16(uint16_t a_v)
{
  return (uint16_t) (((a_v >> 8) & 0x00ff) | ((a_v << 8) & 0xff00));
}

static inline int
is_little_endian(void)
{
  uint32_t i = 1;
  return (int) *((uint8_t *) &i);
}

DEFINE_READ_FUNC(chip_rd_word, unsigned short, bdmReadWord)
DEFINE_WRITE_FUNC(chip_wr_word, uint16_t, bdmWriteWord)

/* We need a unique symbol to associate a (target-based) plugin with its
   (host-based) driver.
*/
static char driver_magic[] = "flashintelc3";

#if HOST_FLASHING
static char *
prog_entry(void)
{
  return "flashintelc3_prog";
}
#endif

static uint32_t
flashintelc3_sector_size(chiptype_t* chip, int sector)
{
  if(chip->top) {
    if(sector < (chip->num_sectors - 8)) {
      return 0x10000;
    }
    else {
      return 0x2000;
    }
  }
  else {
    if(sector < 8) {
      return 0x2000;
    }
    else {
      return 0x10000;
    }
  }
}

static uint32_t
flashintelc3_sector_offset(chiptype_t* chip, int sector)
{
  if(chip->top) {
    uint32_t d = chip->num_sectors - 8;
    if(sector < d) {
      return 0x10000 * sector;
    }
    else {
      return (0x10000 * d) + (0x2000 * (sector - d));
    }
  }
  else {
    if(sector < 8) {
      return 0x2000 * sector;
    }
    else {
      return 0x10000 + (0x10000 * (sector - 8));
    }
  }
}

/* 0=unlock, 1=lock */
static void
flashintelc3_lock(void *chip_descr, uint32_t start, uint32_t bytes, int cmd)
{
  chiptype_t *ct = (chiptype_t *) chip_descr;
  int i, s = 0, e = 0;
  int ucmd = (cmd) ? 0x01 : 0xD0;

  if(bytes == 0)
    return;

  /* find start sector */
  for(i = 0; i < ct->num_sectors; ++i) {
    uint32_t off = flashintelc3_sector_offset(ct, i);
    uint32_t size = flashintelc3_sector_size(ct, i);
    if((start >= ct->flash_address + off) &&
       (start < ct->flash_address + off + size)) {
      s = i;
      break;
    }
  }

  /* find end */
  for(; i < ct->num_sectors; ++i) {
    uint32_t off = flashintelc3_sector_offset(ct, i);
    uint32_t size = flashintelc3_sector_size(ct, i);
    if(start + bytes <= ct->flash_address + off + size) {
      e = i;
      break;
    }
  }

  /* unlock range */
  for(i = s; i <= e; ++i) {
    uint32_t off = flashintelc3_sector_offset(ct, i);
    chip_wr_word(ct->flash_address + off, 0x60);
    chip_wr_word(ct->flash_address + off, ucmd);
  }
}

/* The actual programming function.
 * NOTE: pos need to be 2 byte aligned!
 */
static uint32_t
flashintelc3_prog(void *chip_descr,
                  uint32_t pos, unsigned char *data, uint32_t cnt)
{
  chiptype_t *ct = (chiptype_t *) chip_descr;
  uint32_t word_cnt;
  uint32_t n;
  uint16_t status;

  flashintelc3_lock(ct, pos, cnt, 0);

  word_cnt = (cnt + 1) / 2;
  for (n = 0; n < word_cnt; ++n) {
    uint16_t d = ((uint16_t *) (void*) data)[n];

    chip_wr_word(pos, 0x40);

    /* write_word on host assumes host endianess so will perform a
     * byte order swap if host is little endian.  we need to swap if little
     * endian such that the swap will just swap back to the correct endianess
     */
    if(is_little_endian())
      d = swap16(d);
    chip_wr_word(pos, d);

    /*
     * Wait for program operation to finish
     */
    do {
      chip_wr_word(pos, 0x70);
      status = chip_rd_word(pos);
    }
    while (!(status & 0x80));

    pos += 2;
  }

  chip_wr_word(ct->flash_address, 0xff);

  return cnt;
}

static void
flashintelc3_erase(void *chip_descr, int32_t sector_address)
{
  chiptype_t *ct = (chiptype_t *) chip_descr;
  uint16_t status;
  int i;
#if HOST_FLASHING
  int spin = 0;
#endif

  if(sector_address == -1) {
    flashintelc3_lock(ct, ct->flash_address, ct->flash_size, 0);

    /* erasing all sectors */
    for(i = 0; i < ct->num_sectors; ++i) {
      uint32_t off = flashintelc3_sector_offset(ct, i);

      chip_wr_word(ct->flash_address + off, 0x20);
      chip_wr_word(ct->flash_address + off, 0xD0);

      /* wait */
      do {
        chip_wr_word(ct->flash_address + off, 0x70);
        status = chip_rd_word(ct->flash_address + off);
#if HOST_FLASHING
        spin = flash_spin(spin);
#endif
      } while(!(status & 0x80));
    }
  }
  else {
    uint32_t off = flashintelc3_sector_offset(ct, sector_address);
    flashintelc3_lock(ct, ct->flash_address + off, 1, 0);

    /* erasing said sector address */
    chip_wr_word(ct->flash_address + off, 0x20);
    chip_wr_word(ct->flash_address + off, 0xD0);

    /* wait */
    do {
      chip_wr_word(ct->flash_address + off, 0x70);
      status = chip_rd_word(ct->flash_address + off);
#if HOST_FLASHING
        spin = flash_spin(spin);
#endif
    } while(!(status & 0x80));
  }

  chip_wr_word(ct->flash_address, 0xff);
}

/* Blank check operation. Sector address is relative to chip-base.
   With sector address==-1, the whole chip is checked.
 */
static int
flashintelc3_blank_chk(void *chip_descr, int32_t sector_address)
{
#if HOST_FLASHING
  printf("intelc3: blank_chk not implemented!\n");
#endif

  return 0;
}

/* wait for queued erasing operations to finish
 */
static int
flashintelc3_erase_wait(void *chip_descr)
{
#if HOST_FLASHING
  printf("intelc3: no wait needed!\n");
#endif

  return 0;
}

#if HOST_FLASHING

# ifndef NUMOF
#  define NUMOF(ary) (sizeof(ary)/sizeof(ary[0]))
# endif

/* autodetect
 */
static uint32_t
flashintelc3_search_chip(void *chip_descr, char *description, uint32_t pos)
{
  uint32_t size = 0;
  uint32_t m, d;
  chiptype_t *ct = (chiptype_t *) chip_descr;
  const chip_t *chip;
  int i;

  /* read the manufacturer id */
  chip_wr_word(pos, 0x90);
  m = chip_rd_word(pos + 0);

  /* read the device id */
  chip_wr_word(pos, 0x90);
  d = chip_rd_word(pos + 2);

  /* find our device */
  for (i = 0; i < NUMOF(chips); i++) {
    chip = &chips[i];
    if(chip->manufacturer == m && chip->device_id == d) {
      ct->flash_address = pos;
      ct->flash_size = chip->size;
      ct->num_sectors = chip->num_sectors;
      ct->top = chip->top;
      size = chip->size;

      if (description) {
        sprintf(description, "%10s @ 0x%08" PRIx32 "..0x%08" PRIx32 " "
                "manuf:0x%02" PRIx32 " device:0x%04" PRIx32 " size:0x%08" PRIx32,
                chip->name, pos, pos + chip->size, m, d, chip->size);
      }

      break;
    }
  }

  /* put the device back into read mode */
  chip_wr_word(pos, 0xff);

  return size;
}

static int
download_struct(void *chip_descr, uint32_t adr)
{
  chiptype_t *ct = (chiptype_t *) chip_descr;

  bdmWriteLongWord(adr, ct->flash_address);
  adr += 4;
  bdmWriteLongWord(adr, ct->flash_size);
  adr += 4;
  bdmWriteLongWord(adr, ct->num_sectors);
  adr += 4;
  bdmWriteLongWord(adr, ct->top);
  adr += 4;

  return adr;
}

void
init_flashintelc3(int num)
{
  register_algorithm(num, driver_magic, sizeof(chiptype_t),
                     download_struct,
                     flashintelc3_search_chip,
                     flashintelc3_erase,
                     flashintelc3_blank_chk,
                     flashintelc3_erase_wait, flashintelc3_prog, prog_entry);
}

#else

void
init_flashintelc3(int num)
{
  register_algorithm(num, driver_magic, 0,
                     0,
                     0,
                     flashintelc3_erase,
                     flashintelc3_blank_chk,
                     flashintelc3_erase_wait, flashintelc3_prog, 0);
}

#endif
