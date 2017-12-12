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

static Any	loadNameObject(IOSTREAM *);
static int	pceSlotsClass(Class);
static status	checkConvertedObject(Any, ClassDef);
static Int	storeClass(Class, FileObj);
static status	storeExtensionsObject(Any obj, FileObj file);
static status	storeIdObject(Any obj, Int id, FileObj file);
static status	storeSlotsClass(Class class, FileObj file);
static status	restoreClass(IOSTREAM *fd);
static int	offsetVariable(Class class, Name name);

static int objects_saved;
static int classes_saved;
static int save_nesting;		/* depth of saved object */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Binary saved state of PCE object (collection). File format:

<file>		::= <magic>
		    <version>
		    <object>
		    {'s' <object>}		(= support objects)
		    {'n' <from> <slot> <to>}	(= nil-references)
		    {'r' <from> <slot> {'R' <to>} 'x'} (= reference-chains)
		    'x'

<magic>		::= <string>			(= SAVEMAGIC)
<version>	::= <word>			(= SAVEVERSION)

<object>	::= ['C' <class_def>]		(a Class slot definition)
		    'O'
		    <class_id>
		    <object_name>
		    {<extension>} 'x'
		    {<slot>}			(times according to class def)
		  | 'd'				(@default)
		  | 'n'				(@nil)
		  | 'a'				(@on)
		  | 'u'				(@off)
		  | 'r'				(@receiver)
		  | 'b'				(@block)
		  | '1'				(@arg1)
		  | '2'				(@arg2)
		  | '3'				(@arg3)
		  | '4'				(@arg4)
		  | '5'				(@arg5)
		  | '6'				(@arg6)
		  | '7'				(@arg7)
		  | '8'				(@arg8)
		  | '9'				(@arg9)
		  | '0'				(@arg10)
		  | 'N' <string>		(a name)
		  | 'S' <string> <string>	(HACK: a lisp_symbol)
		  | 'I' <integer>		(an integer)
		  | 'R' <object_name>		(reference to saved object)
		  | 'A' <string>		(reference to exernal object)
		  | 'D' <object>		(Descriptive object)

<extension>	::= 'a' <Object>		(Attribute sheet)
		  | 'c' <Object>		(Constraint-list)
		  | 's' <Object>		(SendMethod-list)
		  | 'g' <Object>		(GetMethod-list)
		  | 'r' <Object>		(Recogniser-list)
		  | 'h' <Object>		(Hyper-list)

<object_name>	::= 'N' <string>		(name as reference)
		  | 'I' <word>			(integer as reference)
<abtract>	::= <slot>
<slot>		::= <object>

<class_def>	::= <class_name> <class_id>
		    <slots>			(number of pce typed slots)
		    {<slot_name>}		(<slots> times)
<class_name>	::= <string>
<class_id>	::= <word>
<slots>		::= <word>
<slot_name>	::= <string>
<slot_offset>	::= <byte>			(offset of slot above struct)
						(`object')

<string>	::= <size>{<char>}		(<size> times <char>)
<char>		::= <byte>

<byte>		::= (8 bits)
<word>		::= (32 bits)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static HashTable saveTable;
static HashTable saveClassTable;
static HashTable saveNilRefTable;
static Chain     candidateSaveRelations;

static inline Int
setSavedObj(Any obj)
{ objects_saved++;

  appendHashTable(saveTable, obj, toInt(objects_saved));

  return toInt(objects_saved);
}

static inline Int
setSavedClass(Class class)
{ classes_saved++;

  appendHashTable(saveClassTable, class, toInt(classes_saved));

  return toInt(classes_saved);
}

Int
isSavedObject(Any obj)
{ return getMemberHashTable(saveTable, obj);
}

#define isSavedClass(class)	getMemberHashTable(saveClassTable, class)

static status
candidateSaveRelation(Any r)
{ if ( !isSavedObject(r) )
  { if ( !candidateSaveRelations )
      candidateSaveRelations = newObject(ClassChain, r, EAV);
    else
      appendChain(candidateSaveRelations, r);
  }

  succeed;
}


static status
saveRelations(FileObj f)
{ Any r;

  while( candidateSaveRelations &&
	 (r = getDeleteHeadChain(candidateSaveRelations)) )
  { if ( !isSavedObject(r) )
      TRY(send(r, NAME_SaveRelation, f, EAV));
  }

  succeed;
}


static status
saveNilRefs(FileObj f)
{ if ( saveNilRefTable )
  { for_hash_table(saveNilRefTable, s,
		   { Instance inst = s->name;
		     Variable var  = s->value;
		     Any to = inst->slots[valInt(var->offset)];
		     Int ref;

		     if ( onDFlag(var, D_CLONE_REFCHAIN) )
		     { Cell cell;
		       Chain ch = to;

		       storeCharFile(f, 'r');
		       storeIntFile(f, storeClass(classOfObject(inst), f));
		       storeIdObject(inst, isSavedObject(inst), f);
		       storeIntFile(f, var->offset);
		       for_cell(cell, ch)
		       { if ( (ref=isSavedObject(cell->value)) )
			 { storeCharFile(f, 'R');
			   storeIdObject(cell->value, ref, f);
			 }
		       }
		       storeCharFile(f, 'x');
		     } else
		     { if ( (ref = isSavedObject(to)) )
		       { DEBUG(NAME_save,
			       Cprintf("storing nil-ref %s-%s->%s\n",
				       pp(inst), pp(var->name), pp(to)));
			 storeCharFile(f, 'n');
			 storeIntFile(f, storeClass(classOfObject(inst), f));
			 storeIdObject(inst, isSavedObject(inst), f);
			 storeIntFile(f, var->offset);
			 storeIdObject(to, ref, f);
		       }
		     }
		   });

    freeHashTable(saveNilRefTable);
    saveNilRefTable = NULL;
  }

  succeed;
}


status
saveInFileObject(Any obj, FileObj file)
{ status result;
  string magic;

  TRY(send(file, NAME_kind, NAME_binary, EAV) &&
      send(file, NAME_open, NAME_write, EAV));

  if ( SaveMagic == NULL )
    SaveMagic = SAVEMAGIC;

  objects_saved = classes_saved = save_nesting = 0;
  str_set_n_ascii(&magic, strlen(SaveMagic), SaveMagic);
  storeStringFile(file, &magic);
  storeWordFile(file, (Any) SAVEVERSION);
  saveTable = createHashTable(toInt(256), NAME_none);
  saveClassTable = createHashTable(toInt(256), NAME_none);
  if ( candidateSaveRelations )
    clearChain(candidateSaveRelations);
  result = (storeObject(obj, file) &&
	    saveRelations(file) &&
	    saveNilRefs(file) &&
	    storeCharFile(file, 'x'));
  closeFile(file);
  if ( !result )
    removeFile(file);
  DEBUG(NAME_statistics, Cprintf("Saved %d objects of %d classes\n",
				 objects_saved, classes_saved));
  freeHashTable(saveTable);
  freeHashTable(saveClassTable);


  return result ? SUCCEED : FAIL;
}


status
storeObject(Any obj, FileObj file)
{ /*DEBUG(NAME_save, Cprintf("Storing %s from %ld\n",
	  pp(obj), ftell(file->fd)));*/

  if ( isInteger(obj) )
  { storeCharFile(file, 'I');
    storeIntFile(file, obj);
    succeed;
  }

  assert(isObject(obj));

  if ( instanceOfObject(obj, ClassVar) )
  { intptr_t a = (char*)obj - (char*)Arg(1);

    a++;				/* count 1.. */
    if ( a >= 1 && a <= 9 )
      return storeCharFile(file, '0' + (int) a);
    else if ( a == 10 )
      return storeCharFile(file, '0');
    else if ( obj == RECEIVER )
      return storeCharFile(file, 'r');
  } else if ( instanceOfObject(obj, ClassConstant) )
  { if ( isNil(obj) )
      return storeCharFile(file, 'n');
    else if ( isDefault(obj) )
      return storeCharFile(file, 'd');
    else if ( isOn(obj) )		/* booleans are constants! */
      return storeCharFile(file, 'a');
    else if ( isOff(obj) )
      return storeCharFile(file, 'u');
  }

  { Class class = classOfObject(obj);
    Name name;

    if ( isAClass(class, ClassName) )
    { if ( class == ClassName )
      { storeCharFile(file, 'N');
	storeNameFile(file, obj);
	succeed;
      } else if ( class->name == NAME_lispSymbol ) /* HACK */
      { storeCharFile(file, 'S');
	storeNameFile(file, obj);
	storeNameFile(file, get(obj, NAME_package, EAV));
	succeed;
      }
    }

    DEBUG(NAME_save, Cprintf(" [%3d] Storing %s from %ld\n",
			     save_nesting, pp(obj), Stell(file->fd)));

    if ( class->saveStyle == NAME_nil )
    { return storeCharFile(file, 'n');
    } else if ( class->saveStyle == NAME_external &&
	        (name = getNameAssoc(obj)) )
    { storeCharFile(file, 'A');
      storeNameFile(file, name);
      succeed;
    } else /*if ( equalName(class->saveStyle, NAME_normal) )*/
    { Int ref, classref;
      status rval;
      Any sref;

      if ( (ref = isSavedObject(obj)) )
      { DEBUG(NAME_save, Cprintf("Storing reference\n"));
	storeCharFile(file, 'R');
	return storeIdObject(obj, ref, file);
      }

      if ( (sref = qadGetv(obj, NAME_storageReference, 0, NULL)) )
      { storeCharFile(file, 'D');
	storeNameFile(file, class->name);
	return storeObject(sref, file);
      }

      ref = setSavedObj(obj);

      TRY( classref = storeClass(class, file) );
      storeCharFile(file, 'O');
      storeIntFile(file, classref);
      storeIdObject(obj, ref, file);
      storeExtensionsObject(obj, file);
      save_nesting++;
      if ( class->saveFunction )
      { DEBUG(NAME_save, Cprintf("Using private function\n"));
	rval = (*class->saveFunction)(obj, file);
      } else
      { if ( allPceSlotsClass(class) )
	  rval = storeSlotsObject(obj, file);
	else
	{ errorPce(obj, NAME_cannotSaveObject, NAME_alienData);
	  rval = storeObject(NIL, file);
	}
      }
      save_nesting--;

      return rval;
    }
  }
}


static status
storeExtensionsObject(Any obj, FileObj file)
{ if ( onFlag(obj, F_CONSTRAINT|F_ATTRIBUTE|F_SENDMETHOD|F_GETMETHOD|
	           F_HYPER|F_RECOGNISER) )
  { if ( onFlag(obj, F_CONSTRAINT) )
    { storeCharFile(file, 'c');
      storeObject(getAllConstraintsObject(obj, ON), file);
    }
    if ( onFlag(obj, F_ATTRIBUTE) )
    { storeCharFile(file, 'a');
      storeObject(getAllAttributesObject(obj, ON), file);
    }
    if ( onFlag(obj, F_SENDMETHOD) )
    { storeCharFile(file, 's');
      storeObject(getAllSendMethodsObject(obj, ON), file);
    }
    if ( onFlag(obj, F_GETMETHOD) )
    { storeCharFile(file, 'g');
      storeObject(getAllGetMethodsObject(obj, ON), file);
    }
    if ( onFlag(obj, F_HYPER) )
    { Chain hypers = getAllHypersObject(obj, ON);
      Cell cell;

      for_cell(cell, hypers)
	candidateSaveRelation(cell->value);
    }
    if ( onFlag(obj, F_RECOGNISER) )
    { storeCharFile(file, 'r');
      storeObject(getAllRecognisersGraphical(obj, ON), file);
    }
  }

  return storeCharFile(file, 'x');
}


static status
storeSlotObject(Instance inst, Variable var, FileObj file)
{ int i = valInt(var->offset);
  Any val = inst->slots[i];

  if ( onDFlag(var, D_SAVE_NORMAL) )
    return storeObject(val, file);

  if ( onDFlag(var, D_SAVE_NIL|D_CLONE_REFCHAIN) )
  { if ( isSavedObject(val) )
      return storeObject(val, file);
    if ( !saveNilRefTable )
      saveNilRefTable = createHashTable(toInt(32), NAME_none);
    appendHashTable(saveNilRefTable, inst, var);
    storeObject(NIL, file);
  }

  succeed;
}


status
storeSlotsObject(Any obj, FileObj file)
{ Class class = classOfObject(obj);

  for_vector(class->instance_variables, Variable var,
	     storeSlotObject(obj, var, file));

  succeed;
}


static status
storeIdObject(Any obj, Int id, FileObj file)
{ Name name;

  if ( (name = getNameAssoc(obj)) )
  { storeCharFile(file, 'N');
    storeNameFile(file, name);
    succeed;
  }
  storeCharFile(file, 'I');
  storeIntFile(file, id);
  succeed;
}



/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StoreClass stores the instance layout, as  far as PCE  typed slots are
concerned.  Alien slots  are taken care  of by  specialised load/store
functions defined on the class itself.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static Int
storeClass(Class class, FileObj file)
{ Int ref;

  if ( (ref = isSavedClass(class)) )
    return ref;

  ref = setSavedClass(class);
  storeCharFile(file, 'C');
  storeNameFile(file, class->name);
  storeIntFile(file, ref);
  storeIntFile(file, toInt(pceSlotsClass(class)));
  storeSlotsClass(class, file);

  return ref;
}


static int
pceSlotsClass(Class class)
{ int pce_slots = 0;
  int slots = valInt(class->slots);
  int n;

  for(n = 0; n<slots; n++)
    if ( isPceSlot(class, n) )
      pce_slots++;

  return pce_slots;
}


static status
storeSlotsClass(Class class, FileObj file)
{ for_vector(class->instance_variables, Variable var,
	     if ( var->type->kind != NAME_alien )
	       storeNameFile(file, var->name));

  succeed;
}


		/********************************
		*            LOADING            *
		*********************************/

struct classdef
{ Class	class;			/* current class structure */
  Name	class_name;		/* name of this class */
  int	slots;			/* number of saved slots */
  int	*offset;		/* array of slot offsets */
  Name  *name;			/* array of slot-names */
};

static HashTable savedClassTable;	/* table with saved classes */
static HashTable restoreTable;		/* restored objects table */
static Chain	 restoreMessages;	/* messages for restoration */

int
loadWord(IOSTREAM *fd)
{
#ifndef WORDS_BIGENDIAN
  union
  { unsigned int  l;
    unsigned char c[4];
  } cvrt;
  int rval;

  cvrt.l = Sgetw(fd);
  rval = (cvrt.c[0] << 24) |
         (cvrt.c[1] << 16) |
	 (cvrt.c[2] << 8) |
	  cvrt.c[3];
  DEBUG(NAME_byteOrder, Cprintf("loadWord(0x%lx) --> %ld\n", cvrt.l, rval));
  return rval;
#else /*WORDS_BIGENDIAN*/
  return Sgetw(fd);
#endif /*WORDS_BIGENDIAN*/
}


#ifdef WORDS_BIGENDIAN
static const int double_byte_order[] = { 7,6,5,4,3,2,1,0 };
#else
static const int double_byte_order[] = { 0,1,2,3,4,5,6,7 };
#endif

#define BYTES_PER_DOUBLE (sizeof(double_byte_order)/sizeof(int))

double
loadDouble(IOSTREAM *fd)
{ double f;
  unsigned char *cl = (unsigned char *)&f;
  unsigned int i;

  for(i=0; i<BYTES_PER_DOUBLE; i++)
  { int c = Sgetc(fd);

    cl[double_byte_order[i]] = c;
  }

  return f;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
loadStringFile() loads a string saved using storeStringFile().  See there for
format details.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int
loadStringFile(IOSTREAM *fd, String s)
{ int size = loadWord(fd);

  if ( size >= 0 )
  { str_inithdr(s, FALSE);
    s->s_size = size;

    str_alloc(s);
    if ( Sfread(s->s_textA, sizeof(char), size, fd) != (size_t)size )
      fail;
  } else
  { int i;
    charW *d;
    IOENC oenc;

    str_inithdr(s, TRUE);
    s->s_size = -size;
    str_alloc(s);

    oenc = fd->encoding;
    fd->encoding = ENC_UTF8;
    for(d=s->s_textW, i=0; i<s->s_size; i++)
    { int chr = Sgetcode(fd);

      if ( chr != EOF )
      { *d++ = chr;
      } else
      { fd->encoding = oenc;
	fail;
      }
    }
  }

  succeed;
}


static Name
loadName(IOSTREAM *fd)
{ string s;

  if ( loadStringFile(fd, &s) )
  { Name name = StringToName(&s);
    str_unalloc(&s);

    return name;
  }

  return NULL;
}


void
restoreMessage(Any msg)
{ if ( !restoreMessages )
    restoreMessages = newObject(ClassChain, EAV);

  appendChain(restoreMessages, msg);
}


static status
loadNilRef(IOSTREAM * fd)
{ Int classid  = toInt(loadWord(fd));
  Any r1       = loadNameObject(fd);
  int offset   = loadWord(fd);
  Any r2       = loadNameObject(fd);
  ClassDef def = getMemberHashTable(savedClassTable, classid);
  Instance f   = getMemberHashTable(restoreTable, r1);
  Any t        = getMemberHashTable(restoreTable, r2);

  if ( !def )
    return errorPce(LoadFile, NAME_noSavedClassDef, classid);
  if ( !f )
    return errorPce(LoadFile, NAME_referencedObjectNotLoaded, r1);
  if ( !t )
    return errorPce(LoadFile, NAME_referencedObjectNotLoaded, r2);

  if ( def->offset[offset] >= 0 )
  { DEBUG(NAME_save, Cprintf("Restoring (nil)ref %s-%s --> %s\n",
			     pp(f), pp(def->name[offset]), pp(t)));
    assignField(f, &(f->slots[def->offset[offset]]), t);
  }
  /* else slot is gone; no problem I think */

  succeed;
}


static status
loadReferenceChain(IOSTREAM *fd)
{ Int classid  = toInt(loadWord(fd));
  Any r1       = loadNameObject(fd);
  int offset   = loadWord(fd);
  ClassDef def = getMemberHashTable(savedClassTable, classid);
  Instance f   = getMemberHashTable(restoreTable, r1);

  if ( !def )
    return errorPce(LoadFile, NAME_noSavedClassDef, classid);
  if ( !f )
    return errorPce(LoadFile, NAME_referencedObjectNotLoaded, r1);

  if ( def->offset[offset] >= 0 )
  { Chain ch = newObject(ClassChain, EAV);
    int c;

    assignField(f, &(f->slots[def->offset[offset]]), ch);
    do
    { switch((c=Sgetc(fd)))
      { case 'R':
	{ Any r2 = loadNameObject(fd);
	  Any o2 = getMemberHashTable(restoreTable, r2);

	  if ( !o2 )
	    return errorPce(LoadFile, NAME_referencedObjectNotLoaded, r2);
	  appendChain(ch, o2);
	  break;
	}
	case 'x':
	  break;
	default:
	  errorPce(f, NAME_illegalCharacter, toInt(c), toInt(Stell(fd)));
	  fail;
      }
    } while( c != 'x' );
  }
  /* else slot is gone; no problem I think */

  succeed;
}


status
checkObjectMagic(IOSTREAM *fd)
{ status rval;
  long l;
  long ls;

  if ( SaveMagic == NULL )
    SaveMagic = SAVEMAGIC;

  ls = (long)strlen(SaveMagic);

  if ( (l=loadWord(fd)) == ls )
  { char tmp[LINESIZE];

    Sfread(tmp, sizeof(char), sizeof(SAVEMAGIC)-1, fd);
    tmp[ls] = EOS;
    DEBUG(NAME_save, Cprintf("magic = ``%s''; SaveMagic = ``%s''\n",
			     tmp, SaveMagic) );
    if ( strncmp(tmp, SaveMagic, ls - 1) == 0 )
      rval = SUCCEED;
    else
      rval = FAIL;
  } else
  { rval = FAIL;
    DEBUG(NAME_save, Cprintf("First word = %ld, should be %d\n", l, ls) );
  }

  return rval;
}


Any
getObjectSourceSink(SourceSink f)
{ IOSTREAM *fd;
  Any result;

  if ( !(fd = Sopen_object(f, "rbr")) )
    fail;

  LoadFile = f;				/* TBD: pass as argument */

  if ( !checkObjectMagic(fd) )
  { Sclose(fd);

    errorPce(f, NAME_badFile, NAME_object);
    fail;
  }

  restoreVersion = loadWord(fd);
  if ( restoreVersion != SAVEVERSION )
    errorPce(f, NAME_newSaveVersion,
	     toInt(restoreVersion), toInt(SAVEVERSION));

  savedClassTable = createHashTable(toInt(128), NAME_none);
  restoreTable = createHashTable(toInt(256), NAME_none);
  if ( restoreMessages )
    clearChain(restoreMessages);
  if ( (result = loadObject(fd)) )
    addCodeReference(result);
  if ( restoreVersion >= 13 )
  { char c;

    do
    { switch((c=Sgetc(fd)))
      { case 's':			/* support (relation) objects */
	  if ( !loadObject(fd) )
	    fail;			/* TBD */
	  break;
	case 'n':
	  if ( !loadNilRef(fd) )
	    fail;
	  break;
        case 'r':
	  if ( !loadReferenceChain(fd) )
	    fail;
	  break;
	case 'x':
	  break;
	default:
	  errorPce(f, NAME_illegalCharacter, toInt(c), toInt(Stell(fd)));
	  fail;
      }
    } while( c != 'x' );
  }

  freeHashTable(restoreTable);
  freeHashTable(savedClassTable);
  Sclose(fd);

  if ( result )
  { if ( restoreMessages )
    { Any msg;

      while((msg= getDeleteHeadChain(restoreMessages)))
	forwardCodev(msg, 0, NULL);
    }

    delCodeReference(result);
    pushAnswerObject(result);
  }

  LoadFile = NULL;

  answer(result);
}


static void
updateFlagsObject(Any obj)
{ if ( instanceOfObject(obj, ClassFunction) )
    setFlag(obj, F_ACTIVE|F_NOTANY);
}


static status
loadExtensionsObject(Instance obj, IOSTREAM *fd)
{ if ( restoreVersion <= 7 )
    succeed;				/* extensions in interceptor */

  for(;;)
  { char c;
    Any ext;

    if ( restoreVersion == 8 )
    { if ( (c=Sgetc(fd)) != 'e' )
      { Sungetc(c, fd);
	succeed;
      }
    }

    switch(c=Sgetc(fd))
    { case 'x':
	succeed;
      case 'a':
	setFlag(obj, F_ATTRIBUTE);
	appendHashTable(ObjectAttributeTable, obj, ext = loadObject(fd));
	addRefObj(ext);
	break;
      case 'c':
	setFlag(obj, F_CONSTRAINT);
	appendHashTable(ObjectConstraintTable, obj, ext = loadObject(fd));
	addRefObj(ext);
	break;
      case 's':
	setFlag(obj, F_SENDMETHOD);
	appendHashTable(ObjectSendMethodTable, obj, ext = loadObject(fd));
	addRefObj(ext);
	break;
      case 'g':
	setFlag(obj, F_GETMETHOD);
	appendHashTable(ObjectGetMethodTable, obj, ext = loadObject(fd));
	addRefObj(ext);
	break;
      case 'r':
	setFlag(obj, F_RECOGNISER);
	appendHashTable(ObjectRecogniserTable, obj, ext = loadObject(fd));
	addRefObj(ext);
	break;
      case 'h':
	setFlag(obj, F_HYPER);
	appendHashTable(ObjectHyperTable, obj, ext = loadObject(fd));
	addRefObj(ext);
	break;
      default:
	errorPce(LoadFile, NAME_illegalCharacter, toInt(c), toInt(Stell(fd)));
	fail;
    }
  }
}


Any
loadObject(IOSTREAM *fd)
{ int c;
#ifndef O_RUNTIME
  long start = 0;
#endif

  DEBUG(NAME_save, start = Stell(fd));

  switch( c = Sgetc(fd) )
  { case 'd':	return DEFAULT;
    case 'n':	return NIL;
    case 'a':	return ON;
    case 'u':	return OFF;
    case 'r':	return RECEIVER;
    case '1':	return Arg(1);
    case '2':	return Arg(2);
    case '3':	return Arg(3);
    case '4':	return Arg(4);
    case '5':	return Arg(5);
    case '6':	return Arg(6);
    case '7':	return Arg(7);
    case '8':	return Arg(8);
    case '9':	return Arg(9);
    case '0':	return Arg(10);
    case 'N':   return loadName(fd);
    case 'I':   return toInt(loadWord(fd));
    case 'R': { Any r;
		Any ref = loadNameObject(fd);

		if ( !(r = getMemberHashTable(restoreTable, ref)) )
		{ errorPce(LoadFile, NAME_referencedObjectNotLoaded, ref);
		  fail;;
		}
		return r;
	      }
    case 'A': { Any r;
		Name name = loadName(fd);

		if ( !(r = getObjectFromReferencePce(PCE, name)) )
		{ errorPce(NIL, NAME_noAssoc, name);
		  fail;
		}
		return r;
	      }
    case 'D': { Name classname = loadName(fd);
		Type t         = nameToType(classname);
		Any sref       = loadObject(fd);
		Any rval;

		if ( !isClassType(t) )
		{ errorPce(t, NAME_notClassType);
		  return NIL;
		}

		if ( (rval = checkType(sref, t, NIL)) )
		  return rval;

		errorPce(classname, NAME_cannotConvert, sref);
		return NIL;
	      }
    case 'C':	restoreClass(fd);
		if ( (c=Sgetc(fd)) != 'O' )
		{ errorPce(LoadFile, NAME_illegalCharacter,
			   toInt(c), toInt(Stell(fd)));
		  fail;
		}
		/* FALLTHROUGH */
    case 'O': { ClassDef def;
		Int classid = toInt(loadWord(fd));
		Any name;

		if ( !(def = getMemberHashTable(savedClassTable, classid)) )
		{ errorPce(LoadFile, NAME_noSavedClassDef, classid);
		  fail;
		}

		name = loadNameObject(fd);
		if ( def->class )
		{ Instance obj = allocObject(def->class, FALSE);

		  if ( isName(name) )
		    newAssoc(name, obj);
		  addCodeReference(obj);

		  DEBUG(NAME_save, Cprintf("Loading %s from %ld\n",
					   pp(obj), start));

		  appendHashTable(restoreTable, name, obj);
		  loadExtensionsObject(obj, fd);

		  if ( def->class->loadFunction != NULL )
		    (*def->class->loadFunction)(obj, fd, def);
		  else
		    loadSlotsObject(obj, fd, def);
		  updateFlagsObject(obj);

		  if ( SAVEVERSION != restoreVersion || PCEdebugging )
		    TRY(checkConvertedObject(obj, def));

		  createdClass(def->class, obj, NAME_loaded);

		  DEBUG(NAME_save, CheckObject(obj, OFF));
		  delCodeReference(obj);

		  return obj;
		} else			/* no class; load into sheet */
		{ int i;
		  Any slotValue;
		  Sheet sh = createObjectv(isName(name) ? name : (Name) NIL,
					   ClassSheet, 0, NULL);

		  valueSheet(sh, NAME_className, def->class_name);
		  DEBUG(NAME_save, Cprintf("Loading %s from %ld\n",
					   pp(sh), start));
		  appendHashTable(restoreTable, name, sh);
		  loadExtensionsObject((Any) sh, fd);

		  for( i=0; i<def->slots; i++ )
		  { if ( (slotValue = loadObject(fd)) == FAIL )
		      fail;
		    valueSheet(sh, def->name[i], slotValue);
		  }

		  DEBUG(NAME_save, CheckObject(sh, OFF));
		  return sh;
		}
	      }
    case 'S':				/* lisp-symbol hack */
	{ string ns, ps;

	  if ( loadStringFile(fd, &ns) &&
	       loadStringFile(fd, &ps) )
	  { Name name = StringToName(&ns);
	    Name package = StringToName(&ps);
	    Class symbol_class = getConvertClass(ClassClass, NAME_lispSymbol);
	    Any  symbol = newObject(symbol_class, name, package, EAV);

	    str_unalloc(&ns);
	    str_unalloc(&ps);

	    return symbol;
	  }

	  fail;
	}

    default:  { long index;

		index = Stell(fd) - 1;
		errorPce(LoadFile, NAME_illegalCharacter,
			 toInt(c), toInt(index));
		fail;
	      }
  }
}


static Any
loadNameObject(IOSTREAM *fd)
{ int c;

  switch( (c = Sgetc(fd)) )
  { case 'I':	return (Any) toInt(loadWord(fd));
    case 'N':	return (Any) loadName(fd);
    default:	errorPce(LoadFile, NAME_illegalCharacter,
			 toInt(c), toInt(Stell(fd)-1));
		fail;
  }
}


static status
restoreClass(IOSTREAM *fd)
{ Name name = loadName(fd);
  Int classid = toInt(loadWord(fd));
  int slots = loadWord(fd);
  int i;
  ClassDef def;

  if ( restoreVersion == 1 )
    slots++;

  def = alloc(sizeof(struct classdef));
  def->class_name = name;
  def->offset = alloc(slots * sizeof(int));
  def->name = alloc(slots * sizeof(Name));

  if ( (def->class = checkType(name, TypeClass, NIL)) )
    realiseClass(def->class);
  else
    errorPce(LoadFile, NAME_loadNoClass, name);
  def->slots = slots;
  appendHashTable(savedClassTable, classid, def);

  for( i = 0; i<slots; i++ )
  { Name name = loadName(fd);

    def->name[i] = name;
    if ( def->class )
    { def->offset[i] = offsetVariable(def->class, name);
      if ( def->offset[i] < 0 )
      { errorPce(LoadFile, NAME_loadOldSlot, def->class, name);
      }
    }
  }

  succeed;
}


static status
definedSlotClassDef(ClassDef def, Name slot)
{ int i;

  for(i=0; i<def->slots; i++)
    if ( def->name[i] == slot )
      succeed;

  fail;
}


static int
offsetVariable(Class class, Name name)
{ Variable var;

  if ( (var = getInstanceVariableClass(class, name)) &&
       var->type->kind != NAME_alien )
    return valInt(var->offset);

  return -1;
}


status
loadSlotsObject(Any obj, IOSTREAM *fd, ClassDef def)
{ int i;
  Any slotValue;
  Instance inst = obj;

  for( i=0; i<def->slots; i++ )
  { int slot;

    if ( (slotValue = loadObject(fd)) == FAIL )
      fail;
    if ( (slot = def->offset[i]) < 0 )	/* slot out of use */
    { if ( hasSendMethodObject(inst, NAME_convertOldSlot) ) send(inst,
NAME_convertOldSlot, def->name[i], slotValue, EAV); continue; } if (
restoreVersion != SAVEVERSION || PCEdebugging ) { Any converted;
Variable var = def->class->instance_variables->elements[slot];

      if ( (converted = checkType(slotValue, var->type, inst)) )
	slotValue = converted;
    }
    assignField(inst, &(inst->slots[slot]), slotValue);
  }

  succeed;
}


static status
checkConvertedObject(Any obj, ClassDef def)
{ Class class = def->class;
  int slots = valInt(class->slots);
  Instance inst = obj;
  int i;

  if ( hasSendMethodObject(inst, NAME_convertLoadedObject) )
    send(inst, NAME_convertLoadedObject,
	 toInt(restoreVersion),
	 toInt(SAVEVERSION), EAV);

  for(i=0; i<slots; i++)
  { if ( isPceSlot(class, i) )
    { Variable var = getInstanceVariableClass(class, toInt(i));
      Any value = inst->slots[i];

      if ( !var )
      { Cprintf("Can't find variable %d of %s\n", i, pp(class));
	continue;
      }

      if ( isDefault(value) && getClassVariableClass(class, var->name) )
	continue;

      if ( hasSendMethodObject(inst, NAME_initialiseNewSlot) &&
	   !definedSlotClassDef(def, var->name) )
	send(inst, NAME_initialiseNewSlot, var, EAV);
      value = inst->slots[i];

      if ( !checkType(value, var->type, inst) &&
	   !(isNil(value) && onDFlag(var, D_SAVE_NIL)) )
	errorPce(inst, NAME_badSlotValue, var->name, value);
    }
  }

  succeed;
}
