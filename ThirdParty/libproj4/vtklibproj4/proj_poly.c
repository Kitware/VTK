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
  double ml0; \
  void *en;
#define PROJ_LIB__
#include <lib_proj.h>
PROJ_HEAD(poly, "Polyconic (American)")
  "\n\tConic, Sph&Ell";
#define TOL  1e-10
#define CONV  1e-10
#define I_ITER 20
#define ITOL 1.e-12
FORWARD(e_forward); /* ellipsoid */
  double  ms, sp, cp;

  if (fabs(lp.phi) <= TOL) { xy.x = lp.lam; xy.y = -P->ml0; }
  else {
    sp = sin(lp.phi);
    ms = fabs(cp = cos(lp.phi)) > TOL ? proj_msfn(sp, cp, P->es) / sp : 0.;
    xy.x = ms * sin(lp.lam *= sp);
    xy.y = (proj_mdist(lp.phi, sp, cp, P->en) - P->ml0) + ms * (1. - cos(lp.lam));
  }
  return (xy);
}
FORWARD(s_forward); /* spheroid */
  double  cot, E;

  if (fabs(lp.phi) <= TOL) { xy.x = lp.lam; xy.y = P->ml0; }
  else {
    cot = 1. / tan(lp.phi);
    xy.x = sin(E = lp.lam * sin(lp.phi)) * cot;
    xy.y = lp.phi - P->phi0 + cot * (1. - cos(E));
  }
  return (xy);
}
INVERSE(e_inverse); /* ellipsoid */
  xy.y += P->ml0;
  if (fabs(xy.y) <= TOL) { lp.lam = xy.x; lp.phi = 0.; }
  else {
    double r, c, sp, cp, s2ph, ml, mlb, mlp, dPhi;
    int i;

    r = xy.y * xy.y + xy.x * xy.x;
    for (lp.phi = xy.y, i = I_ITER; i ; --i) {
      sp = sin(lp.phi);
      s2ph = sp * ( cp = cos(lp.phi));
      if (fabs(cp) < ITOL)
        I_ERROR;
      c = sp * (mlp = sqrt(1. - P->es * sp * sp)) / cp;
      ml = proj_mdist(lp.phi, sp, cp, P->en);
      mlb = ml * ml + r;
      mlp = P->one_es / (mlp * mlp * mlp);
      lp.phi += ( dPhi =
        ( ml + ml + c * mlb - 2. * xy.y * (c * ml + 1.) ) / (
        P->es * s2ph * (mlb - 2. * xy.y * ml) / c +
        2.* (xy.y - ml) * (c * mlp - 1. / s2ph) - mlp - mlp ));
      if (fabs(dPhi) <= ITOL)
        break;
    }
    if (!i)
      I_ERROR;
    c = sin(lp.phi);
    lp.lam = asin(xy.x * tan(lp.phi) * sqrt(1. - P->es * c * c)) / sin(lp.phi);
  }
  return (lp);
}
INVERSE(s_inverse); /* spheroid */
  double B, dphi, tp;
  int i;

  if (fabs(xy.y = P->phi0 + xy.y) <= TOL) { lp.lam = xy.x; lp.phi = 0.; }
  else {
    lp.phi = xy.y;
    B = xy.x * xy.x + xy.y * xy.y;
    i = I_ITER;
    do {
      tp = tan(lp.phi);
      lp.phi -= (dphi = (xy.y * (lp.phi * tp + 1.) - lp.phi -
        .5 * ( lp.phi * lp.phi + B) * tp) /
        ((lp.phi - xy.y) / tp - 1.));
    } while (fabs(dphi) > CONV && --i);
    if (! i) I_ERROR;
    lp.lam = asin(xy.x * tan(lp.phi)) / sin(lp.phi);
  }
  return (lp);
}
FREEUP; if (P) { if (P->en) free(P->en); free(P); } }
ENTRY1(poly, en)
  if (P->es) {
    if (!(P->en = proj_mdist_ini(P->es))) E_ERROR_0;
    P->ml0 = proj_mdist(P->phi0, sin(P->phi0), cos(P->phi0), P->en);
    P->inv = e_inverse;
    P->fwd = e_forward;
  } else {
    P->ml0 = -P->phi0;
    P->inv = s_inverse;
    P->fwd = s_forward;
  }
ENDENTRY(P)
/*
** Log: proj_poly.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
