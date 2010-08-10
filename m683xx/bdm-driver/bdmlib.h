/*
 * $Id: bdmlib.h,v 1.1 2003/06/04 01:31:31 ppisa Exp $
 */

#include	<sys/types.h>
#include 	<stdarg.h>
#include 	<stdio.h>

typedef u_short bdmstatus;

extern unsigned int bdmlib_delay, bdmlib_respw;

extern int bdmlib_open(char *device);
extern int bdmlib_close(int);
extern int bdmlib_isopen(void);
extern int bdmlib_ioctl(u_int code);
extern int bdmlib_setioctl(u_int code, u_int val);
extern bdmstatus bdmlib_getstatus(void);
extern int bdmlib_write_var(caddr_t adr, u_short size, u_int val);
extern int bdmlib_read_var(caddr_t adr, u_short size, void *val);
extern int bdmlib_write_block(caddr_t adr, u_int size, u_char *block);
extern int bdmlib_read_block(caddr_t adr, u_int size, u_char *block);
extern int bdmlib_load(char *file, u_long *entry_pt);
extern int bdmlib_do_load_binary(char *file_name, u_long *entry_pt);
extern int bdmlib_do_load_macro(char *file_name, int is_begin_macro);
extern int bdmlib_get_sys_reg(u_int, u_int *);
extern int bdmlib_set_sys_reg(u_int, u_int);
extern int bdmlib_get_reg(u_int, u_int *);
extern int bdmlib_set_reg(u_int, u_int);
extern int bdmlib_get_mbar(u_int, u_int *);
extern int bdmlib_set_mbar(u_int, u_int);
extern int bdmlib_go(void);
extern char *bdmlib_geterror_str(int);
extern char *bdmlib_getstatus_str(bdmstatus);
extern int bdmlib_reset(void);
extern void bdmlib_setdebug(int switch_on);
extern int bdmlib_querydebug(void);
extern void bdmlib_showpc(void);
extern void bdmlib_log(const char *format, ...);
extern void bdmlib_propeller(FILE * fp);
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

