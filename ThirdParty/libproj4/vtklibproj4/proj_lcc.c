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
  double  phi2; \
  double  n, belgiuma; \
  double  rho; \
  double  rho0; \
  double  c; \
  int    ellips, westo;
#define PROJ_LIB__
#include  <lib_proj.h>
/* Belgium correction factor 29.2985 in radians */
#define BELGIUMA 0.0001420431363598774030819409832
PROJ_HEAD(lcc, "Lambert Conformal Conic")
  "\n\tConic, Sph&Ell\n\tlat_1= and lat_2= or lat_0";
# define EPS10  1.e-10
FORWARD(e_forward); /* ellipsoid & spheroid */
  double theta;

  if (fabs(fabs(lp.phi) - HALFPI) < EPS10) {
    if ((lp.phi * P->n) <= 0.) F_ERROR;
    P->rho = 0.;
    }
  else
    P->rho = P->c * (P->ellips ? pow(proj_tsfn(lp.phi, sin(lp.phi),
      P->e), P->n) : pow(tan(FORTPI + .5 * lp.phi), -P->n));
  theta = P->n * lp.lam - P->belgiuma;
  xy.x = P->k0 * (P->rho * sin( theta ) );
  if (P->westo) xy.x = -xy.x;
  xy.y = P->k0 * (P->rho0 - P->rho * cos( theta ) );
  return (xy);
}
INVERSE(e_inverse); /* ellipsoid & spheroid */
  xy.x /= P->k0;
  if (P->westo) xy.x = -xy.x;
  xy.y /= P->k0;
  if ((P->rho = hypot(xy.x, xy.y = P->rho0 - xy.y))) {
    if (P->n < 0.) {
      P->rho = -P->rho;
      xy.x = -xy.x;
      xy.y = -xy.y;
    }
    if (P->ellips) {
      if ((lp.phi = proj_phi2(pow(P->rho / P->c, 1./P->n), P->e))
        == HUGE_VAL)
        I_ERROR;
    } else
      lp.phi = 2. * atan(pow(P->c / P->rho, 1./P->n)) - HALFPI;
    lp.lam = (atan2(xy.x, xy.y) + P->belgiuma) / P->n;
  } else {
    lp.lam = 0.;
    lp.phi = P->n > 0. ? HALFPI : - HALFPI;
  }
  return (lp);
}
SPECIAL(fact) {
  if (fabs(fabs(lp.phi) - HALFPI) < EPS10) {
    if ((lp.phi * P->n) <= 0.) return;
    P->rho = 0.;
  } else
    P->rho = P->c * (P->ellips ? pow(proj_tsfn(lp.phi, sin(lp.phi),
      P->e), P->n) : pow(tan(FORTPI + .5 * lp.phi), -P->n));
  fac->code |= IS_ANAL_HK + IS_ANAL_CONV;
  fac->k = fac->h = P->k0 * P->n * P->rho /
    proj_msfn(sin(lp.phi), cos(lp.phi), P->es);
  fac->conv = - P->n * lp.lam;
}
FREEUP; if (P) free(P); }
ENTRY0(lcc)
  double cosphi, sinphi;
  int secant;

  P->phi1 = proj_param(P->params, "rlat_1").f;
  if (proj_param(P->params, "tlat_2").i)
    P->phi2 = proj_param(P->params, "rlat_2").f;
  else {
    P->phi2 = P->phi1;
    if (!proj_param(P->params, "tlat_0").i)
      P->phi0 = P->phi1;
  }
  P->westo = proj_param(P->params, "twesto").i;
  P->belgiuma = proj_param(P->params, "tbelgium").i ? BELGIUMA : 0.;
  if (fabs(P->phi1 + P->phi2) < EPS10) E_ERROR(-21);
  P->n = sinphi = sin(P->phi1);
  cosphi = cos(P->phi1);
  secant = fabs(P->phi1 - P->phi2) >= EPS10;
  if ((P->ellips = (P->es != 0.))) {
    double ml1, m1;

    P->e = sqrt(P->es);
    m1 = proj_msfn(sinphi, cosphi, P->es);
    ml1 = proj_tsfn(P->phi1, sinphi, P->e);
    if (secant) { /* secant cone */
      P->n = log(m1 /
         proj_msfn(sinphi = sin(P->phi2), cos(P->phi2), P->es));
      P->n /= log(ml1 / proj_tsfn(P->phi2, sinphi, P->e));
    }
    P->c = (P->rho0 = m1 * pow(ml1, -P->n) / P->n);
    P->rho0 *= (fabs(fabs(P->phi0) - HALFPI) < EPS10) ? 0. :
      pow(proj_tsfn(P->phi0, sin(P->phi0), P->e), P->n);
  } else {
    if (secant)
      P->n = log(cosphi / cos(P->phi2)) /
         log(tan(FORTPI + .5 * P->phi2) /
         tan(FORTPI + .5 * P->phi1));
    P->c = cosphi * pow(tan(FORTPI + .5 * P->phi1), P->n) / P->n;
    P->rho0 = (fabs(fabs(P->phi0) - HALFPI) < EPS10) ? 0. :
      P->c * pow(tan(FORTPI + .5 * P->phi0), -P->n);
  }
  P->inv = e_inverse;
  P->fwd = e_forward;
  P->spc = fact;
ENDENTRY(P)
/*
** Log: proj_lcc.c
** Revision 1.1  2008-11-07 16:41:14  jeff
** ENH: Adding a 2D geoview. Adding the geographic projection library libproj4
** to Utilities. Updating the architecture of the geospatial views. All
** multi-resolution sources are now subclasses of vtkGeoSource. Each source
** has its own worker thread for fetching refined images or geometry.
** On the 3D side, vtkGeoGlobeSource is an appropriate source for vtkGeoTerrain,
** and vtkGeoAlignedImageSource is an appropriate source for
** vtkGeoAlignedImageRepresentation. On the 2D side, vtkGeoProjectionSource is an
** appropriate source for vtkGeoTerrain2D, and the image source is the same.
**
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
