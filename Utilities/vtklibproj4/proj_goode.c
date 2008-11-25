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
  double phi_join; \
  double y_corr; \
  struct PROJconsts  *equi; \
  struct PROJconsts  *polar;
#define PROJ_LIB__
#include  <lib_proj.h>
#if defined ( _MSC_VER )
#pragma warning ( disable : 4210 )
  // nonstandard extension used: function given file scope
#endif
PROJ_HEAD(goode, "Goode Homolosine") "\n\tPCyl, Sph.";
PROJ_HEAD(mb_Q3, "McBryde Q3") "\n\tPCyl, Sph.";
PROJ_HEAD(mb_S2, "McBryde S2") "\n\tPCyl, Sph.";
PROJ_HEAD(mb_P3, "McBryde P3") "\n\tPCyl, Sph.";
FORWARD(s_forward); /* spheroid */
  if (fabs(lp.phi) <= P->phi_join)
    xy = P->equi->fwd(lp, P->equi);
  else {
    xy = P->polar->fwd(lp, P->polar);
    xy.y -= lp.phi >= 0.0 ? P->y_corr : -P->y_corr;
  }
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  if (fabs(xy.y) <= P->phi_join)
    lp = P->equi->inv(xy, P->equi);
  else {
    xy.y += xy.y >= 0.0 ? P->y_corr : -P->y_corr;
    lp = P->polar->inv(xy, P->polar);
  }
  return (lp);
}
FREEUP;
  if (P) {
    if (P->equi)
      (*(P->equi->pfree))(P->equi);
    if (P->polar)
      (*(P->polar->pfree))(P->polar);
    free(P);
  }
}
  static PROJ *
setup(PROJ *P, int n) {
  extern PROJ
    *proj_sinu(PROJ *),
    *proj_qua_aut(PROJ *),
    *proj_mbtfpq(PROJ *),
    *proj_eck6(PROJ *),
    *proj_mbtfpp(PROJ *),
    *proj_crast(PROJ *),
    *proj_moll(PROJ *);
  PROJ *(*proj_equi[])(PROJ *) = {
    proj_sinu,
    proj_qua_aut,
    proj_sinu,
    proj_crast
  };
  PROJ *(*proj_polar[])(PROJ *) = {
    proj_moll,
    proj_mbtfpq,
    proj_eck6,
    proj_mbtfpp
  };
  double Y_corr[] = {
    0.05280,
    0.042686,
    0.084398,
    0.035509
  };
  double Phi_join[] = {
    0.71093078197902358062,
    0.9101892049150428743657047357,
    0.8598655448158730382310716519,
    0.861135
  };

  P->equi = P->polar = 0;
  P->es = 0.;
  if (!(P->equi = proj_equi[n](0)) || !(P->polar = proj_polar[n](0)))
    E_ERROR_0;
  if (!(P->equi = proj_equi[n](P->equi)) || !(P->polar = proj_polar[n](P->polar)))
    E_ERROR_0;
  P->y_corr = Y_corr[n];
  P->phi_join = Phi_join[n];
  P->fwd = s_forward;
  P->inv = s_inverse;
  return P;
}
ENTRY0(goode)
ENDENTRY(setup(P, 0))
ENTRY0(mb_Q3)
ENDENTRY(setup(P, 1))
ENTRY0(mb_S2)
ENDENTRY(setup(P, 2))
ENTRY0(mb_P3)
ENDENTRY(setup(P, 3))
/*
** Log: proj_goode.c
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
