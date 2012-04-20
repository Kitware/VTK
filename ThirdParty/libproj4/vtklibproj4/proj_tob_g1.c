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
#define N_TOL 1e-6
#define TOL 1e-7
#define PROJ_PARMS__ \
    double n, np; \
  int mode;
# include  <lib_proj.h>
PROJ_HEAD(tob_g1, "Tobler G1") "\n\tPCyl., Sph., [n=] NoInv.";
FORWARD(s_forward); /* spheroid */
  double cosphi, sinphi, aphi;

  if ((aphi = fabs(lp.phi)) < TOL) {
    xy.y = 0.;
    xy.x = lp.lam;
  } else {
    cosphi = cos(aphi);
    sinphi = sin(aphi);
    if (P->mode) {
      xy.y = sqrt(aphi * sinphi);
      xy.x = 2.0 * lp.lam * cosphi *  xy.y/(sinphi + aphi * cosphi);
    } else {
      xy.y = pow(aphi, P->n) * pow(fabs(sinphi),P->np);
      xy.x = lp.lam * cosphi *
        pow(fabs(aphi), P->np) * pow(fabs(sinphi),P->n) /
        (P->n * sinphi + P->np * aphi * cosphi);
    }
    if (lp.phi < 0.)
      xy.y = -xy.y;
  }
  return (xy);
}
FREEUP; if (P) free(P); }
ENTRY0(tob_g1)
  P->es = 0.;
  if (proj_param(P->params, "tn").i) {
    P->n = proj_param(P->params, "dn").f;
    if ((P->n < N_TOL) || (P->n > 1.-N_TOL))
      E_ERROR(-40)
  } else
         P->n = 0.5;
  P->np = 1.0 - P->n;
  P->mode = fabs(P->n - 0.5) < N_TOL;
  P->fwd = s_forward;
ENDENTRY(P)
/*
** Log: proj_tob_g1.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
