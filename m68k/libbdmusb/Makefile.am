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

AM_CPPFLAGS = -DBDM_DEVICE_USB=1 -I$(srcdir)/../driver -I$(srcdir)/../lib
AM_CFLAGS = -Wall
AM_LDFLAGS = 

if LIBUSB_PATH
AM_CPPFLAGS += -I@LIBUSB_INCLUDE_DIR@
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

bin_PROGRAMS = bdmusb-show bdmusb-boot bdmusb-unsec

bdmusb_show_SOURCES = \
	bdmusb-show.c \
	log.c

bdmusb_show_LDADD =  \
	$(top_builddir)/lib/libBDM.a \
	$(top_builddir)/tblcf/libbdmusb.a

bdmusb_boot_SOURCES = \
	bdmusb_bt.c \
	log_cmdline.c \
	srec.c

bdmusb_boot_LDADD =  \
	$(top_builddir)/tblcf/libbdmusb.a \
	$(top_builddir)/lib/libBDM.a \
	$(top_builddir)/tblcf/libtblcf.a

bdmusb_unsec_SOURCES = \
	bdmusb_unsec.c \
	log_cmdline.c

bdmusb_unsec_LDADD =  \
	$(top_builddir)/tblcf/libbdmusb.a \
	$(top_builddir)/lib/libBDM.a 

lib_LIBRARIES = libbdmusb.a

include_HEADERS = \
	bdmusb.h \
	tblcf.h

libbdmusb_a_SOURCES = \
	bdmusb.c \
	bdm-usb.c \
	tblcf/tblcf.c \
	tblcf/tblcf_usb.c \
	log.c
