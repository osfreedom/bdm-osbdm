prefix = /usr

CFLAGS = -Wall -g -O4 -pipe -I. -I../lib -I../driver

all: bdmd

bdmd: bdmServer.o xmalloc.o ../lib/libBDM.a
	$(CC) -o bdmd bdmServer.o xmalloc.o -L../lib -lBDM

install:
	install -m 755 bdmd /usr/sbin/bdmd

clean:
	rm -f *.o bdmd
