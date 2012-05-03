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
  double  theta; \
  double  m, n; \
  double  two_r_m, two_r_n, rm, rn, hm, hn; \
  double  cp0, sp0;
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(oea, "Oblated Equal Area") "\n\tMisc Sph\n\tn= m= theta=";
FORWARD(s_forward); /* sphere */
  double Az, M, N, cp, sp, cl, shz;

  cp = cos(lp.phi);
  sp = sin(lp.phi);
  cl = cos(lp.lam);
  Az = proj_atan2(cp * sin(lp.lam), P->cp0 * sp - P->sp0 * cp * cl) + P->theta;
  shz = sin(0.5 * proj_acos(P->sp0 * sp + P->cp0 * cp * cl));
  M = proj_asin(shz * sin(Az));
  N = proj_asin(shz * cos(Az) * cos(M) / cos(M * P->two_r_m));
  xy.y = P->n * sin(N * P->two_r_n);
  xy.x = P->m * sin(M * P->two_r_m) * cos(N) / cos(N * P->two_r_n);
  return (xy);
}
INVERSE(s_inverse); /* sphere */
  double N, M, xp, yp, z, Az, cz, sz, cAz;

  N = P->hn * proj_asin(xy.y * P->rn);
  M = P->hm * proj_asin(xy.x * P->rm * cos(N * P->two_r_n) / cos(N));
  xp = 2. * sin(M);
  yp = 2. * sin(N) * cos(M * P->two_r_m) / cos(M);
  cAz = cos(Az = proj_atan2(xp, yp) - P->theta);
  z = 2. * proj_asin(0.5 * hypot(xp, yp));
  sz = sin(z);
  cz = cos(z);
  lp.phi = proj_asin(P->sp0 * cz + P->cp0 * sz * cAz);
  lp.lam = proj_atan2(sz * sin(Az),
    P->cp0 * cz - P->sp0 * sz * cAz);
  return (lp);
}
FREEUP; if (P) free(P); }
ENTRY0(oea)
  if (((P->n = proj_param(P->params, "dn").f) <= 0.) ||
    ((P->m = proj_param(P->params, "dm").f) <= 0.))
    E_ERROR(-39)
  else {
    P->theta = proj_param(P->params, "rtheta").f;
    P->sp0 = sin(P->phi0);
    P->cp0 = cos(P->phi0);
    P->rn = 1./ P->n;
    P->rm = 1./ P->m;
    P->two_r_n = 2. * P->rn;
    P->two_r_m = 2. * P->rm;
    P->hm = 0.5 * P->m;
    P->hn = 0.5 * P->n;
    P->fwd = s_forward;
    P->inv = s_inverse;
    P->es = 0.;
  }
ENDENTRY(P)
/*
** Log: proj_oea.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
