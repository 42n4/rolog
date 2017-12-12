#!/bin/sh

# Run plld from the development system (i.e.   after  the base system is
# compiled, but not yet  installed.  If   we  are  building using shared
# libraries we must set LD_LIBRARY_PATH to be able to find them.
#
# On some machines LD_LIBRARY_PATH is called different.  The configure
# variable DLLIBVAR holds its name.

bdir=/home/Matthias/rolog/src/swipl-7.7.3/src
PLARCH=i386-win32
EXEEXT_FOR_BUILD=.exe
ldir=$bdir/../lib/$PLARCH

if [ "x$PATH" = "x" ]; then
  PATH=$ldir
else
  PATH="$ldir:$PATH"
fi
export PATH

case "$PLARCH" in
    *-win32|*-win64)
        LDFLAGS=-luxnt
	;;
esac

$bdir/swipl-ld-build$EXEEXT_FOR_BUILD -build-defaults "$@" $LDFLAGS
