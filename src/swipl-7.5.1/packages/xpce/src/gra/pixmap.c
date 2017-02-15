/*  Part of XPCE --- The SWI-Prolog GUI toolkit

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        jan@swi.psy.uva.nl
    WWW:           http://www.swi.psy.uva.nl/projects/xpce/
    Copyright (c)  1985-2002, University of Amsterdam
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
#include <h/graphics.h>

static status
initialisePixmap(PixmapObj pm, Any from, Colour fg, Colour bg, Int w, Int h)
{ if ( isNil(from) )
  { initialiseImage((Image) pm, NIL, w, h, NAME_pixmap);
    if ( notDefault(fg) )
      assign(pm, foreground, fg);
    if ( notDefault(bg) )
      assign(pm, background, bg);

    succeed;
  }

  if ( instanceOfObject(from, ClassImage) )
  { Image i = from;

    initialiseImage((Image) pm, NIL, i->size->w, i->size->h, NAME_pixmap);
    if ( notDefault(fg) )
      assign(pm, foreground, fg);
    if ( notDefault(bg) )
      assign(pm, background, bg);

    TRY(send(pm, NAME_copy, i, EAV));

    newObject(ClassHyper, from, pm, NAME_pixmap, NAME_image, EAV);
    succeed;
  }

  if ( instanceOfObject(from, ClassFile) )
  { FileObj f = from;

    assign(pm, name,	   f->name);
    assign(pm, background, fg);
    assign(pm, foreground, bg);
    assign(pm, kind,	   NAME_pixmap);
    assign(pm, file,	   f);
    assign(pm, access,	   NAME_read);
    assign(pm, depth,	   DEFAULT);
    assign(pm, size,	   newObject(ClassSize,	EAV));
    ws_init_image((Image) pm);
    TRY(loadImage((Image) pm, DEFAULT, DEFAULT));
    protectObject(pm);
    appendHashTable(ImageTable, f->name, pm);

    succeed;
  }

  fail;
}


static PixmapObj
getLookupPixmap(Any receiver, Image i, Colour fg, Colour bg, Int w, Int h)
{ Chain ch;

  if ( (ch = getAllHypersObject(i, OFF)) )
  { Cell cell;

    for_cell(cell, ch)
    { Hyper h = cell->value;

      if ( h->from == i && h->forward_name == NAME_pixmap )
      { PixmapObj pm = h->to;

	if ( instanceOfObject(pm, ClassPixmap) &&
	     (isDefault(fg) || pm->foreground == fg) &&
	     (isDefault(bg) || pm->background == bg) )
	  answer(pm);
      }
    }
  }

  fail;
}


static PixmapObj
getConvertPixmap(Class class, Any obj)
{ PixmapObj pm;

  if ( (pm = getLookupPixmap(class, obj, DEFAULT, DEFAULT, DEFAULT, DEFAULT)) )
    answer(pm);

  if ( (pm = getConvertObject(class, obj)) )
  { if ( instanceOfObject(pm, ClassPixmap) )
      answer(pm);

    obj = pm;
  }

  if ( instanceOfObject(obj, ClassBitmap) )
  { pm = (PixmapObj)((BitmapObj)obj)->image;

    if ( instanceOfObject(pm, ClassPixmap) )
      answer(pm);
  }

  if ( instanceOfObject(obj, ClassGraphical) )
  { Graphical gr = obj;

    ComputeGraphical(gr);
    if ( (pm = newObject(ClassPixmap, NIL,
			  DEFAULT, DEFAULT, /* fg, bg */
			  gr->area->w, gr->area->h,
			  EAV)) )
    { send(pm, NAME_drawIn, gr, answerObject(ClassPoint, EAV), EAV);
      answer(pm);
    }
  }

  return answerObject(ClassPixmap, obj, EAV);
}


static Any
getSourcePixmap(PixmapObj pm)
{ Image src;

  if ( notNil(pm->file) )
    answer(pm->file);
  if ( (src = getHyperedObject(pm, NAME_image, DEFAULT)) )
    answer(src);

  answer(NIL);
}


#ifdef WIN32_GRAPHICS
Colour
getReplacementColourPixmap(PixmapObj pm)
{ Colour c;
  Image i;
  Int grey;

  if ( (c = getAttributeObject(pm, NAME_replacementColour)) )
    answer(c);
  if ( (i = getSourcePixmap(pm)) &&
       instanceOfObject(i, ClassImage) &&
       (grey = getAttributeObject(i, NAME_postscriptGrey)) )
  { char buf[100];
    sprintf(buf, "grey%d", 100-(int)valInt(grey));
    c = newObject(ClassColour, CtoName(buf), EAV);
  } else
    c = BLACK_COLOUR;

  errorPce(pm, NAME_replacedByColour, c);
  attributeObject(pm, NAME_replacementColour, c);

  answer(c);
}
#endif /*WIN32_GRAPHICS*/


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */

static char *T_fill[] =
        { "image|colour", "[area]" };
static char *T_initialise[] =
        { "source=[image|file]*", "foreground=[colour]",
	  "background=[colour]", "width=[int]", "height=[int]" };
static char *T_lookup[] =
        { "source=image", "foreground=[colour]", "background=[colour]",
	  "width=[int]", "height=[int]"};

/* Instance Variables */

#define var_pixmap NULL
/*
vardecl var_pixmap[] =
{
};
*/

/* Send Methods */

static senddecl send_pixmap[] =
{ SM(NAME_initialise, 5, T_initialise, initialisePixmap,
     DEFAULT, "Create image of <-kind pixmap"),
  SM(NAME_fill, 2, T_fill, fillImage,
     NAME_edit, "Fill rectangular area of image with pattern")
};

/* Get Methods */

static getdecl get_pixmap[] =
{ GM(NAME_convert, 1, "pixmap", "name|image|graphical|file", getConvertPixmap,
     NAME_oms, "Convert @name, image, graphical or file-data"),
  GM(NAME_lookup, 5, "pixmap", T_lookup, getLookupPixmap,
     NAME_oms, "Lookup already made conversion"),
  GM(NAME_source, 0, "image|file*", NULL, getSourcePixmap,
     NAME_term, "Determine source for term representation")
};

/* Resources */

static classvardecl rc_pixmap[] =
{ RC(NAME_background, "colour", "white",
     "Default background colour"),
  RC(NAME_foreground, "colour", "black",
     "Default foreground colour")
};

/* Class Declaration */

static Name pixmap_termnames[] =
	{ NAME_source, NAME_foreground, NAME_background };

ClassDecl(pixmap_decls,
          var_pixmap, send_pixmap, get_pixmap, rc_pixmap,
          3, pixmap_termnames,
          "$Rev$");


status
makeClassPixmap(Class class)
{ return declareClass(class, &pixmap_decls);
}

