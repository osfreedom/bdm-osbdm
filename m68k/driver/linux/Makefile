#*******************************************************************
# Motorola m68k BDM Driver for Linux Kernel and GNU Debugger
#
# Makefile-mod - Makefile for regular compilation 
#                through Linux kernel rules
#
# This file must be renamed or linked to Makefile
#
# (C) Copyright 1999 by Pavel Pisa 
#
# The BDM driver is distributed under the Gnu General Public Licence. 
# See file COPYING for details.
#*******************************************************************/

# Use "make MODULE_NAME=foobar" if you want to give the driver module
# a different name. This might be useful if you want to install both,
# Chris's and Pavel's drivers at the same machine.
DEFAULT_MODULE_NAME=m68k-bdm
MODULE_NAME=$(DEFAULT_MODULE_NAME)

ifneq ($(MODULE_NAME),linux-bdm)
CLEAN_MODULE_TEMP=rm -f $(MODULE_NAME).c
endif

# currently running kernel
CURRENT=$(shell uname -r)
KERNEL_NEW=$(shell if [ -d /lib/modules/$(CURRENT)/build ] ; \
		then echo yes ; else echo no ; fi )

# Where to look for kernel
#KERNEL_LOCATION=/usr/src/linux
#KERNEL_LOCATION=/usr/src/kernel/$(CURRENT)
#KERNEL_LOCATION=/lib/modules/$(CURRENT)/build
#KERNEL_LOCATION=/usr/src/linux-2.2.19
#KERNEL_LOCATION=/usr/src/linux-2.5.60

ifndef KERNEL_LOCATION
ifeq ($(KERNEL_NEW),yes)
KERNEL_LOCATION=/lib/modules/$(CURRENT)/build
MODULE_CHAR_LOC=/lib/modules/$(CURRENT)/kernel/drivers/char
else
KERNEL_LOCATION=/usr/src/linux
MODULE_CHAR_LOC=/lib/modules/$(CURRENT)/misc
endif
endif

# Test for latest 2.5.xx and future 2.6.xx kernels
KERNEL_VERSION := $(shell awk -F\" '/REL/ {print $$2}' \
	$(KERNEL_LOCATION)/include/linux/version.h | awk -F\- '{print $$1}')

ifeq ($(KERNEL_VERSION),)
KERNEL_VERSION := $(CURRENT)
endif

KERNEL_MODULE_V26 := $(shell echo $(KERNEL_VERSION) \
	| sed -n 's/^.*2\.[5-9]\..*$$/yes/p')

# Target object file if any
O_TARGET     :=
# Regular object files
O_OBJS       = $(MODULE_NAME).o
# Objects with exported symbols (-DEXPORT_SYMTAB)
OX_OBJS      =
# Module objects 
M_OBJS       = $(O_OBJS)
# Module only objects with exported symbols (-DEXPORT_SYMTAB)
MX_OBJS      = 
# Kernel only objects 
L_OBJS       =	
# Kernel only objects with exported symbols (-DEXPORT_SYMTAB)
LX_OBJS      = 
# Additional CFLAGS
EXTRA_CFLAGS =

# Linux 2.4.2 build system needs next
obj-m += $(O_OBJS)

ifndef KERNEL_MODULE_V26
FINAL_MODULE_OBJS=$(obj-m)
else
FINAL_MODULE_OBJS=$(obj-m:%.o=%.ko)
endif

all : make_this_module

install : install_this_module

$(MODULE_NAME).c: linux-bdm.c ../bdm.c ../bdm.h
	cp $< $@

make_this_module: $(MODULE_NAME).c
	DIR=`pwd`; (cd $(KERNEL_LOCATION); make SUBDIRS=$$DIR modules)
	$(CLEAN_MODULE_TEMP)

install_this_module: make_this_module
	su -c "mkdir -v -p $(MODULE_CHAR_LOC) && cp -v $(FINAL_MODULE_OBJS) $(MODULE_CHAR_LOC)"
	$(CLEAN_MODULE_TEMP)

clean:
	rm -f $(M_OBJS) $(MX_OBJS) *.ko .*.o.flags .*.o.cmd .*.ko.cmd .depend *~ 

ifneq ($(KERNEL_MODULE_V26),yes)
include $(KERNEL_MODULE_V26)
export TOPDIR=$(KERNEL_LOCATION)
include $(KERNEL_LOCATION)/Rules.make
endif

