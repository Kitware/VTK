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
  double  phi1; \
  double  fxa; \
  double  fxb; \
  int    mode;
#define EPS  1e-9
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(rpoly, "Rectangular Polyconic")
  "\n\tConic, Sph., no inv.\n\tlat_ts=";
FORWARD(s_forward); /* spheroid */
  double fa;

  if (P->mode)
    fa = tan(lp.lam * P->fxb) * P->fxa;
  else
    fa = 0.5 * lp.lam;
  if (fabs(lp.phi) < EPS) {
    xy.x = fa + fa;
    xy.y = - P->phi0;
  } else {
    xy.y = 1. / tan(lp.phi);
    xy.x = sin(fa = 2. * atan(fa * sin(lp.phi))) * xy.y;
    xy.y = lp.phi - P->phi0 + (1. - cos(fa)) * xy.y;
  }
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(rpoly)
  if ((P->mode = (P->phi1 = fabs(proj_param(P->params, "rlat_ts").f)) > EPS)) {
    P->fxb = 0.5 * sin(P->phi1);
    P->fxa = 0.5 / P->fxb;
  }
  P->es = 0.; P->fwd = s_forward;
ENDENTRY(P)
/*
** Log: proj_rpoly.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
