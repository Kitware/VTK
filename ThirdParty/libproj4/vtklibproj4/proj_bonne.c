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
  double phi1; \
  double cphi1; \
  double am1; \
  double m1; \
  void *en;
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(bonne, "Bonne (Werner lat_1=90)")
  "\n\tConic Sph&Ell\n\tlat_1=";
#define EPS10  1e-10
FORWARD(e_forward); /* ellipsoid */
  double rh, E, c;

  rh = P->am1 + P->m1 - proj_mdist(lp.phi, E = sin(lp.phi), c = cos(lp.phi), P->en);
  E = c * lp.lam / (rh * sqrt(1. - P->es * E * E));
  xy.x = rh * sin(E);
  xy.y = P->am1 - rh * cos(E);
  return (xy);
}
FORWARD(s_forward); /* spheroid */
  double E, rh;

  rh = P->cphi1 + P->phi1 - lp.phi;
  if (fabs(rh) > EPS10) {
    xy.x = rh * sin(E = lp.lam * cos(lp.phi) / rh);
    xy.y = P->cphi1 - rh * cos(E);
  } else
    xy.x = xy.y = 0.;
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  double rh;

  rh = hypot(xy.x, xy.y = P->cphi1 - xy.y);
  lp.phi = P->cphi1 + P->phi1 - rh;
  if (fabs(lp.phi) > HALFPI) I_ERROR;
  if (fabs(fabs(lp.phi) - HALFPI) <= EPS10)
    lp.lam = 0.;
  else
    lp.lam = rh * atan2(xy.x, xy.y) / cos(lp.phi);
  return (lp);
}
INVERSE(e_inverse); /* ellipsoid */
  double s, rh;

  rh = hypot(xy.x, xy.y = P->am1 - xy.y);
  lp.phi = proj_inv_mdist(P->am1 + P->m1 - rh, P->en);
  if ((s = fabs(lp.phi)) < HALFPI) {
    s = sin(lp.phi);
    lp.lam = rh * atan2(xy.x, xy.y) *
       sqrt(1. - P->es * s * s) / cos(lp.phi);
  } else if (fabs(s - HALFPI) <= EPS10)
    lp.lam = 0.;
  else I_ERROR;
  return (lp);
}
FREEUP;
  if (P) {
    if (P->en)
      free(P->en);
    free(P);
  }
}
ENTRY1(bonne, en)
  double c;

  P->phi1 = proj_param(P->params, "rlat_1").f;
  if (fabs(P->phi1) < EPS10) E_ERROR(-23);
  if (P->es) {
    if (!(P->en = proj_mdist_ini(P->es))) E_ERROR_0;
    P->m1 = proj_mdist(P->phi1, P->am1 = sin(P->phi1),
      c = cos(P->phi1), P->en);
    P->am1 = c / (sqrt(1. - P->es * P->am1 * P->am1) * P->am1);
    P->inv = e_inverse;
    P->fwd = e_forward;
  } else {
    if (fabs(P->phi1) + EPS10 >= HALFPI)
      P->cphi1 = 0.;
    else
      P->cphi1 = 1. / tan(P->phi1);
    P->inv = s_inverse;
    P->fwd = s_forward;
  }
ENDENTRY(P)
/*
** Log: proj_bonne.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
