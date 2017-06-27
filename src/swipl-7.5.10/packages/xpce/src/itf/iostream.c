/*  Part of XPCE --- The SWI-Prolog GUI toolkit

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        jan@swi.psy.uva.nl
    WWW:           http://www.swi.psy.uva.nl/projects/xpce/
    Copyright (c)  1999-2013, University of Amsterdam
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

#include <h/kernel.h>
#include <h/unix.h>
#include <errno.h>

		 /*******************************
		 *      OBJECT --> IOSTREAM	*
		 *******************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Reading and writing to objects is  done   using  the `wchar' encoding of
streams to fully support international character   sets. To simplify the
interface we will translate the size of   the read and write requests to
n/sizeof(wchar_t) and do the translation to/from the buffer here.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef struct
{ Any	object;				/* The client (opened) object */
  long	point;				/* Current location */
  IOENC encoding;			/* used encoding */
} open_object, *OpenObject;


static ssize_t
Sread_object(void *handle, char *buf, size_t size)
{ OpenObject h = handle;
  Any argv[2];
  CharArray sub;
  int chread;
  size_t advance;

  if ( isFreedObj(h->object) )
  { errno = EIO;
    return -1;
  }

  if ( h->encoding == ENC_WCHAR )
  { advance = size/sizeof(wchar_t);
  } else if ( h->encoding == ENC_OCTET )
  { advance = size;
  } else
  { assert(0);
    errno = EIO;
    return -1;
  }

  argv[0] = toInt(h->point);
  argv[1] = toInt(advance);

  if ( (sub = getv(h->object, NAME_readAsFile, 2, argv)) &&
       instanceOfObject(sub, ClassCharArray) )
  { String s = &sub->data;

    assert(s->s_size <= advance);

    if ( h->encoding == ENC_WCHAR )
    { if ( isstrA(s) )
      { charW *dest = (charW*)buf;
	const charA *f = s->s_textA;
	const charA *e = &f[s->s_size];

	while(f<e)
	  *dest++ = *f++;
      } else
      { memcpy(buf, s->s_textW, s->s_size*sizeof(charW));
      }
      chread = s->s_size * sizeof(wchar_t);
    } else
    { if ( isstrA(s) )
      { memcpy(buf, s->s_textA, s->s_size);
      } else
      { errno = EIO;
	chread = -1;
      }
      chread = s->s_size;
    }

    h->point += s->s_size;
  } else
  { errno = EIO;
    chread = -1;
  }

  return chread;
}


static ssize_t
Swrite_object(void *handle, char *buf, size_t size)
{ OpenObject h = handle;
  string s;
  CharArray ca;
  status rval;
  Int where = toInt(h->point);
  size_t advance;

  if ( isFreedObj(h->object) )
  { errno = EIO;
    return -1;
  }

  if ( h->encoding == ENC_WCHAR )
  { const wchar_t *wbuf = (const wchar_t*)buf;
    const wchar_t *end = (const wchar_t*)&buf[size];
    const wchar_t *f;

    assert(size%sizeof(wchar_t) == 0);
    advance = size/sizeof(wchar_t);

    for(f=wbuf; f<end; f++)
    { if ( *f > 0xff )
	break;
    }

    if ( f == end )
    { charA *asc = alloca(size);
      charA *t = asc;

      for(f=wbuf; f<end; )
	*t++ = (charA)*f++;

      str_set_n_ascii(&s, advance, (char*)asc);
    } else
    { str_set_n_wchar(&s, advance, (wchar_t*)wbuf);
    }
  } else if ( h->encoding == ENC_OCTET )
  { advance = size;
    str_set_n_ascii(&s, size, buf);
  } else
  { assert(0);
    errno = EIO;
    return -1;
  }

  ca = StringToScratchCharArray(&s);

  if ( (rval = send(h->object, NAME_writeAsFile, where, ca, EAV)) )
    h->point += (long)advance;
  doneScratchCharArray(ca);

  if ( rval )
    return size;

  errno = EIO;
  return -1;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Note: pos is measured  in  bytes.  If   we  use  wchar  encoding we must
compensate for this.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static long
Sseek_object(void *handle, long pos, int whence)
{ OpenObject h = handle;
  Int size;
  int usize = (h->encoding == ENC_WCHAR ? sizeof(wchar_t) : 1);

  pos /= usize;

  if ( isFreedObj(h->object) )
  { errno = EIO;
    return -1;
  }

  switch(whence)
  { case SIO_SEEK_SET:
      h->point = pos;
      break;
    case SIO_SEEK_CUR:
      h->point += pos;			/* check for end!? */
      break;
    case SIO_SEEK_END:
    { if ( hasGetMethodObject(h->object, NAME_sizeAsFile) &&
	   (size = get(h->object, NAME_sizeAsFile, EAV)) )
      { h->point = valInt(size) - pos;
	break;
      } else
      { errno = EPIPE;			/* better idea? */
	return -1;
      }
    }
    default:
    { errno = EINVAL;
      return -1;
    }
  }

  return h->point * usize;
}


static int
Sclose_object(void *handle)
{ OpenObject h = handle;

  if ( isFreedObj(h->object) )
  { errno = EIO;
    return -1;
  }

  delCodeReference(h->object);
  freeableObj(h->object);

  unalloc(sizeof(*h), h);

  return 0;
}


static IOFUNCTIONS Sobjectfunctions =
{ Sread_object,
  Swrite_object,
  Sseek_object,
  Sclose_object
};


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Note that files have their own  open/close.   In  the old days that used
stdio FILE*. Now that they both use IOSTREAM*, this should be merged.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

IOSTREAM *
Sopen_object(Any obj, const char *mode)
{ if ( instanceOfObject(obj, ClassFile) )
  { Name name = getOsNameFile(obj);
    IOSTREAM *s;

    if ( (s=Sopen_file(nameToFN(name), mode)) )
    { if ( !strchr(mode, 'b') )
      { FileObj f = obj;
	Name oldstat = f->status;
	IOSTREAM *ofd = f->fd;
	int rc;

					/* HACKS */
	f->status = (mode[0] == 'r' ? NAME_read : NAME_write);
	f->fd = s;

	switch(mode[0])
	{ case 'r':
	  { if ( (rc = doBOMFile(f)) )
	      setStreamEncodingSourceSink(obj, s);
	    break;
	  }
	  case 'w':
	  { setStreamEncodingSourceSink(obj, s);
	    rc = doBOMFile(f);
	    break;
	  }
	  default:
	  { setStreamEncodingSourceSink(obj, s);
	    rc = 0;
	  }
	}

	s->newline = (f->newline_mode == NAME_posix ? SIO_NL_POSIX :
		      f->newline_mode == NAME_dos   ? SIO_NL_DOS :
						      SIO_NL_DETECT);

	f->fd = ofd;
	f->status = oldstat;
	if ( !rc )
	  return NULL;
      }
      return s;
    }

    errorPce(obj, NAME_openFile,
	     mode[0] == 'r' ? NAME_read : NAME_write,
	     getOsErrorPce(PCE));

    return s;
  } else if ( instanceOfObject(obj, ClassRC) &&
	      TheCallbackFunctions.rc_open )
  { IOSTREAM *s;
    RC rc = obj;
    char *rc_class;

    if ( notDefault(rc->rc_class) )
      rc_class = strName(rc->rc_class);
    else
      rc_class = NULL;

    if ( notNil(rc->context) && TheCallbackFunctions.setHostContext )
    { Any savedcontext =
	(*TheCallbackFunctions.setHostContext)(rc->context);

      s = (*TheCallbackFunctions.rc_open)(strName(rc->name),
					  rc_class,
					  mode);
      (*TheCallbackFunctions.setHostContext)(savedcontext);
    } else
      s = (*TheCallbackFunctions.rc_open)(strName(rc->name),
					  rc_class,
					  mode);

    if ( !s )
      errorPce(obj, NAME_openFile,
	       mode[0] == 'r' ? NAME_read : NAME_write,
	       getOsErrorPce(PCE));

    return s;
  } else
  { int flags = SIO_TEXT|SIO_RECORDPOS;
    OpenObject h;
    IOSTREAM *stream;

    switch(mode[0])
    { case 'r':
	flags |= SIO_INPUT;
        break;
      case 'w':
	flags |= SIO_OUTPUT;
        break;
      default:
	errno = EINVAL;
        return NULL;
    }

    for(mode++; *mode; mode++)
    { switch(*mode)
      { case 'b':			/* binary */
	  flags &= ~SIO_TEXT;
	  break;
	case 'r':			/* no record */
	  flags &= ~SIO_RECORDPOS;
	  break;
	default:
	  errno = EINVAL;
	  return NULL;
      }
    }

    h = alloc(sizeof(*h));
    h->point = 0;
    h->object = obj;
    addCodeReference(obj);

    stream = Snew(h, flags, &Sobjectfunctions);

    if ( (flags&SIO_TEXT) )
      stream->encoding = ENC_WCHAR;	/* see comment above */
    else
      stream->encoding = ENC_OCTET;
    h->encoding = stream->encoding;

    return stream;
  }
}
