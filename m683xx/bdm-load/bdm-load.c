/*
 * Adapted from....
 *
 * Remote debugging interface for 683xx via Background Debug Mode
 * needs a driver, which controls the BDM interface.
 * written by G.Magin
 * (C) 1995 Technische Universitaet Muenchen, Lehrstuhl fuer Prozessrechner
 * 
 * D.Jeff Dionne
 * (C) 1995, Dionne & Associates
 * 
 * V0.2 Enhancements:
 * Added command line arguments
 * Rewritten flash programming functions for more flexability
 *
 * Andrew Dennison <adennison@swin.edu.au>
 * December 1995
 *
 * Modified by Pavel Pisa  pisa@cmp.felk.cvut.cz 2000

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 */

/*
 * NOTE:
 * This file is assumed to be runnable only on Linux/i386 because of
 * HW-restrictions: the driver is currently only available on i386-Linux 
 * So byte-swapping and alignment is handled directly, violating the 
 * GNU-coding standards. However, the "dirty spots" are restricted in this 
 * file.
 *
 * If anybody wants to port to e.g. Sparc, byte-sex has to be handled in
 * a more general way.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <getopt.h>

#include <bfd.h>
#include "bdm.h"
#include "bdmlib.h"
#include "bdmflash.h"

/*
 * Global Types
 **************
 */
typedef caddr_t CORE_ADDR;

#define	END_MACRO	0
#define	BEGIN_MACRO	1
#define	NO_SUFFIX_MACRO	2
#define DEBUG_MESSAGES

#define swap_l(x) (x>>24) | ((x>>8)&0xff00) | ((x<<8)&0xff0000) | ((x&0xff)<<24)

#define AUTO_ADR_CSBOOT ((caddr_t)-100)
#define AUTO_ADR_CSX    ((caddr_t)-100+1)

typedef struct str_list_t{
  char **items;
  int count;
  int alloccnt;
}str_list_t;			/* to store list of filenames */

/*
 * Global Variables
 ******************
 */

char hashmark = 0;		/* flag set by "set hash" */
int bdm_autoreset = 0;		/* automatic reset before load */
int bdm_ttcu = 0;		/* time to come up for init by rom */
char *initname;			/* need reimplement !!!!!!!!! */
char *bdm_dev_name;		/* device name */
static char *entry_name=NULL;

int
mem_dump (char *buf)
{
  caddr_t adr;
  long len, l;
  int i, ret;
  u_char mem[16];
  if (sscanf (buf, " %li %li", &l, &len) != 2)
    {
      printf ("use : dump address len\n");
      return -1;
    }
  adr = (caddr_t) l;
  while (len)
    {
      l = len > 16 ? 16 : len;
      ret = bdmlib_read_block (adr, l, mem);
      if (ret != l)
	{
	  printf ("error to read 0x%lx len 0x%lx ret %d\n",
		  (u_long) adr, l, ret);
	  return -2;
	}
      printf ("%06lx:", (u_long) adr);
      for (i = 0; i < l; i++)
	printf (" %02x", mem[i]);
      printf ("\n");
      len -= l;
      adr += l;
    }
  return 0;
}

int
mem_modify (char *buf)
{
  int ret;
  char *p;
  long addr;
  long count=0;
  long capacity=0;
  u_char *data=NULL;
  u_char val;

  while(*buf&&!isblank(*buf)) buf++;
  while(*buf&&isblank(*buf)) buf++;
  if(!*buf)
  { usage_error:
      if(data) free(data);
      printf ("use : modify address byte1 byte2 ...\n");
      return -1;
  }
  addr=strtol(buf,&p,0);
  if(p==buf) goto usage_error;
  buf=p;
  capacity=128;
  data=malloc(capacity);
  while(*buf&&isblank(*buf)) buf++;
  while(*buf){
    val=strtol(buf,&p,0);
    if(p==buf) goto usage_error;
    buf=p;
    if(count>=capacity){
      capacity*=2;
      data=realloc(data,capacity);
      if(!data) goto usage_error;
    }
    data[count++]=val;
    while(*buf&&isblank(*buf)) buf++;
  }
  
  ret=bdmlib_wrb_filt(bdmlib_bfilt, (caddr_t)addr, count, data);
  //if(ret!=count) 
  printf("bdmlib_wrb_filt: addr=0x%08lx count=%ld return=%ld\n",
           (long)addr,(long)count,(long)ret);

  if(data) free(data);
  return ret;
}

int
cpu_get_rpc(u_int32_t *val)
{
  int ret;
  u_int32_t rpc;
  if ((ret = bdmlib_get_sys_reg (BDM_REG_RPC, &rpc)) < 0)
    return ret;
  *val=swap_l(rpc);
  return 0;
}

int
cpu_stat (char *buf)
{
  int ret;
  u_int32_t rpc;
  ret = bdmlib_getstatus ();
  printf ("MCU ");
  if (ret & BDM_TARGETRESET)
    printf ("Reset ");
  if (ret & BDM_TARGETSTOPPED)
    printf ("Stopped ");
  if (ret & BDM_TARGETPOWER)
    printf ("NoPower ");
  if (ret & BDM_TARGETNC)
    printf ("NoConnect ");
  printf ("\n");
  if (ret & BDM_TARGETSTOPPED)
    {
      if ((ret = cpu_get_rpc (&rpc)) < 0)
	return ret;
      printf ("RPC=%x\n",rpc);
    }
  return 0;
}

int
flash_check_one (bdmlib_bfilt_t *filt, int autoset)
{
  flash_alg_info_t const *alg;
  caddr_t adr;
  flash_d_t readid[2];
  int ret;

  alg = (flash_alg_info_t *) filt->info;
  adr = filt->begin_adr;

  if(filt->flags&BDMLIB_FILT_AUTO)
  {
    autoset|=2;
    if(adr==filt->end_adr)
      autoset|=4;
  }

  if ((autoset & 4)&&(adr>=AUTO_ADR_CSBOOT)&&(adr<=AUTO_ADR_CSBOOT+12))
    {
      u_int32_t csx_opt, csx_base, csx_size;
      caddr_t cpu32csopt_adr;
      cpu32csopt_adr=(caddr_t)0xfffa48+(adr-AUTO_ADR_CSBOOT)*4;
      if (bdmlib_read_var (cpu32csopt_adr, BDM_SIZE_LONG, &csx_opt) >= 0)
	{
	  static u_int32_t csx_sizes[]={1<<11,1<<13,1<<14,1<<16,
					  1<<17,1<<18,1<<19,1<<20};
	  csx_base = (csx_opt >> 8) & 0xfff800;
	  filt->begin_adr = (caddr_t) csx_base;
	  csx_size = csx_sizes[(csx_opt >> 16) & 0x7];
	  filt->end_adr = (caddr_t) (csx_base + csx_size - 1);
	  if(adr==AUTO_ADR_CSBOOT)
	    printf ("Flash addresses taken from CSBOOT\n");
	  else
	    printf ("Flash addresses taken from CS%d\n",
	    			(int)(adr-AUTO_ADR_CSX));
	  printf ("Start %06lX End %06lX\n",
		  (long) filt->begin_adr, (long) filt->end_adr);
	}
    }

  adr = filt->begin_adr;

  ret = bdmflash_check_id (alg, adr, readid);
  if((ret >= 0)&&(autoset&4)&&(filt->begin_adr==filt->end_adr))
    filt->end_adr=filt->begin_adr+alg->addr_mask;
  printf ("Flash@%06lX-%06lX ManID %08X DevID %08X\n",
	  (long) adr, (long) filt->end_adr,
	  (unsigned) readid[0], (unsigned) readid[1]);
  if (ret >= 0)
    {
      printf ("Flash ID is OK\nUsed algorithm %s\n", alg->alg_name);
      return 1;
    }
  if (!autoset & 2)
    {
      printf ("Flash parameters are incorrect !!!\n");
      return -1;
    }
  alg = bdmflash_alg_probe (adr);
  if (alg == NULL)
    {
      printf ("Flash parameters cannot be probed\n");
      return -1;
    }
  ret = bdmflash_check_id (alg, adr, readid);
  if (ret < 0)
    {
      printf ("Probed parameters are incorrect\n");
      return -1;
    }
  filt->info = (void *) alg;
  if(filt->begin_adr==filt->end_adr)
    filt->end_adr=filt->begin_adr+alg->addr_mask;
  printf ("Probed flash parameters\n");
  printf ("Flash@%06lX-%06lX ManID %04X DevID %04X\n",
	  (long) filt->begin_adr, (long) filt->end_adr,
	  (unsigned) readid[0], (unsigned) readid[1]);
  printf ("Used algorithm %s\n", alg->alg_name);

  return 1;
}

int
flash_check(char *buf, int autoset)
{
  int ret;
  bdmlib_bfilt_t *filt = bdmlib_bfilt;
  printf("\n");
  while(filt)
  {
    if(filt->flags&BDMLIB_FILT_FLASH)
    {
      ret=flash_check_one(filt,autoset);
      printf("\n");
      if(ret<0)
	return ret;
    }
    filt=filt->next;
  }
  return 0;
}

int
flash_erase(void)
{
  int ret;
  bdmlib_bfilt_t *filt = bdmlib_bfilt;
  while(filt)
  {
    if(filt->flags&BDMLIB_FILT_FLASH)
    {
      ret=bdmflash_erase_filt (filt, (caddr_t) 0, 0);
      if(ret<0)
	return ret;
    }
    filt=filt->next;
  }
  return 0;
}

int
flash_blankck(void)
{
  int ret,sum=0;
  bdmlib_bfilt_t *filt = bdmlib_bfilt;
  while(filt)
  {
    if(filt->flags&BDMLIB_FILT_FLASH)
    {
      ret=bdmflash_blankck_filt (filt, (caddr_t) 0, 0);
      if(ret<0)
	return ret;
      sum+=ret;
    }
    filt=filt->next;
  }
  return sum;
}

/* bdmlib_propeller(stdout); */

int
flash_setup(char *buf)
{
  char *p,*r;
  int i;
  caddr_t begin_adr=(caddr_t)(0);
  caddr_t end_adr=(caddr_t)(0);
  char alg_name[32];
  flash_alg_info_t const *alg=NULL;
  bdmlib_bfilt_t *filt;

  while(*buf&&isblank(*buf)) buf++;
  if((p=strchr(buf,'@'))!=NULL){/* '@' character in specification */
    i=p-buf;
    if(i>=sizeof(alg_name)-1)
      i=sizeof(alg_name)-1;
    strncpy(alg_name,buf,i);
    alg_name[i]=0;
    p++;
    if(!strncmp(p,"csboot",6)){	/* take address from CSBOOT */
      p+=6;
      end_adr=begin_adr=AUTO_ADR_CSBOOT;
    }else if(!strncmp(p,"cs",2)){
      p+=2;
      end_adr=begin_adr=AUTO_ADR_CSX+strtoul(p,&r,0);
      if((p==r)||(begin_adr>AUTO_ADR_CSX+11)){
        fprintf(stderr,"%s : bad chipselect format\n",buf);
        return -1;
      }
      p=r;
    }else{			/* take begin address after '@' */
      end_adr=begin_adr=(caddr_t)strtoul(p,&r,0);
      if(p==r){
        fprintf(stderr,"%s : bad begin address format\n",buf);
        return -1;
      }
      p=r;
      if(*p=='+'){		/* take end address from size */
        end_adr=begin_adr+strtoul(++p,&r,0)-1;
        if(p==r){
          fprintf(stderr,"%s : bad size format\n",buf);
          return -1;
        }
        p=r;
      } else if(*p=='-') {	/* take absolute end address */
        end_adr=(caddr_t)strtoul(++p,&r,0);
        if(p==r){
          fprintf(stderr,"%s : bad end address format\n",buf);
          return -1;
        }
        p=r;
      }
    }
    while(*buf&&isblank(*buf)) buf++;
    if(*p){
      fprintf(stderr,"%s : extra characters in specification %s\n",buf,p);
      return -1;
    }
  }
  else {
    end_adr=begin_adr=AUTO_ADR_CSBOOT;
    strncpy(alg_name,buf,sizeof(alg_name)-1);
  }

  if(strcmp(alg_name,"auto")){
    int alg_indx=0;
    do{
      alg=flash_alg_infos[alg_indx++];
      if(alg==NULL){
        fprintf(stderr,"%s : unknown flash type or algorithm\n",alg_name);
        return -1;
      }
    }while(strcmp(alg_name,alg->alg_name));
  }

  filt=(bdmlib_bfilt_t*)malloc(sizeof(bdmlib_bfilt_t));
  if(filt==NULL){
    fprintf(stderr,"No memory for flash_setup\n");
    return -1;
  }

  filt->flags=BDMLIB_FILT_FLASH;
  if(alg==NULL){
    filt->flags|=BDMLIB_FILT_AUTO;
    filt->info=flash_alg_infos[0];
  } else filt->info=(void *)alg;
  filt->begin_adr=begin_adr;
  filt->end_adr=end_adr;
  if(begin_adr==end_adr){
    filt->flags|=BDMLIB_FILT_AUTO;
  };
  filt->filt_id=1;
  filt->wrb_filt=&bdmflash_wrb_filt;
  
  filt->next=bdmlib_bfilt;
  bdmlib_bfilt=filt;
  return 0;
}

int
str_list_add(str_list_t *list, char *str)
{
  char **p;
  int a;
  if(list->count>=list->alloccnt)
  { 
    a=list->alloccnt+5;
    if(list->items==NULL)
      p=(char**)malloc(sizeof(char *)*a);
    else  
      p=(char**)realloc(list->items,sizeof(char *)*a);
    if(p==NULL) return -1;
    list->items=p;
    list->alloccnt=a;
  } 
  list->items[list->count++]=str;
  return list->count;
}

void str_list_clear(str_list_t *list)
{
  while(list->count--)
  {
    if(list->items[list->count]!=NULL)
      free(list->items[list->count]);
  }
  list->alloccnt=list->count=0;
  if(list->items!=NULL) free(list->items);
  list->items=NULL;
}

/*
 * Main Program
 **************
 */
int
main (int argc, char *argv[])
{
  int done;
  int error;
  int ret;
  static int print_help = 0;		/* Help */
  static int reset = 0;			/* Reset target */
  static int check = 0;			/* Check flash configuration */
  static int erase = 0;			/* Erase flash */
  static int blankck = 0;		/* Check, that flash is blank */
  static int load = 0;			/* Load files to flash */
  static int run = 0;			/* Run target from entry point */
  static int script = 0;		/* Batch mode */
  static int bdm_delay = 0;		/* Delay/speed for BDM driver */
  static int set_mbar_fl = 0;		/* Set MBAR requested */
  static u_long mbar_val = 0;
  static u_long entry_pt=0;
  str_list_t files2load={0,};		/* List of filenames to load */
  int cmd;
  char buf[255];
  char *p;
  int i;

  /* initialize global flash control variables */
  initname = strdup ("ram_init.srec");	/* default init file for EFI332 */

  /* initialize name of bdm device */
  bdm_dev_name = strdup ("/dev/bdm");

  /* start bdmlib debugging */
  bdmlib_setdebug (0);

  /* Parse arguments and options. Taken from GDB. */
  {
    int c;
    /* When var field is 0, use flag field to record the equivalent
       short option (or arbitrary numbers starting at 10 for those
       with no equivalent).  */
    static struct option long_options[] =
    {
      {"help", no_argument, &print_help, 1},
      {"h", no_argument, &print_help, 1},
      {"flash", required_argument, 0, 'f'},
      {"f", required_argument, 0, 'f'},
      {"init-file", required_argument, 0, 'i'},
      {"i", required_argument, 0, 'i'},
      {"reset", no_argument, &reset, 1},
      {"r", no_argument, &reset, 1},
      {"check", no_argument, &check, 1},
      {"c", no_argument, &check, 1},
      {"erase", no_argument, &erase, 1},
      {"e", no_argument, &erase, 1},
      {"blankck", no_argument, &blankck, 1},
      {"b", no_argument, &blankck, 1},
      {"load", no_argument, &load, 1},
      {"l", no_argument, &load, 1},
      {"go", optional_argument, NULL, 'g'},
      {"g", no_argument, &run, 1},
      {"mbar", required_argument, NULL, 'M'},
      {"M", required_argument, NULL, 'M'},
      {"script", no_argument, &script, 1},
      {"s", no_argument, &script, 1},
      {"bdm-delay", required_argument, 0, 'd'},
      {"d", required_argument, 0, 'd'},
      {"bdm-device", required_argument, 0, 'D'},
      {"D", required_argument, 0, 'D'},
      {0, no_argument, 0, 0}
    };

    while (1)
      {
	int option_index;

	c = getopt_long_only (argc, argv, "",
			      long_options, &option_index);
	if (c == EOF)
	  break;

	/* Long option that takes an argument.  */
	if (c == 0 && long_options[option_index].flag == 0)
	  c = long_options[option_index].val;

	switch (c)
	  {
	  case 0:
	    /* Long option that just sets a flag.  */
	    break;
	  case 'f':
	    /* Setup flash region */
	    if(flash_setup(optarg)<0)
	      exit(1);
	    break;
	  case 'g':
	    /* Run target from default or optional entry point */
	    run=1;
	    if (optarg) {
		if (entry_name) free (entry_name);
		entry_name = strdup (optarg);
	    }
	    break;
	  case 'M':
	    /* Set MBAR register */
	    mbar_val=strtoul(optarg,&p,0);
	    set_mbar_fl=1;
	    if(optarg==p){
	      fprintf(stderr,"%s: bad numeric format for MBAR\n",argv[0]);
	      exit(1);
	    }
	    break;
	  case 'D':
	    if (bdm_dev_name)
	      free (bdm_dev_name);
	    bdm_dev_name = strdup (optarg);
	    break;
	  case 'i':
	    /* Init File */
	    if(initname)
	      free(initname);
	    initname = strdup (optarg);
	    break;
	  case 'd':
	    /* BDM driver delay */
	    bdm_delay=strtol(optarg,&p,0);
	    if(optarg==p){
	      fprintf(stderr,"%s: bad numeric format for BDM delay\n",argv[0]);
	      exit(1);
	    }
	    break;
	  case '?':
	    fprintf(stderr,
		"Use `%s --help' for a complete list of options.\n",
			argv[0]);
	    exit (1);
	  }
      }
  }

  if(print_help)
  {
     printf ("Usage bdm-load [OPTIONS] file1 ...\n"
	"\t-h --help            - this help information!\n"
	"\t-i --init-file=FILE  - object file to initialize processor\n"
	"\t-r --reset           - reset target CPU before other operations\n"
	"\t-c --check           - check flash setup (needed for auto)\n"
	"\t-e --erase           - erase flash\n"
	"\t-b --blankck         - blank check of flash\n"
	"\t-l --load            - download listed filenames\n"
	"\t   --go[=address]    - run target CPU from entry address\n"
	"\t-M --mbar=val        - setup 68360 MBAR register\n"
	"\t-s --script          - do actions and return\n"
	"\t-d --bdm-delay=d     - sets BDM driver speed/delay\n"
	"\t-D --bdm-device      - sets BDM device file\n"
	"\t-f --flash=TYPE@ADR  - select flash\n"
	"For flash TYPE@ADR can be\n"
	"  {<TYPE>|auto}[@{csboot|cs<x>|<start>{+<size>|-<end>}}]\n"
	"Examples\n"
	"  auto@csboot  amd29f400@0x800000-0x87ffff  auto@0+0x80000\n"
	"If auto type or cs address is used, check must be\n"
	"specified to take effect.\n"
	"Known flash types/algorithms :\n");
     for(i=0;flash_alg_infos[i]!=NULL;i++)
       printf("  %s",flash_alg_infos[i]->alg_name);
     printf("\n\n");
     exit (0);
  }

  while(optind<argc)
    str_list_add(&files2load,strdup(argv[optind++]));

#ifdef DEBUG_MESSAGES
  printf ("Init file is: %s\n", initname);
  for(i=0;i<files2load.count;i++){
    printf ("Object file : %s\n", files2load.items[i]);
  }
  printf ("Actions to do%s%s%s%s%s\n", (reset) ? " Reset" : "",
		(check) ? " Check" : "", (erase) ? " Erase" : "", 
		(blankck) ? " Blankck" : "", (load) ? " Load" : "");
#endif


  /* Connect to target */
  if((ret=bdmlib_open (bdm_dev_name)) != BDM_NO_ERROR) {
    printf("%s: Open \"%s\" reports %s\n",
    		argv[0],bdm_dev_name,bdmlib_geterror_str(ret));
    exit(1);
  }
  if((ret=bdmlib_setioctl (BDM_SPEED, bdm_delay)) != BDM_NO_ERROR) {
    printf("%s: Set bdm_delay failed with error %s\n",
    		argv[0],bdmlib_geterror_str(ret));
    exit(1);
  }
  hashmark = 1;
  if(set_mbar_fl) {
    printf ("MBAR setup to %08lX\n",mbar_val);
    if(bdmlib_set_mbar(mbar_val) != BDM_NO_ERROR) {
      printf("%s: MBAR setup failed with error %s\n",
    		argv[0],bdmlib_geterror_str(ret));
      exit(1);
    }
  }
  if(files2load.count>0)
    bdmlib_do_load_macro (files2load.items[0], BEGIN_MACRO);

  done = cmd = 0;

  do
    {
      error = 0;
      if (reset)
	{
	  /* bdmlib_init(); */
	  if(bdmlib_reset()<0)
	  {
	    printf ("Reset Failed\n");
	    error=4;
	  }
	  else 
	  {
	    if(cpu_stat(NULL)<0)
	    {
	      printf ("CPU Stat Failed\n");
	      error=4;
	    }
	    else
	    {
	      u_int32_t rpc;
	      reset = 0;
	      if(entry_pt==0)
	      {
	        cpu_get_rpc (&rpc);
	        entry_pt=rpc;
	      }
	    }
	  }
	}

      if (check && !error)
        {
          if(flash_check(NULL,0)<0)
	  {
	    printf ("Flash Check Failed\n");
	    error=3;
	  }
	  else
	    check = 0;
        }

      if (erase && !error)
	{
	  printf ("Erasing Flash...");
	  fflush (stdout);
	  if (flash_erase() >= 0)
	  {
	    printf ("done.\n");
	    erase = 0;
	  }
	  else
	  {
	    printf ("Erase Failed\n");
	    error = 2;
	  }
	}

      if (blankck && !error)
	{
	  int res;
	  printf ("Flash blank check ...\n");
	  res=flash_blankck();
          if(!res){
            printf("Flash blank check OK\n");
            blankck = 0;
	  }else{
	    error = 5;
	    if(res<0) printf("Flash blank check FAILED\n");
	    else printf("Flash is NOT erased\n");
	  }
	}

      if (load && !error)
	{
	  bdmlib_load_use_lma = 1;
          for(i=0;i<files2load.count;i++){
            printf("Loading file : %s\n",files2load.items[i]);
            fflush(stdout);
	    if((ret=bdmlib_load(files2load.items[i], entry_name, &entry_pt))<0)
	    {
	      printf ("Load Failed %d\n", ret);
	      error = 1; //break;
	    }
	  }
	  if(!error) load = 0;
	  bdmlib_load_use_lma = 0;
	}

      if (run && !error)
      {
	printf ("starting at 0x%lx\n", entry_pt);
	bdmlib_set_sys_reg (BDM_REG_RPC, entry_pt);
	bdmlib_go ();
	run=0;
      }

      if(script)
	exit(0);

      printf ("bdm-load %d> ", ++cmd);
      fgets (buf, 250, stdin);

      if (!strcmp (buf, "run"))
	{
	  run = 1;
	}

      if (!strcmp (buf, "reset"))
	{
	  reset = 1;
	}

      if (!strncmp (buf, "erase", 5))
	erase = 1;

      if (!strncmp (buf, "blankck", 5))
	blankck = 1;

      if (!strncmp (buf, "load", 4))
	{
	  load = 1;
	  if ((strlen (buf) > 5))
	    {
	      str_list_clear(&files2load);
	      str_list_add(&files2load,strdup(&buf[5]));
	    }
	}

      if (!strncmp (buf, "exit", 4) || !strncmp (buf, "quit", 4))
	done = 1;

      if (!strncmp (buf, "dump", 4))
	mem_dump (buf + 4);

      if (!strncmp (buf, "modify", 3))
	mem_modify (buf + 3);

      if (!strncmp (buf, "stat", 4))
	cpu_stat (buf + 4);

      if (!strncmp (buf, "check", 5))
	flash_check (buf + 5, 0);

      if (!strncmp (buf, "autoset", 5))
	flash_check (buf + 5, 0xff);

      if (!strncmp (buf, "stop", 5))
      {
        bdmlib_ioctl(BDM_STOP_CHIP);
        reset=check=erase=blankck=load=run=0;
      }

      if (!strncmp (buf, "make", 4))
	system (buf);

      if (!strncmp (buf, "help", 4) || !strncmp (buf, "?", 1)) 
      {
	printf("Next commands are accepted :\n"\
	       "  run     - enable full speed ran of target CPU\n"\
	       "  stop    - stop target CPU\n"\
	       "  reset   - reset target\n"\
	       "  erase	  - erase all configured FLASH chips\n"\
	       "  blankck - check blank state of chips\n"\
	       "  load    - loand all object files specified\n"\
	       "            at command line to RAM or FLASH\n"\
	       "            using VMA addresses, name of file to\n"\
	       "            can be optionaly specified after \"load\"\n"\
	       "  dump <from> <len>  - dump target memory\n"\
	       "  modify <from> <byte> ... - modify target memory (even flash)\n"\
	       "  state   - print target status\n"\
	       "  check   - check presence of configured chips\n"\
	       "  autoset - set FLASH chip types from their IDs\n"\
	       "  make <par> - call make command in current directory\n"\
	       "  quit    - leave loader\n"\
	       );
      }
    }
  while (!done);
  
  str_list_clear(&files2load);

  exit (0);
}
