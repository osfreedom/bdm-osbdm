## 
## $Id: Makefile.am,v 1.1 2003/09/17 15:24:44 cjohns Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

INCLUDES = -I$(srcdir)/../driver

CFLAGS = @BDM_CFLAGS@

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
	../driver/bdm.h

libBDM_a_SOURCES = \
	bdmIO.c \
	bdmRemote.c

