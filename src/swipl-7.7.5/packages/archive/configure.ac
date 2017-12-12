dnl Process this file with autoconf to produce a configure script.

AC_INIT(install-sh)
AC_PREREQ([2.50])
AC_CONFIG_HEADER(config.h)

AC_SUBST(ARCHIVE_TARGETS)
AC_SUBST(ARCHIVE_PLTARGETS)

m4_include([../ac_swi_c.m4])

dnl Hacks for MacOS handling of libarchive.  We want to prefer the MacOS X
dnl libraries, but we need to get libarchive from Macports, as MacOS has some
dnl mostly incompatible version with this name.

case "$PLARCH" in
  *darwin*)
	if test -z "$ARPREFIX"; then
	  ARPREFIX=/opt/local
        fi
	;;
esac

if test ! -z "ARPREFIX"; then
  AC_MSG_CHECKING(Whether setting for ARPREFIX is ok)
  if test -f $ARPREFIX/include/archive.h -a \
	  -f $ARPREFIX/lib/libarchive.$PLSOEXT; then
      LIBS="$LIBS -v $ARPREFIX/lib/libarchive.$PLSOEXT"
      AC_MSG_RESULT(Using $ARPREFIX)
      AC_DEFINE(HAVE_LIBARCHIVE, 1,
                [Define to 1 if you have libarchive])
      AC_DEFINE(HAVE_ARCHIVE_H, 1,
                [Define to 1 if you have <archive.h>])
      CFLAGS="-I$ARPREFIX/include $CFLAGS"
      LDFLAGS="-L$ARPREFIX/lib $LDFLAGS"
      ARCHIVELIB=yes
  else
    AC_MSG_RESULT(no)
    ARPREFIX=
  fi
fi

if test -z "$ARPREFIX"; then
  AC_CHECK_HEADER(archive.h,
		  [ AC_DEFINE([HAVE_ARCHIVE_H], 1,
			      [Define to 1 if you have <archive.h>.])
		    ARCHIVEH=yes
		  ],
		  ARCHIVEH=no)

  if test "$ARCHIVEH" = yes; then
    AC_CHECK_LIB(archive, archive_read_new,
		 LIBS="$LIBS -larchive"
		 ARCHIVELIB=yes,
		 ARCHIVELIB=no)
  fi
fi

AC_CHECK_FUNCS(archive_read_support_filter_bzip2 \
	       archive_read_support_filter_compress \
	       archive_read_support_filter_gzip \
	       archive_read_support_filter_grzip \
	       archive_read_support_filter_lrzip \
	       archive_read_support_filter_lzip \
	       archive_read_support_filter_lzma \
	       archive_read_support_filter_lzop \
	       archive_read_support_filter_none \
	       archive_read_support_filter_rpm \
	       archive_read_support_filter_uu \
	       archive_read_support_filter_xz)

AC_CHECK_FUNCS(archive_read_support_compression_bzip2 \
	       archive_read_support_compression_compress \
	       archive_read_support_compression_gzip \
	       archive_read_support_compression_lzma \
	       archive_read_support_compression_none \
	       archive_read_support_compression_xz)

AC_CHECK_FUNCS(archive_read_support_format_7zip \
	       archive_read_support_format_ar \
	       archive_read_support_format_cab \
	       archive_read_support_format_cpio \
	       archive_read_support_format_empty \
	       archive_read_support_format_gnutar \
	       archive_read_support_format_iso9660 \
	       archive_read_support_format_lha \
	       archive_read_support_format_mtree \
	       archive_read_support_format_rar \
	       archive_read_support_format_raw \
	       archive_read_support_format_tar \
	       archive_read_support_format_xar \
	       archive_read_support_format_zip)

AC_CHECK_FUNCS(archive_read_free \
	       archive_filter_count \
	       archive_read_open1)

if test "$ARCHIVELIB" = yes; then
  ARCHIVE_TARGETS="archive4pl.$SO"
  ARCHIVE_PLTARGETS=archive.pl
else
  ARCHIVE_TARGETS=nolib
  cat << EOF
##################################################################
# ERROR: Could not find library archive (-larchive).  Dropped
# library(archive). Library archive is available from
# http://code.google.com/p/libarchive/
#
# Most Unix/Linux distributions are shipped with binaries.  Make
# sure to have the development library installed.  E.g.
#
#   Debian/Ubuntu/Mint:	aptitude install libarchive-dev
#   Fedora/...		yum install libarchive-devel
#   MacOS (Macports):   port install libarchive
##################################################################
EOF
fi

AC_OUTPUT(Makefile)
