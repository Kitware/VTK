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
** projection initialization and closure
*/
#define PROJ_LIB__
#include <lib_proj.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
  PROJ *
proj_init(int argc, char **argv) {
  const char *s, *name;
  PROJ *(*proj)(PROJ *);
  paralist *curr = 0, *start = 0;
  int i;
  PROJ *PIN = 0;

  errno = proj_errno = 0;
  /* put arguments into internal linked list */
  if (argc <= 0) { proj_errno = -1; goto bum_call; }
  for (i = 0; i < argc; ++i)
    if (i)
      curr = curr->next = proj_mkparam(argv[i]);
    else
      start = curr = proj_mkparam(argv[i]);
  if (proj_errno) goto bum_call;
  /* find projection selection */
  if (!(name = proj_param(start, "sproj").s))
    { proj_errno = -4; goto bum_call; }
  for (i = 0; (s = proj_list[i].id) && strcmp(name, s) ; ++i) ;
  if (!s) { proj_errno = -5; goto bum_call; }
  proj = proj_list[i].proj;
  /* allocate projection structure */
  if (!(PIN = (*proj)(0))) goto bum_call;
  PIN->params = start;
  /* set ellipsoid/sphere parameters */
  if (proj_ell_set(start, &PIN->a, &PIN->es)) goto bum_call;
  PIN->e = sqrt(PIN->es);
  PIN->ra = 1. / PIN->a;
  PIN->one_es = 1. - PIN->es;
  if (PIN->one_es == 0.) { proj_errno = -6; goto bum_call; }
  PIN->rone_es = 1./PIN->one_es;
  /* set PIN->geoc coordinate system */
  PIN->geoc = (PIN->es && proj_param(start, "bgeoc").i);
  /* over-ranging flag */
  PIN->over = proj_param(start, "bover").i;
  /* central meridian */
  PIN->lam0=proj_param(start, "rlon_0").f;
  /* central latitude */
  PIN->phi0 = proj_param(start, "rlat_0").f;
  /* false easting and northing */
  PIN->x0 = proj_param(start, "dx_0").f;
  PIN->y0 = proj_param(start, "dy_0").f;
  /* general scaling factor */
  if (proj_param(start, "tk_0").i)
    PIN->k0 = proj_param(start, "dk_0").f;
  else if (proj_param(start, "tk").i)
    PIN->k0 = proj_param(start, "dk").f;
  else
    PIN->k0 = 1.;
  if (PIN->k0 <= 0.) {
    proj_errno = -31;
    goto bum_call;
  }
  /* set units */
  s = 0;
  if ((name = proj_param(start, "sunits").s)) { 
    for (i = 0; (s = proj_units[i].id) && strcmp(name, s) ; ++i) ;
    if (!s) { proj_errno = -7; goto bum_call; }
    s = proj_units[i].to_meter;
  }
  if (s || (s = proj_param(start, "sto_meter").s)) {
    PIN->to_meter = strtod(s, (char **)&s);
    if (*s == '/') /* ratio number */
      PIN->to_meter /= strtod(++s, 0);
    PIN->fr_meter = 1. / PIN->to_meter;
  } else
    PIN->to_meter = PIN->fr_meter = 1.;
  /* projection specific initialization */
  if (!(PIN = (*proj)(PIN)) || errno || proj_errno) {
bum_call: /* cleanup error return */
    if (!proj_errno)
      proj_errno = errno;
    if (PIN)
      proj_free(PIN);
    else
      for ( ; start; start = curr) {
        curr = start->next;
        free(start);
      }
    PIN = 0;
  }
  return PIN;
}
  void
proj_free(PROJ *P) {
  if (P) {
    paralist *t = P->params, *n;

    /* free parameter list elements */
    for (t = P->params; t; t = n) {
      n = t->next;
      free(t);
    }
    /* free projection parameters */
    P->pfree(P);
  }
}
/* Revision log
** Log: proj_init.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
