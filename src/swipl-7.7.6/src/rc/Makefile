################################################################
# Makefile for the SWI-Prolog resource database
#
# Author: Jan Wielemaker
# E-mail: J.Wielemaker@cs.vu.nl
#
# Copying: Simplified BSD
#
# Copyright (c) 1996-2016, VU University Amsterdam
#			   University of Amsterdam
# ###############################################################

prefix=/usr/local
exec_prefix=${prefix}
srcdir=.
vpath %.c .
bindir=$(exec_prefix)/bin
PLBASE=${exec_prefix}/lib/swipl-$(PLVERSION)
man_prefix=$(prefix)/man
mansec=1
lmansec=3
mandir=$(man_prefix)/man$(mansec)
lmandir=$(man_prefix)/man$(lmansec)
plmandir=$(PLBASE)/man

CC=gcc
EXEEXT = .exe
AR=ar
RANLIB=ranlib
PLARCH=i386-win32
COFLAGS=-O2 -fno-strict-aliasing
CWFLAGS=-Wall
CIFLAGS=
CMFLAGS= -D__WINDOWS__ -DWIN32
CPFLAGS=
LDFLAGS= $(CPFLAGS)
CFLAGS=	$(CWFLAGS) $(COFLAGS) $(CIFLAGS) $(CMFLAGS) $(CPFLAGS)

INSTALL=/bin/install -c
INSTALL_PROGRAM=${INSTALL}
INSTALL_DATA=${INSTALL} -m 644

MINGW=true
ifeq ($(MINGW),true)
UXNTLIB=../../lib/$(PLARCH)/libuxnt.a
else
UXNTLIB=
endif

LIBOBJ=	html.o access.o build.o util.o

all:	swipl-rc$(EXEEXT)

swipl-rc$(EXEEXT):	librc.a rc.o $(UXNTLIB)
	$(CC) $(LDFLAGS) -o $@ rc.o $(UXNTLIB) -L. -lrc

librc.a:	$(LIBOBJ)
	rm -f $@
	$(AR) cq $@ $(LIBOBJ)
	$(RANLIB) $@

ifeq ($(MINGW),true)
$(UXNTLIB):
	@(cd ../os/windows && $(MAKE) COFLAGS="$(COFLAGS)")
endif

.c.o:
	$(CC) -c -I.. $(CFLAGS) $< -o $@

################################################################
# Documentation
################################################################

html:	rc.tex
	latex2html rc

dvi:	rc.dvi

rc.dvi:	rc.tex
	./latex_if_needed rc

rc.tex:	rc.doc
	../xpce/bin/doc2tex rc.doc > rc.tex

################################################################
# Cleanup
################################################################

clean:
	rm -f *% *~ $(LIBOBJ) rc.o

profclean:
	rm -f *.gcda

distclean: clean profclean
	rm -f Makefile
	rm -f swipl-rc librc.a

