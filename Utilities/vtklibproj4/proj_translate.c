/*
** libproj -- library of cartographic projections
**
** Copyright (c) 2003, 2005, 2006   Gerald I. Evenden
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
** Procedure to translate axis on a sphere
*/
#define PROJ_LIB__
#include <lib_proj.h>

struct TRANS {
  double cosal, sinal, beta; \
  int mode;
};
#define TOL 1e-12
#define EN ((struct TRANS *)en)
  PROJ_LP
proj_translate(PROJ_LP in, const void *en) {
  PROJ_LP out;
  double cp, sp, cl;

  cp = cos(in.phi);
  cl = cos(in.lam);
  switch (EN->mode) {
  case 0:
    sp = sin(in.phi);
    out.phi = proj_asin(EN->sinal * sp - EN->cosal * cp * cl);
    out.lam = proj_adjlon(
      atan2(cp * sin(in.lam) , (EN->sinal * cp * cl + EN->cosal * sp))
      + EN->beta);
    break;
  case 1:
    out.phi = proj_asin(- cp * sin(in.lam));
    out.lam = atan2(-cl, tan(in.phi));
    break;
  case 2:
  default:
    out.phi = proj_asin(- cp * sin(in.lam));
    out.lam = atan2(tan(in.phi), cl);
    break;
  }  
  return (out);
}
  PROJ_LP
proj_inv_translate(PROJ_LP in, const void *en) {
  PROJ_LP out;
  double cp, sp, cl, sl;

  cp = cos(in.phi);
  sp = sin(in.phi);
  switch (EN->mode) {
  case 0:
    cl = cos(in.lam -= EN->beta);
    sl = sin(in.lam);
    out.phi = proj_asin(EN->sinal * sp + EN->cosal * cp * cl);
    out.lam = atan2(cp * sl, EN->sinal * cp * cl - EN->cosal * sp);
    break;
  case 1:
    cl = cos(in.lam);
    sl = sin(in.lam);
    out.phi = proj_asin(cp * cl);
    out.lam = atan2(sl * cp,  -sp) + HALFPI;
    break;
  case 2:
  default:
    cl = sin(in.lam);
    sl = cos(in.lam);
    out.phi = proj_asin(cp * cl);
    out.lam = atan2(sl * cp, sp) - HALFPI;
    break;
  }
  return (out);
}
  void *
proj_translate_ini(double alpha, double beta) {
  struct TRANS *en;

  if ((en = (struct TRANS *)malloc(sizeof(struct TRANS))) == NULL)
    return (NULL);
  EN->mode = 0;
  if ( fabs(alpha) < TOL ) {
    if ( (fabs(beta) < TOL) )
      EN->mode = 1;
    else if ( (fabs(beta - HALFPI) < TOL) )
      EN->mode = 2;
  }
  if (EN->mode == 0) {
    EN->sinal = sin(alpha);
    EN->cosal = cos(alpha);
    EN->beta = beta;
  }
  return ((void *)en);
}
/*
** Log: proj_translate.c
** Revision 3.4  2008/06/26 14:58:15  gie
** fudge to avoid warning---default's
**
** Revision 3.3  2006/09/25 02:04:13  gie
** corrected sign error
**
** Revision 3.2  2006/09/24 00:24:26  gie
** corrected some inverse forms
**
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
