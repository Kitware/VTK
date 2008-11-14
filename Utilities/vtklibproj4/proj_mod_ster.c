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
/* based upon Snyder and Linck, USGS-NMD */
#define PROJ_PARMS__ \
    PROJ_COMPLEX  *zcoeff; \
  double  cchio, schio; \
  int    n;
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(mil_os, "Miller Oblated Stereographic") "\n\tAzi(mod)";
PROJ_HEAD(lee_os, "Lee Oblated Stereographic") "\n\tAzi(mod)";
PROJ_HEAD(gs48, "Mod. Stererographics of 48 U.S.") "\n\tAzi(mod)";
PROJ_HEAD(alsk, "Mod. Stererographics of Alaska") "\n\tAzi(mod)";
PROJ_HEAD(gs50, "Mod. Stererographics of 50 U.S.") "\n\tAzi(mod)";
#define EPSLN 1e-10

FORWARD(e_forward); /* ellipsoid */
  double sinlon, coslon, esphi, chi, schi, cchi, s;
  PROJ_COMPLEX p;

  sinlon = sin(lp.lam);
  coslon = cos(lp.lam);
  esphi = P->e * sin(lp.phi);
  chi = 2. * atan(tan((HALFPI + lp.phi) * .5) *
    pow((1. - esphi) / (1. + esphi), P->e * .5)) - HALFPI;
  schi = sin(chi);
  cchi = cos(chi);
  s = 2. / (1. + P->schio * schi + P->cchio * cchi * coslon);
  p.r = s * cchi * sinlon;
  p.i = s * (P->cchio * schi - P->schio * cchi * coslon);
  p = proj_zpoly1(p, P->zcoeff, P->n);
  xy.x = p.r;
  xy.y = p.i;
  return xy;
}
INVERSE(e_inverse); /* ellipsoid */
  int nn;
  PROJ_COMPLEX p, fxy, fpxy, dp;
  double den, rh = 0., z, sinz = 0., cosz = 0., chi, phi = 0., dphi, esphi;

  p.r = xy.x;
  p.i = xy.y;
  for (nn = 20; nn ;--nn) {
    fxy = proj_zpolyd1(p, P->zcoeff, P->n, &fpxy);
    fxy.r -= xy.x;
    fxy.i -= xy.y;
    den = fpxy.r * fpxy.r + fpxy.i * fpxy.i;
    dp.r = -(fxy.r * fpxy.r + fxy.i * fpxy.i) / den;
    dp.i = -(fxy.i * fpxy.r - fxy.r * fpxy.i) / den;
    p.r += dp.r;
    p.i += dp.i;
    if ((fabs(dp.r) + fabs(dp.i)) <= EPSLN)
      break;
  }
  if (nn) {
    rh = hypot(p.r, p.i);
    z = 2. * atan(.5 * rh);
    sinz = sin(z);
    cosz = cos(z);
    lp.lam = P->lam0;
    if (fabs(rh) <= EPSLN) {
      lp.phi = P->phi0;
      return lp;
    }
    chi = proj_asin(cosz * P->schio + p.i * sinz * P->cchio / rh);
    phi = chi;
    for (nn = 20; nn ;--nn) {
      esphi = P->e * sin(phi);
      dphi = 2. * atan(tan((HALFPI + chi) * .5) *
        pow((1. + esphi) / (1. - esphi), P->e * .5)) - HALFPI - phi;
      phi += dphi;
      if (fabs(dphi) <= EPSLN)
        break;
    }
  }
  if (nn) {
    lp.phi = phi;
    lp.lam = atan2(p.r * sinz, rh * P->cchio * cosz - p.i * 
      P->schio * sinz);
    } else
    lp.lam = lp.phi = HUGE_VAL;
  return lp;
}
FREEUP; if (P) free(P); }
  static PROJ *
setup(PROJ *P) { /* general initialization */
  double esphi, chio;

  if (P->es) {
    esphi = P->e * sin(P->phi0);
    chio = 2. * atan(tan((HALFPI + P->phi0) * .5) *
      pow((1. - esphi) / (1. + esphi), P->e * .5)) - HALFPI;
  } else
    chio = P->phi0;
  P->schio = sin(chio);
  P->cchio = cos(chio);
  P->inv = e_inverse; P->fwd = e_forward;
  return P;
}
ENTRY0(mil_os)
  static PROJ_COMPLEX /* Miller Oblated Stereographic */
AB[] = {
  {0.924500,  0.},
  {0.,    0.},
  {0.019430,  0.}
};

  P->n = 2;
  P->lam0 = DEG_TO_RAD * 20.;
  P->phi0 = DEG_TO_RAD * 18.;
  P->zcoeff = AB;
  P->es = 0.;
ENDENTRY(setup(P))
ENTRY0(lee_os)
  static PROJ_COMPLEX /* Lee Oblated Stereographic */
AB[] = {
  {0.721316,  0.},
  {0.,    0.},
  {-0.0088162,   -0.00617325}
};

  P->n = 2;
  P->lam0 = DEG_TO_RAD * -165.;
  P->phi0 = DEG_TO_RAD * -10.;
  P->zcoeff = AB;
  P->es = 0.;
ENDENTRY(setup(P))
ENTRY0(gs48)
  static PROJ_COMPLEX /* 48 United States */
AB[] = {
  {0.98879,  0.},
  {0,      0.},
  {-0.050909,  0.},
  {0.,    0.},
  {0.075528,  0.}
};

  P->n = 4;
  P->lam0 = DEG_TO_RAD * -96.;
  P->phi0 = DEG_TO_RAD * -39.;
  P->zcoeff = AB;
  P->es = 0.;
  P->a = 6370997.;
ENDENTRY(setup(P))
ENTRY0(alsk)
  static PROJ_COMPLEX
ABe[] = { /* Alaska ellipsoid */
  {.9945303,  0.},
  {.0052083,  -.0027404},
  {.0072721,  .0048181},
  {-.0151089,  -.1932526},
  {.0642675,  -.1381226},
  {.3582802,  -.2884586}},
ABs[] = { /* Alaska sphere */
  {.9972523,  0.},
  {.0052513,  -.0041175},
  {.0074606,  .0048125},
  {-.0153783,  -.1968253},
  {.0636871,  -.1408027},
  {.3660976,  -.2937382}
};

  P->n = 5;
  P->lam0 = DEG_TO_RAD * -152.;
  P->phi0 = DEG_TO_RAD * 64.;
  if (P->es) { /* fixed ellipsoid/sphere */
    P->zcoeff = ABe;
    P->a = 6378206.4;
    P->e = sqrt(P->es = 0.00676866);
  } else {
    P->zcoeff = ABs;
    P->a = 6370997.;
  }
ENDENTRY(setup(P))
ENTRY0(gs50)
  static PROJ_COMPLEX
ABe[] = { /* GS50 ellipsoid */
  {.9827497,  0.},
  {.0210669,  .0053804},
  {-.1031415,  -.0571664},
  {-.0323337,  -.0322847},
  {.0502303,  .1211983},
  {.0251805,  .0895678},
  {-.0012315,  -.1416121},
  {.0072202,  -.1317091},
  {-.0194029,  .0759677},
  {-.0210072,  .0834037}
},
ABs[] = { /* GS50 sphere */
  {.9842990,  0.},
  {.0211642,  .0037608},
  {-.1036018,  -.0575102},
  {-.0329095,  -.0320119},
  {.0499471,  .1223335},
  {.0260460,  .0899805},
  {.0007388,  -.1435792},
  {.0075848,  -.1334108},
  {-.0216473,  .0776645},
  {-.0225161,  .0853673}
};

  P->n = 9;
  P->lam0 = DEG_TO_RAD * -120.;
  P->phi0 = DEG_TO_RAD * 45.;
  if (P->es) { /* fixed ellipsoid/sphere */
    P->zcoeff = ABe;
    P->a = 6378206.4;
    P->e = sqrt(P->es = 0.00676866);
  } else {
    P->zcoeff = ABs;
    P->a = 6370997.;
  }
ENDENTRY(setup(P))
/*
** Log: proj_mod_ster.c
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
