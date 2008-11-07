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
#define MAX_ITER 10
#define DEL_TOL 1e-12
#define PROJ_PARMS__ \
  void  *en; \
  double  r0, l, M0; \
  double  C;
#define PROJ_LIB__
#include  <lib_proj.h>

PROJ_HEAD(lcca, "Lambert Conformal Conic Alternative")
  "\n\tConic, Sph&Ell\n\tlat_0=";

  static double /* func to compute dr */
fS(double S, double C) {
    return(S * ( 1. + S * S * C));
}
  static double /* deriv of fs */
fSp(double S, double C) {
  return(1. + 3.* S * S * C);
}
FORWARD(e_forward); /* ellipsoid */
  double S, r, dr;
  
  S = proj_mdist(lp.phi, sin(lp.phi), cos(lp.phi), P->en) - P->M0;
  dr = fS(S, P->C);
  r = P->r0 - dr;
  xy.x = P->k0 * (r * sin( lp.lam *= P->l ) );
  xy.y = P->k0 * (P->r0 - r * cos(lp.lam) );
  return (xy);
}
INVERSE(e_inverse); /* ellipsoid & spheroid */
  double theta, dr, S, dif;
  int i;

  xy.x /= P->k0;
  xy.y /= P->k0;
  theta = atan2(xy.x , P->r0 - xy.y);
  dr = xy.y - xy.x * tan(0.5 * theta);
  lp.lam = theta / P->l;
  S = dr;
  for (i = MAX_ITER; i ; --i) {
    S -= (dif = (fS(S, P->C) - dr) / fSp(S, P->C));
    if (fabs(dif) < DEL_TOL) break;
  }
  if (!i) I_ERROR
  lp.phi = proj_inv_mdist(S + P->M0, P->en);
  return (lp);
}
FREEUP; if (P) { if (P->en) free(P->en); free(P); } }
ENTRY0(lcca)
  double s2p0, N0, R0, tan0, tan20;

  if (!((P->en = proj_mdist_ini(P->es)))) E_ERROR_0;
  if (!proj_param(P->params, "tlat_0").i) E_ERROR(-44);
  if (P->phi0 == 0.) E_ERROR(-45);
  P->l = sin(P->phi0);
  P->M0 = proj_mdist(P->phi0, P->l, cos(P->phi0), P->en);
  s2p0 = P->l * P->l;
  R0 = 1. / (1. - P->es * s2p0);
  N0 = sqrt(R0);
  R0 *= P->one_es * N0;
  tan0 = tan(P->phi0);
  tan20 = tan0 * tan0;
  P->r0 = N0 / tan0;
  P->C = 1. / (6. * R0 * N0);
  P->inv = e_inverse;
  P->fwd = e_forward;
ENDENTRY(P)
/*
** Log: proj_lcca.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
