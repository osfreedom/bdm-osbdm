CFLAGS = -I../lib -I../driver -O -g -Wall -L../lib

PROGS = bdmreset

prefix := /usr

ifeq ($(OSTYPE),cygwin)
EXE=.exe
endif

all: $(PROGS)$(EXT)

bdmreset: ../lib/libBDM.a

%.: %.c
	$(CC) $(CFLAGS) -o $(@F) $< -lBDM

install:
	-mkdir -p $(prefix)/bin
	cp bdmreset$(EXE) $(prefix)/bin/bdmreset$(EXE)

clean:
	rm -f $(PROGS)$(EXE) *.o 
