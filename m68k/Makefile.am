## 
## $Id: Makefile.am,v 1.8 2004/06/01 02:02:25 codewiz Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

CFLAGS = @CFLAGS@

SUBDIRS = @BDM_SUBDIRS@

EXTRA_DIST = \
	README.insight \
	BUGS \
	config/bootstrap \
	driver/freebsd/bdm \
	driver/freebsd/bdm.8 \
	driver/freebsd/freebsd-bdm.c \
	driver/freebsd/Makefile \
	driver/freebsd/README \
	driver/freebsd/setdef0.c \
	driver/freebsd/setdef1.c \
	driver/freebsd/unbdm \
	driver/freebsd/bus_if.h \
	driver/freebsd/device_if.h \
	driver/freebsd/isa_if.h \
	driver/freebsd/setdefs.h \
	driver/openbsd/Makefile \
	driver/openbsd/README \
	driver/openbsd/bdm-install.sh \
	driver/openbsd/openbsd-bdm.c \
	driver/linux/linux-bdm.c \
	driver/linux/Makefile \
	driver/linux-2.6/bdm.c \
	driver/linux-2.6/Makefile \
	driver/sco/Makefile \
	driver/sco/Master \
	driver/sco/Node \
	driver/sco/README \
	driver/sco/sco.c \
	driver/sco/Space.c \
	driver/sco/System \
	driver/win/win-bdm.c \
	driver/win/win-io.h

dist-hook:
	cp -R -p $(srcdir)/local_scripts $(distdir)
	rm -r -f `find $(distdir)/local_scripts -name CVS`
	cp -R -p $(srcdir)/../gdb $(distdir)
	rm -r -f `find $(distdir)/gdb -name CVS`
	cp -R -p $(srcdir)/../schematics $(distdir)
	rm -r -f `find $(distdir)/schematics -name CVS`

