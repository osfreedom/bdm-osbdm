/*
 * $Id: bdmfilt.h,v 1.1 2003/11/10 23:25:00 ppisa Exp $
 */

#ifndef BDMFILT_H
#define BDMFILT_H

#include <sys/types.h>
#include <bfd.h>

/* support of filtered write for flash programming */

typedef struct bdmlib_bfilt{
	struct bdmlib_bfilt *next;	
	caddr_t begin_adr;
	caddr_t end_adr;
	int filt_id;
	u_int flags;
	int (*wrb_filt)(struct bdmlib_bfilt *, caddr_t , u_int, u_char * );
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
bdmlib_wrb_filt(bdmlib_bfilt_t * bdmlib_bfilt, caddr_t in_adr,
		u_int size, u_char * bl_ptr);

#endif /* BDMFILT_H */
