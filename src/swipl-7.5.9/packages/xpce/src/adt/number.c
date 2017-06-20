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

Number
CtoNumber(long i)
{ Number n = answerObject(ClassNumber, ZERO, EAV);

  n->value = i;
  answer(n);
}


static long
valArg(Any i)
{ if ( isInteger(i) )
    return valInt(i);
  else
  { Number n = i;
    return n->value;
  }
}


static status
initialiseNumber(Number n, Any i)
{ n->value = valArg(i);

  succeed;
}


static Number
getCopyNumber(Number n)
{ Number n2 = answerObject(ClassNumber, ZERO, EAV);

  n2->value = n->value;
  answer(n2);
}


static status
storeNumber(Number n, FileObj file)
{ TRY(storeSlotsObject(n, file));

  return storeWordFile(file, (Any)n->value);
}


static status
loadNumber(Number n, IOSTREAM *fd, ClassDef def)
{ TRY(loadSlotsObject(n, fd, def));
  if ( restoreVersion >= 16 )
    n->value = loadWord(fd);
  succeed;
}


static status
convertOldSlotNumber(Number n, Name name, Any value)
{ if ( name == NAME_value && isInteger(value) )
  { n->value = valInt(value);
    succeed;
  }

  fail;
}


static Number
getConvertNumber(Class class, Any obj)
{ Int i;

  TRY( i = toInteger(obj) );
  answer( answerObject(ClassNumber, i, EAV) );
}


static StringObj
getPrintNameNumber(Number n)
{ char buf[100];

  sprintf(buf, INTPTR_FORMAT, n->value);

  answer(CtoString(buf));
}


static Name
getCompareNumber(Number n, Any i)
{ long v;

  if ( isInteger(i) )
    v = valInt(i);
  else if ( instanceOfObject(i, ClassNumber) )
  { Number n = i;
    v = n->value;
  } else
  { double va = valReal(i);
    double vm = (double) n->value;

    answer(vm > va ? NAME_larger : vm < va ? NAME_smaller : NAME_equal);
  }

  if ( n->value > v )
    answer(NAME_larger);
  if ( n->value < v )
    answer(NAME_smaller);
  answer(NAME_equal);
}


static status
equalNumber(Number n, Any i)
{ if ( getCompareNumber(n, i) == NAME_equal )
    succeed;
  fail;
}


static status
notEqualNumber(Number n, Any i)
{ if ( getCompareNumber(n, i) == NAME_equal )
    fail;
  succeed;
}


static status
smallerNumber(Number n, Int i)
{ if ( getCompareNumber(n, i) == NAME_smaller )
    succeed;
  fail;
}


static status
largerNumber(Number n, Any i)
{ if ( getCompareNumber(n, i) == NAME_larger )
    succeed;
  fail;
}


static status
lessEqualNumber(Number n, Any i)
{ if ( getCompareNumber(n, i) != NAME_larger )
    succeed;
  fail;
}


static status
largerEqualNumber(Number n, Any i)
{ if ( getCompareNumber(n, i) != NAME_smaller )
    succeed;
  fail;
}


static status
plusNumber(Number n, Any i)
{ n->value += valArg(i);

  succeed;
}


static status
minusNumber(Number n, Any i)
{ n->value -= valArg(i);
  succeed;
}


static status
timesNumber(Number n, Any i)
{ n->value *= valArg(i);
  succeed;
}


static status
divideNumber(Number n, Any i)
{ n->value /= valArg(i);
  succeed;
}


static status
maximumNumber(Number n, Any i)
{ long m = valArg(i);

  if ( n->value < m )
    n->value = m;

  succeed;
}


static status
minimumNumber(Number n, Any i)
{ long m = valArg(i);

  if ( n->value > m )
    n->value = m;

  succeed;
}


static Number
getCatchAllNumber(Number n, Name selector, int argc, Any *argv)
{ Number result;

  if ( n->class == ClassNumber )
    result = getCopyNumber(n);
  else
    result = getCloneObject(n);

  if ( sendv(result, selector, argc, argv) )
    answer(result);
  else
  { freeObject(result);
    fail;
  }
}


static Any
getValueNumber(Number n)
{ if ( n->value >= PCE_MIN_INT && n->value < PCE_MAX_INT )
    answer(toInt(n->value));

  answer(n);
}


static status
valueNumber(Number n, Any i)
{ n->value = valArg(i);

  succeed;
}


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declaractions */

static char *T_catchAll[] =
        { "selector=name", "argument=unchecked ..." };
static char *T_convertOldSlot[] =
	{ "slot=name", "value=unchecked" };

static char T_arg[] = "int|number";
static char T_rarg[] = "int|number|real";

/* Instance Variables */

static vardecl var_number[] =
{ IV(NAME_value, "alien:long", IV_NONE,
     NAME_storage, "Value of number object")
};

/* Send Methods */

static senddecl send_number[] =
{ SM(NAME_initialise, 1, "value=int|number", initialiseNumber,
     DEFAULT, "Create from integer"),
  SM(NAME_convertOldSlot, 2, T_convertOldSlot, convertOldSlotNumber,
     NAME_file, "Convert old value slot to new representation"),
  SM(NAME_value, 1, T_arg, valueNumber,
     NAME_storage, "Assign a value to the number"),
  SM(NAME_divide, 1, T_arg, divideNumber,
     NAME_calculate, "Divide value by argument"),
  SM(NAME_maximum, 1, T_arg, maximumNumber,
     NAME_calculate, "Set value to largest of current and argument"),
  SM(NAME_minimum, 1, T_arg, minimumNumber,
     NAME_calculate, "Set value to largest of current and argument"),
  SM(NAME_minus, 1, T_arg, minusNumber,
     NAME_calculate, "Subtract argument from value"),
  SM(NAME_plus, 1, T_arg, plusNumber,
     NAME_calculate, "Add argument to value"),
  SM(NAME_times, 1, T_arg, timesNumber,
     NAME_calculate, "Multiply value by argument"),
  SM(NAME_equal, 1, T_rarg, equalNumber,
     NAME_compare, "Test if equal to argument"),
  SM(NAME_larger, 1, T_rarg, largerNumber,
     NAME_compare, "Test if larger than argument"),
  SM(NAME_largerEqual, 1, T_rarg, largerEqualNumber,
     NAME_compare, "Test if larger-or-equal than argument"),
  SM(NAME_lessEqual, 1, T_rarg, lessEqualNumber,
     NAME_compare, "Test if less-or-equal than argument"),
  SM(NAME_notEqual, 1, T_rarg, notEqualNumber,
     NAME_compare, "Test if not-equal to argument"),
  SM(NAME_smaller, 1, T_rarg, smallerNumber,
     NAME_compare, "Test if less than argument")
};

/* Get Methods */

static getdecl get_number[] =
{ GM(NAME_convert, 1, "number", "any", getConvertNumber,
     DEFAULT, "Converts int, real and char_array"),
  GM(NAME_compare, 1, "{smaller,equal,larger}", T_rarg, getCompareNumber,
     NAME_compare, "Compare with argument"),
  GM(NAME_catchAll, 2, "copy=number", T_catchAll, getCatchAllNumber,
     NAME_copy, "Create copy and run method on it"),
  GM(NAME_printName, 0, "string", NULL, getPrintNameNumber,
     NAME_textual, "Formatted version (%d) of value"),
  GM(NAME_value, 0, T_arg, NULL, getValueNumber,
     NAME_value, "Integer representing value")
};

/* Resources */

#define rc_number NULL
/*
static classvardecl rc_number[] =
{
};
*/

/* Class Declaration */

static Name number_termnames[] = { NAME_value };

ClassDecl(number_decls,
          var_number, send_number, get_number, rc_number,
          1, number_termnames,
          "$Rev$");

status
makeClassNumber(Class class)
{ declareClass(class, &number_decls);

  setLoadStoreFunctionClass(class, loadNumber, storeNumber);

  succeed;
}
