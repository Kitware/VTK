/*
** libproj -- library of cartographic projections
**
** Copyright (c) 2008   Gerald I. Evenden
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
/* The code in this file is largly based upon procedures:
 *
 * Written by: Knud Poder and Karsten Engsager
 *
 * Base upon math from: R.Koenig and K.H. Weise, "Mathematische
 * Grundlagen der hoeheren Geodaesie und Kartographie,
 * Springer-Verlag, Berlin/Goettingen" Heidelberg, 1951.
 *
 * Modified and used here by permission of Reference Networks
 * Division, Kort og Matrikelstyrelsen (KMS), Copenhagen, Denmark
*/
#define PROJ_PARMS__ \
  double    Qn;    /* Merid. quad., scaled to the projection */ \
  double    Zb;    /* Radius vector in polar coord. systems  */ \
  double    cgb[5]; /* Constants for Gauss -> Geo lat */ \
  double    cbg[5]; /* Constants for Geo lat -> Gauss */ \
  double    utg[5]; /* Constants for transv. merc. -> geo */ \
  double    gtu[5]; /* Constants for geo -> transv. merc. */
#define PROJ_LIB__
# include   <lib_proj.h>
#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif /* M_PI */
#ifndef M_PI_4
#  define M_PI_4 ( M_PI / 4. )
#endif /* M_PI_4 */
PROJ_HEAD(etmerc, "Extended Transverse Mercator")
    "\n\tCyl, Ellips\n\tlat_ts=(0)\nlat_0=(0)";
  static double
gatg(double *p1, int len_p1, double B) {
  double *p;
  double h = 0., h1, h2 = 0., cos_2B;

  cos_2B = 2.*cos(2.0*B);
     for (p = p1 + len_p1, h1 = *--p; p - p1; h2 = h1, h1 = h)
       h = -h2 + cos_2B*h1 + *--p;
     return (B + h*sin(2.0*B));
}
  static double
clenS(double *a, int size, double arg_r, double arg_i, double *R, double *I) {
  double      *p, r, i, hr, hr1, hr2, hi, hi1, hi2;
  double      sin_arg_r, cos_arg_r, sinh_arg_i, cosh_arg_i;

  /* arguments */
  p = a + size;
  sin_arg_r  = sin(arg_r);
  cos_arg_r  = cos(arg_r);
  sinh_arg_i = sinh(arg_i);
  cosh_arg_i = cosh(arg_i);
  r          =  2.0*cos_arg_r*cosh_arg_i;
  i          = -2.0*sin_arg_r*sinh_arg_i;
  /* summation loop */
  for (hi1 = hr1 = hi = 0.0, hr = *--p; a - p;) {
    hr2 = hr1;
    hi2 = hi1;
    hr1 = hr;
    hi1 = hi;
    hr  = -hr2 + r*hr1 - i*hi1 + *--p;
    hi  = -hi2 + i*hr1 + r*hi1;
  }
  r   = sin_arg_r*cosh_arg_i;
  i   = cos_arg_r*sinh_arg_i;
  *R  = r*hr - i*hi;
  *I  = r*hi + i*hr;
  return(*R);
}
  static double
clens(double *a, int size, double arg_r) {
  double      *p, r, hr, hr1, hr2, cos_arg_r;

  p = a + size;
  cos_arg_r  = cos(arg_r);
  r          =  2.0*cos_arg_r;
  /* summation loop */
  for (hr1 = 0.0, hr = *--p; a - p;) {
    hr2 = hr1;
    hr1 = hr;
    hr  = -hr2 + r*hr1 + *--p;
  }
  return(sin(arg_r)*hr);
}
FORWARD(e_forward); /* ellipsoid */
  double sin_Cn, cos_Cn, cos_Ce, dCn, dCe;
  double Cn = lp.phi, Ce = lp.lam;

  /* ell. LAT, LNG -> Gaussian LAT, LNG */
  Cn  = gatg(P->cbg, 5, Cn);
  /* Gaussian LAT, LNG -> compl. sph. LAT */
  sin_Cn = sin(Cn);
  cos_Cn = cos(Cn);
  cos_Ce = cos(Ce);
  Cn     = atan2(sin_Cn, cos_Ce*cos_Cn);
  Ce     = atan2(sin(Ce)*cos_Cn, hypot(sin_Cn, cos_Cn*cos_Ce));
  /* compl. sph. N, E -> ell. norm. N, E */
  Ce  = log(tan(M_PI_4 + Ce*0.5));
  Cn += clenS(P->gtu, 5, 2.*Cn, 2.*Ce, &dCn, &dCe);
  Ce += dCe;
  if (fabs(Ce) <= 2.623395162778) {
    xy.y  = P->Qn * Cn + P->Zb;  /* Northing */
    xy.x  = P->Qn * Ce;  /* Easting  */
  } else
    xy.x = xy.y = HUGE_VAL;
  return (xy);
}
INVERSE(e_inverse); /* ellipsoid */
  double sin_Ce, cos_Ce, cos_Cn, dCn, dCe;
  double Cn = xy.y, Ce = xy.x;

  /* normalize N, E */
  Cn = (Cn - P->Zb)/P->Qn;
  Ce = Ce/P->Qn;
  if (fabs(Ce) <= 2.623395162778) { /* 150 degrees */
  /* norm. N, E -> compl. sph. LAT, LNG */
    Cn += clenS(P->utg, 5, 2.*Cn, 2.*Ce, &dCn, &dCe);
    Ce += dCe;
    Ce = 2.0*(atan(exp(Ce)) - M_PI_4);
    /* compl. sph. LAT -> Gaussian LAT, LNG */
    cos_Cn = cos(Cn);
    sin_Ce = sin(Ce);
    cos_Ce = cos(Ce);
    Ce     = atan2(sin_Ce, cos_Ce*cos_Cn);
    Cn     = atan2(sin(Cn)*cos_Ce, hypot(sin_Ce, cos_Ce*cos_Cn));
    /* Gaussian LAT, LNG -> ell. LAT, LNG */
    lp.phi = gatg(P->cgb,  5, Cn);
    lp.lam = Ce;
  }
  else
    lp.phi = lp.lam = HUGE_VAL;
  return (lp);
}
FREEUP; if (P) free(P); }
ENTRY0(etmerc)
  double f, n, np, Z;

  if (P->es <= 0.) E_ERROR(-34);
  f = 1. - sqrt(1. - P->es);
  /* third flattening */
  np = n = f/(2.0 - f);

  /* COEF. OF TRIG SERIES GEO <-> GAUSS */
  /* cgb := Gaussian -> Geodetic, KW p190 - 191 (61) - (62) */
  /* cbg := Geodetic -> Gaussian, KW p186 - 187 (51) - (52) */
  /* 5 degree : Engsager and Poder: ICC2007 */
  P->cgb[0] = n*( 2.0 + n*(-2.0/3.0  + n*(-2.0      + n*(116.0/45.0 + n*(26.0/45.0)))));
  P->cbg[0] = n*(-2.0 + n*( 2.0/3.0  + n*( 4.0/3.0  + n*(-82.0/45.0 + n*(32.0/45.0)))));
  np     *= n;
  P->cgb[1] = np*(7.0/3.0     + n*( -8.0/5.0  + n*(-227.0/45.0 + n*(2704.0/315.0))));
  P->cbg[1] = np*(5.0/3.0     + n*(-16.0/15.0 + n*( -13.0/ 9.0 + n*( 904.0/315.0))));
  np     *= n;
  P->cgb[2] = np*( 56.0/15.0  + n*(-136.0/35.0 + n*(1262.0/105.0)));
  P->cbg[2] = np*(-26.0/15.0  + n*(  34.0/21.0 + n*(   8.0/  5.0)));
  np     *= n;
  P->cgb[3] = np*(4279.0/630.0 + n*(-322.0/35.0));
  P->cbg[3] = np*(1237.0/630.0 + n*( -12.0/ 5.0));
  np     *= n;
  P->cgb[4] = np*(4174.0/315.0);
  P->cbg[4] = np*(-734.0/315.0);

  /* Constants of the projections */
  /* Transverse Mercator (UTM, ITM, etc) */
  np = n*n;
  /* Norm. mer. quad, K&W p.50 (96), p.19 (38b), p.5 (2) */
  P->Qn = P->k0/(1 + n) * (1. + np*(1./4.0 + np*(1./64.0 + np/256.0)));
  /* coef of trig series */
  /* utg := ell. N, E -> sph. N, E,  KW p194 (65) */
  /* gtu := sph. N, E -> ell. N, E,  KW p196 (69) */
  P->utg[0] = n*(-0.5  + n*( 2.0/3.0 + n*(-37.0/96.0 + n*( 1.0/360.0 + n*(  81.0/512.0)))));
  P->gtu[0] = n*( 0.5  + n*(-2.0/3.0 + n*(  5.0/16.0 + n*(41.0/180.0 + n*(-127.0/288.0)))));
  P->utg[1] = np*(-1.0/48.0 + n*(-1.0/15.0 + n*(437.0/1440.0 + n*(-46.0/105.0))));
  P->gtu[1] = np*(13.0/48.0 + n*(-3.0/5.0  + n*(557.0/1440.0 + n*(281.0/630.0))));
  np      *= n;
  P->utg[2] = np*(-17.0/480.0 + n*(  37.0/840.0 + n*(  209.0/ 4480.0)));
  P->gtu[2] = np*( 61.0/240.0 + n*(-103.0/140.0 + n*(15061.0/26880.0)));
  np      *= n;
  P->utg[3] = np*(-4397.0/161280.0 + n*(  11.0/504.0));
  P->gtu[3] = np*(49561.0/161280.0 + n*(-179.0/168.0));
  np     *= n;
  P->utg[4] = np*(-4583.0/161280.0);
  P->gtu[4] = np*(34729.0/ 80640.0);
  /* Gaussian latitude value of the origin latitude */
  Z = gatg(P->cbg, 5, P->phi0);
  /* Origin northing minus true northing at the origin latitude */
  /* i.e. true northing = N - P->Zb                         */
  P->Zb = - P->Qn*(Z + clens(P->gtu, 5, 2.0*Z));
  P->inv = e_inverse;
  P->fwd = e_forward;
ENDENTRY(P)
