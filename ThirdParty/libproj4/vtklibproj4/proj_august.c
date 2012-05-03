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
#include  <lib_proj.h>
PROJ_HEAD(august, "August Epicycloidal") "\n\tMisc Sph, no inv.";
#define M 1.333333333333333
FORWARD(s_forward); /* spheroid */
  double t, c1, c, x1, x12, y1val, y12;
  (void) P; /* avoid warning */

  t = tan(.5 * lp.phi);
  c1 = sqrt(1. - t * t);
  c = 1. + c1 * cos(lp.lam *= .5);
  x1 = sin(lp.lam) *  c1 / c;
  y1val =  t / c;
  xy.x = M * x1 * (3. + (x12 = x1 * x1) - 3. * (y12 = y1val *  y1val));
  xy.y = M * y1val * (3. + 3. * x12 - y12);
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(august) P->inv = 0; P->fwd = s_forward; P->es = 0.; ENDENTRY(P)
/*
** Log: proj_august.c
** Revision 1.1  2008-11-07 16:41:13  jeff
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
