## 
## $Id: Makefile.am,v 1.3 2003/11/26 21:20:26 codewiz Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

INCLUDES = @INCLUDES@ -I$(srcdir)/../driver

CFLAGS = @CFLAGS@

if BDM_IOPERM
CFLAGS += -DBDM_IOPERM
endif

if BDM_REMOTE
CFLAGS += -DBDM_DEVICE_REMOTE
endif

if BDM_DRIVER
CFLAGS += -DBDM_DEVICE_LOCAL
endif

pkglib_LIBRARIES = libBDM.a

include_HEADERS = \
	BDMlib.h \
	bdmRemote.h \
	../driver/bdm.h

libBDM_a_SOURCES = \
	bdmIO.c \
	bdmRemote.c

# FIXME: these would belong to driver/Makefile.am, which we don't have
EXTRA_DIST = \
	../driver/bdm.c \
	../driver/ioperm/ioperm.c
