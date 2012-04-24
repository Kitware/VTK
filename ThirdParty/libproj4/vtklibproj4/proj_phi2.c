/*
** libproj -- library of cartographic projections
**
** Copyright (c) 2003   Gerald I. Evenden
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
/* determine latitude angle phi-2 */
#include <lib_proj.h>

#define HALFPI    1.5707963267948966
#define TOL 1.0e-10
#define N_ITER 15

  double
proj_phi2(double ts, double e) {
  double eccnth, Phi, con, dphi;
  int i;

  eccnth = .5 * e;
  Phi = HALFPI - 2. * atan (ts);
  i = N_ITER;
  do {
    con = e * sin (Phi);
    dphi = HALFPI - 2. * atan (ts * pow((1. - con) /
       (1. + con), eccnth)) - Phi;
    Phi += dphi;
  } while ( fabs(dphi) > TOL && --i);
  if (i <= 0)
    proj_errno = -18;
  return Phi;
}
/*
** Log: proj_phi2.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
