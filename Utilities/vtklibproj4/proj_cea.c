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
  double qp; \
  double *apa;
#define PROJ_LIB__
# include  <lib_proj.h>
PROJ_HEAD(cea, "Equal Area Cylindrical") "\n\tCyl, Sph&Ell\n\tlat_ts=(0)";
# define EPS  1e-10
FORWARD(e_forward); /* ellipsoid */
  xy.x = P->k0 * lp.lam;
  xy.y = .5 * proj_qsfn(lp.phi, P->apa) / P->k0;
  return (xy);
}
FORWARD(s_forward); /* spheroid */
  xy.x = P->k0 * lp.lam;
  xy.y = sin(lp.phi) / P->k0;
  return (xy);
}
INVERSE(e_inverse); /* ellipsoid */
  lp.phi = proj_auth_inv(asin( 2. * xy.y * P->k0 / P->qp), P->apa);
  lp.lam = xy.x / P->k0;
  return (lp);
}
INVERSE(s_inverse); /* spheroid */
  double t;

  if ((t = fabs(xy.y *= P->k0)) - EPS <= 1.) {
    if (t >= 1.)
      lp.phi = xy.y < 0. ? -HALFPI : HALFPI;
    else
      lp.phi = asin(xy.y);
    lp.lam = xy.x / P->k0;
  } else I_ERROR;
  return (lp);
}
FREEUP;
  if (P) {
    if (P->apa)
      free(P->apa);
    free(P);
  }
}
ENTRY1(cea, apa)
  double t;

  t = proj_param(P->params, "tlat_ts").i ?
    proj_param(P->params, "rlat_ts").f : P->phi0;
  if ((P->k0 = cos(t)) < 0.) E_ERROR(-24)
  if (P->es) {
    t = sin(t);
    P->k0 /= sqrt(1. - P->es * t * t);
    if (!(P->apa = proj_auth_ini(P->es, &t))) E_ERROR_0;
    P->qp = proj_qsfn(HALFPI, P->apa);
    P->inv = e_inverse;
    P->fwd = e_forward;
  } else {
    P->inv = s_inverse;
    P->fwd = s_forward;
  }
ENDENTRY(P)
/*
** Log: proj_cea.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
