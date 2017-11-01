################################################################
# The uxnt library provides some additional Unix compatibility
# stuff on top of the MSVC (posix) runtime library.
################################################################

srcdir=.
vpath %.c .

CC=gcc
LDEXE=gcc
AR=ar
RANLIB=ranlib
PLARCH=i386-win32

COFLAGS=-O2 -fno-strict-aliasing
CWFLAGS=-Wall
CIFLAGS=
CMFLAGS= -D__WINDOWS__ -DWIN32
CPFLAGS=
CFLAGS=	$(CWFLAGS) $(COFLAGS) $(CIFLAGS) $(CMFLAGS) $(CPFLAGS)
LDFLAGS= $(CPFLAGS)

PLHOME=../../..

include $(srcdir)/../../common.mk

OBJ=	uxnt.o
OUT=	$(PLHOME)/lib/$(PLARCH)/libuxnt.a
INCLDIR=$(PLHOME)/include
HDR=	$(INCLDIR)/uxnt.h $(INCLDIR)/dirent.h

all:	$(OUT) $(HDR)

$(OUT):	$(OBJ)
	rm -f $@
	$(AR) r $@ $(OBJ)
	$(RANLIB) $@

$(HDR):	$(INCLDIR)

$(INCLDIR):
	mkdir -p $@

$(PLHOME)/include/uxnt.h: $(srcdir)/uxnt.h
	cp -f $< $@

$(PLHOME)/include/dirent.h: $(srcdir)/dirent.h
	cp -f $< $@

clean:
	rm -f *.o
	rm -f *~

distclean: clean
	rm -f libuxnt.a
