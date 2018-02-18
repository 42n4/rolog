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

#define NO_BUILT_IN_DECL
#include <h/kernel.h>
#include <h/trace.h>

#define BENCHNAMES 0			/* include benchmark code */

static void	insertName(Name);
static void	deleteName(Name);
static Name	getLookupName(Class, CharArray);
static status	registerName(Name n);


		/********************************
		*         BUILTIN NAMES		*
		********************************/

#define BUILTIN_NAME(s) { 0L, 0L, NULL, 0, s },

NewClass(bname)
  unsigned long str_header;
  char *text;
End;

struct bname builtin_names[] =
{
#include <h/names.ic>
  { 0L, 0L, NULL, 0, NULL }
};


void
trapGdb(void)
{
}


static int
nextBucketSize(n)
int n;
{ n *= 2;

  if ( !(n % 2) )
    n++;

  for(;; n += 2)
  { int m;
    int to = isqrt(n);

    for(m=3 ; m <= to; m += 2)
      if ( !(n % m) )
	break;

    if ( m > to )
      return n;
  }
}



		/********************************
		*          HASH-TABLE		*
		********************************/

static int	buckets = 2048;		/* initial size */
static int      names = 0;		/* number of names */
static Name    *name_table;
static int	builtins;		/* number of builtin names */

static inline int
stringHashValue(String s)
{ unsigned int value = 0;
  unsigned int shift = 5;
  int size = s->s_size;
  charA *t = s->s_textA;

  if ( isstrW(s) )
    size *= sizeof(charW);

  while(--size >= 0)
  { unsigned int c = *t++;

    c -= 'a';
    value ^= c << shift;
    shift += 3;
    if ( shift > 24 )
      shift = 1;
  }

  return value % buckets;
}


static Int
getHashValueName(Name name)
{ answer(toInt(stringHashValue(&name->data)));
}


static Name
getBucketValueName(Name name, Int bucket)
{ if ( valInt(bucket) < buckets && name_table[valInt(bucket)] )
    answer(name_table[valInt(bucket)]);

  fail;
}



static void
rehashNames(void)
{ int old_buckets = buckets;
  Name *old_table = name_table;
  Name *nm;
  int n;

  buckets = nextBucketSize(buckets);
  DEBUG((Name)NAME_name, Cprintf("Rehashing names ... "));
  name_table = pceMalloc(buckets * sizeof(Name));
  for(n=buckets, nm = name_table; n-- > 0; nm++)
    *nm = NULL;

  names = 0;
  for(n=old_buckets, nm=old_table; n-- > 0; nm++)
    if ( *nm )
      insertName(*nm);

  DEBUG((Name)NAME_name, Cprintf("done\n"));

  pceFree(old_table);
}


static void
insertName(Name name)
{ Name *nm;
  Name *end;

  if ( 5*names > 3*buckets )
    rehashNames();

  nm = &name_table[stringHashValue(&name->data)];
  end = &name_table[buckets];

  while( *nm != NULL )
  { if ( ++nm == end )
      nm = name_table;
  }

  *nm = name;
  names++;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Deletion algorithm from Donald E. Knuth, ``The Art of Computer
Programming'', Volume 3 page 527.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void
deleteName(Name name)
{ int hashkey = stringHashValue(&name->data);
  Name *j, *i = &name_table[hashkey];
  Name *end = &name_table[buckets];

  while(*i && *i != name)
  { if ( ++i == end )
      i = name_table;
  }
  assert(*i);

  *i = NULL;				/* R1 */
  j = i;

  for(;;)
  { Name *r;

    if ( ++i == end )			/* R2 */
      i = name_table;

    if ( *i == NULL )			/* R3 */
    { names--;
      return;
    }

    r = &name_table[stringHashValue(&(*i)->data)];
    if ( (i >= r && r > j) || (r > j && j > i) || (j > i && i >= r) )
      continue;

    *j = *i;
    *i = NULL;				/* R1 and to R2 */
    j = i;
  }
}


void
initNamesPass1(void)
{ Name name;

  allocRange(builtin_names, sizeof(builtin_names));

  for( name=(Name)builtin_names; name->data.s_text != NULL; name++)
  { str_inithdr(&name->data, FALSE);
    name->data.s_size = (int)strlen((char *)name->data.s_text);
  }
}


void
initNamesPass2(void)
{ int n;
  Name name;

  buckets = nextBucketSize(buckets);	/* initialise to first valid value */
  name_table = pceMalloc(buckets * sizeof(Name));
  for(n=0; n < buckets; n++)
    name_table[n] = NULL;

  for(n = 0, name=(Name)builtin_names; name->data.s_text != NULL; name++, n++)
  { initHeaderObj(name, ClassName);
    registerName(name);
    createdObject(name, (Name)NAME_new);
  }

  builtins = n;

  DEBUG_BOOT(checkNames(TRUE));
}



		/********************************
		*       CHECK CONSISTENCY	*
		********************************/

static int shifted;

/*#define WRITE_BAD_NAMES 1 ... */

void
checkNames(int prt)
{ int n;
  int cnt = 0;

  shifted = 0;

  for(n=0; n < buckets; n++)
  { Name name = name_table[n];
#ifdef WRITE_BAD_NAMES
    Name n2;
#endif

    if ( name != NULL )
    { cnt++;
      assert(isProperObject(name));
      assert(isName(name));		/* checks F_ISNAME */
      assert(classOfObject(name) == ClassName);
      assert(isProtectedObj(name));
      assert(name->data.s_text != NULL);
#ifdef WRITE_BAD_NAMES
      if ( (n2=getLookupName(NULL, (CharArray) name)) != name )
	Cprintf("@%ld: getLookupName('%s') --> %s; "
		"len = %d, strlen = %d (data at @%ld)\n",
		valInt(PointerToInt(name)),
		strName(name), n2 ? strName(n2) : "(NULL)",
	        name->data.size, strlen(strName(name)),
		valInt(PointerToInt(strName(name))));
#else
      assert(getLookupName(NULL, (CharArray) name) == name);
#endif
    }
  }

  if ( prt )
    Cprintf("%d names in %d buckets. %d shifts\n",
	    names, buckets, shifted);

  assert(cnt == names);
}


		/********************************
		*         CREATE/CONVERT	*
		********************************/


status
initialiseName(Name n, CharArray value)
{ initialiseCharArray((CharArray) n, value);

  if ( inBoot )
    return registerName(n);
  else
    return qadSendv(n, (Name)NAME_register, 0, NULL);
}


static status
unlinkName(Name n)
{ assert(0);				/* names cannot be unlinked! */
  fail;
}


static status
registerName(Name n)
{ insertName(n);
  setFlag(n, F_PROTECTED|F_ISNAME);

  succeed;
}


static Name
getLookupName(Class class, CharArray value)
{ int hashkey = stringHashValue(&value->data);
  Name *name = &name_table[hashkey];

  while(*name)
  { if ( str_eq(&(*name)->data, &value->data) )
      answer(*name);

    shifted++;				/* debugging */
    if ( ++hashkey == buckets )
    { hashkey = 0;
      name = name_table;
    } else
      name++;
  }

  fail;
}


static Name
getConvertName(Any ctx, Any val)
{ if ( instanceOfObject(val, ClassCharArray) )
  { CharArray ca = val;

    return StringToName(&ca->data);
  } else
  { char *s;

    TRY(s = toCharp(val));
    answer(CtoName(s));
  }
}


static Name
getModifyName(Name n, Name n2)
{ Name name;

  if ( (name = getLookupName(ClassName, (CharArray) n2)) )
    answer(name);

  answer(newObject(ClassName, n2, EAV));
}


static Name
getCopyName(Name n)
{ answer(n);
}

		/********************************
		*       MODIFYING NAMES		*
		********************************/

static int
isBuiltInName(Name n)
{ return n >= (Name)&builtin_names[0] &&
         n <  (Name)&builtin_names[builtins];
}


static status
ValueName(Name n, CharArray val)
{ Name existing;

  DEBUG((Name)NAME_name, Cprintf("Converting %s --> ", strName(n)));

  if ( (existing = getLookupName(classOfObject(n), val)) )
  { if ( existing != n )
      return errorPce(n, (Name)NAME_nameAlreadyExists);
    succeed;
  }

  deleteName(n);
  if ( !isBuiltInName(n) )
    str_unalloc(&n->data);
  str_cphdr(&n->data, &val->data);
  str_alloc(&n->data);
  str_ncpy(&n->data, 0, &val->data, 0, val->data.s_size);
  insertName(n);

  DEBUG((Name)NAME_name, Cprintf("%s\n", strName(n)));

  succeed;
}


static status
syntaxName(Name n, Name casemap, Int ws)
{ String s = &n->data;
  int size = s->s_size;
  int i;
  StringObj str;

  for(i=0; i<size; i++)
  { wint_t c = str_fetch(s, i);

    if ( iswupper(c) || c == '%' || c == '.' )
      succeed;
  }

  str = newObject(ClassString, name_procent_s, n, EAV);
  upcaseString(str);
  if ( notDefault(ws) )
  { s = &str->data;
    size = s->s_size;

    for(i=0; i<size; i++)
      if ( str_fetch(s, i) == (wint_t)syntax.word_separator )
	str_store(s, i, valInt(ws));
  }

  TRY(ValueName(n, (CharArray) str));

  return doneObject(str);
}


status
forNamePce(Pce pce, Code code)
{ Name *a = (Name *)alloca(sizeof(Name) * names);
  Name *i=name_table, *o = a;
  int nms = names;			/* might change: copy */
  int n;

  for(; i < &name_table[buckets]; i++)
  { if ( *i )
      *o++ = *i;
  }

  for(n = 0, i = a; n < nms; n++, i++)
  { if ( !forwardCodev(code, 1, (Any*)i) )
      fail;
  }

  succeed;
}

		/********************************
		*       RAW MODIFICATIONS	*
		********************************/

Name
getCapitaliseName(Name n)
{ return (Name) getCapitaliseCharArray((CharArray) n);
}


Name
GetLabelNameName(Name n)
{ return qadGetv(n, (Name)NAME_labelName, 0, NULL);
}


Name
getDeleteSuffixName(Name n, Name suffix)
{ return (Name) getDeleteSuffixCharArray((CharArray) n, (CharArray)suffix);
}


		/********************************
		*          C-CONVERSIONS	*
		********************************/

#ifdef BENCHNAMES
static int str_eq_failed;
#endif


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Note that the encoding of a name   is canonical. I.e. iff all characters
are < 256, it is an  ISO  Latin-1   string  and  otherwise  it is a wide
character string.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

Name
StringToName(String s)
{ int hashkey;
  Name *namep;
  string s2;
  void *do_free = NULL;
  Name name;

  if ( s->s_iswide )
  { const charW *txt = s->s_textW;
    const charW *end = &txt[s->s_size];
    charA *p;

    for( ; txt < end; txt++ )
    { if ( *txt > 0xff )
	goto canonical;
    }

    str_inithdr(&s2, FALSE);
    s2.s_size = s->s_size;
    if ( !(s2.s_textA = alloca(s->s_size)) )
    { s2.s_textA = pceMalloc(s->s_size);
      do_free = s2.s_textA;
    }
    for(txt = s->s_textW, p = s2.s_textA; txt < end; )
      *p++ = *txt++;

    s = &s2;
  }

canonical:
  hashkey = stringHashValue(s);
  for( namep = &name_table[hashkey]; *namep; )
  { if ( str_eq(&(*namep)->data, s) )
      answer(*namep);
#ifdef BENCHNAMES
    else
      str_eq_failed++;
#endif
  if ( ++hashkey == buckets )
    { hashkey = 0;
      namep = name_table;
    } else
      namep++;
  }

  if ( inBoot )
  { name = alloc(sizeof(struct name));
    initHeaderObj(name, ClassName);

    str_cphdr(&name->data, s);
    str_alloc(&name->data);
    str_ncpy(&name->data, 0, s, 0, s->s_size);
    registerName(name);
    createdObject(name, (Name)NAME_new);
  } else
  { CharArray scratch = StringToScratchCharArray(s);

    ServiceMode(PCE_EXEC_SERVICE, name = newObject(ClassName, scratch, EAV));

    doneScratchCharArray(scratch);
  }

  if ( do_free )
    pceFree(do_free);

  answer(name);
}


Name
CtoKeyword(const char *s)
{ if ( syntax.uppercase )
  { CharBuf(buf, strlen(s));
    char *q;

    for(q=(char *)buf; *s; s++)
    { if ( islower(*s) )
	*q++ = toupper(*s);
      else if ( *s == '_' )
	*q++ = syntax.word_separator;
      else
	*q++ = *s;
    }
    *q = EOS;

    return CtoName((char *)buf);
  }

  return CtoName(s);
}


		/********************************
		*           ANALYSIS		*
		********************************/

#ifdef BENCHNAMES

static Int
GetBucketsName(Name name)
{ answer(toInt(buckets));
}


static Int
GetBenchName(Name name, Int count)
{ int cnt = valInt(count);
  int n;

  str_eq_failed = 0;

  for(;;)
  { for(n=0; n<buckets; n++)
    { Name nm;

      if ( (nm = name_table[n]) )
      { if ( cnt-- > 0 )
	  StringToName(&nm->data);
	else
	  answer(toInt(str_eq_failed));
      }
    }
  }
}

#endif /*BENCHNAMES*/

		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declaractions */

static char *T_syntax[] =
        { "{uppercase}", "char" };

/* Instance Variables */

#define var_name NULL
/*
vardecl var_name[] =
{
};
*/

/* Send Methods */

static senddecl send_name[] =
{ SM((Name)NAME_initialise, 1, "text=char_array", initialiseName,
     DEFAULT, "Create a name from name"),
  SM((Name)NAME_unlink, 0, NULL, unlinkName,
     DEFAULT, "Trap error"),
  SM((Name)NAME_equal, 1, "name", equalObject,
     (Name)NAME_compare, "Test if names represent same text"),
  SM((Name)NAME_Value, 1, "char_array", ValueName,
     (Name)NAME_internal, "Modify name, preserving identity"),
  SM((Name)NAME_syntax, 2, T_syntax, syntaxName,
     (Name)NAME_internal, "Upcase and map word separator"),
  SM((Name)NAME_register, 0, NULL, registerName,
     (Name)NAME_oms, "Register in unique table")
};

/* Get Methods */

static getdecl get_name[] =
{ GM((Name)NAME_copy, 0, "name", NULL, getCopyName,
     DEFAULT, "The name itself"),
  GM((Name)NAME_modify, 1, "name", "char_array", getModifyName,
     DEFAULT, "Lookup or create argument name"),
  GM((Name)NAME_convert, 1, "name", "any", getConvertName,
     (Name)NAME_conversion, "Convert `text-convertable'"),
  GM((Name)NAME_lookup, 1, "name", "char_array", getLookupName,
     (Name)NAME_oms, "Perform lookup in current name-table"),
#ifdef BENCHNAMES
  GM((Name)NAME_Bench, 1, "int", "int", GetBenchName,
     (Name)NAME_statistics, "Lookup text of this name <n> times"),
  GM((Name)NAME_Buckets, 0, "int", NULL, GetBucketsName,
     (Name)NAME_statistics, "Number of buckets in name-table"),
#endif
  GM((Name)NAME_bucketValue, 1, "name", "0..", getBucketValueName,
     (Name)NAME_statistics, "Name at indicated bucket"),
  GM((Name)NAME_hashValue, 0, "int", NULL, getHashValueName,
     (Name)NAME_statistics, "Its hash-value")
};

/* Resources */

#define rc_name NULL
/*
static classvardecl rc_name[] =
{
};
*/

/* Class Declaration */

static Name name_termnames[] = { (Name)NAME_value };

ClassDecl(name_decls,
          var_name, send_name, get_name, rc_name,
          1, name_termnames,
          "$Rev$");


status
makeClassName(Class class)
{ declareClass(class, &name_decls);

  cloneStyleClass(class, (Name) NAME_none);

  succeed;
}
