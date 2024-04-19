/******************************************************************************
 * Project:  PROJ.4
 * Purpose:  Implementation of the aeqd (Azimuthal Equidistant) projection.
 * Author:   Gerald Evenden
 *
 ******************************************************************************
 * Copyright (c) 1995, Gerald Evenden
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#define PJ_LIB__
#include "geodesic.h"
#include "proj.h"
#include <errno.h>
#include "proj_internal.h"
#include <math.h>

namespace { // anonymous namespace
enum Mode {
    N_POLE = 0,
    S_POLE = 1,
    EQUIT  = 2,
    OBLIQ  = 3
};
} // anonymous namespace

namespace { // anonymous namespace
struct pj_opaque {
    double  sinph0;
    double  cosph0;
    double  *en;
    double  M1;
    double  N1;
    double  Mp;
    double  He;
    double  G;
    enum Mode mode;
    struct geod_geodesic g;
};
} // anonymous namespace

PROJ_HEAD(aeqd, "Azimuthal Equidistant") "\n\tAzi, Sph&Ell\n\tlat_0 guam";

#define EPS10 1.e-10
#define TOL 1.e-14


static PJ *destructor (PJ *P, int errlev) {                        /* Destructor */
    if (nullptr==P)
        return nullptr;

    if (nullptr==P->opaque)
        return pj_default_destructor (P, errlev);

    free (static_cast<struct pj_opaque*>(P->opaque)->en);
    return pj_default_destructor (P, errlev);
}



static PJ_XY e_guam_fwd(PJ_LP lp, PJ *P) {        /* Guam elliptical */
    PJ_XY xy = {0.0,0.0};
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(P->opaque);
    double  cosphi, sinphi, t;

    cosphi = cos(lp.phi);
    sinphi = sin(lp.phi);
    t = 1. / sqrt(1. - P->es * sinphi * sinphi);
    xy.x = lp.lam * cosphi * t;
    xy.y = pj_mlfn(lp.phi, sinphi, cosphi, Q->en) - Q->M1 +
        .5 * lp.lam * lp.lam * cosphi * sinphi * t;

    return xy;
}


static PJ_XY aeqd_e_forward (PJ_LP lp, PJ *P) {          /* Ellipsoidal, forward */
    PJ_XY xy = {0.0,0.0};
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(P->opaque);
    double  coslam, cosphi, sinphi, rho;
    double azi1, azi2, s12;
    double lam1, phi1, lam2, phi2;

    coslam = cos(lp.lam);
    cosphi = cos(lp.phi);
    sinphi = sin(lp.phi);
    switch (Q->mode) {
    case N_POLE:
        coslam = - coslam;
        /*-fallthrough*/
    case S_POLE:
        rho = fabs(Q->Mp - pj_mlfn(lp.phi, sinphi, cosphi, Q->en));
        xy.x = rho * sin(lp.lam);
        xy.y = rho * coslam;
        break;
    case EQUIT:
    case OBLIQ:
        if (fabs(lp.lam) < EPS10 && fabs(lp.phi - P->phi0) < EPS10) {
            xy.x = xy.y = 0.;
            break;
        }

        phi1 = P->phi0 / DEG_TO_RAD;
        lam1 = P->lam0 / DEG_TO_RAD;
        phi2 = lp.phi / DEG_TO_RAD;
        lam2 = (lp.lam+P->lam0) / DEG_TO_RAD;

        geod_inverse(&Q->g, phi1, lam1, phi2, lam2, &s12, &azi1, &azi2);
        azi1 *= DEG_TO_RAD;
        xy.x = s12 * sin(azi1) / P->a;
        xy.y = s12 * cos(azi1) / P->a;
        break;
    }
    return xy;
}


static PJ_XY aeqd_s_forward (PJ_LP lp, PJ *P) {           /* Spheroidal, forward */
    PJ_XY xy = {0.0,0.0};
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(P->opaque);
    double  coslam, cosphi, sinphi;

    sinphi = sin(lp.phi);
    cosphi = cos(lp.phi);
    coslam = cos(lp.lam);
    switch (Q->mode) {
    case EQUIT:
        xy.y = cosphi * coslam;
        goto oblcon;
    case OBLIQ:
        xy.y = Q->sinph0 * sinphi + Q->cosph0 * cosphi * coslam;
oblcon:
        if (fabs(fabs(xy.y) - 1.) < TOL)
            if (xy.y < 0.) {
                proj_errno_set(P, PROJ_ERR_COORD_TRANSFM_OUTSIDE_PROJECTION_DOMAIN);
                return xy;
            }
            else
                return aeqd_e_forward(lp, P);
        else {
            xy.y = acos(xy.y);
            xy.y /= sin(xy.y);
            xy.x = xy.y * cosphi * sin(lp.lam);
            xy.y *= (Q->mode == EQUIT) ? sinphi :
                Q->cosph0 * sinphi - Q->sinph0 * cosphi * coslam;
        }
        break;
    case N_POLE:
        lp.phi = -lp.phi;
        coslam = -coslam;
        /*-fallthrough*/
    case S_POLE:
        if (fabs(lp.phi - M_HALFPI) < EPS10) {
            proj_errno_set(P, PROJ_ERR_COORD_TRANSFM_OUTSIDE_PROJECTION_DOMAIN);
            return xy;
        }
        xy.y = (M_HALFPI + lp.phi);
        xy.x = xy.y * sin(lp.lam);
        xy.y *= coslam;
        break;
    }
    return xy;
}


static PJ_LP e_guam_inv(PJ_XY xy, PJ *P) { /* Guam elliptical */
    PJ_LP lp = {0.0,0.0};
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(P->opaque);
    double x2, t = 0.0;
    int i;

    x2 = 0.5 * xy.x * xy.x;
    lp.phi = P->phi0;
    for (i = 0; i < 3; ++i) {
        t = P->e * sin(lp.phi);
        t = sqrt(1. - t * t);
        lp.phi = pj_inv_mlfn(P->ctx, Q->M1 + xy.y -
            x2 * tan(lp.phi) * t, P->es, Q->en);
    }
    lp.lam = xy.x * t / cos(lp.phi);
    return lp;
}


static PJ_LP aeqd_e_inverse (PJ_XY xy, PJ *P) {          /* Ellipsoidal, inverse */
    PJ_LP lp = {0.0,0.0};
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(P->opaque);
    double c;
    double azi1, azi2, s12, x2, y2, lat1, lon1, lat2, lon2;

    if ((c = hypot(xy.x, xy.y)) < EPS10) {
        lp.phi = P->phi0;
        lp.lam = 0.;
        return (lp);
    }
    if (Q->mode == OBLIQ || Q->mode == EQUIT) {

        x2 = xy.x * P->a;
        y2 = xy.y * P->a;
        lat1 = P->phi0 / DEG_TO_RAD;
        lon1 = P->lam0 / DEG_TO_RAD;
        azi1 = atan2(x2, y2) / DEG_TO_RAD;
        s12 = sqrt(x2 * x2 + y2 * y2);
        geod_direct(&Q->g, lat1, lon1, azi1, s12, &lat2, &lon2, &azi2);
        lp.phi = lat2 * DEG_TO_RAD;
        lp.lam = lon2 * DEG_TO_RAD;
        lp.lam -= P->lam0;
    } else { /* Polar */
        lp.phi = pj_inv_mlfn(P->ctx, Q->mode == N_POLE ? Q->Mp - c : Q->Mp + c,
            P->es, Q->en);
        lp.lam = atan2(xy.x, Q->mode == N_POLE ? -xy.y : xy.y);
    }
    return lp;
}


static PJ_LP aeqd_s_inverse (PJ_XY xy, PJ *P) {           /* Spheroidal, inverse */
    PJ_LP lp = {0.0,0.0};
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(P->opaque);
    double cosc, c_rh, sinc;

    c_rh = hypot(xy.x, xy.y);
    if (c_rh > M_PI) {
        if (c_rh - EPS10 > M_PI) {
            proj_errno_set(P, PROJ_ERR_COORD_TRANSFM_OUTSIDE_PROJECTION_DOMAIN);
            return lp;
        }
        c_rh = M_PI;
    } else if (c_rh < EPS10) {
        lp.phi = P->phi0;
        lp.lam = 0.;
        return (lp);
    }
    if (Q->mode == OBLIQ || Q->mode == EQUIT) {
        sinc = sin(c_rh);
        cosc = cos(c_rh);
        if (Q->mode == EQUIT) {
            lp.phi = aasin(P->ctx, xy.y * sinc / c_rh);
            xy.x *= sinc;
            xy.y = cosc * c_rh;
        } else {
            lp.phi = aasin(P->ctx,cosc * Q->sinph0 + xy.y * sinc * Q->cosph0 /
                c_rh);
            xy.y = (cosc - Q->sinph0 * sin(lp.phi)) * c_rh;
            xy.x *= sinc * Q->cosph0;
        }
        lp.lam = xy.y == 0. ? 0. : atan2(xy.x, xy.y);
    } else if (Q->mode == N_POLE) {
        lp.phi = M_HALFPI - c_rh;
        lp.lam = atan2(xy.x, -xy.y);
    } else {
        lp.phi = c_rh - M_HALFPI;
        lp.lam = atan2(xy.x, xy.y);
    }
    return lp;
}


PJ *PROJECTION(aeqd) {
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(calloc (1, sizeof (struct pj_opaque)));
    if (nullptr==Q)
        return pj_default_destructor (P, PROJ_ERR_OTHER /*ENOMEM*/);
    P->opaque = Q;
    P->destructor = destructor;

    geod_init(&Q->g, P->a, P->es / (1 + sqrt(P->one_es)));

    if (fabs(fabs(P->phi0) - M_HALFPI) < EPS10) {
        Q->mode = P->phi0 < 0. ? S_POLE : N_POLE;
        Q->sinph0 = P->phi0 < 0. ? -1. : 1.;
        Q->cosph0 = 0.;
    } else if (fabs(P->phi0) < EPS10) {
        Q->mode = EQUIT;
        Q->sinph0 = 0.;
        Q->cosph0 = 1.;
    } else {
        Q->mode = OBLIQ;
        Q->sinph0 = sin(P->phi0);
        Q->cosph0 = cos(P->phi0);
    }
    if (P->es == 0.0) {
        P->inv = aeqd_s_inverse;
        P->fwd = aeqd_s_forward;
    } else {
        if (!(Q->en = pj_enfn(P->es)))
            return pj_default_destructor (P, 0);
        if (pj_param(P->ctx, P->params, "bguam").i) {
            Q->M1 = pj_mlfn(P->phi0, Q->sinph0, Q->cosph0, Q->en);
            P->inv = e_guam_inv;
            P->fwd = e_guam_fwd;
        } else {
            switch (Q->mode) {
            case N_POLE:
                Q->Mp = pj_mlfn(M_HALFPI, 1., 0., Q->en);
                break;
            case S_POLE:
                Q->Mp = pj_mlfn(-M_HALFPI, -1., 0., Q->en);
                break;
            case EQUIT:
            case OBLIQ:
                Q->N1 = 1. / sqrt(1. - P->es * Q->sinph0 * Q->sinph0);
                Q->He = P->e / sqrt(P->one_es);
                Q->G = Q->sinph0 * Q->He;
                Q->He *= Q->cosph0;
                break;
            }
            P->inv = aeqd_e_inverse;
            P->fwd = aeqd_e_forward;
        }
    }

    return P;
}


