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
PROJ_HEAD(mbtfpq, "McBryde-Thomas Flat-Polar Quartic") "\n\tCyl., Sph.";
#define NITER  20
#define EPS  1e-7
#define ONETOL 1.000001
#define C  1.70710678118654752440
#define RC  0.58578643762690495119
#define FYC  1.87475828462269495505
#define RYC  0.53340209679417701685
#define FXC  0.31245971410378249250
#define RXC  3.20041258076506210122
FORWARD(s_forward); /* spheroid */
  double th1, c;
  int i;
  (void) P; /* avoid warning */

  c = C * sin(lp.phi);
  for (i = NITER; i; --i) {
    lp.phi -= th1 = (sin(.5*lp.phi) + sin(lp.phi) - c) /
      (.5*cos(.5*lp.phi)  + cos(lp.phi));
    if (fabs(th1) < EPS) break;
  }
  xy.x = FXC * lp.lam * (1.0 + 2. * cos(lp.phi)/cos(0.5 * lp.phi));
  xy.y = FYC * sin(0.5 * lp.phi);
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  double t;
  (void) P; /* avoid warning */

  lp.phi = RYC * xy.y;
  if (fabs(lp.phi) > 1.) {
    if (fabs(lp.phi) > ONETOL)  I_ERROR
    else if (lp.phi < 0.) { t = -1.; lp.phi = -PI; }
    else { t = 1.; lp.phi = PI; }
  } else
    lp.phi = 2. * asin(t = lp.phi);
  lp.lam = RXC * xy.x / (1. + 2. * cos(lp.phi)/cos(0.5 * lp.phi));
  lp.phi = RC * (t + sin(lp.phi));
  if (fabs(lp.phi) > 1.)
    if (fabs(lp.phi) > ONETOL)  I_ERROR
    else      lp.phi = lp.phi < 0. ? -HALFPI : HALFPI;
  else
    lp.phi = asin(lp.phi);
  return (lp);
}
FREEUP; if (P) free(P); }
ENTRY0(mbtfpq) P->es = 0.; P->inv = s_inverse; P->fwd = s_forward; ENDENTRY(P)
/*
** Log: proj_mbtfpq.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
