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
#include  <lib_proj.h>
PROJ_HEAD(ardn_cls, "Arden-Close") "\n\tCyl, Sph, NI";
#define EPS10 1.e-10
FORWARD(s_forward); /* spheroid */
  (void) P; /* avoid warning */
  if (fabs(fabs(lp.phi) - HALFPI) <= EPS10) F_ERROR;
  xy.x = lp.lam;
  xy.y = 0.5 * (log(tan(FORTPI + 0.5 * lp.phi)) + sin(lp.phi));
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(ardn_cls) P->es = 0.; P->inv = 0; P->fwd = s_forward; ENDENTRY(P)
/*
** Log: proj_ardn_cls.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
