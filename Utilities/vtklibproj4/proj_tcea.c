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
#define TOL 1e-10
#define PROJ_PARMS__ \
  double M0, MP; \
  double qp; \
  void *en; \
  void *apa;
#define PROJ_LIB__
# include  <lib_proj.h>
PROJ_HEAD(tcea, "Transverse Cylindrical Equal-Area")
   "\n\tCyl, Sph&Ell\n\tk_0=(1)";
# define EPS  1e-10
FORWARD(e_forward); /* ellipsoid */
  double beta, betac, phic, sc, M;

  beta = proj_auth_lat(lp.phi, P->apa);
  if (fabs(fabs(lp.lam) - HALFPI) > TOL) {
    betac = atan2(tan(beta),cos(lp.lam));
    phic = proj_auth_inv(betac, P->apa);
    sc = sin(phic);
    xy.x = cos(beta)*cos(phic)*sin(lp.lam)/
      (P->k0*cos(betac)*sqrt(1.-P->es*sc*sc));
  } else {
    betac = phic = lp.phi >=0. ? HALFPI : -HALFPI;
    sc = lp.phi >= 0. ? 1. : -1.;
    xy.x = cos(beta)*sin(lp.lam)/ (P->k0*sqrt(1.-P->es));
  }
  M = proj_mdist(phic, sc, cos(phic), P->en);
  xy.y = P->k0 * (M - P->M0);
  return (xy);
}
FORWARD(s_forward); /* spheroid */
  xy.x = cos(lp.phi)*sin(lp.lam)/P->k0;
  xy.y = P->k0 * (atan2(tan(lp.phi), cos(lp.lam)) - P->phi0);
  return (xy);
}
INVERSE(e_inverse); /* ellipsoid */
  double phic, betac, betap, beta, sc, t;

  t = P->M0 + xy.y/P->k0;
  phic = proj_inv_mdist(t, P->en);
  sc = sin(phic);
  betac = proj_auth_lat(phic, P->apa);
  betap = -asin(P->k0 * xy.x * cos(betac) * sqrt(1.-P->es*sc*sc)/cos(phic));
  beta = asin(cos(betap) * sin(betac));
  lp.lam = - atan2(tan(betap),cos(betac));
  if (fabs(t) > P->MP)
    lp.lam += lp.lam < 0. ? PI : - PI;
  lp.phi = proj_auth_inv(beta, P->apa);
  return (lp);
}
INVERSE(s_inverse); /* spheroid */
  double t, D, r;

  t = xy.x * P->k0;
  r = sqrt(1. - t * t);
  D = xy.y / P->k0 + P->phi0;
  lp.phi = asin(r * sin(D));
  lp.lam = atan2(t,(r * cos(D)));
  return (lp);
}
FREEUP;
  if (P) {
    if (P->apa)
      free(P->apa);
    if (P->en)
      free(P->en);
    free(P);
  }
}
ENTRY2(tcea, apa, en)
  double t;

  if (P->es) {
    if (!(P->apa = proj_auth_ini(P->es, &t))) E_ERROR_0;
    if (!(P->en = proj_mdist_ini(P->es))) E_ERROR_0;
    P->qp = proj_qsfn(HALFPI, P->apa);
    P->M0 = proj_mdist(P->phi0, sin(P->phi0), cos(P->phi0), P->en);
    P->MP = proj_mdist(HALFPI, 1., 0., P->en);
    P->inv = e_inverse;
    P->fwd = e_forward;
  } else {
    P->inv = s_inverse;
    P->fwd = s_forward;
  }
ENDENTRY(P)
/*
** Log: proj_tcea.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
