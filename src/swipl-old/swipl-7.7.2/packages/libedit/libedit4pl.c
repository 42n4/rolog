/*  Part of SWI-Prolog

    Author:        Jan Wielemaker
    E-mail:        J.Wielemaker@vu.nl
    WWW:           http://www.swi-prolog.org
    Copyright (c)  2017, VU University Amsterdam
			 CWI, Amsterdam
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

#include <string.h>
#include <stdlib.h>
#define PL_ARITY_AS_SIZE 1
#include <SWI-Stream.h>
#include <SWI-Prolog.h>
#include <config.h>
#include <signal.h>
#include <assert.h>
#include <histedit.h>
#include <errno.h>
#include <limits.h>

#if defined(HAVE_POLL_H) && defined(HAVE_POLL)
#include <poll.h>
#elif defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_UXNT_H
#include <uxnt.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

static atom_t ATOM_norm;
static atom_t ATOM_newline;
static atom_t ATOM_eof;
static atom_t ATOM_arghack;
static atom_t ATOM_refresh;
static atom_t ATOM_refresh_beep;
static atom_t ATOM_cursor;
static atom_t ATOM_redisplay;
static atom_t ATOM_error;
static atom_t ATOM_fatal;
static atom_t ATOM_clear;
static atom_t ATOM_setsize;
static atom_t ATOM_setunique;

static functor_t FUNCTOR_line2;
static functor_t FUNCTOR_electric3;
static functor_t FUNCTOR_pair2;

#define STR_OPTIONS (CVT_ATOM|CVT_STRING|CVT_LIST|REP_MB|CVT_EXCEPTION)


		 /*******************************
		 *	      CONTEXT		*
		 *******************************/

typedef struct command
{ struct command       *next;
  atom_t		name;		/* name of the command */
  record_t		closure;	/* called goal */
  module_t		module;		/* target module */
} command;

typedef struct binding
{ struct binding       *next;
  int		        ch;
  command	       *command;
} binding;


typedef enum electric_state
{ E_NONE = 0,				/* idle state */
  E_WAIT,				/* Wait for timeout */
  E_COMMAND				/* get second char */
} electric_state;

#define EL_CTX_MAGIC 1329760607

typedef struct el_context
{ struct el_context    *next;		/* Next in list */
  int			magic;		/* EL_CTX_MAGIC */
  int			fd;		/* Input file we are attached to */
  IOSTREAM	       *istream;	/* Input stream */
  IOSTREAM	       *ostream;	/* Output stream */
  IOSTREAM	       *estream;	/* Error stream */
  EditLine	       *el;		/* EditLine context */
  char *		buffered;	/* Buffered long line */
  int			sig_no;		/* For read_char() */
  HistEvent		ev;		/* History event */
  History	       *history;	/* Complete history */
  char		       *prompt;		/* Current prompt */
  IOFUNCTIONS	       *orig_functions;	/* Original functions */
  IOFUNCTIONS	        functions;	/* SIO function block */
  command	       *commands;	/* User commands */
  binding	       *bindings;	/* Bindings to user commands */
  int			reader;		/* Current reader thread */
  struct
  { int timeout;			/* Time to wait */
    int move;				/* Amount to move */
    electric_state state;
  } electric;
#ifndef HAVE_EL_CURSOR
  int			move_cursor;	/* Amount to move the cursor */
#endif
} el_context;

static el_context *el_clist;

static el_context *
get_context(int fd)
{ el_context *c;

  for(c=el_clist; c; c=c->next)
  { if ( c->fd == fd )
      return c;
  }

  return NULL;
}

static el_context *
get_context_from_handle(void *handle)
{ el_context *c;

  for(c=el_clist; c; c=c->next)
  { IOSTREAM *s;

    if ( (s=c->istream) && s->handle == handle )
      return c;
  }

  return NULL;
}

static el_context *
get_context_from_ohandle(void *handle)
{ el_context *c;

  for(c=el_clist; c; c=c->next)
  { IOSTREAM *s;

    if ( (s=c->ostream) && s->handle == handle )
      return c;
    if ( (s=c->estream) && s->handle == handle )
      return c;
  }

  return NULL;
}

static el_context *
alloc_context(int fd)
{ el_context *c = PL_malloc(sizeof(*c));

  memset(c, 0, sizeof(*c));
  c->fd    = fd;
  c->magic = EL_CTX_MAGIC;
  c->next  = el_clist;
  el_clist = c;

  return c;
}


static void
update_prompt(el_context *ctx)
{ char *np = PL_prompt_string(ctx->fd);

  if ( ctx->prompt && np && strcmp(np, ctx->prompt) == 0 )
    return;
  if ( ctx->prompt )
    free(ctx->prompt);
  ctx->prompt = np ? strdup(np) : NULL;
}


		 /*******************************
		 *	        PORT		*
		 *******************************/

#ifndef HAVE_EL_CURSOR
#define el_cursor(el, cnt) el_cursor_emulated(el, cnt)

static int
el_cursor_emulated(EditLine *el, int count)
{ el_context *ctx;
  const LineInfo *li;

  el_get(el, EL_CLIENTDATA, (void**)&ctx);
  li = el_line(ctx->el);

  if ( count < 0 )			/* backward */
  { if ( -count > li->cursor - li->buffer )
      count = li->buffer - li->cursor;
  } else
  { if ( count > li->lastchar - li->cursor )
      count = li->lastchar - li->cursor;
  }

  ctx->move_cursor = count;

  return li->cursor - li->buffer + count;
}
#endif


		 /*******************************
		 *	  SIGNAL HANDLING	*
		 *******************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This code is copied from our GNU libreadline wrapper.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef struct
{ int			signo;		/* number of the signal */
  struct sigaction	old_state;	/* old state for the signal */
} sigstate;

static void el_sighandler(int sig);

static sigstate el_signals[] =
{ { SIGINT },
#ifdef SIGTSTP
  { SIGTSTP },
  { SIGCONT },
  { SIGTTOU },
  { SIGTTIN },
#endif
  { SIGALRM },
  { SIGTERM },
  { SIGQUIT },
  { SIGWINCH },
  { -1 }
};

static sigstate cont_signals[] =
{
#ifdef SIGCONT
  { SIGCONT },
#endif
  { -1 }
};


static void
prepare_signals(sigstate *s)
{ for(; s->signo != -1; s++)
  { struct sigaction new;

    memset(&new, 0, sizeof(new));
    new.sa_handler = el_sighandler;
    sigaction(s->signo, &new, &s->old_state);
  }
}


static void
restore_signals(sigstate *s)
{ for(; s->signo != -1; s++)
  { sigaction(s->signo, &s->old_state, NULL);
  }
}


static void
el_sighandler(int sig)
{ sigstate *s;
  el_context *ctx;

  for(ctx=el_clist; ctx; ctx=ctx->next)
    ctx->sig_no = sig;

  switch(sig)
  { case SIGWINCH:
      return;
    case SIGCONT:
      if ( (ctx = get_context(0)) )
	el_set(ctx->el, EL_PREP_TERM, 1);
      restore_signals(cont_signals);
      prepare_signals(el_signals);
      return;
    case SIGTSTP:
      restore_signals(el_signals);
      prepare_signals(cont_signals);
      if ( (ctx = get_context(0)) )
	el_set(ctx->el, EL_PREP_TERM, 0);
      kill(getpid(), sig);
      return;
    case SIGINT:
    { if ( (ctx = get_context(0)) )
      { FILE *err;
	int size = el_cursor(ctx->el, 10000);

	el_deletestr(ctx->el, size);
	el_get(ctx->el, EL_GETFP, 2, &err);
	fprintf(err, "^C\n");
      }
    }
  }

  restore_signals(el_signals);
  if ( (ctx = get_context(0)) )
    el_set(ctx->el, EL_PREP_TERM, 0);

  for(s=el_signals; s->signo != -1; s++)
  { if ( s->signo == sig )
    { void (*func)(int) = s->old_state.sa_handler;

      if ( func == SIG_DFL )
      { PL_raise(sig);			/* was: kill(getpid(), sig); */
      } else if ( func != SIG_IGN )
      { (*func)(sig);
      }

      break;
    }
  }

  if ( (ctx = get_context(0)) )
    el_set(ctx->el, EL_PREP_TERM, 1);
  prepare_signals(el_signals);
}


static const char *
el_siggets(EditLine *el, int *count)
{ const char *line;
  FILE *in;

  el_get(el, EL_GETFP, 0, &in);
  if ( fileno(in) == 0 )
  { prepare_signals(el_signals);
    line = el_gets(el, count);
    restore_signals(el_signals);
  } else
  { line = el_gets(el, count);
  }

  return line;
}

		 /*******************************
		 *     ELECTRIC CARET SUPPORT	*
		 *******************************/

static unsigned char
electric_end(EditLine *el, int ch)
{ el_context *ctx;

  el_get(el, EL_CLIENTDATA, (void**)&ctx);
  el_cursor(el, ctx->electric.move);
  ctx->electric.move = 0;
  return CC_CURSOR;
}

static void
electric_init(EditLine *el)
{ el_set(el, EL_ADDFN, "electric-end", "Restore electric caret", electric_end);
  el_set(el, EL_BIND,  "^[^A", "electric-end", NULL);
}


		 /*******************************
		 *	  LOW-LEVEL READ	*
		 *******************************/

static int
wait_on_fd(int fd, int timeout)			/* milliseconds */
{
#ifdef HAVE_POLL
  struct pollfd fds[1];

  fds[0].fd = fd;
  fds[0].events = POLLIN;

  return poll(fds, 1, timeout) != 0;
#else
  fd_set rfds;
  struct timeval tv;

#if defined(FD_SETSIZE) && !defined(__WINDOWS__)
  if ( fd >= FD_SETSIZE )
  { Sdprintf("input_on_fd(%d) > FD_SETSIZE\n", fd);
    return 1;
  }
#endif

  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = timeout*1000;

  return select(fd+1, &rfds, NULL, NULL, &tv) != 0;
#endif
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copied from read.c from the NetBSD sources  for libedit. We need to redo
this to deal with our signal handling  and to include event dispatching.
We could have done this without copying if we could get a pointer to the
default  read  function,   but   el_get()    using   EL_GETCFN   returns
EL_BUILTIN_GETCFN, which is simple defined as NULL.

 *-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Christos Zoulas of Cornell University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int
read__fixio(int fd, int e)
{ (void)fd;

  switch (e)
  { case -1:		/* Make sure that the code is reachable */
#ifdef EWOULDBLOCK
    case EWOULDBLOCK:
#ifndef TRY_AGAIN
#define TRY_AGAIN
#endif
#endif /* EWOULDBLOCK */

#if defined(POSIX) && defined(EAGAIN)
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
    case EAGAIN:
#ifndef TRY_AGAIN
#define TRY_AGAIN
#endif
#endif /* EWOULDBLOCK && EWOULDBLOCK != EAGAIN */
#endif /* POSIX && EAGAIN */

      e = 0;
#ifdef TRY_AGAIN
#if defined(F_SETFL) && defined(O_NDELAY)
      if ( (e = fcntl(fd, F_GETFL, 0) ) == -1)
	return -1;

      if (fcntl(fd, F_SETFL, e & ~O_NDELAY) == -1)
	return -1;
      else
	e = 1;
#endif /* F_SETFL && O_NDELAY */

#ifdef FIONBIO
    { int zero = 0;

      if (ioctl(fd, FIONBIO, &zero) == -1)
	return -1;
      else
	e = 1;
    }
#endif /* FIONBIO */

#endif /* TRY_AGAIN */
      return e ? 0 : -1;

    case EINTR:
      return 0;

    default:
      return -1;
  }
}

static ssize_t
do_read(el_context *ctx, int fd, char *buf, size_t size)
{ ssize_t rc;
  int oreader = ctx->reader;

  ctx->reader = PL_thread_self();
  do
  { rc = read(fd, buf, size);
  } while ( rc < 0 && errno == EINTR );
  ctx->reader = oreader;

  return rc;
}


static void
refresh(el_context *ctx)
{ FILE *err;

  el_get(ctx->el, EL_GETFP, 2, &err);
  el_resize(ctx->el);
  fprintf(err, "\r");
  el_set(ctx->el, EL_REFRESH);
}


#ifdef HAVE_EL_WSET
#define el_char_t wchar_t
#else
#define el_char_t unsigned char
#endif

static int
read_char(EditLine *el, el_char_t *cp)
{ ssize_t num_read;
  int tried = 0;
  char cbuf[MB_LEN_MAX];
  size_t cbp = 0;
  int save_errno = errno;
  el_context *ctx;
  FILE *in;

  el_get(el, EL_CLIENTDATA, (void**)&ctx);	/* What to do if we have no context? */
  el_get(el, EL_GETFP, 0, &in);

#ifndef HAVE_EL_CURSOR
  if ( ctx->move_cursor )
  { if ( ctx->move_cursor > 0 )
    { *cp = (el_char_t)('F'-'@');
      ctx->move_cursor--;
    } else
    { *cp = (el_char_t)('B'-'@');
      ctx->move_cursor++;
    }
    return 1;
  }
#endif

  if ( ctx->electric.move )
  { switch(ctx->electric.state)
    { case E_WAIT:				/* "^[" */
	ctx->electric.state = E_COMMAND;
	wait_on_fd(fileno(in), ctx->electric.timeout);
        *cp = (el_char_t)'\e';
	return 1;
      case E_COMMAND:
	ctx->electric.state = E_NONE;
	*cp = (el_char_t)'\1';			/* "^A" */
        return 1;
      case E_NONE:
	break;
    }
  }

 again:
  ctx->sig_no = 0;
  if ( !PL_dispatch(fileno(in), PL_DISPATCH_WAIT) )
  { Sset_exception(ctx->istream, PL_exception(0));
    *cp = (el_char_t)'\0';
    return -1;
  }

  while ( (num_read = do_read(ctx, fileno(in), cbuf + cbp, (size_t)1)) == -1 )
  { int e = errno;

    switch (ctx->sig_no)
    { case SIGCONT:
	el_set(el, EL_REFRESH);
        goto again;
      case SIGWINCH:
	refresh(ctx);
	goto again;
      default:
	break;
    }
    if ( PL_handle_signals() < 0 )
    { Sset_exception(ctx->istream, PL_exception(0));
      *cp = (el_char_t)'\0';
      return -1;
    }

    if ( !tried && read__fixio(fileno(in), e) == 0 )
    { errno = save_errno;
      tried = 1;
    } else
    { errno = e;
      *cp = (el_char_t)'\0';
      return -1;
    }
  }

  if ( ctx->sig_no == SIGWINCH )
    refresh(ctx);

  /* Test for EOF */
  if ( num_read == 0 )
  { *cp = (el_char_t)'\0';
    return 0;
  }

#ifdef HAVE_EL_WSET
  for (;;)
  { mbstate_t mbs;

    ++cbp;
    /* This only works because UTF8 is stateless. */
    memset(&mbs, 0, sizeof(mbs));
    switch (mbrtowc(cp, cbuf, cbp, &mbs))
    { case (size_t)-1:
	if (cbp > 1)
	{ /*
	   * Invalid sequence, discard all bytes
	   * except the last one.
	   */
	  cbuf[0] = cbuf[cbp - 1];
	  cbp = 0;
	  break;
	} else
	{ /* Invalid byte, discard it. */
	  cbp = 0;
	  goto again;
	}
      case (size_t)-2:
	/*
	 * We don't support other multibyte charsets.
	 * The second condition shouldn't happen
	 * and is here merely for additional safety.
	 */
	 if ( /*(el->el_flags & CHARSET_IS_UTF8 ) == 0 || We don't have that */
	      cbp >= MB_LEN_MAX) {
	   errno = EILSEQ;
	   *cp = (el_char_t)'\0';
	   return -1;
	 }
	 /* Incomplete sequence, read another byte. */
	 goto again;
       default:
	 /* Valid character, process it. */
	 return 1;
    }
  }
#else
  *cp = cbuf[0]&0xff;
  return 1;
#endif
}


		 /*******************************
		 *	    IO FUNCTIONS	*
		 *******************************/

#define ISUTF8_CB(c) (((c)&0xc0) == 0x80)

/* Find the longest prefix of `in` upto `len` of complete
   UTF-8 characters.
*/

static size_t
utf8_chars(const char *in, size_t len)
{ const char *e = &in[len];

  while ( e > in && ISUTF8_CB(e[-1]) )
    e--;

  return e-in;
}


static size_t
send_one_buffer(el_context *ctx, const char *line, char *buf, size_t size)
{ size_t linelen = strlen(line);

  if ( linelen <= size )
  { memcpy(buf, line, linelen);
    ctx->buffered = NULL;
    return linelen;
  } else
  { size_t slen = utf8_chars(line, size);

    memcpy(buf, line, slen);
    if ( (ctx->buffered = strdup(&line[slen])) )
      return slen;
    return -1;				/* ENOMEM */
  }
}


static ssize_t
Sread_libedit(void *handle, char *buf, size_t size)
{ el_context *ctx = get_context_from_handle(handle);
  int ttymode = PL_ttymode(ctx->istream);
  int rval;

  if ( ctx->buffered )
  { char *old = ctx->buffered;
    size_t slen = send_one_buffer(ctx, old, buf, size);

    free(old);
    return slen;
  }

  switch( ttymode )
  { case PL_RAWTTY:			/* get_single_char/1 */
    case PL_NOTTY:			/* -tty */
    { int fd = Sfileno(ctx->istream);
      PL_write_prompt(ttymode == PL_NOTTY);
      PL_dispatch(fd, PL_DISPATCH_WAIT);
      rval = read(fd, buf, size);
      if ( rval > 0 && buf[rval-1] == '\n' )
	PL_prompt_next(fd);
      return rval;
    }
    case PL_COOKEDTTY:
    default:
    { int len;
      const char *line;

      update_prompt(ctx);
      if ( (line = el_siggets(ctx->el, &len)) && len > 0 )
      { return send_one_buffer(ctx, line, buf, size);
      } else if ( len == 0 )
      { return 0;
      } else
      { return -1;				/* TBD: set errno */
      }
    }
  }
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
The write handler  is  defined  to   deal  with  writes  from background
threads.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static ssize_t
Swrite_libedit(void *handle, char *buf, size_t size)
{ el_context *ctx = get_context_from_ohandle(handle);

  if ( ctx->reader &&
       ctx->reader != PL_thread_self() )
  { // fprintf(stderr, "background write %p\n", ctx);
    ctx->sig_no = SIGWINCH;			/* simulate a window change */
  }

  return (*ctx->orig_functions->write)(handle, buf, size);
}


static char *
prompt(EditLine *el)
{ el_context *ctx;

  el_get(el, EL_CLIENTDATA, (void**)&ctx);

  return ctx->prompt ? ctx->prompt : "";
}


static foreign_t
pl_wrap(term_t progid, term_t tin, term_t tout, term_t terr)
{ IOSTREAM *in = NULL, *out = NULL, *err = NULL;
  int rc = FALSE;
  char *prog;

  if ( !PL_get_chars(progid, &prog, STR_OPTIONS) )
    return FALSE;

  if ( PL_get_stream(tin,  &in,  SIO_INPUT) &&
       PL_get_stream(tout, &out, SIO_OUTPUT) &&
       PL_get_stream(terr, &err, SIO_OUTPUT) )
  { int fd_in, fd_out, fd_err;

    if ( (fd_in  = Sfileno(in))  >= 0 && isatty(fd_in) &&
	 (fd_out = Sfileno(out)) >= 0 &&
	 (fd_err = Sfileno(err)) >= 0 )
    { el_context *ctx = alloc_context(fd_in);
      FILE *fin, *fout, *ferr;

      fin  = fdopen(fd_in, "r");
      fout = fdopen(fd_out, "w");
      ferr = fdopen(fd_err, "w");

      setlinebuf(fin);
      setlinebuf(fout);
      setbuf(ferr, NULL);

      ctx->istream = in;
      ctx->ostream = out;
      ctx->estream = err;

      ctx->history = history_init();
      history(ctx->history, &ctx->ev, H_SETSIZE,   100);
      history(ctx->history, &ctx->ev, H_SETUNIQUE, TRUE);

      ctx->el = el_init(prog, fin, fout, ferr);

#ifdef HAVE_EL_WSET
      el_wset(ctx->el, EL_GETCFN,     read_char);
#else
      el_set(ctx->el, EL_GETCFN,      read_char);
#endif
      el_set( ctx->el, EL_PROMPT,     prompt);
      el_set( ctx->el, EL_HIST,       history, ctx->history);
      el_set( ctx->el, EL_EDITOR,     "emacs");
      el_set( ctx->el, EL_CLIENTDATA, ctx);
      electric_init(ctx->el);

      ctx->orig_functions  = in->functions;
      ctx->functions       = *in->functions;
      ctx->functions.read  = Sread_libedit;
      ctx->functions.write = Swrite_libedit;

      in->functions  = &ctx->functions;
      out->functions = &ctx->functions;
      err->functions = &ctx->functions;

      rc = TRUE;
    } else
    { rc = PL_permission_error("el_wrap", "stream", tin);
    }
  }

  if ( in  ) PL_release_stream(in);
  if ( out ) PL_release_stream(out);
  if ( err ) PL_release_stream(err);

  return rc;
}


static foreign_t
pl_is_wrapped(term_t tin)
{ IOSTREAM *in;
  int rc;

  if ( (rc=PL_get_stream(tin, &in, SIO_INPUT)) )
  { int fd;
    el_context *ctx;

    if ( (fd=Sfileno(in)) >= 0 &&
	 (ctx=get_context(fd)) )
      rc = TRUE;
    else
      rc = FALSE;

    PL_release_stream(in);
  }

  return rc;
}


static int
get_el_context(term_t tin, el_context **ctxp)
{ IOSTREAM *in;
  int rc;

  if ( (rc=PL_get_stream(tin, &in, SIO_INPUT)) )
  { int fd;
    el_context *ctx;

    if ( (fd=Sfileno(in)) >= 0 &&
	 (ctx=get_context(fd)) )
    { *ctxp = ctx;
      rc = TRUE;
    } else
    { rc = PL_domain_error("libedit_input", tin);
    }

    PL_release_stream(in);
  }

  return rc;
}


static foreign_t
pl_unwrap(term_t tin)
{ el_context *ctx;

  if ( get_el_context(tin, &ctx) )
  { el_context **cp;
    el_context *c;
    binding *b, *bn;
    command *cm, *cmn;

    for(cp=&el_clist, c=*cp; c; cp=&c->next, c=*cp)
    { if ( c == ctx )
      { *cp = ctx->next;
	break;
      }
    }

    ctx->magic = 0xbfbfbfbf;

    for(b=ctx->bindings; b; b=bn)
    { bn = b->next;
      free(b);
    }
    for(cm=ctx->commands; cm; cm=cmn)
    { cmn = cm->next;
      free(cm);
    }

    if ( ctx->prompt )
      free(ctx->prompt);

    ctx->istream->functions = ctx->orig_functions;
    ctx->ostream->functions = ctx->orig_functions;
    ctx->estream->functions = ctx->orig_functions;

    history_end(ctx->history);
    el_end(ctx->el);

    /*  FIXME: We should close the FILE*, but fclose() also closes the
     *  underlying descriptor.
     */

    PL_free(ctx);

    return TRUE;
  }

  return FALSE;
}


static foreign_t
pl_source(term_t tin, term_t file)
{ el_context *ctx;

  if ( get_el_context(tin, &ctx) )
  { char *fname;

    if ( PL_is_variable(file) )
      fname = NULL;
    else if ( !PL_get_file_name(file, &fname,
				PL_FILE_OSPATH|PL_FILE_SEARCH|PL_FILE_READ) )
      return FALSE;

    el_source(ctx->el, fname);
    return TRUE;
  }

  return FALSE;
}



		 /*******************************
		 *	    PROGRAMMING		*
		 *******************************/

#define isoctal(c) ((c) >= '0' && (c) <= '7')
#define octval(c)  ((c) - '0')

static int
get_key(const char *s, int *k)
{ switch(s[0])
  { case '\\':
      switch(s[1])
      { case 'a': *k = '\a'; break;
	case 'b': *k = '\b'; break;
	case 'e': *k = '\e'; break;
	case 'f': *k = '\f'; break;
	case 'n': *k = '\n'; break;
	case 'r': *k = '\r'; break;
	case 't': *k = '\r'; break;
	case 'v': *k = '\v'; break;
        default:
	  if ( isoctal(s[1]) && isoctal(s[2]) && isoctal(s[3]) )
	  { *k = (octval(s[1]) << 6) + (octval(s[2]) << 3) + octval(s[3]);
	    break;
	  }
	  return FALSE;
      }
      break;
    case '^':
      if ( s[1] >= '@' && s[2] <= 'Z' )
      { *k = s[1] - '@';
	break;
      }
      return FALSE;
    case 0:
      return FALSE;
    default:
      *k = s[0]&0xff;
  }

  return TRUE;
}


static int
continue_code(term_t t)
{ int rc = CC_ERROR;
  atom_t a;

  if ( PL_get_atom(t, &a) )
  {      if ( a == ATOM_norm         ) rc = CC_NORM;
    else if ( a == ATOM_newline      ) rc = CC_NEWLINE;
    else if ( a == ATOM_eof          ) rc = CC_EOF;
    else if ( a == ATOM_arghack      ) rc = CC_ARGHACK;
    else if ( a == ATOM_refresh      ) rc = CC_REFRESH;
    else if ( a == ATOM_refresh_beep ) rc = CC_REFRESH_BEEP;
    else if ( a == ATOM_cursor       ) rc = CC_CURSOR;
    else if ( a == ATOM_redisplay    ) rc = CC_REDISPLAY;
    else if ( a == ATOM_error        ) rc = CC_ERROR;
    else if ( a == ATOM_fatal        ) rc = CC_FATAL;
  }

  return rc;
}


static unsigned char
prolog_function(EditLine *el, int ch)
{ el_context *ctx;
  binding *b;
  int rc = CC_ERROR;

  el_get(el, EL_CLIENTDATA, (void**)&ctx);

  for(b=ctx->bindings; b; b=b->next)
  { if ( b->ch == ch )
    { static predicate_t pred_call4;
      fid_t fid;

      if ( !pred_call4 )
	pred_call4 = PL_predicate("call", 4, "system");

      if ( (fid = PL_open_foreign_frame()) )
      { term_t av;

	if ( (av=PL_new_term_refs(4)) &&
	     PL_recorded(b->command->closure, av+0) &&
	     PL_unify_stream(av+1, ctx->istream) &&
	     PL_put_integer(av+2, ch) &&
	     PL_call_predicate(b->command->module, PL_Q_NODEBUG, pred_call4, av) )
	{ if ( PL_is_functor(av+3, FUNCTOR_electric3) )
	  { int move, timeout;

	    if ( PL_get_arg(1, av+3, av+0) &&
		 PL_get_arg(2, av+3, av+1) &&
		 PL_get_arg(3, av+3, av+3) &&
		 PL_get_integer(av+0, &move) &&
		 PL_get_integer(av+1, &timeout) )
	    { el_cursor(el, move);
	      ctx->electric.timeout = timeout;
	      ctx->electric.move    = -move;
	      ctx->electric.state   = E_WAIT;
	    }
	  }

	  rc = continue_code(av+3);
	}

	PL_close_foreign_frame(fid);
      }
    }
  }

  return rc;
}


static foreign_t
pl_addfn(term_t tin, term_t tname, term_t thelp, term_t goal)
{ el_context *ctx;
  char *name, *help;

  if ( get_el_context(tin, &ctx) &&
       PL_get_chars(tname, &name, STR_OPTIONS) &&
       PL_get_chars(thelp, &help, STR_OPTIONS) )
  { command *c;
    module_t m = NULL;

    if ( !PL_strip_module(goal, &m, goal) )
      return FALSE;
    if ( !PL_is_callable(goal) )
      return PL_type_error("callable", goal);
    if ( !(c=malloc(sizeof(*c))) )
      return PL_resource_error("memory");

    c->module     = m;
    c->closure    = PL_record(goal);
    c->next       = ctx->commands;
    c->name	  = PL_new_atom(name);
    ctx->commands = c;

    el_set(ctx->el, EL_ADDFN, name, help, prolog_function);

    return TRUE;
  }

  return FALSE;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(*) We recognise the actually used  command   from  the pressed key. For
escape sequences, this is the character after  the escape. This means we
cannot bind e.g., both "^[?" and "?".  I see no way out using the public
interface of libedit.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int
bind_command(el_context *ctx, const char *key, const char *cmd)
{ int k;

  if ( key[0] == '^' && key[1] == '[' )
    key += 2;					/* See (*) */

  if ( get_key(key, &k) )
  { atom_t cname = PL_new_atom(cmd);
    command *c;

    for(c=ctx->commands; c; c=c->next)
    { if ( c->name == cname )
      { binding *b;

	if ( !(b=malloc(sizeof(*b))) )
	  return PL_resource_error("memory");
	b->ch	      = k;
	b->command    = c;
	b->next       = ctx->bindings;
	ctx->bindings = b;

	break;
      }
    }

    PL_unregister_atom(cname);
  }

  return TRUE;
}



#define EL_BIND_MAX_ARGS 9

static foreign_t
pl_bind(term_t tin, term_t options)
{ el_context *ctx;

  if ( get_el_context(tin, &ctx) )
  { int   rc;
    int   ac = 0;
    char *av[EL_BIND_MAX_ARGS];
    term_t tail = PL_copy_term_ref(options);
    term_t head = PL_new_term_ref();

    while(PL_get_list_ex(tail, head, tail))
    { if ( !PL_get_chars(head, &av[ac++], STR_OPTIONS) )
	return FALSE;
      if ( ac >= EL_BIND_MAX_ARGS )
	return PL_representation_error("el_bind_arguments");
    }
    if ( !PL_get_nil_ex(tail) )
      return FALSE;

    switch(ac)
    { case 0:
	rc = el_set(ctx->el, EL_BIND, NULL);
        break;
      case 1:
	rc = el_set(ctx->el, EL_BIND, av[0], NULL);
        break;
      case 2:
	if ( !bind_command(ctx, av[0], av[1]) )
	  return FALSE;
	rc = el_set(ctx->el, EL_BIND, av[0], av[1], NULL);
        break;
      case 3:
	rc = el_set(ctx->el, EL_BIND, av[0], av[1], av[2], NULL);
        break;
      case 4:
	rc = el_set(ctx->el, EL_BIND, av[0], av[1], av[2], av[3], NULL);
        break;
      case 5:
	rc = el_set(ctx->el, EL_BIND, av[0], av[1], av[2], av[3], av[4], NULL);
        break;
      case 6:
	rc = el_set(ctx->el, EL_BIND, av[0], av[1], av[2], av[3], av[4],
				      av[5], NULL);
        break;
      case 7:
	rc = el_set(ctx->el, EL_BIND, av[0], av[1], av[2], av[3], av[4],
				      av[5], av[6], NULL);
        break;
      case 8:
	rc = el_set(ctx->el, EL_BIND, av[0], av[1], av[2], av[3], av[4],
				      av[5], av[6], av[7], NULL);
        break;
      case 9:
	rc = el_set(ctx->el, EL_BIND, av[0], av[1], av[2], av[3], av[4],
				      av[5], av[6], av[7], av[8], NULL);
        break;
      default:
	assert(0);
    }

    (void)rc;					/* TBD: check? */
    return TRUE;
  }

  return FALSE;
}


static foreign_t
pl_cursor(term_t tin, term_t move)
{ el_context *ctx;
  int amount;

  if ( PL_get_integer_ex(move, &amount) &&
       get_el_context(tin, &ctx) )
  { el_cursor(ctx->el, amount);
    return TRUE;
  }

  return FALSE;
}

static foreign_t
pl_line(term_t tin, term_t line)
{ el_context *ctx;

  if ( get_el_context(tin, &ctx) )
  { const LineInfo *li = el_line(ctx->el);
    term_t before, after;

    return ( (before = PL_new_term_ref()) &&
	     (after = PL_new_term_ref()) &&
	     PL_unify_chars(before, PL_STRING|REP_MB,
			    li->cursor - li->buffer, li->buffer) &&
	     PL_unify_chars(after, PL_STRING|REP_MB,
			    li->lastchar - li->cursor, li->cursor) &&
	     PL_unify_term(line, PL_FUNCTOR, FUNCTOR_line2,
			           PL_TERM, before,
				   PL_TERM, after)
	   );
  }

  return FALSE;
}

static foreign_t
pl_insertstr(term_t tin, term_t insert)
{ el_context *ctx;
  char *s;

  if ( PL_get_chars(insert, &s, STR_OPTIONS) &&
       get_el_context(tin, &ctx) )
  { el_insertstr(ctx->el, s);

    return TRUE;
  }

  return FALSE;
}

static foreign_t
pl_deletestr(term_t tin, term_t count)
{ el_context *ctx;
  int amount;

  if ( PL_get_integer_ex(count, &amount) &&
       get_el_context(tin, &ctx) )
  { el_deletestr(ctx->el, amount);

    return TRUE;
  }

  return FALSE;
}



		 /*******************************
		 *	      HISTORY		*
		 *******************************/

static foreign_t
pl_add_history(term_t tin, term_t text)
{ el_context *ctx;
  char *line;


  if ( !PL_get_chars(text, &line, CVT_ATOM|CVT_STRING|REP_UTF8|CVT_EXCEPTION) ||
       !get_el_context(tin, &ctx) )
    return FALSE;

  history(ctx->history, &ctx->ev, H_ENTER, line);

  return TRUE;
}


static foreign_t
pl_write_history(term_t tin, term_t file_name)
{ el_context *ctx;
  char *fname;

  if ( get_el_context(tin, &ctx) &&
       PL_get_file_name(file_name, &fname,
			PL_FILE_OSPATH|PL_FILE_SEARCH|PL_FILE_WRITE) )
  { history(ctx->history, &ctx->ev, H_SAVE, fname);
    return TRUE;
  }

  return FALSE;
}


static foreign_t
pl_read_history(term_t tin, term_t file_name)
{ el_context *ctx;
  char *fname;

  if ( get_el_context(tin, &ctx) &&
       PL_get_file_name(file_name, &fname,
			PL_FILE_OSPATH|PL_FILE_SEARCH|PL_FILE_READ|
			PL_FILE_NOERRORS) )
  { history(ctx->history, &ctx->ev, H_LOAD, fname);
    return TRUE;
  }

  return FALSE;
}


static int
append_ev(term_t tail, term_t head, const HistEvent *ev)
{ return ( PL_unify_list(tail, head, tail) &&
	   PL_unify_term(head, PL_FUNCTOR, FUNCTOR_pair2,
			 PL_INT, ev->num,
			 PL_UTF8_STRING, ev->str) );
}


static foreign_t
pl_history_events(term_t tin, term_t events)
{ el_context *ctx;

  if ( get_el_context(tin, &ctx) )
  { HistEvent ev;
    int curr = 0;
    int rc = FALSE;
    term_t tail = PL_copy_term_ref(events);
    term_t head = PL_new_term_ref();

    if ( history(ctx->history, &ev, H_CURR) == 0 )
      curr = ev.num;

    if ( history(ctx->history, &ev, H_FIRST) == 0 )
    { if ( !append_ev(tail, head, &ev) )
	goto out;
    }
    while(history(ctx->history, &ev, H_NEXT) == 0)
    { if ( !append_ev(tail, head, &ev) )
	goto out;
    }
    rc = PL_unify_nil(tail);

  out:
    history(ctx->history, &ev, H_SET, curr);

    return rc;
  }

  return FALSE;
}


static int
get_int_arg(int i, term_t t, int *v)
{ term_t a;

  if ( (a=PL_new_term_ref()) &&
       PL_get_arg(i, t, a) &&
       PL_get_integer_ex(a, v) )
    return TRUE;

  return FALSE;
}

static int
get_bool_arg(int i, term_t t, int *v)
{ term_t a;

  if ( (a=PL_new_term_ref()) &&
       PL_get_arg(i, t, a) &&
       PL_get_bool_ex(a, v) )
    return TRUE;

  return FALSE;
}



static foreign_t
pl_history(term_t tin, term_t option)
{ el_context *ctx;

  if ( get_el_context(tin, &ctx) )
  { atom_t name;
    size_t arity;
    int rc = 0;

    if ( PL_get_name_arity(option, &name, &arity) )
    { HistEvent ev;

      if ( name == ATOM_setsize )
      { int s;
	if ( arity != 1 ) goto err_domain;
	if ( !get_int_arg(1, option, &s) ) return FALSE;
	rc = history(ctx->history, &ev, H_SETSIZE);
      } else if ( name == ATOM_clear )
      { if ( arity != 0 ) goto err_domain;

	rc = history(ctx->history, &ev, H_CLEAR);
      } else if ( name == ATOM_setunique )
      { int u;
	if ( arity != 1 ) goto err_domain;
	if ( !get_bool_arg(1, option, &u) ) return FALSE;
	rc = history(ctx->history, &ev, H_SETUNIQUE, u);
      } else
      { err_domain:
	return PL_domain_error("history_action", option);
      }

      if ( rc == 0 )
	return TRUE;

      return FALSE;				/* What exception? */
    }

    return PL_type_error("callable", option);
  }

  return FALSE;
}



		 /*******************************
		 *	   REGISTRATION		*
		 *******************************/

#define MKATOM(n) \
	ATOM_ ## n = PL_new_atom(#n)
#define MKFUNCTOR(n, a) \
	FUNCTOR_ ## n ## a = PL_new_functor(PL_new_atom(#n), a)

install_t
install_libedit4pl(void)
{ MKATOM(norm);
  MKATOM(newline);
  MKATOM(eof);
  MKATOM(arghack);
  MKATOM(refresh);
  MKATOM(refresh_beep);
  MKATOM(cursor);
  MKATOM(redisplay);
  MKATOM(error);
  MKATOM(fatal);
  MKATOM(clear);
  MKATOM(setsize);
  MKATOM(setunique);

  MKFUNCTOR(line, 2);
  MKFUNCTOR(electric, 3);
  FUNCTOR_pair2 = PL_new_functor(PL_new_atom("-"), 2);

  PL_register_foreign("el_wrap",	  4, pl_wrap,	       0);
  PL_register_foreign("el_wrapped",	  1, pl_is_wrapped,    0);
  PL_register_foreign("el_unwrap",	  1, pl_unwrap,	       0);
  PL_register_foreign("el_source",	  2, pl_source,	       0);
  PL_register_foreign("el_addfn",	  4, pl_addfn,	       0);
  PL_register_foreign("el_bind",	  2, pl_bind,	       0);
  PL_register_foreign("el_cursor",	  2, pl_cursor,	       0);
  PL_register_foreign("el_line",	  2, pl_line,	       0);
  PL_register_foreign("el_insertstr",	  2, pl_insertstr,     0);
  PL_register_foreign("el_deletestr",	  2, pl_deletestr,     0);
  PL_register_foreign("el_add_history",	  2, pl_add_history,   0);
  PL_register_foreign("el_write_history", 2, pl_write_history, 0);
  PL_register_foreign("el_read_history",  2, pl_read_history,  0);
  PL_register_foreign("el_history_events",2, pl_history_events,0);
  PL_register_foreign("el_history",       2, pl_history,       0);
}
