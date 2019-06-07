/* Copyright 2003-2018, University Corporation for Atmospheric
 * Research. See the COPYRIGHT file for copying and redistribution
 * conditions. */
/**
 * @file
 * @internal This file is part of netcdf-4, a netCDF-like interface
 * for HDF5, or a HDF5 backend for netCDF, depending on your point of
 * view.
 *
 * This file handles the nc4 dimension functions.
 *
 * @author Ed Hartnett
 */

#include "nc4internal.h"
#include "nc4dispatch.h"

/**
 * @internal Netcdf-4 files might have more than one unlimited
 * dimension, but return the first one anyway.
 *
 * @note that this code is inconsistent with nc_inq
 *
 * @param ncid File and group ID.
 * @param unlimdimidp Pointer that gets ID of first unlimited
 * dimension, or -1.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @author Ed Hartnett
 */
int
NC4_inq_unlimdim(int ncid, int *unlimdimidp)
{
    NC_GRP_INFO_T *grp, *g;
    NC_FILE_INFO_T *h5;
    NC_DIM_INFO_T *dim;
    int found = 0;
    int retval;
    int i;

    LOG((2, "%s: called", __func__));

    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;
    assert(h5 && grp);

    if (unlimdimidp)
    {
        /* According to netcdf-3 manual, return -1 if there is no unlimited
           dimension. */
        *unlimdimidp = -1;
        for (g = grp; g && !found; g = g->parent)
        {
            for(i=0;i<ncindexsize(grp->dim);i++)
            {
                dim = (NC_DIM_INFO_T*)ncindexith(grp->dim,i);
                if(dim == NULL) continue;
                if (dim->unlimited)
                {
                    *unlimdimidp = dim->hdr.id;
                    found++;
                    break;
                }
            }
        }
    }

    return NC_NOERR;
}

/**
 * @internal Given dim name, find its id.
 *
 * @param ncid File and group ID.
 * @param name Name of the dimension to find.
 * @param idp Pointer that gets dimension ID.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EBADDIM Dimension not found.
 * @return ::NC_EINVAL Invalid input. Name must be provided.
 * @author Ed Hartnett
 */
int
NC4_inq_dimid(int ncid, const char *name, int *idp)
{
    NC *nc;
    NC_GRP_INFO_T *grp, *g;
    NC_FILE_INFO_T *h5;
    NC_DIM_INFO_T *dim;
    char norm_name[NC_MAX_NAME + 1];
    int retval;
    int found;

    LOG((2, "%s: ncid 0x%x name %s", __func__, ncid, name));

    /* Check input. */
    if (!name)
        return NC_EINVAL;

    /* Find metadata for this file. */
    if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
        return retval;
    assert(h5 && nc && grp);

    /* Normalize name. */
    if ((retval = nc4_normalize_name(name, norm_name)))
        return retval;

    /* check for a name match in this group and its parents */
    found = 0;
    for (g = grp; g ; g = g->parent) {
        dim = (NC_DIM_INFO_T*)ncindexlookup(g->dim,norm_name);
        if(dim != NULL) {found = 1; break;}
    }
    if(!found)
        return NC_EBADDIM;
    assert(dim != NULL);
    if (idp)
        *idp = dim->hdr.id;
    return NC_NOERR;
}

/**
 * @internal Returns an array of unlimited dimension ids.The user can
 * get the number of unlimited dimensions by first calling this with
 * NULL for the second pointer.
 *
 * @param ncid File and group ID.
 * @param nunlimdimsp Pointer that gets the number of unlimited
 * dimensions. Ignored if NULL.
 * @param unlimdimidsp Pointer that gets arrray of unlimited dimension
 * ID. Ignored if NULL.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @author Ed Hartnett, Dennis Heimbigner
 */
int
NC4_inq_unlimdims(int ncid, int *nunlimdimsp, int *unlimdimidsp)
{
    NC_DIM_INFO_T *dim;
    NC_GRP_INFO_T *grp;
    NC *nc;
    NC_FILE_INFO_T *h5;
    int num_unlim = 0;
    int retval;
    int i;

    LOG((2, "%s: ncid 0x%x", __func__, ncid));

    /* Find info for this file and group, and set pointer to each. */
    if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
        return retval;
    assert(h5 && nc && grp);

    /* Get our dim info. */
    assert(h5);
    {
        for(i=0;i<ncindexsize(grp->dim);i++)
        {
            dim = (NC_DIM_INFO_T*)ncindexith(grp->dim,i);
            if(dim == NULL) continue;
            if (dim->unlimited)
            {
                if (unlimdimidsp)
                    unlimdimidsp[num_unlim] = dim->hdr.id;
                num_unlim++;
            }
        }
    }

    /* Give the number if the user wants it. */
    if (nunlimdimsp)
        *nunlimdimsp = num_unlim;

    return NC_NOERR;
}
