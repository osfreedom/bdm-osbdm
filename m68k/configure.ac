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

AC_PREREQ(2.59)
AC_INIT([m68k-bdm],[1.4.0],[bdm-devel@lists.sourceforge.net])
AC_CONFIG_AUX_DIR(config)
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE([-Wall -Wno-portability -Wno-unsupported dist-bzip2 dist-zip])

AC_CONFIG_HEADERS(config.h:config.h.in)

AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LEX()

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

AM_PROG_CC_C_O

dnl
dnl Used in the gdbserver.
dnl
AC_HEADER_STDC
AC_CHECK_HEADERS(sys/ioctl.h)
AC_CHECK_HEADERS(sys/file.h)
AC_CHECK_HEADERS(netinet/in.h)
AC_CHECK_HEADERS(sys/socket.h)
AC_CHECK_HEADERS(netdb.h)
AC_CHECK_HEADERS(netinet/tcp.h)
AC_CHECK_HEADERS(sys/ioctl.h)
AC_CHECK_HEADERS(signal.h)
AC_CHECK_HEADERS(fcntl.h)
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(arpa/inet.h)
AC_CHECK_HEADERS(errno.h)
AC_CHECK_HEADERS(termios.h)
AC_CHECK_HEADERS(termio.h)
AC_CHECK_HEADERS(sgtty.h)
AC_CHECK_HEADERS(syslog.h)
AC_CHECK_DECLS([strerror, perror])
AC_CHECK_TYPES(socklen_t, [], [],
[#include <sys/types.h>
#include <sys/socket.h>
])

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

AC_CHECK_FUNCS(ioperm)

case ${host} in
  *freebsd*)
    ac_ioperm=yes
    ;;
  *)
    if test "$ac_cv_func_ioperm" = yes; then
      ac_ioperm=yes
    fi
    ;;
esac

AC_MSG_CHECKING([direct ioperm hardware access])
AC_ARG_ENABLE(ioperm,
 [  --enable-ioperm Turn on direct ioperm hardware access (autodetected)],
 [case "${enableval}" in
    yes)
      AC_MSG_RESULT([yes (enabled by user)])
      bdm_ioperm=true
      ;;
    no)
      AC_MSG_RESULT([no (disabled by user)])
      bdm_ioperm=false
      ;;
    *)
      AC_MSG_ERROR(bad value ${enableval} for --enable-ioperm)
      ;;
   esac],
 [
   if test "$ac_ioperm" = yes; then
     AC_MSG_RESULT([yes (autodetected)])
     bdm_ioperm=true
   else
     AC_MSG_RESULT([no (autodetected)])
     bdm_ioperm=false
   fi
 ])

AM_CONDITIONAL(BDM_IOPERM, test x$bdm_ioperm = xtrue)

AC_MSG_CHECKING([for kernel driver support])
AC_ARG_ENABLE(driver,
 [  --enable-driver Turn on driver support (autodetected)],
 [case "${enableval}" in
    yes)
      AC_MSG_RESULT([yes (enabled by user)])
      bdm_driver=true
      ;;
    no)
      AC_MSG_RESULT([no (disabled by user)])
      bdm_driver=false
      ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-driver) ;;
   esac],
 [
   case "$target_os" in
   darwin*)
     AC_MSG_RESULT([no (autodetected)])
     bdm_driver=false
     ;;
   *)
     AC_MSG_RESULT([yes (autodetected)])
     bdm_driver=true
     ;;
   esac
 ])

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
 [bdm_bdmctrl=true])

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

AS_IF([test x$LEX != xflex], [bdm_bdmmon=false])
AM_CONDITIONAL(BDMMON, test x$bdm_bdmmon = xtrue)

AC_ARG_ENABLE(gdbserver,
 [  --enable-gdbserver Build the GDB Remote Server (enabled)],
 [case "${enableval}" in
    yes) bdm_gdbserver=true ;;
    no)  bdm_gdbserver=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-gdbserver) ;;
   esac],
 [bdm_gdbserver=true])

AM_CONDITIONAL(GDBSERVER, test x$bdm_gdbserver = xtrue)

AC_ARG_ENABLE(tblcf,
 [  --enable-tblcf Build the Turbo BDM Light Coldfire USB driver (enabled)],
 [case "${enableval}" in
    yes) bdm_tblcf=true ;;
    no)  bdm_tblcf=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-tblcf) ;;
   esac],
 [bdm_tblcf=true])

case ${host} in
  *mingw*)
    ac_libusb_include_dir_part=include
    ac_libusb_lib_dir=lib/gcc
    ;;
  *)
    ac_libusb_include_dir_part=include
    ac_libusb_lib_dir=lib
    ;;
esac

AC_ARG_WITH(libusb-dir,
 [  --with-libusb-dir=DIR  path for libusb package ($ac_libusb_include_dir_part/usb.h, $ac_libusb_lib_dir/libusb.a)],
 [ ac_libusb_include_dir=$withval/$ac_libusb_include_dir_part
   ac_libusb_lib_dir=$withval/$ac_libusb_lib_dir ],
 [ ac_libusb_include_dir=NONE
   ac_libusb_lib_dir=NONE ]
) 

AS_IF([test x$bdm_tblcf = xtrue],
      [AS_IF([test "$ac_libusb_include_dir" != "NONE"],
             [ac_tmp_ldflags=${LDFLAGS}
              LDFLAGS="$LDFLAGS -L $ac_libusb_lib_dir"])
       AC_CHECK_LIB(usb, usb_init, ,
                    [AC_MSG_ERROR([Could not find libusb. Try --with-libusb-dir.])])
       AS_IF([test "$ac_libusb_include_dir" != "NONE"],
             [LDFLAGS=$ac_tmp_ldflags])])

AM_CONDITIONAL(TBLCF_USB, test x$bdm_tblcf = xtrue)

AM_CONDITIONAL(LIBUSB_PATH, test $ac_libusb_include_dir != NONE)
AC_SUBST(LIBUSB_INCLUDE_DIR, "${ac_libusb_include_dir}")
AC_SUBST(LIBUSB_LIB_DIR, "${ac_libusb_lib_dir}")

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
AS_IF([test x$bdm_tblcf = xtrue], bdm_subdirs="${bdm_subdirs} tblcf")
AS_IF([test x$bdm_server = xtrue],   bdm_subdirs="${bdm_subdirs} server")
AS_IF([test x$bdm_flashlib = xtrue], bdm_subdirs="${bdm_subdirs} flashlib")
AS_IF([test x$bdm_bdmflash = xtrue -o x$bdm_bdmmon = xtrue], 
                    bdm_subdirs="${bdm_subdirs} bdmabstraction")
AS_IF([test x$bdm_gdbserver = xtrue], bdm_subdirs="${bdm_subdirs} gdbserver")
bdm_subdirs="libelf ${bdm_subdirs} test utils"
AC_SUBST(BDM_SUBDIRS, ${bdm_subdirs})

dnl
dnl Configure libelf
dnl
AC_CONFIG_SUBDIRS(libelf)

dnl
dnl Check if a m68k GCC compiler is present and can be used.
dnl

AC_PATH_PROGS([flash_plugin_cc],
              [m68k-elf-gcc m68k-rtems4.9-gcc m68k-rtems4.8-gcc],
              [none],
              [/usr/local/bin:/opt/rtems-4.9/bin:/opt/rtems-4.8/bin])
AM_CONDITIONAL(BUILD_FLASH_PLUGINS, test x${flash_plugin_cc} != xnone)
AC_SUBST(FLASH_PLUGIN_GCC, ${flash_plugin_cc})

dnl
dnl Output the Makefiles and config.h.
dnl

AC_CONFIG_FILES(Makefile \
                flashlib/Makefile \
                lib/Makefile \
                tblcf/Makefile \
                server/Makefile \
                gdbserver/Makefile \
                test/Makefile \
                bdmabstraction/Makefile \
                utils/Makefile)

AC_OUTPUT
