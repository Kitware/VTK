/*
** libproj -- library of cartographic projections
**
** Copyright (c) 2003, 2006   Gerald I. Evenden
*/
static const char
LIBPROJ_ID[] = "Id";
/*
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#define PROJ_LIB__
# include  <lib_proj.h>
#define Cx 0.5641895835477562869480794515
#define Cy 0.8862269254527580136490837416

PROJ_HEAD(four2, "Fournier II") "\n\tPCyl.";
FORWARD(s_forward); /* spheroid */
  (void) P; /* avoid warning */

  xy.x = Cx * lp.lam * cos(lp.phi);
  xy.y = Cy * sin(lp.phi);
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  (void) P; /* avoid warning */

  lp.phi = asin(xy.y / Cy);
  lp.lam = xy.x / (Cx * cos(lp.phi));
  return(lp);
}
FREEUP; if (P) free(P); }
ENTRY0(four2) P->es = 0.; P->fwd = s_forward; P->inv = s_inverse; ENDENTRY(P)
/*
** Log: proj_four2.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
