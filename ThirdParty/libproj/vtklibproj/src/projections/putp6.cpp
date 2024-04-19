#define PJ_LIB__

#include <errno.h>
#include <math.h>

#include "proj.h"
#include "proj_internal.h"

namespace { // anonymous namespace
struct pj_opaque {
    double C_x, C_y, A, B, D;
};
} // anonymous namespace

PROJ_HEAD(putp6, "Putnins P6") "\n\tPCyl, Sph";
PROJ_HEAD(putp6p, "Putnins P6'") "\n\tPCyl, Sph";

#define EPS      1e-10
#define NITER    10
#define CON_POLE 1.732050807568877


static PJ_XY putp6_s_forward (PJ_LP lp, PJ *P) {           /* Spheroidal, forward */
    PJ_XY xy = {0.0,0.0};
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(P->opaque);
    int i;

    const double p = Q->B * sin(lp.phi);
    lp.phi *=  1.10265779;
    for (i = NITER; i ; --i) {
        const double r = sqrt(1. + lp.phi * lp.phi);
        const double V = ( (Q->A - r) * lp.phi - log(lp.phi + r) - p ) /
                            (Q->A - 2. * r);
        lp.phi -= V;
        if (fabs(V) < EPS)
            break;
    }
    if (!i)
        lp.phi = p < 0. ? -CON_POLE : CON_POLE;
    xy.x = Q->C_x * lp.lam * (Q->D - sqrt(1. + lp.phi * lp.phi));
    xy.y = Q->C_y * lp.phi;

    return xy;
}


static PJ_LP putp6_s_inverse (PJ_XY xy, PJ *P) {           /* Spheroidal, inverse */
    PJ_LP lp = {0.0,0.0};
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(P->opaque);
    double r;

    lp.phi = xy.y / Q->C_y;
    r = sqrt(1. + lp.phi * lp.phi);
    lp.lam = xy.x / (Q->C_x * (Q->D - r));
    lp.phi = aasin( P->ctx, ( (Q->A - r) * lp.phi - log(lp.phi + r) ) / Q->B);

    return lp;
}


PJ *PROJECTION(putp6) {
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(calloc (1, sizeof (struct pj_opaque)));
    if (nullptr==Q)
        return pj_default_destructor(P, PROJ_ERR_OTHER /*ENOMEM*/);
    P->opaque = Q;

    Q->C_x = 1.01346;
    Q->C_y = 0.91910;
    Q->A   = 4.;
    Q->B   = 2.1471437182129378784;
    Q->D   = 2.;

    P->es = 0.;
    P->inv = putp6_s_inverse;
    P->fwd = putp6_s_forward;

    return P;
}


PJ *PROJECTION(putp6p) {
    struct pj_opaque *Q = static_cast<struct pj_opaque*>(calloc (1, sizeof (struct pj_opaque)));
    if (nullptr==Q)
        return pj_default_destructor(P, PROJ_ERR_OTHER /*ENOMEM*/);
    P->opaque = Q;

    Q->C_x = 0.44329;
    Q->C_y = 0.80404;
    Q->A   = 6.;
    Q->B   = 5.61125;
    Q->D   = 3.;

    P->es = 0.;
    P->inv = putp6_s_inverse;
    P->fwd = putp6_s_forward;

    return P;
}
