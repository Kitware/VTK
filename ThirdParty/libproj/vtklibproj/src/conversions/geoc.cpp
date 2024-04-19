/******************************************************************************
 * Project:  PROJ.4
 * Purpose:  Conversion from geographic to geocentric latitude and back.
 * Author:   Thomas Knudsen (2017)
 *
 ******************************************************************************
 * Copyright (c) 2017, SDFE, http://www.sdfe.dk
 * Copyright (c) 2017, Thomas Knudsen
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

#include <math.h>

#include "proj.h"
#include "proj_internal.h"

PROJ_HEAD(geoc, "Geocentric Latitude");

/* Geographical to geocentric */
static PJ_COORD forward(PJ_COORD coo, PJ *P) {
    return pj_geocentric_latitude (P, PJ_FWD, coo);
}

/* Geocentric to geographical */
static PJ_COORD inverse(PJ_COORD coo, PJ *P) {
    return pj_geocentric_latitude (P, PJ_INV, coo);
}


static PJ *CONVERSION(geoc, 1) {
    P->inv4d = inverse;
    P->fwd4d = forward;

    P->left   =  PJ_IO_UNITS_RADIANS;
    P->right  =  PJ_IO_UNITS_RADIANS;

    P->is_latlong = 1;
    return P;
}
