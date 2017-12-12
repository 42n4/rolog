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


static status
initialiseHyper(Hyper h, Any from, Any to, Name fname, Name bname)
{ Any av[2];

  initialiseProgramObject(h);

  if ( isDefault(fname) )
    fname = NAME_hypered;
  if ( isDefault(bname) )
    bname = fname;

  assign(h, from, from);
  assign(h, to, to);
  assign(h, forward_name, fname);
  assign(h, backward_name, bname);

  av[0] = h;
  av[1] = to;

  TRY(sendv(from,  NAME_attachHyper, 2, av));
  av[1] = from;
  return sendv(to, NAME_attachHyper, 2, av);
}


static status
unlinkHyper(Hyper h)
{ if ( !onFlag(h->to, F_FREED|F_FREEING) )
    sendv(h->to, NAME_deleteHyper, 1, (Any *)&h);
  if ( !onFlag(h->from, F_FREED|F_FREEING) )
    sendv(h->from, NAME_deleteHyper, 1, (Any *)&h);

  succeed;
}


static status
unlinkFromHyper(Hyper h)
{ return freeObject(h);
}


static status
unlinkToHyper(Hyper h)
{ return freeObject(h);
}

		 /*******************************
		 *	    LOAD/SAVE		*
		 *******************************/

static status
SaveRelationHyper(Hyper h, FileObj f)
{ if ( isSavedObject(h->from) && isSavedObject(h->to) )
  { storeCharFile(f, 's');
    return storeObject(h, f);
  }

  succeed;
}


static status
loadHyper(Hyper h, IOSTREAM *fd, ClassDef def)
{ TRY(loadSlotsObject(h, fd, def));

  if ( restoreVersion >= 13 )
  { attachHyperObject(h->from, h, h->to);
    attachHyperObject(h->to, h, h->from);
  }

  succeed;
}


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */

static char *T_initialise[] =
        { "from=object", "to=object", "forward=[name]", "backward=[name]" };

/* Instance Variables */

static vardecl var_hyper[] =
{ IV(NAME_from, "object", IV_GET,
     NAME_client, "From side of hyper link"),
  IV(NAME_to, "object", IV_GET,
     NAME_client, "To side of hyper link"),
  IV(NAME_forwardName, "name", IV_BOTH,
     NAME_name, "Name as visible from <-from"),
  IV(NAME_backwardName, "name", IV_BOTH,
     NAME_name, "Name as visible from <-to")
};

/* Send Methods */

static senddecl send_hyper[] =
{ SM(NAME_initialise, 4, T_initialise, initialiseHyper,
     DEFAULT, "Create named link between objects"),
  SM(NAME_unlink, 0, NULL, unlinkHyper,
     DEFAULT, "Unlink hyper from objects"),
  SM(NAME_SaveRelation, 1, "file", SaveRelationHyper,
     NAME_file, "Consider saving relation (->save_in_file)"),
  SM(NAME_unlinkFrom, 0, NULL, unlinkFromHyper,
     NAME_internal, "<-from side is being unlinked"),
  SM(NAME_unlinkTo, 0, NULL, unlinkToHyper,
     NAME_internal, "<-to side is being unlinked")
};

/* Get Methods */

#define get_hyper NULL
/*
static getdecl get_hyper[] =
{
};
*/

/* Resources */

#define rc_hyper NULL
/*
static classvardecl rc_hyper[] =
{
};
*/

/* Class Declaration */

static Name hyper_termnames[] = { NAME_from, NAME_to, NAME_forwardName, NAME_backwardName };

ClassDecl(hyper_decls,
          var_hyper, send_hyper, get_hyper, rc_hyper,
          4, hyper_termnames,
          "$Rev$");

status
makeClassHyper(Class class)
{ declareClass(class, &hyper_decls);
  setLoadStoreFunctionClass(class, loadHyper, storeSlotsObject);

  succeed;
}


		 /*******************************
		 *	    CHAIN-HYPER		*
		 *******************************/

static status
unlinkFromChainHyper(Hyper h)
{ if ( isObject(h->to) && !isFreeingObj(h->to) )
  { if ( hasSendMethodObject(h->to, NAME_destroy) )
      send(h->to, NAME_destroy, EAV);
    else
      freeObject(h->to);
  }

  return freeObject(h);
}


static status
unlinkToChainHyper(Hyper h)
{ if ( isObject(h->from) && !isFreeingObj(h->from) )
  { if ( hasSendMethodObject(h->from, NAME_destroy) )
      send(h->from, NAME_destroy, EAV);
    else
      freeObject(h->from);
  }

  return freeObject(h);
}


/* Instance Variables */

#define var_chain_hyper NULL
/*
static vardecl var_chain_hyper[] =
{
};
*/

/* Send Methods */

static senddecl send_chain_hyper[] =
{ SM(NAME_unlinkFrom, 0, NULL, unlinkFromChainHyper,
     NAME_internal, "->free <-to"),
  SM(NAME_unlinkTo, 0, NULL, unlinkToChainHyper,
     NAME_internal, "->free <-from")
};

/* Get Methods */

#define get_chain_hyper NULL
/*
static getdecl get_chain_hyper[] =
{
};
*/

/* Resources */

#define rc_chain_hyper NULL
/*
static classvardecl rc_chain_hyper[] =
{
};
*/

/* Class Declaration */

ClassDecl(chain_hyper_decls,
          var_chain_hyper, send_chain_hyper, get_chain_hyper, rc_chain_hyper,
          0, NULL,
          "$Rev$");

status
makeClassChainHyper(Class class)
{ return declareClass(class, &chain_hyper_decls);
}







