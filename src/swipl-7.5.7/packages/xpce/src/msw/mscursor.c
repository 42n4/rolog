/*  Part of XPCE --- The SWI-Prolog GUI toolkit

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        jan@swi.psy.uva.nl
    WWW:           http://www.swi.psy.uva.nl/projects/xpce/
    Copyright (c)  1995-2013, University of Amsterdam
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

#define USE_C_CONVERTED_CURSORS 1

#include "include.h"
#include "xcursor.h"
#ifdef USE_C_CONVERTED_CURSORS
#include "xcursors.c"
#else
#include <h/unix.h>
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MS-Windows cursors is a difficult one.   MS-Windows predefines only very
few standard cursors.  For this reason we decided to make the X11 cursor
font cursors available for the MS-Windows version.

Copyright notice: the X11 cursor-font is not   copyrighted  (as far as I
manged to determine).

The X11 cursors have been translated  to   ASCII  using  the X11 program
showfont.   The  resulting  ASCII  file   is    parsed   into  a  binary
representation using the xcvncurs  program   in  the xpce/lib directory.
The converted cursor  file  is   located  in  $PCEHOME/lib/X11.crs.  The
format on this (binary) file is described in the file "xcursor.h".

The function read_cursor_glyphs() reads this binary file into memory and
provides a cache for the final  MS-Window images for CreateCursor(). The
function get_cursor_bits() used the X11Glyhps() to return mask and image
bitmaps  of  the  size  demanded  by  CreateCursor().  It  performs  the
following tasks:

	1) Copy the mask and images into a pattern of the desired size
	2) Convert the bits such that the intended cursor is displayed:

	X11 glyph	| MS-Windows	| Result
	===========================================
	Mask	| Image	| Mask	| Image |
	===========================================
	0	| 0	| 1	| 0	| untouched
	0	| 1	| 1	| 0	| untouched
	1	| 0	| 0	| 1	| white
	1	| 1	| 0	| 0	| black
			| 1	| 1	| invert
	===========================================

	So, the operations is: image = ~image & mask
			       mask  = ~mask
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int cursor_width;		/* only width allowed */
static int cursor_height;		/* only height allowed */

#define SEC(c) (((int)(c) << 24) >> 24)	/* sign-extend-character */
#define XID_MIN 0
#define XID_MAX 152


		 /*******************************
		 *    X11 CURSOR DESCRIPTION	*
		 *******************************/

static Sheet	cursorNames = NIL;

static struct standardCursor
{ char *name;				/* X name of the cursor */
  int	id;				/* X font id of the cursor */
} standard_cursors[] =
{ { "X_cursor",			0 },
  { "arrow",			2 },
  { "based_arrow_down",		4 },
  { "based_arrow_up",		6 },
  { "boat",			8 },
  { "bogosity",			10 },
  { "bottom_left_corner",	12 },
  { "bottom_right_corner",	14 },
  { "bottom_side",		16 },
  { "bottom_tee",		18 },
  { "box_spiral",		20 },
  { "center_ptr",		22 },
  { "circle",			24 },
  { "clock",			26 },
  { "coffee_mug",		28 },
  { "cross",			30 },
  { "cross_reverse",		32 },
  { "crosshair",		34 },
  { "diamond_cross",		36 },
  { "dot",			38 },
  { "dotbox",			40 },
  { "double_arrow",		42 },
  { "draft_large",		44 },
  { "draft_small",		46 },
  { "draped_box",		48 },
  { "exchange",			50 },
  { "fleur",			52 },
  { "gobbler",			54 },
  { "gumby",			56 },
  { "hand1",			58 },
  { "hand2",			60 },
  { "heart",			62 },
  { "icon",			64 },
  { "iron_cross",		66 },
  { "left_ptr",			68 },
  { "left_side",		70 },
  { "left_tee",			72 },
  { "leftbutton",		74 },
  { "ll_angle",			76 },
  { "lr_angle",			78 },
  { "man",			80 },
  { "middlebutton",		82 },
  { "mouse",			84 },
  { "pencil",			86 },
  { "pirate",			88 },
  { "plus",			90 },
  { "question_arrow",		92 },
  { "right_ptr",		94 },
  { "right_side",		96 },
  { "right_tee",		98 },
  { "rightbutton",		100 },
  { "rtl_logo",			102 },
  { "sailboat",			104 },
  { "sb_down_arrow",		106 },
  { "sb_h_double_arrow",	108 },
  { "sb_left_arrow",		110 },
  { "sb_right_arrow",		112 },
  { "sb_up_arrow",		114 },
  { "sb_v_double_arrow",	116 },
  { "shuttle",			118 },
  { "sizing",			120 },
  { "spider",			122 },
  { "spraycan",			124 },
  { "star",			126 },
  { "target",			128 },
  { "tcross",			130 },
  { "top_left_arrow",		132 },
  { "top_left_corner",		134 },
  { "top_right_corner",		136 },
  { "top_side",			138 },
  { "top_tee",			140 },
  { "trek",			142 },
  { "ul_angle",			144 },
  { "umbrella",			146 },
  { "ur_angle",			148 },
  { "watch",			150 },
  { "xterm",			152 },

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
These are the standard windows  ones.   MAKEINTRESOURCE()  just tags the
value. We tag it back to int  ...   This  is  a bit dangerous for future
compatibility, but the compiler is likely to  complain if it changes and
it shouldn't be hard to find another trick.

We use negative numbers, just to distinguish   them  from the others and
avoid too much dependency on where Microsoft places these numbers.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define MSINTVAL(charp) (-((int)(intptr_t)(charp)))

  { "win_arrow",		MSINTVAL(IDC_ARROW) },
  { "win_ibeam",		MSINTVAL(IDC_IBEAM) },
  { "win_wait",			MSINTVAL(IDC_WAIT) },
  { "win_cross",		MSINTVAL(IDC_CROSS) },
  { "win_uparrow",		MSINTVAL(IDC_UPARROW) },
  { "win_size",			MSINTVAL(IDC_SIZE) },
  { "win_icon",			MSINTVAL(IDC_ICON) },
  { "win_sizenwse",		MSINTVAL(IDC_SIZENWSE) },
  { "win_sizenesw",		MSINTVAL(IDC_SIZENESW) },
  { "win_sizewe",		MSINTVAL(IDC_SIZEWE) },
  { "win_sizens",		MSINTVAL(IDC_SIZENS) },
  { "win_sizeall",		MSINTVAL(IDC_SIZEALL) },
  { "win_no",			MSINTVAL(IDC_NO) },
  { "win_appstarting",		MSINTVAL(IDC_APPSTARTING) },

  { NULL,			0 }
};



void
ws_init_cursor_font()
{ struct standardCursor *sc;

  cursorNames = globalObject(NAME_cursorNames, ClassSheet, EAV);

  for(sc = standard_cursors; sc->name; sc++)
    valueSheet(cursorNames, (Any) CtoName(sc->name), toInt(sc->id));

  cursor_width  = GetSystemMetrics(SM_CXCURSOR);
  cursor_height = GetSystemMetrics(SM_CYCURSOR);

  DEBUG(NAME_cursor,
	Cprintf("Cursor size is %dx%d\n", cursor_width, cursor_height));
}


Int
ws_cursor_font_index(Name name)
{ return getValueSheet(cursorNames, name);
}


		 /*******************************
		 *     CONVERSION FUNCTIONS	*
		 *******************************/

typedef struct
{ unsigned short *	mask;	/* mask (cursor_width x cursor_height) */
  unsigned short *	image;	/* image (cursor_width x cursor_height) */
} cursor_bits, *CursorBits;


typedef struct
{ cursor_glyph_file_header	header;
  CursorGlyph			glyphs;
  unsigned short *		data;
  CursorBits			cache;
  int				cwidth;	/* width of Windows Cursor */
  int				cheight; /* height of Windows Cursor */
} cursor_glyph_set, *CursorGlyphSet;


#ifndef USE_C_CONVERTED_CURSORS
static CursorGlyphSet
read_cursor_glyphs(FILE *in)
{ CursorGlyphSet set = pceMalloc(sizeof(cursor_glyph_set));
  int entries;
  int dshorts;
  int cachesize;

  fread(&set->header, sizeof(cursor_glyph_file_header), 1, in);
  if ( set->header.magic != XPCE_CURSOR_FILE_MAGIC )
  { Cprintf("Bad magic number\n");
    pceFree(set);
    return NULL;
  }

  entries   = set->header.entries;
  dshorts   = set->header.dsize / sizeof(unsigned short);
  cachesize = entries/2 * sizeof(cursor_bits);

  if ( !(set->glyphs = pceMalloc(sizeof(cursor_glyph) * entries)) ||
       !(set->data   = pceMalloc(set->header.dsize)) ||
       !(set->cache  = pceMalloc(cachesize)) )
  { Cprintf("Not enough memory\n");
    return NULL;
  }

  if ( fread(set->glyphs, sizeof(cursor_glyph), entries, in) != entries ||
       fread(set->data, sizeof(unsigned short), dshorts, in) != dshorts )
  { Cprintf("Read failed\n");
    return NULL;
  }

  memset(set->cache, 0, cachesize);
  set->cwidth = cursor_width;
  set->cheight = cursor_height;

  fclose(in);

  return set;
}
#endif /*USE_C_CONVERTED_CURSORS*/


static int
ws_sizeof_bits(int w, int h)
{ int bytes = ((w+15)/16) * 2 * h;

  return ((bytes + 3)/4) * 4;		/* round on longs */
}


static void
copy_pattern(unsigned short *dbits, int dw, int dh,
	     unsigned short *cbits, int w, int h)
{ int x, y;
  unsigned short *c, *d;

  for(y=0; y<h; y++)
  { c = cbits + y*((w+15)/16);
    d = dbits + y*((dw+15)/16);
    x = 0;

    if ( y < dh )
    { for(; x < w && x < dw; x += 16)
	*c++ = *d++;
      if ( x-dw > 0 )			/* need partial padding */
      { unsigned short mask = (0xffff0000L >> (16-(x-dw)));

	c[-1] &= mask;
      }
    }
    for(; x < w; x += 16)
      *c++ = 0x0000;
  }
}


static void
copy_mask_pattern(unsigned short *dbits, int dw, int dh,
		  unsigned short *cbits, int w, int h)
{ copy_pattern(dbits, dw, dh, cbits, w, h);
}


static void
copy_image_pattern(unsigned short *dbits, int dw, int dh,
		   unsigned short *cbits, int w, int h,
		   int sx, int sy)
{ int spl = (w+15)/16;			/* shorts-per-line */

  copy_pattern(dbits, dw, dh, cbits, w, h);

  if ( sx<0 || sx > 16 || sy < 0 || sy > 16 )
  { Cprintf("sx = %d, sy = %d\n", sx, sy);
    return;
  }

  if ( sx > 0 )				/* shifts <= 16 pixels! */
  { int x, y;
    unsigned short *c;

    for(y=0; y<h; y++)
    { unsigned short carry = 0;
      c = cbits + y*spl;

      for(x=0; x<w; x+=16)
      { unsigned short nc = (*c << (16-sx));
	*c >>= sx;
	*c++ |= carry;
	carry = nc;
      }
    }
  }

  if ( sy > 0 )
  { int x, y;
    unsigned short *t, *f;

    for(y=h-1; y>=sx; y--)
    { t = cbits + y*spl;
      f = cbits + (y-1)*spl;

      for(x=0; x<w; x+=16)
	*t++ = *f++;
    }
    for(; y >= 0; y--)			/* clear top-lines */
    { t = cbits + y*spl;
      for(x=0; x<w; x+=16)
	*t++ = 0x0000;
    }
  }
}


static void
swap_bytes(unsigned short *data, int w, int h)
{ int x, y;
  int spl = (w+15)/16;			/* shorts-per-line */

  for(y=0; y<h; y++)
  { for(x=0; x<spl; x++)
    { unsigned short w;

      w =  (*data << 8) & 0xff00;
      w |= (*data >> 8) & 0xff;

      *data++ = w;
    }
  }
}


static void
window_bits(unsigned short *mask,  unsigned short *image, int w, int h)
{ int x, y;
  int spl = (w+15)/16;			/* shorts-per-line */

  for(y=0; y<h; y++)
  { for(x=0; x<spl; x++)
    { *image = ~*image & *mask;
      *mask  = ~*mask;
      image++, mask++;
    }
  }
}


static CursorBits
get_cursor_bits(CursorGlyphSet set, int cursor, int *hx, int *hy)
{ CursorBits result;
  int cache_idx = cursor/2;		/* cache index */
  CursorGlyph mglyph = &set->glyphs[cursor+1];

  if ( cursor < 0 || cursor > set->header.entries-1 )
  { Cprintf("Cursor %d out of range\n", cursor);
    return NULL;
  }

  result = &set->cache[cache_idx];

  if ( !result->image )
  { CursorGlyph iglyph = &set->glyphs[cursor];

    result->image = pceMalloc(ws_sizeof_bits(set->cwidth, set->cheight));
    result->mask  = pceMalloc(ws_sizeof_bits(set->cwidth, set->cheight));

    DEBUG(NAME_cursor, Cprintf("Mask hotspot = %d, %d\n",
			       SEC(mglyph->hot_x), SEC(mglyph->hot_y)));
    DEBUG(NAME_cursor, Cprintf("Image hotspot = %d, %d\n",
			       SEC(iglyph->hot_x), SEC(iglyph->hot_y)));

    copy_mask_pattern(&set->data[mglyph->offset/sizeof(unsigned short)],
		      mglyph->width, mglyph->height,
		      result->mask,
		      set->cwidth, set->cheight);
    copy_image_pattern(&set->data[iglyph->offset/sizeof(unsigned short)],
		       iglyph->width, iglyph->height,
		       result->image,
		       set->cwidth, set->cheight,
		       SEC(mglyph->hot_x) - SEC(iglyph->hot_x),
		       SEC(mglyph->hot_y) - SEC(iglyph->hot_y));

    window_bits(result->mask,  result->image, set->cwidth, set->cheight);

    swap_bytes(result->mask,  set->cwidth, set->cheight);
    swap_bytes(result->image, set->cwidth, set->cheight);
  }

  if ( hx )
    *hx = mglyph->hot_x;
  if ( hy )
    *hy = mglyph->hot_y;

  return result;
}


#ifdef USE_C_CONVERTED_CURSORS

static CursorGlyphSet
X11Glyhps()
{ static CursorGlyphSet glyphs = NULL;

  if ( !glyphs )
  { int entries;
    int cachesize;

    if ( !(glyphs=pceMalloc(sizeof(cursor_glyph_set))) )
      goto nomem;

    memcpy(&glyphs->header, &x11_glyph_header, sizeof(x11_glyph_header));
    glyphs->glyphs = x11_glyps;
    glyphs->data   = x11_glyph_data;

    entries   = glyphs->header.entries;
    cachesize = (entries/2) * sizeof(cursor_bits);

    if ( !(glyphs->cache  = pceMalloc(cachesize)) )
    {
    nomem:
      Cprintf("Not enough memory\n");
      return NULL;
    }

    memset(glyphs->cache, 0, cachesize);
    glyphs->cwidth  = cursor_width;
    glyphs->cheight = cursor_height;
  }

  return glyphs;
}

#else /*USE_C_CONVERTED_CURSORS*/

static CursorGlyphSet
X11Glyhps()
{ static CursorGlyphSet glyphs = NULL;

  if ( !glyphs )
  { FileObj f = answerObject(ClassFile,
			     CtoName("$PCEHOME/lib/X11.crs"),
			     NAME_binary, EAV);

    if ( send(f, NAME_open, NAME_read, EAV) )
    { glyphs = read_cursor_glyphs(f->fd);
      send(f, NAME_close, EAV);
    }

    if ( !glyphs )
      sysPce("Failed to read cursor file %s", pp(f->name));
  }

  return glyphs;
}

#endif /*USE_C_CONVERTED_CURSORS*/

#if 0
		 /*******************************
		 *	     TEST/DEBUG		*
		 *******************************/

static void
print_bits(CursorGlyphSet set, CursorBits bits, int hx, int hy, char show)
{ int x, y;
  int w = set->cwidth;
  int h = set->cheight;
  int spl = (w+15)/16;			/* shorts-per-line */

  Cprintf("Image is %dx%d, HotSpot at %d, %d\n\n", w, h, hx, hy);

  for(y=0; y<h; y++)
  { unsigned short *i = bits->image + y * spl;
    unsigned short *m = bits->mask  + y * spl;
    int bit=15;

    for(x=0; x<w; x++)
    { int bm = *m & (1<<bit);
      int bi = *i & (1<<bit);

      if ( x == hx && y == hy )
	Cputchar('X');
      else
      { if ( show == 'm' )
	  Cputchar(bm ? '#' : '.');
        else if ( show == 'i' )
	  Cputchar(bi ? '#' : '.');
	else
	  Cputchar(bm  && !bi ? '.' :
		   bm  && bi  ? '-' :
		   !bm && bi  ? '#' : '$');
      }

      if ( bit-- == 0 )
      { i++, m++;
	bit = 15;
      }
    }
    Cputchar('\n');
  }
}

#endif

		 /*******************************
		 *	  CREATE/DESTROY	*
		 *******************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Note: cursors are limited  in   size  to GetSystemMetrics(SM_CXCURSOR) x
GetSystemMetrics(SM_CYCURSOR). If the system  finds   larger  cursors it
will not create the cursor.  See   ws_window_cursor()  in mswindow.c for
details.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

status
ws_create_cursor(CursorObj c, DisplayObj d)
{ HCURSOR msc = 0;			/* Windows cursor */

  if ( notNil(c->font_id) )
  { CursorBits bits;
    int hx, hy;				/* hot spot */
    int id;

    if ( isDefault(c->font_id) )
    { Int id;

      if ( !(id = ws_cursor_font_index(c->name)) )
	return errorPce(c, NAME_noNamedCursor, c->name);

      assign(c, font_id, id);
    }

    id = valInt(c->font_id);
    if ( id >= XID_MIN && id <= XID_MAX )
    { if ( (bits = get_cursor_bits(X11Glyhps(), id, &hx, &hy)) )
      { msc = CreateCursor(PceHInstance,
			   hx, hy,
			   cursor_width, cursor_height,
			   bits->mask, bits->image);
      }
    } else if ( id < 0 )
    { id = -id;
      msc = LoadCursor(NULL, MAKEINTRESOURCE(id));
    }
  } else
  { void *source = ws_image_bits_for_cursor(c->image, NAME_image,
					    cursor_width, cursor_height);
    void *mask   = ws_image_bits_for_cursor(c->mask, NAME_mask,
					    cursor_width, cursor_height);

    window_bits(mask, source, cursor_width, cursor_height);
    msc = CreateCursor(PceHInstance,
		       valInt(c->hot_spot->x),
		       valInt(c->hot_spot->y),
		       cursor_width,
		       cursor_height,
		       mask, source);
    pceFree(source);
    pceFree(mask);
  }

  if ( !msc )
  { Cprintf("Failed to create cursor for %s\n", pp(c));
    msc = LoadCursor(NULL, IDC_ARROW);
  }

  return registerXrefObject(c, d, (void *) msc);
}


void
ws_destroy_cursor(CursorObj c, DisplayObj d)
{ Xref r;

  while( (r = unregisterXrefObject(c, d)) )
  { HCURSOR msc = (HCURSOR) r->xref;

    if ( msc == GetCursor() )
      SetCursor(LoadCursor(NULL, IDC_ARROW));

    DestroyCursor(msc);
  }
}


		 /*******************************
		 *	   BIG CURSORS		*
		 *******************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
XPCE's drag-and-drop gestures require BIG cursors.  We'll try to fake these
below.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static HBITMAP	saved_bits = NULL;	/* saved background */
static HBITMAP  image;			/* the image bits */
static HBITMAP  mask;			/* the mask bits */
static HCURSOR  hcursorsave;		/* saved cursor */
static int	is_saved;		/* have we been saved? */
static int	saved_x;		/* x-position of the background */
static int	saved_y;		/* y-position of the background */
static int	hot_x;			/* x-hot-spot */
static int	hot_y;			/* y-hot-spot */
static int	big_cursor_width;	/* width of our giant */
static int	big_cursor_height;	/* height of our giant */

status
has_big_cursor()
{ return saved_bits ? SUCCEED : FAIL;
}


status
save_big_cursor_background()
{ HDC hdc = GetDC(NULL);
  HDC bmhdc = CreateCompatibleDC(hdc);
  HBITMAP obm;
  POINT pt;

  GetCursorPos(&pt);
  saved_x = pt.x - hot_x;
  saved_y = pt.y - hot_y;

  if ( !saved_bits )
  { saved_bits = ZCreateCompatibleBitmap(hdc,
					 big_cursor_width, big_cursor_height);
    if ( !saved_bits )
      fail;
  }

  DEBUG(NAME_cursor,
	Cprintf("BIG: save from %d,%d (%dx%d)\n",
		saved_x, saved_y, big_cursor_width, big_cursor_height));

  obm = ZSelectObject(bmhdc, saved_bits);
  BitBlt(bmhdc, 0, 0, big_cursor_width, big_cursor_height,
	 hdc, saved_x, saved_y, SRCCOPY);
  ZSelectObject(bmhdc, obm);
  DeleteDC(bmhdc);
  ReleaseDC(NULL, hdc);

  is_saved = TRUE;

  succeed;
}


status
restore_big_cursor_background()
{ if ( saved_bits && is_saved )
  { HDC hdc = GetDC(NULL);
    HDC bmhdc = CreateCompatibleDC(hdc);
    HBITMAP obm;

    DEBUG(NAME_cursor, Cprintf("BIG: restore at %d,%d\n", saved_x, saved_y));

    obm = ZSelectObject(bmhdc, saved_bits);
    BitBlt(hdc, saved_x, saved_y, big_cursor_width, big_cursor_height,
	   bmhdc, 0, 0, SRCCOPY);
    ZSelectObject(bmhdc, obm);
    DeleteDC(bmhdc);
    ReleaseDC(NULL, hdc);

    is_saved = FALSE;
  }

  succeed;
}


status
paint_big_cursor()
{ if ( image && mask )
  { HDC hdc = GetDC(NULL);
    HDC bmhdc = CreateCompatibleDC(hdc);
    HBITMAP obm;

    DEBUG(NAME_image, Cprintf("BIG: paint at %d,%d\n", saved_x, saved_y));

    obm = ZSelectObject(bmhdc, mask);
    BitBlt(hdc, saved_x, saved_y, big_cursor_width, big_cursor_height,
	   bmhdc, 0, 0, MERGEPAINT);
    ZSelectObject(bmhdc, image);
    BitBlt(hdc, saved_x, saved_y, big_cursor_width, big_cursor_height,
	   bmhdc, 0, 0, SRCINVERT);
    ZSelectObject(bmhdc, obm);
    DeleteDC(bmhdc);
    ReleaseDC(NULL, hdc);
  }

  succeed;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MASK	IMG	IMAGE
 0       0        1
 0	 1        0
 1       0        0
 1       1        0

	 --> IMAGE = ~(IMG|MASK)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static HBITMAP
mask_image(HBITMAP img, HBITMAP mask, int w, int h)
{ HDC src       = CreateCompatibleDC(NULL);
  HBITMAP image = ZCreateCompatibleBitmap(src, w, h);
  HDC dst	= CreateCompatibleDC(src);
  HBITMAP osrc, odst;

  odst = ZSelectObject(dst, image);
  osrc = ZSelectObject(src, mask);
  BitBlt(dst, 0, 0, w, h, src, 0, 0, SRCCOPY);
  ZSelectObject(src, img);
  BitBlt(dst, 0, 0, w, h, src, 0, 0, SRCPAINT);
  BitBlt(dst, 0, 0, w, h, src, 0, 0, DSTINVERT);

  ZSelectObject(src, osrc);
  ZSelectObject(dst, odst);
  DeleteDC(src);
  DeleteDC(dst);

  return image;
}


status
start_big_cursor(CursorObj c)
{ DisplayObj d = CurrentDisplay(NIL);
  HBITMAP img;

  if ( isNil(c->mask) || isNil(c->image) || isNil(c->hot_spot) )
  { Cprintf("Cannot create BIG cursor from non-image cursor");
    fail;
  }

  img		    = (HBITMAP) getXrefObject(c->image, d);
  mask              = (HBITMAP) getXrefObject(c->mask, d);
  big_cursor_width  = valInt(c->mask->size->w);
  big_cursor_height = valInt(c->mask->size->h);
  hot_x             = valInt(c->hot_spot->x);
  hot_y             = valInt(c->hot_spot->y);

  image = mask_image(img, mask, big_cursor_width, big_cursor_height);

  DEBUG(NAME_cursor, Cprintf("Started BIG cursor of %dx%d\n",
			     big_cursor_width, big_cursor_height));

/*hcursorsave = SetCursor(LoadCursor(NULL, IDC_ICON));  empty cursor */
  save_big_cursor_background();
  paint_big_cursor();

  succeed;
}


status
move_big_cursor()
{ if ( saved_bits )			/* indicates we have one */
  { restore_big_cursor_background();
    save_big_cursor_background();
    paint_big_cursor();
  }

  succeed;
}


status
exit_big_cursor()
{ if ( saved_bits )
  { DEBUG(NAME_cursor, Cprintf("BIG: exit\n"));
    restore_big_cursor_background();
    ZDeleteObject(saved_bits);
    saved_bits = NULL;
  }

  if ( hcursorsave )
  { SetCursor(hcursorsave);
    hcursorsave = NULL;
  }

  if ( image )
  { ZDeleteObject(image);
    image = NULL;
  }

  if ( mask )				/* is Xref of XPCE image! */
    mask = NULL;

  succeed;
}
