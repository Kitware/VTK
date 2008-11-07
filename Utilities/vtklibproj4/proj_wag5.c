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
#define CPM1 3.008955224453420926376007179
#define CM2 0.8855017059025996450524064573
#define CX 0.9097725087960359780692854132
#define CY 1.650144798052019424282977532
#define LOOP_TOL 1e-7
#define MAX_ITER 10
# include  <lib_proj.h>
PROJ_HEAD(wag5, "Wagner V") "\n\tPCyl., Sph., NoInv.";
FORWARD(s_forward); /* spheroid */
  double V, k;
  int i;
  (void) P; /* avoid warning */

  k = CPM1 * sin(CM2 * lp.phi);
  lp.phi *= 1.33;
  for (i = MAX_ITER; i; --i) {
    lp.phi -= V = (lp.phi + sin(lp.phi) - k) /
      (1. + cos(lp.phi));
    if (fabs(V) < LOOP_TOL)
      break;
  }
  xy.x = CX * lp.lam * cos(lp.phi*=0.5);
  xy.y = CY * sin(lp.phi);
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(wag5) P->es = 0.; P->fwd = s_forward; ENDENTRY(P)
/*
** Log: proj_wag5.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
