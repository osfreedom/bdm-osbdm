## 
## $Id: Makefile.am,v 1.1 2003/11/10 23:25:00 ppisa Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

INCLUDES = @INCLUDES@ -I$(srcdir)/../driver -I$(srcdir)/../lib

CFLAGS = @CFLAGS@

if BDMFLASHLIB

pkglib_LIBRARIES = libbdmflash.a

include_HEADERS = \
	bdmfilt.h \
	bdmflash.h

libbdmflash_a_SOURCES = \
	bdmfilt.c \
	bdmflash.c

endif
