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

INCLUDES = -I$(srcdir)/../driver -I$(srcdir)/../lib

CFLAGS = @BDM_CFLAGS@

##
## Only way I know of anding in automake.
##

if WIN32
if BDM_REMOTE
LIBS += -l wsock32
endif
endif

bin_PROGRAMS = bdm-chk bdm-cpu32-chk

bdm_chk_SOURCES = \
	chk.c
bdm_chk_LDADD = ../lib/libBDM.a

bdm_cpu32_chk_SOURCES = \
	cpu32-check.c
bdm_cpu32_chk_LDADD = ../lib/libBDM.a
