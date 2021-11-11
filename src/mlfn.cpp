#include <math.h>

#include "proj.h"
#include "proj_internal.h"
#include "mlfn.hpp"

/* meridional distance for ellipsoid and inverse
**	8th degree - accurate to < 1e-5 meters when used in conjunction
**		with typical major axis values.
**	Inverse determines phi to EPS (1e-11) radians, about 1e-6 seconds.
*/
#define C00 1.
#define C02 .25
#define C04 .046875
#define C06 .01953125
#define C08 .01068115234375
#define C22 .75
#define C44 .46875
#define C46 .01302083333333333333
#define C48 .00712076822916666666
#define C66 .36458333333333333333
#define C68 .00569661458333333333
#define C88 .3076171875
#define EN_SIZE 5

double *pj_enfn(double es) {
    double t, *en;

    en = (double *) malloc(EN_SIZE * sizeof (double));
    if (nullptr==en)
        return nullptr;

    en[0] = C00 - es * (C02 + es * (C04 + es * (C06 + es * C08)));
    en[1] = es * (C22 - es * (C04 + es * (C06 + es * C08)));
    en[2] = (t = es * es) * (C44 - es * (C46 + es * C48));
    en[3] = (t *= es) * (C66 - es * C68);
    en[4] = t * es * C88;

    return en;
}

double
pj_mlfn(double phi, double sphi, double cphi, const double *en) {
    return inline_pj_mlfn(phi, sphi, cphi, en);
}

double
pj_inv_mlfn(PJ_CONTEXT *ctx, double arg, double es, const double *en) {
    double sinphi_ignored;
    double cosphi_ignored;
    return inline_pj_inv_mlfn(ctx, arg, es, en, &sinphi_ignored, &cosphi_ignored);
}
