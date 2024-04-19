#define PJ_LIB__

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#include "proj_internal.h"
#include "proj/internal/mutex.hpp"
#include "grids.hpp"

PROJ_HEAD(vgridshift, "Vertical grid shift");

static NS_PROJ::mutex gMutex{};
static std::set<std::string> gKnownGrids{};

using namespace NS_PROJ;

namespace { // anonymous namespace
struct vgridshiftData {
    double t_final = 0;
    double t_epoch = 0;
    double forward_multiplier = 0;
    ListOfVGrids grids{};
    bool defer_grid_opening = false;
};
} // anonymous namespace

static void deal_with_vertcon_gtx_hack(PJ *P)
{
    struct vgridshiftData *Q = (struct vgridshiftData *) P->opaque;
    // The .gtx VERTCON files stored millimeters, but the .tif files
    // are in metres.
    if( Q->forward_multiplier != 0.001 ) {
        return;
    }
    const char* gridname = pj_param(P->ctx, P->params, "sgrids").s;
    if( !gridname ) {
        return;
    }
    if( strcmp(gridname, "vertconw.gtx") != 0 &&
        strcmp(gridname, "vertconc.gtx") != 0 &&
        strcmp(gridname, "vertcone.gtx") != 0 ) {
        return;
    }
    if( Q->grids.empty() ) {
        return;
    }
    const auto& grids = Q->grids[0]->grids();
    if( !grids.empty() &&
        grids[0]->name().find(".tif") != std::string::npos ) {
        Q->forward_multiplier = 1.0;
    }
}

static PJ_XYZ forward_3d(PJ_LPZ lpz, PJ *P) {
    struct vgridshiftData *Q = (struct vgridshiftData *) P->opaque;
    PJ_COORD point = {{0,0,0,0}};
    point.lpz = lpz;

    if ( Q->defer_grid_opening ) {
        Q->defer_grid_opening = false;
        Q->grids = pj_vgrid_init(P, "grids");
        deal_with_vertcon_gtx_hack(P);
        if ( proj_errno(P) ) {
            return proj_coord_error().xyz;
        }
    }

    if (!Q->grids.empty()) {
        /* Only try the gridshift if at least one grid is loaded,
         * otherwise just pass the coordinate through unchanged. */
        point.xyz.z += pj_vgrid_value(P, Q->grids, point.lp, Q->forward_multiplier);
    }

    return point.xyz;
}


static PJ_LPZ reverse_3d(PJ_XYZ xyz, PJ *P) {
    struct vgridshiftData *Q = (struct vgridshiftData *) P->opaque;
    PJ_COORD point = {{0,0,0,0}};
    point.xyz = xyz;

    if ( Q->defer_grid_opening ) {
        Q->defer_grid_opening = false;
        Q->grids = pj_vgrid_init(P, "grids");
        deal_with_vertcon_gtx_hack(P);
        if ( proj_errno(P) ) {
            return proj_coord_error().lpz;
        }
    }

    if (!Q->grids.empty()) {
        /* Only try the gridshift if at least one grid is loaded,
         * otherwise just pass the coordinate through unchanged. */
        point.xyz.z -= pj_vgrid_value(P, Q->grids, point.lp, Q->forward_multiplier);
    }

    return point.lpz;
}


static PJ_COORD forward_4d(PJ_COORD obs, PJ *P) {
    struct vgridshiftData *Q = (struct vgridshiftData *) P->opaque;
    PJ_COORD point = obs;

    /* If transformation is not time restricted, we always call it */
    if (Q->t_final==0 || Q->t_epoch==0) {
        point.xyz = forward_3d (obs.lpz, P);
        return point;
    }

    /* Time restricted - only apply transform if within time bracket */
    if (obs.lpzt.t < Q->t_epoch && Q->t_final > Q->t_epoch)
        point.xyz = forward_3d (obs.lpz, P);


    return point;
}

static PJ_COORD reverse_4d(PJ_COORD obs, PJ *P) {
    struct vgridshiftData *Q = (struct vgridshiftData *) P->opaque;
    PJ_COORD point = obs;

    /* If transformation is not time restricted, we always call it */
    if (Q->t_final==0 || Q->t_epoch==0) {
        point.lpz = reverse_3d (obs.xyz, P);
        return point;
    }

    /* Time restricted - only apply transform if within time bracket */
    if (obs.lpzt.t < Q->t_epoch && Q->t_final > Q->t_epoch)
        point.lpz = reverse_3d (obs.xyz, P);

    return point;
}

static PJ *destructor (PJ *P, int errlev) {
    if (nullptr==P)
        return nullptr;

    delete static_cast<struct vgridshiftData*>(P->opaque);
    P->opaque = nullptr;

    return pj_default_destructor(P, errlev);
}

static void reassign_context( PJ* P, PJ_CONTEXT* ctx )
{
    auto Q = (struct vgridshiftData *) P->opaque;
    for( auto& grid: Q->grids ) {
        grid->reassign_context(ctx);
    }
}


PJ *TRANSFORMATION(vgridshift,0) {
    auto Q = new vgridshiftData;
    P->opaque = (void *) Q;
    P->destructor = destructor;
    P->reassign_context = reassign_context;

   if (!pj_param(P->ctx, P->params, "tgrids").i) {
        proj_log_error(P, _("+grids parameter missing."));
        return destructor (P, PROJ_ERR_INVALID_OP_MISSING_ARG);
    }

   /* TODO: Refactor into shared function that can be used  */
   /* by both vgridshift and hgridshift                     */
   if (pj_param(P->ctx, P->params, "tt_final").i) {
        Q->t_final = pj_param (P->ctx, P->params, "dt_final").f;
        if (Q->t_final == 0) {
            /* a number wasn't passed to +t_final, let's see if it was "now" */
            /* and set the time accordingly.                                 */
            if (!strcmp("now", pj_param(P->ctx, P->params, "st_final").s)) {
                    time_t now;
                    struct tm *date;
                    time(&now);
                    date = localtime(&now);
                    Q->t_final = 1900.0 + date->tm_year + date->tm_yday/365.0;
            }
        }
    }

   if (pj_param(P->ctx, P->params, "tt_epoch").i)
        Q->t_epoch = pj_param (P->ctx, P->params, "dt_epoch").f;

    /* historical: the forward direction subtracts the grid offset. */
    Q->forward_multiplier = -1.0;
    if (pj_param(P->ctx, P->params, "tmultiplier").i) {
        Q->forward_multiplier = pj_param(P->ctx, P->params, "dmultiplier").f;
    }

    if( P->ctx->defer_grid_opening ) {
        Q->defer_grid_opening = true;
    }
    else {
        const char *gridnames = pj_param(P->ctx, P->params, "sgrids").s;
        gMutex.lock();
        const bool isKnownGrid = gKnownGrids.find(gridnames) != gKnownGrids.end();
        gMutex.unlock();

        if( isKnownGrid ) {
            Q->defer_grid_opening = true;
        }
        else {
            /* Build gridlist. P->vgridlist_geoid can be empty if +grids only ask for optional grids. */
            Q->grids = pj_vgrid_init(P, "grids");

            /* Was gridlist compiled properly? */
            if ( proj_errno(P) ) {
                proj_log_error(P, _("could not find required grid(s)."));
                return destructor(P, PROJ_ERR_INVALID_OP_FILE_NOT_FOUND_OR_INVALID);
            }

            gMutex.lock();
            gKnownGrids.insert(gridnames);
            gMutex.unlock();
        }
    }

    P->fwd4d = forward_4d;
    P->inv4d = reverse_4d;
    P->fwd3d  = forward_3d;
    P->inv3d  = reverse_3d;
    P->fwd    = nullptr;
    P->inv    = nullptr;

    P->left  = PJ_IO_UNITS_RADIANS;
    P->right = PJ_IO_UNITS_RADIANS;

    return P;
}

void pj_clear_vgridshift_knowngrids_cache() {
    NS_PROJ::lock_guard<NS_PROJ::mutex> lock(gMutex);
    gKnownGrids.clear();
}
