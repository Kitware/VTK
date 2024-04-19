 /*
 * Project:  PROJ
 * Purpose:  Implementation of the krovak (Krovak) projection.
 *           Definition: http://www.ihsenergy.com/epsg/guid7.html#1.4.3
 * Author:   Thomas Flemming, tf@ttqv.com
 *
 ******************************************************************************
 * Copyright (c) 2001, Thomas Flemming, tf@ttqv.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************
 * A description of the (forward) projection is found in:
 *
 *      Bohuslav Veverka,
 *
 *      KROVAK’S PROJECTION AND ITS USE FOR THE
 *      CZECH REPUBLIC AND THE SLOVAK REPUBLIC,
 *
 *      50 years of the Research Institute of
 *      and the Slovak Republic Geodesy, Topography and Cartography
 *
 * which can be found via the Wayback Machine:
 *
 *      https://web.archive.org/web/20150216143806/https://www.vugtk.cz/odis/sborniky/sb2005/Sbornik_50_let_VUGTK/Part_1-Scientific_Contribution/16-Veverka.pdf
 *
 * Further info, including the inverse projection, is given by EPSG:
 *
 *      Guidance Note 7 part 2
 *      Coordinate Conversions and Transformations including Formulas
 *
 *      http://www.iogp.org/pubs/373-07-2.pdf
 *
 * Variable names in this file mostly follows what is used in the
 * paper by Veverka.
 *
 * According to EPSG the full Krovak projection method should have
 * the following parameters.  Within PROJ the azimuth, and pseudo
 * standard parallel are hardcoded in the algorithm and can't be
 * altered from outside. The others all have defaults to match the
 * common usage with Krovak projection.
 *
 *      lat_0 = latitude of centre of the projection
 *
 *      lon_0 = longitude of centre of the projection
 *
 *      ** = azimuth (true) of the centre line passing through the
 *           centre of the projection
 *
 *      ** = latitude of pseudo standard parallel
 *
 *      k  = scale factor on the pseudo standard parallel
 *
 *      x_0 = False Easting of the centre of the projection at the
 *            apex of the cone
 *
 *      y_0 = False Northing of the centre of the projection at
 *            the apex of the cone
 *
 *****************************************************************************/

#define PJ_LIB__

#include <errno.h>
#include <math.h>

#include "proj.h"
#include "proj_internal.h"

PROJ_HEAD(krovak, "Krovak") "\n\tPCyl, Ell";

#define EPS 1e-15
#define UQ  1.04216856380474   /* DU(2, 59, 42, 42.69689) */
#define S0  1.37008346281555   /* Latitude of pseudo standard parallel 78deg 30'00" N */
/* Not sure at all of the appropriate number for MAX_ITER... */
#define MAX_ITER 100

namespace { // anonymous namespace
struct pj_opaque {
    double alpha;
    double k;
    double n;
    double rho0;
    double ad;
    int czech;
};
} // anonymous namespace


static PJ_XY krovak_e_forward (PJ_LP lp, PJ *P) {                /* Ellipsoidal, forward */
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(P->opaque);
    PJ_XY xy = {0.0,0.0};

    double gfi, u, deltav, s, d, eps, rho;

    gfi = pow ( (1. + P->e * sin(lp.phi)) / (1. - P->e * sin(lp.phi)), Q->alpha * P->e / 2.);

    u = 2. * (atan(Q->k * pow( tan(lp.phi / 2. + M_PI_4), Q->alpha) / gfi)-M_PI_4);
    deltav = -lp.lam * Q->alpha;

    s = asin(cos(Q->ad) * sin(u) + sin(Q->ad) * cos(u) * cos(deltav));
    const double cos_s =  cos(s);
    if( cos_s < 1e-12 )
    {
        xy.x = 0;
        xy.y = 0;
        return xy;
    }
    d = asin(cos(u) * sin(deltav) / cos_s);

    eps = Q->n * d;
    rho = Q->rho0 * pow(tan(S0 / 2. + M_PI_4) , Q->n) / pow(tan(s / 2. + M_PI_4) , Q->n);

    xy.y = rho * cos(eps);
    xy.x = rho * sin(eps);

    xy.y *= Q->czech;
    xy.x *= Q->czech;

    return xy;
}


static PJ_LP krovak_e_inverse (PJ_XY xy, PJ *P) {                /* Ellipsoidal, inverse */
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(P->opaque);
    PJ_LP lp = {0.0,0.0};

    double u, deltav, s, d, eps, rho, fi1, xy0;
    int i;

    xy0 = xy.x;
    xy.x = xy.y;
    xy.y = xy0;

    xy.x *= Q->czech;
    xy.y *= Q->czech;

    rho = sqrt(xy.x * xy.x + xy.y * xy.y);
    eps = atan2(xy.y, xy.x);

    d = eps / sin(S0);
    if( rho == 0.0 ) {
        s = M_PI_2;
    }
    else {
        s = 2. * (atan(  pow(Q->rho0 / rho, 1. / Q->n) * tan(S0 / 2. + M_PI_4)) - M_PI_4);
    }

    u = asin(cos(Q->ad) * sin(s) - sin(Q->ad) * cos(s) * cos(d));
    deltav = asin(cos(s) * sin(d) / cos(u));

    lp.lam = P->lam0 - deltav / Q->alpha;

    /* ITERATION FOR lp.phi */
    fi1 = u;

    for (i = MAX_ITER; i ; --i) {
        lp.phi = 2. * ( atan( pow( Q->k, -1. / Q->alpha)  *
                              pow( tan(u / 2. + M_PI_4) , 1. / Q->alpha)  *
                              pow( (1. + P->e * sin(fi1)) / (1. - P->e * sin(fi1)) , P->e / 2.)
                            )  - M_PI_4);

        if (fabs(fi1 - lp.phi) < EPS)
            break;
        fi1 = lp.phi;
    }
    if( i == 0 )
        proj_context_errno_set( P->ctx, PROJ_ERR_COORD_TRANSFM_OUTSIDE_PROJECTION_DOMAIN );

   lp.lam -= P->lam0;

   return lp;
}


PJ *PROJECTION(krovak) {
    double u0, n0, g;
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(calloc (1, sizeof (struct pj_opaque)));
    if (nullptr==Q)
        return pj_default_destructor (P, PROJ_ERR_OTHER /*ENOMEM*/);
    P->opaque = Q;

    /* we want Bessel as fixed ellipsoid */
    P->a = 6377397.155;
    P->es = 0.006674372230614;
    P->e = sqrt(P->es);

    /* if latitude of projection center is not set, use 49d30'N */
    if (!pj_param(P->ctx, P->params, "tlat_0").i)
            P->phi0 = 0.863937979737193;

    /* if center long is not set use 42d30'E of Ferro - 17d40' for Ferro */
    /* that will correspond to using longitudes relative to greenwich    */
    /* as input and output, instead of lat/long relative to Ferro */
    if (!pj_param(P->ctx, P->params, "tlon_0").i)
            P->lam0 = 0.7417649320975901 - 0.308341501185665;

    /* if scale not set default to 0.9999 */
    if (!pj_param(P->ctx, P->params, "tk").i && !pj_param(P->ctx, P->params, "tk_0").i)
        P->k0 = 0.9999;

    Q->czech = 1;
    if( !pj_param(P->ctx, P->params, "tczech").i )
        Q->czech = -1;

    /* Set up shared parameters between forward and inverse */
    Q->alpha = sqrt(1. + (P->es * pow(cos(P->phi0), 4)) / (1. - P->es));
    u0 = asin(sin(P->phi0) / Q->alpha);
    g = pow( (1. + P->e * sin(P->phi0)) / (1. - P->e * sin(P->phi0)) , Q->alpha * P->e / 2. );
    double tan_half_phi0_plus_pi_4 = tan(P->phi0 / 2. + M_PI_4);
    if( tan_half_phi0_plus_pi_4 == 0.0 ) {
        proj_log_error(P, _("Invalid value for lat_0: lat_0 + PI/4 should be different from 0"));
        return pj_default_destructor(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
    }
    Q->k = tan( u0 / 2. + M_PI_4) / pow  (tan_half_phi0_plus_pi_4 , Q->alpha) * g;
    n0 = sqrt(1. - P->es) / (1. - P->es * pow(sin(P->phi0), 2));
    Q->n = sin(S0);
    Q->rho0 = P->k0 * n0 / tan(S0);
    Q->ad = M_PI_2 - UQ;

    P->inv = krovak_e_inverse;
    P->fwd = krovak_e_forward;

    return P;
}
