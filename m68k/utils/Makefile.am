## 
## $Id: Makefile.am,v 1.14 2008/07/31 01:53:44 cjohns Exp $
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

AM_CFLAGS = -Wall

##
## Only way I know of anding in automake.
##

if WIN32
if BDM_REMOTE
LIBS += -l wsock32
endif
endif

if LIB_BDMUSB
BDMUSB_LIB = $(top_builddir)/libbdmusb/libbdmusb.a
endif

if BDM_USB
AM_CPPFLAGS += -I@LIBUSB_INCLUDE_DIR@
AM_LDFLAGS = -L@LIBUSB_LIB_DIR@
LIBS += -l@LIBUSB_LIB_NAME@
endif

LIBELF = $(top_builddir)/libelf/lib/libelf.a

bin_PROGRAMS = bdmreset usb-show

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
	-I$(srcdir)/../libelf/lib -I../libelf/lib \
        $(AM_CPPFLAGS)
bdmctrl_LDADD = \
	$(top_builddir)/lib/libBDM.a \
	$(top_builddir)/flashlib/libbdmflash.a \
	$(BDMUSB_LIB) \
	$(LIBELF)

bdmreset_SOURCES = \
	bdmreset.c
bdmreset_LDADD = \
	$(top_builddir)/lib/libBDM.a \
	$(BDMUSB_LIB)

bdmflash_SOURCES = \
	bdmflash.c
bdmflash_CPPFLAGS = \
	-I$(top_srcdir)/bdmabstraction
bdmflash_LDADD = \
	$(top_builddir)/bdmabstraction/libbdmabstraction.a \
	$(top_builddir)/lib/libBDM.a \
	$(BDMUSB_LIB)

bdmmon_SOURCES = \
	bdmmon.l
bdmmon_CFLAGS = @CFLAGS@ -I$(top_srcdir)/bdmabstraction
bdmmon_LDADD = \
	$(top_builddir)/bdmabstraction/libbdmabstraction.a \
	$(top_builddir)/lib/libBDM.a \
	$(BDMUSB_LIB)

usb_show_SOURCES = \
	usb-show.c 

usb_show_LDADD =  \
	$(top_builddir)/lib/libBDM.a \
	$(top_builddir)/tblcf/libbdmusb.a

EXTRA_DIST = README.bdmctrl m68332.test mcf52235.test mcf5235.test mcf5329.test
