## 
## $Id: Makefile.am,v 1.3 2004/01/08 20:37:25 codewiz Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

AM_CPPFLAGS = -I$(srcdir)/../lib -I$(srcdir)/../driver

sbin_PROGRAMS = bdmd

bdmd_SOURCES = \
	bdmServer.c \
	xmalloc.c
bdmd_LDADD = ../lib/libBDM.a
