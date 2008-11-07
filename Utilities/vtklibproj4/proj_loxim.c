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
  double cosphi1; \
  double tanphi1;
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(loxim, "Loximuthal") "\n\tPCyl Sph";
#define EPS  1e-8
FORWARD(s_forward); /* spheroid */
  xy.y = lp.phi - P->phi1;
  if (fabs(xy.y) < EPS)
    xy.x = lp.lam * P->cosphi1;
  else {
    xy.x = FORTPI + 0.5 * lp.phi;
    if (fabs(xy.x) < EPS || fabs(fabs(xy.x) - HALFPI) < EPS)
      xy.x = 0.;
    else
      xy.x = lp.lam * xy.y / log( tan(xy.x) / P->tanphi1 );
  }
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  lp.phi = xy.y + P->phi1;
  if (fabs(xy.y) < EPS)
    lp.lam = xy.x / P->cosphi1;
  else
    if (fabs( lp.lam = FORTPI + 0.5 * lp.phi ) < EPS ||
      fabs(fabs(lp.lam) - HALFPI) < EPS)
      lp.lam = 0.;
    else
      lp.lam = xy.x * log( tan(lp.lam) / P->tanphi1 ) / xy.y ;
  return (lp);
}
FREEUP; if (P) free(P); }
ENTRY0(loxim);
  P->phi1 = proj_param(P->params, "rlat_1").f;
  if ((P->cosphi1 = cos(P->phi1)) < EPS) E_ERROR(-22);
  P->tanphi1 = tan(FORTPI + 0.5 * P->phi1);
  P->inv = s_inverse; P->fwd = s_forward;
  P->es = 0.;
ENDENTRY(P)
/*
** Log: proj_loxim.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
