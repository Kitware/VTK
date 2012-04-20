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
  double  rok; \
  double  rtk; \
  double  sinphi; \
  double  cosphi; \
  double  singam; \
  double  cosgam;
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(ocea, "Oblique Cylindrical Equal Area") "\n\tCyl, Sph"
  "\n\tlonc= alpha= or\n\tlat_1= lat_2= lon_1= lon_2=";
FORWARD(s_forward); /* spheroid */
  double t;

  xy.y = sin(lp.lam);
/*
  xy.x = atan2((tan(lp.phi) * P->cosphi + P->sinphi * xy.y) , cos(lp.lam));
*/
  t = cos(lp.lam);
  xy.x = atan((tan(lp.phi) * P->cosphi + P->sinphi * xy.y) / t);
  if (t < 0.)
    xy.x += PI;
  xy.x *= P->rtk;
  xy.y = P->rok * (P->sinphi * sin(lp.phi) - P->cosphi * cos(lp.phi) * xy.y);
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  double t, s;

  xy.y /= P->rok;
  xy.x /= P->rtk;
  t = sqrt(1. - xy.y * xy.y);
  lp.phi = asin(xy.y * P->sinphi + t * P->cosphi * (s = sin(xy.x)));
  lp.lam = atan2(t * P->sinphi * s - xy.y * P->cosphi,
    t * cos(xy.x));
  return (lp);
}
FREEUP; if (P) free(P); }
ENTRY0(ocea)
  double phi_0 = 0., phi_1, phi_2, lam_1, lam_2, lonz, alpha;

  P->rok = 1. / P->k0;
  P->rtk = 1. * P->k0;
  if ( proj_param(P->params, "talpha").i) {
    alpha  = proj_param(P->params, "ralpha").f;
    lonz = proj_param(P->params, "rlonc").f;
    P->singam = atan(-cos(alpha)/(-sin(phi_0) * sin(alpha))) + lonz;
    P->sinphi = asin(cos(phi_0) * sin(alpha));
  } else {
    phi_1 = proj_param(P->params, "rlat_1").f;
    phi_2 = proj_param(P->params, "rlat_2").f;
    lam_1 = proj_param(P->params, "rlon_1").f;
    lam_2 = proj_param(P->params, "rlon_2").f;
    P->singam = atan2(cos(phi_1) * sin(phi_2) * cos(lam_1) -
      sin(phi_1) * cos(phi_2) * cos(lam_2),
      sin(phi_1) * cos(phi_2) * sin(lam_2) -
      cos(phi_1) * sin(phi_2) * sin(lam_1) );
    P->sinphi = atan(-cos(P->singam - lam_1) / tan(phi_1));
  }
  P->lam0 = P->singam + HALFPI;
  P->cosphi = cos(P->sinphi);
  P->sinphi = sin(P->sinphi);
  P->cosgam = cos(P->singam);
  P->singam = sin(P->singam);
  P->inv = s_inverse;
  P->fwd = s_forward;
  P->es = 0.;
ENDENTRY(P)
/*
** Log: proj_ocea.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
