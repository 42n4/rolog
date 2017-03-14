#!/bin/sh

# Run pl from the development system (i.e.   after  the base system is
# compiled, but not yet  installed.  If   we  are  building using shared
# libraries we must set LD_LIBRARY_PATH to be able to find them.
#
# On some machines LD_LIBRARY_PATH is called different.  The configure
# variable DLLIBVAR holds its name.

bdir=/c/Users/Matthias Gondan/Documents/Dropbox/rolog/rolog/src/swipl-7.5.2/src
PLARCH=i386-win32
PL=swipl
EXEEXT=.exe
ldir=$bdir/../lib/$PLARCH
sep=":"

SWIPL_BOOT_FILE=$bdir/swipl.prc
export SWIPL_BOOT_FILE

if [ ":" != ":" ]; then
  lib="$(: -w $lib)"
  sep=";"
fi

if [ "x$PATH" = "x" ]; then
  PATH=$ldir
else
  PATH="$ldir$sep$PATH"
fi
export PATH

case "$1" in
  -C) cd "$2"
      shift
      shift
      ;;
   *) ;;
esac

$bdir/$PL$EXEEXT "$@"
