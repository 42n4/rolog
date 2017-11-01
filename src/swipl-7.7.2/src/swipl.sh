#!/bin/sh

# This script is used to start Prolog in the development environment
# when its shared libraries are not yet installed
#
# Typically, this must be done  using   LD_LIBRARY_PATH  on ELF systems.
# Unfortunately, we use rpath=<path> to find libswipl.so, which sets the
# ELF-symbol DT_RPATH, which overrules LD_LIBRARY_PATH.  What we want is
# DT_RUNPATH, which is behind LD_LIBRARY_PATH  in the search-order. This
# however cannot be set  directly  on   some  systems.  Therefore, where
# available, we use chrpath -c in the  Makefile to convert DT_RPATH into
# DT_RUNPATH. If this process fails,  you   may  have problems upgrading
# SWI-Prolog from source because the version-being-built uses the shared
# object from the installed version. If  the installed version is broken
# or incompatible, your build will  fail.   There  are  two ways around:
# install chrpath (if available) or remove   the installed system before
# rebuilding.

bdir=/home/matthias/rolog/src/swipl-7.7.2/src
bdir=$BDIR
lib=$bdir/../lib/i386-win32
libso=$lib/libswipl.dll
sep=":"

if [ ":" != ":" ]; then
  lib="$(: -w $lib)"
  sep=";"
fi

if [ "x$PATH" = "x" ]; then
  PATH="$lib"
else
  PATH="$lib$sep$PATH"
fi

export PATH

case "$1" in
  -C) cd "$2"
      shift
      shift
      ;;
   *) ;;
esac

$bdir/swipl.exe "$@" || exit 1
