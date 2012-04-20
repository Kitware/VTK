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
#define PROJ_LIB__
#include <lib_proj.h>

#define MAX_ITER 20

struct GAUSS {
  double C;
  double K;
  double e;
  double ratexp;
};
#define EN ((struct GAUSS *)en)
#define DEL_TOL 1e-14
  static double
srat(double esinp, double expval) {
  return(pow((1.-esinp)/(1.+esinp), expval));
}

  void *
proj_gauss_ini(double e, double phi0, double *chi, double *rc) {
  double sphi, cphi, es;
  struct GAUSS *en;

  if ((en = (struct GAUSS *)malloc(sizeof(struct GAUSS))) == NULL)
    return (NULL);
  es = e * e;
  EN->e = e;
  sphi = sin(phi0);
  cphi = cos(phi0);  cphi *= cphi;
  *rc = sqrt(1. - es) / (1. - es * sphi * sphi);
  EN->C = sqrt(1. + es * cphi * cphi / (1. - es));
  *chi = asin(sphi / EN->C);
  EN->ratexp = 0.5 * EN->C * e;
  EN->K = tan(.5 * *chi + FORTPI) / (
    pow(tan(.5 * phi0 + FORTPI), EN->C) *
    srat(EN->e * sphi, EN->ratexp)  );
  return ((void *)en);
}
  PROJ_LP
proj_gauss(PROJ_LP elp, const void *en) {
  PROJ_LP slp;

  slp.phi = 2. * atan( EN->K *
    pow(tan(.5 * elp.phi + FORTPI), EN->C) *
    srat(EN->e * sin(elp.phi), EN->ratexp) ) - HALFPI;
  slp.lam = EN->C * (elp.lam);
  return(slp);
}
  PROJ_LP
proj_inv_gauss(PROJ_LP slp, const void *en) {
  PROJ_LP elp;
  double num;
  int i;

  elp.lam = slp.lam / EN->C;
  num = pow(tan(.5 * slp.phi + FORTPI)/EN->K, 1./EN->C);
  for (i = MAX_ITER; i; --i) {
    elp.phi = 2. * atan(num * srat(EN->e * sin(slp.phi), -.5 * EN->e))
      - HALFPI;
    if (fabs(elp.phi - slp.phi) < DEL_TOL) break;
      slp.phi = elp.phi;
  }  
  /* convergence failed */
  if (!i)
    proj_errno = -17;
  return (elp);
}
/* Revision Log:
** Log: proj_gauss.c
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
