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
#define PROJ_PARMS__
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(larr, "Larrivee") "\n\tMisc Sph, no inv.";
#define SIXTH .16666666666666666
FORWARD(s_forward); /* sphere */
  (void) P; /* avoid warning */
  xy.x = 0.5 * lp.lam * (1. + sqrt(cos(lp.phi)));
  xy.y = lp.phi / (cos(0.5 * lp.phi) * cos(SIXTH * lp.lam));
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(larr) P->fwd = s_forward; P->inv = 0; P->es = 0.; ENDENTRY(P)
/*
** Log: proj_larr.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
