%{
/* @#Copyright:
 * Copyright (c) 1997, Rolf Fiedler. .
 * Copyright (c) 1999-2000, Brett Wuth.
 */
/* @#License:
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* File:	bdm_mon.lex (BDM Monitor)
 * Purpose:
 * Author:	Rolf Fielder
 * Created:
 *
 * Initials:
 *	BCW - Brett Wuth
 *		@#[ContactWuth:
 *		Phone: +1 403 627-2460
 *		E-mail: support@castrov.cuug.ab.ca, wuth@acm.org]
 *
 * HISTORY:
 * $Log: bdmmon.l,v $
 * Revision 1.3  2005/10/24 01:32:21  cjohns
 * Removed warnings when built with gcc 4.x
 *
 * Revision 1.2  2004/01/08 14:16:17  codewiz
 * Avoid duplicate definition of TRUE/FALSE. Remove trailing whitespace.
 *
 * Revision 1.1  2003/12/29 22:19:11  codewiz
 * Move bdmmon and bdmflash to m68k/utils.
 *
 * Revision 1.1  2003/06/03 15:42:04  codewiz
 * Import userland tools from bdm-fiedler
 *
 * Revision 1.7  2000/09/19 00:28:29  wuth
 * cleanly use Fiedler's bdm driver; bdm_mon detects flash errors
 *
 * Revision 1.6  2000/08/03 06:29:18  wuth
 * MultiProject Sync; Support Micron-style flash; Report flash model
 *
 * Revision 1.5  2000/04/20 04:56:23  wuth
 * GPL.  Abstract flash interface.
 *
 * Revision 1.4  2000/03/28 20:24:42  wuth
 * Break out flash code into separate executable.  Make run under Chris Johns BDM driver.
 *
 * Revision 1.3  2000/03/27 22:05:24  wuth
 * Add memory dump to file.
 *
 * Revision 1.2  1999/07/05 22:09:50  wuth
 * Abort if can't sync BDM.  Work with Am29F800 flash.
 */

#include <BDMFlash.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#define REG_NAMES
#include "bdmops.h"

#ifndef TRUE
# define TRUE -1
# define FALSE 0
#endif

void print_status(void);

char help[]=
	" commands of the monitor:\n\n"
	" RReg - show all registers\n"
	" WReg reg_name value\n"
	" md start-addr length - show memory dump\n"
	" mf start-addr length long-word - fill memory\n"
	" me from - memory edit\n"
	" mw start-addr length file - dump memory into file\n"
	" up start-addr file - load contents of file up\n"
	" DOwn start-addr length file - load down and write to file\n"
	" stop - stop processor\n"
	" Step - step processor\n"
	" reset - reset processor\n"
	" restart - reset in freeze\n"
	" Run - resume execution\n"
	" flash_config base NumParallelChips ChipWidthBytes - configure flash memory\n"
	" erase - erase whole flash rom\n"
	" flash addr file - program flash from file\n"
	" watch from-addr to_addr - set address breakpoint\n"
	" trigger value mask - set data breakpoint\n"
	" Break addr mask - set PC breakpoint\n"
	" clear - clear all breakpoints\n"
	" source file - read commands from file\n"
	" poke.X addr valie - set memory to val\n"
	" BackTrace - show stack-backtrace (use maxstack first)\n"
	" maxstack - set maximum stack value\n"
	" Wait - wait for target to hit breakpoint\n"
	" query - query target status\n"
	" loop addr count - test bdm access to target\n"
	" Help - show this screen\n"
	" exit/Quit - leace monitor\n"
	" <enter> - repeat last command\n";

struct token {
    char name[8];
    int number;
};

int tokenize(char * str, char *args[])
{
    int n=0;
    static char argv_buf[1024];
    char *string;

    strcpy(&argv_buf[0], str);
    string=&argv_buf[0];

    while(1) {
	while(*string==' ' || *string=='\t' || *string=='\n') {
	    if(*string == '\0') {
		args[n]=0;
		return n;
	    }
	    string++;
	}
	args[n++]=string;
	while(*string!=' ' && *string!='\t' && *string!='\n') {
	    if(*string == '\0') {
		args[n]=0;
		return n;
	    }
	    string++;
	}
	*string++='\0';
    }
}

int find_token(char *string, struct token array[])
{
    char *x;
    int n;

    x=string;
    while(*x) {
	*x=toupper(*x);
	x++;
    }
    while(array[n].name[0]) {
	if(strcmp(string, array[n].name) == 0) {
	    return array[n].number;
	}
	n++;
    }
}

int mem_dump(unsigned long from, int count)
{
    int error, i, read;
    unsigned char buffer[256], line[17];

    error=0;
    while(count) {
        if(error) break;
        if(count>256) read=256;
	else read=count;
        if(bdm_read_mem(from, buffer, read)<0) {
	    error=1;
	    printf("Bus error or target disconnected.\n");
	    break;
	};
	for(i=0; i<read; i++) {
	    if(i%16==0) {
        strcpy((char*)line, "                ");
	        printf("%08" PRIxMAX ": ", from);
	    }
	    printf("%02x ", buffer[i]);
	    if(buffer[i]<' ' || buffer[i]>0x7f) {
	        line[i%16]='.';
	    } else {
	        line[i%16]=buffer[i];
	    }
	    if(i%16==15) {
	        printf(" * %s *\n", line);
	    }
	    from++;
	}
	count-=read;
	if(count) {
	    printf("press enter to continue...\n");
	    getchar();
	} else {
	    if(i%16 != 15) printf("\n");
	}
    }
    return error;
}


/* Write memory image to a file */
static
int
mem_write(unsigned long from, unsigned long count, FILE *file )
{
    int error, i, read;
    unsigned char buffer[256], line[17];

    error=0;
    while(count) {
        if(error) break;
        if(count>256) read=256;
	else read=count;
        if(bdm_read_mem(from, buffer, read)<0) {
	    error=1;
	    printf("Bus error or target disconnected.\n");
	    break;
	};
	for(i=0; i<read; i++) {
	    fputc( buffer[i], file );
	    from++;
	}
	count-=read;
    }
    return error;
}


static unsigned char buffer[1024];

void store_line(void)
{
  strcpy((char*) buffer, yytext);
/*    printf("last line was: %s\n", buffer); */
}

int no_prompt=FALSE;
void prompt(void)
{
    if(!no_prompt) {
        store_line();
        printf("> ");
    }
}

static unsigned long me_addr=0, addr=0, len=0x10, maxstack=0;
static int flash_ok=0;
static int BDMHandle = -1;

%}
%array
%option noyywrap

%x EDIT 

REGNAME (([%][A-Za-z])|([G-Zg-z]))[0-9A-Z_a-z]*
DIGIT [0-9]
NUMBER [0-9]+
HEXN 0x[0-9A-Fa-f]+
SRECORD S[0-9]([0-9A-F][0-9A-F])+
VALUE ((0x)?[0-9A-Fa-f]+)
TOKEN [A-Za-z][0-9A-Z_a-z]*
FILENAME [!-~]+
WS [ \t]
RREG (rreg|RREG|Rreg|rr|RR|Rr)
WREG (wreg|WREG|Wreg|wr|WR|Wr)
MD (md|MD|Md)
MF (mf|MF|Mf)
ME (me|ME|Me)
MW (mw|MW|Mw)
UP (up|UP|Up)
DOWN (down|DOWN|Down|do|DO|Do)
RUN (run|RUN|Run|R|r)
STOP (stop|STOP|Stop)
STEP (step|STEP|Step|S|s)
RESET (reset|RESET|Reset)
RESTART (restart|RESTART|Restart)
FLASHCONFIG (flash_config|FLASH_CONFIG|Flash_config)
ERASE (erase|ERASE|Erase)
FLASH (flash|FLASH|Flash)
MAXSTACK (maxstack|MAXSTACK|Maxstack)
BACKTRACE (backtrace|BACKTRACE|Backtrace|BT|bt|Bt)
WATCH (watch|WATCH|Watch)
TRIGGER (trigger|TRIGGER|Trigger)
BREAK (break|BREAK|Break|B|b)
CLEAR (clear|CLEAR|Clear|C|c)
WAIT (wait|WAIT|Wait|W|w)
QUERY (query|QUERY|Query|QU|qu)
SOURCE (source|SOURCE|Source)
POKE (poke|POKE|Poke)
HELP (help|HELP|Help|h|H)
LOOP (loop|LOOP|Loop)
FOO (foo)
EXIT (exit|EXIT|Exit|quit|QUIT|Quit|Q|q)

%%
{WS}*{HELP}{WS}*\n {
    printf("%s", help);
    prompt();
}

{WS}*{RREG}{WS}+{REGNAME}{WS}*\n {
    char *args[20];
    int i;
    unsigned long j;

    bdm_stop();
    tokenize(yytext, args);
    j=0;
    if(args[1][0] == '%') args[1]++; /* skip optional % */
    for(i=0; i<strlen(args[1]); i++) {
        args[1][i]=toupper(args[1][i]);
    }
    for(i=0; strlen(reg_names[i].name); i++) {
        if(strcmp(args[1], reg_names[i].name)==0)
	    break;
    }
    if(!strlen(reg_names[i].name)) {
        printf("ERROR\n");
    } else {
	bdm_read_reg(reg_names[i].id, &j);
	printf("%8s=0x%08" PRIxMAX "\n", reg_names[i].name, j);
    }
    prompt();
}

{WS}*{RREG}{WS}*\n {
    int i, n;
    unsigned long j;

    bdm_stop();
    for(i=0; i<=REG_CSR; i++) {
        bdm_read_reg(i, &j);
	for(n=0; strlen(reg_names[n].name); n++) {
	    if(reg_names[n].id == i)
		break;
	}
	printf("%8s=%08" PRIxMAX " ", reg_names[n].name, j);
	if(i%4==3) printf("\n");
    }
    if(i%4!=3) printf("\n");
    prompt();
}
    
{WS}*{WREG}{WS}+{REGNAME}{WS}+{VALUE}{WS}*\n {
    char *args[20];
    int i;
    unsigned long j;

    bdm_stop();
    tokenize(yytext, args);
    j=0;
    if(sscanf(args[2], "%" PRIxMAX, &j) != 1) {
	printf("ERROR\n");
    } else {
        if(args[1][0] == '%') args[1]++; /* skip optional % */
	for(i=0; i<strlen(args[1]); i++) {
	    args[1][i]=toupper(args[1][i]);
	}
	for(i=0; strlen(reg_names[i].name); i++) {
	    if(strcmp(args[1], reg_names[i].name)==0)
		break;
	}
	if(!strlen(reg_names[i].name)) {
	    printf("ERROR\n");
	} else {
	    printf("Writing 0x%08" PRIxMAX " to %s.\n", j, reg_names[i].name);
	    i = reg_names[i].id;
	    bdm_write_reg(i, &j);
	}
    }
    prompt();
}

{WS}*{MD}{WS}+{VALUE}{WS}+{VALUE}{WS}*\n {
    char *args[20];
    int i, error;

    tokenize(yytext, args);
    error=0;
    if(sscanf(args[1], "%" PRIxMAX, &addr) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(sscanf(args[2], "%" PRIxMAX, &len) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(!error) {
        mem_dump(addr, len);
    }
    prompt();
}

{WS}*{MD}{WS}*\n {
    addr+=len;
    mem_dump(addr, len);
    prompt();
}

{WS}*{MW}{WS}+{VALUE}{WS}+{VALUE}{WS}+{FILENAME}{WS}*\n {
    char *args[20];
    int i, error;
    FILE *file;

    tokenize(yytext, args);
    error=0;
    if(sscanf(args[1], "%" PRIxMAX, &addr) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(sscanf(args[2], "%" PRIxMAX, &len) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if((file=fopen(args[3], "w")) == NULL) {
	printf("ERROR opening file %s\n", args[3]);
	error=1;
    }
    if(!error) {
        mem_write(addr, len, file);
	fclose( file );
    }
    prompt();
}

{WS}*{MF}{WS}+{VALUE}{WS}+{VALUE}{WS}+{VALUE}{WS}*\n {
    unsigned long buf[1024/4];
    char *args[20];
    int error, i;
    unsigned long addr, val, len;
    tokenize(yytext, args);

    error=0;
    if(sscanf(args[1], "%" PRIxMAX, &addr) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(sscanf(args[2], "%" PRIxMAX, &len) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(sscanf(args[3], "%" PRIxMAX, &val) != 1) {
	printf("ERROR\n");
	error=1;
    }
    for(i=0; i<1024/4; i++)
	buf[i]=val;
    while(len>=1024) {
	if(bdm_write_mem(addr, (unsigned char*)buf, 1024)<0) {
	    error=1;
	    printf("Bus error or target disconnected.\n");
	    len=0;
	    break;
	}
	len-=1024;
	addr+=1024;
    }
    if(len) {
        if(bdm_write_mem(addr, (unsigned char*)buf, len)<0) {
            error=1;
	    printf("Bus error or target disconnected.\n");
	}
    }
    prompt();
}

{WS}*{ME}{WS}+{VALUE}{WS}*\n {
    char *args[20];
    unsigned char x;
    tokenize(yytext, args);
    if(sscanf(args[1], "%" PRIxMAX, &me_addr)!=1) {
	printf("ERROR\n> ");
    } else {
	if(bdm_read_mem(me_addr, &x, 1)<0) {
	    printf("Bus error or target disconnected.\n> ");
	} else {
	    printf("%08" PRIxMAX ": %02" PRIx32" = ", me_addr, x);
	    BEGIN(EDIT);
	}
    }
    store_line();
}

<EDIT>{WS}*{VALUE}{WS}*\n {
    char *args[20];
    unsigned long value;
    unsigned char x;
    tokenize(yytext, args);
    if(sscanf(args[0], "%" PRIxMAX, &value)!=1) {
	printf("ERROR\n> ");
    } else {
	x = (unsigned char)(0xff & value);
	if(bdm_write_mem(me_addr++, &x, 1)<0) {
	    printf("Bus error or target disconnected.\n> ");
	    BEGIN(INITIAL);
	} else {
	    if(bdm_read_mem(me_addr, &x, 1)<0) {
		printf("Bus error or target disconnected.\n> ");
		BEGIN(INITIAL);
	    } else {
		printf("%08" PRIxMAX": %02" PRIx32 " = ", me_addr, x);
	    }
	}
    }
}

<EDIT>. {
}

<EDIT>\n {
    printf("\n> ");
    BEGIN(INITIAL);
}

{WS}*{POKE}\.[bwlBWL]?{WS}+{VALUE}{WS}+{VALUE}{WS}*\n {
    char *args[20];
    int size;
    unsigned long addr, tmp;
    unsigned char value[4];

    tokenize(yytext, args);
    if(strlen(args[0]) != 6) goto error;
    switch(args[0][5]) {
        case 'b': case 'B': size = 1; break;
        case 'w': case 'W': size = 2; break;
        case 'l': case 'L': size = 4; break;
        default: goto error;
    }
    if(sscanf(args[1], "%" PRIxMAX, &addr) != 1 || (addr & (size-1)) != 0) goto error;
    if(sscanf(args[2], "%" PRIxMAX, &tmp ) != 1) goto error;
    switch(size) {
        case 1:
            value[0] = (tmp>> 0) & 0xFF;
	    break;
        case 2:
            value[0] = (tmp>> 8) & 0xFF;
	    value[1] = (tmp>> 0) & 0xFF;
	    break;
        case 4:
            value[0] = (tmp>>24) & 0xFF;
	    value[1] = (tmp>>16) & 0xFF;
	    value[2] = (tmp>> 8) & 0xFF;
	    value[3] = (tmp>> 0) & 0xFF;
	    break;
        default:
            goto error;
    }
    if(bdm_write_mem(addr, value, size) != size) {
      error:
        printf( "ERROR\n" );
    } else {
#if 0
        printf( "poking %x %x %d\n", addr, tmp, size );
#endif
    }
    prompt();
}

{SRECORD}(\r)?\n {
    unsigned char buffer[1024];
    int slen = 0, doff, i, rpc = 0;
    unsigned long addr = 0xDEADBEEF;
    int error = 0;
    switch(yytext[1]) {
      case '0':			/* title record: ignore it */
          rpc   = 0;
	  doff  = 0;
	  slen  = 0;
	  break;
      case '1':                   /* 2 byte address... */
          error = (2 != sscanf(yytext, "S1%2x%4lx", &slen, &addr));
	  rpc   = 0;
	  doff  = 4 + 4;
	  slen -= 2 + 1;
	  break;
      case '2':                   /* 3 byte address... */
          error = (2 != sscanf(yytext, "S2%2x%6lx", &slen, &addr));
	  rpc   = 0;
	  doff  = 4 + 6;
	  slen -= 3 + 1;
	  break;
      case '3':			/* 4 byte address... */
          error = (2 != sscanf(yytext, "S3%2x%8lx", &slen, &addr));
	  rpc   = 0;
	  doff  = 4 + 8;
	  slen -= 4 + 1;
	  break;
      case '7':			/* 4 byte address */
          error = (2 != sscanf(yytext, "S7%2x%8lx", &slen, &addr));
	  rpc   = 1;
	  doff  = 4 + 8;
	  slen -= 4 + 1;
	  break;
      case '8':			/* 3 byte address */
          error = (2 != sscanf(yytext, "S8%2x%6lx", &slen, &addr));
	  rpc   = 1;
	  doff  = 4 + 6;
	  slen -= 3 + 1;
	  break;
      case '9':			/* 2 byte address */
          error = (2 != sscanf(yytext, "S9%2x%4lx", &slen, &addr));
	  rpc   = 1;
	  doff  = 4 + 4;
	  slen -= 2 + 1;
	  break;
      default:
          error = 1;
	  break;
    }
    if(!error && !rpc) {
        for( i = 0; i < slen; i++ ) {
            int b;
	    if(sscanf( yytext+doff+2*i, "%2x", &b ) != 1) {
	        error = 1;
		break;
            }
	    buffer[i] = b;
        }
    }
    if(error) {
        printf( "bad srecord: S%c\n", yytext[1] );
    } else if(rpc == 1) {
        bdm_stop();
	bdm_write_reg( REG_RPC, &addr );
    } else if(slen > 0) {
#if 0
        if(bdm_write_mem(addr, buffer, slen)<0)
	    printf("Bus error or target disconnected.\n> ");
#elif 1
        i = 0;
	while(!error && i < slen && (addr+i) & 3) {
            error = (bdm_write_mem(addr+i, buffer+i, 1)<0);
	    i++;
	}
	if(!error && (slen-i) >= 4) {
	    error = (bdm_write_mem(addr+i, buffer+i, (slen-i) & ~3)<0);
	    i += (slen-i) & ~3;
	}
	while(!error && i < slen) {
	    error = (bdm_write_mem(addr+i, buffer+i, 1)<0);
	    i++;
	}
	if(error) printf("Bus error or target disconnected.\n> ");
#else
        for(i = 0; i < slen; i++) {
	    if((bdm_write_mem(addr+i, buffer+i, 1)<0)) {
	        printf("Bus error or target disconnected.\n> ");
		break;
	    }
	}
#endif
    }
    prompt();
}

{WS}*{SOURCE}{WS}+{FILENAME}{WS}*\n {
    char buffer[1024];
    char *args[20];
    FILE *file, *oldin;
    int error, i;
    tokenize(yytext, args);
    error=0;
    if((file=fopen(args[1], "r")) == NULL) {
	printf("ERROR opening file %s\n", args[1]);
	error=1;
    }
    if(!error) {
        no_prompt = TRUE;
	oldin = yyin;
	yyin = file;
	yylex();
	yyrestart(yyin = oldin);
	no_prompt = FALSE;
	fclose(file);
    }
    prompt();
}

{WS}*{UP}{WS}+{VALUE}{WS}+{FILENAME}{WS}*\n {
    unsigned char buffer[1024];
    FILE *file;
    char *args[20];
    int error, i;
    unsigned long addr;
    tokenize(yytext, args);
    error=0;
    if(sscanf(args[1], "%" PRIxMAX, &addr) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if((file=fopen(args[2], "r")) == NULL) {
	printf("ERROR opening file %s\n", args[2]);
	error=1;
    }
    if(!error) {
	printf("Uploading file %s.\n", args[2]);
	while(1) {
	    i=fread(buffer, 1, 1024, file);
	    if(bdm_write_mem(addr, buffer, i)<0) {
		printf("Bus error or target disconnected.\n> ");
		break;
	    }
	    if(i==0) break;
	    addr+=i;
	}
    }
    fclose(file);
    prompt();
}

{WS}*{DOWN}{WS}+{VALUE}{WS}+{VALUE}{WS}+{FILENAME}{WS}*\n {
    unsigned char buffer[1024];
    FILE *file;
    char *args[20];
    int error, i;
    unsigned long addr, len;
    tokenize(yytext, args);
    error=0;
    if(sscanf(args[1], "%" PRIxMAX, &addr) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(sscanf(args[2], "%" PRIxMAX, &len) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if((file=fopen(args[3], "w")) == NULL) {
	printf("ERROR opening file %s for writing.\n", args[3]);
	error=1;
    }
    if(!error) {
	printf("Downloading file %s.\n", args[3]);
	while(len>1024) {
	    if(bdm_read_mem(addr, buffer, 1024)<0) {
		printf("Bus error or target disconnected.\n> ");
		error = 1;
		break;
	    }
	    i=fwrite(buffer, 1, 1024, file);
	    if(i!=1024) {
		printf("Error writing file.\n> ");
		error = 1;
		break;
	    }
	    addr+=1024;
	    len-=1024;
	}
	if(!error) {
	    if(bdm_read_mem(addr, buffer, len)<0) {
		printf("Bus error or target disconnected.\n> ");
		error = 1;
		break;
	    }
	    if(!error) {
		i=fwrite(buffer, 1, len, file);
		if(i!=len) {
		    printf("Error writing file.\n> ");
		    error = 1;
		    break;
		}
	    }
	}
    }
    fclose(file);
    prompt();
}

{WS}*{STEP}{WS}*\n {
    int i, n;
    unsigned long j;

    bdm_step();
    for(i=0; i<10000; i++) j++; /* wait for step */
    for(i=0; i<=REG_CSR; i++) {
        bdm_read_reg(i, &j);
	for(n=0; strlen(reg_names[n].name); n++) {
	    if(reg_names[n].id == i)
		break;
	}
	printf("%8s=%08" PRIxMAX " ", reg_names[n].name, j);
	if(i%4==3) printf("\n");
    }
    if(i%4!=3) printf("\n");
    prompt();
}
{WS}*{STOP}{WS}*\n {
    bdm_stop();
    prompt();
}

{WS}*{RUN}{WS}*\n {
    bdm_run();
    prompt();
}

{WS}*{RESET}{WS}*\n {
    bdm_reset();
    prompt();
}

{WS}*{RESTART}{WS}*\n {
    bdm_restart();
    prompt();
}

{WS}*{FLASHCONFIG}{WS}+{VALUE}{WS}+{VALUE}{WS}+{VALUE}{WS}*\n {
    char *args[20];
    int error, i;
    unsigned int addr, NumParallelChips, ChipWidthBytes;
    tokenize(yytext, args);
    error=0;
    if(sscanf(args[1], "%x", &addr) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(sscanf(args[2], "%x", &NumParallelChips) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(sscanf(args[3], "%x", &ChipWidthBytes) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(!error) {
        printf( "FLASH-ROM @ 0x%x, %d parallel chip(s) each %d byte(s) wide.\n", 
	        addr, 
		NumParallelChips,
		ChipWidthBytes );
	BDMFlashConfigSet( BDMHandle, addr, NumParallelChips, ChipWidthBytes );
	flash_ok=1;
    }
    printf("\n");
    prompt();
}

{WS}*{FLASH}{WS}+{VALUE}{WS}+{FILENAME}{WS}*\n {
    unsigned char buffer[1024];
    FILE *file;
    char *args[20];
    int error, i;
    unsigned long addr;
    tokenize(yytext, args);
    error=0;
    if(sscanf(args[1], "%" PRIxMAX, &addr) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if((file=fopen(args[2], "r")) == NULL) {
	printf("ERROR opening file %s\n", args[2]);
	error=1;
    }
    if(!flash_ok) {
        printf("Please configure flash memory first!");
	error=1;
    }
    if(!error) {
        bdm_stop();
	printf("Programming FLASH-ROM from file %s.\n", args[2]);
	while(1) {
	    i=fread(buffer, 1, 1024, file);
	    if(BDMFlashWrite( addr, buffer, i )!= FlashErrorOkay_c) {
		printf("Bus error or target disconnected.\n"
		       "Successfully wrote up to address 0x%08lx.\n"
		       "Failure was during following 1024 byte block.\n"
		       "> ",
		       addr );
		break;
	    }
	    if(i==0) break;
	    printf("."); fflush(stdout);
	    addr+=i;
	}
    }
    printf("\n");
    fclose(file);
    prompt();
}

{WS}*{ERASE}{WS}*\n {
    if(!flash_ok) {
        printf("Please configure flash memory first!\n");
    } else {
        bdm_stop();
        BDMFlashErase();
    }
    prompt();
}

{WS}*{MAXSTACK}{WS}+{VALUE}{WS}*\n {
    char *args[20];
    int error, i;
    unsigned int val;
    tokenize(yytext, args);
    error=0;
    if(sscanf(args[1], "%x", &val) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(!error) {
        printf("maximum stack position is 0x%08" PRIx32 ".\n", val);
        maxstack=val;
    }
    printf("\n");
    prompt();
}

{WS}*{BACKTRACE}{WS}*\n {
     unsigned long a7, mem, buf;
     bdm_read_reg(REG_A7, &a7);
     printf("Stack is at 0x%08lx\n", a7);
     for(mem=a7; mem<maxstack && mem<a7+4*16; mem+=4) {
         if(bdm_read_mem(mem, (unsigned char *)&buf, 4)<0) {
	     printf("Bus error or target disconnected.\n");
	     break;
	 } else {
	     printf("\t0x%08" PRIx32 "\n", ntohl(buf));
	 }
     }
     prompt(); 
}

{WS}*{BREAK}{WS}+{VALUE}{WS}+{VALUE}{WS}*\n {
    int error;
    unsigned long addr, mask;
    char *args[20];

    bdm_stop();
    tokenize(yytext, args);
    error=0;
    if(sscanf(args[1], "%" PRIxMAX, &addr) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(sscanf(args[2], "%" PRIxMAX, &mask) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(!error) {
        bdm_set_pc_bp(addr, mask);
    } else {
        printf("ERROR\n");
    }
    prompt();
}

{WS}*{WATCH}{WS}+{VALUE}{WS}+{VALUE}{WS}*\n {
    int error;
    unsigned long from, to;
    char *args[20];

    bdm_stop();
    tokenize(yytext, args);
    error=0;
    if(sscanf(args[1], "%" PRIxMAX, &from) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(sscanf(args[2], "%" PRIxMAX, &to) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(!error) {
        bdm_set_addr_bp(from, to);
    } else {
        printf("ERROR\n");
    }
    prompt();
}

{WS}*{TRIGGER}{WS}+{VALUE}{WS}+{VALUE}{WS}*\n {
    int error;
    unsigned long value, mask;
    char *args[20];

    bdm_stop();
    tokenize(yytext, args);
    error=0;
    if(sscanf(args[1], "%" PRIxMAX, &value) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(sscanf(args[2], "%" PRIxMAX, &mask) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(!error) {
        bdm_set_data_bp(value, mask);
    } else {
        printf("ERROR\n");
    }
    prompt();
}

{WS}*{CLEAR}{WS}*\n {
    bdm_stop();
    bdm_clear_pc_bp();
    bdm_clear_data_bp();
    bdm_clear_addr_bp();
    prompt();
}

{WS}*{WAIT}{WS}*\n {
    bdm_wait();
    print_status();
    prompt();
}

{WS}*{LOOP}{WS}+{VALUE}{WS}+{VALUE}{WS}*\n {
    unsigned char buffer[256];
    int i, error;
    unsigned long addr, len;
    char *args[20];

    bdm_stop();
    tokenize(yytext, args);
    error=0;
    if(sscanf(args[1], "%" PRIxMAX, &addr) != 1) {
	printf("ERROR\n");
	error=1;
    }
    if(sscanf(args[2], "%" PRIxMAX, &len) != 1) {
	printf("ERROR\n");
	error=1;
    }
    for(; len>0; len--) {
        if(bdm_read_mem(addr, buffer, 16)<0) {
            /* report error and clear buffer */
            printf("\nBus error or target disconnected.\n");
	    fflush(stdout);
	    for(i=0; i<16; i++) {
		buffer[i] = 0;
            }
	} else {
	    for(i=0; i<16; i++) {
		if(i%16==0) printf("\r%08" PRIxMAX ": ", addr);
		printf("%02" PRIx32 " ", buffer[i]);
		if(i%16==15) printf("\r");
		/* addr++; */
	    }
	    fflush(stdout);
	}
    }
    printf("\n");
    prompt();
}

{WS}*{QUERY}{WS}*\n {
    print_status();
    prompt();
}

{WS}*{EXIT}{WS}*\n {
    printf("BYE\n");
    bdm_release(0);
    exit(0);
}

.+\n {
    printf("ERROR: %s\n> ", yytext);
}

\n {
    int i;
/*    printf("Empty line, last line was: %s\n", buffer); */
    for(i=strlen((char*)buffer)-1; i>=0; i--) {
        unput(buffer[i]);
    }
    buffer[0]=0;
}

%%
#define USAGE "Usage: %s [-f file] [/dev/bdmx]\n"

int main(int argc, char **argv, char **envp)
{
    int base, i, dev, file;
    char *filename, default_file[]="stdin";
    char *devicename, default_device[]="/dev/bdm0";

    filename = default_file;
    devicename = default_device;
    dev = 0;
    file = 0;

    for(i=1; i<argc; i++) {
        if(!strcmp(argv[i], "-f")) {
	    if(!file) {
	        i++;
	        if(i==argc) {
	            fprintf(stderr, USAGE, argv[0]);
		    return -1;
	        }
		filename = argv[i];
		file = -1;
	    } else {
	        fprintf(stderr, USAGE, argv[0]);
		return -1;
	    }
	} else {
	    if(!dev) {
	        devicename = argv[i];
		dev = -1;
	    } else {
	        fprintf(stderr, USAGE, argv[0]);
		return -1;
	    }
	}
    }

    if((BDMHandle = bdm_init(devicename))<0) {
        fprintf(stderr, "Problem opening bdm device.\n");
        return -1;
    }

    if(strcmp(filename, default_file)) {
	yyin = fopen(filename, "r");
	if(!yyin) {
	    fprintf(stderr, 
		    " %s: Can't open file %s for reading.\n",
		    argv[0], argv[1]);
	    return -1;
	}
    } else {
	yyin = stdin;
    }

    print_status();
    printf("> ");
    yylex();
    return 0;
}

void print_status(void)
{
    int x;

    x=bdm_query_status();
    if(x<0) {
        printf("ERROR\n");
    } else {
        if(x & BDM_STAT_BKPT) printf("Processor halted due to BDM.\n");
        if(x & BDM_STAT_HALT) printf("Processor halted due to HALT insn.\n");
        if(x & BDM_STAT_TRG) printf("Processor halted due to TRIGGER.\n");
        if(x & BDM_STAT_FOF) printf("Processor halted due to fault on fault.\n");
        if(x & BDM_STAT_WAIT1) printf("Processor waiting for level 1 trigger.\n");
        if(x & BDM_STAT_TRIG1) printf("Processor hit level 1 trigger.\n");
        if(x & BDM_STAT_WAIT2) printf("Processor waiting for level 2 trigger.\n");
        if(x & BDM_STAT_TRIG2) printf("Processor hit level 2 trigger.\n");
        if(x & BDM_STAT_SSM) printf("Processor single stepping.\n");
        if(x & BDM_STAT_RESET) printf("Processor is held in reset.\n");
        if(x & BDM_STAT_NOPWR) printf("Processor has no power.\n");
        if(x & BDM_STAT_PSTHALT) printf("Processor has posted PST=HALT.\n");
        if(x & BDM_STAT_NC) printf("No target connected.\n");
	if(x==0) printf("Processor running.\n");
    }
}
