#ifndef BDMFLASH_H
#define BDMFLASH_H

#include <sys/types.h>

#define FLASH_ALG_BITS_x8    0
#define FLASH_ALG_BITS_x16   2
#define FLASH_ALG_BITS_x8x2  3
#define FLASH_ALG_BITS_x32   4
#define FLASH_ALG_BITS_x16x2 5
#define FLASH_ALG_BITS_x8x4  7

#define WITH_TARGET_BUS32

#ifndef WITH_TARGET_BUS32
typedef u_int16_t flash_d_t;	/* Type able to store one flash location */
#else /* WITH_TARGET_BUS32 */
typedef u_int32_t flash_d_t;	/* Type able to store one flash location */
#endif /* WITH_TARGET_BUS32 */

/* Structure describing programming operations for flash type */
typedef struct flash_alg_info {
  /* Sets retid to manufacturer and type ID, returns <0 in case of error */
  int (*check_id)(const struct flash_alg_info *alg, void *addr, flash_d_t retid[2]);
  /* Programs one location of flash and returns number of programmed bytes */
  int (*prog)(const struct flash_alg_info *alg, void *addr, const void *data, long count);
  /* Erase all sectors overlaped by region from addr of size bytes, size=0 => erase all */
  /* This version is capable only of full erase (size=0) and one sector (size=1) */
  int (*erase)(const struct flash_alg_info *alg, void *addr, long size);
  /* Numeric and string fields follows */
  u_int32_t addr_mask;	/* Mask to take offset inside flash */
  u_int32_t reg1_addr;	/* Flash control register 1 */
  u_int32_t reg2_addr;	/* Flash control register 2 */
  u_int32_t sec_size;	/* block size of bigger blocks */
  flash_d_t width;	/* FLASH_ALG_BITS_x8 .. 8 bit data bus,
			   FLASH_ALG_BITS_x16 .. 16 bit data,
  			   FLASH_ALG_BITS_x8x2 .. two interleaved 8 bit */
  flash_d_t cmd_unlock1;/* first byte of command sequence */
  flash_d_t cmd_unlock2;/* second byte of command sequence */
  flash_d_t cmd_rdid;	/* read identifier */
  flash_d_t cmd_prog;	/* program one loc */
  flash_d_t cmd_erase;	/* erase command */
  flash_d_t cmd_reset;	/* leave program mode */
  flash_d_t erase_all;	/* erase all */
  flash_d_t erase_sec;	/* erase sector */
  flash_d_t fault_bit;	/* programing of location failed */
  flash_d_t manid;	/* manufacturer ID */
  flash_d_t devid;	/* device ID */
  char *alg_name;	/* informative flash type name */
} flash_alg_info_t;

int bdmflash_check_id(const flash_alg_info_t *alg, void *addr,
		   flash_d_t retid[2]);

int bdmflash_prog(const flash_alg_info_t *alg, void *addr, const void *data, long count);

int bdmflash_erase(const flash_alg_info_t *alg, void *addr, long size);

flash_alg_info_t **flash_alg_infos;

const flash_alg_info_t *bdmflash_alg_from_id(flash_d_t id[2]);

const flash_alg_info_t *bdmflash_alg_probe(caddr_t flash_adr);

int bdmflash_wrb_filt(bdmlib_bfilt_t * filt, caddr_t in_adr,
		u_int size, u_char * bl_ptr);

int bdmflash_erase_filt(bdmlib_bfilt_t * filt, caddr_t in_adr, u_int size);

int bdmflash_blankck_filt(bdmlib_bfilt_t * filt, caddr_t in_adr, u_int size);

int bdmflash_check_id(const flash_alg_info_t *alg, void *addr, flash_d_t retid[2]);

#endif /* BDMFLASH_H */
