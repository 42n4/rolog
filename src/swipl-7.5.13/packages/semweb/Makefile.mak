################################################################
# Build the SWI-Prolog semantic web package for MS-Windows
#
# Author: Jan Wielemaker
#
# Use:
#	nmake /f Makefile.mak
#	nmake /f Makefile.mak install
################################################################

PLHOME=..\..
!include ..\..\src\rules.mk
!include common.mk

MAKEINDEXSEMWEB=chdir "$(PLBASE)" & del library\semweb\INDEX.pl & bin\swipl.exe \
			-f none -F none \
			-g make_library_index('library/semweb') \
			-t halt

LIBDIR=		$(PLBASE)\library\semweb
PKGDLL=rdf_db

OBJ=		rdf_db.obj md5.obj avl.obj atom_map.obj atom.obj \
		lock.obj debug.obj hash.obj murmur.obj

all:		$(PKGDLL).dll turtle.dll

$(PKGDLL).dll:	$(OBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(OBJ) $(PLLIB) $(LIBS)
turtle.dll:	turtle.obj
		$(LD) /dll /out:$@ $(LDFLAGS) turtle.obj $(PLLIB) $(LIBS)

turtle.obj:	turtle.c turtle_chars.c

install:	idll ilib

idll::
		copy $(PKGDLL).dll "$(BINDIR)"
		copy turtle.dll "$(BINDIR)"
ilib::
		if not exist "$(LIBDIR)/$(NULL)" $(MKDIR) "$(LIBDIR)"
		@echo Copying $(LIBPL)
		@for %f in ($(LIBPL)) do @copy %f "$(LIBDIR)"
		@for %f in ($(DATA)) do @copy %f "$(LIBDIR)"
		copy README "$(LIBDIR)\README.TXT"
		$(MAKEINDEX)
		$(MAKEINDEXSEMWEB)

html-install::
		copy semweb.html "$(PKGDOC)"
		copy modules.gif "$(PKGDOC)"
pdf-install:
		copy semweb.pdf "$(PKGDOC)"

xpce-install::

uninstall::
		del "$(PLBASE)\bin\$(PKGDLL).dll"
		cd $(LIBDIR) & del $(LIBPL) $(DATA) README.TXT
		rmdir $(LIBDIR)
		$(MAKEINDEX)

clean::
		if exist *.obj del *.obj
		if exist *~ del *~

distclean:	clean
		-DEL *.dll *.lib *.exp *.pdb *.ilk 2>nul

