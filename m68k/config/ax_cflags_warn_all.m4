dnl @synopsis AX_CFLAGS_WARN_ALL [(shellvar)]
dnl
dnl Try to find a compiler option that enables most reasonable warnings.
dnl This macro is directly derived from VL_PROG_CC_WARNINGS which is
dnl split up into two AX_CFLAGS_WARN_ALL and AX_CFLAGS_WARN_ALL_ANSI
dnl
dnl For the GNU CC compiler it will be -Wall (and -ansi -pedantic)
dnl The result is added to the shellvar being CFLAGS by default.
dnl
dnl Currently this macro knows about GCC, Solaris C compiler,
dnl Digital Unix C compiler, C for AIX Compiler, HP-UX C compiler,
dnl IRIX C compiler, NEC SX-5 (Super-UX 10) C compiler, and Cray J90
dnl (Unicos 10.0.0.8) C compiler.
dnl
dnl @version $Id: ax_cflags_warn_all.m4,v 1.1 2004/01/08 20:34:22 codewiz Exp $
dnl @author Guido Draheim <guidod@gmx.de>
dnl
AC_DEFUN([AX_CFLAGS_WARN_ALL], 
[AC_MSG_CHECKING(m4_ifval($1,$1,CFLAGS) for maximum warnings)
AC_CACHE_VAL(ac_cv_cflags_warn_all,
[ac_cv_cflags_warn_all="no, unknown"
 AC_LANG_SAVE
 AC_LANG_C
 ac_save_CFLAGS="$CFLAGS"
for ac_arg dnl
in "-pedantic  % -Wall"       dnl   GCC
   "-xstrconst % -v"          dnl Solaris C 
   "-std1      % -verbose -w0 -warnprotos" dnl Digital Unix 
   "-qlanglvl=ansi % -qsrcmsg -qinfo=all:noppt:noppc:noobs:nocnd" dnl AIX
   "-ansi -ansiE % -fullwarn" dnl IRIX
   "+ESlit     % +w1"         dnl HP-UX C 
   "-Xc        % -pvctl[,]fullmsg" dnl NEC SX-5 (Super-UX 10)
   "-h conform % -h msglevel 2" dnl Cray C (Unicos)
   # 
do CFLAGS="$ac_save_CFLAGS "`echo $ac_arg | sed -e 's,%%.*,,' -e 's,%,,'`
   AC_TRY_COMPILE([],[return 0;],
   [ac_cv_cflags_warn_all=`echo $ac_arg | sed -e 's,.*% *,,'`
   break])
done
 CFLAGS="$ac_save_CFLAGS"
 AC_LANG_RESTORE
])
case ".$ac_cv_cflags_warn_all" in
   .|.no,*) AC_MSG_RESULT($ac_cv_cflags_warn_all) ;;
   *) m4_ifval($1,$1,CFLAGS)="$m4_ifval($1,$1,CFLAGS) dnl
$ac_cv_cflags_warn_all"
      AC_MSG_RESULT($ac_cv_cflags_warn_all) ;;
esac])

dnl the only difference - the LANG selection... and the default FLAGS

AC_DEFUN([AX_CXXFLAGS_WARN_ALL], 
[AC_MSG_CHECKING(m4_ifval($1,$1,CXXFLAGS) for maximum warnings)
AC_CACHE_VAL(ac_cv_cxxflags_warn_all,
[ac_cv_cxxflags_warn_all="no, unknown"
 AC_LANG_SAVE
 AC_LANG_CXX
 ac_save_CXXFLAGS="$CXXFLAGS"
for ac_arg dnl
in "-pedantic  % -Wall"       dnl   GCC
   "-xstrconst % -v"          dnl Solaris C 
   "-std1      % -verbose -w0 -warnprotos" dnl Digital Unix 
   "-qlanglvl=ansi % -qsrcmsg -qinfo=all:noppt:noppc:noobs:nocnd" dnl AIX
   "-ansi -ansiE % -fullwarn" dnl IRIX
   "+ESlit     % +w1"         dnl HP-UX C 
   "-Xc        % -pvctl[,]fullmsg" dnl NEC SX-5 (Super-UX 10)
   "-h conform % -h msglevel 2" dnl Cray C (Unicos)
   # 
do CXXFLAGS="$ac_save_CXXFLAGS "`echo $ac_arg | sed -e 's,%%.*,,' -e 's,%,,'`
   AC_TRY_COMPILE([],[return 0;],
   [ac_cv_cxxflags_warn_all=`echo $ac_arg | sed -e 's,.*% *,,'`
   break])
done
 CXXFLAGS="$ac_save_CXXFLAGS"
 AC_LANG_RESTORE
])
case ".$ac_cv_cxxflags_warn_all" in
   .|.no,*) AC_MSG_RESULT($ac_cv_cxxflags_warn_all) ;;
   *) m4_ifval($1,$1,CXXFLAGS)="$m4_ifval($1,$1,CXXFLAGS) dnl
$ac_cv_cxxflags_warn_all"
      AC_MSG_RESULT($ac_cv_cxxflags_warn_all) ;;
esac])

dnl  implementation tactics:
dnl   the for-argument contains a list of options. The first part of
dnl   these does only exist to detect the compiler - usually it is
dnl   a global option to enable -ansi or -extrawarnings. All other
dnl   compilers will fail about it. That was needed since a lot of
dnl   compilers will give false positives for some option-syntax
dnl   like -Woption or -Xoption as they think of it is a pass-through
dnl   to later compile stages or something. The "%" is used as a
dnl   delimimiter. A non-option comment can be given after "%%" marks.

