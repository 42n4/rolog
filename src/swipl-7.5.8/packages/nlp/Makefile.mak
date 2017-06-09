################################################################
# Build the SWI-Prolog nlp package for MS-Windows
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

DMPOBJ=		double_metaphone.obj
STEMOBJ=	porter_stem.obj
SBOBJ=		snowball.obj
ISUBOBJ=	isub.obj pl-isub.obj
SBLIB=		libstemmer_c\libstemmer.lib

all:		double_metaphone.dll porter_stem.dll snowball.dll \
		isub.dll

double_metaphone.dll:	$(DMPOBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(DMPOBJ) $(PLLIB)
porter_stem.dll:	$(STEMOBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(STEMOBJ) $(PLLIB) $(LIBS)
snowball.dll:	$(SBOBJ) $(SBLIB)
		$(LD) /dll /out:$@ $(LDFLAGS) $(SBOBJ) $(PLLIB) $(SBLIB) $(LIBS)
isub.dll:	$(ISUBOBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(ISUBOBJ) $(PLLIB) $(LIBS)

$(SBLIB):
		chdir libstemmer_c & $(MAKE)

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
		copy double_metaphone.dll "$(BINDIR)"
		copy porter_stem.dll "$(BINDIR)"
		copy snowball.dll "$(BINDIR)"
		copy isub.dll "$(BINDIR)"
!IF "$(PDB)" == "true"
		copy double_metaphone.pdb "$(BINDIR)"
		copy porter_stem.pdb "$(BINDIR)"
		copy snowball.pdb "$(BINDIR)"
		copy isub.pdb "$(BINDIR)"
!ENDIF

ilib::
		copy double_metaphone.pl "$(PLBASE)\library"
		copy porter_stem.pl "$(PLBASE)\library"
		copy snowball.pl "$(PLBASE)\library"
		copy isub.pl "$(PLBASE)\library"
		$(MAKEINDEX)

uninstall::
		del "$(BINDIR)\double_metaphone.dll"
		del "$(BINDIR)\porter_stem.dll"
		del "$(BINDIR)\snowball.dll"
		del "$(BINDIR)\isub.dll"
		del "$(PLBASE)\library\double_metaphone.pl"
		del "$(PLBASE)\library\porter_stem.pl"
		del "$(PLBASE)\library\snowball.pl"
		del "$(PLBASE)\library\isub.pl"
		$(MAKEINDEX)

html-install::
		copy nlp.html "$(PKGDOC)"

xpce-install::

clean::
		if exist *.obj del *.obj
		if exist *~ del *~
		chdir libstemmer_c & $(MAKE) clean

distclean:	clean
		-DEL *.dll *.lib *.exp *.ilk *.pdb 2>nul
		chdir libstemmer_c & $(MAKE) distclean


