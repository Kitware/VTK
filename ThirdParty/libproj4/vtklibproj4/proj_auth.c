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
** Determine latitude from authalic latitude
*/
#include <lib_proj.h>
#define MAX_C 9
struct AUTHALIC {
  double C[MAX_C], CP[MAX_C], CQ[MAX_C];
};
#define A ((struct AUTHALIC *)a)
#define MAX_ITER 12
#define TOLER 1.e-12
  static double
betaf(double phi, const void *a) {
  double s, s2, *c, sum;
  int i = MAX_C;

  s = sin(phi);
  s2 = s * s;
  c = A->C + MAX_C;
  sum = *--c;
  while (--i) sum = *--c + s2 * sum;
  return(s * sum);
}
  static double
betap(double phi, const void *a) {
  double s2, *c, sum;
  int i = MAX_C;

  s2 = sin(phi);
  s2 = s2 * s2;
  c = A->CP + MAX_C;
  sum = *--c;
  while (--i)  sum = *--c + s2 * sum;
  return(cos(phi) * sum);
}
  double
proj_qsfn(double phi, const void *a) {
  double s, s2, *c, sum;
  int i = MAX_C;

  s = sin(phi);
  s2 = s * s;
  c = A->CQ + MAX_C;
  sum = *--c;
  while (--i) sum = *--c + s2 * sum;
  return(s * sum);
}
  void *
proj_auth_ini(double es, double *R) {
  double num, den, sum, t;
  int i;
  struct AUTHALIC *a;

  if ((a = (struct AUTHALIC *)malloc(sizeof(struct AUTHALIC))) == NULL)
    return(NULL);
  num = den = 1.;
  t = 1.;
  sum = 0.;
  for (i = 0; i < MAX_C; ++i) {
    sum += a->CQ[i] = a->C[i] = t * num/den;
    a->CP[i] = t * num++;
    t *= es;
    den += 2;
  }
  t = 2.* (1. - es);
  for (i = 0; i < MAX_C; ++i) {
    a->C[i] /= sum;
    a->CP[i] /= sum;
    a->CQ[i] *= t;
  }
  *R = sqrt((1.-es)* sum);
  return A;
}
  double
proj_auth_lat(double phi, const void *a) {
  return(asin(betaf(phi, a)));
}
  double
proj_auth_inv(double beta, const void *a) {
  int max;
  double dl, c, phi;

  c = sin(beta);
  phi = beta;
  for (max = MAX_ITER ; max ; --max) {
    dl = (c - betaf(phi, a))/betap(phi, a);
    phi += dl;
    if (fabs(dl) < TOLER)
      return(phi);
  }
  /* it will usually not fall out of the loop, in fact tests
   * have never made it fall through.  But there are convergence
   * limits at argument approaches 90 degrees */
  return(phi);
}
/*
** Log: proj_auth.c
** Revision 3.2  2006/01/19 03:39:29  gie
** removed unused declared variables
**
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
