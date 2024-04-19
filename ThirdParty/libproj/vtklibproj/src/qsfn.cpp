/* determine small q */
#include <math.h>
#include "proj.h"
#include "proj_internal.h"

# define EPSILON 1.0e-7

double pj_qsfn(double sinphi, double e, double one_es) {
    double con, div1, div2;

    if (e >= EPSILON) {
        con = e * sinphi;
        div1 = 1.0 - con * con;
        div2 = 1.0 + con;

        /* avoid zero division, fail gracefully */
        if (div1 == 0.0 || div2 == 0.0)
            return HUGE_VAL;

        return (one_es * (sinphi / div1 - (.5 / e) * log ((1. - con) / div2 )));
    } else
        return (sinphi + sinphi);
}
