#ifndef MLFN_HPP
#define MLFN_HPP

/* meridional distance for ellipsoid and inverse
**	8th degree - accurate to < 1e-5 meters when used in conjunction
**		with typical major axis values.
**	Inverse determines phi to EPS (1e-11) radians, about 1e-6 seconds.
*/

inline static double inline_pj_mlfn(double phi, double sphi, double cphi, const double *en) {
    cphi *= sphi;
    sphi *= sphi;
    return(en[0] * phi - cphi * (en[1] + sphi*(en[2]
            + sphi*(en[3] + sphi*en[4]))));
}

inline static double
inline_pj_inv_mlfn(PJ_CONTEXT *ctx, double arg, double es, const double *en,
                   double* sinphi, double* cosphi) {
    const double k = 1./(1.-es);
    constexpr double INV_MLFN_EPS = 1e-11;
    constexpr int INV_MFN_MAX_ITER = 10;
    double phi = arg;
    double s = sin(phi);
    double c = cos(phi);
    for (int i = INV_MFN_MAX_ITER; i ; --i) { /* rarely goes over 2 iterations */
        double t = 1. - es * s * s;
        t = (inline_pj_mlfn(phi, s, c, en) - arg) * (t * sqrt(t)) * k;
        phi -= t;
        if (fabs(t) < INV_MLFN_EPS)
        {
            // Instead of recomputing sin(phi), cos(phi) from scratch,
            // use sin(phi-t) and cos(phi-t) approximate formulas with
            // 1-term approximation of sin(t) and cos(t)
            *sinphi = s - c * t;
            *cosphi = c + s * t;
            return phi;
        }
        if (fabs(t) < 1e-3)
        {
            // 2-term approximation of sin(t) and cos(t)
            // Max relative error is 4e-14 on cos(t), and 8e-15 on sin(t)
            const double t2 = t * t;
            const double cos_t = 1 - 0.5 * t2;
            const double sin_t = t * (1 - (1. / 6) * t2);
            const double s_new = s * cos_t - c * sin_t;
            c = c * cos_t + s * sin_t;
            s = s_new;
        }
        else if (fabs(t) < 1e-2)
        {
            // 3-term approximation of sin(t) and cos(t)
            // Max relative error is 2e-15 on cos(t), and 2e-16 on sin(t)
            const double t2 = t * t;
            const double cos_t = 1 - 0.5 * t2 * (1 - (1. / 12) * t2);
            const double sin_t = t * (1 - (1. / 6) * t2 * (1 - (1. / 20) * t2));
            const double s_new = s * cos_t - c * sin_t;
            c = c * cos_t + s * sin_t;
            s = s_new;
        }
        else
        {
            s = sin(phi);
            c = cos(phi);
        }
    }
    *sinphi = s;
    *cosphi = c;
    proj_context_errno_set( ctx, PROJ_ERR_COORD_TRANSFM_OUTSIDE_PROJECTION_DOMAIN );
    return phi;
}

#endif // MLFN_HPP
