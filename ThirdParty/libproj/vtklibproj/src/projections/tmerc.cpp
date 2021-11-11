/*
*                   Transverse Mercator implementations
*
* In this file two transverse mercator implementations are found. One of Gerald
* Evenden/John Snyder origin and one of Knud Poder/Karsten Engsager origin. The
* former is regarded as "approximate" in the following and the latter is "exact".
* This word choice has been made to distinguish between the two algorithms, where
* the Evenden/Snyder implementation is the faster, less accurate implementation
* and the Poder/Engsager algorithm is a slightly slower, but more accurate
* implementation.
*/

#define PJ_LIB__

#include <errno.h>
#include <math.h>

#include "proj.h"
#include "proj_internal.h"
#include <math.h>
#include "mlfn.hpp"

PROJ_HEAD(tmerc, "Transverse Mercator") "\n\tCyl, Sph&Ell\n\tapprox";
PROJ_HEAD(etmerc, "Extended Transverse Mercator") "\n\tCyl, Sph";
PROJ_HEAD(utm, "Universal Transverse Mercator (UTM)") "\n\tCyl, Ell\n\tzone= south approx";

namespace { // anonymous namespace

// Approximate: Evenden/Snyder
struct EvendenSnyder
{
    double  esp;
    double  ml0;
    double  *en;
};

// More exact: Poder/Engsager
struct PoderEngsager
{
    double    Qn;     /* Merid. quad., scaled to the projection */
    double    Zb;     /* Radius vector in polar coord. systems  */
    double    cgb[6]; /* Constants for Gauss -> Geo lat */
    double    cbg[6]; /* Constants for Geo lat -> Gauss */
    double    utg[6]; /* Constants for transv. merc. -> geo */
    double    gtu[6]; /* Constants for geo -> transv. merc. */
};

struct tmerc_data {
    EvendenSnyder approx;
    PoderEngsager exact;
};

} // anonymous namespace

/* Constants for "approximate" transverse mercator */
#define EPS10   1.e-10
#define FC1 1.
#define FC2 .5
#define FC3 .16666666666666666666
#define FC4 .08333333333333333333
#define FC5 .05
#define FC6 .03333333333333333333
#define FC7 .02380952380952380952
#define FC8 .01785714285714285714

/* Constant for "exact" transverse mercator */
#define PROJ_ETMERC_ORDER 6

/*****************************************************************************/
//
//                  Approximate Transverse Mercator functions
//
/*****************************************************************************/


static PJ_XY approx_e_fwd (PJ_LP lp, PJ *P)
{
    PJ_XY xy = {0.0, 0.0};
    const auto *Q = &(static_cast<struct tmerc_data*>(P->opaque)->approx);
    double al, als, n, cosphi, sinphi, t;

    /*
     * Fail if our longitude is more than 90 degrees from the
     * central meridian since the results are essentially garbage.
     * Is error -20 really an appropriate return value?
     *
     *  http://trac.osgeo.org/proj/ticket/5
     */
    if( lp.lam < -M_HALFPI || lp.lam > M_HALFPI ) {
        xy.x = HUGE_VAL;
        xy.y = HUGE_VAL;
        proj_context_errno_set( P->ctx, PROJ_ERR_COORD_TRANSFM_OUTSIDE_PROJECTION_DOMAIN );
        return xy;
    }

    sinphi = sin (lp.phi);
    cosphi = cos (lp.phi);
    t = fabs (cosphi) > 1e-10 ? sinphi/cosphi : 0.;
    t *= t;
    al = cosphi * lp.lam;
    als = al * al;
    al /= sqrt (1. - P->es * sinphi * sinphi);
    n = Q->esp * cosphi * cosphi;
    xy.x = P->k0 * al * (FC1 +
        FC3 * als * (1. - t + n +
        FC5 * als * (5. + t * (t - 18.) + n * (14. - 58. * t)
        + FC7 * als * (61. + t * ( t * (179. - t) - 479. ) )
        )));
    xy.y = P->k0 * (inline_pj_mlfn(lp.phi, sinphi, cosphi, Q->en) - Q->ml0 +
        sinphi * al * lp.lam * FC2 * ( 1. +
        FC4 * als * (5. - t + n * (9. + 4. * n) +
        FC6 * als * (61. + t * (t - 58.) + n * (270. - 330 * t)
        + FC8 * als * (1385. + t * ( t * (543. - t) - 3111.) )
        ))));
    return (xy);
}

static PJ_XY tmerc_spherical_fwd (PJ_LP lp, PJ *P) {
    PJ_XY xy = {0.0,0.0};
    double b, cosphi;
    const auto *Q = &(static_cast<struct tmerc_data*>(P->opaque)->approx);

    cosphi = cos(lp.phi);
    b = cosphi * sin (lp.lam);
    if (fabs (fabs (b) - 1.) <= EPS10) {
        proj_errno_set(P, PROJ_ERR_COORD_TRANSFM_OUTSIDE_PROJECTION_DOMAIN);
        return xy;
    }

    xy.x = Q->ml0 * log ((1. + b) / (1. - b));
    xy.y = cosphi * cos (lp.lam) / sqrt (1. - b * b);

    b = fabs ( xy.y );
    if (cosphi == 1 && (lp.lam < -M_HALFPI || lp.lam > M_HALFPI) ) {
        /* Helps to be able to roundtrip |longitudes| > 90 at lat=0 */
        /* We could also map to -M_PI ... */
        xy.y = M_PI;
    }
    else if (b >= 1.) {
        if ((b - 1.) > EPS10) {
            proj_errno_set(P, PROJ_ERR_COORD_TRANSFM_OUTSIDE_PROJECTION_DOMAIN);
            return xy;
        }
        else xy.y = 0.;
    } else
        xy.y = acos (xy.y);

    if (lp.phi < 0.)
        xy.y = -xy.y;
    xy.y = Q->esp * (xy.y - P->phi0);
    return xy;
}

static PJ_LP approx_e_inv (PJ_XY xy, PJ *P) {
    PJ_LP lp = {0.0,0.0};
    const auto *Q = &(static_cast<struct tmerc_data*>(P->opaque)->approx);

    double sinphi, cosphi;
    lp.phi = inline_pj_inv_mlfn(P->ctx, Q->ml0 + xy.y / P->k0, P->es, Q->en, &sinphi, &cosphi);
    if (fabs(lp.phi) >= M_HALFPI) {
        lp.phi = xy.y < 0. ? -M_HALFPI : M_HALFPI;
        lp.lam = 0.;
    } else {
        double t = fabs (cosphi) > 1e-10 ? sinphi/cosphi : 0.;
        const double n = Q->esp * cosphi * cosphi;
        double con = 1. - P->es * sinphi * sinphi;
        const double d = xy.x * sqrt (con) / P->k0;
        con *= t;
        t *= t;
        const double ds = d * d;
        lp.phi -= (con * ds / (1.-P->es)) * FC2 * (1. -
            ds * FC4 * (5. + t * (3. - 9. *  n) + n * (1. - 4 * n) -
            ds * FC6 * (61. + t * (90. - 252. * n +
                45. * t) + 46. * n
           - ds * FC8 * (1385. + t * (3633. + t * (4095. + 1575. * t)) )
            )));
        lp.lam = d*(FC1 -
            ds*FC3*( 1. + 2.*t + n -
            ds*FC5*(5. + t*(28. + 24.*t + 8.*n) + 6.*n
           - ds * FC7 * (61. + t * (662. + t * (1320. + 720. * t)) )
        ))) / cosphi;
    }
    return lp;
}

static PJ_LP tmerc_spherical_inv (PJ_XY xy, PJ *P) {
    PJ_LP lp = {0.0, 0.0};
    double h, g;
    const auto *Q = &(static_cast<struct tmerc_data*>(P->opaque)->approx);

    h = exp(xy.x / Q->esp);
    if( h == 0 ) {
        proj_errno_set(P, PROJ_ERR_COORD_TRANSFM_OUTSIDE_PROJECTION_DOMAIN);
        return proj_coord_error().lp;
    }
    g = .5 * (h - 1. / h);
    /* D, as in equation 8-8 of USGS "Map Projections - A Working Manual" */
    const double D = P->phi0 + xy.y / Q->esp;
    h = cos (D);
    lp.phi = asin(sqrt((1. - h * h) / (1. + g * g)));

    /* Make sure that phi is on the correct hemisphere when false northing is used */
    lp.phi = copysign(lp.phi, D);

    lp.lam = (g != 0.0 || h != 0.0) ? atan2 (g, h) : 0.;
    return lp;
}


static PJ *destructor(PJ *P, int errlev) {
    if (nullptr==P)
        return nullptr;

    if (nullptr==P->opaque)
        return pj_default_destructor(P, errlev);

    free (static_cast<struct tmerc_data*>(P->opaque)->approx.en);
    return pj_default_destructor(P, errlev);
}


static PJ *setup_approx(PJ *P) {
    auto *Q = &(static_cast<struct tmerc_data*>(P->opaque)->approx);

    if (P->es != 0.0) {
        if (!(Q->en = pj_enfn(P->es)))
            return pj_default_destructor(P, PROJ_ERR_OTHER /*ENOMEM*/);

        Q->ml0 = pj_mlfn(P->phi0, sin(P->phi0), cos(P->phi0), Q->en);
        Q->esp = P->es / (1. - P->es);
    } else {
        Q->esp = P->k0;
        Q->ml0 = .5 * Q->esp;
    }
    return P;
}



/*****************************************************************************/
//
//                  Exact Transverse Mercator functions
//
//
// The code in this file is largly based upon procedures:
//
// Written by: Knud Poder and Karsten Engsager
//
// Based on math from: R.Koenig and K.H. Weise, "Mathematische
// Grundlagen der hoeheren Geodaesie und Kartographie,
// Springer-Verlag, Berlin/Goettingen" Heidelberg, 1951.
//
// Modified and used here by permission of Reference Networks
// Division, Kort og Matrikelstyrelsen (KMS), Copenhagen, Denmark
//
/*****************************************************************************/

/* Helper functions for "exact" transverse mercator */
inline
static double gatg(const double *p1, int len_p1, double B, double cos_2B, double sin_2B) {
    double h = 0, h1, h2 = 0;

    const double two_cos_2B = 2*cos_2B;
    const double* p = p1 + len_p1;
    h1 = *--p;
    while (p - p1) {
        h = -h2 + two_cos_2B*h1 + *--p;
        h2 = h1;
        h1 = h;
    }
    return (B + h*sin_2B);
}

/* Complex Clenshaw summation */
inline
static double clenS(const double *a, int size,
                    double sin_arg_r, double cos_arg_r,
                    double sinh_arg_i, double cosh_arg_i,
                    double *R, double *I) {
    double      r, i, hr, hr1, hr2, hi, hi1, hi2;

    /* arguments */
    const double* p = a + size;
    r          =  2*cos_arg_r*cosh_arg_i;
    i          = -2*sin_arg_r*sinh_arg_i;

    /* summation loop */
    hi1 = hr1 = hi = 0;
    hr = *--p;
    for (; a - p;) {
        hr2 = hr1;
        hi2 = hi1;
        hr1 = hr;
        hi1 = hi;
        hr  = -hr2 + r*hr1 - i*hi1 + *--p;
        hi  = -hi2 + i*hr1 + r*hi1;
    }

    r   = sin_arg_r*cosh_arg_i;
    i   = cos_arg_r*sinh_arg_i;
    *R  = r*hr - i*hi;
    *I  = r*hi + i*hr;
    return *R;
}


/* Real Clenshaw summation */
static double clens(const double *a, int size, double arg_r) {
    double      r, hr, hr1, hr2, cos_arg_r;

    const double* p = a + size;
    cos_arg_r  = cos(arg_r);
    r          =  2*cos_arg_r;

    /* summation loop */
    hr1 = 0;
    hr = *--p;
    for (; a - p;) {
        hr2 = hr1;
        hr1 = hr;
        hr  = -hr2 + r*hr1 + *--p;
    }
    return sin (arg_r)*hr;
}

/* Ellipsoidal, forward */
static PJ_XY exact_e_fwd (PJ_LP lp, PJ *P) {
    PJ_XY xy = {0.0,0.0};
    const auto *Q = &(static_cast<struct tmerc_data*>(P->opaque)->exact);

    /* ell. LAT, LNG -> Gaussian LAT, LNG */
    double Cn  = gatg (Q->cbg, PROJ_ETMERC_ORDER, lp.phi, cos(2*lp.phi), sin(2*lp.phi));
    /* Gaussian LAT, LNG -> compl. sph. LAT */
    const double sin_Cn = sin (Cn);
    const double cos_Cn = cos (Cn);
    const double sin_Ce = sin (lp.lam);
    const double cos_Ce = cos (lp.lam);

    const double cos_Cn_cos_Ce = cos_Cn*cos_Ce;
    Cn     = atan2 (sin_Cn, cos_Cn_cos_Ce);

    const double inv_denom_tan_Ce = 1. / hypot (sin_Cn, cos_Cn_cos_Ce);
    const double tan_Ce = sin_Ce*cos_Cn * inv_denom_tan_Ce;
#if 0
    // Variant of the above: found not to be measurably faster
    const double sin_Ce_cos_Cn = sin_Ce*cos_Cn;
    const double denom = sqrt(1 - sin_Ce_cos_Cn * sin_Ce_cos_Cn);
    const double tan_Ce = sin_Ce_cos_Cn / denom;
#endif

    /* compl. sph. N, E -> ell. norm. N, E */
    double Ce  = asinh ( tan_Ce );     /* Replaces: Ce  = log(tan(FORTPI + Ce*0.5)); */

/*
 *  Non-optimized version:
 *  const double sin_arg_r  = sin(2*Cn);
 *  const double cos_arg_r  = cos(2*Cn);
 *
 *  Given:
 *      sin(2 * Cn) = 2 sin(Cn) cos(Cn)
 *          sin(atan(y)) = y / sqrt(1 + y^2)
 *          cos(atan(y)) = 1 / sqrt(1 + y^2)
 *      ==> sin(2 * Cn) = 2 tan_Cn / (1 + tan_Cn^2)
 *
 *      cos(2 * Cn) = 2cos^2(Cn) - 1
 *                  = 2 / (1 + tan_Cn^2) - 1
 */
    const double two_inv_denom_tan_Ce = 2 * inv_denom_tan_Ce;
    const double two_inv_denom_tan_Ce_square = two_inv_denom_tan_Ce * inv_denom_tan_Ce;
    const double tmp_r = cos_Cn_cos_Ce * two_inv_denom_tan_Ce_square;
    const double sin_arg_r  = sin_Cn * tmp_r;
    const double cos_arg_r  = cos_Cn_cos_Ce * tmp_r - 1;

/*
 *  Non-optimized version:
 *  const double sinh_arg_i = sinh(2*Ce);
 *  const double cosh_arg_i = cosh(2*Ce);
 *
 *  Given
 *      sinh(2 * Ce) = 2 sinh(Ce) cosh(Ce)
 *          sinh(asinh(y)) = y
 *          cosh(asinh(y)) = sqrt(1 + y^2)
 *      ==> sinh(2 * Ce) = 2 tan_Ce sqrt(1 + tan_Ce^2)
 *
 *      cosh(2 * Ce) = 2cosh^2(Ce) - 1
 *                   = 2 * (1 + tan_Ce^2) - 1
 *
 * and 1+tan_Ce^2 = 1 + sin_Ce^2 * cos_Cn^2 / (sin_Cn^2 + cos_Cn^2 * cos_Ce^2)
 *                = (sin_Cn^2 + cos_Cn^2 * cos_Ce^2 + sin_Ce^2 * cos_Cn^2) / (sin_Cn^2 + cos_Cn^2 * cos_Ce^2)
 *                = 1. / (sin_Cn^2 + cos_Cn^2 * cos_Ce^2)
 *                = inv_denom_tan_Ce^2
 *
 */
    const double sinh_arg_i = tan_Ce * two_inv_denom_tan_Ce;
    const double cosh_arg_i = two_inv_denom_tan_Ce_square - 1;

    double dCn, dCe;
    Cn += clenS (Q->gtu, PROJ_ETMERC_ORDER,
                 sin_arg_r, cos_arg_r, sinh_arg_i, cosh_arg_i,
                 &dCn, &dCe);
    Ce += dCe;
    if (fabs (Ce) <= 2.623395162778) {
        xy.y  = Q->Qn * Cn + Q->Zb;  /* Northing */
        xy.x  = Q->Qn * Ce;          /* Easting  */
    } else {
        proj_errno_set(P, PROJ_ERR_COORD_TRANSFM_OUTSIDE_PROJECTION_DOMAIN);
        xy.x = xy.y = HUGE_VAL;
    }
    return xy;
}


/* Ellipsoidal, inverse */
static PJ_LP exact_e_inv (PJ_XY xy, PJ *P) {
    PJ_LP lp = {0.0,0.0};
    const auto *Q = &(static_cast<struct tmerc_data*>(P->opaque)->exact);

    /* normalize N, E */
    double Cn = (xy.y - Q->Zb)/Q->Qn;
    double Ce = xy.x/Q->Qn;

    if (fabs(Ce) <= 2.623395162778) { /* 150 degrees */
        /* norm. N, E -> compl. sph. LAT, LNG */
        const double sin_arg_r  = sin(2*Cn);
        const double cos_arg_r  = cos(2*Cn);

        //const double sinh_arg_i = sinh(2*Ce);
        //const double cosh_arg_i = cosh(2*Ce);
        const double exp_2_Ce = exp(2*Ce);
        const double half_inv_exp_2_Ce = 0.5 / exp_2_Ce;
        const double sinh_arg_i = 0.5 * exp_2_Ce - half_inv_exp_2_Ce;
        const double cosh_arg_i = 0.5 * exp_2_Ce + half_inv_exp_2_Ce;

        double dCn_ignored, dCe;
        Cn += clenS(Q->utg, PROJ_ETMERC_ORDER,
                    sin_arg_r, cos_arg_r, sinh_arg_i, cosh_arg_i,
                    &dCn_ignored, &dCe);
        Ce += dCe;

        /* compl. sph. LAT -> Gaussian LAT, LNG */
        const double sin_Cn = sin (Cn);
        const double cos_Cn = cos (Cn);

#if 0
        // Non-optimized version:
        double sin_Ce, cos_Ce;
        Ce = atan (sinh (Ce));  // Replaces: Ce = 2*(atan(exp(Ce)) - FORTPI);
        sin_Ce = sin (Ce);
        cos_Ce = cos (Ce);
        Ce     = atan2 (sin_Ce, cos_Ce*cos_Cn);
        Cn     = atan2 (sin_Cn*cos_Ce,  hypot (sin_Ce, cos_Ce*cos_Cn));
#else
/*
 *      One can divide both member of Ce = atan2(...) by cos_Ce, which gives:
 *      Ce     = atan2 (tan_Ce, cos_Cn) = atan2(sinh(Ce), cos_Cn)
 *
 *      and the same for Cn = atan2(...)
 *      Cn     = atan2 (sin_Cn, hypot (sin_Ce, cos_Ce*cos_Cn)/cos_Ce)
 *             = atan2 (sin_Cn, hypot (sin_Ce/cos_Ce, cos_Cn))
 *             = atan2 (sin_Cn, hypot (tan_Ce, cos_Cn))
 *             = atan2 (sin_Cn, hypot (sinhCe, cos_Cn))
 */
        const double sinhCe = sinh (Ce);
        Ce     = atan2 (sinhCe, cos_Cn);
        const double modulus_Ce = hypot (sinhCe, cos_Cn);
        Cn     = atan2 (sin_Cn, modulus_Ce);
#endif

        /* Gaussian LAT, LNG -> ell. LAT, LNG */

        // Optimization of the computation of cos(2*Cn) and sin(2*Cn)
        const double tmp = 2 * modulus_Ce / (sinhCe * sinhCe + 1);
        const double sin_2_Cn  = sin_Cn * tmp;
        const double cos_2_Cn  = tmp * modulus_Ce - 1.;
        //const double cos_2_Cn = cos(2 * Cn);
        //const double sin_2_Cn = sin(2 * Cn);

        lp.phi = gatg (Q->cgb,  PROJ_ETMERC_ORDER, Cn, cos_2_Cn, sin_2_Cn);
        lp.lam = Ce;
    }
    else {
        proj_errno_set(P, PROJ_ERR_COORD_TRANSFM_OUTSIDE_PROJECTION_DOMAIN);
        lp.phi = lp.lam = HUGE_VAL;
    }
    return lp;
}

static PJ *setup_exact(PJ *P) {
    auto *Q = &(static_cast<struct tmerc_data*>(P->opaque)->exact);

    assert( P->es > 0 );

    /* third flattening */
    const double n = P->n;
    double np = n;

    /* COEF. OF TRIG SERIES GEO <-> GAUSS */
    /* cgb := Gaussian -> Geodetic, KW p190 - 191 (61) - (62) */
    /* cbg := Geodetic -> Gaussian, KW p186 - 187 (51) - (52) */
    /* PROJ_ETMERC_ORDER = 6th degree : Engsager and Poder: ICC2007 */

    Q->cgb[0] = n*( 2 + n*(-2/3.0  + n*(-2      + n*(116/45.0 + n*(26/45.0 +
                n*(-2854/675.0 ))))));
    Q->cbg[0] = n*(-2 + n*( 2/3.0  + n*( 4/3.0  + n*(-82/45.0 + n*(32/45.0 +
                n*( 4642/4725.0))))));
    np     *= n;
    Q->cgb[1] = np*(7/3.0 + n*( -8/5.0  + n*(-227/45.0 + n*(2704/315.0 +
                n*( 2323/945.0)))));
    Q->cbg[1] = np*(5/3.0 + n*(-16/15.0 + n*( -13/9.0  + n*( 904/315.0 +
                n*(-1522/945.0)))));
    np     *= n;
    /* n^5 coeff corrected from 1262/105 -> -1262/105 */
    Q->cgb[2] = np*( 56/15.0  + n*(-136/35.0 + n*(-1262/105.0 +
                n*( 73814/2835.0))));
    Q->cbg[2] = np*(-26/15.0  + n*(  34/21.0 + n*(    8/5.0   +
                n*(-12686/2835.0))));
    np     *= n;
    /* n^5 coeff corrected from 322/35 -> 332/35 */
    Q->cgb[3] = np*(4279/630.0 + n*(-332/35.0 + n*(-399572/14175.0)));
    Q->cbg[3] = np*(1237/630.0 + n*( -12/5.0  + n*( -24832/14175.0)));
    np     *= n;
    Q->cgb[4] = np*(4174/315.0 + n*(-144838/6237.0 ));
    Q->cbg[4] = np*(-734/315.0 + n*( 109598/31185.0));
    np     *= n;
    Q->cgb[5] = np*(601676/22275.0 );
    Q->cbg[5] = np*(444337/155925.0);

    /* Constants of the projections */
    /* Transverse Mercator (UTM, ITM, etc) */
    np = n*n;
    /* Norm. mer. quad, K&W p.50 (96), p.19 (38b), p.5 (2) */
    Q->Qn = P->k0/(1 + n) * (1 + np*(1/4.0 + np*(1/64.0 + np/256.0)));
    /* coef of trig series */
    /* utg := ell. N, E -> sph. N, E,  KW p194 (65) */
    /* gtu := sph. N, E -> ell. N, E,  KW p196 (69) */
    Q->utg[0] = n*(-0.5  + n*( 2/3.0 + n*(-37/96.0 + n*( 1/360.0 +
                n*(  81/512.0 + n*(-96199/604800.0))))));
    Q->gtu[0] = n*( 0.5  + n*(-2/3.0 + n*(  5/16.0 + n*(41/180.0 +
                n*(-127/288.0 + n*(  7891/37800.0 ))))));
    Q->utg[1] = np*(-1/48.0 + n*(-1/15.0 + n*(437/1440.0 + n*(-46/105.0 +
                n*( 1118711/3870720.0)))));
    Q->gtu[1] = np*(13/48.0 + n*(-3/5.0  + n*(557/1440.0 + n*(281/630.0 +
                n*(-1983433/1935360.0)))));
    np      *= n;
    Q->utg[2] = np*(-17/480.0 + n*(  37/840.0 + n*(  209/4480.0  +
                n*( -5569/90720.0 ))));
    Q->gtu[2] = np*( 61/240.0 + n*(-103/140.0 + n*(15061/26880.0 +
                n*(167603/181440.0))));
    np      *= n;
    Q->utg[3] = np*(-4397/161280.0 + n*(  11/504.0 + n*( 830251/7257600.0)));
    Q->gtu[3] = np*(49561/161280.0 + n*(-179/168.0 + n*(6601661/7257600.0)));
    np     *= n;
    Q->utg[4] = np*(-4583/161280.0 + n*(  108847/3991680.0));
    Q->gtu[4] = np*(34729/80640.0  + n*(-3418889/1995840.0));
    np     *= n;
    Q->utg[5] = np*(-20648693/638668800.0);
    Q->gtu[5] = np*(212378941/319334400.0);

    /* Gaussian latitude value of the origin latitude */
    const double Z = gatg (Q->cbg, PROJ_ETMERC_ORDER, P->phi0, cos(2*P->phi0), sin(2*P->phi0));

    /* Origin northing minus true northing at the origin latitude */
    /* i.e. true northing = N - P->Zb                         */
    Q->Zb  = - Q->Qn*(Z + clens(Q->gtu, PROJ_ETMERC_ORDER, 2*Z));

    return P;
}



static PJ_XY auto_e_fwd (PJ_LP lp, PJ *P) {
    if( fabs(lp.lam) > 3 * DEG_TO_RAD )
        return exact_e_fwd(lp, P);
    else
        return approx_e_fwd(lp, P);
}

static PJ_LP auto_e_inv (PJ_XY xy, PJ *P) {
    // For k = 1 and lon = 3 (from central meridian),
    // At lat = 0, we get x ~= 0.052, y = 0
    // At lat = 90, we get x = 0, y ~= 1.57
    // And the shape of this x=f(y) frontier curve is very very roughly a
    // parabola. Hence:
    if( fabs(xy.x) > 0.053 - 0.022 * xy.y * xy.y )
        return exact_e_inv(xy, P);
    else
        return approx_e_inv(xy, P);
}

static PJ *setup(PJ *P, TMercAlgo eAlg) {

    struct tmerc_data *Q = static_cast<struct tmerc_data*>(calloc (1, sizeof (struct tmerc_data)));
    if (nullptr==Q)
        return pj_default_destructor (P, PROJ_ERR_OTHER /*ENOMEM*/);
    P->opaque = Q;

    if( P->es == 0 )
        eAlg = TMercAlgo::EVENDEN_SNYDER;

    switch( eAlg )
    {
        case TMercAlgo::EVENDEN_SNYDER:
        {
            P->destructor = destructor;
            if( !setup_approx(P) )
                return nullptr;
            if( P->es == 0 )
            {
                P->inv = tmerc_spherical_inv;
                P->fwd = tmerc_spherical_fwd;
            }
            else
            {
                P->inv = approx_e_inv;
                P->fwd = approx_e_fwd;
            }
            break;
        }

        case TMercAlgo::PODER_ENGSAGER:
        {
            setup_exact(P);
            P->inv = exact_e_inv;
            P->fwd = exact_e_fwd;
            break;
        }

        case TMercAlgo::AUTO:
        {
            P->destructor = destructor;
            if( !setup_approx(P) )
                return nullptr;
            setup_exact(P);

            P->inv = auto_e_inv;
            P->fwd = auto_e_fwd;
            break;
        }
    }
    return P;
}


static bool getAlgoFromParams(PJ* P, TMercAlgo& algo)
{
    if( pj_param (P->ctx, P->params, "bapprox").i )
    {
        algo = TMercAlgo::EVENDEN_SNYDER;
        return true;
    }

    const char* algStr = pj_param (P->ctx, P->params, "salgo").s;
    if( algStr )
    {
        if( strcmp(algStr, "evenden_snyder") == 0 )
        {
            algo = TMercAlgo::EVENDEN_SNYDER;
            return true;
        }
        if( strcmp(algStr, "poder_engsager") == 0 )
        {
            algo = TMercAlgo::PODER_ENGSAGER;
            return true;
        }
        if( strcmp(algStr, "auto") == 0 )
        {
            algo = TMercAlgo::AUTO;
            // Don't return so that we can run a later validity check
        }
        else
        {
            proj_log_error (P, "unknown value for +algo");
            return false;
        }
    }
    else
    {
        pj_load_ini(P->ctx); // if not already done
        proj_context_errno_set(P->ctx, 0); // reset error in case proj.ini couldn't be found
        algo = P->ctx->defaultTmercAlgo;
    }

    // We haven't worked on the criterion on inverse transformation
    // when phi0 != 0 or if k0 is not close to 1 or for very oblate
    // ellipsoid (es > 0.1 is ~ rf < 200)
    if( algo == TMercAlgo::AUTO &&
            (P->es > 0.1 || P->phi0 != 0 || fabs(P->k0 - 1) > 0.01) )
    {
        algo = TMercAlgo::PODER_ENGSAGER;
    }

    return true;
}


/*****************************************************************************/
//
//                                Operation Setups
//
/*****************************************************************************/

PJ *PROJECTION(tmerc) {
    /* exact transverse mercator only exists in ellipsoidal form, */
    /* use approximate version if +a sphere is requested          */

    TMercAlgo algo;
    if( !getAlgoFromParams(P, algo) )
    {
        proj_log_error(P, _("Invalid value for algo"));
        return pj_default_destructor(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
    }
    return setup(P, algo);
}


PJ *PROJECTION(etmerc) {
    if (P->es == 0.0) {
        proj_log_error(P, _("Invalid value for eccentricity: it should not be zero"));
        return pj_default_destructor(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
    }

   return setup (P, TMercAlgo::PODER_ENGSAGER);
}


/* UTM uses the Poder/Engsager implementation for the underlying projection      */
/* UNLESS +approx is set in which case the Evenden/Snyder implementation is used. */
PJ *PROJECTION(utm) {
    long zone;
    if (P->es == 0.0) {
        proj_log_error(P, _("Invalid value for eccentricity: it should not be zero"));
        return pj_default_destructor(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
    }
    if (P->lam0 < -1000.0 || P->lam0 > 1000.0) {
        proj_log_error(P, _("Invalid value for lon_0"));
        return pj_default_destructor(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
    }

    P->y0 = pj_param (P->ctx, P->params, "bsouth").i ? 10000000. : 0.;
    P->x0 = 500000.;
    if (pj_param (P->ctx, P->params, "tzone").i) /* zone input ? */
    {
        zone = pj_param(P->ctx, P->params, "izone").i;
        if (zone > 0 && zone <= 60)
            --zone;
        else {
            proj_log_error(P, _("Invalid value for zone"));
            return pj_default_destructor(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
        }
    }
    else /* nearest central meridian input */
    {
        zone = lround((floor ((adjlon (P->lam0) + M_PI) * 30. / M_PI)));
        if (zone < 0)
            zone = 0;
        else if (zone >= 60)
            zone = 59;
    }
    P->lam0 = (zone + .5) * M_PI / 30. - M_PI;
    P->k0 = 0.9996;
    P->phi0 = 0.;

    TMercAlgo algo;
    if( !getAlgoFromParams(P, algo) )
    {
        proj_log_error(P, _("Invalid value for algo"));
        return pj_default_destructor(P, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE);
    }
    return setup(P, algo);
}
