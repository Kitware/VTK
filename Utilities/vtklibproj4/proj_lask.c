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
PROJ_HEAD(lask, "Laskowski") "\n\tMisc Sph, no inv.";
#define  a10   0.975534
#define  a12  -0.119161
#define  a32  -0.0143059
#define  a14  -0.0547009
#define  b01   1.00384
#define  b21   0.0802894
#define  b03   0.0998909
#define  b41   0.000199025
#define  b23  -0.0285500
#define  b05  -0.0491032
FORWARD(s_forward); /* sphere */
  double l2, p2;
  (void) P; /* avoid warning */

  l2 = lp.lam * lp.lam;
  p2 = lp.phi * lp.phi;
  xy.x = lp.lam * (a10 + p2 * (a12 + l2 * a32 + p2 * a14));
  xy.y = lp.phi * (b01 + l2 * (b21 + p2 * b23 + l2 * b41) +
    p2 * (b03 + p2 * b05));
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(lask) P->fwd = s_forward; P->inv = 0; P->es = 0.; ENDENTRY(P)
/*
** Log: proj_lask.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
