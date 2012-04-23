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
PROJ_HEAD(nicol, "Nicolosi Globular") "\n\tMisc Sph, no inv.";
#define EPS  1e-10
FORWARD(s_forward); /* spheroid */
  (void) P; /* avoid warning */
  if (fabs(lp.lam) < EPS) {
    xy.x = 0;
    xy.y = lp.phi;
  } else if (fabs(lp.phi) < EPS) {
    xy.x = lp.lam;
    xy.y = 0.;
  } else if (fabs(fabs(lp.lam) - HALFPI) < EPS) {
    xy.x = lp.lam * cos(lp.phi);
    xy.y = HALFPI * sin(lp.phi);
  } else if (fabs(fabs(lp.phi) - HALFPI) < EPS) {
    xy.x = 0;
    xy.y = lp.phi;
  } else {
    double tb, c, d, m, n, r2, sp;

    tb = HALFPI / lp.lam - lp.lam / HALFPI;
    c = lp.phi / HALFPI;
    d = (1 - c * c)/((sp = sin(lp.phi)) - c);
    r2 = tb / d;
    r2 *= r2;
    m = (tb * sp / d - 0.5 * tb)/(1. + r2);
    n = (sp / r2 + 0.5 * d)/(1. + 1./r2);
    xy.x = cos(lp.phi);
    xy.x = sqrt(m * m + xy.x * xy.x / (1. + r2));
    xy.x = HALFPI * ( m + (lp.lam < 0. ? -xy.x : xy.x));
    xy.y = sqrt(n * n - (sp * sp / r2 + d * sp - 1.) /
      (1. + 1./r2));
    xy.y = HALFPI * ( n + (lp.phi < 0. ? xy.y : -xy.y ));
  }
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(nicol) P->es = 0.; P->fwd = s_forward; ENDENTRY(P)
/*
** Log: proj_nocol.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
