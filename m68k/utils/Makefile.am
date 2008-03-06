## 
## $Id: Makefile.am,v 1.11 2008/03/06 10:35:40 cjohns Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

AM_CPPFLAGS = -I$(srcdir)/../driver \
	      -I$(srcdir)/../lib

##
## Only way I know of anding in automake.
##

if WIN32
if BDM_REMOTE
LIBS += -l wsock32
endif
endif

if TBLCF_USB
TBLCF_USB_LIB = $(top_builddir)/tblcf/libtblcf.a
endif

if LIBUSB_PATH
AM_CPPFLAGS += -I@LIBUSB_INCLUDE_DIR@
AM_LDFLAGS = -L@LIBUSB_LIB_DIR@
endif

bin_PROGRAMS = bdmreset

if BDMCTRL
bin_PROGRAMS += bdmctrl
endif

if BDMFLASH
bin_PROGRAMS += bdmflash
endif

if BDMMON
bin_PROGRAMS += bdmmon
endif

bdmctrl_SOURCES = \
	bdmctrl.c
bdmctrl_CPPFLAGS = \
	-I$(srcdir)/../flashlib \
	@BFD_INCLUDE@ @CFLAGS@ $(AM_CPPFLAGS)
bdmctrl_LDADD = \
	$(top_builddir)/lib/libBDM.a \
	$(top_builddir)/flashlib/libbdmflash.a \
	$(TBLCF_USB_LIB) \
	@LIBBFD@ \
	@LIBIBERTY@

bdmreset_SOURCES = \
	bdmreset.c
bdmreset_LDADD = \
	$(top_builddir)/lib/libBDM.a \
	$(TBLCF_USB_LIB)

bdmflash_SOURCES = \
	bdmflash.c
bdmflash_CPPFLAGS = \
	-I$(top_srcdir)/bdmabstraction
bdmflash_LDADD = \
	$(top_builddir)/bdmabstraction/libbdmabstraction.a \
	$(top_builddir)/lib/libBDM.a \
	$(TBLCF_USB_LIB)

bdmmon_SOURCES = \
	bdmmon.l
bdmmon_CFLAGS = @CFLAGS@ -I$(top_srcdir)/bdmabstraction
bdmmon_LDADD = \
	$(top_builddir)/bdmabstraction/libbdmabstraction.a \
	$(top_builddir)/lib/libBDM.a \
	$(TBLCF_USB_LIB)

EXTRA_DIST = README.bdmctrl m68332.test
