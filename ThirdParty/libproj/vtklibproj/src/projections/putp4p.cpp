#define PJ_LIB__

#include <errno.h>
#include <math.h>

#include "proj.h"
#include "proj_internal.h"

namespace { // anonymous namespace
struct pj_opaque {
    double C_x, C_y;
};
} // anonymous namespace

PROJ_HEAD(putp4p, "Putnins P4'") "\n\tPCyl, Sph";
PROJ_HEAD(weren, "Werenskiold I") "\n\tPCyl, Sph";


static PJ_XY putp4p_s_forward (PJ_LP lp, PJ *P) {           /* Spheroidal, forward */
    PJ_XY xy = {0.0,0.0};
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(P->opaque);

    lp.phi = aasin(P->ctx,0.883883476 * sin(lp.phi));
    xy.x = Q->C_x * lp.lam * cos(lp.phi);
    lp.phi *= 0.333333333333333;
    xy.x /= cos(lp.phi);
    xy.y = Q->C_y * sin(lp.phi);

    return xy;
}


static PJ_LP putp4p_s_inverse (PJ_XY xy, PJ *P) {           /* Spheroidal, inverse */
    PJ_LP lp = {0.0,0.0};
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(P->opaque);

    lp.phi = aasin(P->ctx,xy.y / Q->C_y);
    lp.lam = xy.x * cos(lp.phi) / Q->C_x;
    lp.phi *= 3.;
    lp.lam /= cos(lp.phi);
    lp.phi = aasin(P->ctx,1.13137085 * sin(lp.phi));

    return lp;
}


PJ *PROJECTION(putp4p) {
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(calloc (1, sizeof (struct pj_opaque)));
    if (nullptr==Q)
        return pj_default_destructor (P, PROJ_ERR_OTHER /*ENOMEM*/);
    P->opaque = Q;

    Q->C_x = 0.874038744;
    Q->C_y = 3.883251825;

    P->es = 0.;
    P->inv = putp4p_s_inverse;
    P->fwd = putp4p_s_forward;

    return P;
}


PJ *PROJECTION(weren) {
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(calloc (1, sizeof (struct pj_opaque)));
    if (nullptr==Q)
        return pj_default_destructor (P, PROJ_ERR_OTHER /*ENOMEM*/);
    P->opaque = Q;

    Q->C_x = 1.;
    Q->C_y = 4.442882938;

    P->es = 0.;
    P->inv = putp4p_s_inverse;
    P->fwd = putp4p_s_forward;

    return P;
}
