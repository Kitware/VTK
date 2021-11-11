#define PJ_LIB__

#include "proj_internal.h"

PROJ_HEAD(noop,    "No operation");

static PJ_COORD noop(PJ_COORD coord, PJ *P) {
    (void) P;
    return coord;
}

PJ *CONVERSION(noop, 0) {
    P->fwd4d = noop;
    P->inv4d = noop;
    P->left  = PJ_IO_UNITS_WHATEVER;
    P->right = PJ_IO_UNITS_WHATEVER;
    return P;
}

