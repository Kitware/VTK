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
#define TOL 1e-10
#define PROJ_PARMS__ \
  double m0; \
  void *en;
#define PROJ_LIB__
# include  <lib_proj.h>
PROJ_HEAD(cass, "Cassini") "\n\tCyl, Sph&Ell";
# define EPS10  1e-10
# define C1  .16666666666666666666
# define C2  .00833333333333333333
# define C3  .04166666666666666666
# define C4  .33333333333333333333
# define C5  .06666666666666666666
FORWARD(e_forward); /* ellipsoid */
  double n, t, tn, a1, c, a2;
  
  xy.y = proj_mdist(lp.phi, n = sin(lp.phi), c = cos(lp.phi), P->en);
  n = 1./sqrt(1. - P->es * n * n);
  tn = tan(lp.phi); t = tn * tn;
  a1 = lp.lam * c;
  c *= P->es * c / (1 - P->es);
  a2 = a1 * a1;
  xy.x = n * a1 * (1. - a2 * t * (C1 - (8. - t + 8. * c) * a2 * C2));
  xy.y -= P->m0 - n * tn * a2 * (.5 + (5. - t + 6. * c) * a2 * C3);
  return (xy);
}
FORWARD(s_forward); /* spheroid */
  xy.x = asin(cos(lp.phi) * sin(lp.lam));
  xy.y = atan2(tan(lp.phi) , cos(lp.lam)) - P->phi0;
  return (xy);
}
INVERSE(e_inverse); /* ellipsoid */
  double phip, n, t, tn, r, dd, d2;

  phip = proj_inv_mdist(P->m0 + xy.y, P->en);
  if (fabs(fabs(phip) - HALFPI) < TOL) {
    lp.phi = phip;
    lp.lam = 0;
  } else {
    tn = tan(phip); t = tn * tn;
    n = sin(phip);
    r = 1. / (1. - P->es * n * n);
    n = sqrt(r);
    r *= (1. - P->es) * n;
    dd = xy.x / n;
    d2 = dd * dd;
    lp.phi = phip - (n * tn / r) * d2 * (.5 - (1. + 3. * t) * d2 * C3);
    lp.lam = dd * (1. + t * d2 * (-C4 + (1. + 3. * t) * d2 * C5))
        / cos(phip);
  }
  return (lp);
}
INVERSE(s_inverse); /* spheroid */
  double dd = xy.y + P->phi0;

  lp.phi = asin(sin(dd) * cos(xy.x));
  lp.lam = atan2(tan(xy.x), cos(dd));
  return (lp);
}
FREEUP;
  if (P) {
    if (P->en)
      free(P->en);
    free(P);
  }
}
ENTRY1(cass, en)
  if (P->es) {
    if (!((P->en = proj_mdist_ini(P->es)))) E_ERROR_0;
    P->m0 = proj_mdist(P->phi0, sin(P->phi0), cos(P->phi0), P->en);
    P->inv = e_inverse;
    P->fwd = e_forward;
  } else {
    P->inv = s_inverse;
    P->fwd = s_forward;
  }
ENDENTRY(P)
/*
** Log: proj_cass.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
