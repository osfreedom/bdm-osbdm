## 
## $Id: Makefile.am,v 1.3 2003/11/07 23:30:28 joewolf Exp $
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

##
## Only way I know of anding in automake.
##

if WIN32
if BDM_REMOTE
LIBS += -l wsock32
endif
endif

bin_PROGRAMS = bdmreset

if BDMCTRL
bin_PROGRAMS += bdmctrl
endif

bdmctrl_SOURCES = \
	bdmctrl.c
#bdmctrl_CFLAGS = \
#	@CFLAGS@ \
#	-I${prefix}/${host_alias}/${target_alias}/include \
#	-I${prefix}/include
bdmctrl_LDFLAGS = -static
bdmctrl_LDADD = ../lib/libBDM.a	-lbfd -liberty \
	-L${prefix}/${host_alias}/${target_alias}/lib


bdmreset_SOURCES = \
	bdmreset.c
bdmreset_LDADD = ../lib/libBDM.a
