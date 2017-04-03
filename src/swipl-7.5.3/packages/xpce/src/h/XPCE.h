/*  Part of XPCE --- The SWI-Prolog GUI toolkit

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        jan@swi.psy.uva.nl
    WWW:           http://www.swi.psy.uva.nl/projects/xpce/
    Copyright (c)  2005-2011, University of Amsterdam
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

#ifndef XPCE_H_INCLUDED
#define XPCE_H_INCLUDED
#include "kernel.h"

		 /*******************************
		 *	     WIN32 DLL		*
		 *******************************/

#ifndef __pce_export
#if defined(PCE_INCLUDED) && (defined(WIN32) || defined(__CYWIN32__))
#define __pce_export __declspec(dllexport)
#else
#define __pce_export extern
#endif /*PCE_INCLUDED*/
#endif /*__pce_export*/


		 /*******************************
		 *	       TYPES		*
		 *******************************/

typedef SendFunc XPCE_send_function_t;
typedef Class	 XPCE_class_t;

typedef struct XPCE_class_definition_t
{ const char		*name;		/* name of the class */
  const char		*super;		/* name of the super-class */
  XPCE_send_function_t	makefunction;	/* function to create the class */
  XPCE_class_t 		*global;	/* pointer to global Class Struct */
  const char 		*summary;	/* Summary definition */
} XPCE_class_definition_t;

#define XPCE_BeginClasses(id) \
	const XPCE_class_definition_t id[] = {
#define XPCE_EndClasses \
	XPCE_Class(NULL, NULL, NULL, NULL, NULL) \
	};
#define XPCE_Class(name, super, make, global, summary) \
	{ name, super, make, global, summary }

		 /*******************************
		 *	      CLASSDEF		*
		 *******************************/

#undef SM
#undef GM
#undef RC
#undef IV
#undef SV

#define SM(n, a, t, f, g, s)	{ (Name)(n), a, t, \
				  (SendFunc) f, (Name)(g), s }
#define GM(n, a, r, t, f, g, s) { (Name)(n), a, r, t, (GetFunc) f, \
 				  (Name)(g), s }
#define RC(n, t, d, s)		{ (Name)(n), t, d, s }
#define IV(n, t, f, g, s)	{ (Name)(n), t, f, NULL, \
				  (Name)(g), s }
#define SV(n, t, f, c, g, s)	{ (Name)(n), t, f, (void *)(c), \
				  (Name)(g), s }

#define XPCE_CLASSDECL(name, vs, ss, gs, rs, tn) \
	static classdecl name = \
	{ vs, ss, gs, rs, \
	  IVEntries(vs), SMEntries(ss), GMEntries(gs), RCEntries(rs), \
	  TNEntries(tn), (Name *)tn, __FILE__ \
	}


		 /*******************************
		 *	      MACROS		*
		 *******************************/

#undef assign

#define assign(o, s, v)	XPCE_assignField((Instance) (o), \
					 (Any *) &((o)->s), \
					 (Any) (v))

		 /*******************************
		 *	    CONSTANTS		*
		 *******************************/

typedef enum
{ XPCE_NIL_ID = 0,
  XPCE_DEFAULT_ID,
  XPCE_CLASSDEFAULT_ID,
  XPCE_ON_ID,
  XPCE_OFF_ID
} xpce_constant_id;

#ifndef XPCE_PUBLIC_IMPL
#undef NIL
#undef DEFAULT
#undef CLASSDEFAULT
#undef ON
#undef OFF

#define NIL		XPCE_constant(XPCE_NIL_ID) 		/* @nil */
#define DEFAULT		XPCE_constant(XPCE_DEFAULT_ID) 		/* @default */
#define CLASSDEFAULT	XPCE_constant(XPCE_CLASSDEFAULT_ID)
#define ON		XPCE_constant(XPCE_ON_ID) 		/* @on */
#define OFF		XPCE_constant(XPCE_OFF_ID) 		/* @off */
#endif /*XPCE_PUBLIC_IMPL*/

		 /*******************************
		 *	    PROTOTYPES		*
		 *******************************/

__pce_export int  XPCE_define_classes(const XPCE_class_definition_t *defs);
__pce_export int  XPCE_declare_class(Class Class, classdecl *decl);
__pce_export void XPCE_assignField(Instance instance, Any *field, Any value);
__pce_export Any  XPCE_constant(xpce_constant_id id);

#endif /*XPCE_H_INCLUDED*/
