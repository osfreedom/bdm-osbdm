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

AM_CPPFLAGS = -I$(srcdir)/../driver -I$(srcdir)/../lib
AM_CFLAGS = -Wall -g

if LIBUSB_PATH
AM_CPPFLAGS += -I@LIBUSB_INCLUDE_DIR@
AM_LDFLAGS = -L@LIBUSB_LIB_DIR@
endif

if WIN32
LIBS += -l wsock32
endif

bin_PROGRAMS = tblcf-show tblcf-boot tblcf-unsec

tblcf_show_SOURCES = \
	tblcf-show.c \
	log.c

tblcf_show_LDADD =  \
	$(top_builddir)/lib/libBDM.a \
	$(top_builddir)/tblcf/libtblcf.a

tblcf_boot_SOURCES = \
	tblcf_bt.c \
	log_cmdline.c \
	srec.c

tblcf_boot_LDADD =  \
	$(top_builddir)/tblcf/libtblcf.a \
	$(top_builddir)/lib/libBDM.a \
	$(top_builddir)/tblcf/libtblcf.a

tblcf_unsec_SOURCES = \
	tblcf_unsec.c \
	log_cmdline.c

tblcf_unsec_LDADD =  \
	$(top_builddir)/lib/libBDM.a \
	$(top_builddir)/tblcf/libtblcf.a

lib_LIBRARIES = libtblcf.a

include_HEADERS = \
	tblcf.h

libtblcf_a_SOURCES = \
	tblcf.c \
	tblcf_usb.c \
	log.c
