## 
## $Id: Makefile.am,v 1.5 2004/03/21 01:48:20 codewiz Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

AM_CPPFLAGS = -I$(srcdir)/../driver -I$(srcdir)/../lib
AM_CFLAGS = @BFD_INCLUDE@ @CFLAGS@

if BDMFLASHLIB

lib_LIBRARIES = libbdmflash.a

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
