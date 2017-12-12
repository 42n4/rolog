/*  Part of XPCE --- The SWI-Prolog GUI toolkit

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        wielemak@science.uva.nl
    WWW:           http://www.swi-prolog.org/projects/xpce/
    Copyright (c)  2011-2016, University of Amsterdam
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in
       the documentation and/or other materials provided with the
       distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/

#define PL_ARITY_AS_SIZE 1
#include <SWI-Stream.h>
#include <SWI-Prolog.h>
#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include <ctype.h>

#ifdef O_DEBUG
#define DEBUG(g) g
#else
#define DEBUG(g) (void)0
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

static int   breakargs(char *program, char *line, char **argv);
static char *program_name(HANDLE hInstance);
static void  bind_terminal(void);
static int   attach_console(void);

static char *program;

static void
ok(const char *msg, ...)
{ char buf[1024];
  va_list args;

  va_start(args, msg);
  vsprintf(buf, msg, args);
  va_end(args);

  MessageBox(NULL, buf, program, MB_OK|MB_TASKMODAL);
}


int PASCAL
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine, int nCmdShow)
{ char *	argv[100];
  int		argc;

  program = program_name(hInstance);
  argc = breakargs(program, lpszCmdLine, argv);

  bind_terminal();
  PL_set_prolog_flag("verbose", PL_ATOM, "silent"); /* operate silently */
  DEBUG(ok("About to start Prolog with %d arguments", argc));
  if ( !PL_initialise(argc, argv) )
  { ok("Initialisation failed");
    PL_halt(1);
  }

  if ( PL_toplevel() )
  { PL_halt(0);
  } else
  { ok("Toplevel failed");
    PL_halt(1);
  }

  return 0;
}


		 /*******************************
		 *	  USE MS-Console	*
		 *******************************/

static IOFUNCTIONS console_functions;

typedef enum
{ C_NONE,				/* normal operation */
  C_READ,				/* attach on read */
  C_NEVER,				/* do not attach */
  C_ATTACHED				/* a console is attached */
} console_mode;


static console_mode use_console;	/* use the console */
static HANDLE cin;			/* console input handle */
static HANDLE cout;			/* console output handle */
static HANDLE cerr;

static struct recall
{ size_t size;				/* # characters stored */
  size_t allocated;			/* # characters allocated */
  char  *data;				/* buffered text; */
} recall_buffer;



typedef struct
{ HANDLE input;
  LPVOID buf;
  DWORD  len;
  DWORD  done;
  DWORD  rc;
} input_context;

static DWORD WINAPI
getInput(LPVOID h)
{ input_context *ctx = h;

  ctx->rc = ReadConsole(ctx->input, ctx->buf, ctx->len, &ctx->done, NULL);

  return ctx->rc;
}

static ssize_t
read_console(void *h, char *buf, size_t len)
{ HANDLE th;
  input_context ctx;
  DWORD tid;

  ctx.input = cin;
  ctx.buf = buf;
  ctx.len = (DWORD)len;

  th = CreateThread(NULL, 10240, getInput, &ctx, 0, &tid);

  for(;;)
  { DWORD rc = MsgWaitForMultipleObjects(1, &th, FALSE,
					 INFINITE, QS_ALLINPUT);
    if ( rc == WAIT_OBJECT_0 )
    { CloseHandle(th);
      if ( ctx.rc )
      { return ctx.done;
      }
      return -1;			/* error */
    } else /*if ( rc == WAIT_OBJECT_0+1 )*/
    { MSG msg;

      if ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
      { TranslateMessage(&msg);
	DispatchMessage(&msg);
      }
    }
  }
}


static ssize_t
write_console(void *h, char *buf, size_t len)
{ HANDLE co = (h == Soutput->handle ? cout : cerr);
  DWORD done;

  if ( WriteConsole(co, buf, (DWORD)len, &done, NULL) )
    return done;

  return -1;
}


static int
attach_console(void)
{ static int done = 0;

  switch(done)
  { case 0:
      if ( AllocConsole() )
      { cin  = GetStdHandle(STD_INPUT_HANDLE);
	cout = GetStdHandle(STD_OUTPUT_HANDLE);
	cerr = GetStdHandle(STD_ERROR_HANDLE);

	if ( cin  != INVALID_HANDLE_VALUE &&
	     cerr != INVALID_HANDLE_VALUE &&
	     cout != INVALID_HANDLE_VALUE )
	{ use_console = C_ATTACHED;
	  done = 1;

	  if ( recall_buffer.size )
	  { write_console(Soutput, recall_buffer.data, recall_buffer.size);
	    recall_buffer.size = 0;
	    recall_buffer.allocated = 0;
	    PL_free(recall_buffer.data);
	    recall_buffer.data = NULL;
	  }

	  return TRUE;
	}
      }
      done = -1;
      return FALSE;
    case 1:
      return TRUE;			/* already done so */
    default:
      return FALSE;			/* tried but failed */
  }
}



		 /*******************************
		 *	       I/O		*
		 *******************************/

static int
ask_attach(int read)
{ int rc;

  if ( read )
  { rc = MessageBox(NULL,
		    "The application wants to read from its console.\n"
		    "Normally this is caused by an unexpected error.\n"
		    "\n"
		    "Use [OK] to attach a console window\n"
		    "or [CANCEL] to terminate the application",
		    program,
		    MB_OKCANCEL|MB_DEFBUTTON1|
		    MB_TASKMODAL|MB_ICONEXCLAMATION);
  } else
  { rc = MessageBox(NULL,
		    "The application wants to write error messages.\n"
		    "Normally this is caused by an unexpected error.\n"
		    "\n"
		    "Use [YES] to attach a console window\n"
		    "Use [NO] to ignore all messages\n"
		    "or [CANCEL] to terminate the application",
		    program,
		    MB_YESNOCANCEL|MB_DEFBUTTON1|
		    MB_TASKMODAL|MB_ICONEXCLAMATION);
  }

  switch(rc)
  { case IDABORT:
    case IDCANCEL:

      PostQuitMessage(1);
      ExitProcess(1);
    case IDYES:
    case IDOK:
    case IDRETRY:
      return TRUE;
    case IDNO:
      use_console = C_READ;
      return FALSE;
    case IDIGNORE:
    default:
      return FALSE;
  }
}


static ssize_t
do_read(void *handle, char *buffer, size_t size)
{ switch( use_console )
  { case C_ATTACHED:
      return read_console(handle, buffer, size);
    case C_NONE:
    case C_READ:
      if ( ask_attach(TRUE) )
      { if ( attach_console() )
	  return read_console(handle, buffer, size);
      }
    case C_NEVER:
      return -1;
  }

  return -1;
}


static ssize_t
do_write(void *handle, char *buffer, size_t size)
{ struct recall *rc = &recall_buffer;

  switch(use_console)
  { case C_ATTACHED:
      return write_console(handle, buffer, size);
    case C_NONE:
      if ( handle == Serror->handle )
      { if ( ask_attach(FALSE) )
	{ if ( attach_console() )
	    return write_console(handle, buffer, size);
	}
      }
      break;
    case C_READ:
      break;
    case C_NEVER:
      return size;
  }

  if ( !rc->data )
  { rc->allocated = 512;
    rc->data = PL_malloc(rc->allocated);
    rc->size = 0;
  }
  if ( size + rc->size <= rc->allocated )
  { memcpy(&rc->data[rc->size], buffer, size);
    rc->size += size;
  } else if ( size >= rc->allocated )
  { memcpy(rc->data, &buffer[size-rc->allocated], rc->allocated);
    rc->size = rc->allocated;
  } else
  { int leave = rc->allocated - size;

    memmove(rc->data, &rc->data[rc->size-leave], leave);
    memcpy(&rc->data+leave, buffer, size);
    rc->size = rc->allocated;
  }

  return size;
}


static void
bind_terminal(void)
{ console_functions       = *Sinput->functions;
  console_functions.read  = do_read;
  console_functions.write = do_write;

  Sinput->functions  = &console_functions;
  Soutput->functions = &console_functions;
  Serror->functions  = &console_functions;
}



		 /*******************************
		 *		UTIL		*
		 *******************************/

static void
long_name(char *file)
{ char buf[MAXPATHLEN];
  char *i = file;
  char *o = buf;
  char *ok = buf;
  int changed = 0;

  while(*i)
  { int dirty = FALSE;

    while(*i && *i != '\\')
    { if ( *i == '~' )
	dirty++;
      *o++ = *i++;
    }
    if ( dirty )
    { WIN32_FIND_DATA data;
      HANDLE h;

      *o = '\0';
      if ( (h=FindFirstFile(buf, &data)) != INVALID_HANDLE_VALUE )
      { strcpy(ok, data.cFileName);
	FindClose(h);
	o = ok + strlen(ok);
	changed++;
      }
    }
    if ( *i )
      *o++ = *i++;
    ok = o;
  }

  if ( changed )
  { *o = '\0';
    strcpy(file, buf);
  }
}


static char *
program_name(HANDLE hInstance)
{ char program[MAXPATHLEN];

  GetModuleFileName(hInstance, program, sizeof(program));
  long_name(program);

  return strcpy(malloc(strlen(program)+1), program);
}


static int
breakargs(char *program, char *line, char **argv)
{ int argc = 1;

  argv[0] = program;

  while(*line)
  { while(*line && isspace(*line))
      line++;

    if ( *line == '"' )			/* Windows-95 quoted arguments */
    { char *start = line+1;
      char *end = start;

      while( *end && *end != '"' )
	end++;
      if ( *end == '"' )
      { *end = '\0';
        argv[argc++] = start;
	line = end+1;
	continue;
      }
    }

    if ( *line )
    { argv[argc++] = line;
      while(*line && !isspace(*line))
	line++;
      if ( *line )
	*line++ = '\0';
    }
  }
  argv[argc] = NULL;			/* add trailing NULL pointer to argv */

  return argc;
}
