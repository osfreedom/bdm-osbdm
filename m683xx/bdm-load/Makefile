#TARGET_ARCH = -bi586-pc-linux-gnulibc1
#CC = egcc

CFLAGS	+= -Wall -ggdb -O2
CFLAGS  += -Wstrict-prototypes
CFLAGS	+= ${TARGET_ARCH}

#/* Select right place or libraries with configured m68k-coff support */
LDLIBS += -lbfd
#CFLAGS	+= -I ./gdblibs
#LOCLIBS += ./gdblibs/libbfd.a ./gdblibs/libiberty.a
#LDFLAGS += -lintl
LDLIBS += -liberty
LDFLAGS	+= ${TARGET_ARCH}

all : bdm-load bdm_test tpudb

dep:
	$(CC) $(CFLAGS) $(CPPFLAGS) -w -E -M *.c $(MORE_C_FILES) > depend

depend:
	@touch depend

clean :
	rm -f *.o *.lo *~ *.a *.so *.so.* depend

bdm-load : bdm-load.o bdmlib.o bdmflash.o ${LOCLIBS}

bdm_test : bdm_test.o bdmlib.o ${LOCLIBS}

tpudb : tpudb.o bdmlib.o tpudis.o ${LOCLIBS}

-include depend
