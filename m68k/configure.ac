dnl Process this file with autoconf to produce a configure script.
dnl
dnl Use the config/bootstrap file to generate the configure.
dnl
dnl This file is part of a free BDM package
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.

AC_PREREQ(2.57)
AC_INIT([m68k-bdm],[1.3.0],[bdm-devel@lists.sourceforge.net])
AC_CONFIG_AUX_DIR(config)
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE([-Wall dist-bzip2 dist-zip])

AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

dnl For backwards compatibility with automake 1.4 (AC_PROG_LEX should suffice)
AC_DECL_YYTEXT

dnl bernie: we should fix all warnings before enabling this
dnl AX_CFLAGS_WARN_ALL

CFLAGS=${CFLAGS-"-Wall -O2 -g"}
AC_CHECK_TOOL(CC, gcc, gcc)
AC_SUBST(CC)
AC_SUBST(CFLAGS)

AC_CHECK_TOOL(AR, ar, ar)
AC_SUBST(AR)
AC_CHECK_TOOL(AS, as, as)
AC_SUBST(AS)
AC_CHECK_TOOL(RANLIB, ranlib, ranlib)
AC_SUBST(RANLIB)
AC_CHECK_TOOL(LD, ld, ld)
AC_SUBST(LD)


dnl
dnl libbfd and libiberty are required to build bdmctrl
dnl

AC_ARG_WITH(libbfd,
 [  --with-libbfd=FILE     path to libbfd.a library to use],
 [ac_libbfd=$withval],
 [ac_libbfd=NONE]
)

LIBBFD="$ac_libbfd"
AC_SUBST(LIBBFD)

AC_ARG_WITH(libiberty,
 [  --with-libiberty=FILE  path to libiberty.a library to use ],
 [ac_libiberty=$withval],
 [ac_libiberty=NONE]
)

LIBIBERTY="$ac_libiberty"
AC_SUBST(LIBIBERTY)

AC_ARG_WITH(bfd-include-dir,
 [  --with-bfd-include-dir=DIR  include path for correct bfd.h ],
 [ ac_bfd_include_dir=$withval ],
 [ ac_bfd_include_dir=NONE ]
)

BFD_INCLUDE="-I$ac_bfd_include_dir"
AC_SUBST(BFD_INCLUDE)

if test "$ac_libbfd" = "NONE" -o "$ac_libiberty" = "NONE" -o "$ac_bfd_include_dir" = "NONE" ; then
	echo "WARNING: To build bdmctrl, you need to specify the location of target-specific"
	echo "WARNING: libbfd.a and libiberty.a libraries from the binutils org GDB package."
	echo "WARNING: "
	echo "WARNING: Please re-run configure with the following options:"
	echo "WARNING:   ./configure --with-bfd-include-dir=<dir> --with-libbfd=<libbfd.a> --with-libiberty=<libiberty.a>"
	ac_bdmctrl_auto=false
else
	ac_bdmctrl_auto=true
fi

dnl
dnl Handle our configuration options.
dnl

AC_ARG_ENABLE(debug,
 [  --enable-debug Turn on debug information (enabled)],
 [case "${enableval}" in
    yes) debug=true ;;
    no)  debug=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
   esac],
 [debug=true])

AC_ARG_ENABLE(remote,
 [  --enable-remote Turn on the remote protocol (enabled)],
 [case "${enableval}" in
    yes) bdm_remote=true ;;
    no)  bdm_remote=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-remote) ;;
   esac],
 [bdm_remote=true])

AM_CONDITIONAL(BDM_REMOTE, test x$bdm_remote = xtrue)

AC_ARG_ENABLE(ioperm,
 [  --enable-ioperm Turn on direct ioperm hardware access (enabled)],
 [case "${enableval}" in
    yes) bdm_ioperm=true ;;
    no)  bdm_ioperm=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-ioperm) ;;
   esac],
 [bdm_ioperm=true])

AM_CONDITIONAL(BDM_IOPERM, test x$bdm_ioperm = xtrue)

AC_ARG_ENABLE(driver,
 [  --enable-driver Turn on driver support (enabled)],
 [case "${enableval}" in
    yes) bdm_driver=true ;;
    no)  bdm_driver=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-driver) ;;
   esac],
 [bdm_driver=true])

AM_CONDITIONAL(BDM_DRIVER, test x$bdm_driver = xtrue)

AC_ARG_ENABLE(server,
 [  --enable-server Turn on server support (enabled)],
 [case "${enableval}" in
    yes) bdm_server=true ;;
    no)  bdm_server=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-server) ;;
   esac],
 [bdm_server=true])

AC_ARG_ENABLE(bdmctrl,
 [  --enable-bdmctrl Turn on bdmctrl support (disabled)],
 [case "${enableval}" in
    yes) bdm_bdmctrl=true ;;
    no)  bdm_bdmctrl=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-bdmctrl) ;;
   esac],
 [bdm_bdmctrl=$ac_bdmctrl_auto])

AM_CONDITIONAL(BDMCTRL, test x$bdm_bdmctrl = xtrue)

AC_ARG_ENABLE(flashlib,
 [  --enable-flashlib Turn on flashlib support (enabled)],
 [case "${enableval}" in
    yes) bdm_flashlib=true ;;
    no)  bdm_flashlib=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-flashlib) ;;
   esac],
 [bdm_flashlib=true])

AM_CONDITIONAL(BDMFLASHLIB, test x$bdm_flashlib = xtrue)

AC_ARG_ENABLE(bdmflash,
 [  --enable-bdmflash Build the bdmflash utility (enabled)],
 [case "${enableval}" in
    yes) bdm_bdmflash=true ;;
    no)  bdm_bdmflash=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-bdmflash) ;;
   esac],
 [bdm_bdmflash=true])

AC_ARG_ENABLE(bdmmon,
 [  --enable-bdmmon Build the bdmmon utility (enabled)],
 [case "${enableval}" in
    yes) bdm_bdmmon=true ;;
    no)  bdm_bdmmon=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-bdmmon) ;;
   esac],
 [bdm_bdmmon=true])

AM_CONDITIONAL(BDMFLASH, test x$bdm_bdmflash = xtrue)
AM_CONDITIONAL(BDMMON, test x$bdm_bdmmon = xtrue)

dnl
dnl If on a Win32 target such as MinGW do not build
dnl the server.
dnl

case ${host} in
  *mingw*) 
    bdm_win32=true
    bdm_server=false
    ;;
  *)
    bdm_win32=false
    ;;
esac

AM_CONDITIONAL(WIN32, test x$bdm_win32 = xtrue)

dnl
dnl Collect list of subdirectories to build
dnl
bdm_subdirs="lib"
AS_IF([test x$bdm_server = xtrue],   bdm_subdirs="${bdm_subdirs} server")
AS_IF([test x$bdm_flashlib = xtrue], bdm_subdirs="${bdm_subdirs} flashlib")
AS_IF([test x$bdm_bdmflash = xtrue -o x$bdm_bdmmon = xtrue], bdm_subdirs="${bdm_subdirs} bdmabstraction")
bdm_subdirs="${bdm_subdirs} test utils"
AC_SUBST(BDM_SUBDIRS, ${bdm_subdirs})

dnl
dnl Output the Makefiles.
dnl

AC_CONFIG_FILES(Makefile \
                flashlib/Makefile \
                lib/Makefile \
                server/Makefile \
                test/Makefile \
                bdmabstraction/Makefile \
                utils/Makefile)
AC_OUTPUT
