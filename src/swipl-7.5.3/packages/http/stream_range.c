/*  Part of SWI-Prolog

    Author:        Jan Wielemaker
    E-mail:        J.Wielemaker@vu.nl
    WWW:           http://www.swi-prolog.org
    Copyright (c)  2007-2015, University of Amsterdam,
			      VU University Amsterdam
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <errno.h>

static atom_t ATOM_size;		/* size(Int) */
static atom_t ATOM_onclose;		/* onclose(:CallBack) */

		 /*******************************
		 *	       TYPES		*
		 *******************************/

#define BUFSIZE SIO_BUFSIZE		/* raw I/O buffer */

typedef struct range_context
{ IOSTREAM	   *stream;		/* Original stream */
  IOSTREAM	   *range_stream;	/* Stream I'm handle of */
  IOENC		    parent_encoding;	/* Saved encoding of parent */
  size_t	    read;		/* data already read */
  size_t	    size;		/* #bytes of data available */
  module_t	    context;		/* onclose context module */
  record_t	    onclose;		/* Call-back when closed */
} range_context;


static range_context*
alloc_range_context(IOSTREAM *s)
{ range_context *ctx = PL_malloc(sizeof(*ctx));

  memset(ctx, 0, sizeof(*ctx));
  ctx->stream = s;

  return ctx;
}


static void
free_range_context(range_context *ctx)
{ if ( ctx->stream->upstream )
    Sset_filter(ctx->stream, NULL);
  else
    PL_release_stream(ctx->stream);

  if ( ctx->onclose )
  { PL_erase(ctx->onclose);
    ctx->onclose = 0;
  }
  PL_free(ctx);
}


		 /*******************************
		 *	RANGE LIMITED INPUT	*
		 *******************************/

static ssize_t				/* range-limited read */
range_read(void *handle, char *buf, size_t size)
{ range_context *ctx = handle;
  size_t max_rd;
  ssize_t rd;

  if ( ctx->read == ctx->size )
    return 0;

  if ( ctx->size - ctx->read < size )
    max_rd = ctx->size - ctx->read;
  else
    max_rd = size;

  if ( (rd = Sfread(buf, sizeof(char), max_rd, ctx->stream)) >= 0 )
  { ctx->read += rd;

    return rd;
  }

  return rd;
}


static ssize_t				/* no writing! */
range_write(void *handle, char *buf, size_t size)
{ return -1;
}


static int
range_control(void *handle, int op, void *data)
{ range_context *ctx = handle;

  switch(op)
  { case SIO_FLUSHOUTPUT:
    case SIO_SETENCODING:
      return 0;				/* allow switching encoding */
    case SIO_GETSIZE:
    { int64_t *rval = data;
      *rval = ctx->size;
      return 0;
    }
    default:
      if ( ctx->stream->functions->control )
	return (*ctx->stream->functions->control)(ctx->stream->handle, op, data);
      return -1;
  }
}


static int
range_close(void *handle)
{ int rc = 0;
  range_context *ctx = handle;

  ctx->stream->encoding = ctx->parent_encoding;
  if ( ctx->onclose )
  { static predicate_t call3 = 0;
    fid_t fid;
    term_t av;
    size_t left = ctx->size - ctx->read;

    if ( !call3 )
      call3 = PL_predicate("call", 3, "system");
    if ( (fid=PL_open_foreign_frame()) &&
	 (av=PL_new_term_refs(3)) &&
	 PL_recorded(ctx->onclose, av+0) &&
	 PL_unify_stream(av+1, ctx->stream) &&
	 PL_put_int64(av+2, (int64_t)left)
       )
    { term_t ex;

      if ( PL_call_predicate(ctx->context, PL_Q_PASS_EXCEPTION, call3, av) )
      { rc = 0;
      } else if ( (ex=PL_exception(0)) )
      { Sset_exception(ctx->stream, ex);
	rc = -1;
      } else
      { Sseterr(ctx->stream, SIO_FERR, "onclose hook failed");
	rc = -1;
      }
    }
    if ( fid )
      PL_close_foreign_frame(fid);
  }

  free_range_context(ctx);

  return rc;
}


static IOFUNCTIONS range_functions =
{ range_read,
  range_write,
  NULL,					/* seek */
  range_close,
  range_control,			/* control */
  NULL,					/* seek64 */
};


		 /*******************************
		 *	 PROLOG CONNECTION	*
		 *******************************/

#define COPY_FLAGS (SIO_INPUT|SIO_OUTPUT| \
		    SIO_TEXT| \
		    SIO_REPXML|SIO_REPPL|\
		    SIO_RECORDPOS)

static foreign_t
pl_stream_range_open(term_t org, term_t new, term_t options)
{ term_t tail = PL_copy_term_ref(options);
  term_t head = PL_new_term_ref();
  range_context *ctx;
  IOSTREAM *s, *s2;
  int size = 0;
  module_t ocm = NULL;
  record_t onclose = 0;

  while(PL_get_list(tail, head, tail))
  { atom_t name;
    size_t arity;
    term_t arg = PL_new_term_ref();

    if ( !PL_get_name_arity(head, &name, &arity) || arity != 1 )
      return type_error(head, "option");
    _PL_get_arg(1, head, arg);

    if ( name == ATOM_size )
    { if ( !get_int_ex(arg, &size) )
	return FALSE;
      if ( size <= 0 )
	return domain_error(arg, "nonneg");
    } else if ( name == ATOM_onclose )
    { if ( !PL_strip_module(arg, &ocm, arg) )
	return FALSE;
      onclose = PL_record(arg);
    }
  }
  if ( !PL_get_nil(tail) )
    return type_error(tail, "list");

  if ( !PL_get_stream_handle(org, &s) )
    return FALSE;			/* Error */
  ctx = alloc_range_context(s);
  ctx->size = size;
  ctx->onclose = onclose;
  ctx->context = ocm;

  if ( !(s2 = Snew(ctx,
		   (s->flags&COPY_FLAGS)|SIO_FBUF,
		   &range_functions))	)
  { free_range_context(ctx);			/* no memory */

    return FALSE;
  }

  s2->encoding = s->encoding;
  ctx->parent_encoding = s->encoding;
  s->encoding = ENC_OCTET;
  ctx->range_stream = s2;
  if ( PL_unify_stream(new, s2) )
  { Sset_filter(s, s2);
    PL_release_stream(s);

    return TRUE;
  } else
  { return instantiation_error();
  }
}


		 /*******************************
		 *	       INSTALL		*
		 *******************************/

static void
install_stream_range()
{ ATOM_size    = PL_new_atom("size");
  ATOM_onclose = PL_new_atom("onclose");

  PL_register_foreign("stream_range_open",  3, pl_stream_range_open,  0);
}
