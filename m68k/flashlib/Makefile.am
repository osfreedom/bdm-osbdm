## 
## $Id: Makefile.am,v 1.7 2008/06/16 00:01:21 cjohns Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

AM_CPPFLAGS = -I$(srcdir)/../libelf/lib -I../libelf/lib \
              -I$(srcdir)/../driver -I$(srcdir)/../lib

lib_LIBRARIES = libbdmflash.a

include_HEADERS = \
	bdmfilt.h \
	bdmflash.h \
	flash_filter.h \
	flash29.h \
	flashcfm.h

libbdmflash_a_SOURCES = \
	elf-utils.c \
	bdmfilt.c \
	bdmflash.c \
	flash_filter.c \
	flash29.c \
	flashcfm.c

EXTRA_DIST = README tcompile
