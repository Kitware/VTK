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
#define C2SQ2 2.828427124746190097603377448
PROJ_HEAD(baker, "Baker Dinomic") "\n\tPCyl., Sph. NoInv.";
FORWARD(s_forward); /* spheroid */
  double aphi = fabs(lp.phi);
  
  if (aphi < FORTPI) {
    xy.x = P->k0 * lp.lam;
    xy.y = P->k0 * log(tan(FORTPI + .5 * lp.phi));
  } else {
    xy.x = lp.lam * cos(aphi) * (C2SQ2 - 1./sin(aphi));
    xy.y = -log(tan(0.5*aphi)) + C2SQ2 * (aphi - FORTPI);
    if (lp.phi < 0.)
      xy.y = -xy.y;
  }
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(baker)
  P->es = 0.; P->fwd = s_forward;
ENDENTRY(P)
/*
** Log: proj_baker.c
** Revision 3.2  2006/01/19 03:37:50  gie
** removed 'cs' because unused
**
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
