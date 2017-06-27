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
#include <h/lang.h>

NewClass(parser)
  Tokeniser	tokeniser;		/* The tokeniser */
  ChainTable	operators;		/* Operator table */
  HashTable	active;			/* Active symbols */
End;


Name openbracket;
Name closebracket;
Name comma;

static status	operatorParser(Parser p, Operator op);

static status
initialiseParserv(Parser p, Tokeniser t, int nops, Any *ops)
{ assign(p, tokeniser, t);
  assign(p, operators, newObject(ClassChainTable, EAV));

  for(; nops > 0; nops--, ops++)
    operatorParser(p, *ops);

  succeed;
}

		 /*******************************
		 *	   DECLARACTIONS	*
		 *******************************/

static status
operatorParser(Parser p, Operator op)
{ appendChainTable(p->operators, op->name, op);
  symbolTokeniser(p->tokeniser, op->name);

  succeed;
}


static status
activeParser(Parser p, Any token, Any msg)
{ if ( isFunction(msg) )
    msg = newObject(ClassQuoteFunction, msg, EAV);
  if ( isNil(p->active) )
    assign(p, active, newObject(ClassHashTable, EAV));

  return appendHashTable(p->active, token, msg);
}



		 /*******************************
		 *	OUTPUT GENERATION	*
		 *******************************/

static Any
getBuildTermParser(Parser p, Class class, int argc, Any *argv)
{ answer(answerObjectv(class, argc, argv));
}


		 /*******************************
		 *	      SPECIALS		*
		 *******************************/

#define MAX_ARGV 256

#define getTokenParser(p)	qadGetv((p)->tokeniser, NAME_token, 0, NULL)
#define ungetTokenParser(p, t)	qadSendv((p)->tokeniser, NAME_token, 1, &(t))
#define DCHAINCACHESIZE 10

static Chain DelimiterChainCache[DCHAINCACHESIZE];

static Chain
delimiterChain(Name d1, Name d2)
{ int i;
  Chain ch;

  for(i=0; i<DCHAINCACHESIZE; i++)
  { if ( (ch=DelimiterChainCache[i]) )
    { if ( ch->size == TWO &&
	   ch->head->value == d1 &&
	   ch->tail->value == d2 )
	return ch;
    } else
    { ch = DelimiterChainCache[i] = newObject(ClassChain, d1, d2, EAV);
      protectObject(ch);
      return ch;
    }
  }

  for(i=DCHAINCACHESIZE-1; i>0; i--)
    DelimiterChainCache[i] = DelimiterChainCache[i-1];

  ch = DelimiterChainCache[i] = newObject(ClassChain, d1, d2, EAV);
  protectObject(ch);
  return ch;
}


static Any
getListParser(Parser p, Name end, Name delimiter, Name functor)
{ Any argv[MAX_ARGV];
  int argc = 0;
  Any arg;
  Any token;
  Chain endterm;

  if ( isDefault(end) )
    end = closebracket;
  if ( isDefault(delimiter) )
    delimiter = comma;
  if ( notDefault(functor) )
    argv[argc++] = functor;

  if ( !(token = getTokenParser(p)) || token == EndOfFile )
    fail;
  if ( token == end )
    answer(getv(p, NAME_buildTerm, argc, argv));
  else
    ungetTokenParser(p, token);

  endterm = delimiterChain(end, delimiter);
					/* TBD: avoid this! */
  for(;;)
  { Any dl;

    TRY(arg = qadGetv(p, NAME_term, 1, (Any *)&endterm));
    argv[argc++] = arg;

    if ( !(dl = getTokenParser(p)) || dl == EndOfFile )
      fail;
    if ( dl == end )
      answer(getv(p, NAME_buildTerm, argc, argv));
    if ( isNil(delimiter) )
      ungetTokenParser(p, token);
  }
}


static Operator
prefix_op(Chain ch)
{ Cell cell;

  for_cell(cell, ch)
  { Operator o = cell->value;
    if ( o->left_priority == ZERO )
      return o;
  }

  fail;
}


static Operator
postfix_op(Chain ch)
{ Cell cell;

  for_cell(cell, ch)
  { Operator o = cell->value;
    if ( o->right_priority == ZERO )
      return o;
  }

  fail;
}


static Operator
infix_op(Chain ch)
{ Cell cell;

  for_cell(cell, ch)
  { Operator o = cell->value;
    if ( o->left_priority != ZERO && o->right_priority != ZERO )
      return o;
  }

  fail;
}


#define FAST_VALUES 10

typedef struct
{ Any	*values;
  Any	fast_values[FAST_VALUES];
  int	size;
  int	allocated;
} stack, *Stack;


static void
initStack(Stack s)
{ s->values = s->fast_values;
  s->size = 0;
  s->allocated = FAST_VALUES;
}


static void
pushStack(Stack s, Any v)
{ if ( s->size >= s->allocated )
  { int new = s->allocated * 2;

    if ( s->values == s->fast_values )
    { s->values = pceMalloc(sizeof(Any) * new);
      cpdata(s->values, s->fast_values, Any, s->size);
    } else
      s->values = pceRealloc(s->values, sizeof(Any) * new);
  }

  s->values[s->size++] = v;
}


static Any
popStack(Stack s)
{ return s->size > 0 ? s->values[--s->size] : FAIL;
}


static Any
peekStack(Stack s)
{ return s->size > 0 ? s->values[s->size-1] : FAIL;
}


static void
doneStack(Stack s)
{ if ( s->values != s->fast_values )
    pceFree(s->values);
}


static status
reduce(Parser p, Stack out, Stack side, int pri)
{ Operator o2;

  while( (o2=popStack(side)) && valInt(o2->priority) <= pri )
  { DEBUG(NAME_term, Cprintf("Reduce %s\n", pp(o2->name)));
    if ( o2->left_priority != ZERO && o2->right_priority != ZERO ) /* infix */
    { Any t, av[3];

      av[2] = popStack(out);
      av[1] = popStack(out);
      av[0] = o2->name;

      TRY(t = getv(p, NAME_buildTerm, 3, av));
      pushStack(out, t);
    } else				/* pre- or postfix */
    { Any t, av[2];

      av[1] = popStack(out);
      av[0] = o2->name;

      TRY(t = getv(p, NAME_buildTerm, 2, av));
      pushStack(out, t);
    }
  }

  succeed;
}


static int
modify(Parser p, int rmo, Stack out, Stack side, int pri)
{ Operator s, o2;
  Chain ops;

  if ( (s = peekStack(side)) && valInt(s->priority) < pri )
  { if ( s->left_priority == ZERO && rmo == 0 )	/* prefix */
    { rmo++;
      pushStack(out, s->name);
      popStack(side);
      DEBUG(NAME_term, Cprintf("Modify prefix %s --> name\n", pp(s->name)));
    } else if ( s->left_priority != ZERO && s->right_priority != ZERO &&
		rmo == 0 &&
		out->size > 0 &&
		(ops = getMemberHashTable((HashTable)p->operators, s->name)) &&
		(o2 = postfix_op(ops)) )
    { Any t, av[2];

      av[1] = popStack(out);
      av[0] = o2->name;
      t = getv(p, NAME_buildTerm, 2, av);

      rmo++;
      pushStack(out, t);
      popStack(side);
      DEBUG(NAME_term, Cprintf("Modify infix %s --> postfix\n", pp(s->name)));
    }
  }

  return rmo;
}



static Any
getTermParser(Parser p, Chain end)
{ Any token;
  Any active, rval;
  Function f;
  stack os, ss;
  Stack out = &os;
  Stack side = &ss;
  int rmo = 0;

  initStack(out);
  initStack(side);

  for(;;)
  { Chain ops;

    if ( !(token = getTokenParser(p)) )
      fail;
    if ( token == EndOfFile )
      goto exit;

					/* Active tokens */
    if ( notNil(p->active) && (active = getMemberHashTable(p->active, token)) )
    { if ( (f = checkType(active, TypeFunction, NIL)) &&
	   (rval = getForwardReceiverFunctionv(f, p, 1, &token)) )
	token = rval;
      else if ( instanceOfObject(active, ClassCode) )
      { forwardReceiverCodev(active, p, 1, &token);
	continue;
      }
    }

    if ( isName(token) && getPeekTokeniser(p->tokeniser) == toInt('(') )
    { Any t2;

      if ( (t2 = getTokenParser(p)) != openbracket )
	ungetTokenParser(p, t2);
      else
	TRY(token = get(p, NAME_list, closebracket, comma, token, EAV));
    }
					/* end detection */
    if ( notDefault(end) && memberChain(end, token) )
    { ungetTokenParser(p, token);
      goto exit;
    }

					/* operators */
    if ( isName(token) &&
	 (ops = getMemberHashTable((HashTable)p->operators, token)) )
    { Operator op;

      if ( (op = infix_op(ops)) )
      { DEBUG(NAME_term, Cprintf("Infix op %s\n", pp(token)));

	rmo = modify(p, rmo, out, side, valInt(op->left_priority));
	if ( rmo == 1 )
	{ TRY(reduce(p, out, side, valInt(op->left_priority)));
	  pushStack(side, op);
	  rmo--;
	  continue;
	}
      }
      if ( (op = postfix_op(ops)) )
      { DEBUG(NAME_term, Cprintf("Postfix op %s\n", pp(token)));

	rmo = modify(p, rmo, out, side, valInt(op->left_priority));
	if ( rmo == 1 )
	{ TRY(reduce(p, out, side, valInt(op->left_priority)));
	  pushStack(side, op);
	  continue;
	}
      }

      if ( rmo == 0 && (op = prefix_op(ops)) )
      { DEBUG(NAME_term, Cprintf("Prefix op %s\n", pp(token)));

	TRY(reduce(p, out, side, valInt(op->left_priority)));
	pushStack(side, op);
	continue;
      }
    }

    if ( rmo == 0 )
    { rmo++;
      DEBUG(NAME_term, Cprintf("Pushing %s\n", pp(token)));
      pushStack(out, token);
    } else
    { send(p, NAME_syntaxError, CtoName("Operator expected"), EAV);
      fail;
    }
  }

exit:
  rmo = modify(p, rmo, out, side, 100000);
  TRY(reduce(p, out, side, 100000));

  DEBUG(NAME_term, Cprintf("out->size = %d; side->size = %d\n",
			   out->size == 1, side->size));

  if ( out->size == 1 && side->size == 0 )
    rval = popStack(out);
  else if ( out->size == 0 && side->size == 1 )
  { Operator op = popStack(side);

    rval = op->name;
  } else
  { send(p, NAME_syntaxError, CtoName("Unbalanced operators"), EAV);
    rval = FAIL;
  }

  doneStack(out);
  doneStack(side);

  return rval;
}


static Any
getParseParser(Parser p, Any input)
{ Any rval;
  Tokeniser t = p->tokeniser;
  Tokeniser t2 = getOpenTokeniser(t, input);

  addCodeReference(t);
  addCodeReference(input);
  if ( t2 != t )
    assign(p, tokeniser, t2);
  rval = getTermParser(p, DEFAULT);
  if ( t2 != t )
    assign(p, tokeniser, t);
  delCodeReference(input);
  delCodeReference(t);

  answer(rval);
}



		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */

static char *T_buildTerm[] =
        { "class=class", "argument=unchecked ..." };
static char *T_list[] =
        { "end=[name]", "delimiter=[name]*", "functor=[name]" };
static char *T_active[] =
        { "token=any", "message=code|function" };
static char *T_initialise[] =
        { "tokeniser=tokeniser", "operators=operator..." };

/* Instance Variables */

static vardecl var_parser[] =
{ IV(NAME_tokeniser, "tokeniser", IV_BOTH,
     NAME_syntax, "Tokeniser used for this parser"),
  IV(NAME_operators, "chain_table", IV_BOTH,
     NAME_syntax, "Operator table for this parser"),
  IV(NAME_active, "hash_table*", IV_BOTH,
     NAME_syntax, "Active tokens")
};

/* Send Methods */

static senddecl send_parser[] =
{ SM(NAME_initialise, 2, T_initialise, initialiseParserv,
     DEFAULT, "Create from tokeniser and operators"),
  SM(NAME_active, 2, T_active, activeParser,
     NAME_syntax, "Declare token to call message"),
  SM(NAME_operator, 1, "operator=operator", operatorParser,
     NAME_syntax, "Declare operator for parser")
};

/* Get Methods */

static getdecl get_parser[] =
{ GM(NAME_buildTerm, 2, "object=unchecked", T_buildTerm, getBuildTermParser,
     NAME_build, "Create object from data read"),
  GM(NAME_list, 3, "object=unchecked", T_list, getListParser,
     NAME_parse, "Read terms upto end"),
  GM(NAME_parse, 1, "unchecked", "input=char_array|file|text_buffer", getParseParser,
     NAME_parse, "Open, read <-term and close"),
  GM(NAME_term, 1, "term=unchecked", "end=[chain]", getTermParser,
     NAME_parse, "Read next term")
};

/* Resources */

#define rc_parser NULL
/*
static classvardecl rc_parser[] =
{
};
*/

/* Class Declaration */

static Name parser_termnames[] = { NAME_tokeniser };

ClassDecl(parser_decls,
          var_parser, send_parser, get_parser, rc_parser,
          1, parser_termnames,
          "$Rev$");

status
makeClassParser(Class class)
{ declareClass(class, &parser_decls);
  delegateClass(class, NAME_tokeniser);

  openbracket = CtoName("(");
  closebracket = CtoName(")");
  comma = CtoName(",");

  succeed;
}
