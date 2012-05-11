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
PROJ_HEAD(mill, "Miller Cylindrical") "\n\tCyl, Sph";
FORWARD(s_forward); /* spheroid */
  (void) P; /* avoid warning */
  xy.x = lp.lam;
  xy.y = log(tan(FORTPI + lp.phi * .4)) * 1.25;
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  (void) P; /* avoid warning */
  lp.lam = xy.x;
  lp.phi = 2.5 * (atan(exp(.8 * xy.y)) - FORTPI);
  return (lp);
}
FREEUP; if (P) free(P); }
ENTRY0(mill) P->es = 0.; P->inv = s_inverse; P->fwd = s_forward; ENDENTRY(P)
/*
** Log: proj_mill.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
