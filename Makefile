#
# $Id: Makefile,v 1.3 2003/10/06 00:01:08 codewiz Exp $
#
# Build distribution archives - crude but effective
# Author: Bernardo Innocenti <bernie@develer.com>
#
# $Log: Makefile,v $
# Revision 1.3  2003/10/06 00:01:08  codewiz
# Fix source distribution for autoconfiscated m68k-bdm.
#
# Revision 1.2  2003/07/04 22:21:50  codewiz
# Add missing -o to find command line.
#
# Revision 1.1  2003/07/01 13:45:20  codewiz
# Build distribution archives
#

MKDIR	= mkdir -p
RM_R	= rm -rf
CP	= cp -a
GZIP	= gzip
BZIP2	= bzip2

M68K_DISTDATE	= 20030921
M68K_DISTDIR	= m68k-bdm-$(M68K_DISTDATE)
M68K_TARGZ	= m68k-bdm-$(M68K_DISTDATE).tar.gz
M68K_TARBZ2	= m68k-bdm-$(M68K_DISTDATE).tar.bz2
M683XX_DISTDATE	= 20030701
M683XX_DISTDIR	= m683xx-bdm-$(M683XX_DISTDATE)
M683XX_TARGZ	= m683xx-bdm-$(M683XX_DISTDATE).tar.gz
M683XX_TARBZ2	= m683xx-bdm-$(M683XX_DISTDATE).tar.bz2

.PHONY: all
all:
	@echo "No automated build yet"

clean:
	$(RM_R) $(M68K_DISTDIR) $(M68K_TARGZ) $(M68K_TARBZ2)
	$(RM_R) $(M683XX_DISTDIR) $(M683XX_TARGZ) $(M683XX_TARBZ2)

.PHONY: dist
dist: dist-m68k dist-m683xx

.PHONY: dist-m68k
dist-m68k:
	$(RM_R) "$(M68K_DISTDIR)"
	$(CP) m68k "$(M68K_DISTDIR)"
	find "$(M68K_DISTDIR)" \
			-name 'CVS' -o \
			-name '.cvsignore' -o \
			-name '*.o' -o \
			-name '*.a' -o \
			-name '*.ko*' -o \
			-name '.*.cmd' -o \
			-name '*.mod.c' -o \
			-name '.libs' -o \
			-name '.deps' \
		| xargs $(RM_R)
	tar -c -v $(M68K_DISTDIR) | $(GZIP) >"$(M68K_TARGZ)"
	tar -c -v $(M68K_DISTDIR) | $(BZIP2) >"$(M68K_TARBZ2)"

.PHONY: dist-m683xx
dist-m683xx:
	$(RM_R) "$(M683XX_DISTDIR)"
	$(CP) m683xx "$(M683XX_DISTDIR)"
	find "$(M683XX_DISTDIR)" \
			-name 'CVS' -o \
			-name '.cvsignore' -o \
			-name '*.o' -o \
			-name '*.a' -o \
			-name '*.ko*' -o \
			-name '.*.cmd' -o \
			-name '*.mod.c' -o \
			-name '.libs' -o \
			-name '.deps' \
		| xargs $(RM_R)
	tar -c -v $(M683XX_DISTDIR) | $(GZIP) >"$(M683XX_TARGZ)"
	tar -c -v $(M683XX_DISTDIR) | $(BZIP2) >"$(M683XX_TARBZ2)"

