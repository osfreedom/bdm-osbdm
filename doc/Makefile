#
# $Id: Makefile,v 1.1 2003/06/02 15:13:33 codewiz Exp $
#

all: gdb-bdm.info gdb-bdm.html

gdb-bdm.info: gdb-bdm.texinfo
	makeinfo gdb-bdm.texinfo

gdb-bdm.html: gdb-bdm.texinfo gdb-bdm.init
	texi2html -init_file gdb-bdm.init gdb-bdm.texinfo
