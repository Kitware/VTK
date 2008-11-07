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
#define PROJ_PARMS__ \
  double C_x, C_y, A, B, D;
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(putp6, "Putnins P6") "\n\tPCyl., Sph.";
PROJ_HEAD(putp6p, "Putnins P6'") "\n\tPCyl., Sph.";
#define EPS  1e-10
#define NITER  10
#define CON_POLE 1.732050807568877
FORWARD(s_forward); /* spheroid */
  double p, r, V;
  int i;

  p = P->B * sin(lp.phi);
  lp.phi *=  1.10265779;
  for (i = NITER; i ; --i) {
    r = sqrt(1. + lp.phi * lp.phi);
    lp.phi -= V = ( (P->A - r) * lp.phi - log(lp.phi + r) - p ) /
      (P->A - 2. * r);
    if (fabs(V) < EPS)
      break;
  }
  if (!i)
    lp.phi = p < 0. ? -CON_POLE : CON_POLE;
  xy.x = P->C_x * lp.lam * (P->D - sqrt(1. + lp.phi * lp.phi));
  xy.y = P->C_y * lp.phi;
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  double r;

  lp.phi = xy.y / P->C_y;
  r = sqrt(1. + lp.phi * lp.phi);
  lp.lam = xy.x / (P->C_x * (P->D - r));
  lp.phi = proj_asin( ( (P->A - r) * lp.phi - log(lp.phi + r) ) / P->B);
  return (lp);
}
FREEUP; if (P) free(P); }
  static PROJ *
setup(PROJ *P) {
  P->es = 0.;
  P->inv = s_inverse;
  P->fwd = s_forward;
  return P;
}
ENTRY0(putp6)
  P->C_x = 1.01346;
  P->C_y = 0.91910;
  P->A   = 4.;
  P->B   = 2.1471437182129378784;
  P->D   = 2.;
ENDENTRY(setup(P))
ENTRY0(putp6p)
  P->C_x = 0.44329;
  P->C_y = 0.80404;
  P->A   = 6.;
  P->B   = 5.61125;
  P->D   = 3.;
ENDENTRY(setup(P))
/*
** Log: proj_putp6.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
