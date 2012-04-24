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
#define PROJ_PARMS__ \
  double C_x, C_y, A, B;
#define PROJ_LIB__
#define CP1X 1.89490
#define CP1Y 0.94745
#define CP1A -0.5
#define CP1B 0.3039635509270133143316383896
#define CP1PX 0.94745
#define CP1PY 0.94745
#define CP1PA 0.0
#define CP1PB 0.3039635509270133143316383896
#define CW6X 1.0
#define CW6Y 1.0
#define CW6A 0.0
#define CW6B 0.3039635509270133143316383896
#define CE3X 0.4222382003157712014929445259
#define CE3Y 0.8444764006315424029858890519
#define CE3A 1.0
#define CE3B 0.4052847345693510857755178528
#define CK7X 0.8660254037844386467637231707
#define CK7Y 1.0
#define CK7A 0.0
#define CK7B 0.3039635509270133143316383896
#include  <lib_proj.h>
PROJ_HEAD(eck3, "Eckert III") "\n\tPCyl, Sph.";
PROJ_HEAD(putp1, "Putnins P1") "\n\tPCyl, Sph.";
PROJ_HEAD(putp1p, "Putnins P1'") "\n\tPCyl, Sph.";
PROJ_HEAD(wag6, "Wagner VI") "\n\tPCyl, Sph.";
PROJ_HEAD(kav7, "Kavraisky VII") "\n\tPCyl, Sph.";
FORWARD(s_forward); /* spheroid */
  xy.y = P->C_y * lp.phi;
  xy.x = P->C_x * lp.lam * (P->A + proj_sqrt(1. - P->B * lp.phi * lp.phi));
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  lp.phi = xy.y / P->C_y;
  lp.lam = xy.x / (P->C_x * (P->A + proj_sqrt(1. - P->B * lp.phi * lp.phi)));
  return (lp);
}
FREEUP; if (P) free(P); }
  static PROJ *
setup(PROJ *P) {
  P->es = 0.;
  P->inv = s_inverse;
  P->fwd = s_forward;
  return P;
}
ENTRY0(eck3)
  P->C_x = CE3X;
  P->C_y = CE3Y;
  P->A = CE3A;
  P->B = CE3B;
ENDENTRY(setup(P))
ENTRY0(kav7)
  P->C_x = CK7X;
  P->C_y = CK7Y;
  P->A = CK7A;
  P->B = CK7B;
ENDENTRY(setup(P))
ENTRY0(wag6);
  P->C_x = CW6X;
  P->C_y = CW6Y;
  P->A = CW6A;
  P->B = CW6B;
ENDENTRY(setup(P))
ENTRY0(putp1);
  P->C_x = CP1X;
  P->C_y = CP1Y;
  P->A = CP1A;
  P->B = CP1B;
ENDENTRY(setup(P))
ENTRY0(putp1p);
  P->C_x = CP1PX;
  P->C_y = CP1PY;
  P->A = CP1PA;
  P->B = CP1PB;
ENDENTRY(setup(P))
/*
** Log: proj_eck3.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
