################################################################
# Build the SWI-Prolog zlib package for MS-Windows
#
# Author: Jan Wielemaker
#
# Use:
#	nmake /f Makefile.mak
#	nmake /f Makefile.mak install
################################################################

PLHOME=..\..
!include $(PLHOME)\src\rules.mk
CFLAGS=$(CFLAGS) /D__SWI_PROLOG__

ARLIB=archive_static
LIBS=zlibwapi.lib $(LIBS)
CFLAGS=$(CFLAGS)

OBJ=		archive4pl.obj

all:		archive4pl.dll

archive4pl.dll:	$(OBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(OBJ) $(ARLIB).lib $(PLLIB) $(LIBS)

!IF "$(CFG)" == "rt"
install:	idll
!ELSE
install:	idll ilib
!ENDIF

################################################################
# Testing
################################################################

check::

################################################################
# Installation
################################################################

idll::
		copy archive4pl.dll "$(BINDIR)"
!IF "$(PDB)" == "true"
		copy archive4pl.pdb "$(BINDIR)"
!ENDIF

ilib::
		copy archive.pl "$(PLBASE)\library"
		$(MAKEINDEX)

uninstall::
		del "$(BINDIR)\archive4pl.dll"
		del "$(PLBASE)\library\archive.pl"
		$(MAKEINDEX)

html-install::
		-copy archive.html "$(PKGDOC)"

xpce-install::

clean::
		if exist *.obj del *.obj
		if exist *~ del *~

distclean:	clean
		-DEL *.dll *.lib *.exp *.ilk *.pdb 2>nul


