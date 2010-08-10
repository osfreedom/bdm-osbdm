## 
## $Id: Makefile.am,v 1.8 2008/06/16 00:01:21 cjohns Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

AM_CPPFLAGS = -I$(srcdir)/../driver \
	      -I$(srcdir)/../lib \
	      -DGDBSERVER=1 \
	      -DNO_REMOTE_ASYNC=1 \
	      -DNO_PASS_SIGNALS=1 \
	      -DNO_LIBRARIES=1
AM_CFLAGS = -Wall

##
## XML files.
##
XML_FILES = \
	$(srcdir)/m68k-core.xml \
	$(srcdir)/m68k-fpu.xml \
	$(srcdir)/m68k-cpu32.xml \
	$(srcdir)/m68k-cpu32plus.xml \
	$(srcdir)/m68k-cf5200.xml \
	$(srcdir)/m68k-cf5235.xml \
	$(srcdir)/m68k-cf5272.xml \
	$(srcdir)/m68k-cf5282.xml \
	$(srcdir)/m68k-cf52223.xml \
	$(srcdir)/m68k-cf5307.xml \
	$(srcdir)/m68k-cfv4e.xml

##
## Only way I know of anding in automake.
##

if WIN32
AM_CFLAGS += -DUSE_WIN32API=1
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

if GDBSERVER
bin_PROGRAMS = m68k-bdm-gdbserver
endif

m68k_bdm_gdbserver_SOURCES = \
	inferiors.c m68k-bdm-low.c mem-break.c \
	regcache.c remote-utils.c \
	server.c signals.c target.c utils.c version.c \
	xml-builtin.c \
	m68k-core-regs.c m68k-cpu32-regs.c m68k-cpu32plus-regs.c\
	m68k-cf5200-regs.c m68k-cf52223-regs.c m68k-cf5235-regs.c \
	m68k-cf5272-regs.c m68k-cf5282-regs.c m68k-cf5307-regs.c \
	m68k-cfv4e-regs.c 

m68k_bdm_gdbserver_CPPFLAGS = \
	@CFLAGS@ $(AM_CPPFLAGS)
m68k_bdm_gdbserver_LDADD = \
	$(top_builddir)/lib/libBDM.a \
	$(TBLCF_USB_LIB)

version.c: Makefile $(srcdir)/version.in
	rm -f version.c-tmp version.c
	echo '#include "server.h"' >> version.c-tmp
	echo 'const char version[] = "'"`sed q ${srcdir}/version.in`"'";' >> version.c-tmp
	echo 'const char host_name[] = "$(host_alias)";' >> version.c-tmp
	mv version.c-tmp version.c

xml-builtin.c: $(srcdir)/feature_to_c.sh Makefile $(XML_FILES)
	rm -f xml-builtin.tmp
	sh $(srcdir)/feature_to_c.sh xml-builtin.tmp $(XML_FILES)
	mv xml-builtin.tmp xml-builtin.c
	echo stamp > stamp-xml
