/*******************************************************************
  Flash programming algorithms for use with BDM flash utility

  bdmflash.c      - test driver implementation

  (C) Copyright 2000 by  Pavel Pisa <pisa@waltz.felk.cvut.cz>

  This souce is distributed under the Gnu General Public Licence.
  See file COPYING for details.

  Source could be used under any other license for embeded systems,
  but in such case enhancements must be sent to original author.
  If some of contributors does not agree with above two lines,
  he can delete them and only GPL will apply.
 *******************************************************************/
/*
  Revision history

    2001-01-16	Added experimental support of single 8 bit flash memory
		based on Avi Cohen Stuart <astuart@baan.nl> changes

    2001-07-10	Added support for amd_29f010 with x8x2 configuration
		based on John S. Gwynne <jsg@jsgpc.mrcday.com> changes
		Trying to solve x8x2 race condition with shortest 
		possible code - needs more tests

    2001-08-30	Added alg-info for at_49f040 with x8x2 configuration
		x8x2 configuration needs more testing, may be broken

    2003-05-19	Added initial support for x32 memory configurations.
		The x16x2 and x8x4 should work as well. Changes were
		motivated by Andrei Smirnov <andrei.s@myrealbox.com>
		code, but his small update results in deeper code redesign.
		This could enable to support page writes for modern versions
		of FLASH memories in future.
*/

#include <sys/types.h>
#include "bdm.h"
#include "bdmlib.h"
#include "bdmflash.h"

/* predefined flash algorithms metods */

int bdmflash_check_id_x32(const flash_alg_info_t *alg, void *addr, flash_d_t retid[2]);
int bdmflash_prog_x32(const flash_alg_info_t *alg, void *addr, const void *data, long count);
int bdmflash_erase_x32(const flash_alg_info_t *alg, void *addr, long size);

int bdmflash_check_id_x16(const flash_alg_info_t *alg, void *addr, flash_d_t retid[2]);
int bdmflash_prog_x16(const flash_alg_info_t *alg, void *addr, const void *data, long count);
int bdmflash_erase_x16(const flash_alg_info_t *alg, void *addr, long size);

int bdmflash_check_id_x8(const flash_alg_info_t *alg, void *addr, flash_d_t retid[2]);
int bdmflash_prog_x8(const flash_alg_info_t *alg, void *addr, const void *data, long count);
int bdmflash_erase_x8(const flash_alg_info_t *alg, void *addr, long size);

/* predefined flash types */

flash_alg_info_t amd_29f800_x16={
  check_id:   bdmflash_check_id_x16,
  prog:       bdmflash_prog_x16,
  erase:      bdmflash_erase_x16,
  addr_mask:  0xfffff,	/* 1MB */
  reg1_addr:  0x555*2,
  reg2_addr:  0x2aa*2,
  sec_size:   0x10000,
  width:      FLASH_ALG_BITS_x16,
  cmd_unlock1:0xaa,	/* reg1 */
  cmd_unlock2:0x55,	/* reg2 */
  cmd_rdid:   0x90,	/* reg1 */
  cmd_prog:   0xa0,	/* reg1 */
  cmd_erase:  0x80,	/* reg1 */
  cmd_reset:  0xf0,	/* any */
  erase_all:  0x10,	/* reg1 */
  erase_sec:  0x30,	/* sector */
  fault_bit:  0x20,
  manid:      1,
  devid:      0x2258,
  alg_name:   "amd29f800"
};

flash_alg_info_t amd_29f400_x16={
  check_id:   bdmflash_check_id_x16,
  prog:       bdmflash_prog_x16,
  erase:      bdmflash_erase_x16,
  addr_mask:  0x7ffff,	/* 0.5MB */
  reg1_addr:  0x555*2,
  reg2_addr:  0x2aa*2,
  sec_size:   0x10000,
  width:      FLASH_ALG_BITS_x16,
  cmd_unlock1:0xaa,	/* reg1 */
  cmd_unlock2:0x55,	/* reg2 */
  cmd_rdid:   0x90,	/* reg1 */
  cmd_prog:   0xa0,	/* reg1 */
  cmd_erase:  0x80,	/* reg1 */
  cmd_reset:  0xf0,	/* any */
  erase_all:  0x10,	/* reg1 */
  erase_sec:  0x30,	/* sector */
  fault_bit:  0x20,
  manid:      1,
  devid:      0x22ab,
  alg_name:   "amd29f400"
};

flash_alg_info_t amd_29f040_x8={
  check_id:   bdmflash_check_id_x8,
  prog:       bdmflash_prog_x8,
  erase:      bdmflash_erase_x8,
  addr_mask:  0x7ffff,	/* 0.5MB */
  reg1_addr:  0x555,
  reg2_addr:  0x2aa,
  sec_size:   0x10000,
  width:      FLASH_ALG_BITS_x8,
  cmd_unlock1:0xaa,	/* reg1 */
  cmd_unlock2:0x55,	/* reg2 */
  cmd_rdid:   0x90,	/* reg1 */
  cmd_prog:   0xa0,	/* reg1 */
  cmd_erase:  0x80,	/* reg1 */
  cmd_reset:  0xf0,	/* any */
  erase_all:  0x10,	/* reg1 */
  erase_sec:  0x30,	/* sector */
  fault_bit:  0x20,
  manid:      1,
  devid:      0xa4,
  alg_name:   "amd29f040"
};

flash_alg_info_t amd_29f010_x8x2={
  check_id:   bdmflash_check_id_x16,
  prog:       bdmflash_prog_x16,
  erase:      bdmflash_erase_x16,
  addr_mask:  0x3ffff,	/* 2x128kB = 256kB */
  reg1_addr:  0x5555*2,
  reg2_addr:  0x2aaa*2,
  sec_size:   0x10000,
  width:      FLASH_ALG_BITS_x8x2,
  cmd_unlock1:0xaaaa,	/* reg1 */
  cmd_unlock2:0x5555,	/* reg2 */
  cmd_rdid:   0x9090,	/* reg1 */
  cmd_prog:   0xa0a0,	/* reg1 */
  cmd_erase:  0x8080,	/* reg1 */
  cmd_reset:  0xf0f0,	/* any */
  erase_all:  0x1010,	/* reg1 */
  erase_sec:  0x3030,	/* sector */
  fault_bit:  0x2020,
  manid:      0x0101,
  devid:      0x2020,
  alg_name:   "amd29f010x2"
};

flash_alg_info_t at_49f040_x8x2={
  check_id:   bdmflash_check_id_x16,
  prog:       bdmflash_prog_x16,
  erase:      bdmflash_erase_x16,
  addr_mask:  0xfffff,	/* 2x0.5MB = 1MB */
  reg1_addr:  0x5555*2,
  reg2_addr:  0x2aaa*2,
  sec_size:   0x10000,
  width:      FLASH_ALG_BITS_x8x2,
  cmd_unlock1:0xaaaa,	/* reg1 */
  cmd_unlock2:0x5555,	/* reg2 */
  cmd_rdid:   0x9090,	/* reg1 */
  cmd_prog:   0xa0a0,	/* reg1 */
  cmd_erase:  0x8080,	/* reg1 */
  cmd_reset:  0xf0f0,	/* any */
  erase_all:  0x1010,	/* reg1 */
  erase_sec:  0x3030,	/* sector */
  fault_bit:  0x2020,
  manid:      0x1F1F,
  devid:      0x1313,
  alg_name:   "at49f040x2"
};

#ifdef WITH_TARGET_BUS32

flash_alg_info_t amd_29f080_x8x4={
  check_id:   bdmflash_check_id_x32,
  prog:       bdmflash_prog_x32,
  erase:      bdmflash_erase_x32,
  addr_mask:  0x3fffff,	/* 4x1MB */
  reg1_addr:  0x555*4,
  reg2_addr:  0x2aa*4,
  sec_size:   0x10000,
  width:      FLASH_ALG_BITS_x8x4,
  cmd_unlock1:0xaaaaaaaa,	/* reg1 */
  cmd_unlock2:0x55555555,	/* reg2 */
  cmd_rdid:   0x90909090,	/* reg1 */
  cmd_prog:   0xa0a0a0a0,	/* reg1 */
  cmd_erase:  0x80808080,	/* reg1 */
  cmd_reset:  0xf0f0f0f0,	/* any */
  erase_all:  0x10101010,	/* reg1 */
  erase_sec:  0x30303030,	/* sector */
  fault_bit:  0x20202020,
  manid:      0x01010101,
  devid:      0xd5d5d5d5,
  alg_name:   "amd29f080x4"
};

#endif /* WITH_TARGET_BUS32 */

/* please keep sorted by flash sizes, smallest first */
flash_alg_info_t *flash_alg_infos_def[]={
  &amd_29f010_x8x2,	/* 256 kB */
  &amd_29f040_x8,	/* 512 kB */
  &amd_29f400_x16,	/* 512 kB */
  &amd_29f800_x16,	/* 1 MB */
  &at_49f040_x8x2,	/* 1 MB */
 #ifdef WITH_TARGET_BUS32
  &amd_29f080_x8x4,	/* 4 MB */
 #endif /* WITH_TARGET_BUS32 */
  NULL
};

flash_alg_info_t **flash_alg_infos=flash_alg_infos_def;

#if 0
  #define FLASH_WR32(adr,val) (*(volatile u_int32_t*)(adr)=(val))
  #define FLASH_RD32(adr) (*(volatile u_int32_t*)(adr))
  #define FLASH_WR16(adr,val) (*(volatile u_int16_t*)(adr)=(val))
  #define FLASH_RD16(adr) (*(volatile u_int16_t*)(adr))
  #define FLASH_WR8(adr,val) (*(volatile u_int8_t*)(adr)=(val))
  #define FLASH_RD8(adr) (*(volatile u_int8_t*)(adr))
#else
  #define FLASH_WR32(adr,val) \
  		({ \
  		  if(bdmlib_write_var(adr,BDM_SIZE_LONG,val)<0) \
  		  	goto mem_op_error; \
  		  val; \
  		})
  #define FLASH_RD32(adr) \
  		({ u_int32_t temp_val; \
  		  if(bdmlib_read_var(adr,BDM_SIZE_LONG,&temp_val)<0) \
  		  	goto mem_op_error; \
  		  temp_val; \
  		})
  #define FLASH_WR16(adr,val) \
  		({ \
  		  if(bdmlib_write_var(adr,BDM_SIZE_WORD,val)<0) \
  		  	goto mem_op_error; \
  		  val; \
  		})
  #define FLASH_RD16(adr) \
  		({ u_int16_t temp_val; \
  		  if(bdmlib_read_var(adr,BDM_SIZE_WORD,&temp_val)<0) \
  		  	goto mem_op_error; \
  		  temp_val; \
  		})
  #define FLASH_WR8(adr,val) \
  		({ \
  		  if(bdmlib_write_var(adr,BDM_SIZE_BYTE,val)<0) \
  		  	goto mem_op_error; \
  		  val; \
  		})
  #define FLASH_RD8(adr) \
  		({ u_int8_t temp_val; \
  		  if(bdmlib_read_var(adr,BDM_SIZE_BYTE,&temp_val)<0) \
  		  	goto mem_op_error; \
  		  temp_val; \
  		})
#endif



static  
int bdmflash_prepval_x16(u_int16_t *pval, void *addr, const void *data, long count)
{
  if(!((long)addr&1) && (count>=2)){
    *pval=(((u_char*)data)[0]<<8)|(((u_char*)data)[1]);
    return 2;
  }
  if(!count) return count;
  if(!((long)addr&1)){
    *pval=(((u_char*)data)[0]<<8) | FLASH_RD8((u_char*)addr+1);
  }else{
    *pval=(FLASH_RD8((u_char*)addr-1)<<8) | (((u_char*)data)[0]);
  }
  return 1;

  mem_op_error:
    return -4;
}

#ifdef WITH_TARGET_BUS32
static  
int bdmflash_prepval_x32(u_int32_t *pval, void *addr, const void *data, long count)
{
  int offs=(long)addr&3;
  int rest=4-offs;
  u_int32_t val=0;
  if(!offs && (count>=4)){
    *pval=((u_int32_t)((u_char*)data)[0]<<24)|((u_int32_t)((u_char*)data)[1]<<16)|
          (((u_char*)data)[2]<<8)|(((u_char*)data)[3]);
    return 4;
  }
  if(!count) return count;
  if(count>rest) count=rest;
  while(offs){
    val<<=8;
    val|=FLASH_RD8((u_char*)addr-offs);
    offs--;
  }
  while(offs<count){
    val<<=8;
    val|=((u_char*)data)[offs];
    offs++;
  }
  while(offs<rest){
    val<<=8;
    val|=FLASH_RD8((u_char*)addr+offs);
    offs++;
  }
  *pval=val;
  return count;

  mem_op_error:
    return -4;
}
#endif /* WITH_TARGET_BUS32 */

/*******************************************************************/
/* routines for 16 bit wide Intel or AMD flash or two interleaved 
   8 bit flashes */

int bdmflash_check_id_x16(const flash_alg_info_t *alg, void *addr, flash_d_t retid[2])
{
  int ret=0;
  u_int16_t devid, manid;
  caddr_t a=(caddr_t)((u_int32_t)addr&~alg->addr_mask);
  /* reset */
  FLASH_WR16(a,alg->cmd_reset);
  /* security sequence */
  FLASH_WR16(a+alg->reg1_addr,alg->cmd_unlock1);
  if(alg->cmd_unlock2)
    FLASH_WR16(a+alg->reg2_addr,alg->cmd_unlock2);
  /* read manufacturer ID */
  FLASH_WR16(a+alg->reg1_addr,alg->cmd_rdid);
  manid=FLASH_RD16(a+0);
  if(manid!=alg->manid) ret=-1;
  /* reset */
  FLASH_WR16(a,alg->cmd_reset);
  /* security sequence */
  FLASH_WR16(a+alg->reg1_addr,alg->cmd_unlock1);
  if(alg->cmd_unlock2)
    FLASH_WR16(a+alg->reg2_addr,alg->cmd_unlock2);
  /* read device ID */
  FLASH_WR16(a+alg->reg1_addr,alg->cmd_rdid);
  devid=FLASH_RD16(a+2);
  if(devid!=alg->devid) ret=-1;
  /* reset */
  FLASH_WR16(a,alg->cmd_reset);
  if(retid)
    {retid[0]=manid;retid[1]=devid;};
  return ret;

  mem_op_error:
    return -5;
}

int bdmflash_prog_x16(const flash_alg_info_t *alg, void *addr, const void *data, long count)
{
  int ret;
  u_int16_t old,new,fault,val;
  caddr_t a=(caddr_t)((u_int32_t)addr&~alg->addr_mask);
  ret=bdmflash_prepval_x16(&val,addr,data,count);
  if(ret<=0)
    return ret;
  addr=(void*)((long)addr&~1l);
  /* check if FLASH needs programming */
  if(1){
    old=FLASH_RD16(addr);
    if(old==val) return ret;
  }
  /* security sequence */
  FLASH_WR16(a+alg->reg1_addr,alg->cmd_unlock1);
  if(alg->cmd_unlock2)
    FLASH_WR16(a+alg->reg2_addr,alg->cmd_unlock2);
  /* program command */
  FLASH_WR16(a+alg->reg1_addr,alg->cmd_prog);
  FLASH_WR16(addr,val);
  /* wait for result */
  old=FLASH_RD16(addr);
  while((new=FLASH_RD16(addr))!=old){
    if((fault=old&new&alg->fault_bit)){
      old=new;
      /* check for some false fault at finish or race of x8x2 */
      if(!(old^=(new=FLASH_RD16(addr)))) break; /* finished now */
      else{
        /* one chip of x8x2 configuration can finish earlier */
        if(!(old&0x00ff)||(fault&0x00ff))
          if(!(old&0xff00)||(fault&0xff00)) 
            {ret=-2;break;}
      }
    }
    old=new;
  }
  /* reset */
  FLASH_WR16(a,alg->cmd_reset);
  if((FLASH_RD16(addr)!=val) && (ret>=0)) ret=-3;

  return ret;

  mem_op_error:
    return -5;
}

int bdmflash_erase_x16(const flash_alg_info_t *alg, void *addr, long size)
{
  u_int16_t old,new,fault;
  int ret=0;
  caddr_t a=(caddr_t)((u_int32_t)addr&~alg->addr_mask);
  /* reset */
  FLASH_WR16(a,alg->cmd_reset);
  /* security sequence */
  FLASH_WR16(a+alg->reg1_addr,alg->cmd_unlock1);
  if(alg->cmd_unlock2)
    FLASH_WR16(a+alg->reg2_addr,alg->cmd_unlock2);
  /* erase command */
  FLASH_WR16(a+alg->reg1_addr,alg->cmd_erase);
  /* security sequence */
  FLASH_WR16(a+alg->reg1_addr,alg->cmd_unlock1);
  if(alg->cmd_unlock2)
    FLASH_WR16(a+alg->reg2_addr,alg->cmd_unlock2);
  /* select erase range */
  a=addr;
  if(size==0)
    FLASH_WR16(a+alg->reg1_addr,alg->erase_all);
  else{
    FLASH_WR16(addr,alg->erase_sec);
  }  
  /* wait for result */
  old=FLASH_RD16(addr);
  while((new=FLASH_RD16(addr))!=old){
    if((fault=old&new&alg->fault_bit)){
      old=new;
      /* check for some false fault at finish or race of x8x2 */
      if(!(old^=(new=FLASH_RD16(addr)))) break; /* finished now */
      else{
        /* one chip of x8x2 configuration can finish earlier */
        if(!(old&0x00ff)||(fault&0x00ff))
          if(!(old&0xff00)||(fault&0xff00)) 
            {ret=-2;break;}
      }
    }
    old=new;
  }
  /* reset */
  FLASH_WR16(a,alg->cmd_reset);
  if(FLASH_RD16(addr)!=0xffff) ret--;    
  return ret;

  mem_op_error:
    return -5;
}

/*******************************************************************/
/* routines for single 8 bit wide Intel or AMD flash */

int bdmflash_check_id_x8(const flash_alg_info_t *alg, void *addr, 
		   flash_d_t retid[2])
{
  int ret=0;
  u_int16_t devid, manid;
  caddr_t a=(caddr_t)((u_int32_t)addr&~alg->addr_mask);
  /* reset */
  FLASH_WR8(a,alg->cmd_reset);
  /* security sequence */
  FLASH_WR8(a+alg->reg1_addr,alg->cmd_unlock1);
  if(alg->cmd_unlock2)
    FLASH_WR8(a+alg->reg2_addr,alg->cmd_unlock2);
  /* read manufacturer ID */
  FLASH_WR8(a+alg->reg1_addr,alg->cmd_rdid);
  manid=FLASH_RD8(a+0);
  if(manid!=alg->manid) ret=-1;
  /* reset */
  FLASH_WR8(a,alg->cmd_reset);
  /* security sequence */
  FLASH_WR8(a+alg->reg1_addr,alg->cmd_unlock1);
  if(alg->cmd_unlock2)
    FLASH_WR8(a+alg->reg2_addr,alg->cmd_unlock2);
  /* read device ID */
  FLASH_WR8(a+alg->reg1_addr,alg->cmd_rdid);
  devid=FLASH_RD8(a+1);
  if(devid!=alg->devid) ret=-1;
  /* reset */
  FLASH_WR8(a,alg->cmd_reset);
  if(retid)
    {retid[0]=manid;retid[1]=devid;};
  return ret;
  
 mem_op_error:
  return -5;
}

int bdmflash_prog_x8(const flash_alg_info_t *alg, void *addr, const void *data, long count)
{
  int ret=1;
  u_int8_t old,new,val;
  caddr_t a=(caddr_t)((u_int32_t)addr&~alg->addr_mask);
  val=*(u_int8_t*)data;
  /* check if FLASH needs programming */
  if(1){
    old=FLASH_RD8(addr);
    if(old==val) return ret;
  }
  /* security sequence */
  FLASH_WR8(a+alg->reg1_addr,alg->cmd_unlock1);
  if(alg->cmd_unlock2)
    FLASH_WR8(a+alg->reg2_addr,alg->cmd_unlock2);
  /* program command */
  FLASH_WR8(a+alg->reg1_addr,alg->cmd_prog);
  FLASH_WR8(addr,val);
  /* wait for result */
  old=FLASH_RD8(addr);
  while((new=FLASH_RD8(addr))!=old){
    if((old&alg->fault_bit)&&(new&alg->fault_bit)){
      if((FLASH_RD8(addr))!=new) ret=-2;
      break;
    }
    old=new;
  }
  /* reset */
  FLASH_WR8(a,alg->cmd_reset);
  if((FLASH_RD8(addr)!=val) && (ret>=0)) ret=-3;
  return ret;

  mem_op_error:
    return -5;
}

int bdmflash_erase_x8(const flash_alg_info_t *alg, void *addr, long size)
{
  u_int8_t old,new;
  int ret=0;
  caddr_t a=(caddr_t)((u_int32_t)addr&~alg->addr_mask);
  /* reset */
  FLASH_WR8(a,alg->cmd_reset);
  /* security sequence */
  FLASH_WR8(a+alg->reg1_addr,alg->cmd_unlock1);
  if(alg->cmd_unlock2)
    FLASH_WR8(a+alg->reg2_addr,alg->cmd_unlock2);
  /* erase command */
  FLASH_WR8(a+alg->reg1_addr,alg->cmd_erase);
  /* security sequence */
  FLASH_WR8(a+alg->reg1_addr,alg->cmd_unlock1);
  if(alg->cmd_unlock2)
    FLASH_WR8(a+alg->reg2_addr,alg->cmd_unlock2);
  /* select erase range */
  a=addr;
  if(size==0)
    FLASH_WR8(a+alg->reg1_addr,alg->erase_all);
  else{
    FLASH_WR8(addr,alg->erase_sec);
  }  
  old=FLASH_RD8(addr);
  while((new=FLASH_RD8(addr))!=old){
    if((old&alg->fault_bit)&&(new&alg->fault_bit)){
      if((FLASH_RD8(addr))!=new) ret=-2;
      break;
    }
    old=new;
  }
  /* reset */
  FLASH_WR8(a,alg->cmd_reset);
  if(FLASH_RD16(addr)!=0xffff) ret--;    
  return ret;
  
  mem_op_error:
    return -5;
}


/*******************************************************************/
/* routines for two/four interleaved 16/8 bit wide Intel or AMD flashes */

#ifdef WITH_TARGET_BUS32

int bdmflash_check_id_x32(const flash_alg_info_t *alg, void *addr, flash_d_t retid[2])
{
  int ret=0;
  u_int32_t devid, manid;
  caddr_t a=(caddr_t)((u_int32_t)addr&~alg->addr_mask);
  /* reset */
  FLASH_WR32(a,alg->cmd_reset);
  /* security sequence */
  FLASH_WR32(a+alg->reg1_addr,alg->cmd_unlock1);
  if(alg->cmd_unlock2)
    FLASH_WR32(a+alg->reg2_addr,alg->cmd_unlock2);
  /* read manufacturer ID */
  FLASH_WR32(a+alg->reg1_addr,alg->cmd_rdid);
  /* bank slecets swapped as i suspect (360DK only) !!!!!! */
  /*manid=FLASH_RD32(a+0xc); */
  manid=FLASH_RD32(a+0); 
  if(manid!=alg->manid) ret=-1;
  /* reset */
  FLASH_WR32(a,alg->cmd_reset);
  /* security sequence */
  FLASH_WR32(a+alg->reg1_addr,alg->cmd_unlock1);
  if(alg->cmd_unlock2)
    FLASH_WR32(a+alg->reg2_addr,alg->cmd_unlock2);
  /* read device ID */
  FLASH_WR32(a+alg->reg1_addr,alg->cmd_rdid);
  devid=FLASH_RD32(a+4);
  if(devid!=alg->devid) ret=-1;
  /* reset */
  FLASH_WR32(a,alg->cmd_reset);
  if(retid)
    {retid[0]=manid;retid[1]=devid;};
  return ret;

  mem_op_error:
    return -5;
}

int bdmflash_prog_x32(const flash_alg_info_t *alg, void *addr, const void *data, long count)
{
  int ret=4;
  u_int32_t old,new,fault,val;
  caddr_t a=(caddr_t)((u_int32_t)addr&~alg->addr_mask);
  ret=bdmflash_prepval_x32(&val,addr,data,count);
  if(ret<=0)
    return ret;
  addr=(void*)((long)addr&~3l);
  /* check if FLASH needs programming */
  if(1){
    old=FLASH_RD32(addr);
    if(old==val) return ret;
  }
  /* security sequence */
  FLASH_WR32(a+alg->reg1_addr,alg->cmd_unlock1);
  if(alg->cmd_unlock2)
    FLASH_WR32(a+alg->reg2_addr,alg->cmd_unlock2);
  /* program command */
  FLASH_WR32(a+alg->reg1_addr,alg->cmd_prog);
  FLASH_WR32(addr,val);
  /* wait for result */
  old=FLASH_RD32(addr);
  while((new=FLASH_RD32(addr))!=old){
    if((fault=old&new&alg->fault_bit)){
      old=new;
      /* check for some false fault at finish or race of x8x2 */
      if(!(old^=(new=FLASH_RD32(addr)))) break; /* finished now */
      else{
        /* one chip of x16x2 or x8x4 configuration can finish earlier */
        if(!(old&0x000000ff)||(fault&0x000000ff))
          if(!(old&0x0000ff00)||(fault&0x0000ff00))
            if(!(old&0x00ff0000)||(fault&0x00ff0000))
              if(!(old&0xff000000)||(fault&0xff000000))
                {ret=-2;break;}
      }
    }
    old=new;
  }
  /* reset */
  FLASH_WR32(a,alg->cmd_reset);
  if((FLASH_RD32(addr)!=val) && (ret>=0)) ret=-3;

  return ret;

  mem_op_error:
    return -5;
}


int bdmflash_erase_x32(const flash_alg_info_t *alg, void *addr, long size)
{
  u_int32_t old,new,fault;
  int ret=0;
  caddr_t a=(caddr_t)((u_int32_t)addr&~alg->addr_mask);
  /* reset */
  FLASH_WR32(a,alg->cmd_reset);
  /* security sequence */
  FLASH_WR32(a+alg->reg1_addr,alg->cmd_unlock1);
  if(alg->cmd_unlock2)
    FLASH_WR32(a+alg->reg2_addr,alg->cmd_unlock2);
  /* erase command */
  FLASH_WR32(a+alg->reg1_addr,alg->cmd_erase);
  /* security sequence */
  FLASH_WR32(a+alg->reg1_addr,alg->cmd_unlock1);
  if(alg->cmd_unlock2)
    FLASH_WR32(a+alg->reg2_addr,alg->cmd_unlock2);
  /* select erase range */
  a=addr;
  if(size==0)
    FLASH_WR32(a+alg->reg1_addr,alg->erase_all);
  else{
    FLASH_WR32(addr,alg->erase_sec);
  }  
  /* wait for result */
  old=FLASH_RD32(addr);
  while((new=FLASH_RD32(addr))!=old){
    if((fault=old&new&alg->fault_bit)){
      old=new;
      /* check for some false fault at finish or race of x16x2 or x8x4 */
      if(!(old^=(new=FLASH_RD32(addr)))) break; /* finished now */
      else{
        /* one chip of x16x2 or x8x4 configuration can finish earlier */
        if(!(old&0x000000ff)||(fault&0x000000ff))
          if(!(old&0x0000ff00)||(fault&0x0000ff00))
            if(!(old&0x00ff0000)||(fault&0x00ff0000))
              if(!(old&0xff000000)||(fault&0xff000000))
                {ret=-2;break;}
      }
    }
    old=new;
  }
  /* reset */
  FLASH_WR32(a,alg->cmd_reset);
  if(FLASH_RD32(addr)!=0xffffffff) ret--;
  return ret;

  mem_op_error:
    return -5;
}


#endif /* WITH_TARGET_BUS32 */

/*******************************************************************/

/* flash type independent check_id */
int bdmflash_check_id(const flash_alg_info_t *alg, void *addr, flash_d_t retid[2])
{
  if(alg->check_id)
    return alg->check_id(alg, addr, retid);
  else
    return -10;	/* we really need to define error constants in future */
}

/* flash type independent program one location */
int bdmflash_prog(const flash_alg_info_t *alg, void *addr, const void *data, long count)
{
  if(alg->prog)
    return alg->prog(alg,addr,data,count);
  else
    return -10;
}

/* flash type independent erase region */
int bdmflash_erase(const flash_alg_info_t *alg, void *addr, long size)
{
  if(alg->erase)
    return alg->erase(alg,addr,size);
  else
    return -10;
}

const flash_alg_info_t *
bdmflash_alg_from_id(flash_d_t id[2])
{
  int i;
  const flash_alg_info_t *alg;
  for(i=0;(alg=flash_alg_infos_def[i])!=NULL;i++)
    if((alg->manid==id[0])&&(alg->devid==id[0]))
      return alg;
  return NULL;
}

const flash_alg_info_t *
bdmflash_alg_probe(caddr_t flash_adr)
{
  int i;
  const flash_alg_info_t *alg,*alg1;
  flash_d_t testid[2];
  for(i=0;(alg=flash_alg_infos_def[i])!=NULL;i++){
    if(bdmflash_check_id(alg,flash_adr,testid)>=0)
      return alg;
    alg1=bdmflash_alg_from_id(testid);
    if(alg1!=NULL)
      if(bdmflash_check_id(alg1,flash_adr,testid)>=0)
        return alg1;
  }
  return NULL;
}

int
bdmflash_wrb_filt(bdmlib_bfilt_t * filt, caddr_t in_adr,
		u_int size, u_char * bl_ptr)
{
  int offs=0,res;
  flash_d_t val;
  const flash_alg_info_t *alg=(flash_alg_info_t*)filt->info;

 #if 0
  if(alg->width!=FLASH_ALG_BITS_x8) {
    /* 16 bit wide flash write path */
    if((u_long)in_adr&1) {
      in_adr-=1;
      val=(FLASH_RD16(in_adr)&0xff00)|bl_ptr[offs];
      if((res=bdmflash_prog(alg, in_adr, val))<0) {
	fprintf(stderr, "flash byte write error %d at 0x%lx\n",
                	res,(u_long)in_adr);
	return offs;
      }
      in_adr+=2;
      size-=1;
      offs+=1;
    }
    while(size>=2) {
      val=(bl_ptr[offs]<<8)|bl_ptr[offs+1];
      if((res=bdmflash_prog(alg, in_adr, val))<0) {
	fprintf(stderr, "flash write error %d at 0x%lx\n",
                	res,(u_long)in_adr);
	return offs;
      }
      in_adr+=2;
      size-=2;
      offs+=2;
    }
    if(size) {
      val=(FLASH_RD16(in_adr)&0x00ff)|(bl_ptr[offs]<<8);
      if((res=bdmflash_prog(alg, in_adr, val))<0) {
	fprintf(stderr, "flash byte write error %d at 0x%lx\n",
                	res,(u_long)in_adr);
	return offs;
      }
      size-=1;
      offs+=1;
    }
  }
 #endif 
  
  while(size>=1) {
    val=bl_ptr[offs];
    if((res=bdmflash_prog(alg, in_adr, bl_ptr+offs, size))<=0) {
    fprintf(stderr, "flash write error %d at 0x%lx\n",
               	    res,(u_long)in_adr);
      return offs;
    }
    in_adr+=res;
    size-=res;
    offs+=res;
  }
  return offs;

 #if 0
  mem_op_error:
    fprintf(stderr, "bdm memory access error\n");
    return 0;
 #endif
}

int
bdmflash_erase_filt(bdmlib_bfilt_t * filt, caddr_t in_adr,
		u_int size)
{
  int res=0;
  const flash_alg_info_t *alg=(flash_alg_info_t*)filt->info;
  if(!in_adr) in_adr=filt->begin_adr;

  res=bdmflash_erase(alg, in_adr,size);
  if(res<0)
    fprintf(stderr, "flash erase error %d\n",res);
  return res;
}

#if 0

/* slow version of blank check */
int
bdmflash_blankck_filt(bdmlib_bfilt_t * filt, caddr_t in_adr,
		u_int size)
{
  int errors=0;
  if(!in_adr||!size){
    in_adr=filt->begin_adr;
    size=filt->end_adr-in_adr+1;
  }
  if(((long)in_adr&1)&&size){
    if(FLASH_RD8(in_adr)!=0xff){
      fprintf(stderr,"blank check error at 0x%06lX",(long)in_adr);
      errors++;
    }
    in_adr++; size--;
  }
  while(size>=2){
    if(FLASH_RD16(in_adr)!=0xffff){
      if(errors<5){
        if(errors) fprintf(stderr,",0x%06lX",(long)in_adr);
          else fprintf(stderr,"blank check error at 0x%06lX",(long)in_adr);
      }else if(errors==5) fprintf(stderr, " and more");
      errors++;
    }
    if(!((long)in_adr&0xfff)) bdmlib_propeller(stdout);
    in_adr+=2; size-=2;
  }
  if(size){
    if(FLASH_RD8(in_adr)!=0xff){
      if(errors) fprintf(stderr,",0x%06lX",(long)in_adr);
        else fprintf(stderr,"blank check error at 0x%06lX",(long)in_adr);
      errors++;
    }
  }
  if(errors) fprintf(stderr,"\n");

  return errors;

  mem_op_error:
    fprintf(stderr, "bdm memory access error\n");
    return -5;
}

#else

/* hopefully faster version of blank check */
int
bdmflash_blankck_filt(bdmlib_bfilt_t * filt, caddr_t in_adr,
		u_int size)
{
  int errors=0, in_buf;
  u_char buf[1024], *p;
  if(!in_adr||!size){
    in_adr=filt->begin_adr;
    size=filt->end_adr-in_adr+1;
  }

  while(size){
    if(size<sizeof(buf)) in_buf=size;
    else in_buf=sizeof(buf);
   
    if(bdmlib_read_block(in_adr,in_buf,buf)!=in_buf) 
      goto mem_op_error;
 
    size-=in_buf;
    for(p=buf;in_buf--;in_adr++,p++){
      if(*p!=0xff){
        if(errors<10){
          if(errors) fprintf(stderr,",0x%06lX",(long)in_adr);
            else fprintf(stderr,"blank check error at 0x%06lX",(long)in_adr);
        }else if(errors==10) fprintf(stderr, " and more");
        errors++;
      }
    }
    bdmlib_propeller((u_long)in_adr, stdout);
  }

  if(errors) fprintf(stderr,"\n");

  return errors;

  mem_op_error:
    fprintf(stderr, "\nbdm memory access error\n");
    return -5;
}

#endif
