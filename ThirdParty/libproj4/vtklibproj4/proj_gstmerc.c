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
  double  chi; \
  double  aks0, aks5; \
  void  *en;
#define PROJ_LIB__
#include  <lib_proj.h>
#define EPS10 1e-10

PROJ_HEAD(gstmerc, "Gauss-Schreiber Transverse Mercator") "\n\tCyl, Ell";

FORWARD(e_forward); /* ellipsoid */

  /* translate to Gaussian sphere */
  lp = proj_gauss(lp, P->en);
  /* basic spherical transverse projection */
  xy.x = P->aks0 * atanh(cos(lp.phi)*sin(lp.lam));
  xy.y = P->aks0 * (atan2(tan(lp.phi),cos(lp.lam)));
  return (xy);
}
INVERSE(e_inverse); /* ellipsoid */
  double D;

  xy.x /= P->aks0;
  D = xy.y / P->aks0 + P->chi;
  lp.phi = proj_asin(sin(D)/cosh(xy.x));
  lp.lam = atan2(sinh(xy.x), cos(D));
  lp = proj_inv_gauss(lp, P->en);
  return (lp);
}
FREEUP;
  if (P) {
    if (P->en)
      free(P->en);
    free(P);
  }
}
ENTRY1(gstmerc, en)
  double R;

  if (!(P->en = proj_gauss_ini(P->e, P->phi0, &(P->chi), &R)))
    E_ERROR_0;
  P->aks5 = 0.5 * (P->aks0 = P->k0 * R);
  P->fwd = e_forward;
  P->inv = e_inverse;
ENDENTRY(P)
/*
** Log: proj_gstmerc.c
** Revision 3.2  2008/07/28 15:43:09  gie
** renamed to gstmerc
**
** Revision 3.1  2008/06/26 14:22:18  gie
** Schreiber transverse mercator reestablished
**
*/
