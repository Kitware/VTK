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
# include  <lib_proj.h>
#define MAX_ITER 20
#define PI_DIV3 1.047197551196597746154214461
#define LOOP_TOL 1e-7
PROJ_HEAD(erdi_krusz, "Erdi-Krausz") "\n\tPCyl, Sph., No Inv.";
FORWARD(s_forward); /* spheroid */
  double theta;
  (void) P; /* avoid warning */

  if (fabs(lp.phi) < PI_DIV3) {
    theta = asin(0.8 * sin(lp.phi));
    xy.x = 0.96042 * lp.lam * cos(theta);
    xy.y = 1.30152 * theta;
  } else {
    double V, t, k;
    int i;

    k = PI * sin(lp.phi);
    theta = lp.phi;
    for (i = MAX_ITER; i ; --i) {
      t = theta + theta;
      theta -= V = (t + sin(t) - k)/(2.0 + 2.0 * cos(t));
      if (fabs(V) < 1e-7)
        break;
    }
    if ( !i )
      theta = (lp.phi < 0.) ? -HALFPI : HALFPI;

    xy.x = 1.07023 * lp.lam * cos(theta);
    xy.y = 1.68111 * sin(theta);
    if (lp.phi < 0.)
      xy.y += 0.28549;
    else
      xy.y -= 0.28549;
  }
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(erdi_krusz)
  P->es = 0.;
  P->fwd = s_forward;
ENDENTRY(P)
/*
** Log: proj_erdi_ksz.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
