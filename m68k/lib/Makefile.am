## 
## $Id: Makefile.am,v 1.2 2003/11/06 21:02:25 joewolf Exp $
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
	../driver/bdm.h

libBDM_a_SOURCES = \
	bdmIO.c \
	bdmRemote.c

