/* Copyright 2003-2018, University Corporation for Atmospheric
 * Research. See COPYRIGHT file for copying and redistribution
 * conditions. */
/**
 * @file
 * @internal This file contains functions that are used in file
 * opens.
 *
 * @author Ed Hartnett
 */

#include "config.h"
#include "hdf5internal.h"
#include "ncrc.h"
#include "ncmodel.h"

#ifdef ENABLE_BYTERANGE
#include "H5FDhttp.h"
#endif

#define NUM_TYPES 12 /**< Number of netCDF atomic types. */
#define CD_NELEMS_ZLIB 1 /**< Number of parameters needed for ZLIB filter. */

/** @internal Native HDF5 constants for atomic types. For performance,
 * fill this array only the first time, and keep it in global memory
 * for each further use. */
static hid_t h5_native_type_constant_g[NUM_TYPES];

/** @internal NetCDF atomic type names. */
static const char nc_type_name_g[NUM_TYPES][NC_MAX_NAME + 1] = {"char", "byte", "short",
                                                                "int", "float", "double", "ubyte",
                                                                "ushort", "uint", "int64",
                                                                "uint64", "string"};

/** @internal NetCDF atomic types. */
static const nc_type nc_type_constant_g[NUM_TYPES] = {NC_CHAR, NC_BYTE, NC_SHORT,
                                                      NC_INT, NC_FLOAT, NC_DOUBLE, NC_UBYTE,
                                                      NC_USHORT, NC_UINT, NC_INT64,
                                                      NC_UINT64, NC_STRING};

/** @internal NetCDF atomic type sizes. */
static const int nc_type_size_g[NUM_TYPES] = {sizeof(char), sizeof(char), sizeof(short),
                                              sizeof(int), sizeof(float), sizeof(double), sizeof(unsigned char),
                                              sizeof(unsigned short), sizeof(unsigned int), sizeof(long long),
                                              sizeof(unsigned long long), sizeof(char *)};

/** @internal These flags may not be set for open mode. */
static const int ILLEGAL_OPEN_FLAGS = (NC_MMAP);

/* From libsrc4, these are the netcdf-4 cache sizes. */
extern size_t nc4_chunk_cache_size;
extern size_t nc4_chunk_cache_nelems;
extern float nc4_chunk_cache_preemption;

/* From nc4mem.c */
extern int NC4_open_image_file(NC_FILE_INFO_T* h5);

/* Defined later in this file. */
static int rec_read_metadata(NC_GRP_INFO_T *grp);

/**
 * @internal Struct to track HDF5 object info, for
 * rec_read_metadata(). We get this info for every object in the
 * HDF5 file when we H5Literate() over the file. */
typedef struct hdf5_obj_info
{
    hid_t oid;                          /* HDF5 object ID */
    char oname[NC_MAX_NAME + 1];        /* Name of object */
    H5G_stat_t statbuf;                 /* Information about the object */
    struct hdf5_obj_info *next; /* Pointer to next node in list */
} hdf5_obj_info_t;

/**
 * @internal User data struct for call to H5Literate() in
 * rec_read_metadata(). When iterating through the objects in a
 * group, if we find child groups, we save their hdf5_obj_info_t
 * object in a list. Then we processes them after completely
 * processing the parent group. */
typedef struct user_data
{
    NClist *grps; /* NClist<hdf5_obj_info_t*> */
    NC_GRP_INFO_T *grp; /* Pointer to parent group */
} user_data_t;

/* Custom iteration callback data */
typedef struct {
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
} att_iter_info;

/**
 * @internal Given an HDF5 type, set a pointer to netcdf type_info
 * struct, either an existing one (for user-defined types) or a newly
 * created one.
 *
 * @param h5 Pointer to HDF5 file info struct.
 * @param datasetid HDF5 dataset ID.
 * @param type_info Pointer to pointer that gets type info struct.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EHDFERR HDF5 returned error.
 * @return ::NC_EBADTYPID Type not found.
 * @author Ed Hartnett
 */
static int
get_type_info2(NC_FILE_INFO_T *h5, hid_t datasetid, NC_TYPE_INFO_T **type_info)
{
    NC_HDF5_TYPE_INFO_T *hdf5_type;
    htri_t is_str, equal = 0;
    H5T_class_t class;
    hid_t native_typeid, hdf_typeid;
    H5T_order_t order;
    int t;

    assert(h5 && type_info);

    /* Because these N5T_NATIVE_* constants are actually function calls
     * (!) in H5Tpublic.h, I can't initialize this array in the usual
     * way, because at least some C compilers (like Irix) complain
     * about calling functions when defining constants. So I have to do
     * it like this. Note that there's no native types for char or
     * string. Those are handled later. */
    if (!h5_native_type_constant_g[1])
    {
        h5_native_type_constant_g[1] = H5T_NATIVE_SCHAR;
        h5_native_type_constant_g[2] = H5T_NATIVE_SHORT;
        h5_native_type_constant_g[3] = H5T_NATIVE_INT;
        h5_native_type_constant_g[4] = H5T_NATIVE_FLOAT;
        h5_native_type_constant_g[5] = H5T_NATIVE_DOUBLE;
        h5_native_type_constant_g[6] = H5T_NATIVE_UCHAR;
        h5_native_type_constant_g[7] = H5T_NATIVE_USHORT;
        h5_native_type_constant_g[8] = H5T_NATIVE_UINT;
        h5_native_type_constant_g[9] = H5T_NATIVE_LLONG;
        h5_native_type_constant_g[10] = H5T_NATIVE_ULLONG;
    }

    /* Get the HDF5 typeid - we'll need it later. */
    if ((hdf_typeid = H5Dget_type(datasetid)) < 0)
        return NC_EHDFERR;

    /* Get the native typeid. Will be equivalent to hdf_typeid when
     * creating but not necessarily when reading, a variable. */
    if ((native_typeid = H5Tget_native_type(hdf_typeid, H5T_DIR_DEFAULT)) < 0)
        return NC_EHDFERR;

    /* Is this type an integer, string, compound, or what? */
    if ((class = H5Tget_class(native_typeid)) < 0)
        return NC_EHDFERR;

    /* Is this an atomic type? */
    if (class == H5T_STRING || class == H5T_INTEGER || class == H5T_FLOAT)
    {
        /* Allocate a phony NC_TYPE_INFO_T struct to hold type info. */
        if (!(*type_info = calloc(1, sizeof(NC_TYPE_INFO_T))))
            return NC_ENOMEM;

        /* Allocate storage for HDF5-specific type info. */
        if (!(hdf5_type = calloc(1, sizeof(NC_HDF5_TYPE_INFO_T))))
            return NC_ENOMEM;
        (*type_info)->format_type_info = hdf5_type;

        /* H5Tequal doesn't work with H5T_C_S1 for some reason. But
         * H5Tget_class will return H5T_STRING if this is a string. */
        if (class == H5T_STRING)
        {
            if ((is_str = H5Tis_variable_str(native_typeid)) < 0)
                return NC_EHDFERR;
            /* Make sure fixed-len strings will work like variable-len
             * strings */
            if (is_str || H5Tget_size(hdf_typeid) > 1)
            {
                /* Set a class for the type */
                t = NUM_TYPES - 1;
                (*type_info)->nc_type_class = NC_STRING;
            }
            else
            {
                /* Set a class for the type */
                t = 0;
                (*type_info)->nc_type_class = NC_CHAR;
            }
        }
        else
        {
            for (t = 1; t < NUM_TYPES - 1; t++)
            {
                if ((equal = H5Tequal(native_typeid,
                                      h5_native_type_constant_g[t])) < 0)
                    return NC_EHDFERR;
                if (equal)
                    break;
            }

            /* Find out about endianness. As of HDF 1.8.6, this works
             * with all data types Not just H5T_INTEGER. See
             * https://www.hdfgroup.org/HDF5/doc/RM/RM_H5T.html#Datatype-GetOrder */
            if ((order = H5Tget_order(hdf_typeid)) < 0)
                return NC_EHDFERR;

            if (order == H5T_ORDER_LE)
                (*type_info)->endianness = NC_ENDIAN_LITTLE;
            else if (order == H5T_ORDER_BE)
                (*type_info)->endianness = NC_ENDIAN_BIG;
            else
                return NC_EBADTYPE;

            if (class == H5T_INTEGER)
                (*type_info)->nc_type_class = NC_INT;
            else
                (*type_info)->nc_type_class = NC_FLOAT;
        }
        (*type_info)->hdr.id = nc_type_constant_g[t];
        (*type_info)->size = nc_type_size_g[t];
        if (!((*type_info)->hdr.name = strdup(nc_type_name_g[t])))
            return NC_ENOMEM;
        hdf5_type->hdf_typeid = hdf_typeid;
        hdf5_type->native_hdf_typeid = native_typeid;
        return NC_NOERR;
    }
    else
    {
        NC_TYPE_INFO_T *type;

        /* This is a user-defined type. */
        if((type = nc4_rec_find_hdf_type(h5, native_typeid)))
            *type_info = type;

        /* The type entry in the array of user-defined types already has
         * an open data typeid (and native typeid), so close the ones we
         * opened above. */
        if (H5Tclose(native_typeid) < 0)
            return NC_EHDFERR;
        if (H5Tclose(hdf_typeid) < 0)
            return NC_EHDFERR;

        if (type)
            return NC_NOERR;
    }

    return NC_EBADTYPID;
}

/**
 * @internal This function reads the coordinates attribute used for
 * multi-dimensional coordinates. It then sets var->dimids[], and
 * attempts to find a pointer to the dims and sets var->dim[] as well.
 *
 * @param grp Group info pointer.
 * @param var Var info pointer.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOTATT Attribute does not exist.
 * @return ::NC_EATTMETA Attribute metadata error.
 * @return ::NC_EHDFERR HDF5 error.
 * @author Ed Hartnett
 */
static int
read_coord_dimids(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *var)
{
    NC_HDF5_VAR_INFO_T *hdf5_var;
    hid_t coord_att_typeid = -1, coord_attid = -1, spaceid = -1;
    hssize_t npoints;
    htri_t attr_exists;
    int d;
    int retval = NC_NOERR;

    assert(grp && var && var->format_var_info);
    LOG((3, "%s: var->hdr.name %s", __func__, var->hdr.name));

    /* Have we already read the coordinates hidden att for this var? */
    if (var->coords_read)
        return NC_NOERR;

    /* Get HDF5-sepecific var info. */
    hdf5_var = (NC_HDF5_VAR_INFO_T *)var->format_var_info;

    /* Does the COORDINATES att exist? */
    if ((attr_exists = H5Aexists(hdf5_var->hdf_datasetid, COORDINATES)) < 0)
        return NC_EHDFERR;
    if (!attr_exists)
        return NC_ENOTATT;

    /* There is a hidden attribute telling us the ids of the
     * dimensions that apply to this multi-dimensional coordinate
     * variable. Read it. */
    if ((coord_attid = H5Aopen_name(hdf5_var->hdf_datasetid, COORDINATES)) < 0)
        BAIL(NC_EATTMETA);

    if ((coord_att_typeid = H5Aget_type(coord_attid)) < 0)
        BAIL(NC_EATTMETA);

    /* How many dimensions are there? */
    if ((spaceid = H5Aget_space(coord_attid)) < 0)
        BAIL(NC_EATTMETA);
    if ((npoints = H5Sget_simple_extent_npoints(spaceid)) < 0)
        BAIL(NC_EATTMETA);

    /* Check that the number of points is the same as the number of
     * dimensions for the variable. */
    if (npoints != var->ndims)
        BAIL(NC_EATTMETA);

    /* Read the dimids for this var. */
    if (H5Aread(coord_attid, coord_att_typeid, var->dimids) < 0)
        BAIL(NC_EATTMETA);
    LOG((4, "read dimids for this var"));

    /* Update var->dim field based on the var->dimids. Ok if does not
     * find a dim at this time, but if found set it. */
    for (d = 0; d < var->ndims; d++)
        nc4_find_dim(grp, var->dimids[d], &var->dim[d], NULL);

    /* Remember that we have read the coordinates hidden attribute. */
    var->coords_read = NC_TRUE;

exit:
    if (spaceid >= 0 && H5Sclose(spaceid) < 0)
        BAIL2(NC_EHDFERR);
    if (coord_att_typeid >= 0 && H5Tclose(coord_att_typeid) < 0)
        BAIL2(NC_EHDFERR);
    if (coord_attid >= 0 && H5Aclose(coord_attid) < 0)
        BAIL2(NC_EHDFERR);
    return retval;
}

/**
 * @internal This function is called when reading a file's metadata
 * for each dimension scale attached to a variable.
 *
 * @param did HDF5 ID for dimscale.
 * @param dim
 * @param dsid
 * @param dimscale_hdf5_objids
 *
 * @return 0 for success, -1 for error.
 * @author Ed Hartnett
 */
static herr_t
dimscale_visitor(hid_t did, unsigned dim, hid_t dsid,
                 void *dimscale_hdf5_objids)
{
    H5G_stat_t statbuf;

    LOG((4, "%s", __func__));

    /* Get more info on the dimscale object.*/
    if (H5Gget_objinfo(dsid, ".", 1, &statbuf) < 0)
        return -1;

    /* Pass this information back to caller. */
    (*(HDF5_OBJID_T *)dimscale_hdf5_objids).fileno[0] = statbuf.fileno[0];
    (*(HDF5_OBJID_T *)dimscale_hdf5_objids).fileno[1] = statbuf.fileno[1];
    (*(HDF5_OBJID_T *)dimscale_hdf5_objids).objno[0] = statbuf.objno[0];
    (*(HDF5_OBJID_T *)dimscale_hdf5_objids).objno[1] = statbuf.objno[1];
    return 0;
}

/**
 * @internal For files without any netCDF-4 dimensions defined, create
 * phony dimension to match the available datasets. Each new dimension
 * of a new size gets a phony dimension. However, if a var has more
 * than one dimension defined, and they are the same size, they each
 * get their own phony dimension (starting in netcdf-c-4.7.3).
 *
 * @param grp Pointer to the group info.
 * @param hdf_datasetid HDF5 datsetid for the var's dataset.
 * @param var Pointer to the var info.
 *
 * @returns NC_NOERR No error.
 * @returns NC_EHDFERR HDF5 returned an error.
 * @returns NC_ENOMEM Out of memory.
 * @author Ed Hartnett
 */
static int
create_phony_dims(NC_GRP_INFO_T *grp, hid_t hdf_datasetid, NC_VAR_INFO_T *var)
{
    NC_DIM_INFO_T *dim;
    hid_t spaceid = 0;
    hsize_t *h5dimlen = NULL, *h5dimlenmax = NULL;
    int dataset_ndims;
    int d;
    int retval = NC_NOERR;

    /* Find the space information for this dimension. */
    if ((spaceid = H5Dget_space(hdf_datasetid)) < 0)
        BAIL(NC_EHDFERR);

    /* Get the len of each dim in the space. */
    if (var->ndims)
    {
        /* Allocate storage for dim lens and max lens for this var. */
        if (!(h5dimlen = malloc(var->ndims * sizeof(hsize_t))))
            return NC_ENOMEM;
        if (!(h5dimlenmax = malloc(var->ndims * sizeof(hsize_t))))
            BAIL(NC_ENOMEM);

        /* Get ndims, also len and mac len of all dims. */
        if ((dataset_ndims = H5Sget_simple_extent_dims(spaceid, h5dimlen,
                                                       h5dimlenmax)) < 0)
            BAIL(NC_EHDFERR);
        assert(dataset_ndims == var->ndims);
    }
    else
    {
        /* Make sure it's scalar. */
        assert(H5Sget_simple_extent_type(spaceid) == H5S_SCALAR);
    }

    /* Create a phony dimension for each dimension in the dataset,
     * unless there already is one the correct size. */
    for (d = 0; d < var->ndims; d++)
    {
        int k;
        int match = 0;

        /* Is there already a phony dimension of the correct size? */
        for (k = 0; k < ncindexsize(grp->dim); k++)
        {
            dim = (NC_DIM_INFO_T *)ncindexith(grp->dim, k);
            assert(dim);
            if ((dim->len == h5dimlen[d]) &&
                ((h5dimlenmax[d] == H5S_UNLIMITED && dim->unlimited) ||
                 (h5dimlenmax[d] != H5S_UNLIMITED && !dim->unlimited)))
            {
                int k1;

                /* We found a match! */
                match++;

                /* If this phony dimension has already in use for this
                 * var, we should not use it again. */
                for (k1 = 0; k1 < d; k1++)
                    if (var->dimids[k1] == dim->hdr.id)
                        match = 0;

                if (match)
                    break;
            }
        }

        /* Didn't find a phony dim? Then create one. */
        if (!match)
        {
            char phony_dim_name[NC_MAX_NAME + 1];
            sprintf(phony_dim_name, "phony_dim_%d", grp->nc4_info->next_dimid);
            LOG((3, "%s: creating phony dim for var %s", __func__, var->hdr.name));

            /* Add phony dim to metadata list. */
            if ((retval = nc4_dim_list_add(grp, phony_dim_name, h5dimlen[d], -1, &dim)))
                BAIL(retval);

            /* Create struct for HDF5-specific dim info. */
            if (!(dim->format_dim_info = calloc(1, sizeof(NC_HDF5_DIM_INFO_T))))
                BAIL(NC_ENOMEM);
            if (h5dimlenmax[d] == H5S_UNLIMITED)
                dim->unlimited = NC_TRUE;
        }

        /* The variable must remember the dimid. */
        var->dimids[d] = dim->hdr.id;
        var->dim[d] = dim;
    } /* next dim */

exit:
    /* Free resources. */
    if (spaceid > 0 && H5Sclose(spaceid) < 0)
        BAIL2(NC_EHDFERR);
    if (h5dimlenmax)
        free(h5dimlenmax);
    if (h5dimlen)
        free(h5dimlen);

    return retval;
}

/**
 * @internal Iterate through the vars in this file and make sure we've
 * got a dimid and a pointer to a dim for each dimension. This may
 * already have been done using the COORDINATES hidden attribute, in
 * which case this function will not have to do anything. This is
 * desirable because recurdively matching the dimscales (when
 * necessary) is very much the slowest part of opening a file.
 *
 * @param grp Pointer to group info struct.
 *
 * @returns NC_NOERR No error.
 * @returns NC_EHDFERR HDF5 returned an error.
 * @returns NC_ENOMEM Out of memory.
 * @author Ed Hartnett
 */
static int
rec_match_dimscales(NC_GRP_INFO_T *grp)
{
    NC_VAR_INFO_T *var;
    NC_DIM_INFO_T *dim;
    int retval = NC_NOERR;
    int i;

    assert(grp && grp->hdr.name);
    LOG((4, "%s: grp->hdr.name %s", __func__, grp->hdr.name));

    /* Perform var dimscale match for child groups. */
    for (i = 0; i < ncindexsize(grp->children); i++)
        if ((retval = rec_match_dimscales((NC_GRP_INFO_T *)ncindexith(grp->children, i))))
            return retval;

    /* Check all the vars in this group. If they have dimscale info,
     * try and find a dimension for them. */
    for (i = 0; i < ncindexsize(grp->vars); i++)
    {
        NC_HDF5_VAR_INFO_T *hdf5_var;
        int d;

        /* Get pointer to var and to the HDF5-specific var info. */
        var = (NC_VAR_INFO_T *)ncindexith(grp->vars, i);
        assert(var && var->format_var_info);
        hdf5_var = (NC_HDF5_VAR_INFO_T *)var->format_var_info;

        /* Check all vars and see if dim[i] != NULL if dimids[i]
         * valid. Recall that dimids were initialized to -1. */
        for (d = 0; d < var->ndims; d++)
        {
            if (!var->dim[d])
                nc4_find_dim(grp, var->dimids[d], &var->dim[d], NULL);
        }

        /* Skip dimension scale variables */
        if (var->dimscale)
            continue;

        /* If we have already read hidden coordinates att, then we don't
         * have to match dimscales for this var. */
        if (var->coords_read)
            continue;

        /* Skip dimension scale variables */
        if (!var->dimscale)
        {
            int d;
            int j;

            /* Are there dimscales for this variable? */
            if (hdf5_var->dimscale_hdf5_objids)
            {
                for (d = 0; d < var->ndims; d++)
                {
                    NC_GRP_INFO_T *g;
                    nc_bool_t finished = NC_FALSE;
                    LOG((5, "%s: var %s has dimscale info...", __func__, var->hdr.name));

                    /* If we already have the dimension, we don't need to
                     * match the dimscales. This is better because matching
                     * the dimscales is slow. */
                    if (var->dim[d])
                        continue;

                    /* Now we have to try to match dimscales. Check this
                     * and parent groups. */
                    for (g = grp; g && !finished; g = g->parent)
                    {
                        /* Check all dims in this group. */
                        for (j = 0; j < ncindexsize(g->dim); j++)
                        {
                            /* Get the HDF5 specific dim info. */
                            NC_HDF5_DIM_INFO_T *hdf5_dim;
                            dim = (NC_DIM_INFO_T *)ncindexith(g->dim, j);
                            assert(dim && dim->format_dim_info);
                            hdf5_dim = (NC_HDF5_DIM_INFO_T *)dim->format_dim_info;

                            /* Check for exact match of fileno/objid arrays
                             * to find identical objects in HDF5 file. */
                            if (hdf5_var->dimscale_hdf5_objids[d].fileno[0] == hdf5_dim->hdf5_objid.fileno[0] &&
                                hdf5_var->dimscale_hdf5_objids[d].objno[0] == hdf5_dim->hdf5_objid.objno[0] &&
                                hdf5_var->dimscale_hdf5_objids[d].fileno[1] == hdf5_dim->hdf5_objid.fileno[1] &&
                                hdf5_var->dimscale_hdf5_objids[d].objno[1] == hdf5_dim->hdf5_objid.objno[1])
                            {
                                LOG((4, "%s: for dimension %d, found dim %s", __func__,
                                     d, dim->hdr.name));
                                var->dimids[d] = dim->hdr.id;
                                var->dim[d] = dim;
                                finished = NC_TRUE;
                                break;
                            }
                        } /* next dim */
                    } /* next grp */
                } /* next var->dim */
            }
            else
            {
                /* No dimscales for this var! Invent phony dimensions. */
                if ((retval = create_phony_dims(grp, hdf5_var->hdf_datasetid, var)))
                    return retval;
            }
        }
    }

    return retval;
}

/**
 * @internal Check for the attribute that indicates that netcdf
 * classic model is in use.
 *
 * @param root_grp pointer to the group info for the root group of the
 * @param is_classic store 1 if this is a classic file.
 * file.
 *
 * @return NC_NOERR No error.
 * @author Ed Hartnett
 */
static int
check_for_classic_model(NC_GRP_INFO_T *root_grp, int *is_classic)
{
    htri_t attr_exists;
    hid_t grpid;

    /* Check inputs. */
    assert(root_grp && root_grp->format_grp_info && !root_grp->parent
           && is_classic);

    /* Get the HDF5 group id. */
    grpid = ((NC_HDF5_GRP_INFO_T *)(root_grp->format_grp_info))->hdf_grpid;

    /* If this attribute exists in the root group, then classic model
     * is in effect. */
    if ((attr_exists = H5Aexists(grpid, NC3_STRICT_ATT_NAME)) < 0)
        return NC_EHDFERR;
    *is_classic = attr_exists ? 1 : 0;

    return NC_NOERR;
}

/**
 * @internal Open a netcdf-4 file. Things have already been kicked off
 * in ncfunc.c in nc_open, but here the netCDF-4 part of opening a
 * file is handled.
 *
 * @param path The file name of the new file.
 * @param mode The open mode flag.
 * @param parameters File parameters.
 * @param ncid The ncid that has been assigned to this file.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 * @return ::NC_EINTERNAL Internal list error.
 * @return ::NC_EHDFERR HDF error.
 * @return ::NC_EMPI MPI error for parallel.
 * @return ::NC_EPARINIT Parallel I/O initialization error.
 * @return ::NC_EINMEMMORY Memory file error.
 * @author Ed Hartnett, Dennis Heimbigner
 */
static int
nc4_open_file(const char *path, int mode, void* parameters, int ncid)
{
    NC_FILE_INFO_T *nc4_info = NULL;
    NC_HDF5_FILE_INFO_T *h5 = NULL;
    NC *nc;
    hid_t fapl_id = H5P_DEFAULT;
    unsigned flags;
    int is_classic;
#ifdef USE_PARALLEL4
    NC_MPI_INFO *mpiinfo = NULL;
    int comm_duped = 0; /* Whether the MPI Communicator was duplicated */
    int info_duped = 0; /* Whether the MPI Info object was duplicated */
#endif
    int retval;

    LOG((3, "%s: path %s mode %d", __func__, path, mode));
    assert(path);

    /* Find pointer to NC. */
    if ((retval = NC_check_id(ncid, &nc)))
        return retval;
    assert(nc);

    /* Determine the HDF5 open flag to use. */
    flags = (mode & NC_WRITE) ? H5F_ACC_RDWR : H5F_ACC_RDONLY;

    /* Add necessary structs to hold netcdf-4 file data. */
    if ((retval = nc4_nc4f_list_add(nc, path, mode)))
        BAIL(retval);
    nc4_info = (NC_FILE_INFO_T *)nc->dispatchdata;
    assert(nc4_info && nc4_info->root_grp);

    /* Add struct to hold HDF5-specific file metadata. */
    if (!(nc4_info->format_file_info = calloc(1, sizeof(NC_HDF5_FILE_INFO_T))))
        BAIL(NC_ENOMEM);

    /* Add struct to hold HDF5-specific group info. */
    if (!(nc4_info->root_grp->format_grp_info = calloc(1, sizeof(NC_HDF5_GRP_INFO_T))))
        BAIL(NC_ENOMEM);

    h5 = (NC_HDF5_FILE_INFO_T*)nc4_info->format_file_info;

#ifdef ENABLE_BYTERANGE
    /* See if we want the byte range protocol */
    if(NC_testmode(path,"bytes")) {
        h5->http.iosp = 1;
        /* Kill off any conflicting modes flags */
        mode &= ~(NC_WRITE|NC_DISKLESS|NC_PERSIST|NC_INMEMORY);
        parameters = NULL; /* kill off parallel */
    } else
        h5->http.iosp = 0;
#endif /*ENABLE_BYTERANGE*/

    nc4_info->mem.inmemory = ((mode & NC_INMEMORY) == NC_INMEMORY);
    nc4_info->mem.diskless = ((mode & NC_DISKLESS) == NC_DISKLESS);
    nc4_info->mem.persist = ((mode & NC_PERSIST) == NC_PERSIST);

    /* Does the mode specify that this file is read-only? */
    if ((mode & NC_WRITE) == 0)
        nc4_info->no_write = NC_TRUE;

    if(nc4_info->mem.inmemory && nc4_info->mem.diskless)
        BAIL(NC_EINTERNAL);

#ifdef USE_PARALLEL4
    mpiinfo = (NC_MPI_INFO*)parameters; /* assume, may be changed if inmemory is true */
#endif /* !USE_PARALLEL4 */

    /* Need this access plist to control how HDF5 handles open objects
     * on file close. (Setting H5F_CLOSE_SEMI will cause H5Fclose to
     * fail if there are any open objects in the file). */
    if ((fapl_id = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        BAIL(NC_EHDFERR);

    if (H5Pset_fclose_degree(fapl_id, H5F_CLOSE_SEMI) < 0)
        BAIL(NC_EHDFERR);

#ifdef USE_PARALLEL4
    if (!(mode & (NC_INMEMORY | NC_DISKLESS)) && mpiinfo != NULL) {
        /* If this is a parallel file create, set up the file creation
         * property list.
         */
        nc4_info->parallel = NC_TRUE;
        LOG((4, "opening parallel file with MPI/IO"));
        if (H5Pset_fapl_mpio(fapl_id, mpiinfo->comm, mpiinfo->info) < 0)
            BAIL(NC_EPARINIT);

        /* Keep copies of the MPI Comm & Info objects */
        if (MPI_SUCCESS != MPI_Comm_dup(mpiinfo->comm, &nc4_info->comm))
            BAIL(NC_EMPI);
        comm_duped++;
        if (MPI_INFO_NULL != mpiinfo->info)
        {
            if (MPI_SUCCESS != MPI_Info_dup(mpiinfo->info, &nc4_info->info))
                BAIL(NC_EMPI);
            info_duped++;
        }
        else
        {
            /* No dup, just copy it. */
            nc4_info->info = mpiinfo->info;
        }
    }

#ifdef HDF5_HAS_COLL_METADATA_OPS
    if (H5Pset_all_coll_metadata_ops(fapl_id, 1) < 0)
        BAIL(NC_EPARINIT);
#endif

#else /* only set cache for non-parallel. */
    if (H5Pset_cache(fapl_id, 0, nc4_chunk_cache_nelems, nc4_chunk_cache_size,
                     nc4_chunk_cache_preemption) < 0)
        BAIL(NC_EHDFERR);
    LOG((4, "%s: set HDF raw chunk cache to size %d nelems %d preemption %f",
         __func__, nc4_chunk_cache_size, nc4_chunk_cache_nelems,
         nc4_chunk_cache_preemption));
#endif /* USE_PARALLEL4 */

    /* Process  NC_INMEMORY */
    if(nc4_info->mem.inmemory) {
        NC_memio* memio;
        /* validate */
        if(parameters == NULL)
            BAIL(NC_EINMEMORY);
        memio = (NC_memio*)parameters;
        if(memio->memory == NULL || memio->size == 0)
            BAIL(NC_EINMEMORY);
        /* initialize h5->mem */
        nc4_info->mem.memio = *memio;
        /* Is the incoming memory locked? */
        nc4_info->mem.locked = (nc4_info->mem.memio.flags & NC_MEMIO_LOCKED) == NC_MEMIO_LOCKED;
        /* As a safeguard, if not locked and not read-only,
           then we must take control of the incoming memory */
        if(!nc4_info->mem.locked && !nc4_info->no_write) {
            memio->memory = NULL; /* take control */
            memio->size = 0;
        }
        retval = NC4_open_image_file(nc4_info);
        if(retval)
            BAIL(NC_EHDFERR);
    }
    else
        if(nc4_info->mem.diskless) {   /* Process  NC_DISKLESS */
            size_t min_incr = 65536; /* Minimum buffer increment */
            /* Configure FAPL to use the core file driver */
            if (H5Pset_fapl_core(fapl_id, min_incr, (nc4_info->mem.persist?1:0)) < 0)
                BAIL(NC_EHDFERR);
            /* Open the HDF5 file. */
            if ((h5->hdfid = H5Fopen(path, flags, fapl_id)) < 0)
                BAIL(NC_EHDFERR);
        }
#ifdef ENABLE_BYTERANGE
        else
            if(h5->http.iosp) {   /* Arrange to use the byte-range driver */
                /* Configure FAPL to use the byte-range file driver */
                if (H5Pset_fapl_http(fapl_id) < 0)
                    BAIL(NC_EHDFERR);
                /* Open the HDF5 file. */
                if ((h5->hdfid = H5Fopen(path, flags, fapl_id)) < 0)
                    BAIL(NC_EHDFERR);
            }
#endif
            else
            {
                /* Open the HDF5 file. */
                if ((h5->hdfid = H5Fopen(path, flags, fapl_id)) < 0)
                    BAIL(NC_EHDFERR);
            }

    /* Now read in all the metadata. Some types and dimscale
     * information may be difficult to resolve here, if, for example, a
     * dataset of user-defined type is encountered before the
     * definition of that type. */
    if ((retval = rec_read_metadata(nc4_info->root_grp)))
        BAIL(retval);

    /* Check for classic model attribute. */
    if ((retval = check_for_classic_model(nc4_info->root_grp, &is_classic)))
        BAIL(retval);
    if (is_classic)
        nc4_info->cmode |= NC_CLASSIC_MODEL;

    /* Set the provenance info for this file */
    if ((retval = NC4_read_provenance(nc4_info)))
        BAIL(retval);

    /* Now figure out which netCDF dims are indicated by the dimscale
     * information. */
    if ((retval = rec_match_dimscales(nc4_info->root_grp)))
        BAIL(retval);

#ifdef LOGGING
    /* This will print out the names, types, lens, etc of the vars and
       atts in the file, if the logging level is 2 or greater. */
    log_metadata_nc(nc4_info);
#endif

    /* Close the property list. */
    if (H5Pclose(fapl_id) < 0)
        BAIL(NC_EHDFERR);

    return NC_NOERR;

exit:
#ifdef USE_PARALLEL4
    if (comm_duped) MPI_Comm_free(&nc4_info->comm);
    if (info_duped) MPI_Info_free(&nc4_info->info);
#endif

    if (fapl_id > 0 && fapl_id != H5P_DEFAULT)
        H5Pclose(fapl_id);
    if (nc4_info)
        nc4_close_hdf5_file(nc4_info, 1, 0); /*  treat like abort*/
    return retval;
}

/**
 * @internal Open a netCDF-4 file.
 *
 * @param path The file name of the new file.
 * @param mode The open mode flag.
 * @param basepe Ignored by this function.
 * @param chunksizehintp Ignored by this function.
 * @param parameters pointer to struct holding extra data (e.g. for parallel I/O)
 * layer. Ignored if NULL.
 * @param dispatch Pointer to the dispatch table for this file.
 * @param nc_file Pointer to an instance of NC.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EINVAL Invalid inputs.
 * @author Ed Hartnett
 */
int
NC4_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
         void *parameters, const NC_Dispatch *dispatch, int ncid)
{
    assert(path && dispatch);

    LOG((1, "%s: path %s mode %d params %x",
         __func__, path, mode, parameters));

    /* Check the mode for validity */
    if (mode & ILLEGAL_OPEN_FLAGS)
        return NC_EINVAL;

    if((mode & NC_DISKLESS) && (mode & NC_INMEMORY))
        return NC_EINVAL;

    /* If this is our first file, initialize HDF5. */
    if (!nc4_hdf5_initialized)
        nc4_hdf5_initialize();

#ifdef LOGGING
    /* If nc logging level has changed, see if we need to turn on
     * HDF5's error messages. */
    hdf5_set_log_level();
#endif /* LOGGING */

    /* Open the file. */
    return nc4_open_file(path, mode, parameters, ncid);
}

/**
 * @internal Find out what filters are applied to this HDF5 dataset,
 * fletcher32, deflate, and/or shuffle. All other filters are just
 * dumped The possible values of
 *
 * @param propid ID of HDF5 var creation properties list.
 * @param var Pointer to NC_VAR_INFO_T for this variable.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 * @return ::NC_EHDFERR HDF5 returned error.
 * @author Dennis Heimbigner, Ed Hartnett
 */
static int get_filter_info(hid_t propid, NC_VAR_INFO_T *var)
{
    H5Z_filter_t filter;
    int num_filters;
    unsigned int cd_values_zip[CD_NELEMS_ZLIB];
    size_t cd_nelems = CD_NELEMS_ZLIB;
    int f;

    assert(var);

    if ((num_filters = H5Pget_nfilters(propid)) < 0)
        return NC_EHDFERR;

    for (f = 0; f < num_filters; f++)
    {
        if ((filter = H5Pget_filter2(propid, f, NULL, &cd_nelems, cd_values_zip,
                                     0, NULL, NULL)) < 0)
            return NC_EHDFERR;
        switch (filter)
        {
        case H5Z_FILTER_SHUFFLE:
            var->shuffle = NC_TRUE;
            break;

        case H5Z_FILTER_FLETCHER32:
            var->fletcher32 = NC_TRUE;
            break;

        case H5Z_FILTER_DEFLATE:
            var->deflate = NC_TRUE;
            if (cd_nelems != CD_NELEMS_ZLIB ||
                cd_values_zip[0] > NC_MAX_DEFLATE_LEVEL)
                return NC_EHDFERR;
            var->deflate_level = cd_values_zip[0];
            break;

        case H5Z_FILTER_SZIP:
            /* Szip is tricky because the filter code expands the set of parameters from 2 to 4
               and changes some of the parameter values */
            var->filterid = filter;
            if(cd_nelems == 0)
                var->params = NULL;
            else {
                /* We have to re-read the parameters based on actual nparams,
                   which in the case of szip, differs from users original nparams */
                var->params = (unsigned int*)calloc(1,sizeof(unsigned int)*cd_nelems);
                if(var->params == NULL)
                    return NC_ENOMEM;
                if((filter = H5Pget_filter2(propid, f, NULL, &cd_nelems,
                                            var->params, 0, NULL, NULL)) < 0)
                    return NC_EHDFERR;
                /* fix up the parameters and the #params */
                var->nparams = cd_nelems;
            }
            break;

        default:
            var->filterid = filter;
            var->nparams = cd_nelems;
            if(cd_nelems == 0)
                var->params = NULL;
            else {
                /* We have to re-read the parameters based on actual nparams */
                var->params = (unsigned int*)calloc(1,sizeof(unsigned int)*var->nparams);
                if(var->params == NULL)
                    return NC_ENOMEM;
                if((filter = H5Pget_filter2(propid, f, NULL, &cd_nelems,
                                            var->params, 0, NULL, NULL)) < 0)
                    return NC_EHDFERR;
            }
            break;
        }
    }
    return NC_NOERR;
}

/**
 * @internal Learn if there is a fill value defined for a variable,
 * and, if so, its value.
 *
 * @param propid ID of HDF5 var creation properties list.
 * @param var Pointer to NC_VAR_INFO_T for this variable.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 * @return ::NC_EHDFERR HDF5 returned error.
 * @author Dennis Heimbigner, Ed Hartnett
 */
static int get_fill_info(hid_t propid, NC_VAR_INFO_T *var)
{
    H5D_fill_value_t fill_status;

    /* Is there a fill value associated with this dataset? */
    if (H5Pfill_value_defined(propid, &fill_status) < 0)
        return NC_EHDFERR;

    /* Get the fill value, if there is one defined. */
    if (fill_status == H5D_FILL_VALUE_USER_DEFINED)
    {
        /* Allocate space to hold the fill value. */
        if (!var->fill_value)
        {
            if (var->type_info->nc_type_class == NC_VLEN)
            {
                if (!(var->fill_value = malloc(sizeof(nc_vlen_t))))
                    return NC_ENOMEM;
            }
            else if (var->type_info->nc_type_class == NC_STRING)
            {
                if (!(var->fill_value = malloc(sizeof(char *))))
                    return NC_ENOMEM;
            }
            else
            {
                assert(var->type_info->size);
                if (!(var->fill_value = malloc(var->type_info->size)))
                    return NC_ENOMEM;
            }
        }

        /* Get the fill value from the HDF5 property lust. */
        if (H5Pget_fill_value(propid, ((NC_HDF5_TYPE_INFO_T *)var->type_info->format_type_info)->native_hdf_typeid,
                              var->fill_value) < 0)
            return NC_EHDFERR;
    }
    else
        var->no_fill = NC_TRUE;

    return NC_NOERR;
}

/**
 * @internal Learn the chunking settings of a var.
 *
 * @param propid ID of HDF5 var creation properties list.
 * @param var Pointer to NC_VAR_INFO_T for this variable.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 * @return ::NC_EHDFERR HDF5 returned error.
 * @author Dennis Heimbigner, Ed Hartnett
 */
static int get_chunking_info(hid_t propid, NC_VAR_INFO_T *var)
{
    H5D_layout_t layout;
    hsize_t chunksize[H5S_MAX_RANK] = {0};
    int d;

    /* Get the chunking info the var. */
    if ((layout = H5Pget_layout(propid)) < -1)
        return NC_EHDFERR;

    /* Remember the layout and, if chunked, the chunksizes. */
    if (layout == H5D_CHUNKED)
    {
        if (H5Pget_chunk(propid, H5S_MAX_RANK, chunksize) < 0)
            return NC_EHDFERR;
        if (!(var->chunksizes = malloc(var->ndims * sizeof(size_t))))
            return NC_ENOMEM;
        for (d = 0; d < var->ndims; d++)
            var->chunksizes[d] = chunksize[d];
    }
    else if (layout == H5D_CONTIGUOUS || layout == H5D_COMPACT)
        var->contiguous = NC_TRUE;

    return NC_NOERR;
}

/**
 * @internal This function gets info about the dimscales attached to a
 * dataset. The info is used later for dimscale matching.
 *
 * @param var Pointer to var info struct.
 * @param hdf5_var Pointer to HDF5 var info struct.
 * @param ndims Number of dims for this var.
 * @param datasetid HDF5 datasetid.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_ENOMEM Out of memory.
 * @return ::NC_EHDFERR HDF5 returned error.
 * @return ::NC_EVARMETA Error with var metadata.
 * @author Ed Hartnett, Dennis Heimbigner
 */
static int
get_attached_info(NC_VAR_INFO_T *var, NC_HDF5_VAR_INFO_T *hdf5_var, int ndims,
                  hid_t datasetid)
{
    int d;
    int num_scales = 0;

    LOG((4, "%s ndims %d datasetid %ld", __func__, ndims, datasetid));

    /* Find out how many scales are attached to this
     * dataset. H5DSget_num_scales returns an error if there are no
     * scales, so convert a negative return value to zero. */
    num_scales = H5DSget_num_scales(datasetid, 0);
    if (num_scales < 0)
        num_scales = 0;
    LOG((4, "num_scales %d", num_scales));

    /* If an enddef has already been called, the dimscales will already
     * be taken care of. */
    if (num_scales && ndims && !var->dimscale_attached)
    {
        /* Allocate space to remember whether the dimscale has been
         * attached for each dimension, and the HDF5 object IDs of the
         * scale(s). */
        assert(!hdf5_var->dimscale_hdf5_objids);
        if (!(var->dimscale_attached = calloc(ndims, sizeof(nc_bool_t))))
            return NC_ENOMEM;
        if (!(hdf5_var->dimscale_hdf5_objids = malloc(ndims *
                                                      sizeof(struct hdf5_objid))))
            return NC_ENOMEM;

        /* Store id information allowing us to match hdf5 dimscales to
         * netcdf dimensions. */
        for (d = 0; d < var->ndims; d++)
        {
            LOG((4, "about to iterate scales for dim %d", d));
            if (H5DSiterate_scales(hdf5_var->hdf_datasetid, d, NULL, dimscale_visitor,
                                   &(hdf5_var->dimscale_hdf5_objids[d])) < 0)
                return NC_EHDFERR;
            var->dimscale_attached[d] = NC_TRUE;
            LOG((4, "dimscale attached"));
        }
    }

    return NC_NOERR;
}

/**
 * @internal This function reads scale info for vars, whether they
 * are scales or not.
 *
 * @param grp Pointer to group info struct.
 * @param dim Pointer to dim info struct if this is a scale, NULL
 * otherwise.
 * @param var Pointer to var info struct.
 * @param hdf5_var Pointer to HDF5 var info struct.
 * @param ndims Number of dims for this var.
 * @param datasetid HDF5 datasetid.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_ENOMEM Out of memory.
 * @return ::NC_EHDFERR HDF5 returned error.
 * @return ::NC_EVARMETA Error with var metadata.
 * @author Ed Hartnett, Dennis Heimbigner
 */
static int
get_scale_info(NC_GRP_INFO_T *grp, NC_DIM_INFO_T *dim, NC_VAR_INFO_T *var,
               NC_HDF5_VAR_INFO_T *hdf5_var, int ndims, hid_t datasetid)
{
    int retval;

    /* If it's a scale, mark it as such. */
    if (dim)
    {
        assert(ndims);
        var->dimscale = NC_TRUE;

        /* If this is a multi-dimensional coordinate var, then the
         * dimids must be stored in the hidden coordinates attribute. */
        if (var->ndims > 1)
        {
            if ((retval = read_coord_dimids(grp, var)))
                return retval;
        }
        else
        {
            /* This is a 1-dimensional coordinate var. */
            assert(!strcmp(var->hdr.name, dim->hdr.name));
            var->dimids[0] = dim->hdr.id;
            var->dim[0] = dim;
        }
        dim->coord_var = var;
    }
    else /* Not a scale. */
    {
        if (!var->coords_read)
            if ((retval = get_attached_info(var, hdf5_var, ndims, datasetid)))
                return retval;
    }

    return NC_NOERR;
}

/**
 * @internal Get the metadata for a variable.
 *
 * @param var Pointer to var info struct.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_ENOMEM Out of memory.
 * @return ::NC_EHDFERR HDF5 returned error.
 * @return ::NC_EVARMETA Error with var metadata.
 * @author Ed Hartnett
 */
int
nc4_get_var_meta(NC_VAR_INFO_T *var)
{
    NC_HDF5_VAR_INFO_T *hdf5_var;
    hid_t access_pid = 0;
    hid_t propid = 0;
    double rdcc_w0;
    int retval = NC_NOERR;

    assert(var && var->format_var_info);
    LOG((3, "%s: var %s", __func__, var->hdr.name));

    /* Have we already read the var metadata? */
    if (var->meta_read)
        return NC_NOERR;

    /* Get pointer to the HDF5-specific var info struct. */
    hdf5_var = (NC_HDF5_VAR_INFO_T *)var->format_var_info;

    /* Get the current chunk cache settings. */
    if ((access_pid = H5Dget_access_plist(hdf5_var->hdf_datasetid)) < 0)
        BAIL(NC_EVARMETA);

    /* Learn about current chunk cache settings. */
    if ((H5Pget_chunk_cache(access_pid, &(var->chunk_cache_nelems),
                            &(var->chunk_cache_size), &rdcc_w0)) < 0)
        BAIL(NC_EHDFERR);
    var->chunk_cache_preemption = rdcc_w0;

    /* Get the dataset creation properties. */
    if ((propid = H5Dget_create_plist(hdf5_var->hdf_datasetid)) < 0)
        BAIL(NC_EHDFERR);

    /* Get var chunking info. */
    if ((retval = get_chunking_info(propid, var)))
        BAIL(retval);

    /* Get filter info for a var. */
    if ((retval = get_filter_info(propid, var)))
        BAIL(retval);

    /* Get fill value, if defined. */
    if ((retval = get_fill_info(propid, var)))
        BAIL(retval);

    /* Is this a deflated variable with a chunksize greater than the
     * current cache size? */
    if ((retval = nc4_adjust_var_cache(var->container, var)))
        BAIL(retval);

    if (var->coords_read && !var->dimscale)
        if ((retval = get_attached_info(var, hdf5_var, var->ndims, hdf5_var->hdf_datasetid)))
            return retval;

    /* Remember that we have read the metadata for this var. */
    var->meta_read = NC_TRUE;

exit:
    if (access_pid && H5Pclose(access_pid) < 0)
        BAIL2(NC_EHDFERR);
    if (propid > 0 && H5Pclose(propid) < 0)
        BAIL2(NC_EHDFERR);
    return retval;
}

/**
 * @internal This function is called by read_dataset(), (which is
 * called by rec_read_metadata()) when a netCDF variable is found in
 * the file. This function reads in all the metadata about the
 * var. Attributes are not read until the user asks for information
 * about one of them.
 *
 * @param grp Pointer to group info struct.
 * @param datasetid HDF5 dataset ID.
 * @param obj_name Name of the HDF5 object to read.
 * @param ndims Number of dimensions.
 * @param dim If non-NULL, then this var is a coordinate var for a
 * dimension, and this points to the info for that dimension.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_ENOMEM Out of memory.
 * @return ::NC_EHDFERR HDF5 returned error.
 * @return ::NC_EVARMETA Error with var metadata.
 * @author Ed Hartnett, Dennis Heimbigner
 */
static int
read_var(NC_GRP_INFO_T *grp, hid_t datasetid, const char *obj_name,
         size_t ndims, NC_DIM_INFO_T *dim)
{
    NC_VAR_INFO_T *var = NULL;
    NC_HDF5_VAR_INFO_T *hdf5_var;
    int incr_id_rc = 0; /* Whether dataset ID's ref count has been incremented */
    char *finalname = NULL;
    int retval = NC_NOERR;

    assert(obj_name && grp);
    LOG((4, "%s: obj_name %s", __func__, obj_name));

    /* Check for a weird case: a non-coordinate variable that has the
     * same name as a dimension. It's legal in netcdf, and requires
     * that the HDF5 dataset name be changed. */
    if (strlen(obj_name) > strlen(NON_COORD_PREPEND) &&
        !strncmp(obj_name, NON_COORD_PREPEND, strlen(NON_COORD_PREPEND)))
    {
        /* Allocate space for the name. */
        if (!(finalname = malloc(((strlen(obj_name) -
                                   strlen(NON_COORD_PREPEND))+ 1) * sizeof(char))))
            BAIL(NC_ENOMEM);
        strcpy(finalname, &obj_name[strlen(NON_COORD_PREPEND)]);
    } else
        finalname = strdup(obj_name);

    /* Add a variable to the end of the group's var list. */
    if ((retval = nc4_var_list_add(grp, finalname, ndims, &var)))
        BAIL(retval);

    /* Add storage for HDF5-specific var info. */
    if (!(var->format_var_info = calloc(1, sizeof(NC_HDF5_VAR_INFO_T))))
        BAIL(NC_ENOMEM);
    hdf5_var = (NC_HDF5_VAR_INFO_T *)var->format_var_info;

    /* Fill in what we already know. */
    hdf5_var->hdf_datasetid = datasetid;
    H5Iinc_ref(hdf5_var->hdf_datasetid); /* Increment number of objects using ID */
    incr_id_rc++; /* Indicate that we've incremented the ref. count (for errors) */
    var->created = NC_TRUE;
    var->atts_read = 0;

    /* Try and read the dimids from the COORDINATES attribute. If it's
     * not present, we will have to do dimsscale matching to locate the
     * dims for this var. */
    retval = read_coord_dimids(grp, var);
    if (retval && retval != NC_ENOTATT)
        BAIL(retval);
    retval = NC_NOERR;

    /* Handle scale info. */
    if ((retval = get_scale_info(grp, dim, var, hdf5_var, ndims, datasetid)))
        BAIL(retval);

    /* Learn all about the type of this variable. This will fail for
     * HDF5 reference types, and then the var we just created will be
     * deleted, thus ignoring HDF5 reference type objects. */
    if ((retval = get_type_info2(var->container->nc4_info, hdf5_var->hdf_datasetid,
                                 &var->type_info)))
        BAIL(retval);

    /* Indicate that the variable has a pointer to the type */
    var->type_info->rc++;

exit:
    if (finalname)
        free(finalname);
    if (retval)
    {
        /* If there was an error, decrement the dataset ref counter, and
         * delete the var info struct we just created. */
        if (incr_id_rc && H5Idec_ref(datasetid) < 0)
            BAIL2(NC_EHDFERR);
        if (var)
            nc4_var_list_del(grp, var);
    }

    return retval;
}

/**
 * @internal Given an HDF5 type, set a pointer to netcdf type.
 *
 * @param h5 Pointer to HDF5 file info struct.
 * @param native_typeid HDF5 type ID.
 * @param xtype Pointer that gets netCDF type.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EHDFERR HDF5 returned error.
 * @return ::NC_EBADTYPID Type not found.
 * @author Ed Hartnett
 */
static int
get_netcdf_type(NC_FILE_INFO_T *h5, hid_t native_typeid,
                nc_type *xtype)
{
    NC_TYPE_INFO_T *type;
    H5T_class_t class;
    htri_t is_str, equal = 0;

    assert(h5 && xtype);

    if ((class = H5Tget_class(native_typeid)) < 0)
        return NC_EHDFERR;

    /* H5Tequal doesn't work with H5T_C_S1 for some reason. But
     * H5Tget_class will return H5T_STRING if this is a string. */
    if (class == H5T_STRING)
    {
        if ((is_str = H5Tis_variable_str(native_typeid)) < 0)
            return NC_EHDFERR;
        if (is_str)
            *xtype = NC_STRING;
        else
            *xtype = NC_CHAR;
        return NC_NOERR;
    }
    else if (class == H5T_INTEGER || class == H5T_FLOAT)
    {
        /* For integers and floats, we don't have to worry about
         * endianness if we compare native types. */
        if ((equal = H5Tequal(native_typeid, H5T_NATIVE_SCHAR)) < 0)
            return NC_EHDFERR;
        if (equal)
        {
            *xtype = NC_BYTE;
            return NC_NOERR;
        }
        if ((equal = H5Tequal(native_typeid, H5T_NATIVE_SHORT)) < 0)
            return NC_EHDFERR;
        if (equal)
        {
            *xtype = NC_SHORT;
            return NC_NOERR;
        }
        if ((equal = H5Tequal(native_typeid, H5T_NATIVE_INT)) < 0)
            return NC_EHDFERR;
        if (equal)
        {
            *xtype = NC_INT;
            return NC_NOERR;
        }
        if ((equal = H5Tequal(native_typeid, H5T_NATIVE_FLOAT)) < 0)
            return NC_EHDFERR;
        if (equal)
        {
            *xtype = NC_FLOAT;
            return NC_NOERR;
        }
        if ((equal = H5Tequal(native_typeid, H5T_NATIVE_DOUBLE)) < 0)
            return NC_EHDFERR;
        if (equal)
        {
            *xtype = NC_DOUBLE;
            return NC_NOERR;
        }
        if ((equal = H5Tequal(native_typeid, H5T_NATIVE_UCHAR)) < 0)
            return NC_EHDFERR;
        if (equal)
        {
            *xtype = NC_UBYTE;
            return NC_NOERR;
        }
        if ((equal = H5Tequal(native_typeid, H5T_NATIVE_USHORT)) < 0)
            return NC_EHDFERR;
        if (equal)
        {
            *xtype = NC_USHORT;
            return NC_NOERR;
        }
        if ((equal = H5Tequal(native_typeid, H5T_NATIVE_UINT)) < 0)
            return NC_EHDFERR;
        if (equal)
        {
            *xtype = NC_UINT;
            return NC_NOERR;
        }
        if ((equal = H5Tequal(native_typeid, H5T_NATIVE_LLONG)) < 0)
            return NC_EHDFERR;
        if (equal)
        {
            *xtype = NC_INT64;
            return NC_NOERR;
        }
        if ((equal = H5Tequal(native_typeid, H5T_NATIVE_ULLONG)) < 0)
            return NC_EHDFERR;
        if (equal)
        {
            *xtype = NC_UINT64;
            return NC_NOERR;
        }
    }

    /* Maybe we already know about this type. */
    if (!equal)
        if((type = nc4_rec_find_hdf_type(h5, native_typeid)))
        {
            *xtype = type->hdr.id;
            return NC_NOERR;
        }

    *xtype = NC_NAT;
    return NC_EBADTYPID;
}

/**
 * @internal Read an attribute. This is called by
 * att_read_callbk().
 *
 * @param grp Pointer to group info struct.
 * @param attid Attribute ID.
 * @param att Pointer that gets att info struct.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EHDFERR HDF5 returned error.
 * @return ::NC_EATTMETA Att metadata error.
 * @return ::NC_ENOMEM Out of memory.
 * @author Ed Hartnett
 */
static int
read_hdf5_att(NC_GRP_INFO_T *grp, hid_t attid, NC_ATT_INFO_T *att)
{
    NC_HDF5_ATT_INFO_T *hdf5_att;
    hid_t spaceid = 0, file_typeid = 0;
    hsize_t dims[1] = {0}; /* netcdf attributes always 1-D. */
    size_t type_size;
    int att_ndims;
    hssize_t att_npoints;
    H5T_class_t att_class;
    int fixed_len_string = 0;
    size_t fixed_size = 0;
    int retval = NC_NOERR;

    assert(att && att->hdr.name && att->format_att_info);
    LOG((5, "%s: att->hdr.id %d att->hdr.name %s att->nc_typeid %d att->len %d",
         __func__, att->hdr.id, att->hdr.name, (int)att->nc_typeid, att->len));

    /* Get HDF5-sepecific info struct for this attribute. */
    hdf5_att = (NC_HDF5_ATT_INFO_T *)att->format_att_info;

    /* Get type of attribute in file. */
    if ((file_typeid = H5Aget_type(attid)) < 0)
        return NC_EATTMETA;
    if ((hdf5_att->native_hdf_typeid = H5Tget_native_type(file_typeid,
                                                          H5T_DIR_DEFAULT)) < 0)
        BAIL(NC_EHDFERR);
    if ((att_class = H5Tget_class(hdf5_att->native_hdf_typeid)) < 0)
        BAIL(NC_EATTMETA);
    if (att_class == H5T_STRING &&
        !H5Tis_variable_str(hdf5_att->native_hdf_typeid))
    {
        fixed_len_string++;
        if (!(fixed_size = H5Tget_size(hdf5_att->native_hdf_typeid)))
            BAIL(NC_EATTMETA);
    }
    if ((retval = get_netcdf_type(grp->nc4_info, hdf5_att->native_hdf_typeid,
                                  &(att->nc_typeid))))
        BAIL(retval);


    /* Get len. */
    if ((spaceid = H5Aget_space(attid)) < 0)
        BAIL(NC_EATTMETA);
    if ((att_ndims = H5Sget_simple_extent_ndims(spaceid)) < 0)
        BAIL(NC_EATTMETA);
    if ((att_npoints = H5Sget_simple_extent_npoints(spaceid)) < 0)
        BAIL(NC_EATTMETA);

    /* If both att_ndims and att_npoints are zero, then this is a
     * zero length att. */
    if (att_ndims == 0 && att_npoints == 0)
        dims[0] = 0;
    else if (att->nc_typeid == NC_STRING)
        dims[0] = att_npoints;
    else if (att->nc_typeid == NC_CHAR)
    {
        /* NC_CHAR attributes are written as a scalar in HDF5, of type
         * H5T_C_S1, of variable length. */
        if (att_ndims == 0)
        {
            if (!(dims[0] = H5Tget_size(file_typeid)))
                BAIL(NC_EATTMETA);
        }
        else
        {
            /* This is really a string type! */
            att->nc_typeid = NC_STRING;
            dims[0] = att_npoints;
        }
    }
    else
    {
        H5S_class_t space_class;

        /* All netcdf attributes are scalar or 1-D only. */
        if (att_ndims > 1)
            BAIL(NC_EATTMETA);

        /* Check class of HDF5 dataspace */
        if ((space_class = H5Sget_simple_extent_type(spaceid)) < 0)
            BAIL(NC_EATTMETA);

        /* Check for NULL HDF5 dataspace class (should be weeded out
         * earlier) */
        if (H5S_NULL == space_class)
            BAIL(NC_EATTMETA);

        /* check for SCALAR HDF5 dataspace class */
        if (H5S_SCALAR == space_class)
            dims[0] = 1;
        else /* Must be "simple" dataspace */
        {
            /* Read the size of this attribute. */
            if (H5Sget_simple_extent_dims(spaceid, dims, NULL) < 0)
                BAIL(NC_EATTMETA);
        }
    }

    /* Tell the user what the length if this attribute is. */
    att->len = dims[0];

    /* Allocate some memory if the len is not zero, and read the
       attribute. */
    if (dims[0])
    {
        if ((retval = nc4_get_typelen_mem(grp->nc4_info, att->nc_typeid,
                                          &type_size)))
            return retval;
        if (att_class == H5T_VLEN)
        {
            if (!(att->vldata = malloc((unsigned int)(att->len * sizeof(hvl_t)))))
                BAIL(NC_ENOMEM);
            if (H5Aread(attid, hdf5_att->native_hdf_typeid, att->vldata) < 0)
                BAIL(NC_EATTMETA);
        }
        else if (att->nc_typeid == NC_STRING)
        {
            if (!(att->stdata = calloc(att->len, sizeof(char *))))
                BAIL(NC_ENOMEM);
            /* For a fixed length HDF5 string, the read requires
             * contiguous memory. Meanwhile, the netCDF API requires that
             * nc_free_string be called on string arrays, which would not
             * work if one contiguous memory block were used. So here I
             * convert the contiguous block of strings into an array of
             * malloced strings (each string with its own malloc). Then I
             * copy the data and free the contiguous memory. This
             * involves copying the data, which is bad, but this only
             * occurs for fixed length string attributes, and presumably
             * these are small. (And netCDF-4 does not create them - it
             * always uses variable length strings. */
            if (fixed_len_string)
            {
                int i;
                char *contig_buf, *cur;

                /* Alloc space for the contiguous memory read. */
                if (!(contig_buf = malloc(att->len * fixed_size * sizeof(char))))
                    BAIL(NC_ENOMEM);

                /* Read the fixed-len strings as one big block. */
                if (H5Aread(attid, hdf5_att->native_hdf_typeid, contig_buf) < 0) {
                    free(contig_buf);
                    BAIL(NC_EATTMETA);
                }

                /* Copy strings, one at a time, into their new home. Alloc
                   space for each string. The user will later free this
                   space with nc_free_string. */
                cur = contig_buf;
                for (i = 0; i < att->len; i++)
                {
                    if (!(att->stdata[i] = malloc(fixed_size))) {
                        free(contig_buf);
                        BAIL(NC_ENOMEM);
                    }
                    strncpy(att->stdata[i], cur, fixed_size);
                    cur += fixed_size;
                }

                /* Free contiguous memory buffer. */
                free(contig_buf);
            }
            else
            {
                /* Read variable-length string atts. */
                if (H5Aread(attid, hdf5_att->native_hdf_typeid, att->stdata) < 0)
                    BAIL(NC_EATTMETA);
            }
        }
        else
        {
            if (!(att->data = malloc((unsigned int)(att->len * type_size))))
                BAIL(NC_ENOMEM);
            if (H5Aread(attid, hdf5_att->native_hdf_typeid, att->data) < 0)
                BAIL(NC_EATTMETA);
        }
    }

    if (H5Tclose(file_typeid) < 0)
        BAIL(NC_EHDFERR);
    if (H5Sclose(spaceid) < 0)
        return NC_EHDFERR;

    return NC_NOERR;

exit:
    if (H5Tclose(file_typeid) < 0)
        BAIL2(NC_EHDFERR);
    if (spaceid > 0 && H5Sclose(spaceid) < 0)
        BAIL2(NC_EHDFERR);
    return retval;
}

/**
 * @internal Wrap HDF5 allocated memory free operations
 *
 * @param memory Pointer to memory to be freed.
 *
 * @return ::NC_NOERR No error.
 * @author Dennis Heimbigner
 */
static void
hdf5free(void* memory)
{
#ifndef JNA
    /* On Windows using the microsoft runtime, it is an error
       for one library to free memory allocated by a different library.*/
#ifdef HAVE_H5FREE_MEMORY
    if(memory != NULL) H5free_memory(memory);
#else
#ifndef _MSC_VER
    if(memory != NULL) free(memory);
#endif
#endif
#endif
}

/**
 * @internal Read information about a user defined type from the HDF5
 * file, and stash it in the group's list of types.
 *
 * @param grp Pointer to group info struct.
 * @param hdf_typeid HDF5 type ID.
 * @param type_name Pointer that gets the type name.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EHDFERR HDF5 returned error.
 * @return ::NC_EBADTYPID Type not found.
 * @return ::NC_ENOMEM Out of memory.
 * @author Ed Hartnett
 */
static int
read_type(NC_GRP_INFO_T *grp, hid_t hdf_typeid, char *type_name)
{
    NC_TYPE_INFO_T *type;
    NC_HDF5_TYPE_INFO_T *hdf5_type;
    H5T_class_t class;
    hid_t native_typeid;
    size_t type_size;
    int nmembers;
    int retval;

    assert(grp && type_name);

    LOG((4, "%s: type_name %s grp->hdr.name %s", __func__, type_name,
         grp->hdr.name));

    /* What is the native type for this platform? */
    if ((native_typeid = H5Tget_native_type(hdf_typeid, H5T_DIR_DEFAULT)) < 0)
        return NC_EHDFERR;

    /* What is the size of this type on this platform. */
    if (!(type_size = H5Tget_size(native_typeid)))
        return NC_EHDFERR;
    LOG((5, "type_size %d", type_size));

    /* Add to the list for this new type, and get a local pointer to it. */
    if ((retval = nc4_type_list_add(grp, type_size, type_name, &type)))
        return retval;

    /* Allocate storage for HDF5-specific type info. */
    if (!(hdf5_type = calloc(1, sizeof(NC_HDF5_TYPE_INFO_T))))
        return NC_ENOMEM;
    type->format_type_info = hdf5_type;

    /* Remember HDF5-specific type info. */
    hdf5_type->hdf_typeid = hdf_typeid;
    hdf5_type->native_hdf_typeid = native_typeid;

    /* Remember we have committed this type. */
    type->committed = NC_TRUE;

    /* Increment number of objects using ID. */
    if (H5Iinc_ref(hdf5_type->hdf_typeid) < 0)
        return NC_EHDFERR;

    /* What is the class of this type, compound, vlen, etc. */
    if ((class = H5Tget_class(hdf_typeid)) < 0)
        return NC_EHDFERR;
    switch (class)
    {
    case H5T_STRING:
        type->nc_type_class = NC_STRING;
        break;

    case H5T_COMPOUND:
    {
        int nmembers;
        unsigned int m;
        char* member_name = NULL;
#ifdef JNA
        char jna[1001];
#endif

        type->nc_type_class = NC_COMPOUND;

        if ((nmembers = H5Tget_nmembers(hdf_typeid)) < 0)
            return NC_EHDFERR;
        LOG((5, "compound type has %d members", nmembers));
        type->u.c.field = nclistnew();
        nclistsetalloc(type->u.c.field,nmembers);

        for (m = 0; m < nmembers; m++)
        {
            hid_t member_hdf_typeid;
            hid_t member_native_typeid;
            size_t member_offset;
            H5T_class_t mem_class;
            nc_type member_xtype;

            /* Get the typeid and native typeid of this member of the
             * compound type. */
            if ((member_hdf_typeid = H5Tget_member_type(native_typeid, m)) < 0)
                return NC_EHDFERR;

            if ((member_native_typeid = H5Tget_native_type(member_hdf_typeid,
                                                           H5T_DIR_DEFAULT)) < 0)
                return NC_EHDFERR;

            /* Get the name of the member.*/
            member_name = H5Tget_member_name(native_typeid, m);
            if (!member_name || strlen(member_name) > NC_MAX_NAME) {
                retval = NC_EBADNAME;
                break;
            }
#ifdef JNA
            else {
                strncpy(jna,member_name,1000);
                member_name = jna;
            }
#endif

            /* Offset in bytes on *this* platform. */
            member_offset = H5Tget_member_offset(native_typeid, m);

            /* Get dimensional data if this member is an array of something. */
            if ((mem_class = H5Tget_class(member_hdf_typeid)) < 0)
                return NC_EHDFERR;
            if (mem_class == H5T_ARRAY)
            {
                int ndims, dim_size[NC_MAX_VAR_DIMS];
                hsize_t dims[NC_MAX_VAR_DIMS];
                int d;

                if ((ndims = H5Tget_array_ndims(member_hdf_typeid)) < 0)
                    return NC_EHDFERR;

                if (H5Tget_array_dims(member_hdf_typeid, dims, NULL) != ndims)
                    return NC_EHDFERR;

                for (d = 0; d < ndims; d++)
                    dim_size[d] = dims[d];

                /* What is the netCDF typeid of this member? */
                if ((retval = get_netcdf_type(grp->nc4_info, H5Tget_super(member_hdf_typeid),
                                              &member_xtype)))
                    return retval;

                /* Add this member to our list of fields in this compound type. */
                if ((retval = nc4_field_list_add(type, member_name, member_offset,
                                                 member_xtype, ndims, dim_size)))
                    return retval;
            }
            else
            {
                /* What is the netCDF typeid of this member? */
                if ((retval = get_netcdf_type(grp->nc4_info, member_native_typeid,
                                              &member_xtype)))
                    return retval;

                /* Add this member to our list of fields in this compound type. */
                if ((retval = nc4_field_list_add(type, member_name, member_offset,
                                                 member_xtype, 0, NULL)))
                    return retval;
            }

            hdf5free(member_name);
        }
    }
    break;

    case H5T_VLEN:
    {
        htri_t ret;

        /* For conveninence we allow user to pass vlens of strings
         * with null terminated strings. This means strings are
         * treated slightly differently by the API, although they are
         * really just VLENs of characters. */
        if ((ret = H5Tis_variable_str(hdf_typeid)) < 0)
            return NC_EHDFERR;
        if (ret)
            type->nc_type_class = NC_STRING;
        else
        {
            hid_t base_hdf_typeid;
            nc_type base_nc_type = NC_NAT;

            type->nc_type_class = NC_VLEN;

            /* Find the base type of this vlen (i.e. what is this a
             * vlen of?) */
            if (!(base_hdf_typeid = H5Tget_super(native_typeid)))
                return NC_EHDFERR;

            /* What size is this type? */
            if (!(type_size = H5Tget_size(base_hdf_typeid)))
                return NC_EHDFERR;

            /* What is the netcdf corresponding type. */
            if ((retval = get_netcdf_type(grp->nc4_info, base_hdf_typeid,
                                          &base_nc_type)))
                return retval;
            LOG((5, "base_hdf_typeid 0x%x type_size %d base_nc_type %d",
                 base_hdf_typeid, type_size, base_nc_type));

            /* Remember the base type for this vlen. */
            type->u.v.base_nc_typeid = base_nc_type;
        }
    }
    break;

    case H5T_OPAQUE:
        type->nc_type_class = NC_OPAQUE;
        break;

    case H5T_ENUM:
    {
        hid_t base_hdf_typeid;
        nc_type base_nc_type = NC_NAT;
        void *value;
        int i;
        char *member_name = NULL;
#ifdef JNA
        char jna[1001];
#endif

        type->nc_type_class = NC_ENUM;

        /* Find the base type of this enum (i.e. what is this a
         * enum of?) */
        if (!(base_hdf_typeid = H5Tget_super(hdf_typeid)))
            return NC_EHDFERR;
        /* What size is this type? */
        if (!(type_size = H5Tget_size(base_hdf_typeid)))
            return NC_EHDFERR;
        /* What is the netcdf corresponding type. */
        if ((retval = get_netcdf_type(grp->nc4_info, base_hdf_typeid,
                                      &base_nc_type)))
            return retval;
        LOG((5, "base_hdf_typeid 0x%x type_size %d base_nc_type %d",
             base_hdf_typeid, type_size, base_nc_type));

        /* Remember the base type for this enum. */
        type->u.e.base_nc_typeid = base_nc_type;

        /* Find out how many member are in the enum. */
        if ((nmembers = H5Tget_nmembers(hdf_typeid)) < 0)
            return NC_EHDFERR;
        type->u.e.enum_member = nclistnew();
        nclistsetalloc(type->u.e.enum_member,nmembers);

        /* Allocate space for one value. */
        if (!(value = calloc(1, type_size)))
            return NC_ENOMEM;

        /* Read each name and value defined in the enum. */
        for (i = 0; i < nmembers; i++)
        {
            /* Get the name and value from HDF5. */
            if (!(member_name = H5Tget_member_name(hdf_typeid, i)))
                return NC_EHDFERR;

#ifdef JNA
            strncpy(jna,member_name,1000);
            member_name = jna;
#endif

            if (strlen(member_name) > NC_MAX_NAME)
                return NC_EBADNAME;

            if (H5Tget_member_value(hdf_typeid, i, value) < 0)
                return NC_EHDFERR;

            /* Insert new field into this type's list of fields. */
            if ((retval = nc4_enum_member_add(type, type->size,
                                              member_name, value)))
                return retval;

            hdf5free(member_name);
        }
        free(value);
    }
    break;

    default:
        LOG((0, "unknown class"));
        return NC_EBADCLASS;
    }
    return retval;
}

/**
 * @internal Callback function for reading attributes. This is used
 * for both global and variable attributes.
 *
 * @param loc_id HDF5 attribute ID.
 * @param att_name Name of the attrigute.
 * @param ainfo HDF5 info struct for attribute.
 * @param att_data Pointer to an att_iter_info struct, which contains
 * pointers to the NC_GRP_INFO_T and (for variable attributes) the
 * NC_VAR_INFO_T. For global atts the var pointer is NULL.
 *
 * @return ::NC_NOERR No error. Iteration continues.
 * @return ::-1 Error. Stop iteration.
 * @author Ed Hartnett
 */
static herr_t
att_read_callbk(hid_t loc_id, const char *att_name, const H5A_info_t *ainfo,
                void *att_data)
{

    hid_t attid = 0;
    NC_ATT_INFO_T *att;
    NCindex *list;
    att_iter_info *att_info = (att_iter_info *)att_data;
    int retval = NC_NOERR;

    /* Determine what list is being added to. */
    list = att_info->var ? att_info->var->att : att_info->grp->att;

    /* This may be an attribute telling us that strict netcdf-3 rules
     * are in effect. If so, we will make note of the fact, but not add
     * this attribute to the metadata. It's not a user attribute, but
     * an internal netcdf-4 one. */
    if (!strcmp(att_name, NC3_STRICT_ATT_NAME))
    {
        /* Only relevant for groups, not vars. */
        if (!att_info->var)
            att_info->grp->nc4_info->cmode |= NC_CLASSIC_MODEL;
        return NC_NOERR;
    }

    /* Should we ignore this attribute? */
    if (NC_findreserved(att_name))
        return NC_NOERR;

    /* Add to the end of the list of atts for this var. */
    if ((retval = nc4_att_list_add(list, att_name, &att)))
        BAIL(-1);

    /* Allocate storage for the HDF5 specific att info. */
    if (!(att->format_att_info = calloc(1, sizeof(NC_HDF5_ATT_INFO_T))))
        BAIL(-1);

    /* Open the att by name. */
    if ((attid = H5Aopen(loc_id, att_name, H5P_DEFAULT)) < 0)
        BAIL(-1);
    LOG((4, "%s::  att_name %s", __func__, att_name));

    /* Read the rest of the info about the att,
     * including its values. */
    if ((retval = read_hdf5_att(att_info->grp, attid, att)))
        BAIL(retval);

    if (att)
        att->created = NC_TRUE;

exit:
    if (retval == NC_EBADTYPID)
    {
        /* NC_EBADTYPID will be normally converted to NC_NOERR so that
           the parent iterator does not fail. */
        retval = nc4_att_list_del(list, att);
        att = NULL;
    }
    if (attid > 0 && H5Aclose(attid) < 0)
        retval = -1;

    /* Since this is a HDF5 iterator callback, return -1 for any error
     * to stop iteration. */
    if (retval)
        retval = -1;
    return retval;
}

/**
 * @internal This function reads all the attributes of a variable or
 * the global attributes of a group.
 *
 * @param grp Pointer to the group info.
 * @param var Pointer to the var info. NULL for global att reads.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EATTMETA Some error occurred reading attributes.
 * @author Ed Hartnett
 */
int
nc4_read_atts(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *var)
{
    att_iter_info att_info;         /* Custom iteration information */
    hid_t locid; /* HDF5 location to read atts from. */

    /* Check inputs. */
    assert(grp);

    /* Assign var and grp in struct. (var may be NULL). */
    att_info.var = var;
    att_info.grp = grp;

    /* Determine where to read from in the HDF5 file. */
    locid = var ? ((NC_HDF5_VAR_INFO_T *)(var->format_var_info))->hdf_datasetid :
        ((NC_HDF5_GRP_INFO_T *)(grp->format_grp_info))->hdf_grpid;

    /* Now read all the attributes at this location, ignoring special
     * netCDF hidden attributes. */
    if ((H5Aiterate2(locid, H5_INDEX_CRT_ORDER, H5_ITER_INC, NULL,
                     att_read_callbk, &att_info)) < 0)
        return NC_EATTMETA;

    /* Remember that we have read the atts for this var or group. */
    if (var)
        var->atts_read = 1;
    else
        grp->atts_read = 1;

    return NC_NOERR;
}

/**
 * @internal This function is called by read_dataset() when a
 * dimension scale dataset is encountered. It reads in the dimension
 * data (creating a new NC_DIM_INFO_T object), and also checks to see
 * if this is a dimension without a variable - that is, a coordinate
 * dimension which does not have any coordinate data.
 *
 * @param grp Pointer to group info struct.
 * @param datasetid The HDF5 dataset ID.
 * @param obj_name The HDF5 object name.
 * @param statbuf HDF5 status buffer.
 * @param scale_size Size of dimension scale.
 * @param max_scale_size Maximum size of dim scale.
 * @param dim Pointer to pointer that gets new dim info struct.
 *
 * @returns ::NC_NOERR No error.
 * @return ::NC_EHDFERR HDF5 returned error.
 * @author Ed Hartnett
 */
static int
read_scale(NC_GRP_INFO_T *grp, hid_t datasetid, const char *obj_name,
           const H5G_stat_t *statbuf, hsize_t scale_size,
           hsize_t max_scale_size, NC_DIM_INFO_T **dim)
{
    NC_DIM_INFO_T *new_dim; /* Dimension added to group */
    NC_HDF5_DIM_INFO_T *new_hdf5_dim; /* HDF5-specific dim info. */
    char dimscale_name_att[NC_MAX_NAME + 1] = "";    /* Dimscale name, for checking if dim without var */
    htri_t attr_exists = -1; /* Flag indicating hidden attribute exists */
    hid_t attid = -1; /* ID of hidden attribute (to store dim ID) */
    int dimscale_created = 0; /* Remember if a dimension was created (for error recovery) */
    short initial_next_dimid = grp->nc4_info->next_dimid;/* Retain for error recovery */
    size_t len = 0;
    int too_long = NC_FALSE;
    int assigned_id = -1;
    int retval = NC_NOERR;

    assert(grp && dim);

    /* Does this dataset have a hidden attribute that tells us its
     * dimid? If so, read it. */
    if ((attr_exists = H5Aexists(datasetid, NC_DIMID_ATT_NAME)) < 0)
        BAIL(NC_EHDFERR);
    if (attr_exists)
    {
        if ((attid = H5Aopen_name(datasetid, NC_DIMID_ATT_NAME)) < 0)
            BAIL(NC_EHDFERR);

        if (H5Aread(attid, H5T_NATIVE_INT, &assigned_id) < 0)
            BAIL(NC_EHDFERR);

        /* Check if scale's dimid should impact the group's next dimid */
        if (assigned_id >= grp->nc4_info->next_dimid)
            grp->nc4_info->next_dimid = assigned_id + 1;
    }

    /* Get dim size. On machines with a size_t of less than 8 bytes, it
     * is possible for a dimension to be too long. */
    if (SIZEOF_SIZE_T < 8 && scale_size > NC_MAX_UINT)
    {
        len = NC_MAX_UINT;
        too_long = NC_TRUE;
    }
    else
        len = scale_size;

    /* Create the dimension for this scale. */
    if ((retval = nc4_dim_list_add(grp, obj_name, len, assigned_id, &new_dim)))
        BAIL(retval);
    new_dim->too_long = too_long;

    /* Create struct for HDF5-specific dim info. */
    if (!(new_dim->format_dim_info = calloc(1, sizeof(NC_HDF5_DIM_INFO_T))))
        BAIL(NC_ENOMEM);
    new_hdf5_dim = (NC_HDF5_DIM_INFO_T *)new_dim->format_dim_info;

    dimscale_created++;

    /* Remember these 4 values to uniquely identify this dataset in the
     * HDF5 file. */
    new_hdf5_dim->hdf5_objid.fileno[0] = statbuf->fileno[0];
    new_hdf5_dim->hdf5_objid.fileno[1] = statbuf->fileno[1];
    new_hdf5_dim->hdf5_objid.objno[0] = statbuf->objno[0];
    new_hdf5_dim->hdf5_objid.objno[1] = statbuf->objno[1];

    /* If the dimscale has an unlimited dimension, then this dimension
     * is unlimited. */
    if (max_scale_size == H5S_UNLIMITED)
        new_dim->unlimited = NC_TRUE;

    /* If the scale name is set to DIM_WITHOUT_VARIABLE, then this is a
     * dimension, but not a variable. (If get_scale_name returns an
     * error, just move on, there's no NAME.) */
    if (H5DSget_scale_name(datasetid, dimscale_name_att, NC_MAX_NAME) >= 0)
    {
        if (!strncmp(dimscale_name_att, DIM_WITHOUT_VARIABLE,
                     strlen(DIM_WITHOUT_VARIABLE)))
        {
            if (new_dim->unlimited)
            {
                size_t len = 0, *lenp = &len;

                /* Find actual length by checking all datasets that use
                 * this dim. */
                if ((retval = nc4_find_dim_len(grp, new_dim->hdr.id, &lenp)))
                    BAIL(retval);
                new_dim->len = *lenp;
            }

            /* Hold open the dataset, since the dimension doesn't have a
             * coordinate variable */
            new_hdf5_dim->hdf_dimscaleid = datasetid;
            H5Iinc_ref(new_hdf5_dim->hdf_dimscaleid);        /* Increment number of objects using ID */
        }
    }

    /* Set the dimension created. */
    *dim = new_dim;

exit:
    /* Close the hidden attribute, if it was opened. */
    if (attid > 0 && H5Aclose(attid) < 0)
        BAIL2(NC_EHDFERR);

    /* On error, undo any dimscale creation */
    if (retval && dimscale_created)
    {
        /* free the dimension */
        if ((retval = nc4_dim_list_del(grp, new_dim)))
            BAIL2(retval);

        /* Reset the group's information */
        grp->nc4_info->next_dimid = initial_next_dimid;
    }

    return retval;
}

/**
 * @internal Read a HDF5 dataset. This function is called when
 * read_hdf5_obj() encounters an HDF5 dataset when opening a file.
 *
 * @param grp Pointer to group info struct.
 * @param datasetid HDF5 dataset ID.
 * @param obj_name Object name.
 * @param statbuf HDF5 status buffer.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EHDFERR HDF5 returned error.
 * @author Ed Hartnett
 */
static int
read_dataset(NC_GRP_INFO_T *grp, hid_t datasetid, const char *obj_name,
             const H5G_stat_t *statbuf)
{
    NC_DIM_INFO_T *dim = NULL;   /* Dimension created for scales */
    NC_HDF5_DIM_INFO_T *hdf5_dim;
    hid_t spaceid = 0;
    int ndims;
    htri_t is_scale;
    int retval = NC_NOERR;

    /* Get the dimension information for this dataset. */
    if ((spaceid = H5Dget_space(datasetid)) < 0)
        BAIL(NC_EHDFERR);
    if ((ndims = H5Sget_simple_extent_ndims(spaceid)) < 0)
        BAIL(NC_EHDFERR);

    /* Is this a dimscale? */
    if ((is_scale = H5DSis_scale(datasetid)) < 0)
        BAIL(NC_EHDFERR);
    if (is_scale)
    {
        hsize_t dims[H5S_MAX_RANK];
        hsize_t max_dims[H5S_MAX_RANK];

        /* Query the scale's size & max. size */
        if (H5Sget_simple_extent_dims(spaceid, dims, max_dims) < 0)
            BAIL(NC_EHDFERR);

        /* Read the scale information. */
        if ((retval = read_scale(grp, datasetid, obj_name, statbuf, dims[0],
                                 max_dims[0], &dim)))
            BAIL(retval);
        hdf5_dim = (NC_HDF5_DIM_INFO_T *)dim->format_dim_info;
    }

    /* Add a var to the linked list, and get its metadata,
     * unless this is one of those funny dimscales that are a
     * dimension in netCDF but not a variable. (Spooky!) */
    if (!dim || (dim && !hdf5_dim->hdf_dimscaleid))
        if ((retval = read_var(grp, datasetid, obj_name, ndims, dim)))
            BAIL(retval);

exit:
    if (spaceid && H5Sclose(spaceid) <0)
        BAIL2(retval);

    return retval;
}

/**
 * @internal Add HDF5 object info for a group to a list for later
 * processing. We do this when we encounter groups, so that the parent
 * group can be fully processed before the child groups.
 *
 * @param udata Pointer to the user data, in this case a
 * user_data_t.
 * @param oinfo The HDF5 object info.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 * @author Ed Hartnett, Dennis Heimbigner
 */
static int
oinfo_list_add(user_data_t *udata, const hdf5_obj_info_t *oinfo)
{
    hdf5_obj_info_t *new_oinfo;    /* Pointer to info for object */

    /* Allocate memory for the object's info. */
    if (!(new_oinfo = calloc(1, sizeof(hdf5_obj_info_t))))
        return NC_ENOMEM;

    /* Make a copy of the object's info. */
    memcpy(new_oinfo, oinfo, sizeof(hdf5_obj_info_t));

    /* Add it to the list for future processing. */
    nclistpush(udata->grps, new_oinfo);

    return NC_NOERR;
}

/**
 * @internal Callback function called by H5Literate() for every HDF5
 * object in the file.
 *
 * @note This function is called by HDF5 so does not return a netCDF
 * error code.
 *
 * @param grpid HDF5 group ID.
 * @param name Name of object.
 * @param info Info struct for object.
 * @param _op_data Pointer to user data, a user_data_t. It will
 * contain a pointer to the current group and a list of
 * hdf5_obj_info_t. Any child groups will get their hdf5_obj_info
 * added to this list.
 *
 * @return H5_ITER_CONT No error, continue iteration.
 * @return H5_ITER_ERROR HDF5 error, stop iteration.
 * @author Ed Hartnett
 */
static int
read_hdf5_obj(hid_t grpid, const char *name, const H5L_info_t *info,
              void *_op_data)
{
    /* Pointer to user data for callback */
    user_data_t *udata = (user_data_t *)_op_data;
    hdf5_obj_info_t oinfo;    /* Pointer to info for object */
    int retval = H5_ITER_CONT;

    /* Open this critter. */
    if ((oinfo.oid = H5Oopen(grpid, name, H5P_DEFAULT)) < 0)
        BAIL(H5_ITER_ERROR);

    /* Get info about the object.*/
    if (H5Gget_objinfo(oinfo.oid, ".", 1, &oinfo.statbuf) < 0)
        BAIL(H5_ITER_ERROR);

    strncpy(oinfo.oname, name, NC_MAX_NAME);

    /* Add object to list, for later */
    switch(oinfo.statbuf.type)
    {
    case H5G_GROUP:
        LOG((3, "found group %s", oinfo.oname));

        /* Defer descending into child group immediately, so that the
         * types in the current group can be processed and be ready for
         * use by vars in the child group(s). */
        if (oinfo_list_add(udata, &oinfo))
            BAIL(H5_ITER_ERROR);
        break;

    case H5G_DATASET:
        LOG((3, "found dataset %s", oinfo.oname));

        /* Learn all about this dataset, which may be a dimscale
         * (i.e. dimension metadata), or real data. */
        if ((retval = read_dataset(udata->grp, oinfo.oid, oinfo.oname,
                                   &oinfo.statbuf)))
        {
            /* Allow NC_EBADTYPID to transparently skip over datasets
             * which have a datatype that netCDF-4 doesn't understand
             * (currently), but break out of iteration for other
             * errors. */
            if (retval != NC_EBADTYPID)
                BAIL(H5_ITER_ERROR);
            else
                retval = H5_ITER_CONT;
        }

        /* Close the object */
        if (H5Oclose(oinfo.oid) < 0)
            BAIL(H5_ITER_ERROR);
        break;

    case H5G_TYPE:
        LOG((3, "found datatype %s", oinfo.oname));

        /* Process the named datatype */
        if (read_type(udata->grp, oinfo.oid, oinfo.oname))
            BAIL(H5_ITER_ERROR);

        /* Close the object */
        if (H5Oclose(oinfo.oid) < 0)
            BAIL(H5_ITER_ERROR);
        break;

    default:
        LOG((0, "Unknown object class %d in %s!", oinfo.statbuf.type, __func__));
        BAIL(H5_ITER_ERROR);
    }

exit:
    if (retval)
    {
        if (oinfo.oid > 0 && H5Oclose(oinfo.oid) < 0)
            BAIL2(H5_ITER_ERROR);
    }

    return (retval);
}

/**
 * @internal This is the main function to recursively read all the
 * metadata for the file. The links in the 'grp' are iterated over and
 * added to the file's metadata information. Note that child groups
 * are not immediately processed, but are deferred until all the other
 * links in the group are handled (so that vars in the child groups
 * are guaranteed to have types that they use in a parent group in
 * place).
 *
 * @param grp Pointer to a group.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EHDFERR HDF5 error.
 * @return ::NC_ENOMEM Out of memory.
 * @return ::NC_ECANTWRITE File must be opened read-only.
 * @author Ed Hartnett, Dennis Heimbigner
 */
static int
rec_read_metadata(NC_GRP_INFO_T *grp)
{
    NC_HDF5_GRP_INFO_T *hdf5_grp;
    user_data_t udata;         /* User data for iteration */
    hdf5_obj_info_t *oinfo;    /* Pointer to info for object */
    hsize_t idx = 0;
    hid_t pid = -1;
    unsigned crt_order_flags = 0;
    H5_index_t iter_index;
    int i, retval = NC_NOERR;

    assert(grp && grp->hdr.name && grp->format_grp_info);
    LOG((3, "%s: grp->hdr.name %s", __func__, grp->hdr.name));

    /* Get HDF5-specific group info. */
    hdf5_grp = (NC_HDF5_GRP_INFO_T *)grp->format_grp_info;

    /* Open this HDF5 group and retain its grpid. It will remain open
     * with HDF5 until this file is nc_closed. */
    if (!hdf5_grp->hdf_grpid)
    {
        if (grp->parent)
        {
            /* This is a child group. */
            NC_HDF5_GRP_INFO_T *parent_hdf5_grp;
            parent_hdf5_grp = (NC_HDF5_GRP_INFO_T *)grp->parent->format_grp_info;

            if ((hdf5_grp->hdf_grpid = H5Gopen2(parent_hdf5_grp->hdf_grpid,
                                                grp->hdr.name, H5P_DEFAULT)) < 0)
                BAIL(NC_EHDFERR);
        }
        else
        {
            /* This is the root group. */
            NC_HDF5_FILE_INFO_T *hdf5_info;
            hdf5_info = (NC_HDF5_FILE_INFO_T *)grp->nc4_info->format_file_info;

            if ((hdf5_grp->hdf_grpid = H5Gopen2(hdf5_info->hdfid, "/",
                                                H5P_DEFAULT)) < 0)
                BAIL(NC_EHDFERR);
        }
    }
    assert(hdf5_grp->hdf_grpid > 0);

    /* Get the group creation flags, to check for creation ordering. */
    if ((pid = H5Gget_create_plist(hdf5_grp->hdf_grpid)) < 0)
        BAIL(NC_EHDFERR);
    if (H5Pget_link_creation_order(pid, &crt_order_flags) < 0)
        BAIL(NC_EHDFERR);

    /* Set the iteration index to use. */
    if (crt_order_flags & H5P_CRT_ORDER_TRACKED)
        iter_index = H5_INDEX_CRT_ORDER;
    else
    {
        NC_FILE_INFO_T *h5 = grp->nc4_info;

        /* Without creation ordering, file must be read-only. */
        if (!h5->no_write)
            BAIL(NC_ECANTWRITE);

        iter_index = H5_INDEX_NAME;
    }

    /* Set user data for iteration over any child groups. */
    udata.grp = grp;
    udata.grps = nclistnew();

    /* Iterate over links in this group, building lists for the types,
     * datasets and groups encountered. A pointer to udata will be
     * passed as a parameter to the callback function
     * read_hdf5_obj(). (I have also tried H5Oiterate(), but it is much
     * slower iterating over the same file - Ed.) */
    if (H5Literate(hdf5_grp->hdf_grpid, iter_index, H5_ITER_INC, &idx,
                   read_hdf5_obj, (void *)&udata) < 0)
        BAIL(NC_EHDFERR);

    /* Process the child groups found. (Deferred until now, so that the
     * types in the current group get processed and are available for
     * vars in the child group(s).) */
    for (i = 0; i < nclistlength(udata.grps); i++)
    {
        NC_GRP_INFO_T *child_grp;
        oinfo = (hdf5_obj_info_t*)nclistget(udata.grps, i);

        /* Add group to file's hierarchy. */
        if ((retval = nc4_grp_list_add(grp->nc4_info, grp, oinfo->oname,
                                       &child_grp)))
            BAIL(retval);

        /* Allocate storage for HDF5-specific group info. */
        if (!(child_grp->format_grp_info = calloc(1, sizeof(NC_HDF5_GRP_INFO_T))))
            return NC_ENOMEM;

        /* Recursively read the child group's metadata. */
        if ((retval = rec_read_metadata(child_grp)))
            BAIL(retval);
    }

    /* When reading existing file, mark all variables as written. */
    for (i = 0; i < ncindexsize(grp->vars); i++)
        ((NC_VAR_INFO_T *)ncindexith(grp->vars, i))->written_to = NC_TRUE;

exit:
    if (pid > 0 && H5Pclose(pid) < 0)
        BAIL2(NC_EHDFERR);

    /* Clean up list of child groups. */
    for (i = 0; i < nclistlength(udata.grps); i++)
    {
        oinfo = (hdf5_obj_info_t *)nclistget(udata.grps, i);
        /* Close the open HDF5 object. */
        if (H5Oclose(oinfo->oid) < 0)
            BAIL2(NC_EHDFERR);
        free(oinfo);
    }
    nclistfree(udata.grps);

    return retval;
}
