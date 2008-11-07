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
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(vandg4, "van der Grinten IV") "\n\tMisc Sph, no inv.";
#define TOL  1e-10
#define TWORPI  0.63661977236758134308
FORWARD(s_forward); /* spheroid */
  double x1, t, bt, ct, ft, bt2, ct2, dt, dt2;
  (void) P; /* avoid warning */

  if (fabs(lp.phi) < TOL) {
    xy.x = lp.lam;
    xy.y = 0.;
  } else if (fabs(lp.lam) < TOL || fabs(fabs(lp.phi) - HALFPI) < TOL) {
    xy.x = 0.;
    xy.y = lp.phi;
  } else {
    bt = fabs(TWORPI * lp.phi);
    bt2 = bt * bt;
    ct = 0.5 * (bt * (8. - bt * (2. + bt2)) - 5.)
      / (bt2 * (bt - 1.));
    ct2 = ct * ct;
    dt = TWORPI * lp.lam;
    dt = dt + 1. / dt;
    dt = sqrt(dt * dt - 4.);
    if ((fabs(lp.lam) - HALFPI) < 0.) dt = -dt;
    dt2 = dt * dt;
    x1 = bt + ct; x1 *= x1;
    t = bt + 3.*ct;
    ft = x1 * (bt2 + ct2 * dt2 - 1.) + (1.-bt2) * (
      bt2 * (t * t + 4. * ct2) +
      ct2 * (12. * bt * ct + 4. * ct2) );
    x1 = (dt*(x1 + ct2 - 1.) + 2.*sqrt(ft)) /
      (4.* x1 + dt2);
    xy.x = HALFPI * x1;
    xy.y = HALFPI * sqrt(1. + dt * fabs(x1) - x1 * x1);
    if (lp.lam < 0.) xy.x = -xy.x;
    if (lp.phi < 0.) xy.y = -xy.y;
  }
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(vandg4) P->es = 0.; P->fwd = s_forward; ENDENTRY(P)
/*
** Log: proj_vandg4.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
