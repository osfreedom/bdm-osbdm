## 
## $Id: Makefile.am,v 1.3 2004/01/07 12:49:04 joewolf Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

INCLUDES = @INCLUDES@ -I$(srcdir)/../driver -I$(srcdir)/../lib

CFLAGS = @BFD_INCLUDE@ @CFLAGS@

if BDMFLASHLIB

pkglib_LIBRARIES = libbdmflash.a

include_HEADERS = \
	bdmfilt.h \
	bdmflash.h \
	flash_filter.h \
	flash29.h

libbdmflash_a_SOURCES = \
	bdmfilt.c \
	bdmflash.c \
	flash_filter.c \
	flash29.c

endif
