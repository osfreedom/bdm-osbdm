## 
## $Id: Makefile.am,v 1.1 2003/12/29 22:18:49 codewiz Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

INCLUDES = @INCLUDES@ -I$(top_srcdir)/driver -I$(top_srcdir)/lib

CFLAGS = @CFLAGS@

noinst_LIBRARIES = libbdmabstraction.a

libbdmabstraction_a_SOURCES = \
	BDMTargetAddress.c \
	BDMFlash.c \
	bdmops.c \
	Flash.c

noinst_HEADERS = \
	bdmcf.h \
	BDMDriver.h \
	BDMFlash.h \
	bdmops.h \
	BDMTargetAddress.h \
	Debug.h \
	Flash.h
