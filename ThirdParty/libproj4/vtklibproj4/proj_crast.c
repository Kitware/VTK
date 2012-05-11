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
PROJ_HEAD(crast, "Craster Parabolic (Putnins P4)")
"\n\tPCyl., Sph.";
#define XM  0.97720502380583984317
#define RXM  1.02332670794648848847
#define YM  3.06998012383946546542
#define RYM  0.32573500793527994772
#define THIRD  0.333333333333333333
FORWARD(s_forward); /* spheroid */
  (void) P; /* avoid warning */
  lp.phi *= THIRD;
  xy.x = XM * lp.lam * (2. * cos(lp.phi + lp.phi) - 1.);
  xy.y = YM * sin(lp.phi);
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  (void) P; /* avoid warning */
  lp.phi = 3. * asin(xy.y * RYM);
  lp.lam = xy.x * RXM / (2. * cos((lp.phi + lp.phi) * THIRD) - 1);
  return (lp);
}
FREEUP; if (P) free(P); }
ENTRY0(crast) P->es = 0.; P->inv = s_inverse; P->fwd = s_forward; ENDENTRY(P)
/*
** Log: proj_crast.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
