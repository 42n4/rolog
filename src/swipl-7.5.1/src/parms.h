#ifndef PLHOME
#define PLHOME       "/usr/local/swipl-7.5.1"
#endif
#ifndef DEFSTARTUP
#define DEFSTARTUP   "swipl.ini"
#endif
#define PLVERSION 70501
#ifndef PLARCH
#define PLARCH	    "i386-win32"
#endif
#define C_LIBS	    "-limagehlp -ldbghelp -lm -lpthread -lwsock32 -lwinmm -lshell32 "
#define C_PLLIB	    "-lswipl"
#define C_LIBPLSO    "-lswipl"
#ifndef C_CC
#define C_CC	    "gcc"
#endif
#ifndef C_CFLAGS
#define C_CFLAGS	    "-D__WINDOWS__ -DWIN32 "
#endif
#ifndef C_LDFLAGS
#define C_LDFLAGS    ""
#endif
