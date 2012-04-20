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
PROJ_HEAD(mb_S3, "McBryde S3") "\n\tPCyl, Sph., No Inv.";
#define MAX_ITER 8
#define LOOP_TOL 1e-7
#define PHI_LIM 0.9747663872388330687118812941
#define M 0.5
#define N 1.785398163397448309615660845
#define CY 0.9165962744127515074839558704
#define CX 0.6110641829418343383226372469
#define YF 0.069065
FORWARD(s_forward); /* sphere */
  (void) P; /* avoid warning */
  if (fabs(lp.phi) <= PHI_LIM) {
    xy.x = lp.lam * cos(lp.phi);
    xy.y = lp.phi;
  } else {
    double k, V;
    int i, sphi = lp.phi < 0;

    k = N * sin(lp.phi);
    for (i = MAX_ITER; i ; --i) {
      lp.phi -= V = (M * lp.phi + sin(lp.phi) - k) /
        (M + cos(lp.phi));
      if (fabs(V) < LOOP_TOL)
        break;
    }
    if (!i)
      F_ERROR
    xy.x = CX * lp.lam * (M + cos(lp.phi));
    xy.y = CY * lp.phi;
    if (sphi)
      xy.y += YF;
    else
      xy.y -= YF;
  }
  return (xy);
}
FREEUP; if (P) free(P); }
/*
ENTRY0(mcb_s3)
*/
ENTRY0(mb_S3)
  P->es = 0;
  P->fwd = s_forward;
ENDENTRY(P)
/*
** Log: proj_mcb_s3.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
