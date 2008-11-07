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
#define PROJ_PARMS__ \
  struct PROJconsts *link; \
  void *en; \
  double costh, sinth; \
  int  rot;
#define PROJ_LIB__
#include  <lib_proj.h>
#include  <string.h>
PROJ_HEAD(ob_tran, "General Oblique Transformation") "\n\tMisc Sph"
"\n\to_proj= plus parameters for projection"
"\n\to_lat_p= o_lon_p= (new pole) or"
"\n\to_alpha= o_lon_c= o_lat_c= or"
"\n\to_lon_1= o_lat_1= o_lon_2= o_lat_2=";
#define TOL 1e-10
FORWARD(forward);

  xy = P->link->fwd(proj_translate(lp, P->en), P->link);
  if (xy.x != HUGE_VAL && P->rot) {
    double tmp = xy.x;

    xy.x = xy.x * P->costh - xy.y * P->sinth;
    xy.y = tmp * P->sinth + xy.y * P->costh;;
  }
  return (xy);
}
INVERSE(inverse);

  if (P->rot) {
    double tmp = xy.x;

    xy.x = xy.x * P->costh + xy.y * P->sinth;
    xy.y = tmp * - P->sinth + xy.y * P->costh;;
  }
  lp = P->link->inv(xy, P->link);
  if (lp.lam != HUGE_VAL)
    lp = proj_inv_translate(lp, P->en);
  return (lp);
}
FREEUP;
  if (P) {
    if (P->link)
      (*(P->link->pfree))(P->link);
    if (P->en) free(P->en);
    free(P);
  }
}
ENTRY1(ob_tran, link)
  int i;
  double phip, lamp, theta;
  const char *name, *s;

  /* get name of projection to be translated */
  if (!(name = proj_param(P->params, "so_proj").s)) E_ERROR(-26);
  for (i = 0; (s = proj_list[i].id) && strcmp(name, s) ; ++i) ;
  if (!s || !(P->link = (*proj_list[i].proj)(0))) E_ERROR(-37);
  /* copy existing header into new */
  P->es = 0.; /* force to spherical */
  P->link->params = P->params;
  P->link->over = P->over;
  P->link->geoc = P->geoc;
  P->link->a = P->a;
  P->link->es = P->es;
  P->link->ra = P->ra;
  P->link->lam0 = P->lam0;
  P->link->phi0 = P->phi0;
  P->link->x0 = P->x0;
  P->link->y0 = P->y0;
  P->link->k0 = P->k0;
  /* force spherical earth */
  P->link->one_es = P->link->rone_es = 1.;
  P->link->es = P->link->e = 0.;
  if (!(P->link = proj_list[i].proj(P->link))) {
    freeup(P);
    return 0;
  }
  if ((P->rot = (fabs(theta = - proj_param(P->params, "rrot").f) != 0.))) {
    P->costh = cos(theta);
    P->sinth = sin(theta);
  }
  if (proj_param(P->params, "to_alpha").i) {
    double lamc, phic, alpha;

    lamc  = proj_param(P->params, "ro_lon_c").f;
    phic  = proj_param(P->params, "ro_lat_c").f;
    alpha  = proj_param(P->params, "ro_alpha").f;
    if (fabs(fabs(phic) - HALFPI) <= TOL)
      E_ERROR(-32);
    lamp = proj_atan2(-cos(alpha), -sin(alpha) * sin(phic));
    P->lam0 = lamc + proj_atan2(-cos(alpha), -sin(alpha) * sin(phic)) - PI;
    phip = fabs(proj_asin(cos(phic) * sin(alpha)));
    lamp = P->link->lam0;
  } else if (proj_param(P->params, "to_lat_p").i) { /* specified new pole */
    lamp = proj_param(P->params, "ro_lon_p").f;
    phip = proj_param(P->params, "ro_lat_p").f;
  } else { /* specified new "equator" points */
    double lam1, lam2, phi1, phi2, con;

    lam1 = proj_param(P->params, "ro_lon_1").f;
    phi1 = proj_param(P->params, "ro_lat_1").f;
    lam2 = proj_param(P->params, "ro_lon_2").f;
    phi2 = proj_param(P->params, "ro_lat_2").f;
    if (fabs(phi1 - phi2) <= TOL ||
      (con = fabs(phi1)) <= TOL ||
      fabs(con - HALFPI) <= TOL ||
      fabs(fabs(phi2) - HALFPI) <= TOL) E_ERROR(-33);
    lamp = atan2(cos(phi1) * sin(phi2) * cos(lam1) -
      sin(phi1) * cos(phi2) * cos(lam2),
      sin(phi1) * cos(phi2) * sin(lam2) -
      cos(phi1) * sin(phi2) * sin(lam1));
    phip = fabs(atan(-cos(lamp - lam1) / tan(phi1)));
    P->lam0 = lamp;
    lamp = P->link->lam0;
  }
  P->fwd = forward;
  P->inv = P->link->inv ? inverse : 0;
  if (!(P->en = proj_translate_ini(phip, lamp))) E_ERROR_0;
ENDENTRY(P)
/*
** Log: proj_ob_tran.c
** Revision 3.2  2006/01/19 16:16:08  gie
** minor "warning"
**
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
