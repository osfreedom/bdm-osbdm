CFLAGS = -I../lib -I../driver -O -g -Wall -L../lib

ifeq ($(OSTYPE),cygwin)
EXE=.exe
endif

PROG = chk cpu32-check

all: $(PROG)

chk: chk.c ../lib/libBDM.a
	$(CC) $(CFLAGS) chk.c -lBDM -o chk

cpu32-check: cpu32-check.c ../lib/libBDM.a
	$(CC) $(CFLAGS) cpu32-check.c -lBDM -o cpu32-check

clean:
	rm -f $(PROG) $(PROG).exe *.o 

install:
	cp chk$(EXE) /usr/bin/bdm-chk$(EXE)
	cp cpu32-check$(EXE) /usr/bin/bdm-cpu32-check$(EXE)
