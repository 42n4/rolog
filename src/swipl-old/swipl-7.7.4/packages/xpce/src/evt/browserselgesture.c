/*  Part of XPCE --- The SWI-Prolog GUI toolkit

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        jan@swi.psy.uva.nl
    WWW:           http://www.swi.psy.uva.nl/projects/xpce/
    Copyright (c)  1999-2011, University of Amsterdam
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

NewClass(browser_select_gesture)
  ABSTRACT_GESTURE
  Any	saved_selection;		/* Saved selection for cancel */
  BoolObj	scrolling;			/* We are redirecting events */
End;

typedef struct browser_select_gesture *BrowserSelectGesture;

static status
initialiseBrowserSelectGesture(BrowserSelectGesture g)
{ initialiseGesture((Gesture) g,
		    NAME_left,
		    newObject(ClassModifier, DEFAULT, DEFAULT, NAME_up, EAV));
  assign(g, scrolling, OFF);

  succeed;
}

		/********************************
		*       GESTURE BEHAVIOUR	*
		********************************/

static ListBrowser
get_list_browser(EventObj ev)
{ if ( instanceOfObject(ev->receiver, ClassListBrowser) )
    return (ListBrowser)ev->receiver;

  if ( instanceOfObject(ev->receiver, ClassBrowser) )
  { Browser b = (Browser)ev->receiver;

    return(b->list_browser);
  }

  fail;
}


static status
eventBrowserSelectGesture(BrowserSelectGesture g, EventObj ev)
{ status rval;
  ListBrowser lb = get_list_browser(ev);

					/* Handle the browsers scrollbar */
  if ( g->scrolling == ON )
  { send(lb->scroll_bar, NAME_event, ev, EAV);
    if ( isUpEvent(ev) )
      assign(g, scrolling, OFF);
    succeed;
  }
  if ( isDownEvent(ev) && insideEvent(ev, (Graphical)lb->scroll_bar) )
  { assign(g, scrolling, ON);
    send(lb->scroll_bar, NAME_event, ev, EAV);
    succeed;
  }

  rval = eventGesture(g, ev);

  if ( g->status == NAME_active &&
       (isAEvent(ev, NAME_locMove) ||
	isAEvent(ev, NAME_wheel)) )
  { send(g, NAME_drag, ev, EAV);
    succeed;
  }

  return rval;
}


static status
verifyBrowserSelectGesture(BrowserSelectGesture g, EventObj ev)
{ if ( get_list_browser(ev) )
    succeed;

  fail;
}


static status
selectBrowserSelectGesture(BrowserSelectGesture g, EventObj ev)
{ ListBrowser lb = get_list_browser(ev);
  DictItem di;

  if ( lb && (di = getDictItemListBrowser(lb, ev)) )
  { if ( lb->multiple_selection == OFF )
    { send(lb, NAME_changeSelection, NAME_set, di, EAV);
    } else
    { if ( valInt(ev->buttons) & BUTTON_shift )
      { send(lb, NAME_changeSelection, NAME_extend, di, EAV);
      } else if ( valInt(ev->buttons) & BUTTON_control )
      { send(lb, NAME_changeSelection, NAME_toggle, di, EAV);
      } else
	send(lb, NAME_changeSelection, NAME_set, di, EAV);
    }

    succeed;
  }

  fail;
}



static status
initiateBrowserSelectGesture(BrowserSelectGesture g, EventObj ev)
{ ListBrowser lb = get_list_browser(ev);

  if ( !lb )
    fail;

  if ( instanceOfObject(lb->selection, ClassChain) )
    assign(g, saved_selection, getCopyChain(lb->selection));
  else
    assign(g, saved_selection, lb->selection);

  if ( selectBrowserSelectGesture(g, ev) )
    succeed;

  send(lb, NAME_changeSelection, NAME_clear, EAV);

  succeed;
}


static status
dragBrowserSelectGesture(BrowserSelectGesture g, EventObj ev)
{ return selectBrowserSelectGesture(g, ev);
}


static status
terminateBrowserSelectGesture(BrowserSelectGesture g, EventObj ev)
{ ListBrowser lb = get_list_browser(ev);

  if ( lb )
  { if ( !insideEvent(ev, (Graphical)lb/*->image*/) ) /* cancel action */
    { send(lb, NAME_changeSelection, NAME_cancel, g->saved_selection, EAV);
    } else
    { if ( notNil(lb->open_message) &&
	   getMulticlickEvent(ev) == NAME_double )
	forwardListBrowser(lb, NAME_open);
      else
	forwardListBrowser(lb, NAME_select);
    }
  }

  assign(g, saved_selection, NIL);
  assign(g, scrolling, OFF);		/* make sure! */

  succeed;
}


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */

static vardecl var_browser_select_gesture[] =
{ IV(NAME_savedSelection, "dict_item|chain*", IV_GET,
     NAME_cancel, "Selection is saved on ->initiate"),
  IV(NAME_scrolling, "bool", IV_NONE,
     NAME_event, "If @on, redirect events to the scroll_bar")
};

/* Send Methods */

static senddecl send_browser_select_gesture[] =
{ SM(NAME_event, 1, "event", eventBrowserSelectGesture,
     DEFAULT, "Process an event"),
  SM(NAME_drag, 1, "event", dragBrowserSelectGesture,
     DEFAULT, "Drag to next position"),
  SM(NAME_initialise, 0, NULL, initialiseBrowserSelectGesture,
     DEFAULT, "Create from button and modifier"),
  SM(NAME_initiate, 1, "event", initiateBrowserSelectGesture,
     DEFAULT, "Initiate browser_select"),
  SM(NAME_terminate, 1, "event", terminateBrowserSelectGesture,
     DEFAULT, "Finish the browser_select"),
  SM(NAME_verify, 1, "event", verifyBrowserSelectGesture,
     DEFAULT, "Verify receiver is a list_browser")
};

/* Get Methods */

#define get_browser_select_gesture NULL
/*
static getdecl get_browser_select_gesture[] =
{
};
*/

/* Resources */

static classvardecl rc_browser_select_gesture[] =
{ RC(NAME_button, "button_name", "middle",
     "Active on which button (middle)"),
  RC(NAME_dragScroll, "{self,device,search}*", "self",
     "Scroll when dragging outside the area")
};

/* Class Declaration */

ClassDecl(browser_select_gesture_decls,
          var_browser_select_gesture,
	  send_browser_select_gesture, get_browser_select_gesture,
	  rc_browser_select_gesture,
          0, NULL,
          "$Rev$");

status
makeClassBrowserSelectGesture(Class class)
{ return declareClass(class, &browser_select_gesture_decls);
}

