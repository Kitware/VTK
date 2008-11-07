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
PROJ_HEAD(baranyi4, "Baranyi IV") "\n\tPCyl., Sph., NoInv.";
FORWARD(s_forward); /* spheroid */
  double aphi;
  (void) P; /* avoid warning */

  aphi = fabs(lp.phi);
  xy.y = lp.phi * (1. + aphi * aphi *(.112579 + aphi *
    (-.107505 + aphi * .0273759)));
  xy.x = log(1. + 0.11679 * fabs(lp.lam))/0.31255;
  if (aphi <= 1.36258)
    xy.x *= 1.22172 + sqrt(2.115292 - xy.y * xy.y);
  else {
    double t = 4.5848 + fabs(xy.y);
      xy.x *= sqrt(fabs(38.4304449 - t * t));
  }
  if (lp.lam < 0.)
    xy.x = -xy.x;
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(baranyi4) P->es = 0.; P->fwd = s_forward; ENDENTRY(P)
/*
** Log: proj_baranyi4.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
