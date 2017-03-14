################################################################
# Build the SWI-Prolog tabling package for MS-Windows
#
# Author: Jan Wielemaker
#
# Use:
#	nmake /f Makefile.mak
#	nmake /f Makefile.mak install
################################################################

PLHOME=..\..
!include $(PLHOME)\src\rules.mk
PKGDLL=socket

SOCKOBJ=	socket.obj nonblockio.obj error.obj
CGIOBJ=		error.obj form.obj cgi.obj
CRYPTOBJ=	error.obj crypt.obj md5.obj md5passwd.obj bsd-crypt.obj
MEMOBJ=		error.obj memfile.obj
MIMELIBS=	rfc2045.lib rfc822.lib
TIMEOBJ=	error.obj time.obj
READOBJ=	readutil.obj
PROCESSOBJ=	error.obj process.obj
SHAOBJ=		error.obj sha4pl.obj sha1/sha1.obj sha1/sha2.obj \
		sha1/hmac_sha1.obj sha1/hmac_sha256.obj
URIOBJ=		uri.obj
FILESOBJ=	error.obj files.obj
STREAMINFOOBJ=	error.obj streaminfo.obj
TIMELIBS=	winmm.lib

all:		socket.dll cgi.dll memfile.dll time.dll readutil.dll \
		crypt.dll sha4pl.dll process.dll uri.dll files.dll streaminfo.dll

readutil.dll:	$(READOBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(READOBJ) $(PLLIB) $(LIBS)
process.dll:	$(PROCESSOBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(PROCESSOBJ) $(PLLIB) $(LIBS)
socket.dll:	$(SOCKOBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(SOCKOBJ) $(PLLIB) $(LIBS)
cgi.dll:	$(CGIOBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(CGIOBJ) $(PLLIB) $(LIBS)
crypt.dll:	$(CRYPTOBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(CRYPTOBJ) $(PLLIB) $(LIBS)
memfile.dll:	$(MEMOBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(MEMOBJ) $(PLLIB) $(LIBS)
time.dll:	$(TIMEOBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(TIMEOBJ) $(PLLIB) $(LIBS) $(TIMELIBS)
sha4pl.dll:	$(SHAOBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(SHAOBJ) $(PLLIB) $(LIBS)
uri.dll:	$(URIOBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(URIOBJ) $(PLLIB) $(LIBS)
files.dll:	$(FILESOBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(FILESOBJ) $(PLLIB) $(LIBS)
streaminfo.dll:	$(STREAMINFOOBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(STREAMINFOOBJ) $(PLLIB) $(LIBS)

sha1/hmac_sha1.obj:	sha1/hmac.c
		$(CC) -I $(PLHOME)\include $(CFLAGS) /DUSE_SHA1 /Fo$@ sha1/hmac.c
sha1/hmac_sha256.obj:	sha1/hmac.c
		$(CC) -I $(PLHOME)\include $(CFLAGS) /DUSE_SHA256 /Fo$@ sha1/hmac.c

process.obj:	win_error.c
files.obj:	win_error.c
socket.obj:	sockcommon.c

!IF "$(CFG)" == "rt"
install:	idll
!ELSE
install:	idll ilib
!ENDIF

################################################################
# Testing
################################################################

check:		check-socket

torture:	torture-socket

check-socket::
		"$(PLCON)" -q -f testsocket.pl -F none -g tcp_test,halt -t 'halt(1)'

torture-socket::
		"$(PLCON)" -q -f stresssocket.pl -F none -g test,halt -t 'halt(1)'

################################################################
# Installation
################################################################

idll::
		copy socket.dll "$(BINDIR)"
		copy cgi.dll "$(BINDIR)"
		copy crypt.dll "$(BINDIR)"
		copy memfile.dll "$(BINDIR)"
		copy time.dll "$(BINDIR)"
		copy readutil.dll "$(BINDIR)"
		copy process.dll "$(BINDIR)"
		copy sha4pl.dll "$(BINDIR)"
		copy uri.dll "$(BINDIR)"
		copy files.dll "$(BINDIR)"
		copy streaminfo.dll "$(BINDIR)"
!IF "$(PDB)" == "true"
		copy socket.pdb "$(BINDIR)"
		copy cgi.pdb "$(BINDIR)"
		copy memfile.pdb "$(BINDIR)"
		copy time.pdb "$(BINDIR)"
		copy readutil.pdb "$(BINDIR)"
		copy process.pdb "$(BINDIR)"
		copy sha4pl.pdb "$(BINDIR)"
		copy uri.pdb "$(BINDIR)"
		copy files.pdb "$(BINDIR)"
		copy streaminfo.pdb "$(BINDIR)"
!ENDIF

ilib::
		copy socket.pl "$(PLBASE)\library"
		copy prolog_server.pl "$(PLBASE)\library"
		copy streampool.pl "$(PLBASE)\library"
		copy streaminfo.pl "$(PLBASE)\library"
		copy cgi.pl "$(PLBASE)\library"
		copy crypt.pl "$(PLBASE)\library"
		copy memfile.pl "$(PLBASE)\library"
		copy time.pl "$(PLBASE)\library"
		copy sha.pl "$(PLBASE)\library"
		copy uri.pl "$(PLBASE)\library"
		copy filesex.pl "$(PLBASE)\library"
		copy process.pl "$(PLBASE)\library"
		copy udp_broadcast.pl "$(PLBASE)\library"
		$(MAKEINDEX)

uninstall::
		del "$(BINDIR)\socket.dll"
		del "$(BINDIR)\streampool.dll"
		del "$(BINDIR)\streaminfo.dll"
		del "$(BINDIR)\cgi.dll"
		del "$(BINDIR)\crypt.dll"
		del "$(BINDIR)\memfile.dll"
		del "$(BINDIR)\time.dll"
		del "$(BINDIR)\readutil.dll"
		del "$(BINDIR)\sha4pl.dll"
		del "$(BINDIR)\uri.dll"
		del "$(BINDIR)\files.dll"
		del "$(PLBASE)\library\socket.pl"
		del "$(PLBASE)\library\cgi.pl"
		del "$(PLBASE)\library\crypt.pl"
		del "$(PLBASE)\library\memfile.pl"
		del "$(PLBASE)\library\time.pl"
		del "$(PLBASE)\library\sha.pl"
		del "$(PLBASE)\library\uri.pl"
		del "$(PLBASE)\library\filesex.pl"
		del "$(PLBASE)\library\process.pl"
		del "$(PLBASE)\library\udp_broadcast.pl"
		$(MAKEINDEX)

html-install::
		copy clib.html "$(PKGDOC)"

xpce-install::

clean::
		if exist *.obj del *.obj
		if exist sha1\*.obj del sha1\*.obj
		if exist *~ del *~

distclean:	clean
		-DEL *.dll *.lib *.exp *.ilk *.pdb 2>nul


