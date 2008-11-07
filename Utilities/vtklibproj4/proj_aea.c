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
# define EPS10  1.e-10
# define TOL7  1.e-7
#define PROJ_PARMS__ \
  double  ec; \
  double  n; \
  double  c; \
  double  dd; \
  double  n2; \
  double  qp; \
  double  rho0; \
  double  rho; \
  double  phi1; \
  double  phi2; \
  void  *en; \
  void  *apa; \
  int    ellips;
#define PROJ_LIB__
# include  <lib_proj.h>
PROJ_HEAD(aea, "Albers Equal Area")
  "\n\tConic Sph&Ell\n\tlat_1= lat_2=";
PROJ_HEAD(leac, "Lambert Equal Area Conic")
  "\n\tConic, Sph&Ell\n\tlat_1= south";
/* determine latitude angle phi-1 */
# define N_ITER 15
# define EPSILON 1.0e-7
# define TOL 1.0e-10
FORWARD(e_forward); /* ellipsoid & spheroid */
  if ((P->rho = P->c - (P->ellips ? P->n * proj_qsfn(lp.phi, P->apa) :
      P->n2 * sin(lp.phi))) < 0.) F_ERROR
  P->rho = P->dd * sqrt(P->rho);
  xy.x = P->rho * sin( lp.lam *= P->n );
  xy.y = P->rho0 - P->rho * cos(lp.lam);
  return (xy);
}
INVERSE(e_inverse) /* ellipsoid & spheroid */;
  if ((P->rho = hypot(xy.x, xy.y = P->rho0 - xy.y))) {
    if (P->n < 0.) {
      P->rho = -P->rho;
      xy.x = -xy.x;
      xy.y = -xy.y;
    }
    lp.phi =  P->rho / P->dd;
    if (P->ellips) {
      lp.phi = (P->c - lp.phi * lp.phi) / P->n;
      lp.phi = proj_auth_inv(asin(lp.phi/P->qp), P->apa);
    } else if (fabs(lp.phi = (P->c - lp.phi * lp.phi) / P->n2) <= 1.)
      lp.phi = asin(lp.phi);
    else
      lp.phi = lp.phi < 0. ? -HALFPI : HALFPI;
    lp.lam = atan2(xy.x, xy.y) / P->n;
  } else {
    lp.lam = 0.;
    lp.phi = P->n > 0. ? HALFPI : - HALFPI;
  }
  return (lp);
}
FREEUP;
  if (P) {
    if (P->en) free(P->en);
    if (P->apa) free(P->apa);
    free(P);
  }
}
  static PROJ *
setup(PROJ *P) {
  double cosphi, sinphi, t;
  int secant;

  if (fabs(P->phi1 + P->phi2) < EPS10) E_ERROR(-21);
  P->n = sinphi = sin(P->phi1);
  cosphi = cos(P->phi1);
  secant = fabs(P->phi1 - P->phi2) >= EPS10;
  if ((P->ellips = P->es > 0.)) {
    double ml1, m1;

    if (!((P->en = proj_mdist_ini(P->es)))) E_ERROR_0;
    if (!((P->apa = proj_auth_ini(P->es, &t)))) E_ERROR_0;
    m1 = proj_msfn(sinphi, cosphi, P->es);
    ml1 = proj_qsfn(P->phi1, P->apa);
    if (secant) { /* secant cone */
      double ml2, m2;

      sinphi = sin(P->phi2);
      cosphi = cos(P->phi2);
      m2 = proj_msfn(sinphi, cosphi, P->es);
      ml2 = proj_qsfn(P->phi2, P->apa);
      P->n = (m1 * m1 - m2 * m2) / (ml2 - ml1);
    }
    P->ec = 1. - .5 * P->one_es * log((1. - P->e) /
      (1. + P->e)) / P->e;
    P->c = m1 * m1 + P->n * ml1;
    P->dd = 1. / P->n;
    P->rho0 = P->dd * sqrt(P->c - P->n * proj_qsfn(P->phi0, P->apa));
    P->qp = proj_qsfn(HALFPI, P->apa);
  } else {
    if (secant) P->n = .5 * (P->n + sin(P->phi2));
    P->n2 = P->n + P->n;
    P->c = cosphi * cosphi + P->n2 * sinphi;
    P->dd = 1. / P->n;
    P->rho0 = P->dd * sqrt(P->c - P->n2 * sin(P->phi0));
  }
  P->inv = e_inverse; P->fwd = e_forward;
  return P;
}
ENTRY2(aea, en, apa)
  P->phi1 = proj_param(P->params, "rlat_1").f;
  P->phi2 = proj_param(P->params, "rlat_2").f;
ENDENTRY(setup(P))
ENTRY2(leac, en, apa)
  P->phi2 = proj_param(P->params, "rlat_1").f;
  P->phi1 = proj_param(P->params, "bsouth").i ? - HALFPI: HALFPI;
ENDENTRY(setup(P))
/*
** Log: proj_aea.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
