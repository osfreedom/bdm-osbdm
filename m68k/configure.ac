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
AC_INIT([bdm], [2.0], [bdm-devel@lists.sourceforge.net])
AC_CONFIG_AUX_DIR(config)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

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
dnl Defaults.
dnl

bdm_subdirs="lib test utils"
bdm_win32=false

AC_SUBST(INCLUDES, ${INCLUDES})

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
 [bdm_bdmctrl=false])

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

dnl
dnl If on a Win32 target such as MinGW do not build
dnl the server.
dnl

case ${host} in
  *mingw*) 
    bdm_win32=true
    bdm_server=false
    ;;
esac

AM_CONDITIONAL(WIN32, test x$bdm_win32 = xtrue)

AS_IF([test x$bdm_server = xtrue],   bdm_subdirs="${bdm_subdirs} server")
AS_IF([test x$bdm_flashlib = xtrue], bdm_subdirs="${bdm_subdirs} flashlib")
AC_SUBST(BDM_SUBDIRS, ${bdm_subdirs})

dnl
dnl Output the Makefiles.
dnl

AC_CONFIG_FILES(Makefile \
                flashlib/Makefile \
                lib/Makefile \
                server/Makefile \
                test/Makefile \
                utils/Makefile)
AC_OUTPUT
