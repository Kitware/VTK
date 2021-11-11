/* based upon Snyder and Linck, USGS-NMD */
#define PJ_LIB__

#include <errno.h>
#include <math.h>

#include "proj.h"
#include "proj_internal.h"

PROJ_HEAD(lsat, "Space oblique for LANDSAT")
    "\n\tCyl, Sph&Ell\n\tlsat= path=";

#define TOL 1e-7

namespace { // anonymous namespace
struct pj_opaque {
    double a2, a4, b, c1, c3;
    double q, t, u, w, p22, sa, ca, xj, rlm, rlm2;
};
} // anonymous namespace

static void seraz0(double lam, double mult, PJ *P) {
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(P->opaque);
    double sdsq, h, s, fc, sd, sq, d__1 = 0;

    lam *= DEG_TO_RAD;
    sd = sin(lam);
    sdsq = sd * sd;
    s = Q->p22 * Q->sa * cos(lam) * sqrt((1. + Q->t * sdsq)
        / ((1. + Q->w * sdsq) * (1. + Q->q * sdsq)));

    d__1 = 1. + Q->q * sdsq;
    h = sqrt((1. + Q->q * sdsq) / (1. + Q->w * sdsq)) * ((1. + Q->w * sdsq)
        / (d__1 * d__1) - Q->p22 * Q->ca);

    sq = sqrt(Q->xj * Q->xj + s * s);
    fc = mult * (h * Q->xj - s * s) / sq;
    Q->b += fc;
    Q->a2 += fc * cos(lam + lam);
    Q->a4 += fc * cos(lam * 4.);
    fc = mult * s * (h + Q->xj) / sq;
    Q->c1 += fc * cos(lam);
    Q->c3 += fc * cos(lam * 3.);
}


static PJ_XY lsat_e_forward (PJ_LP lp, PJ *P) {          /* Ellipsoidal, forward */
    PJ_XY xy = {0.0,0.0};
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(P->opaque);
    int l, nn;
    double lamt = 0.0, xlam, sdsq, c, d, s, lamdp = 0.0, phidp, lampp, tanph;
    double lamtp, cl, sd, sp, sav, tanphi;

    if (lp.phi > M_HALFPI)
        lp.phi = M_HALFPI;
    else if (lp.phi < -M_HALFPI)
        lp.phi = -M_HALFPI;

    if (lp.phi >= 0. )
        lampp = M_HALFPI;
    else
        lampp = M_PI_HALFPI;
    tanphi = tan(lp.phi);
    for (nn = 0;;) {
        double fac;
        sav = lampp;
        lamtp = lp.lam + Q->p22 * lampp;
        cl = cos(lamtp);
        if( cl < 0 )
            fac = lampp + sin(lampp) * M_HALFPI;
        else
            fac = lampp - sin(lampp) * M_HALFPI;
        for (l = 50; l >= 0; --l) {
            lamt = lp.lam + Q->p22 * sav;
            c = cos(lamt);
            if (fabs(c) < TOL)
                lamt -= TOL;
            xlam = (P->one_es * tanphi * Q->sa + sin(lamt) * Q->ca) / c;
            lamdp = atan(xlam) + fac;
            if (fabs(fabs(sav) - fabs(lamdp)) < TOL)
                break;
            sav = lamdp;
        }
        if (!l || ++nn >= 3 || (lamdp > Q->rlm && lamdp < Q->rlm2))
            break;
        if (lamdp <= Q->rlm)
            lampp = M_TWOPI_HALFPI;
        else if (lamdp >= Q->rlm2)
            lampp = M_HALFPI;
    }
    if (l) {
        sp = sin(lp.phi);
        phidp = aasin(P->ctx,(P->one_es * Q->ca * sp - Q->sa * cos(lp.phi) *
            sin(lamt)) / sqrt(1. - P->es * sp * sp));
        tanph = log(tan(M_FORTPI + .5 * phidp));
        sd = sin(lamdp);
        sdsq = sd * sd;
        s = Q->p22 * Q->sa * cos(lamdp) * sqrt((1. + Q->t * sdsq)
             / ((1. + Q->w * sdsq) * (1. + Q->q * sdsq)));
        d = sqrt(Q->xj * Q->xj + s * s);
        xy.x = Q->b * lamdp + Q->a2 * sin(2. * lamdp) + Q->a4 *
            sin(lamdp * 4.) - tanph * s / d;
        xy.y = Q->c1 * sd + Q->c3 * sin(lamdp * 3.) + tanph * Q->xj / d;
    } else
        xy.x = xy.y = HUGE_VAL;
    return xy;
}


static PJ_LP lsat_e_inverse (PJ_XY xy, PJ *P) {          /* Ellipsoidal, inverse */
    PJ_LP lp = {0.0,0.0};
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(P->opaque);
    int nn;
    double lamt, sdsq, s, lamdp, phidp, sppsq, dd, sd, sl, fac, scl, sav, spp;
    lamdp = xy.x / Q->b;
    nn = 50;
    do {
        sav = lamdp;
        sd = sin(lamdp);
        sdsq = sd * sd;
        s = Q->p22 * Q->sa * cos(lamdp) * sqrt((1. + Q->t * sdsq)
             / ((1. + Q->w * sdsq) * (1. + Q->q * sdsq)));
        lamdp = xy.x + xy.y * s / Q->xj - Q->a2 * sin(
            2. * lamdp) - Q->a4 * sin(lamdp * 4.) - s / Q->xj * (
            Q->c1 * sin(lamdp) + Q->c3 * sin(lamdp * 3.));
        lamdp /= Q->b;
    } while (fabs(lamdp - sav) >= TOL && --nn);
    sl = sin(lamdp);
    fac = exp(sqrt(1. + s * s / Q->xj / Q->xj) * (xy.y -
        Q->c1 * sl - Q->c3 * sin(lamdp * 3.)));
    phidp = 2. * (atan(fac) - M_FORTPI);
    dd = sl * sl;
    if (fabs(cos(lamdp)) < TOL)
        lamdp -= TOL;
    spp = sin(phidp);
    sppsq = spp * spp;
    const double denom = 1. - sppsq * (1. + Q->u);
    if( denom == 0.0 ) {
        proj_errno_set(P, PROJ_ERR_COORD_TRANSFM_OUTSIDE_PROJECTION_DOMAIN);
        return proj_coord_error().lp;
    }
    lamt = atan(((1. - sppsq * P->rone_es) * tan(lamdp) *
        Q->ca - spp * Q->sa * sqrt((1. + Q->q * dd) * (
        1. - sppsq) - sppsq * Q->u) / cos(lamdp)) / denom);
    sl = lamt >= 0. ? 1. : -1.;
    scl = cos(lamdp) >= 0. ? 1. : -1;
    lamt -= M_HALFPI * (1. - scl) * sl;
    lp.lam = lamt - Q->p22 * lamdp;
    if (fabs(Q->sa) < TOL)
        lp.phi = aasin(P->ctx,spp / sqrt(P->one_es * P->one_es + P->es * sppsq));
    else
        lp.phi = atan((tan(lamdp) * cos(lamt) - Q->ca * sin(lamt)) /
            (P->one_es * Q->sa));
    return lp;
}


PJ *PROJECTION(lsat) {
    int land, path;
    double lam, alf, esc, ess;
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(calloc (1, sizeof (struct pj_opaque)));
    if (nullptr==Q)
        return pj_default_destructor(P, PROJ_ERR_OTHER /*ENOMEM*/);
    P->opaque = Q;

    land = pj_param(P->ctx, P->params, "ilsat").i;
    if (land <= 0 || land > 5)
    {
        proj_log_error(P, _("Invalid value for lsat: lsat should be in [1, 5] range"));
        return pj_default_destructor(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
    }

    path = pj_param(P->ctx, P->params, "ipath").i;
    const int maxPathVal = (land <= 3 ? 251 : 233);
    if (path <= 0 || path > maxPathVal)
    {
        proj_log_error(P, _("Invalid value for path: path should be in [1, %d] range"), maxPathVal);
        return pj_default_destructor(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
    }

    if (land <= 3) {
        P->lam0 = DEG_TO_RAD * 128.87 - M_TWOPI / 251. * path;
        Q->p22 = 103.2669323;
        alf = DEG_TO_RAD * 99.092;
    } else {
        P->lam0 = DEG_TO_RAD * 129.3 - M_TWOPI / 233. * path;
        Q->p22 = 98.8841202;
        alf = DEG_TO_RAD * 98.2;
    }
    Q->p22 /= 1440.;
    Q->sa = sin(alf);
    Q->ca = cos(alf);
    if (fabs(Q->ca) < 1e-9)
        Q->ca = 1e-9;
    esc = P->es * Q->ca * Q->ca;
    ess = P->es * Q->sa * Q->sa;
    Q->w = (1. - esc) * P->rone_es;
    Q->w = Q->w * Q->w - 1.;
    Q->q = ess * P->rone_es;
    Q->t = ess * (2. - P->es) * P->rone_es * P->rone_es;
    Q->u = esc * P->rone_es;
    Q->xj = P->one_es * P->one_es * P->one_es;
    Q->rlm = M_PI * (1. / 248. + .5161290322580645);
    Q->rlm2 = Q->rlm + M_TWOPI;
    Q->a2 = Q->a4 = Q->b = Q->c1 = Q->c3 = 0.;
    seraz0(0., 1., P);
    for (lam = 9.; lam <= 81.0001; lam += 18.)
        seraz0(lam, 4., P);
    for (lam = 18; lam <= 72.0001; lam += 18.)
        seraz0(lam, 2., P);
    seraz0(90., 1., P);
    Q->a2 /= 30.;
    Q->a4 /= 60.;
    Q->b /= 30.;
    Q->c1 /= 15.;
    Q->c3 /= 45.;

    P->inv = lsat_e_inverse;
    P->fwd = lsat_e_forward;

    return P;
}
