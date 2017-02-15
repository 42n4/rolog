/*  Part of XPCE --- The SWI-Prolog GUI toolkit

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        jan@swi.psy.uva.nl
    WWW:           http://www.swi.psy.uva.nl/projects/xpce/
    Copyright (c)  1998-2011, University of Amsterdam
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
#include <h/graphics.h>
#include <h/text.h>

NewClass(paragraph)
  ABSTRACT_DEVICE
  Font		font;			/* Font (override) */
  Style		style;			/* style (override) */
  Int		start_indent;		/* indent */
  Int		first_line_indent;	/* indent of first line */
  Int		space_before;		/* Space above text */
  Int		space_after;		/* Space below text */
  Name		quadding;		/* left,center,right */
  BoolObj		justify;		/* justify text? */
  TextBuffer	text_buffer;		/* contained text and graphics */
End;

typedef struct
{ int		ascent;			/* height above baseline */
  int		decent;			/* height below baseline */
  int		x;			/* X-position */
  int		w;			/* total width */
  lineitem     *items;			/* list of items */
} line;

typedef struct
{ FontObj	font;			/* font of the item */
  Style		style;			/* its style */
  String	text;			/* represented text */
  int		x;			/* X-position */
} lineitem;


		 /*******************************
		 *	      CREATE		*
		 *******************************/

static status
initialiseParagraph(Paragraph par, TextBuffer tb)
{ initialiseDevice(par);

  if ( isDefault(tb) )
    tb = newObject(ClassTextBuffer, EAV);

  assign(c, text_buffer, tb);

  return obtainClassVariablesObject(par);
}

		 /*******************************
		 *	    PLACEMENT		*
		 *******************************/










		 /*******************************
		 *	      COMPUTE		*
		 *******************************/

static status
computeParagraph(Paragraph par)
{ if ( notNil(par->request_compute) )
  {

    assign(par, request_compute, NIL);
  }

  succeed;
}

		 /*******************************
		 *	      REDRAW		*
		 *******************************/

static status
RedrawAreaParagraph(Paragraph par, Area area)
{
}


		 /*******************************
		 *	    ATTRIBUTES		*
		 *******************************/

static status
assignParagraph(Paragraph par, Name slot, Any value)
{ Class class = classOfObject(par);
  Variable var;

  if ( (var = getInstanceVariableClass(class, (Any) slot)) )
  { if ( getGetVariable(var, par, 0, NULL) != value )
    { setSlotInstance(tab, var, value);
      requestComputeDevice(par, DEFAULT);
    }

    succeed;
  }

  fail;
}


static status
startIndentParagraph(Paragraph par, Int val)
{ return assignParagraph(par, NAME_startIndent, val);
}


static status
firstLineIndentParagraph(Paragraph par, Int val)
{ return assignParagraph(par, NAME_firstLineIndent, val);
}


static status
spaceBeforeParagraph(Paragraph par, Int val)
{ return assignParagraph(par, NAME_spaceBefore, val);
}


static status
spaceAfterParagraph(Paragraph par, Int val)
{ return assignParagraph(par, NAME_spaceAfter, val);
}


static status
styleParagraph(Paragraph par, Style val)
{ return assignParagraph(par, NAME_style, val);
}


static status
fontParagraph(Paragraph par, Font val)
{ return assignParagraph(par, NAME_font, val);
}


static status
justifyParagraph(Paragraph par, BoolObj justify)
{ return assignParagraph(par, NAME_justify, justify);
}


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */

/* Instance Variables */

static vardecl var_paragraph[] =
{ SV(NAME_font, "[font]", IV_GET|IV_STORE, fontParagraph,
     NAME_appearance, "Default font for text"),
  SV(NAME_style, "[style]", IV_GET|IV_STORE, styleParagraph,
     NAME_appearance, "Default style attributes for text"),
  SV(NAME_startIndent, "0..", IV_GET|IV_STORE, startIndentParagraph,
     NAME_layout, "Indentation of lines"),
  SV(NAME_firstLineIndent, "int", IV_GET|IV_STORE, firstLineIndentParagraph,
     NAME_layout, "Indentation of first line"),
  SV(NAME_spaceBefore, "0..", IV_GET|IV_STORE, spaceBeforeParagraph,
     NAME_layout, "Vertical space above paragraph"),
  SV(NAME_spaceAfter, "0..", IV_GET|IV_STORE, spaceAfterParagraph,
     NAME_layout, "Vertical space below paragraph"),
  SV(NAME_quadding, "[{left,center,right}]", IV_GET|IV_STORE,
     quaddingParagraph,
     NAME_appearance, "Alignment of text in paragraph"),
  SV(NAME_justify, "bool", IV_GET|IV_STORE, justifyParagraph,
     NAME_layout, "Align all lines at the right"),
  IV(NAME_content,  "chain", IV_GET,
     NAME_contents, "Text and graphics represented")
};

/* Send Methods */

static senddecl send_paragraph[] =
{ SM(NAME_initialise, 0, NULL, initialiseParagraph,
     DEFAULT, "Initialise paragraph"),
  SM(NAME_append, 1, "char_array|graphical", appendParagraph,
     DEFAULT, "Append text or graphical to the paragraph layout"),
  SM(NAME_compute, 0, NULL, computeParagraph,
     DEFAULT, "Compute the layout of the member graphicals")
};

/* Get Methods */

#define get_paragraph NULL
/*
static getdecl get_paragraph[] =
{
};
*/

/* Resources */

static classvardecl rc_paragraph[] =
{
  RC(NAME_font,		   NULL, "@default", NULL),
  RC(NAME_startIndent,	   NULL, "0",	     NULL),
  RC(NAME_firstLineIndent, NULL, "0",	     NULL),
  RC(NAME_quadding,	   NULL, "@default", NULL),
  RC(NAME_spaceBefore,	   NULL, "0",	     NULL),
  RC(NAME_spaceAfter,	   NULL, "0",	     NULL),
  RC(NAME_justify,	   NULL, "@off",     NULL)
};

/* Class Declaration */

static Name paragraph_termnames[] = { NAME_image };

ClassDecl(paragraph_decls,
          var_paragraph,
	  send_paragraph,
	  get_paragraph,
	  rc_paragraph,
          1, paragraph_termnames,
          "$Rev$");

status
makeClassParagraph(Class class)
{ return declareClass(class, &paragraph_decls);
}

