## 
## $Id: Makefile.am,v 1.8 2004/01/14 00:09:38 joewolf Exp $
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
bdmctrl_LDFLAGS = -static
bdmctrl_LDADD = \
	$(top_builddir)/lib/libBDM.a \
	$(top_builddir)/flashlib/libbdmflash.a \
	@LIBBFD@ \
	@LIBIBERTY@

bdmreset_SOURCES = \
	bdmreset.c
bdmreset_LDADD = \
	$(top_builddir)/lib/libBDM.a

bdmflash_SOURCES = \
	bdmflash.c
bdmflash_CPPFLAGS = \
	-I$(top_srcdir)/bdmabstraction
bdmflash_LDADD = \
	$(top_builddir)/bdmabstraction/libbdmabstraction.a \
	$(top_builddir)/lib/libBDM.a

bdmmon_SOURCES = \
	bdmmon.l
bdmmon_CFLAGS = @CFLAGS@ -I$(top_srcdir)/bdmabstraction
bdmmon_LDADD = \
	$(top_builddir)/bdmabstraction/libbdmabstraction.a \
	$(top_builddir)/lib/libBDM.a

