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

INCLUDES = @INCLUDES@ -I$(srcdir)/../lib -I$(srcdir)/../driver

CFLAGS = @CFLAGS@

sbin_PROGRAMS = bdmd

bdmd_SOURCES = \
	bdmServer.c \
	xmalloc.c
bdmd_LDADD = ../lib/libBDM.a
