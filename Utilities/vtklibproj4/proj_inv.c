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
** Inverse projection entry
*/
#define PROJ_LIB__
#include <lib_proj.h>
#include <errno.h>
# define EPS 1.0e-12
  PROJ_LP /* inverse projection entry */
proj_inv(PROJ_XY xy, PROJ *P) {
  PROJ_LP lp;

  /* can't do as much preliminary checking as with forward */
  if (xy.x == HUGE_VAL || xy.y == HUGE_VAL) {
    lp.lam = lp.phi = HUGE_VAL;
    proj_errno = -15;
  }
  errno = proj_errno = 0;
  xy.x = (xy.x * P->to_meter - P->x0) * P->ra; /* descale and de-offset */
  xy.y = (xy.y * P->to_meter - P->y0) * P->ra;
  lp = (*P->inv)(xy, P); /* inverse project */
  if (proj_errno || (proj_errno = errno))
    lp.lam = lp.phi = HUGE_VAL;
  else {
    lp.lam += P->lam0; /* reduce from del lp.lam */
    if (!P->over)
      lp.lam = proj_adjlon(lp.lam); /* adjust longitude to CM */
    if (P->geoc && fabs(fabs(lp.phi)-HALFPI) > EPS)
      lp.phi = atan(P->one_es * tan(lp.phi));
  }
  return lp;
}
/*
** Log: proj_inv.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
