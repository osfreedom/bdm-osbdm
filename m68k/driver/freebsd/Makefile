# $Id: Makefile,v 1.1 2003/06/02 15:15:54 codewiz Exp $
#
# James Housley's FreeBSD kernel module makefile.
#

.PATH:	${.CURDIR}
KERN	= /usr/src/sys/kern
KMOD	= bdm
SRCS	= bus_if.h device_if.h isa_if.h freebsd-bdm.c
MAN8	= bdm.8

CFLAGS		+= -DBDM_MODULE -Wall -I..

machine:
	@${ECHO} ${.TARGET} "->" /usr/src/sys/i386/include; \
	ln -s /usr/src/sys/i386/include ${.TARGET}

@:
	@${ECHO} ${.TARGET} "->" /usr/src/sys; \
	ln -s /usr/src/sys ${.TARGET}

afterinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/bdm ${DESTDIR}/usr/bin
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/unbdm ${DESTDIR}/usr/bin

.include <bsd.kmod.mk>
