
CFLAGS= -DSCO -D_INKERNEL=1 -DUSE_PST=1 -I..
DRIVER= bdm

all: Driver.o

Driver.o: ../bdm.h ../bdm.c sco.c
	$(CC) $(CFLAGS) -c -o Driver.o sco.c

install: Driver.o
	if /etc/conf/bin/idcheck -p $(DRIVER); \
	then \
		/etc/conf/bin/idinstall -k -a $(DRIVER); \
	else \
		/etc/conf/bin/idinstall -k -u $(DRIVER); \
	fi

clean:
	rm -f Driver.o

