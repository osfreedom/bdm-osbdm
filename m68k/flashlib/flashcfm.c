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

#include "flashcfm.h"
#include "flash_filter.h"
#include <stdint.h>

#if HOST_FLASHING
# include <inttypes.h>
# include <stdio.h>
# include <BDMlib.h>
# include <string.h>
#endif

#define MCF_CFM_CFMCLKD_DIVLD                (0x80)

#define MCF_CFM_CFMUSTAT_BLANK               (0x04)
#define MCF_CFM_CFMUSTAT_CCIF                (0x40)
#define MCF_CFM_CFMUSTAT_CBEIF               (0x80)

#define MCF_CFM_CFMCMD_BLANK_CHECK           (0x5)
#define MCF_CFM_CFMCMD_PAGE_ERASE_VERIFY     (0x6)
#define MCF_CFM_CFMCMD_WORD_PROGRAM          (0x20)
#define MCF_CFM_CFMCMD_PAGE_ERASE            (0x40)
#define MCF_CFM_CFMCMD_MASS_ERASE            (0x41)

/* 
 * Warning! do not change this struct without changing the download_struct
 * function.
 */
typedef struct
{
  uint32_t backdoor_address_start;
  uint32_t backdoor_address_end;
  uint32_t flash_address;
  uint32_t flash_size;
  uint32_t ipsbar;
  uint32_t cfmclkd;
  uint32_t cfmustat;
  uint32_t cfmcmd;
} chiptype_t;

#if HOST_FLASHING

static inline uint8_t
read_byte(uint32_t a_addr)
{
  unsigned char result;

  if (bdmReadByte(a_addr, &result) < 0)
    fprintf(stderr, "read_byte(0x%08x): %s\n", a_addr, bdmErrorString());
  return (uint8_t) result;
}

static inline uint16_t
read_word(uint32_t a_addr)
{
  unsigned short result;

  if (bdmReadWord(a_addr, &result) < 0)
    fprintf(stderr, "read_word(0x%08x): %s\n", a_addr, bdmErrorString());
  return (uint16_t) result;
}

static inline uint32_t
read_longword(uint32_t a_addr)
{
  unsigned long result;

  if (bdmReadLongWord(a_addr, &result) < 0)
    fprintf(stderr, "read_longword(0x%08x): %s\n", a_addr, bdmErrorString());
  return (uint32_t) result;
}

static inline void
write_byte(uint32_t a_addr, uint8_t a_x)
{
  if (bdmWriteByte(a_addr, a_x) < 0)
    fprintf(stderr, "write_byte(0x%08x): %s\n", a_addr, bdmErrorString());
}

static inline void
write_word(uint32_t a_addr, uint16_t a_x)
{
  if (bdmWriteWord(a_addr, a_x) < 0)
    fprintf(stderr, "write_word(0x%08x): %s\n", a_addr, bdmErrorString());
}

static inline void
write_longword(uint32_t a_addr, uint32_t a_x)
{
  if (bdmWriteLongWord(a_addr, a_x) < 0)
    fprintf(stderr, "write_longword(0x%08x): %s\n", a_addr, bdmErrorString());
}

#else

# define read_byte(A) (*((volatile uint8_t *) (A)))
# define read_word(A) (*((volatile uint16_t *) (A)))
# define read_longword(A) (*((volatile uint32_t *) (A)))
# define write_byte(A, B) *((volatile uint8_t *)(A)) = (B)
# define write_word(A, B) *((volatile uint16_t *)(A)) = (B)
# define write_longword(A, B) *((volatile uint32_t *)(A)) = (B)

#endif

static inline uint32_t
swap32(uint32_t a_v)
{
  return (uint32_t) (((a_v >> 24) & 0x000000ff) | ((a_v << 24) & 0xff000000) |
                     ((a_v >> 8) & 0x0000ff00) | ((a_v << 8) & 0x00ff0000));
}

static inline int 
is_little_endian(void)
{
  uint32_t i = 1;
  return (int) *((uint8_t *) &i);
}

/* We need a unique symbol to associate a (target-based) plugin with its
   (host-based) driver.
*/
static char driver_magic[] = "flashcfm";

#if HOST_FLASHING
static char *
prog_entry(void)
{
  return "flashcfm_prog";
}
#endif

/* The actual programming function.
 * NOTE: pos and cnt need to be 4 byte aligned!
 */
static uint32_t
flashcfm_prog(void *chip_descr,
              uint32_t pos, unsigned char *data, uint32_t cnt)
{
  chiptype_t *ct = (chiptype_t *) chip_descr;
  uint32_t offset = ct->backdoor_address_start + (pos - ct->flash_address);
  uint32_t n = 0;

  cnt /= 4;

  if (read_byte(ct->cfmclkd) & MCF_CFM_CFMCLKD_DIVLD) {
    while (!(read_byte(ct->cfmustat) & MCF_CFM_CFMUSTAT_CBEIF)) {
    }

    for (n = 0; n < cnt; ++n) {
      uint32_t d = ((uint32_t *) data)[n];

      /* write_longword on host assumes host endianess so will perform a
       * byte order swap if host is little endian.  we need to swap if little
       * endian such that the swap will just swap back to the correct endianess
       */
      if(is_little_endian())
        d = swap32(d);
      write_longword(offset + n * 4, d);
      
      write_byte(ct->cfmcmd, MCF_CFM_CFMCMD_WORD_PROGRAM);
      write_byte(ct->cfmustat, MCF_CFM_CFMUSTAT_CBEIF);

      if (read_byte(ct->cfmustat) & MCF_CFM_CFMUSTAT_CBEIF) {
        continue;
      }

      while (!(read_byte(ct->cfmustat) & MCF_CFM_CFMUSTAT_CCIF)) {
      }
    }
  }

  return n * 4;
}

/* Initiate erase operation. Sector address is relative to chip-base.
   With sector address==-1, the whole chip is erased. The erase operation
   can be called several times before flash_29_erase_wait() is called for
   simultanous erasure of multiple sectors.
 */
static void
flashcfm_erase(void *chip_descr, int32_t sector_address)
{
  chiptype_t *ct = (chiptype_t *) chip_descr;

  if (read_byte(ct->cfmclkd) & MCF_CFM_CFMCLKD_DIVLD) {
    while (!(read_byte(ct->cfmustat) & MCF_CFM_CFMUSTAT_CBEIF)) {
    }

    if (sector_address == -1) {
      write_longword(ct->backdoor_address_start, 0);
      write_byte(ct->cfmcmd, MCF_CFM_CFMCMD_MASS_ERASE);
    } else {
      write_longword(ct->backdoor_address_start +
                     (sector_address - ct->flash_address), 0);
      write_byte(ct->cfmcmd, MCF_CFM_CFMCMD_PAGE_ERASE);
    }
    write_byte(ct->cfmustat, MCF_CFM_CFMUSTAT_CBEIF);
  }
}

/* Blank check operation. Sector address is relative to chip-base.
   With sector address==-1, the whole chip is checked.
 */
static int
flashcfm_blank_chk(void *chip_descr, int32_t sector_address)
{
  chiptype_t *ct = (chiptype_t *) chip_descr;
  int result = 0;

  if (read_byte(ct->cfmclkd) & MCF_CFM_CFMCLKD_DIVLD) {
    while (!(read_byte(ct->cfmustat) & MCF_CFM_CFMUSTAT_CBEIF)) {
    }

    if (sector_address == -1) {
      write_longword(ct->backdoor_address_start, 0);
      write_byte(ct->cfmcmd, MCF_CFM_CFMCMD_BLANK_CHECK);
    } else {
      write_longword(ct->backdoor_address_start +
                     (sector_address - ct->flash_address), 0);
      write_byte(ct->cfmcmd, MCF_CFM_CFMCMD_PAGE_ERASE_VERIFY);
    }
    write_byte(ct->cfmustat, MCF_CFM_CFMUSTAT_CBEIF);

    while (!(read_byte(ct->cfmustat) & MCF_CFM_CFMUSTAT_CCIF)) {
    }

    if (read_byte(ct->cfmustat) & MCF_CFM_CFMUSTAT_BLANK) {
      result = 1;
      write_byte(ct->cfmustat, MCF_CFM_CFMUSTAT_BLANK);
    }
  }

  return result;
}

/* wait for queued erasing operations to finish
 */
static int
flashcfm_erase_wait(void *chip_descr)
{
  chiptype_t *ct = (chiptype_t *) chip_descr;
#if HOST_FLASHING
  int spin = 0;
#endif        

  if (read_byte(ct->cfmclkd) & MCF_CFM_CFMCLKD_DIVLD) {
    while (!(read_byte(ct->cfmustat) & MCF_CFM_CFMUSTAT_CCIF)) {
#if HOST_FLASHING
      spin = flash_spin(spin);
#endif        
    }
    return 1;
  }
  return 0;
}

#if HOST_FLASHING

/* autodetect
 */
static uint32_t
flashcfm_search_chip(void *chip_descr, char *description, uint32_t pos)
{
  chiptype_t *ct = (chiptype_t *) chip_descr;

  unsigned long flashbar = 0;
  uint32_t fbar_reg;
  
  flash_get_var("%flashbar", &fbar_reg, 0x0c04);

  if (bdmReadControlRegister(fbar_reg, &flashbar) >= 0) {
    if ((flashbar & 1) && ((flashbar & 0xfff80000) == pos)) {

      flash_get_var("IPSBAR", &ct->ipsbar, 0x40000000);
      flash_get_var("FLASH_BACKDOOR", &ct->backdoor_address_start, 0x04000000);
      flash_get_var("FLASH_SIZE", &ct->flash_size, 256 * 1024);

      ct->backdoor_address_start += ct->ipsbar;
      ct->backdoor_address_end = ct->backdoor_address_start + 
        ct->flash_size;
      
      flash_get_var("MCF_CFM_CFMCLKD", &ct->cfmclkd, 0x1D0002);
      flash_get_var("MCF_CFM_CFMUSTAT", &ct->cfmustat, 0x1D0020);
      flash_get_var("ct->cfmcmd", &ct->cfmcmd, 0x1D0024);
      
      ct->cfmclkd += ct->ipsbar;
      ct->cfmustat += ct->ipsbar;
      ct->cfmcmd += ct->ipsbar;
      
      ct->flash_address = pos;

      if (description) {
        sprintf(description, "Coldfire flash module @ 0x%08" PRIx32 \
                "..0x%08" PRIx32 " size:0x%08" PRIx32,
                pos, pos + ct->flash_size, ct->flash_size);
      }

      return ct->flash_size;
    }
  }
  return 0;
}

static int
download_struct(void *chip_descr, uint32_t adr)
{
  chiptype_t *ct = (chiptype_t *) chip_descr;

  bdmWriteLongWord(adr, ct->backdoor_address_start);
  adr += 4;
  bdmWriteLongWord(adr, ct->backdoor_address_end);
  adr += 4;
  bdmWriteLongWord(adr, ct->flash_address);
  adr += 4;
  bdmWriteLongWord(adr, ct->flash_size);
  adr += 4;
  bdmWriteLongWord(adr, ct->ipsbar);
  adr += 4;
  bdmWriteLongWord(adr, ct->cfmclkd);
  adr += 4;
  bdmWriteLongWord(adr, ct->cfmustat);
  adr += 4;
  bdmWriteLongWord(adr, ct->cfmcmd);
  adr += 4;
  
  return adr;
}

void
init_flashcfm(int num)
{
  register_algorithm(num, driver_magic, sizeof(chiptype_t),
                     download_struct,
                     flashcfm_search_chip,
                     flashcfm_erase,
                     flashcfm_blank_chk,
                     flashcfm_erase_wait, flashcfm_prog, prog_entry);
}
#else
void
init_flashcfm(int num)
{
  register_algorithm(num, driver_magic, 0,
                     0,
                     0,
                     flashcfm_erase,
                     flashcfm_blank_chk,
                     flashcfm_erase_wait, flashcfm_prog, 0);
}
#endif
