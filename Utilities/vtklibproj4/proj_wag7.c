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
#define CTHIRD 0.3333333333333333333333333333
#define CM2_7 1.
#define CM1_8 0.9211662819778872735914619944
#define CM1_7 0.9063077870366499632425526567
#define CM2_8 0.8855017059025996450524064573
#define CX_7 2.667233451463325537046514593
#define CX_8 2.811481094659256323024402716
#define CY_7 1.241036383624926058458990485
#define CY_8 1.308153333346774615643262930
#define PROJ_PARMS__ \
  double Cx, Cy; \
  double m1, m2;
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(wag7, "Wagner VII") "\n\tMod. Azi., no inv.";
PROJ_HEAD(wag8, "Wagner VIII") "\n\tMod. Azi., no inv.";
FORWARD(s_forward); /* sphere */
  double S, C0, C1;

  S = P->m1 * sin(P->m2 * lp.phi);
  C0 = sqrt(1. - S * S);
  C1 = sqrt(2./(1 + C0 * cos(CTHIRD * lp.lam)));
  xy.x = P->Cx * C0 * C1 * sin(CTHIRD * lp.lam);
  xy.y = P->Cy * S * C1;
  return (xy);
}
FREEUP; if (P) free(P); }
  static PROJ*
setup(PROJ *P) { /* general initialization */
  P->fwd = s_forward;
  P->inv = 0;
  P->es = 0.;
  return P;
}
ENTRY0(wag7)
  P->Cx = CX_7;
  P->Cy = CY_7;
  P->m1 = CM1_7;
  P->m2 = CM2_7;
ENDENTRY(setup(P))
ENTRY0(wag8)
  P->Cx = CX_8;
  P->Cy = CY_8;
  P->m1 = CM1_8;
  P->m2 = CM2_8;
ENDENTRY(setup(P))
/*
** Log: proj_wag7.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
