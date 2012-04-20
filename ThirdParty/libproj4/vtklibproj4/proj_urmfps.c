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
  double  n, C_x, C_y;
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(urmfps, "Urmaev Flat-Polar Sinusoidal") "\n\tPCyl, Sph.\n\tn=";
PROJ_HEAD(wag1, "Wagner I (Kavraisky VI)") "\n\tPCyl, Sph.";
PROJ_HEAD(weren2, "Werenskiold II") "\n\tPCyl, Sph.";
#define UCX  0.8773826753016616405461459345
#define UCY  1.139753528477388820996781626
#define WCX  0.8773826753016616405461459345
#define WCY  1.316074012952492460819218901
#define WCP  0.8660254037844386467637231707
#define WNM  1.139753528477388820996781625
FORWARD(s_forward); /* sphere */
  lp.phi = proj_asin(P->n * sin(lp.phi));
  xy.x = P->C_x * lp.lam * cos(lp.phi);
  xy.y = P->C_y * lp.phi;
  return (xy);
}
INVERSE(s_inverse); /* sphere */
  xy.y /= P->C_y;
  lp.phi = proj_asin(sin(xy.y) / P->n);
  lp.lam = xy.x / (P->C_x * cos(xy.y));
  return (lp);
}
FREEUP; if (P) free(P); }
  static PROJ *
setup(PROJ *P) {
  P->es = 0.;
  P->inv = s_inverse;
  P->fwd = s_forward;
  return P;
}
ENTRY0(urmfps)
  if (proj_param(P->params, "tn").i) {
    P->n = proj_param(P->params, "dn").f;
    if (P->n <= 0. || P->n > 1.)
      E_ERROR(-40)
  } else
    E_ERROR(-40)
  P->C_x = UCX;
  P->C_y = UCY / P->n;
ENDENTRY(setup(P))
ENTRY0(wag1)
  P->C_x = WCX;
  P->C_y = WCY;
  P->n = WCP;
ENDENTRY(setup(P))
ENTRY0(weren2)
  P->C_x = WNM * WCX;
  P->C_y = WNM * WCY;
  P->n = WCP;
ENDENTRY(setup(P))
/*
** Log: proj_urmfps.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
