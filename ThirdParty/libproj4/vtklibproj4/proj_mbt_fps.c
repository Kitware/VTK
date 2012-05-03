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
#include  <lib_proj.h>
PROJ_HEAD(mbt_fps, "McBryde-Thomas Flat-Pole Sine (No. 2)") "\n\tCyl., Sph.";
#define MAX_ITER  10
#define LOOP_TOL  1e-7
#define C1 0.45503
#define C2 1.36509
#define C3 1.41546
#define C_x 0.22248
#define C_y 1.44492
#define C1_2 0.33333333333333333333333333
FORWARD(s_forward); /* spheroid */
  double k, V, t;
  int i;
  (void) P; /* avoid warning */

  k = C3 * sin(lp.phi);
  for (i = MAX_ITER; i ; --i) {
    t = lp.phi / C2;
    lp.phi -= V = (C1 * sin(t) + sin(lp.phi) - k) /
      (C1_2 * cos(t) + cos(lp.phi));
    if (fabs(V) < LOOP_TOL)
      break;
  }
  t = lp.phi / C2;
  xy.x = C_x * lp.lam * (1. + 3. * cos(lp.phi)/cos(t) );
  xy.y = C_y * sin(t);
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  double t;
  (void) P; /* avoid warning */

  lp.phi = C2 * (t = proj_asin(xy.y / C_y));
  lp.lam = xy.x / (C_x * (1. + 3. * cos(lp.phi)/cos(t)));
  lp.phi = proj_asin((C1 * sin(t) + sin(lp.phi)) / C3);
  return (lp);
}
FREEUP; if (P) free(P); }
ENTRY0(mbt_fps) P->es = 0; P->inv = s_inverse; P->fwd = s_forward; ENDENTRY(P)
/*
** Log: proj_mbt_fps.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
