dnl Process this file with autoconf to produce a configure script.

AC_INIT(install-sh)
AC_PREREQ([2.50])
AC_CONFIG_HEADER(config.h)

AC_SUBST(CIFLAGS)
AC_SUBST(TARGETS)
AC_SUBST(PLTARGETS)

m4_include([../ac_swi_c.m4])

readline_ok=no

dnl Hacks for MacOS handling of libreadline.  We want to prefer the MacOS X
dnl libraries, but we need to get libreadline from Macports, as the BSD
dnl version is severely limited.  Note that Macports installs in
dnl /opt/local and Brew in /usr/local, so we try both.

case "$PLARCH" in
  *darwin*)
	if test -z "$RLPREFIX"; then
	  if test -f /opt/local/lib/libreadline.$SO; then
	    RLPREFIX=/opt/local
	  else
	    RLPREFIX=/usr/local
	  fi
        fi
	;;
esac

AC_CHECK_FUNCS(poll)
AC_CHECK_HEADERS(poll.h)

AC_CHECK_LIB(ncursesw, main)
if test ! "$ac_cv_lib_ncursesw_main" = "yes"; then
  AC_CHECK_LIB(ncurses, main)
fi

AC_DEFUN([pl_RL_CHECK_VAR],
[AS_VAR_PUSHDEF([ac_var], [ac_cv_libreadline_var_$1])dnl
AC_CACHE_CHECK([for $1], [ac_var],
[AC_LINK_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT
#include <readline/readline.h>], [$1 = 0;])],
       [AS_VAR_SET([ac_var], [yes])],
       [AS_VAR_SET([ac_var], [no])])])
 AH_TEMPLATE(AS_TR_CPP([HAVE_$1]),
        [Define to 1 if libreadline has the `$1' variable.])
 AS_IF([test AS_VAR_GET([ac_var]) = yes],
       [AC_DEFINE(AS_TR_CPP([HAVE_$1]))])
 AS_VAR_POPDEF([ac_var])dnl
])

if test ! -z "$RLPREFIX"; then
  AC_MSG_CHECKING(Whether setting for RLPREFIX ($RLPREFIX) ($SO) is ok)
  if test -f $RLPREFIX/include/readline/readline.h -a \
	  -f $RLPREFIX/lib/libreadline.$SO; then
    LIBS="$LIBS $RLPREFIX/lib/libreadline.$SO"
    AC_MSG_RESULT(Using $RLPREFIX)
    AC_DEFINE(HAVE_LIBREADLINE, 1,
	      [Define to 1 if you have libreadline])
    AC_DEFINE(HAVE_READLINE_READLINE_H, 1,
	      [Define to 1 if you have <readline/readline.h>])
    if test -f $RLPREFIX/include/readline/history.h; then
      AC_DEFINE(HAVE_READLINE_HISTORY_H, 1,
		[Define to 1 if you have <readline/history.h>])
    fi
    CFLAGS="-I$RLPREFIX/include $CFLAGS"
    CIFLAGS="-I$RLPREFIX/include"
    AC_CHECK_FUNCS(rl_completion_matches rl_insert_close)
    AC_CHECK_FUNCS(rl_set_prompt rl_clear_pending_input)
    AC_CHECK_FUNCS(rl_event_hook rl_cleanup_after_signal)
    AC_CHECK_FUNCS(rl_filename_completion_function)
    AC_CHECK_FUNCS(rl_set_keyboard_input_timeout)
    AC_CHECK_DECLS([rl_done])
    readline_ok=yes
  else
    AC_MSG_RESULT(no)
    RLPREFIX=
  fi
fi

if test -z "$RLPREFIX"; then
  AC_CHECK_LIB(readline, main,
	       have_libreadline=yes)

  if test "$have_libreadline" = yes; then
    AC_CHECK_HEADERS(readline/readline.h)
    if test "$ac_cv_header_readline_readline_h" = no; then
	unset ac_cv_header_readline_readline_h
	CFLAGS="$CFLAGS -I/usr/local/include"
	AC_CHECK_HEADERS(readline/readline.h)
	if test "$ac_cv_header_readline_readline_h" = yes; then
	    CIFLAGS="$CIFLAGS -I/usr/local/include"
	fi
    fi

    if test "$ac_cv_header_readline_readline_h" = yes; then
      AC_CHECK_HEADERS(readline/history.h)
      if test "$ac_cv_header_readline_history_h" = yes; then
	readline_ok=yes
	LIBS="-lreadline $LIBS"
	AC_DEFINE(HAVE_LIBREADLINE, 1,
		  [ Define if you have libreadline and headers ])
	AC_CHECK_FUNCS(rl_completion_matches rl_insert_close)
	AC_CHECK_FUNCS(rl_set_prompt rl_clear_pending_input)
	AC_CHECK_FUNCS(rl_cleanup_after_signal)
	AC_CHECK_FUNCS(rl_filename_completion_function)
	pl_RL_CHECK_VAR([rl_readline_state])
	pl_RL_CHECK_VAR([rl_done])
	pl_RL_CHECK_VAR([rl_event_hook])
      fi
    fi
  fi
fi

if test "$readline_ok" = yes; then
  TARGETS=readline4pl.$SO
  PLTARGETS=readline.pl
fi

AC_OUTPUT(Makefile)
