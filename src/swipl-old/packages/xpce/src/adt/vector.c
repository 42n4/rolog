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

#define assignVector(v, n, o)	{ assignField((Instance)(v), \
					      &(v)->elements[n], \
					      (o)); \
				}
#define indexVector(v, e)	( valInt(e) - valInt(v->offset) - 1 )
#define validIndex(v, i)	{ if ( i < 0 || i >= valInt(v->size) ) fail; }

status
initialiseVectorv(Vector v, int argc, Any *argv)
{ int n;

  v->offset = ZERO;
  v->size = toInt(argc);
  v->allocated = v->size;
  if ( argc > 0 )
  { v->elements = alloc(argc * sizeof(Any));

    for(n=0; n < argc; n++)
    { v->elements[n] = NIL;
      assignVector(v, n, argv[n]);
    }
  } else
    v->elements = NULL;

  succeed;
}


static status
cloneVector(Vector v, Vector clone)
{ int n, size = valInt(v->size);

  clonePceSlots(v, clone);
  clone->allocated = v->size;
  clone->elements  = alloc(size * sizeof(Any));

  for( n=0; n<size; n++ )
  { clone->elements[n] = NIL;
    assignField((Instance) clone,
		&clone->elements[n],
		getClone2Object(v->elements[n]));
  }

  succeed;
}


Vector
createVectorv(int argc, Any *argv)
{ Vector v = alloc(sizeof(struct vector));

  initHeaderObj(v, ClassVector);
  v->offset      = (Int) NIL;
  v->size        = (Int) NIL;
  v->elements    = NULL;

  initialiseVectorv(v, argc, argv);
  createdObject(v, NAME_new);

  return v;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Load/store a vector on a file. Format:

<vector>		::= {<element>}

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static status
storeVector(Vector v, FileObj file)
{ int n;

  TRY(storeSlotsObject(v, file));
  for(n = 0; n < valInt(v->size); n++)
    TRY(storeObject(v->elements[n], file));

  succeed;
}


static status
loadVector(Vector v, IOSTREAM *fd, ClassDef def)
{ int n;
  Any obj;
  int size;

  loadSlotsObject(v, fd, def);
  size = valInt(v->size);
  v->allocated = v->size;
  v->elements = alloc(size * sizeof(Any));
  for(n = 0; n < size; n++)
  { TRY( obj = loadObject(fd) );
    v->elements[n] = NIL;
    assignVector(v, n, obj);
  }

  succeed;
}


status
unlinkVector(Vector v)
{ if ( v->elements != NULL )
    return clearVector(v);

  succeed;
}


static status
equalVector(Vector v1, Vector v2)
{ if ( classOfObject(v1) == classOfObject(v2) &&
       v1->size == v2->size &&
       v1->offset == v2->offset )
  { Any *e1 = v1->elements;
    Any *e2 = v2->elements;
    int n = valInt(v1->size);

    for(; --n >= 0; e1++, e2++)
    { if ( *e1 != *e2 )
	fail;
    }

    succeed;
  }

  fail;
}


Int
getLowIndexVector(Vector v)
{ answer(add(v->offset, ONE));
}


Int
getHighIndexVector(Vector v)
{ answer(add(v->size, v->offset));
}


static status
highIndexVector(Vector v, Int high)
{ int h  = valInt(high);
  int oh = valInt(v->offset) + valInt(v->size);

  if ( oh > h )				/* too long */
  { int size = h - valInt(v->offset);
    if ( size > 0 )
    { Any *elms = alloc(size * sizeof(Any));

      fillVector(v, NIL, inc(high), DEFAULT); /* dereference */
      cpdata(elms, v->elements, Any, size);
      unalloc(valInt(v->allocated)*sizeof(Any), v->elements);
      v->elements = elms;
      assign(v,	size,	   toInt(size));
      assign(v,	allocated, v->size);

      succeed;
    } else
    { return clearVector(v);
    }
  } else if ( oh < h )			/* too, short */
  { return fillVector(v, NIL, toInt(oh+1), inc(high));
  }

  succeed;
}


static status
lowIndexVector(Vector v, Int low)
{ int l  = valInt(low);
  int ol = valInt(v->offset) + 1;

  if ( l > ol )				/* too long */
  { int size = valInt(v->size) + valInt(v->offset) - l;
    if ( size > 0 )
    { Any *elms = alloc(size * sizeof(Any));

      fillVector(v, NIL, toInt(l), toInt(ol-1)); /* dereference */
      cpdata(elms, &v->elements[l-ol], Any, size);
      unalloc(valInt(v->allocated)*sizeof(Any), v->elements);
      v->elements = elms;
      assign(v, size, toInt(size));
      assign(v, allocated, v->size);

      succeed;
    } else
    { return clearVector(v);
    }
  } else if ( l < ol )			/* too, short */
  { return fillVector(v, NIL, toInt(l), toInt(ol-1));
  }

  succeed;
}


status
rangeVector(Vector v, Int low, Int high)
{ if ( notDefault(low) )
    lowIndexVector(v, low);
  if ( notDefault(high) )
    highIndexVector(v, high);

  succeed;
}


status
clearVector(Vector v)
{ if ( v->elements )
  { fillVector(v, NIL, DEFAULT, DEFAULT);

    unalloc(valInt(v->allocated)*sizeof(Any), v->elements);
    v->elements = NULL;
  }
  assign(v, allocated, ZERO);
  assign(v, size,      ZERO);
  assign(v, offset,    ZERO);

  succeed;
}


Any
getTailVector(Vector v)
{ if ( v->size != ZERO )
    answer(v->elements[valInt(v->size)-1]);

  fail;
}


static Any
getHeadVector(Vector v)
{ if ( v->size != ZERO )
    answer(v->elements[0]);

  fail;
}


Vector
getCopyVector(Vector v)
{ Vector v2 = answerObjectv(classOfObject(v), valInt(v->size), v->elements);

  assign(v2, offset, v->offset);

  answer(v2);
}


status
fillVector(Vector v, Any obj, Int from, Int to)
{ int f, t;

  f = (isDefault(from) ? valInt(getLowIndexVector(v)) : valInt(from));
  t = (isDefault(to)   ? valInt(getHighIndexVector(v)) : valInt(to));

  if ( t < f )
    fail;

  if ( v->size == ZERO )
  { int size = t-f+1;
    int n;

    assign(v, offset,	 toInt(f - 1));
    assign(v, size,	 toInt(size));
    assign(v, allocated, v->size);
    if ( v->elements )
      unalloc(0, v->elements);
    v->elements = alloc(sizeof(Any) * size);
    for(n=0; n<size; n++)
    { v->elements[n] = NIL;
      if ( notNil(obj) )
	assignVector(v, n, obj);
    }
  } else
  { elementVector(v, toInt(f), obj);
    elementVector(v, toInt(t), obj);
    while( ++f < t )
      elementVector(v, toInt(f), obj);
  }

  succeed;
}


status
shiftVector(Vector v, Int places)
{ int n = valInt(v->size);
  int s = valInt(places);
  int i;

  if ( s > 0 )
  { for(i=n-s; i < n; i++)
      assignVector(v, i, NIL);
    for(i = n - 1; i >= s; i--)
      v->elements[i] = v->elements[i-s];
    for( ; i >= 0; i-- )
      v->elements[i] = NIL;
  } else
  { for(i = 0; i < -s; i++)
      assignVector(v, i, NIL);
    for(i = 0; i < n+s; i++)
      v->elements[i] = v->elements[i-s];
    for( ; i < n; i++ )
      v->elements[i] = NIL;
  }

  succeed;
}


Any
getElementVector(Vector v, Int e)
{ int n = indexVector(v, e);

  validIndex(v, n);

  answer(v->elements[n]);
}


status
elementVector(Vector v, Int e, Any obj)
{ int n = indexVector(v, e);

  if ( n < 0 )
  { int nsize = valInt(v->size)-n;
    Any *newElements = alloc(nsize*sizeof(Any));
    int m;

    if ( v->elements )
    { cpdata(&newElements[-n], v->elements, Any, valInt(v->size));
      unalloc(valInt(v->allocated)*sizeof(Any), v->elements);
    }
    v->elements = newElements;
    for( m = 0; m < -n; m++ )
      v->elements[m] = NIL;
    assignVector(v, 0, obj);

    assign(v, size,	 toInt(nsize));
    assign(v, allocated, toInt(nsize));
    assign(v, offset,	 toInt(valInt(e)-1));

    succeed;
  }

  if ( n >= valInt(v->size) )
  { int m;

    if ( n >= valInt(v->allocated) )
    { int nalloc = max(valInt(v->allocated)*2, n+1);
      Any *newElements = alloc(nalloc * sizeof(Any));

      if ( v->elements )
      { cpdata(newElements, v->elements, Any, valInt(v->size));
	unalloc(valInt(v->allocated)*sizeof(Any), v->elements);
      }
      v->elements = newElements;
      assign(v, allocated, toInt(nalloc));
    }
    for( m = valInt(v->size); m <= n ; m++ )
      v->elements[m] = NIL;
    assignVector(v, n, obj);

    assign(v, size, toInt(n+1));

    succeed;
  }

  assignVector(v, n, obj);

  succeed;
}


status
appendVector(Vector v, int argc, Any obj[])
{ if ( argc )
  { int start = valInt(v->size) + valInt(v->offset) + 1;

    fillVector(v, NIL, toInt(start), toInt(start + argc - 1));
    for( ; argc-- > 0; start++, obj++ )
      elementVector(v, toInt(start), *obj);
  }

  succeed;
}


static status
insertVector(Vector v, Int where, Any obj)
{ int size   = valInt(v->size);
  int offset = valInt(v->offset);
  int i;
  Any *s, *p;

  if ( valInt(where) <= offset+1 )
  { assign(v, offset, toInt(offset+1));

    return elementVector(v, where, obj);
  }
  if ( valInt(where) > size+offset )
    return elementVector(v, where, obj);

  elementVector(v, toInt(size+offset+1), NIL);
  i = indexVector(v, where);
  s = &v->elements[i];
  p = &v->elements[valInt(v->size)-1];	/* point to last element */
  for( ; p>s; p-- )
  { p[0] = p[-1];
  }
  v->elements[i] = NIL;
  assignVector(v, i, obj);

  succeed;
}


static status
sortVector(Vector v, Code code, Int from, Int to)
{ int f, t, n;

  f = valInt(v->offset) + 1;
  t = f + valInt(v->size) - 1;

  if ( notDefault(from) && valInt(from) > f )
    f = valInt(from);
  if ( notDefault(to) && valInt(to) > t )
    t = valInt(to);
  if ( t <= f )
    succeed;

  n = t-f+1;
  f -= valInt(v->offset) + 1;

  { Code old = qsortCompareCode;		/* make reentrant */
    qsortCompareCode = code;

    qsort(&v->elements[f], n, sizeof(Any), qsortCompareObjects);

    qsortCompareCode = old;
  }

  succeed;
}


static status
swapVector(Vector v, Int e1, Int e2)
{ int n1 = indexVector(v, e1);
  int n2 = indexVector(v, e2);
  Any tmp;

  validIndex(v, n1);
  validIndex(v, n2);

  tmp = v->elements[n1];		/* do not use assign() here: tmp */
  v->elements[n1] = v->elements[n2];	/* might drop out in that case (JW) */
  v->elements[n2] = tmp;

  succeed;
}

#define BOUNDS(v, l, m) \
	{ if ( (v) < (l) ) (v) = (l); else if ( (v) > (m) ) (v) = (m); }

static int
get_range(Vector v, Int from, Int to, int *f, int *t)
{ int low  = valInt(getLowIndexVector(v));
  int high = valInt(getHighIndexVector(v));

  if ( low > high )
    fail;				/* empty vector */

  if ( isDefault(to) )
  { if ( isDefault(from) )
    { *f = low;
      *t = high;
    } else				/* from, @default */
    { int i = valInt(from);

      if ( i > high )
	fail;
      if ( i < low )
	i = low;
      *f = i;
      *t = high;
    }
  } else
  { if ( isDefault(from) )		/* @default, to */
    { int i = valInt(to);

      if ( low > i )
	fail;
      if ( i > high )
	i = high;
      *t = i;
      *f = low;
    } else				/* from, to */
    { int i = valInt(from);

      BOUNDS(i, low, high);
      *f = i;
      i = valInt(to);
      BOUNDS(i, low, high);
      *t = i;
    }
  }

  succeed;
}


static status
forVector(Vector v, Code code, Int from, Int to, int some)
{ int f, t;

  if ( get_range(v, from, to, &f, &t) )
  { int step = (t >= f ? 1 : -1);
    int offset = valInt(v->offset);

    for(; f != t+step ; f += step)
    { Any av[2];

      av[0] = v->elements[f-offset-1];
      av[1] = toInt(f);
      if ( !(forwardCodev(code, 2, av) || some) )
	fail;
    }
  }

  succeed;
}


static status
forAllVector(Vector v, Code code, Int from, Int to)
{ return forVector(v, code, from, to, FALSE);
}


static status
forSomeVector(Vector v, Code code, Int from, Int to)
{ return forVector(v, code, from, to, TRUE);
}


static Any
getFindVector(Vector v, Code code, Int from, Int to)
{ int f, t;

  if ( get_range(v, from, to, &f, &t) )
  { int step = (t >= f ? 1 : -1);
    int offset = valInt(v->offset);

    for(; f != t+step ; f += step)
    { Any av[2];

      av[0] = v->elements[f-offset-1];
      av[1] = toInt(f);
      if ( forwardCodev(code, 2, av) )
	answer(av[0]);
    }
  }

  fail;
}


static Chain
getFindAllVector(Vector v, Code code, Int from, Int to)
{ Chain result = answerObject(ClassChain, EAV);
  int f, t;

  if ( get_range(v, from, to, &f, &t) )
  { int step = (t >= f ? 1 : -1);
    int offset = valInt(v->offset);

    for(; f != t+step ; f += step)
    { Any av[2];

      av[0] = v->elements[f-offset-1];
      av[1] = toInt(f);
      if ( forwardCodev(code, 2, av) )
	appendChain(result, av[0]);
    }
  }

  answer(result);
}


Int
getIndexVector(Vector v, Any obj)
{ int n;
  int size = valInt(v->size);

  for( n=0; n<size; n++)
    if ( v->elements[n] == obj )
      answer(toInt(n + valInt(v->offset) + 1));

  fail;
}


static Int
getRindexVector(Vector v, Any obj)
{ int n;
  int size = valInt(v->size);

  for( n=size-1; n>=0; n--)
    if ( v->elements[n] == obj )
      answer(toInt(n + valInt(v->offset) + 1));

  fail;
}

		/********************************
		*      TERM REPRESENTATION      *
		*********************************/

Any
getArgVector(Vector v, Int arg)
{ int n = valInt(arg) - 1;

  validIndex(v, n);

  answer(v->elements[n]);
}


Int
getArityVector(Vector v)
{ answer(v->size);
}


static status
changedVector(Vector v, Any *field)
{ if ( onFlag(v, F_INSPECT) )
  { Class class = classOfObject(v);

    if ( notNil(class->changed_messages) )
    { int index = field - v->elements;

      if ( index >= 0 && index < valInt(v->size) )
	return changedObject(v, toName((Any) toInt(index)), EAV);

      return changedFieldObject(v, field);
    }
  }

  succeed;
}

		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declaractions */

static char *T_element[] =
        { "index=int", "value=any" };
static char *T_swap[] =
        { "index_1=int", "index_2=int" };
static char *T_fill[] =
        { "value=any", "from=[int]", "to=[int]" };
static char *T_range[] =
        { "from=[int]", "to=[int]" };
static char *T_sort[] =
        { "compare=code", "from=[int]", "to=[int]" };
static char *T_enum[] =
	{ "code=code", "from=[int]", "to=[int]" };

/* Instance Variables */

static vardecl var_vector[] =
{ IV(NAME_offset, "int", IV_GET,
     NAME_range, "Offset relative to 1-based"),
  IV(NAME_size, "0..", IV_GET,
     NAME_range, "Number of elements"),
  IV(NAME_allocated, "0..", IV_GET,
     NAME_internal, "Allocated size of array"),
  IV(NAME_elements, "alien:Any *", IV_NONE,
     NAME_storage, "The elements themselves")
};

/* Send Methods */

static senddecl send_vector[] =
{ SM(NAME_initialise, 1, "element=any ...", initialiseVectorv,
     DEFAULT, "Create vector with elements at 1, ..."),
  SM(NAME_unlink, 0, NULL, unlinkVector,
     DEFAULT, "Deallocates -elements"),
  SM(NAME_equal, 1, "vector", equalVector,
     NAME_compare, "Test if both vectors contain the same objects"),
  SM(NAME_element, 2, T_element, elementVector,
     NAME_element, "Set specified element"),
  SM(NAME_insert, 2, T_element, insertVector,
     NAME_element, "Insert at location, shifting higher elements"),
  SM(NAME_fill, 3, T_fill, fillVector,
     NAME_element, "Fill index range with one value"),
  SM(NAME_forAll, 3, T_enum, forAllVector,
     NAME_iterate, "Run code on all elements; demand acceptance"),
  SM(NAME_forSome, 3, T_enum, forSomeVector,
     NAME_iterate, "Run code on all elements"),
  SM(NAME_append, 1, "value=any ...", appendVector,
     NAME_list, "Append element at <-high_index+1"),
  SM(NAME_sort, 3, T_sort, sortVector,
     NAME_order, "Sort according to code exit status"),
  SM(NAME_swap, 2, T_swap, swapVector,
     NAME_order, "Swap two elements"),
  SM(NAME_shift, 1, "places=int", shiftVector,
     NAME_range, "Shift contents by n places"),
  SM(NAME_clear, 0, NULL, clearVector,
     NAME_range, "Delete all elements"),
  SM(NAME_range, 2, T_range, rangeVector,
     NAME_range, "Determine range")
};

/* Get Methods */

static getdecl get_vector[] =
{ GM(NAME_Arg, 1, "any", "int", getArgVector,
     DEFAULT, "Get argument for term"),
  GM(NAME_Arity, 0, "int", NULL, getArityVector,
     DEFAULT, "Get arity for term"),
  GM(NAME_copy, 0, "vector", NULL, getCopyVector,
     NAME_copy, "Create a copy of a vector"),
  GM(NAME_element, 1, "any", "index=int", getElementVector,
     NAME_element, "Get element at index"),
  GM(NAME_head, 0, "any", NULL, getHeadVector,
     NAME_element, "First element (as class chain)"),
  GM(NAME_tail, 0, "any", NULL, getTailVector,
     NAME_element, "Last element (as class chain)"),
  GM(NAME_highIndex, 0, "int", NULL, getHighIndexVector,
     NAME_range, "Get highest valid index"),
  GM(NAME_lowIndex, 0, "int", NULL, getLowIndexVector,
     NAME_range, "Get lowest valid index"),
  GM(NAME_find, 3, "unchecked", T_enum, getFindVector,
     NAME_search, "First element accepted by code"),
  GM(NAME_findAll, 3, "unchecked", T_enum, getFindAllVector,
     NAME_search, "Chain of elements accepted by code"),
  GM(NAME_index, 1, "int", "any", getIndexVector,
     NAME_search, "Get first index holding argument"),
  GM(NAME_rindex, 1, "int", "any", getRindexVector,
     NAME_search, "Get last index holding argument")
};

/* Resources */

#define rc_vector NULL
/*
static classvardecl rc_vector[] =
{
};
*/

/* Class Declaration */

ClassDecl(vector_decls,
          var_vector, send_vector, get_vector, rc_vector,
          ARGC_UNKNOWN, NULL,
          "$Rev$");



status
makeClassVector(Class class)
{ declareClass(class, &vector_decls);

  setLoadStoreFunctionClass(class, loadVector, storeVector);
  setCloneFunctionClass(class, cloneVector);
  setChangedFunctionClass(class, changedVector);

  succeed;
}

