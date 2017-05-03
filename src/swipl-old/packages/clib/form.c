/*  Part of SWI-Prolog

    Author:        Jan Wielemaker
    E-mail:        J.Wielemaker@vu.nl
    WWW:           http://www.swi-prolog.org
    Copyright (c)  2000-2014, University of Amsterdam
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <SWI-Prolog.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <assert.h>
#include "form.h"
#ifdef __WINDOWS__
#include <io.h>
#endif

#include "error.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Breaks a string holding data from a WWW form into its values.  Outputs a
sequence of NAME=VALUE commands for a shell.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int
dehex(int chr)
{ chr &= 0xff;

  if ( chr >= '0' && chr <= '9' )
    return chr - '0';
  if ( chr >= 'A' && chr <= 'F' )
    return chr - 'A' + 10;
  if ( chr >= 'a' && chr <= 'f' )
    return chr - 'a' + 10;

  return -1;
}


static size_t
form_argument_decode(const char *in, size_t inlen, char *out, size_t outlen)
{ const char *ein  = in+inlen;
  size_t written = 0;

  for(; in < ein; in++)
  { switch(*in)
    { case '+':
	if ( ++written < outlen )
	  *out++ = ' ';
        break;
      case '%':
	if ( in+2 < ein )
	{ int h1 = dehex(*(++in));
	  int h2 = dehex(*(++in));

	  if ( h1 < 0 || h2 < 0 )
	    return (size_t)-1;

	  if ( ++written < outlen )
	    *out++ = h1<<4|h2;
	} else
	  return (size_t)-1;		/* syntax error */
	break;
      default:
	if ( ++written < outlen )
	  *out++ = *in;
        break;
    }
  }

  if ( written < outlen )
    *out++ = '\0';

  return written;
}


#define SHORTVALUE 512

int
break_form_argument(const char *formdata,
		    int (*func)(const char* name,
				size_t namelen,
				const char *value,
				size_t valuelen,
				void *closure),
		    void *closure)
{ while ( *formdata )
  { char value[SHORTVALUE];
    char *eq = strchr(formdata, '=');

    if ( eq )
    { size_t nlen = eq-formdata;
      char *end;
      size_t vlen;

      eq++;
      end = strchr(eq, '&');
      if ( !end )
	end = eq+strlen(eq);		/* end of the string */

      if ( (vlen=form_argument_decode(eq, end-eq, value, SHORTVALUE)) >= SHORTVALUE )
      { char *buf;

	if ( (buf=malloc(vlen+1)) )
	{ size_t vlen2 = form_argument_decode(eq, end-eq, buf, vlen+1);
	  int rc;

	  assert(vlen2 == vlen);
	  rc = (func)(formdata, nlen, buf, vlen2, closure);
	  free(buf);

	  if ( !rc )
	    return rc;
	} else
	  return ERROR_NOMEM;
      } else if ( vlen == (size_t)-1 )
      { return ERROR_SYNTAX_ERROR;
      } else
      { int rc = (func)(formdata, nlen, value, vlen, closure);

	if ( !rc )
	  return rc;
      }

      if ( *end )
	formdata = end+1;
      else
	formdata = end;
    } else
    { break;
    }
  }

  return TRUE;
}


static char *
find_boundary(const char *data, const char *end, const char *boundary)
{ size_t blen = strlen(boundary);

  while ( data < end &&
	  !(strncmp(data, boundary, blen) == 0) )
    data++;

  if ( data < end )
  { while(data[-1] == '-')
      data--;
    return (char *)data;
  }

  return NULL;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Find a named attribute in a mime header of a multipart form
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static char *
attribute_of_multipart_header(const char *name, char *header, char *endheader)
{ char *value;
  size_t nlen = strlen(name);

  while( header < endheader &&
	 !(header[nlen] == '=' && strncmp(header, name, nlen) == 0) )
    header++;

  if ( header < endheader )
  { header += nlen+1;

    if ( header[0] == '"' )
    { char *end;

      value = ++header;
      if ( (end = strchr(value, '"')) )
      { *end = '\0';
        return value;
      }
    } else
    { char *end;

      value = header;

      for(end=header; *end && isalnum(*end&0xff); end++)
	;
      *end = '\0';
      return value;
    }
  }

  return NULL;
}


static char *
looking_at_blank_lines(const char *line, int n)
{ while(n-- > 0)
  { if ( (line[0] == '\r' && line[1] == '\n') )
      line += 2;
    else if ( line[0] == '\n' )
      line += 1;
    else
      return NULL;
  }

  return (char *)line;
}


char *
next_line(const char *in)
{ if ( (in = strchr(in, '\n')) )
    return (char *)(in+1);

  return NULL;
}


int
break_multipart(char *formdata, size_t len,
		const char *boundary,
		int (*func)(const char *name,
			    size_t namelen,
			    const char *value,
			    size_t valuelen,
			    const char *filename,
			    void *closure),
		void *closure)
{ char *enddata = formdata+len;

  while(formdata < enddata)
  { char *header;
    char *name, *filename;
    char *data = NULL;
    char *end;

    if ( !(formdata=find_boundary(formdata, enddata, boundary)) ||
	 !(formdata=next_line(formdata)) )
      break;

    header = formdata;
					/* find the end of the header */
    for( ; formdata < enddata; formdata++ )
    { char *end;

      if ( (end = looking_at_blank_lines(formdata, 2)) )
      { formdata[0] = '\0';
	formdata = data = end;
	break;
      }
    }

    if ( !data )
      break;

    if ( !(name = attribute_of_multipart_header("name", header, data)) )
    { term_t t = PL_new_term_ref();
      PL_put_atom_chars(t, "name");

      return pl_error(NULL, 0, NULL, ERR_EXISTENCE, "field", t);
    }
    filename = attribute_of_multipart_header("filename", header, data);

    if ( !(formdata=find_boundary(data, enddata, boundary)) )
      break;
    end = formdata-1;
    if ( end[-1] == '\r' )
      end--;
    end[0] = '\0';

    if ( !(func)(name, strlen(name), data, end-data, filename, closure) )
      return FALSE;
  }

  return TRUE;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Get the raw data from the standard   input or QUERY_STRING. If `lenp' is
provided, it is filled with the length  of the contents. The input value
for lenp is the maximum acceptable content-length.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int
get_raw_form_data(char **data, size_t *lenp, int *must_free)
{ char *method;
  char *s;

  if ( (method = getenv("REQUEST_METHOD")) &&
       strcmp(method, "POST") == 0 )
  { char *lenvar = getenv("CONTENT_LENGTH");
    char *q;
    long len;

    if ( !lenvar )
    { term_t env = PL_new_term_ref();
      PL_put_atom_chars(env, "CONTENT_LENGTH");

      return pl_error(NULL, 0, NULL, ERR_EXISTENCE, "environment", env);
    }
    len = atol(lenvar);
    if ( len < 0 )
    { term_t t = PL_new_term_ref();

      if ( !PL_put_integer(t, len) )
	return FALSE;
      return pl_error(NULL, 0, "< 0", ERR_DOMAIN, t, "content_length");
    }
    if ( lenp )
    { if ( *lenp && (size_t)len > *lenp )
      { term_t t = PL_new_term_ref();
	char msg[100];

	if ( !PL_put_integer(t, len) )
	  return FALSE;
	sprintf(msg, "> %ld", (long)*lenp);

	return pl_error(NULL, 0, msg, ERR_DOMAIN, t, "content_length");
      }
      *lenp = len;
    }

    q = s = malloc(len+1);
    if ( !q )
      return pl_error(NULL, 0, NULL, ERR_RESOURCE, "memory");
    while(len > 0)
    { int done;

      while( (done=read(fileno(stdin), q, len)) > 0 )
      { q+=done;
	len-=done;
      }
      if ( done < 0 )
      { int e;
	term_t obj;

      no_data:
	e = errno;
	obj = PL_new_term_ref();

	free(s);
	PL_put_nil(obj);
	return pl_error(NULL, 0, NULL, ERR_ERRNO, e, "read", "cgi_data", obj);
      }
    }
    if ( len == 0 )
    { *q = '\0';
      *data = s;
      *must_free = TRUE;
      return TRUE;
    } else
      goto no_data;
  } else if ( (s = getenv("QUERY_STRING")) )
  { if ( lenp )
      *lenp = strlen(s);
    *data = s;
    *must_free = FALSE;
    return TRUE;
  } else
  { term_t env = PL_new_term_ref();
    PL_put_atom_chars(env, "QUERY_STRING");

    return pl_error(NULL, 0, NULL, ERR_EXISTENCE, "environment", env);
  }
}


