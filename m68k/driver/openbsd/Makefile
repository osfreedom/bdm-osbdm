COMPILEHEADERS?=/var/obj/amd/kaben2/var/obj/NetBSD/1.6/src/sys/arch/i386/compile/GENERIC
KERNELHEADERS?=/projects/NetBSD/1.6/src/sys

openbsd-bdm.o:	openbsd-bdm.c ../bdm.c
	cc -O2 -D_KERNEL \
	-I$(COMPILEHEADERS) -I$(KERNELHEADERS) -c openbsd-bdm.c

clean:
	rm openbsd-bdm.o
