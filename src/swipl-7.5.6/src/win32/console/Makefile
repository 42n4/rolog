################################################################
# Makefile for the SWI-Prolog console window.
################################################################

srcdir=.
vpath %.c .

CC=gcc
LDEXE=gcc
PLARCH=i386-win32
XLIBS= -lgdi32 -lcomdlg32
SOEXT=dll

COFLAGS=-O2 -fno-strict-aliasing
CWFLAGS=-Wall
CIFLAGS=
CMFLAGS= -D__WINDOWS__ -DWIN32 -DUNICODE -D_UNICODE
CPFLAGS=
CFLAGS=	$(CWFLAGS) $(COFLAGS) $(CIFLAGS) $(CMFLAGS) $(CPFLAGS)
LDFLAGS= $(CPFLAGS)

PLHOME=../../..

include $(srcdir)/../../common.mk

OBJ=	complete.o console.o edit.o history.o menu.o
OUT=	$(PLHOME)/lib/$(PLARCH)/plterm.$(SOEXT)
INCLDIR=$(PLHOME)/include
HDR=	$(INCLDIR)/console.h

all:	$(OUT) $(HDR)

$(OUT):	$(OBJ)
	$(CC) -shared $(COFLAGS) -o $@ -Wl,--out-implib,$@.a $(OBJ) $(XLIBS)

$(HDR):	$(INCLDIR)

$(INCLDIR):
	mkdir -p $@

$(PLHOME)/include/console.h: $(srcdir)/console.h
	cp -f $< $@

console.o: $(srcdir)/console_i.h $(srcdir)/console.h

clean:
	rm -f *.o
	rm -f *~

distclean: clean
	rm -f $(PLHOME)/lib/$(PLARCH)/plterm.$(SOEXT)
	rm -f $(PLHOME)/lib/$(PLARCH)/plterm.$(SOEXT).a
	rm -f $(INCLDIR)/console.h
