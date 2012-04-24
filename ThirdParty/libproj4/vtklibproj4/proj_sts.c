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
  double C_x, C_y, C_p; \
  int tan_mode;
#define PROJ_LIB__
# include  <lib_proj.h>
PROJ_HEAD(kav5, "Kavraisky V") "\n\tPCyl., Sph.";
PROJ_HEAD(qua_aut, "Quartic Authalic") "\n\tPCyl., Sph.";
PROJ_HEAD(mbt_s, "McBryde-Thomas Sine (No. 1)") "\n\tPCyl., Sph.";
PROJ_HEAD(fouc, "Foucaut") "\n\tPCyl., Sph.";
PROJ_HEAD(gen_ts, "General Sine-Tangent") "\n\tPCyl., Sph.\n\t+t|+s +p= +q=";
/* sqrt(PI) */
#define SQPI 1.772453850905516027298167483 
FORWARD(s_forward); /* spheroid */
  double c;

  xy.x = P->C_x * lp.lam * cos(lp.phi);
  xy.y = P->C_y;
  lp.phi *= P->C_p;
  c = cos(lp.phi);
  if (P->tan_mode) {
    xy.x *= c * c;
    xy.y *= tan(lp.phi);
  } else {
    xy.x /= c;
    xy.y *= sin(lp.phi);
  }
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  double c;
  
  xy.y /= P->C_y;
  c = cos(lp.phi = P->tan_mode ? atan(xy.y) : proj_asin(xy.y));
  lp.lam = xy.x / (P->C_x * cos(lp.phi /= P->C_p));
  if (P->tan_mode)
    lp.lam /= c * c;
  else
    lp.lam *= c;
  return (lp);
}
FREEUP; if (P) free(P); }
  static PROJ *
setup(PROJ *P, double p, double q, int mode) {
  P->es = 0.;
  P->inv = s_inverse;
  P->fwd = s_forward;
  P->C_x = q / p;
  P->C_y = p;
  P->C_p = 1/ q;
  P->tan_mode = mode;
  return P;
}
ENTRY0(kav5) ENDENTRY(setup(P, 1.50488, 1.35439, 0))
ENTRY0(qua_aut) ENDENTRY(setup(P, 2., 2., 0))
ENTRY0(mbt_s) ENDENTRY(setup(P, 1.48875, 1.36509, 0))
ENTRY0(fouc) ENDENTRY(setup(P, SQPI, 2., 1))
ENTRY0(gen_ts)
  double p, q;
  int mode;

  mode = proj_param(P->params, "tt").i;
  if (!mode && !proj_param(P->params, "ts").i)
    E_ERROR(-48)
  if (!proj_param(P->params, "tp").i || !proj_param(P->params, "tq").i)
    E_ERROR(-49)
  p = proj_param(P->params, "dp").f;
  q = proj_param(P->params, "dq").f;
  (void)setup(P, p, q, mode);
ENDENTRY(P)
/*
** Log: proj_sts.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
