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
#include <h/graphics.h>
#include <math.h>

static status
initialisePoint(Point p, Int x, Int y)
{ if ( isDefault(x) ) x = ZERO;
  if ( isDefault(y) ) y = ZERO;
  assign(p, x, x);
  assign(p, y, y);

  succeed;
}


static Point
getConvertPoint(Class class, Any obj)
{ if ( instanceOfObject(obj, ClassEvent) )
    return getPositionEvent((EventObj) obj, DEFAULT);
  else
  { CharArray ca = obj;
    int x, y;

    if ( isstrA(&ca->data) &&
	 sscanf((char *)ca->data.s_textA, "%d,%d", &x, &y) == 2 )
      answer(newObject(ClassPoint, toInt(x), toInt(y), EAV));
  }

  fail;
}


static StringObj
getPrintNamePoint(Point p)
{ char buf[200];

  sprintf(buf, INTPTR_FORMAT "," INTPTR_FORMAT, valInt(p->x), valInt(p->y));
  answer(CtoString(buf));
}


status
equalPoint(Point p1, Point p2)
{ if (p1->x == p2->x && p1->y == p2->y)
    succeed;
  fail;
}


status
copyPoint(Point p1, Point p2)
{ assign(p1, x, p2->x);
  assign(p1, y, p2->y);

  succeed;
}


static Point
getCopyPoint(Point p)
{ answer(answerObject(p->class, p->x, p->y, EAV));
}


status
setPoint(Point pt, Int x, Int y)
{ if ( notDefault(x) ) assign(pt, x, x);
  if ( notDefault(y) ) assign(pt, y, y);

  succeed;
}


status
offsetPoint(Point pt, Int x, Int y)
{ assign(pt, x, add(x,pt->x));
  assign(pt, y, add(y,pt->y));

  succeed;
}


static Point
getDifferencePoint(Point p, Point q)
{ answer(answerObject(ClassPoint, sub(p->x,q->x), sub(p->y,q->y),EAV));
}


int
get_distance_point(Point p, int x, int y)
{ double px = (double)valInt(p->x);
  double py = (double)valInt(p->y);
  double qx = (double)x;
  double qy = (double)y;
  double d = sqrt((qx-px)*(qx-px) + (qy-py)*(qy-py));

  return (int)d;
}


Int
getDistancePoint(Point p, Point q)
{ double px = (double)valInt(p->x);
  double py = (double)valInt(p->y);
  double qx = (double)valInt(q->x);
  double qy = (double)valInt(q->y);
  double d = sqrt((qx-px)*(qx-px) + (qy-py)*(qy-py));
  long di = (long)d;

  answer(toInt(di));
}


Point
getMidPoint(Point p, Point q)
{ answer(answerObject(ClassPoint,
		      toInt((valInt(p->x)+valInt(q->x)+1)/2),
		      toInt((valInt(p->y)+valInt(q->y)+1)/2),
		      EAV));
}


status
plusPoint(Point p, Point q)
{ assign(p, x, add(p->x,q->x));
  assign(p, y, add(p->y,q->y));

  succeed;
}


status
minusPoint(Point p, Point q)
{ assign(p, x, sub(p->x,q->x));
  assign(p, y, sub(p->y,q->y));

  succeed;
}


static status
mirrorPoint(Point p, Point q)
{ Int mx = ZERO, my = ZERO;

  if ( notDefault(q) )
  { mx = q->x;
    my = q->y;
  }

  assign(p, x, sub(mx, p->x));
  assign(p, y, sub(my, p->y));

  succeed;
}


static Point
getPlusPoint(Point p1, Point p2)
{ Point p = getCopyPoint(p1);

  plusPoint(p, p2);

  answer(p);
}


static Point
getMinusPoint(Point p1, Point p2)
{ Point p = getCopyPoint(p1);

  minusPoint(p, p2);

  answer(p);
}


static Point
getMirrorPoint(Point p1, Point p2)
{ Point p = getCopyPoint(p1);

  mirrorPoint(p, p2);

  answer(p);
}


/* Type declaractions */

static char *T_offset[] = { "dx=int", "dy=int" };
static char *T_xADintD_yADintD[] = { "x=[int]", "y=[int]" };

/* Instance Variables */

static vardecl var_point[] =
{ IV(NAME_x, "int", IV_BOTH,
     NAME_dimension, "x-coordinate of point"),
  IV(NAME_y, "int", IV_BOTH,
     NAME_dimension, "y-coordinate of point")
};

/* Send Methods */

static senddecl send_point[] =
{ SM(NAME_initialise, 2, T_xADintD_yADintD, initialisePoint,
     DEFAULT, "Create point from x- and y-value"),
  SM(NAME_minus, 1, "point", minusPoint,
     NAME_calculate, "Subtract x and y of argument point"),
  SM(NAME_mirror, 1, "origin=[point]", mirrorPoint,
     NAME_calculate, "Mirror point around argument or (0,0)"),
  SM(NAME_offset, 2, T_offset, offsetPoint,
     NAME_calculate, "Move the point by given x and y"),
  SM(NAME_plus, 1, "point", plusPoint,
     NAME_calculate, "Add x- and y-values of argument point"),
  SM(NAME_set, 2, T_xADintD_yADintD, setPoint,
     NAME_calculate, "Set x- and y-values"),
  SM(NAME_equal, 1, "to=point", equalPoint,
     NAME_compare, "Test if argument is same position"),
  SM(NAME_copy, 1, "from=point", copyPoint,
     NAME_copy, "Copy x- and y values from the argument")
};

/* Get Methods */

static getdecl get_point[] =
{ GM(NAME_difference, 1, "point", "to=point", getDifferencePoint,
     NAME_calculate, "New point that reflects distance"),
  GM(NAME_distance, 1, "int", "to=point", getDistancePoint,
     NAME_calculate, "Integer for distance between points"),
  GM(NAME_mid, 1, "point", "to=point", getMidPoint,
     NAME_calculate, "New point in the middle between points"),
  GM(NAME_minus, 1, "point", "point", getMinusPoint,
     NAME_calculate, "New point p1 - p2"),
  GM(NAME_mirror, 1, "point", "origin=[point]", getMirrorPoint,
     NAME_calculate, "New point mirrored over argument"),
  GM(NAME_plus, 1, "point", "point", getPlusPoint,
     NAME_calculate, "New point p1 + p2"),
  GM(NAME_copy, 0, "point", NULL, getCopyPoint,
     NAME_copy, "New point with same <-x and <-y"),
  GM(NAME_convert, 1, "point", "event|char_array", getConvertPoint,
     NAME_textual, "Event <-position or the text `x,y'"),
  GM(NAME_printName, 0, "string", NULL, getPrintNamePoint,
     NAME_textual, "Printed representation as %d,%d")
};

/* Resources */

#define rc_point NULL
/*
static classvardecl rc_point[] =
{
};
*/

/* Class Declaration */

static Name point_termnames[] = { NAME_x, NAME_y };

ClassDecl(point_decls,
          var_point, send_point, get_point, rc_point,
          2, point_termnames,
          "$Rev$");

status
makeClassPoint(Class class)
{ return declareClass(class, &point_decls);
}
