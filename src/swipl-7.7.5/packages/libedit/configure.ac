dnl Process this file with autoconf to produce a configure script.

AC_INIT(install-sh)
AC_PREREQ([2.50])
AC_CONFIG_HEADER(config.h)

AC_SUBST(CIFLAGS)
AC_SUBST(TARGETS)
AC_SUBST(PLTARGETS)

m4_include([../ac_swi_c.m4])

AC_CHECK_FUNCS(poll)
AC_CHECK_HEADERS(unistd.h uxnt.h sys/ioctl.h poll.h)

dnl MacOS hacks.  There is both libedit.dylib in /usr/lib and in the
dnl Macport or Homebrew.  These are typically not compatible and it
dnl seems the MacOS version is also not binary cmpatible between OS
dnl versions, so we use the Macport/Homebrew version

case "$PLARCH" in
    *darwin*)
	if test -z "$LIBEDITPREFIX"; then
          if test -f /opt/local/lib/libedit.$SO; then
            LIBEDITPREFIX=/opt/local
          else
            LIBEDITPREFIX=/usr/local
          fi
        fi
	;;
esac

dnl On some platforms (e.g., OpenBSD), ncurses needs to be linked
dnl seperately.
AC_CHECK_LIB(ncursesw, main)
if test ! "$ac_cv_lib_ncursesw_main" = "yes"; then
  AC_CHECK_LIB(ncurses, main)
fi

if test ! -z "$LIBEDITPREFIX"; then
  AC_MSG_CHECKING(Whether setting for LIBEDITPREFIX is ok)
  if test -f "$LIBEDITPREFIX/include/histedit.h" -a \
          -f "$LIBEDITPREFIX/lib/libedit.$SO"; then
    AC_MSG_RESULT(Using $LIBEDITPREFIX)
    LIBS="$LIBS $LIBEDITPREFIX/lib/libedit.$SO"
    CFLAGS="-I/$LIBEDITPREFIX/include $CFLAGS"
    AC_CHECK_FUNCS(el_cursor el_wset)
    have_libedit=yes
  else
    AC_MSG_RESULT(no)
    LIBEDITPREFIX=
  fi
fi

if test -z "$LIBEDITPREFIX"; then
  AC_CHECK_LIB(edit, el_init, have_libedit=yes)

  if test "$have_libedit" = yes; then
    AC_CHECK_HEADERS(histedit.h)
    if test "$ac_cv_header_histedit_h" = yes; then
      LIBS="-ledit $LIBS"
      AC_CHECK_FUNCS(el_cursor el_wset)
    else
      have_libedit=no
    fi
  fi
fi

if test "$have_libedit" = yes; then
  TARGETS="libedit4pl.$SO"
  PLTARGETS="editline.pl"
fi

AC_OUTPUT(Makefile)
