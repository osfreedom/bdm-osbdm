prefix = /usr/local/RTEMS

SCRIPTDIR	= $(prefix)/lib/gdbScripts
BINDIR		= $(prefix)/bin

all:

install:
	-mkdir -p $(SCRIPTDIR)
	for i in *.gdb ; do sed -e "/===PREFIX===/s@@$(prefix)"@ $$i >$(SCRIPTDIR)/$$i ; done
	-mkdir -p $(BINDIR)
	sed -e "/===PREFIX===/s@@$(prefix)"@ m68k-rtems-gdb >$(BINDIR)/m68k-rtems-gdb
	chmod 755 $(BINDIR)/m68k-rtems-gdb

clean:
