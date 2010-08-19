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

if LIB_BDMUSB
BDMUSB_LIB = $(top_builddir)/libbdmusb/libbdmusb.a
endif

if BDM_USB
AM_CPPFLAGS += -I@LIBUSB_INCLUDE_DIR@
AM_LDFLAGS = -L@LIBUSB_LIB_DIR@
LIBS += -l@LIBUSB_LIB_NAME@
endif

sbin_PROGRAMS = bdmd

bdmd_SOURCES = \
	bdmServer.c \
	xmalloc.c
bdmd_LDADD = ../lib/libBDM.a $(BDMUSB_LIB)
