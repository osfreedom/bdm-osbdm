## 
## $Id: Makefile.am,v 1.1 2008/03/06 10:35:40 cjohns Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

AM_CPPFLAGS = -DBDM_DEVICE_USB=1 -I$(srcdir)/../driver -I$(srcdir)/../lib -DBDM_USERLAND_LIB
AM_CFLAGS = -Wall
AM_LDFLAGS = 

if LIBUSB_PATH
AM_CPPFLAGS += -g -I@LIBUSB_INCLUDE_DIR@ -I$(srcdir)/libbdmusb/tblcf
AM_LDFLAGS += -L@LIBUSB_LIB_DIR@
endif
LIBS += -l usb-1.0

if LIBUSB_VER_0_0
AM_CPPFLAGS += -DLIBUSB_VER_0_0=1
else
AM_CPPFLAGS += -DLIBUSB_VER_1_0=1
endif

if WIN32
LIBS += -l wsock32
endif

bin_PROGRAMS = bdmusb-boot bdmusb-unsec

bdmusb_boot_SOURCES = \
	bdmusb_bt.c \
	log_cmdline.c \
	srec.c

bdmusb_boot_LDADD =  \
	$(top_builddir)/libbdmusb/libbdmusb.a \
	$(top_builddir)/lib/libBDM.a \
	$(top_builddir)/libbdmusb/libbdmusb.a 

bdmusb_unsec_SOURCES = \
	bdmusb_unsec.c \
	log_cmdline.c

bdmusb_unsec_LDADD =  \
	$(top_builddir)/libbdmusb/libbdmusb.a \
	$(top_builddir)/lib/libBDM.a \
	$(top_builddir)/libbdmusb/libbdmusb.a 

lib_LIBRARIES = libbdmusb.a

include_HEADERS = \
	bdmusb.h \
	tblcf/tblcf.h \
	tblcf/tblcf_usb.h

libbdmusb_a_SOURCES = \
	bdmusb.c \
	bdm-usb.c \
	bdmusb_low_level.c \
	tblcf/tblcf.c \
	tblcf/tblcf_usb.c \
	usbdm/usbdm.c \
	log.c
