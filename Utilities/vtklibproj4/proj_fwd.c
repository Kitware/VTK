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
/*
** Forward projection entry point
*/
#define PROJ_LIB__
#include <lib_proj.h>
#include <errno.h>
# define EPS 1.0e-12
  PROJ_XY /* forward projection entry */
proj_fwd(PROJ_LP lp, PROJ *P) {
  PROJ_XY xy;
  double t;

  /* check for forward and latitude or longitude overange */
  if ((t = fabs(lp.phi)-HALFPI) > EPS || fabs(lp.lam) > 10.) {
    xy.x = xy.y = HUGE_VAL;
    proj_errno = -14;
  } else { /* proceed with projection */
    errno = proj_errno = 0;
    if (fabs(t) <= EPS)
      lp.phi = lp.phi < 0. ? -HALFPI : HALFPI;
    else if (P->geoc)
      lp.phi = atan(P->rone_es * tan(lp.phi));
    lp.lam -= P->lam0;  /* compute del lp.lam */
    if (!P->over)
      lp.lam = proj_adjlon(lp.lam); /* adjust del longitude */
    xy = (*P->fwd)(lp, P); /* project */
    if (proj_errno || (proj_errno = errno))
      xy.x = xy.y = HUGE_VAL;
    /* adjust for major axis and easting/northings */
    else {
      xy.x = P->fr_meter * (P->a * xy.x + P->x0);
      xy.y = P->fr_meter * (P->a * xy.y + P->y0);
    }
  }
  return xy;
}
/* Revision log:
** Log: proj_fwd.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
