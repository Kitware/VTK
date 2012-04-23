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
PROJ_HEAD(collg, "Collignon") "\n\tPCyl, Sph.";
#define FXC  1.12837916709551257390
#define FYC  1.77245385090551602729
#define ONEEPS  1.0000001
FORWARD(s_forward); /* spheroid */
  (void) P; /* avoid warning */
  if ((xy.y = 1. - sin(lp.phi)) <= 0.)
    xy.y = 0.;
  else
    xy.y = sqrt(xy.y);
  xy.x = FXC * lp.lam * xy.y;
  xy.y = FYC * (1. - xy.y);
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  (void) P; /* avoid warning */
  lp.phi = xy.y / FYC - 1.;
  if (fabs(lp.phi = 1. - lp.phi * lp.phi) < 1.)
    lp.phi = asin(lp.phi);
  else if (fabs(lp.phi) > ONEEPS) I_ERROR
  else  lp.phi = lp.phi < 0. ? -HALFPI : HALFPI;
  if ((lp.lam = 1. - sin(lp.phi)) <= 0.)
    lp.lam = 0.;
  else
    lp.lam = xy.x / (FXC * sqrt(lp.lam));
  return (lp);
}
FREEUP; if (P) free(P); }
ENTRY0(collg) P->es = 0.; P->inv = s_inverse; P->fwd = s_forward; ENDENTRY(P)
/*
** Log: proj_collg.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
