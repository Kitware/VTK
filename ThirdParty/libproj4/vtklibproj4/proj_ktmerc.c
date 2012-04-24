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
#define PROJ_PARMS__ \
  double  As, Bs, Cs, Ds; /* conformal lat constants */ \
  double  Ap, Bp, Cp, Dp; /* conformal lat constants */ \
  double  beta[4], del[4]; \
  double  K;
#define PROJ_LIB__
# include   <lib_proj.h>
PROJ_HEAD(ktmerc, "Kruger Transverse Mercator")
    "\n\tCyl, Ellipse\n";
FORWARD(e_forward); /* ellipsoid */
  double c, phis, sphi2, z, n, tz, tn, *b;
  int i;

  sphi2 = sin(lp.phi);
  c = cos(lp.phi) * sphi2;
  sphi2 *= sphi2;
  phis = lp.phi - c * (P->As + sphi2 * (P->Bs +
    sphi2 * (P->Cs + sphi2 * P->Ds)));
  xy.y = z = atan2(tan(phis), cos(lp.lam));
  xy.x = n = atanh(cos(phis) * sin(lp.lam));
  for (b = P->beta, i = 2; i <= 8 ; i += 2, ++b) {
    tz = i * z;
    tn = i * n;
    xy.y += *b * sin(tz) * cosh(tn);
    xy.x += *b * cos(tz) * sinh(tn);
  }
  xy.y *= P->K;
  xy.x *= P->K;
  return (xy);
}
INVERSE(e_inverse); /* ellipsoid */
  double *d, c, sinp2, z, n, tz, tn, sz, sn;
  int i;

  sz = z = xy.y / P->K;
  sn = n = xy.x / P->K;
  for (d = P->del, i = 2; i <= 8; i += 2, ++d) {
    tz = i * z;
    tn = i * n;
    sz -= *d * sin(tz) * cosh(tn);
    sn -= *d * cos(tz) * sinh(tn);
  }
  lp.lam = atan2(sinh(sn), cos(sz));
  lp.phi = asin(sin(sz) / cosh(sn));
  sinp2 = sin(lp.phi);
  c = cos(lp.phi) * sinp2;
  sinp2 *= sinp2;
  lp.phi += c * (P->Ap + sinp2 * (P->Bp + sinp2 * (P->Cp + sinp2 * P->Dp)));
  return (lp);
}
FREEUP; if (P) free(P); }
ENTRY0(ktmerc)
  double f, es4, es6, es8, n, n2;

  if (P->es <= 0.) E_ERROR(-34);
  f = 1. - sqrt(P->one_es);
  es8 = (es6 = (es4 = P->es * P->es) * P->es) * P->es;
  P->As = P->es;
  P->Bs = es4 * (5. - P->es) / 6.;
  P->Cs = (es6 * 104. - es8 * 45.) / 120.;
  P->Ds = es8 * 1237. / 1260.;
  P->Ap = P->es + es4 + es6 + es8;
  P->Bp = -(es4 * 7. + es6 * 17. + es8 * 30.) / 6.;
  P->Cp = (es6 * 224. + es8 * 889. ) / 120;
  P->Dp = 4279. * es8 / 1260.;
  n = f/(2.0 - f);
  n2 = n * n;
  P->K = P->k0 * (1.+ n2*(1./4. + n2 * 1./64.))/(1+n);
  P->beta[0] = n*(1./2. + n*(-2./3. +n*(5./16. +n*41./180.)));
  P->beta[1] = n2*(13./48.+n*(-3./5. + n*557./1440.));
  P->beta[2] = n2*n*(61./240. - n*103./140.);
  P->beta[3] = n2 * n2 * 49561./161280.;
  P->del[0] = n *(1./2.+n*(-2./3.+n*(37./96.-n*1./360.)));
  P->del[1] = n2*(1./48.+n*(1./15.-n*437./1440.));
  P->del[2] = n*n2*(17./480.-37./840.);
  P->del[3] = n2*n2*4397./161280.;
  P->inv = e_inverse;
  P->fwd = e_forward;
ENDENTRY(P)
