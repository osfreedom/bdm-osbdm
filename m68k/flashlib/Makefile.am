## 
## $Id: Makefile.am,v 1.10 2008/09/09 11:48:50 cjohns Exp $
##
## This file is part of a free BDM package
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

AM_CPPFLAGS = -I$(srcdir)/../libelf/lib -I../libelf/lib \
              -I$(srcdir)/../driver -I$(srcdir)/../lib \
	      -DPREFIX='"$(prefix)"'

lib_LIBRARIES = libbdmflash.a

include_HEADERS = \
	bdmfilt.h \
	bdmflash.h \
	flash_filter.h \
	flash29.h \
	flashcfm.h \
	flashintelc3.h \
	flashintelp30.h

libbdmflash_a_SOURCES = \
	elf-utils.c \
	bdmfilt.c \
	bdmflash.c \
	flash_filter.c \
	flash29.c \
	flashcfm.c \
	flashintelc3.c \
	flashintelp30.c

if BUILD_FLASH_PLUGINS
#
# Flash Plug-In Source
#
fpi_source = $(1).c $(1).h

#
# Flash Plug-In Target
#
fpi_target = $(foreach target, $(2), $(1)-$(target).plugin)

#
# Full instruction set range for the Coldfire plus the CPU32.
#
fpi_multilib = \
	cpu32 \
	5206 \
	5206e \
	5307 \
	5407 \
	54455 \
	5475

#
# The list of plugins we have.
#
fpi_flash29 = flash29
fpi_flash29_targets = $(fpi_multilib)
fpi_flash29_source  = $(call fpi_source, flash29)
fpi_flash29_plugins = $(call fpi_target, flash29, $(fpi_flash29_targets))

fpi_flashcfm = flashcfm
fpi_flashcfm_targets = \
	5206 \
	5206e \
	54455
fpi_flashcfm_source  = $(call fpi_source, flashcfm)
fpi_flashcfm_plugins = $(call fpi_target, flashcfm, $(fpi_flashcfm_targets))

fpi_flashintelc3 = flashintelc3
fpi_flashintelc3_targets = $(fpi_multilib)
fpi_flashintelc3_source  = $(call fpi_source, flashintelc3)
fpi_flashintelc3_plugins = $(call fpi_target, flashintelc3, $(fpi_flashintelc3_targets))

fpi_flashintelp30 = flashintelp30
fpi_flashintelp30_targets = $(fpi_multilib)
fpi_flashintelp30_source  = $(call fpi_source, flashintelp30)
fpi_flashintelp30_plugins = $(call fpi_target, flashintelp30, $(fpi_flashintelp30_targets))

fpi_plugins = \
	$(fpi_flash29_plugins) \
	$(fpi_flashcfm_plugins) \
	$(fpi_flashintelc3_plugins) \
	$(fpi_flashintelp30_plugins)

all-local: \
	$(fpi_plugins)

$(fpi_flash29_plugins): $(fpi_flash29_source)
	$(srcdir)/m68k-bdm-compile-plugin @FLASH_PLUGIN_GCC@ $< $@

$(fpi_flashcfm_plugins): $(fpi_flashcfm_source)
	$(srcdir)/m68k-bdm-compile-plugin @FLASH_PLUGIN_GCC@ $< $@

$(fpi_flashintelc3_plugins): $(fpi_flashintelc3_source)
	$(srcdir)/m68k-bdm-compile-plugin @FLASH_PLUGIN_GCC@ $< $@

$(fpi_flashintelp30_plugins): $(fpi_flashintelp30_source)
	$(srcdir)/m68k-bdm-compile-plugin @FLASH_PLUGIN_GCC@ $< $@

install-data-local: \
		$(fpi_plugins)
	test -z "$(prefix)/share/m68k-bdm/plugins" || \
		mkdir -p "$(prefix)/share/m68k-bdm/plugins"; \
	for f in $^; do \
	 install -m 644 $$f $(prefix)/share/m68k-bdm/plugins/$$f; \
	done

clean-local:
	rm -f *.plugin

endif

EXTRA_DIST = README $(fpi_plugins)
