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
#define C 2.467401100272339654708622749
#define CN1 1.467401100272339654708622749
#define TOL 1e-9
#define PROJ_LIB__
# include  <lib_proj.h>
PROJ_HEAD(four1, "Fournier Globular I") "\n\tMisc., Sph., NoInv.";
FORWARD(s_forward); /* spheroid */
  double s, p, a;
  (void) P; /* avoid warning */

  if (fabs(lp.lam) < TOL || fabs(fabs(lp.phi) - HALFPI) < TOL) {
      /* lon == 0 || |phi| == pi/2 */
    xy.x = 0;
    xy.y = lp.phi;
  } else if (fabs(lp.phi) < TOL) { /* lat == 0 */
    xy.x = lp.lam;
    xy.y = 0.;
  } else {
    s = sin(lp.phi);
    if (fabs(fabs(lp.lam)-HALFPI) < TOL) {
      xy.x = lp.lam * cos(lp.phi);
      xy.y = HALFPI * s;
    } else {
      p = fabs(PI * s);
      s = (C - lp.phi * lp.phi)/(p - 2.*fabs(lp.phi));
      a = lp.lam * lp.lam / C - 1.;
      xy.y = fabs((sqrt(s*s-a*(C-p*s-lp.lam*lp.lam)) - s)/a);
      if (lp.phi < 0)
        xy.y = -xy.y;
      xy.x = lp.lam * sqrt(1. - xy.y * xy.y / C);
    }
  }
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(four1) P->es = 0.; P->fwd = s_forward; ENDENTRY(P)
/*
** Log: proj_four1.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
