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
#define MCX  0.9003163161571060695551991909
#define MCY  1.414213562373095048801688724
#define MCP  3.141592653589793238462643383
#define WCX  0.8630951398862576896248308873
#define WCY  1.565481415999337518303982239
#define WCP  2.960420506177634139072152092
#define BCY  1.273239544735162686151070106
#define WEREN3 1.15862
#define PROJ_PARMS__ \
  double  C_x, C_y, C_p;
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(moll, "Mollweide") "\n\tPCyl., Sph.";
PROJ_HEAD(bromley, "Bromley") "\n\tPCyl., Sph.";
PROJ_HEAD(wag4, "Wagner IV") "\n\tPCyl., Sph.";
PROJ_HEAD(weren3, "Werenskiold III") "\n\tPCyl., Sph.";
#define MAX_ITER  10
#define LOOP_TOL  1e-7
FORWARD(s_forward); /* spheroid */
  double k, V;
  int i;

  k = P->C_p * sin(lp.phi);
  for (i = MAX_ITER; i ; --i) {
    lp.phi -= V = (lp.phi + sin(lp.phi) - k) /
      (1. + cos(lp.phi));
    if (fabs(V) < LOOP_TOL)
      break;
  }
  if (!i)
    lp.phi = (lp.phi < 0.) ? -HALFPI : HALFPI;
  else
    lp.phi *= 0.5;
  xy.x = P->C_x * lp.lam * cos(lp.phi);
  xy.y = P->C_y * sin(lp.phi);
  return (xy);
}
INVERSE(s_inverse); /* spheroid */

  lp.phi = proj_asin(xy.y / P->C_y);
  lp.lam = xy.x / (P->C_x * cos(lp.phi));
  lp.phi += lp.phi;
  lp.phi = proj_asin((lp.phi + sin(lp.phi)) / P->C_p);
  return (lp);
}
FREEUP; if (P) free(P); }
  static PROJ *
setup(PROJ *P) {

  P->es = 0;
  P->inv = s_inverse;
  P->fwd = s_forward;
  return P;
}
ENTRY0(moll)
  P->C_p = MCP;
  P->C_x = MCX;
  P->C_y = MCY;
  ENDENTRY(setup(P))
ENTRY0(bromley)
  P->C_p = MCP;
  P->C_x = 1.;
  P->C_y = BCY;
  ENDENTRY(setup(P))
ENTRY0(wag4)
  P->C_p = WCP;
  P->C_x = WCX;
  P->C_y = WCY;
  ENDENTRY(setup(P))
ENTRY0(weren3)
  P->C_p = WCP;
  P->C_x = WEREN3 * WCX;
  P->C_y = WEREN3 * WCY;
  ENDENTRY(setup(P))
/*
** Log: proj_moll.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
