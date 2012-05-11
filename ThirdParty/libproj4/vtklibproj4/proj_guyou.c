/*
** libproj -- library of cartographic projections
**
** Copyright (c) 2005, 2006   Gerald I. Evenden
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
#define GUYOU 1
#define PEIRCE_Q 2
#define ADAMS_HEMI 3
#define ADAMS_WSI 4
#define ADAMS_WSII 5
#define TOL 1e-9
#define RSQRT2 0.7071067811865475244008443620
#define PROJ_LIB__
#define PROJ_PARMS__ \
  int mode;
#include  <lib_proj.h>
PROJ_HEAD(guyou, "Guyou") "\n\tMisc., Sph., NoInv.";
PROJ_HEAD(peirce_q, "Pierce Quincuncial") "\n\tMisc., Sph., NoInv.";
PROJ_HEAD(adams_hemi, "Adams Hemisphere in a Square") "\n\tMisc., Sph., NoInv.";
PROJ_HEAD(adams_wsI, "Adams World in a Square I") "\n\tMisc., Sph., NoInv.";
PROJ_HEAD(adams_wsII, "Adams World in a Square II") "\n\tMisc., Sph., NoInv.";
#define TWO_OVER_PI 0.6366197723675813430755350534
#define ORDER 8
/* Procedure to compute elliptic integral of the first kind
 * where k^2=0.5.  Precision good to better than 1e-7
 * The approximation is performed with an even Chebyshev
 * series, thus the coefficients below are the even values
 * and where series evaluation  must be multiplied by the argument. */
  double
ell_int_5(double phi) {
  int i = ORDER;
  double d1 = 0., d2 = 0., y, y2, temp;
  const double C[] = { /* even coefficients */
    2.19174570831038,
    0.0914203033408211,
    -0.00575574836830288,
    -0.0012804644680613,
    5.30394739921063e-05,
    3.12960480765314e-05,
    2.02692115653689e-07,
    -8.58691003636495e-07};
  double const *Cp = C + ORDER - 1;

  y = phi * TWO_OVER_PI;
  y = 2. * y * y - 1.;
  y2 = 2. * y;
  while (--i) {
    temp = d1;
    d1 = y2 * d1 - d2 + *Cp--;
    d2 = temp;
  }
  return phi * (y * d1 - d2 + 0.5 * *Cp);
}
FORWARD(s_forward); /* spheroid */
  double m, n, a=0., b=0.;
  int sm=0, sn=0;
  switch (P->mode) {
  case GUYOU:
    if ((fabs(lp.lam) - TOL) > HALFPI) F_ERROR;
    if (fabs(fabs(lp.phi) - HALFPI) < TOL) {
      xy.x = 0;
      xy.y = lp.phi < 0 ? -1.85407 : 1.85407;
      return xy;
    } else {
      double sl = sin(lp.lam);
      double sp = sin(lp.phi);
      double cp = cos(lp.phi);
      a = proj_acos((cp * sl - sp) * RSQRT2);
      b = proj_acos((cp * sl + sp) * RSQRT2);
      sm = lp.lam < 0.;
      sn = lp.phi < 0.;
    }
    break;
  case PEIRCE_Q: {
      double sl = sin(lp.lam);
      double cl = cos(lp.lam);
      double cp = cos(lp.phi);
      a = proj_acos(cp * (sl + cl) * RSQRT2);
      b = proj_acos(cp * (sl - cl) * RSQRT2);
      sm = sl < 0.;
      sn = cl > 0.;
    }
    break;
  case ADAMS_HEMI: {
      double sp = sin(lp.phi);
      if ((fabs(lp.lam) - TOL) > HALFPI) F_ERROR;
      a = cos(lp.phi) * sin(lp.lam);
      sm = (sp + a) < 0.;
      sn = (sp - a) < 0.;
      a = proj_acos(a);
      b = HALFPI - lp.phi;
    }
    break;
  case ADAMS_WSI: {
      double sp = tan(0.5 * lp.phi);
      b = cos(proj_asin(sp)) * sin(0.5 * lp.lam);
      a = proj_acos((b - sp) * RSQRT2);
      b = proj_acos((b + sp) * RSQRT2);
      sm = lp.lam < 0.;
      sn = lp.phi < 0.;
    }
    break;
  case ADAMS_WSII: {
      double spp = tan(0.5 * lp.phi);
      a = cos(proj_asin(spp)) * sin(0.5 * lp.lam);
      sm = (spp + a) < 0.;
      sn = (spp - a) < 0.;
      b = proj_acos(spp);
      a = proj_acos(a);
    }
    break;
  }
  m = proj_asin(sqrt(fabs(1. + cos(a + b))));
  if (sm) m = -m;
  n = proj_asin(sqrt(fabs(1. - cos(a - b))));
  if (sn) n = -n;
  xy.x = ell_int_5(m);
  xy.y = ell_int_5(n);
  if (P->mode == ADAMS_HEMI || P->mode == ADAMS_WSII) { /* rotate by 45deg. */
    double temp = xy.x;
    xy.x = RSQRT2 * (xy.x - xy.y);
    xy.y = RSQRT2 * (temp + xy.y);
  }
  return (xy);
}
FREEUP; if (P) free(P); }
  static void*
setup(PROJ *P) {
    P->es = 0;
  P->fwd = s_forward;
  return P;
}
ENTRY0(guyou) P->mode = GUYOU; ENDENTRY(setup(P))
ENTRY0(peirce_q) P->mode = PEIRCE_Q; ENDENTRY(setup(P))
ENTRY0(adams_hemi) P->mode = ADAMS_HEMI; ENDENTRY(setup(P))
ENTRY0(adams_wsI) P->mode = ADAMS_WSI; ENDENTRY(setup(P))
ENTRY0(adams_wsII) P->mode = ADAMS_WSII; ENDENTRY(setup(P))
/*
** Log: proj_guyou.c
** Revision 3.2  2008/06/26 14:48:50  gie
** initialized some parameters
**
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
