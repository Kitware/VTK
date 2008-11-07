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
# define TOL  1e-10
# define TWORPI  0.63661977236758134308
#define PROJ_PARMS__ \
  int  vdg3;
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(vandg2, "van der Grinten II") "\n\tMisc Sph, no inv.";
PROJ_HEAD(vandg3, "van der Grinten III") "\n\tMisc Sph, no inv.";
FORWARD(s_forward); /* spheroid */
  double x1, at, bt, ct;

  bt = fabs(TWORPI * lp.phi);
  if ((ct = 1. - bt * bt) < 0.)
    ct = 0.;
  else
    ct = sqrt(ct);
  if (fabs(lp.lam) < TOL) {
    xy.x = 0.;
    xy.y = PI * (lp.phi < 0. ? -bt : bt) / (1. + ct);
  } else {
    at = 0.5 * fabs(PI / lp.lam - lp.lam / PI);
    if (P->vdg3) {
      x1 = bt / (1. + ct);
      xy.x = PI * (sqrt(fabs(at * at + 1. - x1 * x1)) - at);
      xy.y = PI * x1;
    } else {
      x1 = (ct * sqrt(1. + at * at) - at * ct * ct) /
        (1. + at * at * bt * bt);
      xy.x = PI * x1;
      xy.y = PI * sqrt(1. - x1 * (x1 + 2. * at) + TOL);
    }
    if ( lp.lam < 0.) xy.x = -xy.x;
    if ( lp.phi < 0.) xy.y = -xy.y;
  }
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(vandg2)
  P->vdg3 = 0;
  P->es = 0.; P->fwd = s_forward;
ENDENTRY(P)
ENTRY0(vandg3)
  P->vdg3 = 1;
  P->es = 0.; P->fwd = s_forward;
ENDENTRY(P)
/*
** Log: proj_vandg2.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
