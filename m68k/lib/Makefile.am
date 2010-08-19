## 
## $Id: Makefile.am,v 1.6 2008/03/06 10:35:40 cjohns Exp $
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

DRIVER_SRC = ../driver/bdm-user-land.c
DRIVER_HDR = ../driver/bdm.h

if BDM_IOPERM
AM_CFLAGS += -DBDM_DEVICE_IOPERM=1
if WIN32
DRIVER_SRC += win-ioperm.c
else
DRIVER_SRC += ioperm.c
endif
endif

if BDM_REMOTE
AM_CFLAGS += -DBDM_DEVICE_REMOTE=1
DRIVER_SRC += bdmRemote.c
DRIVER_HDR += bdmRemote.h
endif

if BDM_DRIVER
AM_CFLAGS += -DBDM_DEVICE_LOCAL=1
DRIVER_SRC += localIface.c
DRIVER_HDR += localIface.h
endif

if BDM_USB
# Fix when making USB generic
AM_CFLAGS += -DBDM_DEVICE_USB=1 -DBDM_TBLCF_USB=1
AM_CPPFLAGS += -I$(srcdir)/../tblcf
endif

lib_LIBRARIES = libBDM.a

include_HEADERS = \
	BDMlib.h \
	$(DRIVER_HDR)

libBDM_a_SOURCES = \
	bdmIO.c \
	bdmRemote.c \
	$(DRIVER_SRC)

EXTRA_DIST = \
        ../driver/bdm.c
