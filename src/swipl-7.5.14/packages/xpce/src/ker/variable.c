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
#include <h/trace.h>
#include <h/text.h>

static status	initialiseVariable(Variable var, Name name, Type type,
				   Name access, StringObj doc, Name group,
				   Any initial);
static status	typeVariable(Variable var, Type type);

Variable
createVariable(Name name, Type type, Name access)
{ Variable var;

  var = alloc(sizeof(struct variable));
  initHeaderObj(var, ClassObjOfVariable);
  var->name          = var->access = (Name) NIL;
  var->group	     = NIL;
  var->offset        = (Int) NIL;
  var->type	     = (Type) NIL;
  var->dflags	     = (uintptr_t) ZERO;
  var->context	     = NIL;
  var->summary       = NIL;
  var->init_function = NIL;
  var->alloc_value   = NIL;

  TRY(initialiseVariable(var, name, type, access, DEFAULT, DEFAULT, DEFAULT));
  createdObject(var, NAME_new);

  return var;
}


static status
initialiseVariable(Variable var, Name name, Type type, Name access,
		   StringObj doc, Name group, Any initial)
{ initialiseBehaviour((Behaviour) var, name, NIL);

  if ( isDefault(type) )   type   = TypeAny;
  if ( isDefault(access) ) access = NAME_both;
  if ( isDefault(doc) )    doc    = NIL;

  assign(var, group,   group);
  assign(var, access,  access);
  assign(var, offset,  ZERO);
  assign(var, summary, doc);

  var->alloc_value = NIL;
  typeVariable(var, type);
  if ( notDefault(initial) )
    initialValueVariable(var, initial);
  else
  { if ( !includesType(type, TypeNil) &&
	 includesType(type, TypeDefault) )
      initialValueVariable(var, DEFAULT);
  }

  succeed;
}


static status
typeVariable(Variable var, Type type)
{ assign(var, type, type);
  clearDFlag(var, D_CLONE|D_SAVE);

  if ( type->kind == NAME_alien )
  { setDFlag(var, D_CLONE_ALIEN|D_ALIEN);
    var->alloc_value = NULL;
  } else
  { setDFlag(var, D_SAVE_NORMAL);
    setDFlag(var, D_CLONE_RECURSIVE);
  }

  succeed;
}


status
cloneStyleVariable(Variable var, Name style)
{ clearDFlag(var, D_CLONE);

  if ( style == NAME_recursive )
    setDFlag(var, D_CLONE_RECURSIVE);
  else if ( style == NAME_reference )
    setDFlag(var, D_CLONE_REFERENCE);
  else if ( style == NAME_value )
    setDFlag(var, D_CLONE_VALUE);
  else if ( style == NAME_alien )
    setDFlag(var, D_CLONE_ALIEN);
  else if ( style == NAME_nil )
    setDFlag(var, D_CLONE_NIL);
  else if ( style == NAME_referenceChain )
    setDFlag(var, D_CLONE_REFCHAIN);
  else
    fail;

  succeed;
}


status
saveStyleVariable(Variable var, Name style)
{ clearDFlag(var, D_SAVE);

  if ( style == NAME_normal )
    setDFlag(var, D_SAVE_NORMAL);
  else if ( style == NAME_nil )
    setDFlag(var, D_SAVE_NIL);
  else
    fail;

  succeed;
}


static Name
getCloneStyleVariable(Variable var)
{ if ( onDFlag(var, D_CLONE_RECURSIVE) )
    answer(NAME_recursive);
  if ( onDFlag(var, D_CLONE_REFERENCE) )
    answer(NAME_reference);
  if ( onDFlag(var, D_CLONE_REFCHAIN) )
    answer(NAME_referenceChain);
  if ( onDFlag(var, D_CLONE_VALUE) )
    answer(NAME_value);
  if ( onDFlag(var, D_CLONE_ALIEN) )
    answer(NAME_alien);
  if ( onDFlag(var, D_CLONE_NIL) )
    answer(NAME_nil);

  fail;
}


static Name
getSaveStyleVariable(Variable var)
{ if ( onDFlag(var, D_SAVE_NORMAL) )
    answer(NAME_normal);
  if ( onDFlag(var, D_SAVE_NIL) )
    answer(NAME_nil);

  fail;
}


status
sendAccessVariable(Variable var)
{ if ( var->access == NAME_both || var->access == NAME_send )
    succeed;
  fail;
}


status
getAccessVariable(Variable var)
{ if ( var->access == NAME_both || var->access == NAME_get )
    succeed;
  fail;
}


static Type
getArgumentTypeVariable(Variable var, Int n)
{ if ( sendAccessVariable(var) && (isDefault(n) || n == ONE) )
    answer(var->type);

  fail;
}


static Type
getReturnTypeVariable(Variable var)
{ if ( getAccessVariable(var) )
    answer(var->type);

  fail;
}

		 /*******************************
		 *	   INITIAL VALUE	*
		 *******************************/

static status
allocValueVariable(Variable var, Any value)
{ Any old = var->alloc_value;

  var->alloc_value = value;
  if ( isObject(value) && !isProtectedObj(value) )
    addRefObject(var, value);
  if ( isObject(old) && !isProtectedObj(old) )
    delRefObject(var, old);

  succeed;
}


static Any
getAllocValueVariable(Variable var)
{ answer(var->alloc_value);		/* alien = NULL --> fail */
}


static status
initFunctionVariable(Variable var, Any f)
{ assign(var, init_function, f);

  if ( instanceOfObject(var->context, ClassClass) )
    unallocInstanceProtoClass(var->context);

  succeed;
}


static int
is_shareable(Any value)
{ if ( instanceOfObject(value, ClassConstant) ||
       instanceOfObject(value, ClassName) ||
       isInteger(value) )
    succeed;

  fail;
}


status
initialValueVariable(Variable var, Any value)
{ if ( is_shareable(value) )
  { Any val = checkType(value, var->type, NIL);

    if ( !val )
      return errorPce(value, NAME_unexpectedType, var->type);

    if ( val == value || is_shareable(val) ) /* still the case? */
    { allocValueVariable(var, val);
      initFunctionVariable(var, NIL);

      succeed;
    } else
      value = val;
  }

  allocValueVariable(var, NIL);
  initFunctionVariable(var, value);

  succeed;
}


		/********************************
		*          EXECUTION		*
		********************************/

status
sendVariable(Variable var, Any rec, Any val)
{ Any value;
  Any *field = &(((Instance)rec)->slots[valInt(var->offset)]);

  if ( !(value = checkType(val, var->type, rec)) )
    return errorTypeMismatch(rec, var, 1, var->type, val);

  assignField(rec, field, value);

  succeed;
}


Any
getGetVariable(Variable var, Any rec)
{ Any *field = &(((Instance)rec)->slots[valInt(var->offset)]);
  Any rval = *field;

  if ( isClassDefault(rval) )
  { Any value;

    if ( (value = getClassVariableValueObject(rec, var->name)) )
    { Any v2;

      if ( (v2 = checkType(value, var->type, rec)) )
      { assignField(rec, field, v2);
	answer(v2);
      } else
      { errorPce(var, NAME_incompatibleClassVariable, 0);
	fail;
      }
    } else if ( instanceOfObject(rec, ClassClass) &&
		((Class)rec)->realised != ON )
    { realiseClass(rec);
      rval = *field;
    } else
    { errorPce(var, NAME_noClassVariable, 0);
      fail;
    }
  }

  answer(rval);
}


Name
getGroupVariable(Variable v)
{ if ( isDefault(v->group) )
  { Class class = v->context;

    TRY( instanceOfObject(class, ClassClass) );
    for( class = class->super_class; notNil(class); class = class->super_class)
    { Vector vector = class->instance_variables;
      int n;

      for(n=0; n<valInt(vector->size); n++)
      { Variable var = vector->elements[n];

	if ( var->name == v->name && notDefault(var->group) )
	  answer(var->group);
      }
    }

    fail;
  }

  answer(v->group);
}

		/********************************
		*        MANUAL SUPPORT		*
		********************************/

static Name
getAccessArrowVariable(Variable v)
{ if ( v->access == NAME_none ) return CtoName("-");
  if ( v->access == NAME_get  ) return CtoName("<-");
  if ( v->access == NAME_send ) return CtoName("->");
  if ( v->access == NAME_both ) return CtoName("<->");

  fail;
}


static Name
getContextNameVariable(Variable v)
{ if ( instanceOfObject(v->context, ClassClass) )
  { Class class = v->context;

    answer(class->name);
  }

  answer(CtoName("???"));
}


#ifndef O_RUNTIME
static Name
getManIdVariable(Variable v)
{ wchar_t buf[LINESIZE];
  wchar_t *nm, *o;
  Name ctx = getContextNameVariable(v);
  size_t len;
  Name rc;

  len = 4 + ctx->data.s_size + v->name->data.s_size;
  if ( len < LINESIZE )
    nm = buf;
  else
    nm = pceMalloc(sizeof(wchar_t)*len);

  o = nm;
  *o++ = 'V';
  *o++ = '.';
  wcscpy(o, nameToWC(ctx, &len));
  o += len;
  *o++ = '.';
  wcscpy(o, nameToWC(v->name, &len));
  o += len;

  rc = WCToName(nm, o-nm);
  if ( nm != buf )
    pceFree(nm);

  answer(rc);
}


static Name
getManIndicatorVariable(Variable v)
{ answer(CtoName("V"));
}


static StringObj
getManSummaryVariable(Variable v)
{ TextBuffer tb;
  StringObj str;

  tb = newObject(ClassTextBuffer, EAV);
  tb->undo_buffer_size = ZERO;
  CAppendTextBuffer(tb, "V\t");

  if ( instanceOfObject(v->context, ClassClass) )
  { Class class = v->context;

    appendTextBuffer(tb, (CharArray)class->name, ONE);
    CAppendTextBuffer(tb, " ");
  }

  appendTextBuffer(tb, (CharArray)getAccessArrowVariable(v), ONE);
  appendTextBuffer(tb, (CharArray)v->name, ONE);
  CAppendTextBuffer(tb, ": ");
  appendTextBuffer(tb, (CharArray)v->type->fullname, ONE);
  if ( notNil(v->summary) )
  { CAppendTextBuffer(tb, "\t");
    appendTextBuffer(tb, (CharArray)v->summary, ONE);
  }
  if ( send(v, NAME_hasHelp, EAV) )
    CAppendTextBuffer(tb, " (+)");

  str = getContentsTextBuffer(tb, ZERO, DEFAULT);
  doneObject(tb);

  answer(str);
}
#endif /*O_RUNTIME*/


static Name
getPrintNameVariable(Variable var)
{ wchar_t buf[LINESIZE];
  wchar_t *nm, *o;
  Name ctx = getContextNameVariable(var);
  size_t len;
  Name rc;

  len = 5 + ctx->data.s_size + var->name->data.s_size;
  if ( len < LINESIZE )
    nm = buf;
  else
    nm = pceMalloc(sizeof(wchar_t)*len);

  o = nm;
  wcscpy(o, nameToWC(ctx, &len)); o += len;
  *o++ = ' ';
  wcscpy(o, nameToWC(getAccessArrowVariable(var), &len)); o += len;
  wcscpy(o, nameToWC(var->name, &len)); o += len;

  rc = WCToName(nm, o-nm);
  if ( nm != buf )
    pceFree(nm);

  answer(rc);
}


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declaractions */

static char *T_initialise[] =
        { "name=name", "type=[type]", "access=[{none,send,get,both}]",
	  "summary=[string]*", "group=[name]",
	  "initial_value=[any|function]" };
static char *T_send[] =
        { "receiver=object", "value=unchecked" };

/* Instance Variables */

static vardecl var_variable[] =
{ IV(NAME_group, "[name]", IV_NONE,
     NAME_manual, "Conceptual group of variable"),
  IV(NAME_access, "{none,send,get,both}", IV_GET,
     NAME_behaviour, "Read/write access"),
  SV(NAME_type, "type", IV_GET|IV_STORE, typeVariable,
     NAME_type, "Type check"),
  IV(NAME_offset, "int", IV_GET,
     NAME_storage, "Offset in instance structure"),
  IV(NAME_summary, "string*", IV_BOTH,
     NAME_manual, "Summary documentation"),
  SV(NAME_initFunction, "any*", IV_BOTH|IV_STORE, initFunctionVariable,
     NAME_oms, "Function to initialise the variable"),
  IV(NAME_allocValue, "alien:void *", IV_BOTH,
     NAME_oms, "Value used to when allocating")
};

/* Send Methods */

static senddecl send_variable[] =
{ SM(NAME_initialise, 6, T_initialise, initialiseVariable,
     DEFAULT, "Create from name, type, access, doc, group and initial value"),
  SM(NAME_cloneStyle, 1, "{recursive,reference,reference_chain,value,alien,nil}", cloneStyleVariable,
     NAME_copy, "Clone-style for this slot"),
  SM(NAME_send, 2, T_send, sendVariable,
     NAME_execute, "Invoke (write) variable in object"),
  SM(NAME_saveStyle, 1, "{normal,nil}", saveStyleVariable,
     NAME_file, "Slot saved as @nil or its value"),
  SM(NAME_getAccess, 0, NULL, getAccessVariable,
     NAME_meta, "Test if variable has read access"),
  SM(NAME_sendAccess, 0, NULL, sendAccessVariable,
     NAME_meta, "Test if variable has write access"),
  SM(NAME_allocValue, 1, "any|function", initialValueVariable,
     NAME_oms, "Value after allocation when instantiated"),
  SM(NAME_initialValue, 1, "any|function", initialValueVariable,
     NAME_oms, "Initial value for this variable")
};

/* Get Methods */

static getdecl get_variable[] =
{ GM(NAME_cloneStyle, 0, "name", NULL, getCloneStyleVariable,
     NAME_copy, "Clone style for this slot"),
  GM(NAME_get, 1, "unchecked", "object", getGetVariable,
     NAME_execute, "Invoke (read) variable in object"),
  GM(NAME_saveStyle, 0, "{normal,nil}", NULL, getSaveStyleVariable,
     NAME_file, "Save style for this slot"),
  GM(NAME_accessArrow, 0, "{-,<-,->,<->}", NULL, getAccessArrowVariable,
     NAME_manual, "Arrow indicating access-rights"),
  GM(NAME_contextName, 0, "name", NULL, getContextNameVariable,
     NAME_manual, "Name of context class"),
  GM(NAME_group, 0, "name", NULL, getGroupVariable,
     NAME_manual, "(Possible inherited) group-name"),
#ifndef O_RUNTIME
  GM(NAME_manId, 0, "name", NULL, getManIdVariable,
     NAME_manual, "Card Id for variable"),
  GM(NAME_manIndicator, 0, "name", NULL, getManIndicatorVariable,
     NAME_manual, "Manual type indicator (`V')"),
  GM(NAME_manSummary, 0, "string", NULL, getManSummaryVariable,
     NAME_manual, "New string with summary"),
#endif /*O_RUNTIME*/
  GM(NAME_argumentType, 1, "type", "index=[int]", getArgumentTypeVariable,
     NAME_meta, "Type of n-th1 argument if <-access includes `send'"),
  GM(NAME_returnType, 0, "type", NULL, getReturnTypeVariable,
     NAME_meta, "Return type if <-access includes `get'"),
  GM(NAME_allocValue, 0, "unchecked", NULL, getAllocValueVariable,
     NAME_oms, "Initial value when instantiated"),
  GM(NAME_printName, 0, "name", NULL, getPrintNameVariable,
     NAME_textual, "Class <->Name")
};

/* Resources */

#define rc_variable NULL
/*
static classvardecl rc_variable[] =
{
};
*/

/* Class Declaration */

static Name variable_termnames[] = { NAME_name, NAME_type, NAME_access };

ClassDecl(variable_decls,
          var_variable, send_variable, get_variable, rc_variable,
          3, variable_termnames,
          "$Rev$");


status
makeClassVariable(Class class)
{ declareClass(class, &variable_decls);

  succeed;
}
