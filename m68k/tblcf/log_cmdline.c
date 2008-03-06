/*
    Turbo BDM Light ColdFire
    Copyright (C) 2006  Daniel Malik

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "log_cmdline.h"

static int log=0;
#ifdef DETAILED_LOG_CAPABILITY
	static int dbg_level=0;
#endif
FILE *log_file;

/* prints messages on stdout and into log file */
int print_screen(const char *format, ...) {
	va_list list;
	int i;
	va_start(list, format);
	i=vprintf(format, list);
	if (log) vfprintf(log_file, format, list);
	fflush(log_file);
	va_end(list);
	return(i);
}

#ifdef DETAILED_LOG_CAPABILITY
	/* prints messages on stdout and into log file if level < dbg_level level */
	void print_screen_level(unsigned char msg_level, const char *format, ...) {
		va_list list;
		if (dbg_level<msg_level) return;
		va_start(list, format);
		vprintf(format, list);
		if (log) {
			fprintf(log_file, "%04.3f: ",1.0*GetTickCount()/1000);
			vfprintf(log_file, format, list);
			fflush(log_file);
		}
		va_end(list);
		return;
	}

	/* same as print_screen_level, but takes va_list instead of ... */
	void vprint_screen_level(unsigned char msg_level, const char *format, va_list list) {
		if (dbg_level<msg_level) return;
		vprintf(format, list);
		if (log) {
			fprintf(log_file, "%04.3f: ",1.0*GetTickCount()/1000);
			vfprintf(log_file, format, list);
			fflush(log_file);
		}
		return;
	}

	/* prints last system error in human readable form */
	void print_last_error_level(unsigned char msg_level, int error) {
		char buffer[100];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,error,0,buffer,sizeof(buffer),0);
		print_screen_level(msg_level," %s",buffer);
	}

	/* sets debugging level */
	void set_dbg_level(int new_dbg_level) {
		dbg_level=new_dbg_level;
	}

	/* prints data in hex format */
	void print_dump_level(unsigned char msg_level, unsigned char *data, unsigned int size) {
		unsigned int i=0;
		if (dbg_level<msg_level) return;
		while(size--) {
			printf("%02X ",*data);
			if (log) fprintf(log_file, "%02X ",*data);
			data++;
			if (((++i)%16)==0) {
				printf("\n");
				if ((log)&&((i%32)==0)) fprintf(log_file,"\n");
			}
			if ((i%8)==0) {
				if ((i%16)!=0) printf(" ");
				if ((log)&&((i%32)!=0)) fprintf(log_file," ");
			}
		}
		if ((i%32)!=0) {
			if ((i%16)!=0) printf("\n");
			if (log) fprintf(log_file,"\n");
		}
		fflush(log_file);
	}
#endif

/* opens log file */
int open_log(char *path) {
	time_t timer;
	log_file=fopen(path,"wb");
	if (log_file==NULL) {
		printf("Cannot open log file \"%s\"\n",path);
		return(-1);
	}
	if (atexit(close_log)) {
		printf("Error registering atexit function: %s\n",path);
		return(-2);
	}
	printf("Log file: %s\n",path);
	log=1;
	time(&timer);
	print_screen("Timestamp: %s\r", ctime(&timer));
	return(0);
}

/* closes log file */
void close_log(void) {
	if (log) {
		fprintf(log_file, "Closing the log file\n");
		fclose(log_file);
	}
}
