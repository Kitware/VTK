/*
** libproj -- library of cartographic projections
**
** Copyright (c) 2006   Gerald I. Evenden
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
#include  <lib_proj.h>
#define MAX_ITER 11
#define EPS 1e-14

  double /* isometric latitude */
proj_psi(double phi, double sphi, double e) {
  double esp = e * sphi;

  return log(tan(FORTPI + 0.5 * phi) * pow((1. - esp)/(1. + esp), 0.5 * e));
}
  double /* inverse isometric latitude */
proj_apsi(double psi, double e) {
  double esp, phi, phi0, he = e * 0.5, exp_psi = exp(psi);
  int i = MAX_ITER;

  phi = 0.;
  phi0 = 2. * atan(exp_psi) - HALFPI;
  while (--i) {
    esp = e * sin(phi0);
    phi = 2. * atan(pow((1. + esp)/(1. - esp), he) * exp_psi) - HALFPI;
    phi0 = phi;
  } while ((fabs(phi0 - phi) > EPS) && --i);
  // if (!i) runaway
  return phi;
}
/*
** Log: proj_psi.c
** Revision 1.2  2008-11-14 16:56:33  jeff
** COMP: Fixing more libproj warnings.
**
** Revision 1.1  2008-11-07 16:41:15  jeff
** ENH: Adding a 2D geoview. Adding the geographic projection library libproj4
** to Utilities. Updating the architecture of the geospatial views. All
** multi-resolution sources are now subclasses of vtkGeoSource. Each source
** has its own worker thread for fetching refined images or geometry.
** On the 3D side, vtkGeoGlobeSource is an appropriate source for vtkGeoTerrain,
** and vtkGeoAlignedImageSource is an appropriate source for
** vtkGeoAlignedImageRepresentation. On the 2D side, vtkGeoProjectionSource is an
** appropriate source for vtkGeoTerrain2D, and the image source is the same.
**
** Revision 3.1  2006/06/19 01:00:26  gie
** Initial
**
*/
