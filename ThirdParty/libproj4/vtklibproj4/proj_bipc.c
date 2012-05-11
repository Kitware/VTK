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
  int  noskew;
#define PROJ_LIB__
# include  <lib_proj.h>
PROJ_HEAD(bipc, "Bipolar conic of western hemisphere")
  "\n\tConic Sph.";
# define EPS  1e-10
# define EPS10  1e-10
# define NITER  10
# define lamB  -.34894976726250681539
# define n  .63055844881274687180
# define F  1.89724742567461030582
# define Azab  .81650043674686363166
# define Azba  1.82261843856185925133
# define T  1.27246578267089012270
# define rhoc  1.20709121521568721927
# define cAzc  .69691523038678375519
# define sAzc  .71715351331143607555
# define C45  .70710678118654752469
# define S45  .70710678118654752410
# define C20  .93969262078590838411
# define S20  -.34202014332566873287
# define R110  1.91986217719376253360
# define R104  1.81514242207410275904
FORWARD(s_forward); /* spheroid */
  double cphi, sphi, tphi, t, al, Az, z, Av, cdlam, sdlam, r;
  int tag;

  cphi = cos(lp.phi);
  sphi = sin(lp.phi);
  cdlam = cos(sdlam = lamB - lp.lam);
  sdlam = sin(sdlam);
  if (fabs(fabs(lp.phi) - HALFPI) < EPS10) {
    Az = lp.phi < 0. ? PI : 0.;
    tphi = HUGE_VAL;
  } else {
    tphi = sphi / cphi;
    Az = atan2(sdlam , C45 * (tphi - cdlam));
  }
  if ((tag = (Az > Azba))) {
    cdlam = cos(sdlam = lp.lam + R110);
    sdlam = sin(sdlam);
    z = proj_acos(S20 * sphi + C20 * cphi * cdlam);
    if (tphi != HUGE_VAL)
      Az = atan2(sdlam, (C20 * tphi - S20 * cdlam));
    Av = Azab;
    xy.y = rhoc;
  } else {
    z = proj_acos(S45 * (sphi + cphi * cdlam));
    Av = Azba;
    xy.y = -rhoc;
  }
  if (z < 0.) F_ERROR;
  r = F * (t = pow(tan(.5 * z), n));
  if ((al = .5 * (R104 - z)) < 0.) F_ERROR;
  al = proj_acos((t + pow(tan(al), n)) / T);
  if (fabs(t = n * (Av - Az)) < al)
    r /= cos(al + (tag ? t : -t));
  xy.x = r * sin(t);
  xy.y += (tag ? -r : r) * cos(t);
  if (P->noskew) {
    t = xy.x;
    xy.x = -xy.x * cAzc - xy.y * sAzc; 
    xy.y = -xy.y * cAzc + t * sAzc; 
  }
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  double t, r, rp, rl, al, z = 0., fAz, Az, s, c, Av;
  int neg, i;

  if (P->noskew) {
    t = xy.x;
    xy.x = -xy.x * cAzc + xy.y * sAzc; 
    xy.y = -xy.y * cAzc - t * sAzc; 
  }
  if ((neg = (xy.x < 0.))) {
    xy.y = rhoc - xy.y;
    s = S20;
    c = C20;
    Av = Azab;
  } else {
    xy.y += rhoc;
    s = S45;
    c = C45;
    Av = Azba;
  }
  rl = rp = r = hypot(xy.x, xy.y);
  fAz = fabs(Az = atan2(xy.x, xy.y));
  for (i = NITER; i ; --i) {
    z = 2. * atan(pow(r / F,1 / n));
    al = proj_acos((pow(tan(.5 * z), n) +
       pow(tan(.5 * (R104 - z)), n)) / T);
    if (fAz < al)
      r = rp * cos(al + (neg ? Az : -Az));
    if (fabs(rl - r) < EPS)
      break;
    rl = r;
  }
  if (! i) I_ERROR;
  Az = Av - Az / n;
  lp.phi = proj_asin(s * cos(z) + c * sin(z) * cos(Az));
  lp.lam = atan2(sin(Az), c / tan(z) - s * cos(Az));
  if (neg)
    lp.lam -= R110;
  else
    lp.lam = lamB - lp.lam;
  return (lp);
}
FREEUP; if (P) free(P); }
ENTRY0(bipc)
  P->noskew = proj_param(P->params, "bns").i;
  P->inv = s_inverse;
  P->fwd = s_forward;
  P->es = 0.;
ENDENTRY(P)
/*
** Log: proj_bipc.c
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
