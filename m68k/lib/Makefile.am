## 
## $Id: Makefile.am,v 1.4 2004/01/08 20:37:25 codewiz Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

AM_CPPFLAGS = -I$(srcdir)/../driver
AM_CFLAGS =

if BDM_IOPERM
AM_CFLAGS += -DBDM_IOPERM
endif

if BDM_REMOTE
AM_CFLAGS += -DBDM_DEVICE_REMOTE
endif

if BDM_DRIVER
AM_CFLAGS += -DBDM_DEVICE_LOCAL
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
