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
  double  n; \
  double  rho_c; \
  double  rho_0; \
  double  sig; \
  double  c1, c2; \
  int    type;
#define PROJ_LIB__
#include  <lib_proj.h>
#define EULER 0
#define MURD1 1
#define MURD2 2
#define MURD3 3
#define PCONIC 4
#define TISSOT 5
#define VITK1 6
#define EPS10  1.e-10
#define EPS 1e-10
#define LINE2 "\n\tConic, Sph\n\tlat_1= and lat_2="
PROJ_HEAD(tissot, "Tissot")
  LINE2;
PROJ_HEAD(murd1, "Murdoch I")
  LINE2;
PROJ_HEAD(murd2, "Murdoch II")
  LINE2;
PROJ_HEAD(murd3, "Murdoch III")
  LINE2;
PROJ_HEAD(euler, "Euler")
  LINE2;
PROJ_HEAD(pconic, "Perspective Conic")
  LINE2;
PROJ_HEAD(vitk1, "Vitkovsky I")
  LINE2;
/* get common factors for simple conics */
  static int
phi12(PROJ *P, double *del) {
  double p1, p2;
  int err = 0;

  if (!proj_param(P->params, "tlat_1").i ||
    !proj_param(P->params, "tlat_2").i) {
    err = -41;
  } else {
    p1 = proj_param(P->params, "rlat_1").f;
    p2 = proj_param(P->params, "rlat_2").f;
    *del = 0.5 * (p2 - p1);
    P->sig = 0.5 * (p2 + p1);
    err = (fabs(*del) < EPS || fabs(P->sig) < EPS) ? -42 : 0;
    *del = *del;
  }
  return err;
}
FORWARD(s_forward); /* spheroid */
  double rho;

  switch (P->type) {
  case MURD2:
    rho = P->rho_c + tan(P->sig - lp.phi);
    break;
  case PCONIC:
    rho = P->c2 * (P->c1 - tan(lp.phi));
    break;
  default:
    rho = P->rho_c - lp.phi;
    break;
  }
  xy.x = rho * sin( lp.lam *= P->n );
  xy.y = P->rho_0 - rho * cos(lp.lam);
  return (xy);
}
INVERSE(s_inverse); /* ellipsoid & spheroid */
  double rho;

  rho = hypot(xy.x, xy.y = P->rho_0 - xy.y);
  if (P->n < 0.) {
    rho = - rho;
    xy.x = - xy.x;
    xy.y = - xy.y;
  }
  lp.lam = atan2(xy.x, xy.y) / P->n;
  switch (P->type) {
  case PCONIC:
    lp.phi = atan(P->c1 - rho / P->c2) + P->sig;
    break;
  case MURD2:
    lp.phi = P->sig - atan(rho - P->rho_c);
    break;
  default:
    lp.phi = P->rho_c - rho;
  }
  return (lp);
}
FREEUP; if (P) free(P); }
  static PROJ *
setup(PROJ *P) {
  double del, cs;
  int i;

  if ((i = phi12(P, &del)))
    E_ERROR(i);
  switch (P->type) {
  case TISSOT:
    P->n = sin(P->sig);
    cs = cos(del);
    P->rho_c = P->n / cs + cs / P->n;
    P->rho_0 = sqrt((P->rho_c - 2 * sin(P->phi0))/P->n);
    break;
  case MURD1:
    P->rho_c = sin(del)/(del * tan(P->sig)) + P->sig;
    P->rho_0 = P->rho_c - P->phi0;
    P->n = sin(P->sig);
    break;
  case MURD2:
    P->rho_c = (cs = sqrt(cos(del))) / tan(P->sig);
    P->rho_0 = P->rho_c + tan(P->sig - P->phi0);
    P->n = sin(P->sig) * cs;
    break;
  case MURD3:
    P->rho_c = del / (tan(P->sig) * tan(del)) + P->sig;
    P->rho_0 = P->rho_c - P->phi0;
    P->n = sin(P->sig) * sin(del) * tan(del) / (del * del);
    break;
  case EULER:
    P->n = sin(P->sig) * sin(del) / del;
    del *= 0.5;
    P->rho_c = del / (tan(del) * tan(P->sig)) + P->sig;  
    P->rho_0 = P->rho_c - P->phi0;
    break;
  case PCONIC:
    P->n = sin(P->sig);
    P->c2 = cos(del);
    P->c1 = 1./tan(P->sig);
    if (fabs(del = P->phi0 - P->sig) - EPS10 >= HALFPI)
      E_ERROR(-43);
    P->rho_0 = P->c2 * (P->c1 - tan(del));
    break;
  case VITK1:
    P->n = (cs = tan(del)) * sin(P->sig) / del;
    P->rho_c = del / (cs * tan(P->sig)) + P->sig;
    P->rho_0 = P->rho_c - P->phi0;
    break;
  }
  P->inv = s_inverse;
  P->fwd = s_forward;
  P->es = 0;
  return (P);
}
ENTRY0(euler) P->type = EULER; ENDENTRY(setup(P))
ENTRY0(tissot) P->type = TISSOT; ENDENTRY(setup(P))
ENTRY0(murd1) P->type = MURD1; ENDENTRY(setup(P))
ENTRY0(murd2) P->type = MURD2; ENDENTRY(setup(P))
ENTRY0(murd3) P->type = MURD3; ENDENTRY(setup(P))
ENTRY0(pconic) P->type = PCONIC; ENDENTRY(setup(P))
ENTRY0(vitk1) P->type = VITK1; ENDENTRY(setup(P))
/*
** Log: proj_sconics.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
