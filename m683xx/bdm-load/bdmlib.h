/*
 * $Id: bdmlib.h,v 1.3 2009/04/26 18:48:28 ppisa Exp $
 */

#ifndef BDMLIB_H
#define BDMLIB_H

#include	<inttypes.h>
#include 	<stdarg.h>
#include 	<stdio.h>

typedef uint16_t bdmstatus;

extern int bdmlib_open(char *device);
extern int bdmlib_close(int);
extern int bdmlib_isopen(void);
extern int bdmlib_ioctl(uint32_t code);
extern int bdmlib_setioctl(uint32_t code, uint32_t val);
extern bdmstatus bdmlib_getstatus(void);
extern int bdmlib_write_var(caddr_t adr, uint16_t size, uint32_t val);
extern int bdmlib_read_var(caddr_t adr, uint16_t size, void *val);
extern int bdmlib_write_block(caddr_t adr, uint32_t size, uint8_t *block);
extern int bdmlib_read_block(caddr_t adr, uint32_t size, uint8_t *block);
extern int bdmlib_load(char *file, char *entry_name, uint32_t *entry_pt);
extern int bdmlib_do_load_binary(char *file_name, char *entry_name,
				 uint32_t *entry_pt);
extern int bdmlib_do_load_macro(char *file_name, int is_begin_macro);
extern int bdmlib_get_sys_reg(uint32_t, uint32_t *);
extern int bdmlib_set_sys_reg(uint32_t, uint32_t);
extern int bdmlib_get_reg(uint32_t, uint32_t *);
extern int bdmlib_set_reg(uint32_t, uint32_t);
extern int bdmlib_go(void);
extern char *bdmlib_geterror_str(int);
extern char *bdmlib_getstatus_str(bdmstatus);
extern int bdmlib_set_mbar(uint32_t mbar_val);
extern int bdmlib_reset(void);
extern void bdmlib_setdebug(int switch_on);
extern int bdmlib_querydebug(void);
extern void bdmlib_showpc(void);
extern void bdmlib_log(const char *format, ...);
extern void bdmlib_propeller(uint32_t addr, FILE * fp);
extern int bdmlib_do_load_binary_section(char *file_name, char *sect_name);

/* some additional error codes beyond those of the driver */

#define BDM_ERR_NOT_OPEN	-650
#define BDM_ERR_ILL_IOCTL	-651
#define BDM_ERR_WRITE_FAIL	-652
#define BDM_ERR_READ_FAIL	-653
#define BDM_ERR_ILL_SIZE	-654
#define BDM_ERR_OPEN		-655
#define BDM_ERR_LOAD		-656
#define BDM_ERR_MACROFILE	-657
#define BDM_ERR_SECTION 	-658
#define BDM_ERR_VERSION 	-659
#define BDM_NO_ERROR		0

/* support of filtered write for flash programming */

typedef struct bdmlib_bfilt{
	struct bdmlib_bfilt *next;	
	caddr_t begin_adr;
	caddr_t end_adr;
	int filt_id;
	uint32_t flags;
	int (*wrb_filt)(struct bdmlib_bfilt *, caddr_t , uint32_t, uint8_t * );
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
		uint32_t size, uint8_t * bl_ptr);

#endif /* BDMLIB_H */
