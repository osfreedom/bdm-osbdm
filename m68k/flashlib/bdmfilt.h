/*
 * $Id: bdmfilt.h,v 1.4 2008/06/16 00:01:21 cjohns Exp $
 */

#ifndef BDMFILT_H
#define BDMFILT_H

#include <sys/types.h>
#include <stdint.h>

/* support of filtered write for flash programming */

typedef struct bdmlib_bfilt{
	struct bdmlib_bfilt *next;
	uint32_t begin_adr;
	uint32_t end_adr;
	int filt_id;
	unsigned int flags;
	int (*wrb_filt)(struct bdmlib_bfilt *, uint32_t , unsigned int, unsigned char * );
	void *info;
	void *state;
}bdmlib_bfilt_t;

#define BDMLIB_FILT_ERROR  0x08
#define BDMLIB_FILT_ERASED 0x10
#define BDMLIB_FILT_AUTO   0x20
#define BDMLIB_FILT_FLASH  0x40

int bdmlib_load_use_lma;	/* use LMA instead of VMA for load */
bdmlib_bfilt_t * bdmlib_bfilt;

int
bdmlib_wrb_filt(bdmlib_bfilt_t * bdmlib_bfilt, uint32_t in_adr,
                unsigned int size, unsigned char * bl_ptr);

#endif /* BDMFILT_H */
