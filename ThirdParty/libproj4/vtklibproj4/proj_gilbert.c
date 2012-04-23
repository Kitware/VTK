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
#define DEFAULT_LAT 5.*DEG_TO_RAD
#define PROJ_LIB__
#define PROJ_PARMS__ \
  double cp1, sp1;
# include  <lib_proj.h>
PROJ_HEAD(gilbert, "Gilbert Two World Perspective") "\n\tPCyl., Sph., NoInv.\n\tlat_1=";
  static double
phiprime(double phi) { return (proj_asin(tan(0.5 * phi))); }
FORWARD(s_forward); /* spheroid */
  double sp, cp, cl;

  lp.phi = phiprime(lp.phi);
  sp = sin(lp.phi);
  cp = cos(lp.phi);
  cl = cos(lp.lam *= 0.5);
  if ((P->sp1*sp + P->cp1*cp*cl) >= 0.) {
    xy.x = cp * sin(lp.lam);
    xy.y = P->cp1*sp - P->sp1 * cp * cl;
  } else
    F_ERROR;
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(gilbert)
  P->cp1 = phiprime((proj_param(P->params, "tlat_1").i) ?
    proj_param(P->params, "rlat_1").f : DEFAULT_LAT );
  P->sp1 = sin(P->cp1);
  P->cp1 = cos(P->cp1);
  P->es = 0.;
  P->fwd = s_forward;
ENDENTRY(P)
/*
** Log: proj_gilbert.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
