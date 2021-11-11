/***********************************************************************

        Kinematic datum shifting utilizing a deformation model

                    Kristian Evers, 2017-10-29

************************************************************************

Perform datum shifts by means of a deformation/velocity model.

    X_out = X_in + (T_ct - T_obs)*DX
    Y_out = Y_in + (T_ct - T_obs)*DY
    Z_out = Z_in + (T_ct - T_obs)*DZ


The deformation operation takes cartesian coordinates as input and
returns cartesian coordinates as well.

Corrections in the gridded model are in east, north, up (ENU) space.
Hence the input coordinates needs to be converted to ENU-space when
searching for corrections in the grid. The corrections are then converted
to cartesian PJ_XYZ-space and applied to the input coordinates (also in
cartesian space).

A full deformation model is described by two grids, one for the horizontal
components and one for the vertical component. The horizontal grid is
stored in CTable/CTable2 and the vertical grid is stored in the GTX
format. The NTv2 format should not be used for this purpose since grid-
values are scaled upon reading. Both grids are expected to contain
grid-values in units of mm/year in ENU-space.

************************************************************************
* Copyright (c) 2017, Kristian Evers
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
*
***********************************************************************/
#define PJ_LIB__
#include <errno.h>
#include "proj.h"
#include "proj_internal.h"
#include <math.h>
#include "grids.hpp"

#include <algorithm>

PROJ_HEAD(deformation, "Kinematic grid shift");

#define TOL 1e-8
#define MAX_ITERATIONS 10

using namespace NS_PROJ;

namespace { // anonymous namespace
struct deformationData {
    double dt = 0;
    double t_epoch = 0;
    PJ *cart = nullptr;
    ListOfGenericGrids grids{};
    ListOfHGrids hgrids{};
    ListOfVGrids vgrids{};
};
} // anonymous namespace

// ---------------------------------------------------------------------------

static bool get_grid_values(PJ* P,
                            deformationData* Q,
                            const PJ_LP& lp,
                            double& vx,
                            double& vy,
                            double& vz)
{
    GenericShiftGridSet* gridset = nullptr;
    auto grid = pj_find_generic_grid(Q->grids, lp, gridset);
    if( !grid ) {
        return false;
    }
    if( grid->isNullGrid() ) {
        vx = 0;
        vy = 0;
        vz = 0;
        return true;
    }
    const auto samplesPerPixel = grid->samplesPerPixel();
    if( samplesPerPixel < 3 ) {
        proj_log_error(P, "grid has not enough samples");
        return false;
    }
    int sampleE = 0;
    int sampleN = 1;
    int sampleU = 2;
    for( int i = 0; i < samplesPerPixel; i++ )
    {
        const auto desc = grid->description(i);
        if( desc == "east_velocity") {
            sampleE = i;
        } else if( desc == "north_velocity") {
            sampleN = i;
        } else if( desc == "up_velocity") {
            sampleU = i;
        }
    }
    const auto unit = grid->unit(sampleE);
    if( !unit.empty() && unit != "millimetres per year" ) {
        proj_log_error(P, "Only unit=millimetres per year currently handled");
        return false;
    }

    bool must_retry = false;
    if( !pj_bilinear_interpolation_three_samples(P->ctx, grid, lp,
                                                 sampleE, sampleN, sampleU,
                                                 vx, vy, vz,
                                                 must_retry) )
    {
        if( must_retry )
            return get_grid_values( P, Q, lp, vx, vy, vz);
        return false;
    }
    // divide by 1000 to get m/year
    vx /= 1000;
    vy /= 1000;
    vz /= 1000;
    return true;
}

/********************************************************************************/
static PJ_XYZ get_grid_shift(PJ* P, const PJ_XYZ& cartesian) {
/********************************************************************************
    Read correction values from grid. The cartesian input coordinates are
    converted to geodetic coordinates in order look up the correction values
    in the grid. Once the grid corrections are read we need to convert them
    from ENU-space to cartesian PJ_XYZ-space. ENU -> PJ_XYZ formula described in:

    Nørbech, T., et al, 2003(?), "Transformation from a Common Nordic Reference
    Frame to ETRS89 in Denmark, Finland, Norway, and Sweden – status report"

********************************************************************************/
    PJ_COORD geodetic, shift, temp;
    double sp, cp, sl, cl;
    int previous_errno = proj_errno_reset(P);
    auto Q = static_cast<deformationData*>(P->opaque);

    /* cartesian to geodetic */
    geodetic.lpz = pj_inv3d(cartesian, Q->cart);

    /* look up correction values in grids */
    if( !Q->grids.empty() )
    {
        double vx = 0;
        double vy = 0;
        double vz = 0;
        if( !get_grid_values(P, Q, geodetic.lp, vx, vy, vz) )
        {
            return proj_coord_error().xyz;
        }
        shift.xyz.x = vx;
        shift.xyz.y = vy;
        shift.xyz.z = vz;
    }
    else
    {
        shift.lp    = pj_hgrid_value(P, Q->hgrids, geodetic.lp);
        shift.enu.u = pj_vgrid_value(P, Q->vgrids, geodetic.lp, 1.0);

        if (proj_errno(P) == PROJ_ERR_COORD_TRANSFM_OUTSIDE_GRID)
            proj_log_debug(P, "coordinate (%.3f, %.3f) outside deformation model",
                        proj_todeg(geodetic.lpz.lam), proj_todeg(geodetic.lpz.phi));

        /* grid values are stored as mm/yr, we need m/yr */
        shift.xyz.x /= 1000;
        shift.xyz.y /= 1000;
        shift.xyz.z /= 1000;
    }

    /* pre-calc cosines and sines */
    sp = sin(geodetic.lpz.phi);
    cp = cos(geodetic.lpz.phi);
    sl = sin(geodetic.lpz.lam);
    cl = cos(geodetic.lpz.lam);

    /* ENU -> PJ_XYZ */
    temp.xyz.x = -sp*cl*shift.enu.n - sl*shift.enu.e + cp*cl*shift.enu.u;
    temp.xyz.y = -sp*sl*shift.enu.n + cl*shift.enu.e + cp*sl*shift.enu.u;
    temp.xyz.z =     cp*shift.enu.n +                     sp*shift.enu.u;

    shift.xyz = temp.xyz;

    proj_errno_restore(P, previous_errno);

    return shift.xyz;
}

/********************************************************************************/
static PJ_XYZ reverse_shift(PJ *P, PJ_XYZ input, double dt) {
/********************************************************************************
    Iteratively determine the reverse grid shift correction values.
*********************************************************************************/
    PJ_XYZ out, delta, dif;
    double z0;
    int i = MAX_ITERATIONS;

    delta = get_grid_shift(P, input);
    if (delta.x == HUGE_VAL) {
        return delta;
    }

    /* Store the original z shift for later application */
    z0 = delta.z;

    /* When iterating to find the best horizontal coordinate we also carry   */
    /* along the z-component, since we need it for the cartesian -> geodetic */
    /* conversion. The z-component adjustment is overwritten with z0 after   */
    /* the loop has finished.                                                */
    out.x = input.x - dt*delta.x;
    out.y = input.y - dt*delta.y;
    out.z = input.z + dt*delta.z;

    do {
        delta = get_grid_shift(P, out);

        if (delta.x == HUGE_VAL)
            break;

        dif.x  = out.x + dt*delta.x - input.x;
        dif.y  = out.y + dt*delta.y - input.y;
        dif.z  = out.z - dt*delta.z - input.z;
        out.x += dif.x;
        out.y += dif.y;
        out.z += dif.z;

    } while ( --i && hypot(dif.x, dif.y) > TOL );

    out.z = input.z - dt*z0;

    return out;
}

static PJ_XYZ forward_3d(PJ_LPZ lpz, PJ *P) {
    struct deformationData *Q = (struct deformationData *) P->opaque;
    PJ_COORD out, in;
    PJ_XYZ shift;
    in.lpz = lpz;
    out = in;

    if (Q->dt == HUGE_VAL) {
        out = proj_coord_error(); /* in the 3D case +t_obs must be specified */
        proj_log_debug(P, "+dt must be specified");
        return out.xyz;
    }

    shift = get_grid_shift(P, in.xyz);
    if (shift.x == HUGE_VAL) {
        return shift;
    }

    out.xyz.x += Q->dt * shift.x;
    out.xyz.y += Q->dt * shift.y;
    out.xyz.z += Q->dt * shift.z;

    return out.xyz;
}


static PJ_COORD forward_4d(PJ_COORD in, PJ *P) {
    struct deformationData *Q = (struct deformationData *) P->opaque;
    double dt;
    PJ_XYZ shift;
    PJ_COORD out = in;

    if (Q->dt != HUGE_VAL) {
        dt = Q->dt;
    } else {
        dt = in.xyzt.t - Q->t_epoch ;
    }

    shift = get_grid_shift(P, in.xyz);

    out.xyzt.x += dt*shift.x;
    out.xyzt.y += dt*shift.y;
    out.xyzt.z += dt*shift.z;


    return out;
}


static PJ_LPZ reverse_3d(PJ_XYZ in, PJ *P) {
    struct deformationData *Q = (struct deformationData *) P->opaque;
    PJ_COORD out;
    out.xyz = in;

    if (Q->dt == HUGE_VAL) {
        out = proj_coord_error(); /* in the 3D case +t_obs must be specified */
        proj_log_debug(P, "+dt must be specified");
        return out.lpz;
    }

    out.xyz = reverse_shift(P, in, Q->dt);

    return out.lpz;
}

static PJ_COORD reverse_4d(PJ_COORD in, PJ *P) {
    struct deformationData *Q = (struct deformationData *) P->opaque;
    PJ_COORD out = in;
    double dt;


    if (Q->dt != HUGE_VAL) {
            dt = Q->dt;
        } else {
            dt = in.xyzt.t - Q->t_epoch;
    }

    out.xyz = reverse_shift(P, in.xyz, dt);
    return out;
}

static PJ *destructor(PJ *P, int errlev) {
    if (nullptr==P)
        return nullptr;

    auto Q = static_cast<struct deformationData*>(P->opaque);
    if( Q )
    {
        if (Q->cart)
            Q->cart->destructor (Q->cart, errlev);
        delete Q;
    }
    P->opaque = nullptr;

    return pj_default_destructor(P, errlev);
}


PJ *TRANSFORMATION(deformation,1) {
    auto Q = new deformationData;
    P->opaque = (void *) Q;
    P->destructor = destructor;

    // Pass a dummy ellipsoid definition that will be overridden just afterwards
    Q->cart = proj_create(P->ctx, "+proj=cart +a=1");
    if (Q->cart == nullptr)
        return destructor(P, PROJ_ERR_OTHER /*ENOMEM*/);

    /* inherit ellipsoid definition from P to Q->cart */
    pj_inherit_ellipsoid_def (P, Q->cart);

    int has_xy_grids = pj_param(P->ctx, P->params, "txy_grids").i;
    int has_z_grids  = pj_param(P->ctx, P->params, "tz_grids").i;
    int has_grids  = pj_param(P->ctx, P->params, "tgrids").i;

    /* Build gridlists. Both horizontal and vertical grids are mandatory. */
    if ( !has_grids && (!has_xy_grids || !has_z_grids)) {
        proj_log_error(P, _("Either +grids or (+xy_grids and +z_grids) should be specified."));
        return destructor(P, PROJ_ERR_INVALID_OP_MISSING_ARG );
    }

    if( has_grids )
    {
        Q->grids = pj_generic_grid_init(P, "grids");
        /* Was gridlist compiled properly? */
        if ( proj_errno(P) ) {
            proj_log_error(P, _("could not find required grid(s).)"));
            return destructor(P, PROJ_ERR_INVALID_OP_FILE_NOT_FOUND_OR_INVALID);
        }
    }
    else
    {
        Q->hgrids = pj_hgrid_init(P, "xy_grids");
        if (proj_errno(P)) {
            proj_log_error(P, _("could not find requested xy_grid(s)."));
            return destructor(P, PROJ_ERR_INVALID_OP_FILE_NOT_FOUND_OR_INVALID);
        }

        Q->vgrids = pj_vgrid_init(P, "z_grids");
        if (proj_errno(P)) {
            proj_log_error(P, _("could not find requested z_grid(s)."));
            return destructor(P, PROJ_ERR_INVALID_OP_FILE_NOT_FOUND_OR_INVALID);
        }
    }

    Q->dt = HUGE_VAL;
    if (pj_param(P->ctx, P->params, "tdt").i) {
       Q->dt = pj_param(P->ctx, P->params, "ddt").f;
    }

    if (pj_param_exists(P->params, "t_obs")) {
        proj_log_error(P, _("+t_obs parameter is deprecated. Use +dt instead."));
        return destructor(P, PROJ_ERR_INVALID_OP_MISSING_ARG);
    }

    Q->t_epoch = HUGE_VAL;
    if (pj_param(P->ctx, P->params, "tt_epoch").i) {
        Q->t_epoch = pj_param(P->ctx, P->params, "dt_epoch").f;
    }

    if (Q->dt == HUGE_VAL && Q->t_epoch == HUGE_VAL) {
        proj_log_error(P, _("either +dt or +t_epoch needs to be set."));
        return destructor(P, PROJ_ERR_INVALID_OP_MISSING_ARG);
    }

    if (Q->dt != HUGE_VALL && Q->t_epoch != HUGE_VALL) {
        proj_log_error(P, _("+dt or +t_epoch are mutually exclusive."));
        return destructor(P, PROJ_ERR_INVALID_OP_MUTUALLY_EXCLUSIVE_ARGS);
    }

    P->fwd4d = forward_4d;
    P->inv4d = reverse_4d;
    P->fwd3d  = forward_3d;
    P->inv3d  = reverse_3d;
    P->fwd    = nullptr;
    P->inv    = nullptr;

    P->left  = PJ_IO_UNITS_CARTESIAN;
    P->right = PJ_IO_UNITS_CARTESIAN;

    return P;
}

