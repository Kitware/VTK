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
#define R_PI 0.3183098861837906715377675267
#define PI_SQ 9.869604401089358618834490999
PROJ_HEAD(eq_moll, "Equidistant Mollweide") "\n\tPCyl, Sph., No inv.";
FORWARD(s_forward); /* spheroid */
  (void) P; /* avoid warning */

  if ((xy.x = PI_SQ - 4.0 * lp.phi * lp.phi) <= 0.)
    xy.x = 0;
  else
    xy.x = sqrt(xy.x);
  xy.x *= lp.lam * R_PI;
  xy.y = lp.phi;
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(eq_moll)
  P->es = 0.;
  P->fwd = s_forward;
ENDENTRY(P)
/*
** Log: proj_equi_moll.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
