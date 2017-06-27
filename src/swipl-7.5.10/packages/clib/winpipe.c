/*  Part of SWI-Prolog

    Author:        Jan Wielemaker
    E-mail:        J.Wielemaker@vu.nl
    WWW:           http://www.swi-prolog.org
    Copyright (c)  2004-2011, University of Amsterdam
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

#include <SWI-Stream.h>
#include <SWI-Prolog.h>
#include <errno.h>

#ifdef __WINDOWS__
#include <windows.h>

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This file provides a windows alternative to   an anonymous pipe that can
be waited for in the same pool as sockets using by tcp_select/3. It is a
work-around that allows a socket-based server  using tcp_select/3 for IO
multiplexing  to  create  an  additional    communication   channal  for
controlling the server.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define PIPE_MAGIC	0x6b3a914cL
#define ISPIPE(p)	if ( p->magic != PIPE_MAGIC ) \
			{ errno = EINVAL; \
			  return -1; \
			}

typedef struct pipe
{ long	magic;				/* magic code */
  int   size;				/* size of the pipe */
  char *buf;				/* Buffer */
  int   in;
  int   out;
  int   writers;			/* write open count */
  int   readers;			/* read open count */
  int	blocking;			/* pipe is blocking */

  CRITICAL_SECTION mutex;
  HANDLE event;				/* signal if data is available */
} pipe;


static pipe *
create_pipe(int size, int blocking)
{ pipe *p = PL_malloc(sizeof(*p));

  p->buf  = PL_malloc(size);
  p->size = size;
  p->in = p->out = 0;
  p->readers = p->writers = 0;
  p->blocking = blocking;

  InitializeCriticalSection(&p->mutex);
  p->event = CreateEvent(NULL, FALSE, FALSE, NULL);
  p->magic = PIPE_MAGIC;

  return p;
}


static int
destroy_pipe(pipe *p)
{ ISPIPE(p);

  EnterCriticalSection(&p->mutex);

  PL_free(p->buf);
  CloseHandle(p->event);
  LeaveCriticalSection(&p->mutex);
  DeleteCriticalSection(&p->mutex);
  p->magic = 0;
  PL_free(p);

  return 0;
}


static int
write_pipe(pipe *pipe, char *buf, int size)
{ ISPIPE(pipe);

  EnterCriticalSection(&pipe->mutex);

  for(;;)
  { if ( pipe->in + size <= pipe->size )
    { memcpy(pipe->buf+pipe->in, buf, size);
      pipe->in += size;

      SetEvent(pipe->event);
      LeaveCriticalSection(&pipe->mutex);

      return size;
    }

    if ( pipe->out > 0 )
    { memmove(pipe->buf, pipe->buf+pipe->out, pipe->in - pipe->out);
      pipe->in -= pipe->out;
      pipe->out = 0;
    }

    if ( size > pipe->size )
      size = pipe->size;
  }
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Non-blocking read from our pipe.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int
read_pipe(pipe *pipe, char *buf, int size)
{ int avail;

  ISPIPE(pipe);

retry:
  EnterCriticalSection(&pipe->mutex);
  avail = pipe->in - pipe->out;

  if ( avail > 0 )
  { if ( size < avail )
      avail = size;

    memcpy(buf, pipe->buf+pipe->out, avail);
    pipe->out += avail;
    if ( pipe->in == pipe->out )
      pipe->in = pipe->out = 0;

    LeaveCriticalSection(&pipe->mutex);
    return avail;
  }

  if ( pipe->writers == 0 )
  { LeaveCriticalSection(&pipe->mutex);
    return 0;
  }

  if ( pipe->blocking )
  { int rc;

    LeaveCriticalSection(&pipe->mutex);
    rc = MsgWaitForMultipleObjects(1,
				   &pipe->event,
				   FALSE,	/* wait for either event */
				   INFINITE,
				   QS_ALLINPUT);
    if ( rc == WAIT_OBJECT_0+1 )	/* message arrived */
    { MSG msg;

      while( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
      { TranslateMessage(&msg);
	DispatchMessage(&msg);
	if ( PL_handle_signals() < 0 )
	{ errno = EINTR;		/* exception */
	  return -1;
	}
      }
    }

    goto retry;
  }

  errno = EWOULDBLOCK;
  LeaveCriticalSection(&pipe->mutex);

  return -1;
}


static int
reader_close_pipe(pipe *p)
{ ISPIPE(p);
  p->readers--;

  if ( p->readers <= 0 && p->writers <= 0 )
    return destroy_pipe(p);

  return 0;
}


static int
writer_close_pipe(pipe *p)
{ ISPIPE(p);
  p->writers--;

  if ( p->readers <= 0 && p->writers <= 0 )
    return destroy_pipe(p);
  if ( p->writers <= 0 )
  { SetEvent(p->event);			/* Signal EOF */
  }

  return 0;
}


static IOFUNCTIONS pipe_read_functions =
{ (Sread_function)    read_pipe,
  (Swrite_function)   write_pipe,
  (Sseek_function)    0,
  (Sclose_function)   reader_close_pipe,
  (Scontrol_function) 0
};


static IOFUNCTIONS pipe_write_functions =
{ (Sread_function)    read_pipe,
  (Swrite_function)   write_pipe,
  (Sseek_function)    0,
  (Sclose_function)   writer_close_pipe,
  (Scontrol_function) 0
};


static foreign_t
tcp_pipe(term_t in, term_t out)
{ pipe *p = create_pipe(4096, TRUE);
  IOSTREAM *sin, *sout;

  sin  = Snew(p, SIO_FBUF|SIO_INPUT|SIO_RECORDPOS,  &pipe_read_functions);
  p->readers++;
  sout = Snew(p, SIO_FBUF|SIO_OUTPUT|SIO_RECORDPOS, &pipe_write_functions);
  p->writers++;

  if ( !PL_unify_stream(in, sin) ||
       !PL_unify_stream(out, sout) )
  { Sclose(sin);
    Sclose(sout);

    return FALSE;
  }

  return TRUE;
}

#else /*__WINDOWS__*/

static foreign_t
tcp_pipe(term_t Read, term_t Write)
{ int fd[2];
  IOSTREAM *in, *out;

  if ( pipe(fd) != 0 )
    return pl_error("pipe", 2, NULL, ERR_ERRNO, errno, "create", "pipe", 0);

  in  = Sfdopen(fd[0], "r");
  out = Sfdopen(fd[1], "w");

  if ( PL_unify_stream(Read, in) &&
       PL_unify_stream(Write, out) )
    return TRUE;

  Sclose(in);
  Sclose(out);

  return FALSE;
}

#endif /*__WINDOWS__*/

install_t
install_winpipe()
{ PL_register_foreign("tcp_pipe", 2, tcp_pipe, 0);
}
