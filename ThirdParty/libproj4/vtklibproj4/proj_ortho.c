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
  double  sinph0; \
  double  cosph0; \
  int    mode;
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(ortho, "Orthographic") "\n\tAzi, Sph.";
#define EPS10 1.e-10
#define N_POLE  0
#define S_POLE 1
#define EQUIT  2
#define OBLIQ  3
FORWARD(s_forward); /* spheroid */
  double  coslam, cosphi, sinphi;

  cosphi = cos(lp.phi);
  coslam = cos(lp.lam);
  switch (P->mode) {
  case EQUIT:
    if (cosphi * coslam < - EPS10 ||
      fabs(lp.lam) - HALFPI > EPS10) F_ERROR;
    xy.y = sin(lp.phi);
    break;
  case OBLIQ:
    if (P->sinph0 * (sinphi = sin(lp.phi)) +
       P->cosph0 * cosphi * coslam < - EPS10) F_ERROR;
    xy.y = P->cosph0 * sinphi - P->sinph0 * cosphi * coslam;
    break;
  case N_POLE:
    coslam = - coslam;
  case S_POLE:
    if (fabs(lp.phi - P->phi0) - EPS10 > HALFPI) F_ERROR;
    xy.y = cosphi * coslam;
    break;
  }
  xy.x = cosphi * sin(lp.lam);
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  double  rh, cosc, sinc;

  if ((sinc = (rh = hypot(xy.x, xy.y))) > 1.) {
    if ((sinc - 1.) > EPS10) I_ERROR;
    sinc = 1.;
  }
  cosc = sqrt(1. - sinc * sinc); /* in this range OK */
  if (fabs(rh) <= EPS10) {
    lp.phi = P->phi0;
    lp.lam = 0.;
  } else {
    switch (P->mode) {
    case N_POLE:
      xy.y = -xy.y;
      lp.phi = acos(sinc);
      break;
    case S_POLE:
      lp.phi = - acos(sinc);
      break;
    case EQUIT:
      lp.phi = xy.y * sinc / rh;
      xy.x *= sinc;
      xy.y = cosc * rh;
      goto sinchk;
    case OBLIQ:
      lp.phi = cosc * P->sinph0 + xy.y * sinc * P->cosph0 / rh;
      xy.y = (cosc - P->sinph0 * lp.phi) * rh;
      xy.x *= sinc * P->cosph0;
sinchk:
      if (fabs(lp.phi) >= 1.)
        lp.phi = lp.phi < 0. ? -HALFPI : HALFPI;
      else
        lp.phi = asin(lp.phi);
      break;
    }
    lp.lam = (xy.y == 0. && (P->mode == OBLIQ || P->mode == EQUIT)) ?
      (xy.x == 0. ? 0. : xy.x < 0. ? -HALFPI : HALFPI) :
      atan2(xy.x, xy.y);
  }
  return (lp);
}
FREEUP; if (P) free(P); }
ENTRY0(ortho)
  if (fabs(fabs(P->phi0) - HALFPI) <= EPS10)
    P->mode = P->phi0 < 0. ? S_POLE : N_POLE;
  else if (fabs(P->phi0) > EPS10) {
    P->mode = OBLIQ;
    P->sinph0 = sin(P->phi0);
    P->cosph0 = cos(P->phi0);
  } else
    P->mode = EQUIT;
  P->inv = s_inverse;
  P->fwd = s_forward;
  P->es = 0.;
ENDENTRY(P)
/*
** Log: proj_ortho.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
