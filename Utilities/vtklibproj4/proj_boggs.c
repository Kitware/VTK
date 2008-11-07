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
PROJ_HEAD(boggs, "Boggs Eumorphic") "\n\tPCyl., no inv., Sph.";
# define NITER  20
# define EPS  1e-7
# define ONETOL 1.000001
# define FXC  2.00276
# define FXC2  1.11072
# define FYC  0.49931
# define FYC2  1.41421356237309504880
FORWARD(s_forward); /* spheroid */
  double theta, th1, c;
  int i;
  (void) P; /* avoid warning */

  theta = lp.phi;
  if (fabs(fabs(lp.phi) - HALFPI) < EPS)
    xy.x = 0.;
  else {
    c = sin(theta) * PI;
    for (i = NITER; i; --i) {
      theta -= th1 = (theta + sin(theta) - c) /
        (1. + cos(theta));
      if (fabs(th1) < EPS) break;
    }
    theta *= 0.5;
    xy.x = FXC * lp.lam / (1. / cos(lp.phi) + FXC2 / cos(theta));
  }
  xy.y = FYC * (lp.phi + FYC2 * sin(theta));
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(boggs) P->es = 0.; P->fwd = s_forward; ENDENTRY(P)
/*
** Log: proj_boggs.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
