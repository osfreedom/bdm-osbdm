prefix = /usr

OSVER   = $(shell uname -r)
INCDIR	= /lib/modules/$(OSVER)/build/include
MODDIR	= /lib/modules/$(OSVER)/misc

ifneq ($(INCDIR),$(wildcard $(INCDIR)))
INCDIR  = /usr/include
endif

# If the kernel is configured for module versions, uncomment this line
# If the kernel is configured for module versions and you fail to uncomment
# this line the driver will install and work properly, but depmod will
# complain about unresolved symbols.
#MODVERSIONS=-DMODVERSIONS -include $(INCDIR)/linux/modversions.h

LIB	= libBDM.a

CFLAGS = -Wall -g -O -pipe -DMODULE -D__KERNEL__ -I$(INCDIR) -I.. $(MODVERSIONS) 

all: linux-bdm.o

$(LIB): $(LIB)(bdmIO.o)
	ranlib $(LIB)

install: all
	-mkdir -p $(INCDIR) $(MODDIR)
	cp ../bdm.h $(INCDIR)/bdm.h
	cp linux-bdm.o $(MODDIR)/bdm.o
	/sbin/depmod -a

linux-bdm.o: ../bdm.h ../bdm.c

clean:
	rm -f *.o $(LIB)
