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

INCLUDES = -I$(srcdir)/../lib -I$(srcdir)/../driver

CFLAGS = @BDM_CFLAGS@

sbin_PROGRAMS = bdmd

bdmd_SOURCES = \
	bdmServer.c \
	xmalloc.c
bdmd_LDADD = ../lib/libBDM.a
