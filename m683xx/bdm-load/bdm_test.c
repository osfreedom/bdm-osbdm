

#include	<sys/types.h>
#include 	<stdarg.h>
#include 	<stdlib.h>
#include 	<unistd.h>
#include 	<stdio.h>
#include 	<ctype.h>
#include 	<string.h>
#include	"bdm.h"
#include	"bdmlib.h"

char* TestSW(int argc,char *argv[],char swch);

#define swap_l(x) (x>>24) | ((x>>8)&0xff00) | ((x<<8)&0xff0000) | ((x&0xff)<<24)

/* for BDMLIB to work */
char hashmark;
int bdm_autoreset;
int bdm_ttcu;

int bdm_step_chip()
{
 int ret;
 u_int32_t rpc;
 
 if (!bdmlib_isopen())
  return BDM_ERR_NOT_OPEN;
 
 if (!(bdmlib_getstatus()&BDM_TARGETSTOPPED))
  if((ret=bdmlib_ioctl(BDM_STOP_CHIP)<0))
    {return ret;};
   
 if((ret=bdmlib_get_sys_reg(BDM_REG_RPC, &rpc))<0) return ret;
 printf("RPC=%x\n",swap_l(rpc)); 
 ret=bdmlib_ioctl(BDM_STEP_CHIP);
 if((ret=bdmlib_get_sys_reg(BDM_REG_RPC, &rpc))<0) return ret;
 printf("RPC=%x\n",swap_l(rpc)); 
 return ret;
}

int bdm_show_regs(void)
{ 
 u_int reg;
 if (!bdmlib_isopen())
  return BDM_ERR_NOT_OPEN;

 bdmlib_get_reg(BDM_REG_D0, &reg);
 return 0;
};


int rd_test(void)
{ 
 u_int16_t val; 
 caddr_t adr=(caddr_t)0xFFAA;
 int counter=0;
 
 if (!bdmlib_isopen())
  return BDM_ERR_NOT_OPEN;
 while(1){ 
   if(bdmlib_read_var(adr,BDM_SIZE_WORD,&val)<0)
   goto mem_op_error;
   //usleep(1);
   if(counter++>=10000){
     printf(".");
     fflush(stdout);
     counter=0;
   }
 }
 return 0;

 mem_op_error:
  printf("Memory read error\n");
 return -1;
};

int wr_test(void)
{ 
 u_int16_t val=0xAAAA;
 caddr_t adr=(caddr_t)0xFFAA;
 int counter=0;

 if (!bdmlib_isopen())
  return BDM_ERR_NOT_OPEN;
 while(1){ 
   if(bdmlib_write_var(adr,BDM_SIZE_WORD,val)<0)
   goto mem_op_error;
   //usleep(1);
   if(counter++>=10000){
     printf(".");
     fflush(stdout);
     counter=0;
   }
 }
 return 0;

 mem_op_error:
  printf("Memory write error\n");
 return -1;
};


int step_test(void)
{ 
 int counter=0;

 if (!bdmlib_isopen())
  return BDM_ERR_NOT_OPEN;
 while(1){ 
   if(bdmlib_ioctl(BDM_STEP_CHIP)<0)
     goto step_op_error;
   usleep(1);
   if(counter++>=10000){
     printf(".");
     fflush(stdout);
     counter=0;
   }
 }
 return 0;

 step_op_error:
  printf("Stepping error\n");
 return -1;
};


int main(int argc, char *argv[])
{
 int quit_fl=0;
 int ret;
 char c;
 char *p;
 char file_name[100]="";
 char bdm_dev_name[100]="/dev/bdm";
 unsigned long entry_pt=0;

 bdmlib_setdebug(1);
 if((p=TestSW(argc,argv,'D'))!=NULL)
  strncpy(bdm_dev_name,p,sizeof(bdm_dev_name));
 if((ret=bdmlib_open(bdm_dev_name))<0)
  { printf("bdmlib_open : %s\n",bdmlib_geterror_str(ret)); return 1; };
  bdmlib_setioctl(BDM_SPEED,0);
 
 if((p=TestSW(argc,argv,'F'))!=NULL)
 {
  strncpy(file_name,p,sizeof(file_name));
  bdmlib_setioctl(BDM_SPEED,100);
  bdmlib_reset();
  bdmlib_setioctl(BDM_SPEED,10);
  bdmlib_go();
  sleep(2);
  bdm_step_chip();  
  ret=bdmlib_load(file_name, &entry_pt);
  if(ret<0) printf("bdmlib_load : %s\n",bdmlib_geterror_str(ret));
  bdmlib_set_sys_reg(BDM_REG_RPC, entry_pt);
  ret=bdmlib_go();
 };
 
 do {
  c=getchar();
  
  ret=bdmlib_getstatus();
  if(ret<0) printf("bdmlib_go : %s\n",bdmlib_geterror_str(ret));
  else {
   printf("MCU ");
   if(ret&BDM_TARGETRESET)   printf("Reset ");
   if(ret&BDM_TARGETSTOPPED) printf("Stopped ");
   if(ret&BDM_TARGETPOWER)   printf("NoPower ");
   if(ret&BDM_TARGETNC)      printf("NoConnect ");
   printf("\n");
  };

  c=toupper(c);
  
  switch(c) {
   
   case 'B':
   
   case 'D':
    bdmlib_showpc();
    break;
   
   case 'S': 
    ret=bdm_step_chip();
    if(ret<0) printf("bdmlib_go : %s\n",bdmlib_geterror_str(ret));
    break;

   case 'G':
    ret=bdmlib_go();
    if(ret<0) printf("bdmlib_go : %s\n",bdmlib_geterror_str(ret));
    break;
  
   case 'R':
    bdmlib_setioctl(BDM_SPEED,100);
    ret=bdmlib_reset();
    bdmlib_setioctl(BDM_SPEED,0);
    if(ret<0) printf("bdmlib_reset : %s\n",bdmlib_geterror_str(ret));
    break;

   case 'Q':
    quit_fl=1;
    break;
    
   case 'X':
    wr_test();
    break;

   case 'Y':
    rd_test();
    break;
     
   case 'Z':
    step_test();
    break;
     
   case 'L':
    printf("Enter file name : ");
    scanf("%s",file_name);
    printf("\n");
    ret=bdmlib_load(file_name, &entry_pt);
    if(ret<0) printf("bdmlib_load : %s\n",bdmlib_geterror_str(ret));
    else
    {printf("loaded OK, entrypoint %lx\n",entry_pt);
     if((ret=bdmlib_set_sys_reg(BDM_REG_RPC, entry_pt))<0)
      printf("set PC error : %s\n",bdmlib_geterror_str(ret));
    }
    break;  

   case '\n':
    break;
   
   default:
    printf ("Help for BDM_TEST\n"
	"B: Begin Program Execution from Reset\n"
	"D: Dump Target MCU Registers\n"
	"H: Print This Help Summary\n"
	"L: Load S-Record or Object File into Target\n"
	"M: Memory Hex/ASCII Display\n"
	"R: Hardware Reset Target MCU\n"
	"S: Single Step Target MCU\n"
	"G: Go Full Speed Target MCU\n"
	"Q: Quit back to DOS\n");
  };

 } while(!quit_fl);
 
 bdmlib_close(1);
 
 return(0);

};


char* TestSW(int argc,char *argv[],char swch)
{
 int i;
 char *p;
 for(i=1;i<argc;i++)
  if((argv[i][0]=='-')&&(toupper(argv[i][1])==swch))
  {
   p=argv[i]+2;
   while(((*p==' ')||(*p=='\t'))&&(*p)) p++;
   if((!*p)&&(i<argc)) if(argv[++i][0]!='-')
   {
    p=argv[i];
    while(((*p==' ')||(*p=='\t'))&&(*p)) p++;
   }
   return p;
  }
 return(NULL);
}
