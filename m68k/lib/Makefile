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

CFLAGS := -Wall -g -O4 -I. -I../driver $(BUILD)

ifeq ($(OSTYPE),cygwin)
# work around cygwin issue about the location of windows.h
CFLAGS += -I /usr/include/w32api
else
CFLAGS += -DBDM_IOPERM
endif

all: $(LIB)

$(LIB): $(LIB)(bdmIO.o) $(LIB)(bdmRemote.o)
	ranlib $(LIB)

install: $(LIB)
	-mkdir -p $(INCDIR) $(LIBDIR)
	cp -f ../driver/bdm.h $(INCDIR)/bdm.h
	cp -f BDMlib.h $(INCDIR)/BDMlib.h
	cp -f $(LIB) $(LIBDIR)/$(LIB)
	ranlib $(LIBDIR)/$(LIB)

bdmIO.o: bdmIO.c bdmRemote.h BDMlib.h \
	../driver/ioperm/ioperm.c \
	../driver/win/win-bdm.c ../driver/win/win-io.h

bdmRemote.o: bdmRemote.c bdmRemote.h BDMlib.h

clean:
	rm -f *.o $(LIB)
