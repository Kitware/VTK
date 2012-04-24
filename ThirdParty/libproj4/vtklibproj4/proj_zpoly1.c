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
** Evaluate complex polynomial
*/
#include <lib_proj.h>
/* note: coefficients are always from C_1 to C_n
**  i.e. C_0 == (0., 0)
**  n should always be >= 1 though no checks are made
*/
  PROJ_COMPLEX
proj_zpoly1(PROJ_COMPLEX z, PROJ_COMPLEX *C, int n) {
  PROJ_COMPLEX a;
  double t;

  a = *(C += n);
  while (n-- > 0) {
    a.r = (--C)->r + z.r * (t = a.r) - z.i * a.i;
    a.i = C->i + z.r * a.i + z.i * t;
  }
  a.r = z.r * (t = a.r) - z.i * a.i;
  a.i = z.r * a.i + z.i * t;
  return a;
}
/* evaluate complex polynomial and derivative */
  PROJ_COMPLEX
proj_zpolyd1(PROJ_COMPLEX z, PROJ_COMPLEX *C, int n, PROJ_COMPLEX *der) {
  PROJ_COMPLEX a, b;
  double t;
  int first = 1;

  b = a = *(C += n);
  while (n-- > 0) {
    if (first)
      first = 0;
    else {
      b.r = a.r + z.r * (t = b.r) - z.i * b.i;
      b.i = a.i + z.r * b.i + z.i * t;
    }
    a.r = (--C)->r + z.r * (t = a.r) - z.i * a.i;
    a.i = C->i + z.r * a.i + z.i * t;
  }
  b.r = a.r + z.r * (t = b.r) - z.i * b.i;
  b.i = a.i + z.r * b.i + z.i * t;
  a.r = z.r * (t = a.r) - z.i * a.i;
  a.i = z.r * a.i + z.i * t;
  *der = b;
  return a;
}
/*
** Log: proj_zpoly1.c
** Revision 3.2  2008/06/26 15:08:38  gie
** alter initialization sequence
**
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
