## 
## $Id: Makefile.am,v 1.6 2004/01/07 05:20:20 codewiz Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

INCLUDES = @INCLUDES@ -I$(srcdir)/../driver \
		      -I$(srcdir)/../lib \
		      -I$(srcdir)/../flashlib

CFLAGS = @CFLAGS@

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
bdmctrl_CFLAGS = \
	@BFD_INCLUDE@ @CFLAGS@
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
bdmflash_CFLAGS = \
	@CFLAGS@ -I$(top_srcdir)/bdmabstraction
bdmflash_LDADD = \
	$(top_builddir)/bdmabstraction/libbdmabstraction.a \
	$(top_builddir)/lib/libBDM.a

bdmmon_SOURCES = \
	bdmmon.l
bdmmon_CFLAGS = @CFLAGS@ -I$(top_srcdir)/bdmabstraction
bdmmon_LDADD = \
	$(top_builddir)/bdmabstraction/libbdmabstraction.a \
	$(top_builddir)/lib/libBDM.a

