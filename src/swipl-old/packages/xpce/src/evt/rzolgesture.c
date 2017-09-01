/*  Part of XPCE --- The SWI-Prolog GUI toolkit

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        jan@swi.psy.uva.nl
    WWW:           http://www.swi.psy.uva.nl/projects/xpce/
    Copyright (c)  1995-2011, University of Amsterdam
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

static status
initialiseResizeOutlineGesture(ResizeOutlineGesture g, Name button,
			       Modifier modifier)
{ initialiseResizeGesture((ResizeGesture) g, button, modifier);
  obtainClassVariablesObject(g);
  assign(g, outline, newObject(ClassBox, EAV));
  assign(g, outline_gesture, newObject(ClassResizeGesture,
				       g->button, g->modifier, EAV));

  send(g->outline, NAME_texture,
       getClassVariableValueObject(g, NAME_texture), EAV);

  succeed;
}


		/********************************
		*       GESTURE BEHAVIOUR	*
		********************************/

static status
initiateResizeOutlineGesture(ResizeOutlineGesture g, EventObj ev)
{ Graphical gr = ev->receiver;

  if ( !instanceOfObject(gr, ClassGraphical) )
    fail;
  send(g->outline, NAME_area, gr->area, EAV);
  send(gr->device, NAME_display, g->outline, EAV);
  postEvent(ev, (Graphical) g->outline, (Recogniser) g->outline_gesture);

  succeed;
}


static status
dragResizeOutlineGesture(ResizeOutlineGesture g, EventObj ev)
{ postEvent(ev, (Graphical) g->outline, (Recogniser) g->outline_gesture);

  succeed;
}


static status
terminateResizeOutlineGesture(ResizeOutlineGesture g, EventObj ev)
{ Area a;

  postEvent(ev, (Graphical) g->outline, (Recogniser) g->outline_gesture);

  a = g->outline->area;
  send(ev->receiver, NAME_doSet, a->x, a->y, a->w, a->h, EAV);
  send(g->outline, NAME_device, NIL, EAV);

  succeed;
}


		/********************************
		*           ATTRIBUTES		*
		********************************/

static status
minSizeResizeOulineGesture(ResizeOutlineGesture g, Size sz)
{ assign(g->outline_gesture, min_size, sz);
  succeed;
}


static status
maxSizeResizeOulineGesture(ResizeOutlineGesture g, Size sz)
{ assign(g->outline_gesture, max_size, sz);
  succeed;
}


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */

static char *T_initialise[] =
        { "button=[button_name]", "modifier=[modifier]" };

/* Instance Variables */

static vardecl var_resizeOutlineGesture[] =
{ IV(NAME_outline, "box", IV_GET,
     NAME_feedback, "The outline resized"),
  IV(NAME_outlineGesture, "resize_gesture", IV_GET,
     NAME_internal, "The outline resized")
};

/* Send Methods */

static senddecl send_resizeOutlineGesture[] =
{ SM(NAME_initialise, 2, T_initialise, initialiseResizeOutlineGesture,
     DEFAULT, "Create from button and modifier"),
  SM(NAME_drag, 1, "event", dragResizeOutlineGesture,
     NAME_event, "Drag outline to next position"),
  SM(NAME_initiate, 1, "event", initiateResizeOutlineGesture,
     NAME_event, "Display outline and change cursor"),
  SM(NAME_terminate, 1, "event", terminateResizeOutlineGesture,
     NAME_event, "Resize object and undisplay outline"),
  SM(NAME_maxSize, 1, "size*", maxSizeResizeOulineGesture,
     NAME_constraint, "Specify maximum size of the graphical"),
  SM(NAME_minSize, 1, "size*", minSizeResizeOulineGesture,
     NAME_constraint, "Specify minimum size of the graphical")
};

/* Get Methods */

#define get_resizeOutlineGesture NULL
/*
static getdecl get_resizeOutlineGesture[] =
{
};
*/

/* Resources */

static classvardecl rc_resizeOutlineGesture[] =
{ RC(NAME_button, "button_name", "middle",
     "Active on which button (middle)"),
  RC(NAME_texture, "texture_name", "dotted",
     "Texture of the outline box")
};

/* Class Declaration */

static Name resizeOutlineGesture_termnames[] = { NAME_button, NAME_modifier };

ClassDecl(resizeOutlineGesture_decls,
          var_resizeOutlineGesture, send_resizeOutlineGesture,
	  get_resizeOutlineGesture, rc_resizeOutlineGesture,
          2, resizeOutlineGesture_termnames,
          "$Rev$");


status
makeClassResizeOutlineGesture(Class class)
{ return declareClass(class, &resizeOutlineGesture_decls);
}
