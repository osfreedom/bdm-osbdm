# @#[Copyright (c) 2000, Brett Wuth.]
# @#[License:
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.   ]

# File:		Makefile
# Purpose:	
# Author:	Brett Wuth
# Created:	
#
# Initials:
#	BCW - Brett Wuth
#		@#[ContactWuth:
#		Phone: +1 403 627-2460
#		E-mail: support@castrov.cuug.ab.ca, wuth@acm.org]
#
# HISTORY:
# $Log: Makefile,v $
# Revision 1.1  2003/06/03 15:42:04  codewiz
# Import userland tools from bdm-fiedler
#
# Revision 1.5  2000/07/14 18:38:55  wuth
# Flash error status; Fix sector erase support; Command line sector erase
#
# Revision 1.4  2000/04/20 04:56:23  wuth
# GPL.  Abstract flash interface.
#
# Revision 1.3  2000/03/28 20:24:42  wuth
# Break out flash code into separate executable.  Make run under Chris Johns BDM driver.
#
# @#[BasedOnTemplate: Makefile version 3]

#    Every Makefile should contain this line:
# 
SHELL = /bin/sh
# 
# to avoid trouble on systems where the `SHELL' variable might be
# inherited from the environment.  (This is never a problem with GNU
# `make'.)

Platform=$(shell uname)

AR = ar
ifeq ($(Platform),AIX)
CC = xlC
else
CC = cc
endif
LEX = lex

#   Every Makefile should define the variable `INSTALL', which is the
# basic command for installing a file into the system.
INSTALL = cp
#    Every Makefile should also define the variables `INSTALL_PROGRAM'
# and `INSTALL_DATA'.  (The default for each of these should be
# `$(INSTALL)'.)
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL)


prefix = $(InstallDir)
#      A prefix used in constructing the default values of the variables
#      listed below.  The default value of `prefix' should be
#      `/usr/local'.  When building the complete GNU system, the prefix
#      will be empty and `/usr' will be a symbolic link to `/'.
exec_prefix = $(prefix)
#      A prefix used in constructing the default values of some of the
#      variables listed below.  The default value of `exec_prefix' should
#      be `$(prefix)'.  (If you are using Autoconf, write it as
#      `@exec_prefix@'.)
# 
#      Generally, `$(exec_prefix)' is used for directories that contain
#      machine-specific files (such as executables and subroutine
#      libraries), while `$(prefix)' is used directly for other
#      directories.
libdir = $(exec_prefix)/lib
#      The directory for object files and libraries of object code.  Do
#      not install executables here, they probably ought to go in
#      `$(libexecdir)' instead.  The value of `libdir' should normally be
#      `/usr/local/lib', but write it as `$(exec_prefix)/lib'.
includedir = $(prefix)/include
#      The directory for installing header files to be included by user
#      programs with the C `#include' preprocessor directive.  This
#      should normally be `/usr/local/include', but write it as
#      `$(prefix)/include'.
# 
#      Most compilers other than GCC do not look for header files in
#      `/usr/local/include'.  So installing the header files this way is
#      only useful with GCC.  Sometimes this is not a problem because some
#      libraries are only really intended to work with GCC.  But some
#      libraries are intended to work with other compilers.  They should
#      install their header files in two places, one specified by
#      `includedir' and one specified by `oldincludedir'.

# The root directories of the environment (include, lib, bin)
# that this package uses to compile
EnvironmentDirs = ..

# The directories where we will install our parts
InstallLibDir = $(libdir)
InstallIncludeDir = $(includedir)
InstallBinDir = $(exec_prefix)/bin

# options to give to the preprocessor
CPPFLAGS = $(patsubst %,-I%,$(IncludeDirs)) \
	   $(patsubst %,-D%,$(Defines) $(DefinesDebug))
IncludeDirs = . $(SrcDir) $(SrcIncludeDir) \
	      $(patsubst %,%/include,$(EnvironmentDirs))

#    If there are C compiler options that *must* be used for proper
# compilation of certain files, do not include them in `CFLAGS'.  Users
# expect to be able to specify `CFLAGS' freely themselves.  Instead,
# arrange to pass the necessary options to the C compiler independently
# of `CFLAGS', by writing them explicitly in the compilation commands or
# by defining an implicit rule, like this:
CFLAGS = $(CDEBUG) -O
ALL_CFLAGS =  $(CPPFLAGS) $(CFLAGS)
#    Put `CFLAGS' last in the compilation command, after other variables
# containing compiler options, so the user can use `CFLAGS' to override
# the others.
LDFLAGS = $(LDDEBUG)
ALL_LDFLAGS = $(patsubst %,-L%,$(LibDirs)) $(LDFLAGS)
LibDirs = . $(SrcDir) $(OutputDir) \
	  $(patsubst %,%/lib,$(EnvironmentDirs))

ifeq ($(Platform),Linux)
CompilerCPlusPlusLib=stdc++
else
CompilerCPlusPlusLib=
endif

vpath %.a   $(OutputDir)
vpath %.o   $(OutputDir)
vpath %.h   $(SrcDir) $(SrcIncludeDir)
vpath %.c   $(SrcDir)
vpath %.cpp $(SrcDir)
vpath %.d   $(OutputDir)
vpath %.lex $(SrcDir)


#    Different `make' programs have incompatible suffix lists and
# implicit rules, and this sometimes creates confusion or misbehavior.  So
# it is a good idea to set the suffix list explicitly using only the
# suffixes you need in the particular Makefile, like this:
# 
.SUFFIXES:
.SUFFIXES: .cpp .c .d .o
# 
# The first line clears out the suffix list, the second introduces all
# suffixes which may be subject to implicit rules in this Makefile.

default : all
.PHONY : default

##########################################################################
# Customize section

# The root directory of the package source, relative to the directory
# make is run from
PackageDir = .

# Where the source files are
SrcDir = $(PackageDir)

# Where the source include files are
SrcIncludeDir = $(PackageDir)

# Where to write the compiled files
OutputDir = $(PackageDir)/output

# The root directory of where to install our parts (lib include bin)
InstallDir = ..

# Defines we require:
Defines = 

# The libraries we depend on:
DependLibs = BDMFiedler BDM
#DependLibs = BDMFiedlerdbg BDM
# perhaps: wuth $(CompilerCPlusPlusLib)

# The modules we'll compile:
LibModules = 
DriverModules = bdm_mon BDMFlash

# Relationship between executables and modules containing main:
$(OutputDir)/bdm_mon : $(OutputDir)/bdm_mon.o
$(OutputDir)/BDMFlash : $(OutputDir)/BDMFlash.o

# The executables we're creating:
InstallBins = bdm_mon BDMFlash
TestBins = 

# The library we're creating:
InstallLib = 

# The headers we're installing:
# Special value "recurse" means all recursively
InstallIncludes = 

##########################################################################

# debugging options

# values: debug -- create a debugging version of library
#                  modules and executables compiled with -g and without NDEBUG
#                  library has dbg suffix
# Default is to build debug version for internal use:
# Overridden in command line by recursive make for building
# install variants.
DebugBuildMode = debug

ifneq "$(findstring debug,$(DebugBuildMode))" ""
CDEBUG = -g
LDDEBUG = -g
DefinesDebug = 
InstallLibDebug = dbg
else
CDEBUG =
LDDEBUG =
DefinesDebug = NDEBUG
InstallLibDebug = 
endif

Bins = $(InstallBins) $(TestBins)

OutputDirBins        = $(patsubst %,$(OutputDir)/%,$(Bins))
OutputDirInstallBins = $(patsubst %,$(OutputDir)/%,$(InstallBins))

Modules = $(LibModules) $(DriverModules)

all : $(OutputDirBins) $(LocalLib)
.PHONY : all

OutputDirAvail = $(OutputDir)/DirAvailable
OutputDirMade  = $(OutputDir)/RemoveDirWithOutput

$(OutputDirAvail) :
	if [ ! -d $(OutputDir) ]; then mkdir -p $(OutputDir); touch $(OutputDirMade); fi
	touch $@


#    Here is the pattern rule to generate a file of dependencies (i.e., a
# makefile) called `NAME.d' from a C source file called `NAME.c':
# 
# Cannot depend on any phony targets or will reread endlessly
$(OutputDir)/%.d: $(SrcDir)/%.c $(OutputDirAvail)
ifeq ($(Platform),Linux)
	$(SHELL) -ec '$(CC) -M $(CPPFLAGS) $< \
	    | sed "s@\(.*\)\.o:*@$(OutputDir)/\1.o $(OutputDir)/\1.d :@g" > $@'
endif
ifeq ($(Platform),AIX)
	$(CC) -c -M $(CPPFLAGS) -o $(OutputDir)/$*.o $<
	sed "s@\(.*\)\.o:*@$(OutputDir)/\1.o $(OutputDir)/\1.d :@g" <$(OutputDir)/$*.u > $@
endif

$(OutputDir)/%.d: $(SrcDir)/%.cpp $(OutputDirAvail)
ifeq ($(Platform),Linux)
	$(SHELL) -ec '$(CC) -M $(CPPFLAGS) $< \
	    | sed "s@\(.*\)\.o:*@$(OutputDir)/\1.o $(OutputDir)/\1.d :@g" > $@'
endif
ifeq ($(Platform),AIX)
	$(CC) -c -M $(CPPFLAGS) -o $(OutputDir)/$*.o $<
	sed "s@\(.*\)\.o:*@$(OutputDir)/\1.o $(OutputDir)/\1.d :@g" <$(OutputDir)/$*.u > $@
endif
#    The purpose of the `sed' command is to translate (for example):
# 
#      main.o: main.c defs.h
# 
# into:
# 
#      output/main.o output/main.d: main.c defs.h

ifneq "$(strip $(Modules))" ""
-include $(Modules:%=$(OutputDir)/%.d)
endif

Objects = $(Modules:%=$(OutputDir)/%.o)
LibObjects = $(LibModules:%=$(OutputDir)/%.o)

LocalLib = $(OutputDir)/liblocal.a

$(LocalLib) : $(LibObjects) $(OutputDirAvail)
	$(AR) cr $@ $(filter %.o,$?)

$(OutputDir)/%.o: $(SrcDir)/%.c $(OutputDirAvail)
	$(CC) -c $(ALL_CFLAGS) $< -o $@

$(OutputDir)/%.o: $(SrcDir)/%.cpp $(OutputDirAvail)
	$(CC) -c $(ALL_CFLAGS) $< -o $@

$(OutputDir)/%.c: $(SrcDir)/%.lex $(OutputDirAvail)
	$(LEX) -o$@ $<


$(OutputDirBins) : $(LocalLib)
	$(CC) $(ALL_LDFLAGS) $(filter %.o,$^) $(filter %.a,$^) \
	      $(patsubst %,-l%,$(DependLibs)) -o $@

.PHONY : install
install : 
	-rm $(OutputDirAvail) 
	$(MAKE) InstallVariant DebugBuildMode=debug
	rm $(OutputDirAvail) 
	$(MAKE) InstallVariant DebugBuildMode=
	rm $(OutputDirAvail) 
# rm $(OutputDirAvail) : force rebuild on all objects.
# Non-debugging version overwrites debugging version on any files
# that are not distinguished between the two.

.PHONY : InstallVariant
InstallVariant : InstallBins InstallLib InstallIncludes

.PHONY : InstallBins
InstallBins : $(OutputDirInstallBins)
ifneq "$(strip $(OutputDirInstallBins))" ""
	  $(INSTALL_PROGRAM) $^ $(InstallBinDir)
endif

.Phony : InstallLib
InstallLib : $(LocalLib)
ifneq "$(strip $(InstallLib))" ""
	$(INSTALL_DATA) $^ $(InstallLibDir)/lib$(InstallLib)$(InstallLibDebug).a
endif


ifeq "$(strip $(SrcIncludeInstallExpanded))" ""
# avoid match on $(SrcIncludeDir)/.
SrcIncludeInstallExpanded = 
else
SrcIncludeInstallExpanded = $(wildcard $(SrcIncludeDir)/$(InstallIncludes))
endif

.PHONY : InstallIncludes
ifeq "$(strip $(InstallIncludes))" "recurse"
# everything recursively in the SrcIncludeDir
InstallIncludes :
	$(INSTALL_DATA) --recursive $(SrcIncludeDir)/* $(InstallIncludeDir)
else
# just the files mentioned
InstallIncludes : $(SrcIncludeInstallExpanded)
ifneq "$(strip $(SrcIncludeInstallExpanded))" ""
	$(INSTALL_DATA) $^ $(InstallIncludeDir)
endif
endif #!recurse

.PHONY : clean
clean :
	-rm $(OutputDir)/*.{a,d,o,map}
	-rm $(OutputDirBins) $(OutputDirAvail)
	if [ -f $(OutputDirMade) ]; then rm -r $(OutputDir); fi
	find $(PackageDir) -type f "(" -name "*~" -o -name ".*~" ")" -print -exec rm {} ";"
	find $(PackageDir) -type f -name "core" -print -exec rm {} ";"
	find $(PackageDir) -type p -print -exec rm {} ";"

# /*** EOF ***/
