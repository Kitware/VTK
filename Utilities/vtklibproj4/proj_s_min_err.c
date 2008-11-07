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
#define A_1 1.27326
#define A_3 -.04222
#define A_5 -.0293
#define AP_3 -0.12666
#define AP_5 -.1465
#define TOL 0.00001
PROJ_HEAD(s_min_err, "Snyder Minimum Error Pseudocylindrical") "\n\tPCyl, Sph., no inv.";
FORWARD(s_forward); /* spheroid */
  double p2 = lp.phi * lp.phi;
  (void) P; /* avoid warning */

  xy.x = lp.lam * cos(lp.phi) / (A_1 + p2 * (AP_3 + p2 * AP_5));
  xy.y = lp.phi*(A_1 + p2 * (A_3 + p2 * A_5));
  return(xy);
}
FREEUP; if (P) free(P); }
ENTRY0(s_min_err) P->es = 0.; P->inv = 0; P->fwd = s_forward; ENDENTRY(P)
/*
** Log: proj_s_min_err.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
