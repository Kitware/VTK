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
  double  height; \
  double  sinph0; \
  double  cosph0; \
  double  p; \
  double  rp; \
  double  pn1; \
  double  pfact; \
  double  h; \
  double  cg; \
  double  sg; \
  double  sw; \
  double  cw; \
  int    mode; \
  int    tilt;
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(nsper, "Near-sided perspective") "\n\tAzi, Sph\n\th=";
PROJ_HEAD(tpers, "Tilted perspective") "\n\tAzi, Sph\n\ttilt= azi= h=";
# define EPS10 1.e-10
# define N_POLE  0
# define S_POLE 1
# define EQUIT  2
# define OBLIQ  3
FORWARD(s_forward); /* spheroid */
  double  coslam, cosphi, sinphi;

  sinphi = sin(lp.phi);
  cosphi = cos(lp.phi);
  coslam = cos(lp.lam);
  switch (P->mode) {
  case OBLIQ:
    xy.y = P->sinph0 * sinphi + P->cosph0 * cosphi * coslam;
    break;
  case EQUIT:
    xy.y = cosphi * coslam;
    break;
  case S_POLE:
    xy.y = - sinphi;
    break;
  case N_POLE:
    xy.y = sinphi;
    break;
  }
  if (xy.y < P->rp) F_ERROR;
  xy.y = P->pn1 / (P->p - xy.y);
  xy.x = xy.y * cosphi * sin(lp.lam);
  switch (P->mode) {
  case OBLIQ:
    xy.y *= (P->cosph0 * sinphi -
       P->sinph0 * cosphi * coslam);
    break;
  case EQUIT:
    xy.y *= sinphi;
    break;
  case N_POLE:
    coslam = - coslam;
  case S_POLE:
    xy.y *= cosphi * coslam;
    break;
  }
  if (P->tilt) {
    double yt, ba;

    yt = xy.y * P->cg + xy.x * P->sg;
    ba = 1. / (yt * P->sw * P->h + P->cw);
    xy.x = (xy.x * P->cg - xy.y * P->sg) * P->cw * ba;
    xy.y = yt * ba;
  }
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  double  rh, cosz, sinz;

  if (P->tilt) {
    double bm, bq, yt;

    yt = 1./(P->pn1 - xy.y * P->sw);
    bm = P->pn1 * xy.x * yt;
    bq = P->pn1 * xy.y * P->cw * yt;
    xy.x = bm * P->cg + bq * P->sg;
    xy.y = bq * P->cg - bm * P->sg;
  }
  rh = hypot(xy.x, xy.y);
  if ((sinz = 1. - rh * rh * P->pfact) < 0.) I_ERROR;
  sinz = (P->p - sqrt(sinz)) / (P->pn1 / rh + rh / P->pn1);
  cosz = sqrt(1. - sinz * sinz);
  if (fabs(rh) <= EPS10) {
    lp.lam = 0.;
    lp.phi = P->phi0;
  } else {
    switch (P->mode) {
    case OBLIQ:
      lp.phi = asin(cosz * P->sinph0 + xy.y * sinz * P->cosph0 / rh);
      xy.y = (cosz - P->sinph0 * sin(lp.phi)) * rh;
      xy.x *= sinz * P->cosph0;
      break;
    case EQUIT:
      lp.phi = asin(xy.y * sinz / rh);
      xy.y = cosz * rh;
      xy.x *= sinz;
      break;
    case N_POLE:
      lp.phi = asin(cosz);
      xy.y = -xy.y;
      break;
    case S_POLE:
      lp.phi = - asin(cosz);
      break;
    }
    lp.lam = atan2(xy.x, xy.y);
  }
  return (lp);
}
FREEUP; if (P) free(P); }
  static PROJ *
setup(PROJ *P) {
  if ((P->height = proj_param(P->params, "dh").f) <= 0.) E_ERROR(-30);
  if (fabs(fabs(P->phi0) - HALFPI) < EPS10)
    P->mode = P->phi0 < 0. ? S_POLE : N_POLE;
  else if (fabs(P->phi0) < EPS10)
    P->mode = EQUIT;
  else {
    P->mode = OBLIQ;
    P->sinph0 = sin(P->phi0);
    P->cosph0 = cos(P->phi0);
  }
  P->pn1 = P->height / P->a; /* normalize by radius */
  P->p = 1. + P->pn1;
  P->rp = 1. / P->p;
  P->h = 1. / P->pn1;
  P->pfact = (P->p + 1.) * P->h;
  P->inv = s_inverse;
  P->fwd = s_forward;
  P->es = 0.;
  return P;
}
ENTRY0(nsper)
  P->tilt = 0;
ENDENTRY(setup(P))
ENTRY0(tpers)
  double omega, gammaval;

  omega = proj_param(P->params, "dtilt").f * DEG_TO_RAD;
  gammaval = proj_param(P->params, "dazi").f * DEG_TO_RAD;
  P->tilt = 1;
  P->cg = cos(gammaval); P->sg = sin(gammaval);
  P->cw = cos(omega); P->sw = sin(omega);
ENDENTRY(setup(P))
/*
** Log: proj_nsper.c
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
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
