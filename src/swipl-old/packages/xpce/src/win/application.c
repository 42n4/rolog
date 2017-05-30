/*  Part of XPCE --- The SWI-Prolog GUI toolkit

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        jan@swi.psy.uva.nl
    WWW:           http://www.swi.psy.uva.nl/projects/xpce/
    Copyright (c)  1996-2011, University of Amsterdam
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

static Chain TheApplications;

static status
initialiseApplication(Application app, Name name)
{ assign(app, name,    name);
  assign(app, members, newObject(ClassChain, EAV));
  assign(app, kind,    NAME_user);
  assign(app, modal,   newObject(ClassChain, EAV));

  appendChain(TheApplications, app);

  succeed;
}


static status
unlinkApplication(Application app)
{ if ( notNil(app->members) )
  { FrameObj fr;

    for_chain(app->members, fr, send(fr, NAME_destroy, EAV));
  }

  deleteChain(TheApplications, app);

  succeed;
}


static status
leaderApplication(Application app, FrameObj leader)
{ if ( app->leader != leader )
  { if ( notNil(app->leader) )
      send(app, NAME_delete, app->leader, EAV);
    if ( notNil(leader->application) )
      send(leader->application, NAME_delete, leader, EAV);
    assign(app, leader, leader);
    assign(leader, application, app);
  }

  succeed;
}


static status
appendApplication(Application app, FrameObj fr)
{ if ( fr->application != app )
  { if ( notNil(fr->application) )
      send(fr->application, NAME_delete, fr, EAV);

    assign(fr, application, app);
    appendChain(app->members, fr);
    if ( fr->modal == NAME_application )
      send(app, NAME_modal, fr, EAV);
  }

  succeed;
}


static status
deleteApplication(Application app, FrameObj fr)
{ if ( onFlag(app, F_FREED|F_FREEING) )
    succeed;

  if ( fr->application == app )
  { deleteChain(app->members, fr);
    assign(fr, application, NIL);
    deleteChain(app->modal, fr);
    if ( app->leader == fr )
      assign(app, leader, NIL);
    succeed;
  }

  fail;
}


static status
firstApplication(Application app, FrameObj fr)
{ if ( fr->application == app )
  { addCodeReference(fr);
    deleteChain(app->members, fr);
    prependChain(app->members, fr);
    delCodeReference(fr);

    succeed;
  }

  fail;
}


static FrameObj
getMemberApplication(Application app, Name name)
{ Cell cell;

  for_cell(cell, app->members)
  { FrameObj fr = cell->value;

    if ( fr->name == name )
      answer(fr);
  }

  fail;
}


static Chain
getContainsApplication(Application app)
{ answer(app->members);
}


static status
resetApplication(Application app)
{ assign(app, modal, NIL);

  succeed;
}


void
resetApplications()
{ if ( TheApplications )
  { Application app;

    for_chain(TheApplications, app, send(app, NAME_reset, EAV));
  }
}


static status
modalApplication(Application app, FrameObj fr)
{ if ( notNil(fr) )
  { if ( fr->application != app )
    { TRY(send(fr, NAME_application, app, EAV));
    }

    prependChain(app->modal, fr);
  }

  succeed;
}


static FrameObj
getModalApplication(Application app)
{ if ( instanceOfObject(app->modal, ClassChain) )
    answer(getHeadChain(app->modal));

  fail;
}


/* Type declaractions */

/* Instance Variables */

static vardecl var_application[] =
{ IV(NAME_name, "name", IV_BOTH,
     NAME_name, "Identifier name of the application"),
  SV(NAME_leader, "frame*", IV_GET|IV_STORE, leaderApplication,
     NAME_organisation, "Leading frame of the application"),
  IV(NAME_members, "chain", IV_GET,
     NAME_organisation, "Chain holding member frames"),
  IV(NAME_kind, "{user,service}", IV_BOTH,
     NAME_debugging, "If service, events cannot be debugged"),
  IV(NAME_modal, "chain", IV_NONE,
     NAME_event, "Frame for modal operation")
};

/* Send Methods */

static senddecl send_application[] =
{ SM(NAME_initialise, 1, "name", initialiseApplication,
     DEFAULT, "Create named application"),
  SM(NAME_unlink, 0, NULL, unlinkApplication,
     DEFAULT, "->destroy frames and remove from @applications"),
  SM(NAME_append, 1, "frame", appendApplication,
     NAME_organisation, "Add frame to the application"),
  SM(NAME_delete, 1, "frame", deleteApplication,
     NAME_organisation, "Remove frame from the application"),
  SM(NAME_first, 1, "frame", firstApplication,
     NAME_organisation, "Move frame to be the first in <-members"),
  SM(NAME_reset, 0, NULL, resetApplication,
     DEFAULT, "Reset <-modal to @nil"),
  SM(NAME_modal, 1, "frame", modalApplication,
     NAME_event, "Make frame modal to application")
};

/* Get Methods */

static getdecl get_application[] =
{ GM(NAME_member, 1, "frame", "name", getMemberApplication,
     NAME_organisation, "Member frame with given name"),
  GM(NAME_contains,  0, "chain", NULL, getContainsApplication,
     NAME_organisation, "Chain with frames I manage"),
  GM(NAME_modal,  0, "frame", NULL, getModalApplication,
     NAME_event, "Frame that is modal to the application")
};

/* Resources */

#define rc_application NULL
/*
static classvardecl rc_application[] =
{
};
*/

/* Class Declaration */

static Name application_termnames[] = { NAME_name };

ClassDecl(application_decls,
          var_application, send_application, get_application, rc_application,
          1, application_termnames,
          "$Rev$");

status
makeClassApplication(Class class)
{ declareClass(class, &application_decls);

  TheApplications = globalObject(NAME_applications, ClassChain, EAV);

  succeed;
}
