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
#include <h/graphics.h>
#include <h/arith.h>

static status
initialiseHandle(Handle h, Expression x, Expression y, Name kind, Name name)
{ if ( isDefault(kind) )
    kind = NAME_link;
  if ( isDefault(name) )
    name = kind;

  assign(h, xPosition, x);
  assign(h, yPosition, y);
  assign(h, kind, kind);
  assign(h, name, name);

  succeed;
}


status
getXYHandle(Handle h, Graphical gr, Device dev, Int *X, Int *Y)
{ Int x, y, gx, gy;

  if ( isDefault(dev) )
    dev = gr->device;

  TRY( get_absolute_xy_graphical(gr, &dev, &gx, &gy) );

  if ( X )
  { TRY(x = getValueExpression(h->xPosition,
			       VarW, gr->area->w,
			       VarH, gr->area->h,
			       EAV));
    *X = add(x, gx);
  }
  if ( Y )
  { TRY(y = getValueExpression(h->yPosition,
			       VarW, gr->area->w,
			       VarH, gr->area->h,
			       EAV));
    *Y = add(y, gy);
  }
  DEBUG(NAME_handle,
	Cprintf("handle %s on gr=%s,dev=%s at x=%s,y=%s\n",
		pp(h->name), pp(gr), pp(dev), X?"":pp(*X), Y?"":pp(*Y)));

  succeed;
}


static Point
getPositionHandle(Handle h, Graphical gr, Device dev)
{ Int x, y;

  TRY(getXYHandle(h, gr, dev, &x, &y));

  answer(answerObject(ClassPoint, x, y, EAV));
}


Int
getXHandle(Handle h, Graphical gr, Device dev)
{ Int x;

  TRY(getXYHandle(h, gr, dev, &x, NULL));

  answer(x);
}


Int
getYHandle(Handle h, Graphical gr, Device dev)
{ Int y;

  TRY(getXYHandle(h, gr, dev, NULL, &y));

  answer(y);
}


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */

static char *T_relative_toAgraphical_coordinate_system_ofADdeviceD[] =
        { "relative_to=graphical", "coordinate_system_of=[device]" };
static char *T_initialise[] =
        { "x=expression", "y=expression", "kind=[name]", "name=[name]" };

/* Instance Variables */

static vardecl var_handle[] =
{ IV(NAME_xPosition, "expression", IV_BOTH,
     NAME_location, "Expression for X in variable `w'"),
  IV(NAME_yPosition, "expression", IV_BOTH,
     NAME_location, "Expression for Y in variable `h'"),
  IV(NAME_kind, "name", IV_BOTH,
     NAME_relation, "Kind of valid connection end-point"),
  IV(NAME_name, "name", IV_BOTH,
     NAME_name, "Logical name of handle")
};

/* Send Methods */

static senddecl send_handle[] =
{ SM(NAME_initialise, 4, T_initialise, initialiseHandle,
     DEFAULT, "Create from X-, Y expression, kind and name")
};

/* Get Methods */

static getdecl get_handle[] =
{ GM(NAME_position, 2, "point", T_relative_toAgraphical_coordinate_system_ofADdeviceD, getPositionHandle,
     NAME_location, "New point with position on graphical relative to device"),
  GM(NAME_x, 2, "int", T_relative_toAgraphical_coordinate_system_ofADdeviceD, getXHandle,
     NAME_location, "X-position on graphical relative to device"),
  GM(NAME_y, 2, "int", T_relative_toAgraphical_coordinate_system_ofADdeviceD, getYHandle,
     NAME_location, "Y-position on graphical relative to device")
};

/* Resources */

#define rc_handle NULL
/*
static classvardecl rc_handle[] =
{
};
*/

/* Class Declaration */

static Name handle_termnames[] =
	{ NAME_xPosition, NAME_yPosition, NAME_kind, NAME_name };

ClassDecl(handle_decls,
          var_handle, send_handle, get_handle, rc_handle,
          4, handle_termnames,
          "$Rev$");

status
makeClassHandle(Class class)
{ declareClass(class, &handle_decls);

  succeed;
}

