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

status
initialiseMoveGesture(MoveGesture g, Name button, Modifier modifier)
{ initialiseGesture((Gesture) g, button, modifier);
  assign(g, offset, newObject(ClassPoint, ZERO, ZERO, EAV));

  succeed;
}


		/********************************
		*       GESTURE BEHAVIOUR	*
		********************************/

static status
verifyMoveGesture(MoveGesture g, EventObj ev)
{ Graphical gr = ev->receiver;

  if ( !instanceOfObject(gr, ClassGraphical) || isNil(gr->device) )
    fail;

  succeed;
}


static status
initiateMoveGesture(MoveGesture g, EventObj ev)
{ copyPoint(g->offset, getAreaPositionEvent(ev, DEFAULT));

  succeed;
}


static status
dragMoveGesture(MoveGesture g, EventObj ev)
{ Int x, y;

  get_xy_event(ev, get(ev->receiver, NAME_device, EAV), OFF, &x, &y);
  DEBUG(NAME_drag, writef("Receiver = %s; x = %d; y = %d\n",
			  ev->receiver, x, y));
  x = sub(x, g->offset->x);
  y = sub(y, g->offset->y);

  send(ev->receiver, NAME_doSet, x, y, EAV);

  succeed;
}


static status
terminateMoveGesture(MoveGesture g, EventObj ev)
{ return dragMoveGesture(g, ev);
}


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */

static char *T_initialise[] =
        { "button=[button_name]", "modifier=[modifier]" };

/* Instance Variables */

static vardecl var_moveGesture[] =
{ IV(NAME_offset, "point", IV_GET,
     NAME_internal, "Offset of down relative to object")
};

/* Send Methods */

static senddecl send_moveGesture[] =
{ SM(NAME_drag, 1, "event", dragMoveGesture,
     DEFAULT, "Drag to next position"),
  SM(NAME_initialise, 2, T_initialise, initialiseMoveGesture,
     DEFAULT, "Create from button and modifier"),
  SM(NAME_initiate, 1, "event", initiateMoveGesture,
     DEFAULT, "Initiate move"),
  SM(NAME_terminate, 1, "event", terminateMoveGesture,
     DEFAULT, "Finish the move"),
  SM(NAME_verify, 1, "event", verifyMoveGesture,
     DEFAULT, "Verify object moved is displayed on a device")
};

/* Get Methods */

#define get_moveGesture NULL
/*
static getdecl get_moveGesture[] =
{
};
*/

/* Resources */

static classvardecl rc_moveGesture[] =
{ RC(NAME_button, "button_name", "middle",
     "Active on which button (middle)"),
  RC(NAME_cursor, "cursor", "fleur",
     "Cursor while active")
};

/* Class Declaration */

static Name moveGesture_termnames[] = { NAME_button, NAME_modifier };

ClassDecl(moveGesture_decls,
          var_moveGesture, send_moveGesture, get_moveGesture, rc_moveGesture,
          2, moveGesture_termnames,
          "$Rev$");

status
makeClassMoveGesture(Class class)
{ return declareClass(class, &moveGesture_decls);
}

