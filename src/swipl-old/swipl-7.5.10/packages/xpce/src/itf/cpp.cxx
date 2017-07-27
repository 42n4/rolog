/*  Part of XPCE --- The SWI-Prolog GUI toolkit

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        wielemak@science.uva.nl
    WWW:           http://www.swi-prolog.org/projects/xpce/
    Copyright (c)  1985-2007, University of Amsterdam
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

#include "md.h"

#ifndef __pce_export
#ifdef WIN32
#define __pce_export __declspec(dllexport)
#else
#define __pce_export extern
#endif
#endif /*__pce_export*/

#if O_CPLUSPLUS
#ifdef WIN32
#define __GLOBAL __pce_export
#else
#define __GLOBAL
#endif
#include <pce/Pce.h>
#include <pce/Call.h>
#include <pce/Class.h>
#include <stdarg.h>

extern "C" {
void Cprintf(const char *fmt, ...);
__pce_export PceStatus callCPlusPlusProc(Any f, int ac, const Any av[]);
__pce_export Any       callCPlusPlusFunc(Any f, int ac, const Any av[]);
__pce_export PceStatus callCPlusPlusPceMethodProc(Any o, Any f,
						  int ac, const Any av[]);
__pce_export Any       callCPlusPlusPceMethodFunc(Any o, Any f,
						  int ac, const Any av[]);
__pce_export PceStatus callCPlusPlusMethodProc(Any o, Any f,
					       int ac, const Any av[]);
__pce_export Any       callCPlusPlusMethodFunc(Any o, Any f,
					       int ac, const Any av[]);
}


#define A(n) PceArg(av[n])		/* PceArg argument */
#define R(o) PceReceiver(o)		/* PceReceiver argument */
#define C(p) (p)			/* C++ object pointer */

PceStatus
callCPlusPlusProc(Any f, int ac, const Any av[])
{ switch(ac)
  { case 0: return (*(PceProc0)f)();
    case 1: return (*(PceProc1)f)(A(0));
    case 2: return (*(PceProc2)f)(A(0), A(1));
    case 3: return (*(PceProc3)f)(A(0), A(1), A(2));
    case 4: return (*(PceProc4)f)(A(0), A(1), A(2), A(3));
    case 5: return (*(PceProc5)f)(A(0), A(1), A(2), A(3), A(4));
    case 6: return (*(PceProc6)f)(A(0), A(1), A(2), A(3), A(4), A(5));
    case 7: return (*(PceProc7)f)(A(0), A(1), A(2), A(3), A(4), A(5), A(6));
    case 8: return (*(PceProc8)f)(A(0), A(1), A(2), A(3), A(4), A(5), A(6),
				  A(7));
    case 9: return (*(PceProc9)f)(A(0), A(1), A(2), A(3), A(4), A(6), A(7),
				  A(7), A(8));
    default:
      Cprintf("[PCE: Too many C++ arguments]\n");
      return FAIL;
  }
}


Any
callCPlusPlusFunc(Any f, int ac, const Any av[])
{ switch(ac)
  { case 0: return (*(PceFunc0)f)().self;
    case 1: return (*(PceFunc1)f)(A(0)).self;
    case 2: return (*(PceFunc2)f)(A(0), A(1)).self;
    case 3: return (*(PceFunc3)f)(A(0), A(1), A(2)).self;
    case 4: return (*(PceFunc4)f)(A(0), A(1), A(2), A(3)).self;
    case 5: return (*(PceFunc5)f)(A(0), A(1), A(2), A(3), A(4)).self;
    case 6: return (*(PceFunc6)f)(A(0), A(1), A(2), A(3), A(4), A(5)).self;
    case 7: return (*(PceFunc7)f)(A(0), A(1), A(2), A(3), A(4), A(5),
				  A(6)).self;
    case 8: return (*(PceFunc8)f)(A(0), A(1), A(2), A(3), A(4), A(5),
				  A(6), A(7)).self;
    case 9: return (*(PceFunc9)f)(A(0), A(1), A(2), A(3), A(4), A(5),
				  A(6), A(7), A(8)).self;
    default:
      Cprintf("[PCE: Too many C++ arguments]\n");
      return NULL;
  }
}


PceStatus
callCPlusPlusMethodProc(Any obj, Any f, int ac, const Any av[])
{ switch(ac)
  { case 0:
      return (*(CppMethodProc0)f)(C(obj));
    case 1:
      return (*(CppMethodProc1)f)(C(obj), A(0));
    case 2:
      return (*(CppMethodProc2)f)(C(obj), A(0), A(1));
    case 3:
      return (*(CppMethodProc3)f)(C(obj), A(0), A(1), A(2));
    case 4:
      return (*(CppMethodProc4)f)(C(obj), A(0), A(1), A(2), A(3));
    case 5:
      return (*(CppMethodProc5)f)(C(obj), A(0), A(1), A(2), A(3), A(4));
    case 6:
      return (*(CppMethodProc6)f)(C(obj), A(0), A(1), A(2), A(3), A(4), A(5));
    case 7:
      return (*(CppMethodProc7)f)(C(obj), A(0), A(1), A(2), A(3), A(4), A(5),
				  A(6));
    case 8:
      return (*(CppMethodProc8)f)(C(obj), A(0), A(1), A(2), A(3), A(4), A(5),
				  A(6), A(7));
    case 9:
      return (*(CppMethodProc9)f)(C(obj), A(0), A(1), A(2), A(3), A(4), A(5),
				  A(6), A(7), A(8));
    default:
      Cprintf("[PCE: Too many C++ arguments]\n");
      return FAIL;
  }
}


Any
callCPlusPlusMethodFunc(Any obj, Any f, int ac, const Any av[])
{ switch(ac)
  { case 0:
      return (*(CppMethodFunc0)f)(C(obj)).self;
    case 1:
      return (*(CppMethodFunc1)f)(C(obj), A(0)).self;
    case 2:
      return (*(CppMethodFunc2)f)(C(obj), A(0), A(1)).self;
    case 3:
      return (*(CppMethodFunc3)f)(C(obj), A(0), A(1), A(2)).self;
    case 4:
      return (*(CppMethodFunc4)f)(C(obj), A(0), A(1), A(2), A(3)).self;
    case 5:
      return (*(CppMethodFunc5)f)(C(obj), A(0), A(1), A(2), A(3), A(4)).self;
    case 6:
      return (*(CppMethodFunc6)f)(C(obj), A(0), A(1), A(2), A(3), A(4),
				  A(5)).self;
    case 7:
      return (*(CppMethodFunc7)f)(C(obj), A(0), A(1), A(2), A(3), A(4),
				  A(5), A(6)).self;
    case 8:
      return (*(CppMethodFunc8)f)(C(obj), A(0), A(1), A(2), A(3), A(4),
				  A(5), A(6), A(7)).self;
    case 9:
      return (*(CppMethodFunc9)f)(C(obj), A(0), A(1), A(2), A(3), A(4),
				  A(5), A(6), A(7), A(8)).self;
    default:
      Cprintf("[PCE: Too many C++ arguments]\n");
      return NULL;
  }
}


		 /*******************************
		 *   C++ DEFINED PCE CLASSES	*
		 *******************************/

PceStatus
callCPlusPlusPceMethodProc(Any obj, Any f, int ac, const Any av[])
{ switch(ac)
  { case 0:
      return (*(PceMethodProc0)f)(R(obj));
    case 1:
      return (*(PceMethodProc1)f)(R(obj), A(0));
    case 2:
      return (*(PceMethodProc2)f)(R(obj), A(0), A(1));
    case 3:
      return (*(PceMethodProc3)f)(R(obj), A(0), A(1), A(2));
    case 4:
      return (*(PceMethodProc4)f)(R(obj), A(0), A(1), A(2), A(3));
    case 5:
      return (*(PceMethodProc5)f)(R(obj), A(0), A(1), A(2), A(3), A(4));
    case 6:
      return (*(PceMethodProc6)f)(R(obj), A(0), A(1), A(2), A(3), A(4), A(5));
    case 7:
      return (*(PceMethodProc7)f)(R(obj), A(0), A(1), A(2), A(3), A(4), A(5),
				  A(6));
    case 8:
      return (*(PceMethodProc8)f)(R(obj), A(0), A(1), A(2), A(3), A(4), A(5),
				  A(6), A(7));
    case 9:
      return (*(PceMethodProc9)f)(R(obj), A(0), A(1), A(2), A(3), A(4), A(5),
				  A(6), A(7), A(8));
    default:
      Cprintf("[PCE: Too many C++ arguments]\n");
      return FAIL;
  }
}


Any
callCPlusPlusPceMethodFunc(Any obj, Any f, int ac, const Any av[])
{ switch(ac)
  { case 0:
      return (*(PceMethodFunc0)f)(R(obj)).self;
    case 1:
      return (*(PceMethodFunc1)f)(R(obj), A(0)).self;
    case 2:
      return (*(PceMethodFunc2)f)(R(obj), A(0), A(1)).self;
    case 3:
      return (*(PceMethodFunc3)f)(R(obj), A(0), A(1), A(2)).self;
    case 4:
      return (*(PceMethodFunc4)f)(R(obj), A(0), A(1), A(2), A(3)).self;
    case 5:
      return (*(PceMethodFunc5)f)(R(obj), A(0), A(1), A(2), A(3), A(4)).self;
    case 6:
      return (*(PceMethodFunc6)f)(R(obj), A(0), A(1), A(2), A(3), A(4),
				  A(5)).self;
    case 7:
      return (*(PceMethodFunc7)f)(R(obj), A(0), A(1), A(2), A(3), A(4),
				  A(5), A(6)).self;
    case 8:
      return (*(PceMethodFunc8)f)(R(obj), A(0), A(1), A(2), A(3), A(4),
				  A(5), A(6), A(7)).self;
    case 9:
      return (*(PceMethodFunc9)f)(R(obj), A(0), A(1), A(2), A(3), A(4),
				  A(5), A(6), A(7), A(8)).self;
    default:
      Cprintf("[PCE: Too many C++ arguments]\n");
      return NULL;
  }
}

#endif /*O_CPLUSPLUS*/
