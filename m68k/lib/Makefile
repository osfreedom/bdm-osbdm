#
# Build a library for BDM applications.
#
# Set the defines below to `'y' or `n' enable local driver and/or remote
# operations.
#

BUILD=

DO_REMOTE=y
DO_LOCAL=y

prefix := /usr

ifeq ($(DO_REMOTE),y)
BUILD += -DBDM_DEVICE_REMOTE
endif

ifeq ($(DO_LOCAL),y)
BUILD += -DBDM_DEVICE_LOCAL
endif

INCDIR := $(prefix)/include
LIBDIR := $(prefix)/lib

LIB    := libBDM.a

CFLAGS := -Wall -g -O -I. -I../driver $(BUILD)

ifeq ($(OSTYPE),cygwin)
CFLAGS += -I /usr/include/w32api
endif

all: $(LIB)

$(LIB): $(LIB)(bdmIO.o) $(LIB)(bdmBFD.o)
	ranlib $(LIB)

install: $(LIB)
	-mkdir -p $(INCDIR) $(LIBDIR)
	cp -f ../driver/bdm.h $(INCDIR)/bdm.h
	cp -f BDMlib.h $(INCDIR)/BDMlib.h
	cp -f $(LIB) $(LIBDIR)/$(LIB)
	ranlib $(LIBDIR)/$(LIB)

bdmIO.o: bdmRemote.c BDMlib.h \
	../driver/win/win-bdm.c ../driver/win/win-io.h

clean:
	rm -f *.o $(LIB)
