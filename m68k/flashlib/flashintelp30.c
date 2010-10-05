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

#include "flashintelp30.h"
#include "flash_filter.h"
#include <stdint.h>

#if HOST_FLASHING
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
  uint32_t wbuf_mask;
} chiptype_t;

static const chip_t chips[] = {
  {"28F640P30T", 0x0089, 0x8817,  0x800000,  67, 1},    /* INTEL */
  {"28F128P30T", 0x0089, 0x8818, 0x1000000, 131, 1},    /* INTEL */
  {"28F256P30T", 0x0089, 0x8819, 0x2000000, 259, 1},    /* INTEL */
  {"28F640P30B", 0x0089, 0x881A,  0x800000,  67, 0},    /* INTEL */
  {"28F128P30B", 0x0089, 0x881B, 0x1000000, 131, 0},    /* INTEL */
  {"28F256P30B", 0x0089, 0x881C, 0x2000000, 259, 0},    /* INTEL */
  {"28F640P33T", 0x0089, 0x881D,  0x800000,  67, 1},    /* INTEL */
  {"28F128P33T", 0x0089, 0x881E, 0x1000000, 131, 1},    /* INTEL */
  {"28F256P33T", 0x0089, 0x891F, 0x2000000, 259, 1},    /* INTEL */
  {"28F640P33B", 0x0089, 0x8820,  0x800000,  67, 0},    /* INTEL */
  {"28F128P33B", 0x0089, 0x8821, 0x1000000, 131, 0},    /* INTEL */
  {"28F256P33B", 0x0089, 0x8922, 0x2000000, 259, 0},    /* INTEL */
};

#if HOST_FLASHING

# define DEFINE_READ_FUNC(funcname,vartype,bdmfunc) \
    static uint32_t funcname (uint32_t adr) { \
      vartype val; \
      if (bdmfunc (adr, &val) < 0) \
        fprintf (stderr, #bdmfunc "(0x%08lx,xxx): %s\n", \
		    adr, bdmErrorString()); \
      return (uint32_t) val; \
    }

# define DEFINE_WRITE_FUNC(funcname,vartype,bdmfunc) \
    static void funcname (uint32_t adr, uint32_t val) { \
      if (bdmfunc (adr, val) < 0) \
        fprintf (stderr, #bdmfunc "(0x%08lx,0x%08x): %s\n", \
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
static char driver_magic[] = "flashintelp30";

#if HOST_FLASHING
static char *
prog_entry(void)
{
  return "flashintelp30_prog";
}
#endif

static uint32_t
flashintelp30_sector_size(chiptype_t* chip, int sector)
{
  if(chip->top) {
    if(sector < (chip->num_sectors - 4)) {
      return 0x20000;
    }
    else {
      return 0x8000;
    }
  }
  else {
    if(sector < 4) {
      return 0x8000;
    }
    else {
      return 0x20000;
    }
  }
}

static uint32_t
flashintelp30_sector_offset(chiptype_t* chip, int sector)
{
  if(chip->top) {
    uint32_t d = chip->num_sectors - 4;
    if(sector < d) {
      return 0x20000 * sector;
    }
    else {
      return (0x20000 * d) + (0x8000 * (sector - d));
    }
  }
  else {
    if(sector < 4) {
      return 0x8000 * sector;
    }
    else {
      return 0x20000 + (0x20000 * (sector - 4));
    }
  }
}

static uint32_t
flashintelp30_sector_index(chiptype_t* ct, uint32_t start)
{
  int i;

  start -= ct->flash_address;
  for(i = 0; i < ct->num_sectors; ++i) {
    uint32_t off = flashintelp30_sector_offset(ct, i);
    uint32_t size = flashintelp30_sector_size(ct, i);
    if((start >= off) && (start < off + size)) {
      break;
    }
  }
  return i;
}

static inline uint32_t
flashintelp30_sector_address(chiptype_t* ct, uint32_t start)
{
  return ct->flash_address + flashintelp30_sector_offset(ct,
					flashintelp30_sector_index(ct, start));
}

/* 0=unlock, 1=lock */
static void
flashintelp30_lock(void *chip_descr, uint32_t start, uint32_t bytes, int cmd)
{
  chiptype_t *ct = (chiptype_t *) chip_descr;
  int i, s, e = 0;
  int ucmd = (cmd) ? 0x01 : 0xD0;

  if(bytes == 0)
    return;

  /* find start sector */
  s = flashintelp30_sector_index(ct, start);

  /* find end */
  for(i = s; i < ct->num_sectors; ++i) {
    uint32_t off = flashintelp30_sector_offset(ct, i);
    uint32_t size = flashintelp30_sector_size(ct, i);
    if(start + bytes <= ct->flash_address + off + size) {
      e = i;
      break;
    }
  }

  /* unlock range */
  for(i = s; i <= e; ++i) {
    uint32_t sa = ct->flash_address + flashintelp30_sector_offset(ct, i);
    while (1) {
      chip_wr_word(sa, 0x90);
      if (chip_rd_word(sa + 4) == cmd)
	break;
      chip_wr_word(sa, 0x60);
      chip_wr_word(sa, ucmd);
    }
  }
}

/* The actual programming function.
 * NOTE: pos need to be 2 byte aligned!
 */
static uint32_t
flashintelp30_prog(void *chip_descr,
                  uint32_t pos, unsigned char *data, uint32_t cnt)
{
  chiptype_t *ct = (chiptype_t *) chip_descr;
  uint32_t word_cnt;
  uint32_t n;
  uint16_t status;

#define USE_BUFFERED_PROGRAM
  flashintelp30_lock(ct, pos, cnt, 0);

  word_cnt = (cnt + 1) / 2;
  for (n = 0; n < word_cnt; ++n) {
    uint16_t d = ((uint16_t *) (void*) data)[n];

#ifdef USE_BUFFERED_PROGRAM
    {
      uint32_t sa = flashintelp30_sector_address(ct, pos);
      int maxcnt;

      chip_wr_word(sa, 0xe8);
      /*
       * Wait for WSM ready
       */
      do {
        status = chip_rd_word(ct->flash_address);
      }
      while (!(status & 0x80));
      maxcnt = ct->wbuf_mask + 1 - ((pos / 2) & ct->wbuf_mask);
      if (maxcnt > word_cnt - n)
	maxcnt = word_cnt - n;
      chip_wr_word(sa, maxcnt - 1);
      /* write_word on host assumes host endianess so will perform a
       * byte order swap if host is little endian.  we need to swap if little
       * endian such that the swap will just swap back to the correct endianess
       */
      if (is_little_endian()) {
	do {
	  d = ((uint16_t *) (void*) data)[n];
	  d = swap16(d);
	  chip_wr_word(pos, d);
	  pos += 2;
	  n += 1;
	}
	while (--maxcnt);
      } else {
	do {
	  d = ((uint16_t *) (void*) data)[n];
	  chip_wr_word(pos, d);
	  pos += 2;
	  n += 1;
	}
	while (--maxcnt);
      }
      /*
       * Confirm Write
       */
      chip_wr_word(sa, 0xd0);
      /*
       * Wait for program operation to finish
       */
      do {
        chip_wr_word(ct->flash_address, 0x70);
        status = chip_rd_word(ct->flash_address);
      }
      while (!(status & 0x80));
if (status != 0x80) {
#if HOST_FLASHING
	printf("unexpected status %x\n", status);
#endif
	return status;
}
      n -= 1; /* compensate for ++n in outer loop */
    }
#else
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
if (status != 0x80) {
#if HOST_FLASHING
	printf("unexpected status %x\n", status);
#endif
	return status;
}

    pos += 2;
#endif
  }

  chip_wr_word(ct->flash_address, 0xff);

  return cnt;
}

static void
flashintelp30_erase(void *chip_descr, int32_t sector_address)
{
  chiptype_t *ct = (chiptype_t *) chip_descr;
  uint16_t status;
  int i;
#if HOST_FLASHING
  int spin = 0;
#endif

  if(sector_address == -1) {
    flashintelp30_lock(ct, ct->flash_address, ct->flash_size, 0);

    /* erasing all sectors */
    for(i = 0; i < ct->num_sectors; ++i) {
      uint32_t off = flashintelp30_sector_offset(ct, i);

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
    uint32_t off = flashintelp30_sector_offset(ct, sector_address);
    flashintelp30_lock(ct, ct->flash_address + off, 1, 0);

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
flashintelp30_blank_chk(void *chip_descr, int32_t sector_address)
{
#if HOST_FLASHING
  printf("intelp30: blank_chk not implemented!\n");
#endif

  return 0;
}

/* wait for queued erasing operations to finish
 */
static int
flashintelp30_erase_wait(void *chip_descr)
{
#if HOST_FLASHING
  printf("intelp30: no wait needed!\n");
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
flashintelp30_search_chip(void *chip_descr, char *description, uint32_t pos)
{
  uint32_t size = 0;
  uint32_t m, d;
  chiptype_t *ct = (chiptype_t *) chip_descr;
  const chip_t *chip;
  int i;
  int bufsizlog;

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

      /* read the log2 of the write buffer size */
      chip_wr_word(pos, 0x90);
      bufsizlog = chip_rd_word(pos + 0x54);
      ct->wbuf_mask = (1 << (bufsizlog - 1)) - 1;
printf("bufsizlog = %d, bufsizmask = %d\n", bufsizlog, ct->wbuf_mask);

      if (description) {
        sprintf(description, "%10s @ 0x%08lx..0x%08lx "
                "manuf:0x%02lx device:0x%04lx size:0x%08lx",
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
  bdmWriteLongWord(adr, ct->wbuf_mask);
  adr += 4;

  return adr;
}

void
init_flashintelp30(int num)
{
  register_algorithm(num, driver_magic, sizeof(chiptype_t),
                     download_struct,
                     flashintelp30_search_chip,
                     flashintelp30_erase,
                     flashintelp30_blank_chk,
                     flashintelp30_erase_wait, flashintelp30_prog, prog_entry);
}

#else

void
init_flashintelp30(int num)
{
  register_algorithm(num, driver_magic, 0,
                     0,
                     0,
                     flashintelp30_erase,
                     flashintelp30_blank_chk,
                     flashintelp30_erase_wait, flashintelp30_prog, 0);
}

#endif
