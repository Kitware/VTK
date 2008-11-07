/*
** libproj -- library of cartographic projections
**
** Copyright (c) 2005, 2006   Gerald I. Evenden
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
#define ASIN_HALF 0.5235987755982988730771072305
#define TOL 1e-10
#define MAX_ITER 8
#define LOOP_TOL 1e-7
#define PROJ_LIB__
#define PROJ_PARMS__ \
  double K, beta; \
  double rho0; \
  double K1, F1, F2, F3, F4, A; \
  double AK, AK2;
# include  <lib_proj.h>
PROJ_HEAD(hill, "Hill Eucyclic") "\n\tPCyl., Sph., NoInv.\n\tK= or beta=";
FORWARD(s_forward); /* spheroid */
  double rho, beta1, omega;

  if (fabs(fabs(lp.phi) - HALFPI) < TOL) {
    beta1 = 0.;
    rho = lp.phi < 0. ? P->AK2 : P->AK;
  } else {
    double theta = lp.phi;
    double V, c5, st, ct, t;
    int i;

    c5 = 0.5 * (1. - sin(lp.phi)) * P->F1;
    theta = HALFPI - lp.phi;
    for (i = MAX_ITER; i ; --i) {
      st = sin(theta);
      ct = cos(theta);
      t = P->beta + atan(st/(P->K1 - ct));
      theta -= V = ( theta - P->F2 - P->K1 * st +
        (P->F3 - P->F4 * ct) * t - c5 ) /
        (P->F4 * st * t);
      if (fabs(V) < LOOP_TOL)
        break;
    }
    ct = cos(theta);
    rho = P->A * sqrt(P->F3 - P->F4 * ct);
    beta1 = atan(sin(theta)/(P->K1 - ct));
  }
  omega = lp.lam * (beta1 + P->beta) / PI;
  xy.x = rho * sin(omega);
  xy.y = P->rho0 - rho * cos(omega);
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(hill)
  if (proj_param(P->params, "tK").i) {
    P->K = proj_param(P->params, "dK").f;
    P->beta = asin(1./(P->K + 1));
  } else if (proj_param(P->params, "tbeta").i) {
    P->beta = proj_param(P->params, "rbeta").f;
    P->K = 1./sin(P->beta) - 1.;
  } else {
    P->K = 1.;
    P->beta = ASIN_HALF;
  }
  P->K1 = 1. + P->K;
  P->F1 = PI + 4. * P->beta * P->K1;
  P->A = 2. * sqrt(PI/P->F1);
  P->F2 = P->K * P->K * P->beta;
  P->F3 = 1. + P->K1 * P->K1;
  P->F4 = 2. * P->K1;
  P->AK = P->A * P->K;
  P->AK2 = P->A * (P->K + 2.);
  P->rho0 = 0.5 * P->A * (P->K1 + sqrt(P->K * (2. + P->K)));
  P->es = 0.;
  P->fwd = s_forward;
ENDENTRY(P)
/*
** Log: proj_hill.c
** Revision 3.2  2006/12/03 01:04:22  gie
** changed name from Hall to Hill
**
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
