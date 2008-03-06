## 
## $Id: Makefile.am,v 1.4 2008/03/06 10:35:40 cjohns Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

AM_CPPFLAGS = -I$(srcdir)/../lib -I$(srcdir)/../driver

if TBLCF_USB
TBLCF_USB_LIB = $(top_builddir)/tblcf/libtblcf.a
endif

if LIBUSB_PATH
AM_CPPFLAGS += -I@LIBUSB_INCLUDE_DIR@
AM_LDFLAGS = -L@LIBUSB_LIB_DIR@
endif

sbin_PROGRAMS = bdmd

bdmd_SOURCES = \
	bdmServer.c \
	xmalloc.c
bdmd_LDADD = ../lib/libBDM.a $(TBLCF_USB_LIB)
