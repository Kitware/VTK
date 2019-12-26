/* Copyright 2003-2018, University Corporation for Atmospheric
 * Research. See COPYRIGHT file for copying and redistribution
 * conditions.*/
/**
 * @file
 * @internal This file is part of netcdf-4, a netCDF-like interface
 * for HDF5, or a HDF5 backend for netCDF, depending on your point of
 * view. This file handles the NetCDF-4 variable functions.
 *
 * @author Ed Hartnett, Dennis Heimbigner, Ward Fisher
 */

#include "config.h"
#include <nc4internal.h>
#include "nc4dispatch.h"
#ifdef USE_HDF5
#include "hdf5internal.h"
#endif
#include <math.h>

/**
 * @internal This is called by nc_get_var_chunk_cache(). Get chunk
 * cache size for a variable.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param sizep Gets size in bytes of cache.
 * @param nelemsp Gets number of element slots in cache.
 * @param preemptionp Gets cache swapping setting.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_ENOTNC4 Not a netCDF-4 file.
 * @author Ed Hartnett
 */
int
NC4_get_var_chunk_cache(int ncid, int varid, size_t *sizep,
                        size_t *nelemsp, float *preemptionp)
{
    NC *nc;
    NC_GRP_INFO_T *grp;
    NC_FILE_INFO_T *h5;
    NC_VAR_INFO_T *var;
    int retval;

    /* Find info for this file and group, and set pointer to each. */
    if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
        return retval;
    assert(nc && grp && h5);

    /* Find the var. */
    var = (NC_VAR_INFO_T*)ncindexith(grp->vars,varid);
    if(!var)
        return NC_ENOTVAR;
    assert(var && var->hdr.id == varid);

    /* Give the user what they want. */
    if (sizep)
        *sizep = var->chunk_cache_size;
    if (nelemsp)
        *nelemsp = var->chunk_cache_nelems;
    if (preemptionp)
        *preemptionp = var->chunk_cache_preemption;

    return NC_NOERR;
}

/**
 * @internal A wrapper for NC4_get_var_chunk_cache(), we need this
 * version for fortran.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param sizep Gets size in bytes of cache.
 * @param nelemsp Gets number of element slots in cache.
 * @param preemptionp Gets cache swapping setting.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_ENOTNC4 Not a netCDF-4 file.
 * @author Ed Hartnett
 */
int
nc_get_var_chunk_cache_ints(int ncid, int varid, int *sizep,
                            int *nelemsp, int *preemptionp)
{
    size_t real_size, real_nelems;
    float real_preemption;
    int ret;

    if ((ret = NC4_get_var_chunk_cache(ncid, varid, &real_size,
                                       &real_nelems, &real_preemption)))
        return ret;

    if (sizep)
        *sizep = real_size / MEGABYTE;
    if (nelemsp)
        *nelemsp = (int)real_nelems;
    if(preemptionp)
        *preemptionp = (int)(real_preemption * 100);

    return NC_NOERR;
}

/**
 * @internal Get all the information about a variable. Pass NULL for
 * whatever you don't care about. This is the internal function called
 * by nc_inq_var(), nc_inq_var_deflate(), nc_inq_var_fletcher32(),
 * nc_inq_var_chunking(), nc_inq_var_chunking_ints(),
 * nc_inq_var_fill(), nc_inq_var_endian(), nc_inq_var_filter(), and
 * nc_inq_var_szip().
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param name Gets name.
 * @param xtypep Gets type.
 * @param ndimsp Gets number of dims.
 * @param dimidsp Gets array of dim IDs.
 * @param nattsp Gets number of attributes.
 * @param shufflep Gets shuffle setting.
 * @param deflatep Gets deflate setting.
 * @param deflate_levelp Gets deflate level.
 * @param fletcher32p Gets fletcher32 setting.
 * @param contiguousp Gets contiguous setting.
 * @param chunksizesp Gets chunksizes.
 * @param no_fill Gets fill mode.
 * @param fill_valuep Gets fill value.
 * @param endiannessp Gets one of ::NC_ENDIAN_BIG ::NC_ENDIAN_LITTLE
 * ::NC_ENDIAN_NATIVE
 * @param idp Pointer to memory to store filter id.
 * @param nparamsp Pointer to memory to store filter parameter count.
 * @param params Pointer to vector of unsigned integers into which
 * to store filter parameters.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Bad varid.
 * @returns ::NC_ENOMEM Out of memory.
 * @returns ::NC_EINVAL Invalid input.
 * @author Ed Hartnett, Dennis Heimbigner
 */
int
NC4_inq_var_all(int ncid, int varid, char *name, nc_type *xtypep,
                int *ndimsp, int *dimidsp, int *nattsp,
                int *shufflep, int *deflatep, int *deflate_levelp,
                int *fletcher32p, int *contiguousp, size_t *chunksizesp,
                int *no_fill, void *fill_valuep, int *endiannessp,
                unsigned int *idp, size_t *nparamsp, unsigned int *params)
{
    NC_GRP_INFO_T *grp;
    NC_FILE_INFO_T *h5;
    NC_VAR_INFO_T *var;
    int d;
    int retval;

    LOG((2, "%s: ncid 0x%x varid %d", __func__, ncid, varid));

    /* Find info for this file and group, and set pointer to each. */
    if ((retval = nc4_find_nc_grp_h5(ncid, NULL, &grp, &h5)))
        return retval;
    assert(grp && h5);

    /* If the varid is -1, find the global atts and call it a day. */
    if (varid == NC_GLOBAL && nattsp)
    {
        *nattsp = ncindexcount(grp->att);
        return NC_NOERR;
    }

    /* Find the var. */
    if (!(var = (NC_VAR_INFO_T *)ncindexith(grp->vars, varid)))
        return NC_ENOTVAR;
    assert(var && var->hdr.id == varid);

    /* Copy the data to the user's data buffers. */
    if (name)
        strcpy(name, var->hdr.name);
    if (xtypep)
        *xtypep = var->type_info->hdr.id;
    if (ndimsp)
        *ndimsp = var->ndims;
    if (dimidsp)
        for (d = 0; d < var->ndims; d++)
            dimidsp[d] = var->dimids[d];
    if (nattsp)
        *nattsp = ncindexcount(var->att);

    /* Chunking stuff. */
    if (!var->contiguous && chunksizesp)
        for (d = 0; d < var->ndims; d++)
        {
            chunksizesp[d] = var->chunksizes[d];
            LOG((4, "chunksizesp[%d]=%d", d, chunksizesp[d]));
        }

    if (contiguousp)
        *contiguousp = var->contiguous ? NC_CONTIGUOUS : NC_CHUNKED;

    /* Filter stuff. */
    if (deflatep)
        *deflatep = (int)var->deflate;
    if (deflate_levelp)
        *deflate_levelp = var->deflate_level;
    if (shufflep)
        *shufflep = (int)var->shuffle;
    if (fletcher32p)
        *fletcher32p = (int)var->fletcher32;

    if (idp)
        *idp = var->filterid;
    if (nparamsp)
        *nparamsp = (var->params == NULL ? 0 : var->nparams);
    if (params && var->params != NULL)
        memcpy(params,var->params,var->nparams*sizeof(unsigned int));

    /* Fill value stuff. */
    if (no_fill)
        *no_fill = (int)var->no_fill;

    /* Don't do a thing with fill_valuep if no_fill mode is set for
     * this var, or if fill_valuep is NULL. */
    if (!var->no_fill && fill_valuep)
    {
        /* Do we have a fill value for this var? */
        if (var->fill_value)
        {
            if (var->type_info->nc_type_class == NC_STRING)
            {
                assert(*(char **)var->fill_value);
                /* This will allocate memory and copy the string. */
                if (!(*(char **)fill_valuep = strdup(*(char **)var->fill_value)))
                {
                    free(*(char **)fill_valuep);
                    return NC_ENOMEM;
                }
            }
            else
            {
                assert(var->type_info->size);
                memcpy(fill_valuep, var->fill_value, var->type_info->size);
            }
        }
        else
        {
            if (var->type_info->nc_type_class == NC_STRING)
            {
                if (!(*(char **)fill_valuep = calloc(1, sizeof(char *))))
                    return NC_ENOMEM;

                if ((retval = nc4_get_default_fill_value(var->type_info, (char **)fill_valuep)))
                {
                    free(*(char **)fill_valuep);
                    return retval;
                }
            }
            else
            {
                if ((retval = nc4_get_default_fill_value(var->type_info, fill_valuep)))
                    return retval;
            }
        }
    }

    /* Does the user want the endianness of this variable? */
    if (endiannessp)
        *endiannessp = var->type_info->endianness;

    return NC_NOERR;
}

/**
 * @internal Inquire about chunking settings for a var. This is used
 * by the fortran API.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param contiguousp Gets contiguous setting.
 * @param chunksizesp Gets chunksizes.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_EINVAL Invalid input
 * @returns ::NC_ENOMEM Out of memory.
 * @author Ed Hartnett
 */
int
nc_inq_var_chunking_ints(int ncid, int varid, int *contiguousp, int *chunksizesp)
{
    NC_VAR_INFO_T *var;
    size_t *cs = NULL;
    int i, retval;

    /* Get pointer to the var. */
    if ((retval = nc4_find_grp_h5_var(ncid, varid, NULL, NULL, &var)))
        return retval;
    assert(var);

    /* Allocate space for the size_t copy of the chunksizes array. */
    if (var->ndims)
        if (!(cs = malloc(var->ndims * sizeof(size_t))))
            return NC_ENOMEM;

    /* Call the netcdf-4 version directly. */
    retval = NC4_inq_var_all(ncid, varid, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, contiguousp, cs, NULL,
                             NULL, NULL, NULL, NULL, NULL);

    /* Copy from size_t array. */
    if (!retval && chunksizesp && var->contiguous == NC_CHUNKED)
    {
        for (i = 0; i < var->ndims; i++)
        {
            chunksizesp[i] = (int)cs[i];
            if (cs[i] > NC_MAX_INT)
                retval = NC_ERANGE;
        }
    }

    if (var->ndims)
        free(cs);
    return retval;
}

/**
 * @internal Find the ID of a variable, from the name. This function
 * is called by nc_inq_varid().
 *
 * @param ncid File ID.
 * @param name Name of the variable.
 * @param varidp Gets variable ID.

 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Bad variable ID.
 */
int
NC4_inq_varid(int ncid, const char *name, int *varidp)
{
    NC *nc;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    char norm_name[NC_MAX_NAME + 1];
    int retval;

    if (!name)
        return NC_EINVAL;
    if (!varidp)
        return NC_NOERR;

    LOG((2, "%s: ncid 0x%x name %s", __func__, ncid, name));

    /* Find info for this file and group, and set pointer to each. */
    if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, NULL)))
        return retval;

    /* Normalize name. */
    if ((retval = nc4_normalize_name(name, norm_name)))
        return retval;

    /* Find var of this name. */
    var = (NC_VAR_INFO_T*)ncindexlookup(grp->vars,norm_name);
    if(var)
    {
        *varidp = var->hdr.id;
        return NC_NOERR;
    }
    return NC_ENOTVAR;
}

/**
 * @internal
 *
 * This function will change the parallel access of a variable from
 * independent to collective.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param par_access NC_COLLECTIVE or NC_INDEPENDENT.
 *
 * @returns ::NC_NOERR No error.
 * @returns ::NC_EBADID Invalid ncid passed.
 * @returns ::NC_ENOTVAR Invalid varid passed.
 * @returns ::NC_ENOPAR LFile was not opened with nc_open_par/nc_create_var.
 * @returns ::NC_EINVAL Invalid par_access specified.
 * @returns ::NC_NOERR for success
 * @author Ed Hartnett, Dennis Heimbigner
 */
int
NC4_var_par_access(int ncid, int varid, int par_access)
{
#ifndef USE_PARALLEL4
    NC_UNUSED(ncid);
    NC_UNUSED(varid);
    NC_UNUSED(par_access);
    return NC_ENOPAR;
#else
    NC *nc;
    NC_GRP_INFO_T *grp;
    NC_FILE_INFO_T *h5;
    NC_VAR_INFO_T *var;
    int retval;

    LOG((1, "%s: ncid 0x%x varid %d par_access %d", __func__, ncid,
         varid, par_access));

    if (par_access != NC_INDEPENDENT && par_access != NC_COLLECTIVE)
        return NC_EINVAL;

    /* Find info for this file and group, and set pointer to each. */
    if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
        return retval;

    /* This function only for files opened with nc_open_par or nc_create_par. */
    if (!h5->parallel)
        return NC_ENOPAR;

    /* Find the var, and set its preference. */
    var = (NC_VAR_INFO_T*)ncindexith(grp->vars,varid);
    if (!var) return NC_ENOTVAR;
    assert(var->hdr.id == varid);

    if (par_access)
        var->parallel_access = NC_COLLECTIVE;
    else
        var->parallel_access = NC_INDEPENDENT;
    return NC_NOERR;
#endif /* USE_PARALLEL4 */
}

/**
 * @internal Copy data from one buffer to another, performing
 * appropriate data conversion.
 *
 * This function will copy data from one buffer to another, in
 * accordance with the types. Range errors will be noted, and the fill
 * value used (or the default fill value if none is supplied) for
 * values that overflow the type.
 *
 * @param src Pointer to source of data.
 * @param dest Pointer that gets data.
 * @param src_type Type ID of source data.
 * @param dest_type Type ID of destination data.
 * @param len Number of elements of data to copy.
 * @param range_error Pointer that gets 1 if there was a range error.
 * @param fill_value The fill value.
 * @param strict_nc3 Non-zero if strict model in effect.
 *
 * @returns NC_NOERR No error.
 * @returns NC_EBADTYPE Type not found.
 * @author Ed Hartnett, Dennis Heimbigner
 */
int
nc4_convert_type(const void *src, void *dest, const nc_type src_type,
                 const nc_type dest_type, const size_t len, int *range_error,
                 const void *fill_value, int strict_nc3)
{
    char *cp, *cp1;
    float *fp, *fp1;
    double *dp, *dp1;
    int *ip, *ip1;
    short *sp, *sp1;
    signed char *bp, *bp1;
    unsigned char *ubp, *ubp1;
    unsigned short *usp, *usp1;
    unsigned int *uip, *uip1;
    long long *lip, *lip1;
    unsigned long long *ulip, *ulip1;
    size_t count = 0;

    *range_error = 0;
    LOG((3, "%s: len %d src_type %d dest_type %d", __func__, len, src_type,
         dest_type));

    /* OK, this is ugly. If you can think of anything better, I'm open
       to suggestions!

       Note that we don't use a default fill value for type
       NC_BYTE. This is because Lord Voldemort cast a nofilleramous spell
       at Harry Potter, but it bounced off his scar and hit the netcdf-4
       code.
    */
    switch (src_type)
    {
    case NC_CHAR:
        switch (dest_type)
        {
        case NC_CHAR:
            for (cp = (char *)src, cp1 = dest; count < len; count++)
                *cp1++ = *cp++;
            break;
        default:
            LOG((0, "%s: Unknown destination type.", __func__));
        }
        break;

    case NC_BYTE:
        switch (dest_type)
        {
        case NC_BYTE:
            for (bp = (signed char *)src, bp1 = dest; count < len; count++)
                *bp1++ = *bp++;
            break;
        case NC_UBYTE:
            for (bp = (signed char *)src, ubp = dest; count < len; count++)
            {
                if (*bp < 0)
                    (*range_error)++;
                *ubp++ = *bp++;
            }
            break;
        case NC_SHORT:
            for (bp = (signed char *)src, sp = dest; count < len; count++)
                *sp++ = *bp++;
            break;
        case NC_USHORT:
            for (bp = (signed char *)src, usp = dest; count < len; count++)
            {
                if (*bp < 0)
                    (*range_error)++;
                *usp++ = *bp++;
            }
            break;
        case NC_INT:
            for (bp = (signed char *)src, ip = dest; count < len; count++)
                *ip++ = *bp++;
            break;
        case NC_UINT:
            for (bp = (signed char *)src, uip = dest; count < len; count++)
            {
                if (*bp < 0)
                    (*range_error)++;
                *uip++ = *bp++;
            }
            break;
        case NC_INT64:
            for (bp = (signed char *)src, lip = dest; count < len; count++)
                *lip++ = *bp++;
            break;
        case NC_UINT64:
            for (bp = (signed char *)src, ulip = dest; count < len; count++)
            {
                if (*bp < 0)
                    (*range_error)++;
                *ulip++ = *bp++;
            }
            break;
        case NC_FLOAT:
            for (bp = (signed char *)src, fp = dest; count < len; count++)
                *fp++ = *bp++;
            break;
        case NC_DOUBLE:
            for (bp = (signed char *)src, dp = dest; count < len; count++)
                *dp++ = *bp++;
            break;
        default:
            LOG((0, "%s: unexpected dest type. src_type %d, dest_type %d",
                 __func__, src_type, dest_type));
            return NC_EBADTYPE;
        }
        break;

    case NC_UBYTE:
        switch (dest_type)
        {
        case NC_BYTE:
            for (ubp = (unsigned char *)src, bp = dest; count < len; count++)
            {
                if (!strict_nc3 && *ubp > X_SCHAR_MAX)
                    (*range_error)++;
                *bp++ = *ubp++;
            }
            break;
        case NC_SHORT:
            for (ubp = (unsigned char *)src, sp = dest; count < len; count++)
                *sp++ = *ubp++;
            break;
        case NC_UBYTE:
            for (ubp = (unsigned char *)src, ubp1 = dest; count < len; count++)
                *ubp1++ = *ubp++;
            break;
        case NC_USHORT:
            for (ubp = (unsigned char *)src, usp = dest; count < len; count++)
                *usp++ = *ubp++;
            break;
        case NC_INT:
            for (ubp = (unsigned char *)src, ip = dest; count < len; count++)
                *ip++ = *ubp++;
            break;
        case NC_UINT:
            for (ubp = (unsigned char *)src, uip = dest; count < len; count++)
                *uip++ = *ubp++;
            break;
        case NC_INT64:
            for (ubp = (unsigned char *)src, lip = dest; count < len; count++)
                *lip++ = *ubp++;
            break;
        case NC_UINT64:
            for (ubp = (unsigned char *)src, ulip = dest; count < len; count++)
                *ulip++ = *ubp++;
            break;
        case NC_FLOAT:
            for (ubp = (unsigned char *)src, fp = dest; count < len; count++)
                *fp++ = *ubp++;
            break;
        case NC_DOUBLE:
            for (ubp = (unsigned char *)src, dp = dest; count < len; count++)
                *dp++ = *ubp++;
            break;
        default:
            LOG((0, "%s: unexpected dest type. src_type %d, dest_type %d",
                 __func__, src_type, dest_type));
            return NC_EBADTYPE;
        }
        break;

    case NC_SHORT:
        switch (dest_type)
        {
        case NC_UBYTE:
            for (sp = (short *)src, ubp = dest; count < len; count++)
            {
                if (*sp > X_UCHAR_MAX || *sp < 0)
                    (*range_error)++;
                *ubp++ = *sp++;
            }
            break;
        case NC_BYTE:
            for (sp = (short *)src, bp = dest; count < len; count++)
            {
                if (*sp > X_SCHAR_MAX || *sp < X_SCHAR_MIN)
                    (*range_error)++;
                *bp++ = *sp++;
            }
            break;
        case NC_SHORT:
            for (sp = (short *)src, sp1 = dest; count < len; count++)
                *sp1++ = *sp++;
            break;
        case NC_USHORT:
            for (sp = (short *)src, usp = dest; count < len; count++)
            {
                if (*sp < 0)
                    (*range_error)++;
                *usp++ = *sp++;
            }
            break;
        case NC_INT:
            for (sp = (short *)src, ip = dest; count < len; count++)
                *ip++ = *sp++;
            break;
        case NC_UINT:
            for (sp = (short *)src, uip = dest; count < len; count++)
            {
                if (*sp < 0)
                    (*range_error)++;
                *uip++ = *sp++;
            }
            break;
        case NC_INT64:
            for (sp = (short *)src, lip = dest; count < len; count++)
                *lip++ = *sp++;
            break;
        case NC_UINT64:
            for (sp = (short *)src, ulip = dest; count < len; count++)
            {
                if (*sp < 0)
                    (*range_error)++;
                *ulip++ = *sp++;
            }
            break;
        case NC_FLOAT:
            for (sp = (short *)src, fp = dest; count < len; count++)
                *fp++ = *sp++;
            break;
        case NC_DOUBLE:
            for (sp = (short *)src, dp = dest; count < len; count++)
                *dp++ = *sp++;
            break;
        default:
            LOG((0, "%s: unexpected dest type. src_type %d, dest_type %d",
                 __func__, src_type, dest_type));
            return NC_EBADTYPE;
        }
        break;

    case NC_USHORT:
        switch (dest_type)
        {
        case NC_UBYTE:
            for (usp = (unsigned short *)src, ubp = dest; count < len; count++)
            {
                if (*usp > X_UCHAR_MAX)
                    (*range_error)++;
                *ubp++ = *usp++;
            }
            break;
        case NC_BYTE:
            for (usp = (unsigned short *)src, bp = dest; count < len; count++)
            {
                if (*usp > X_SCHAR_MAX)
                    (*range_error)++;
                *bp++ = *usp++;
            }
            break;
        case NC_SHORT:
            for (usp = (unsigned short *)src, sp = dest; count < len; count++)
            {
                if (*usp > X_SHORT_MAX)
                    (*range_error)++;
                *sp++ = *usp++;
            }
            break;
        case NC_USHORT:
            for (usp = (unsigned short *)src, usp1 = dest; count < len; count++)
                *usp1++ = *usp++;
            break;
        case NC_INT:
            for (usp = (unsigned short *)src, ip = dest; count < len; count++)
                *ip++ = *usp++;
            break;
        case NC_UINT:
            for (usp = (unsigned short *)src, uip = dest; count < len; count++)
                *uip++ = *usp++;
            break;
        case NC_INT64:
            for (usp = (unsigned short *)src, lip = dest; count < len; count++)
                *lip++ = *usp++;
            break;
        case NC_UINT64:
            for (usp = (unsigned short *)src, ulip = dest; count < len; count++)
                *ulip++ = *usp++;
            break;
        case NC_FLOAT:
            for (usp = (unsigned short *)src, fp = dest; count < len; count++)
                *fp++ = *usp++;
            break;
        case NC_DOUBLE:
            for (usp = (unsigned short *)src, dp = dest; count < len; count++)
                *dp++ = *usp++;
            break;
        default:
            LOG((0, "%s: unexpected dest type. src_type %d, dest_type %d",
                 __func__, src_type, dest_type));
            return NC_EBADTYPE;
        }
        break;

    case NC_INT:
        switch (dest_type)
        {
        case NC_UBYTE:
            for (ip = (int *)src, ubp = dest; count < len; count++)
            {
                if (*ip > X_UCHAR_MAX || *ip < 0)
                    (*range_error)++;
                *ubp++ = *ip++;
            }
            break;
        case NC_BYTE:
            for (ip = (int *)src, bp = dest; count < len; count++)
            {
                if (*ip > X_SCHAR_MAX || *ip < X_SCHAR_MIN)
                    (*range_error)++;
                *bp++ = *ip++;
            }
            break;
        case NC_SHORT:
            for (ip = (int *)src, sp = dest; count < len; count++)
            {
                if (*ip > X_SHORT_MAX || *ip < X_SHORT_MIN)
                    (*range_error)++;
                *sp++ = *ip++;
            }
            break;
        case NC_USHORT:
            for (ip = (int *)src, usp = dest; count < len; count++)
            {
                if (*ip > X_USHORT_MAX || *ip < 0)
                    (*range_error)++;
                *usp++ = *ip++;
            }
            break;
        case NC_INT: /* src is int */
            for (ip = (int *)src, ip1 = dest; count < len; count++)
            {
                if (*ip > X_INT_MAX || *ip < X_INT_MIN)
                    (*range_error)++;
                *ip1++ = *ip++;
            }
            break;
        case NC_UINT:
            for (ip = (int *)src, uip = dest; count < len; count++)
            {
                if (*ip > X_UINT_MAX || *ip < 0)
                    (*range_error)++;
                *uip++ = *ip++;
            }
            break;
        case NC_INT64:
            for (ip = (int *)src, lip = dest; count < len; count++)
                *lip++ = *ip++;
            break;
        case NC_UINT64:
            for (ip = (int *)src, ulip = dest; count < len; count++)
            {
                if (*ip < 0)
                    (*range_error)++;
                *ulip++ = *ip++;
            }
            break;
        case NC_FLOAT:
            for (ip = (int *)src, fp = dest; count < len; count++)
                *fp++ = *ip++;
            break;
        case NC_DOUBLE:
            for (ip = (int *)src, dp = dest; count < len; count++)
                *dp++ = *ip++;
            break;
        default:
            LOG((0, "%s: unexpected dest type. src_type %d, dest_type %d",
                 __func__, src_type, dest_type));
            return NC_EBADTYPE;
        }
        break;

    case NC_UINT:
        switch (dest_type)
        {
        case NC_UBYTE:
            for (uip = (unsigned int *)src, ubp = dest; count < len; count++)
            {
                if (*uip > X_UCHAR_MAX)
                    (*range_error)++;
                *ubp++ = *uip++;
            }
            break;
        case NC_BYTE:
            for (uip = (unsigned int *)src, bp = dest; count < len; count++)
            {
                if (*uip > X_SCHAR_MAX)
                    (*range_error)++;
                *bp++ = *uip++;
            }
            break;
        case NC_SHORT:
            for (uip = (unsigned int *)src, sp = dest; count < len; count++)
            {
                if (*uip > X_SHORT_MAX)
                    (*range_error)++;
                *sp++ = *uip++;
            }
            break;
        case NC_USHORT:
            for (uip = (unsigned int *)src, usp = dest; count < len; count++)
            {
                if (*uip > X_USHORT_MAX)
                    (*range_error)++;
                *usp++ = *uip++;
            }
            break;
        case NC_INT:
            for (uip = (unsigned int *)src, ip = dest; count < len; count++)
            {
                if (*uip > X_INT_MAX)
                    (*range_error)++;
                *ip++ = *uip++;
            }
            break;
        case NC_UINT:
            for (uip = (unsigned int *)src, uip1 = dest; count < len; count++)
            {
                if (*uip > X_UINT_MAX)
                    (*range_error)++;
                *uip1++ = *uip++;
            }
            break;
        case NC_INT64:
            for (uip = (unsigned int *)src, lip = dest; count < len; count++)
                *lip++ = *uip++;
            break;
        case NC_UINT64:
            for (uip = (unsigned int *)src, ulip = dest; count < len; count++)
                *ulip++ = *uip++;
            break;
        case NC_FLOAT:
            for (uip = (unsigned int *)src, fp = dest; count < len; count++)
                *fp++ = *uip++;
            break;
        case NC_DOUBLE:
            for (uip = (unsigned int *)src, dp = dest; count < len; count++)
                *dp++ = *uip++;
            break;
        default:
            LOG((0, "%s: unexpected dest type. src_type %d, dest_type %d",
                 __func__, src_type, dest_type));
            return NC_EBADTYPE;
        }
        break;

    case NC_INT64:
        switch (dest_type)
        {
        case NC_UBYTE:
            for (lip = (long long *)src, ubp = dest; count < len; count++)
            {
                if (*lip > X_UCHAR_MAX || *lip < 0)
                    (*range_error)++;
                *ubp++ = *lip++;
            }
            break;
        case NC_BYTE:
            for (lip = (long long *)src, bp = dest; count < len; count++)
            {
                if (*lip > X_SCHAR_MAX || *lip < X_SCHAR_MIN)
                    (*range_error)++;
                *bp++ = *lip++;
            }
            break;
        case NC_SHORT:
            for (lip = (long long *)src, sp = dest; count < len; count++)
            {
                if (*lip > X_SHORT_MAX || *lip < X_SHORT_MIN)
                    (*range_error)++;
                *sp++ = *lip++;
            }
            break;
        case NC_USHORT:
            for (lip = (long long *)src, usp = dest; count < len; count++)
            {
                if (*lip > X_USHORT_MAX || *lip < 0)
                    (*range_error)++;
                *usp++ = *lip++;
            }
            break;
        case NC_UINT:
            for (lip = (long long *)src, uip = dest; count < len; count++)
            {
                if (*lip > X_UINT_MAX || *lip < 0)
                    (*range_error)++;
                *uip++ = *lip++;
            }
            break;
        case NC_INT:
            for (lip = (long long *)src, ip = dest; count < len; count++)
            {
                if (*lip > X_INT_MAX || *lip < X_INT_MIN)
                    (*range_error)++;
                *ip++ = *lip++;
            }
            break;
        case NC_INT64:
            for (lip = (long long *)src, lip1 = dest; count < len; count++)
                *lip1++ = *lip++;
            break;
        case NC_UINT64:
            for (lip = (long long *)src, ulip = dest; count < len; count++)
            {
                if (*lip < 0)
                    (*range_error)++;
                *ulip++ = *lip++;
            }
            break;
        case NC_FLOAT:
            for (lip = (long long *)src, fp = dest; count < len; count++)
                *fp++ = *lip++;
            break;
        case NC_DOUBLE:
            for (lip = (long long *)src, dp = dest; count < len; count++)
                *dp++ = *lip++;
            break;
        default:
            LOG((0, "%s: unexpected dest type. src_type %d, dest_type %d",
                 __func__, src_type, dest_type));
            return NC_EBADTYPE;
        }
        break;

    case NC_UINT64:
        switch (dest_type)
        {
        case NC_UBYTE:
            for (ulip = (unsigned long long *)src, ubp = dest; count < len; count++)
            {
                if (*ulip > X_UCHAR_MAX)
                    (*range_error)++;
                *ubp++ = *ulip++;
            }
            break;
        case NC_BYTE:
            for (ulip = (unsigned long long *)src, bp = dest; count < len; count++)
            {
                if (*ulip > X_SCHAR_MAX)
                    (*range_error)++;
                *bp++ = *ulip++;
            }
            break;
        case NC_SHORT:
            for (ulip = (unsigned long long *)src, sp = dest; count < len; count++)
            {
                if (*ulip > X_SHORT_MAX)
                    (*range_error)++;
                *sp++ = *ulip++;
            }
            break;
        case NC_USHORT:
            for (ulip = (unsigned long long *)src, usp = dest; count < len; count++)
            {
                if (*ulip > X_USHORT_MAX)
                    (*range_error)++;
                *usp++ = *ulip++;
            }
            break;
        case NC_UINT:
            for (ulip = (unsigned long long *)src, uip = dest; count < len; count++)
            {
                if (*ulip > X_UINT_MAX)
                    (*range_error)++;
                *uip++ = *ulip++;
            }
            break;
        case NC_INT:
            for (ulip = (unsigned long long *)src, ip = dest; count < len; count++)
            {
                if (*ulip > X_INT_MAX)
                    (*range_error)++;
                *ip++ = *ulip++;
            }
            break;
        case NC_INT64:
            for (ulip = (unsigned long long *)src, lip = dest; count < len; count++)
            {
                if (*ulip > X_INT64_MAX)
                    (*range_error)++;
                *lip++ = *ulip++;
            }
            break;
        case NC_UINT64:
            for (ulip = (unsigned long long *)src, ulip1 = dest; count < len; count++)
                *ulip1++ = *ulip++;
            break;
        case NC_FLOAT:
            for (ulip = (unsigned long long *)src, fp = dest; count < len; count++)
                *fp++ = *ulip++;
            break;
        case NC_DOUBLE:
            for (ulip = (unsigned long long *)src, dp = dest; count < len; count++)
                *dp++ = *ulip++;
            break;
        default:
            LOG((0, "%s: unexpected dest type. src_type %d, dest_type %d",
                 __func__, src_type, dest_type));
            return NC_EBADTYPE;
        }
        break;

    case NC_FLOAT:
        switch (dest_type)
        {
        case NC_UBYTE:
            for (fp = (float *)src, ubp = dest; count < len; count++)
            {
                if (*fp > X_UCHAR_MAX || *fp < 0)
                    (*range_error)++;
                *ubp++ = *fp++;
            }
            break;
        case NC_BYTE:
            for (fp = (float *)src, bp = dest; count < len; count++)
            {
                if (*fp > (double)X_SCHAR_MAX || *fp < (double)X_SCHAR_MIN)
                    (*range_error)++;
                *bp++ = *fp++;
            }
            break;
        case NC_SHORT:
            for (fp = (float *)src, sp = dest; count < len; count++)
            {
                if (*fp > (double)X_SHORT_MAX || *fp < (double)X_SHORT_MIN)
                    (*range_error)++;
                *sp++ = *fp++;
            }
            break;
        case NC_USHORT:
            for (fp = (float *)src, usp = dest; count < len; count++)
            {
                if (*fp > X_USHORT_MAX || *fp < 0)
                    (*range_error)++;
                *usp++ = *fp++;
            }
            break;
        case NC_UINT:
            for (fp = (float *)src, uip = dest; count < len; count++)
            {
                if (*fp > X_UINT_MAX || *fp < 0)
                    (*range_error)++;
                *uip++ = *fp++;
            }
            break;
        case NC_INT:
            for (fp = (float *)src, ip = dest; count < len; count++)
            {
                if (*fp > (double)X_INT_MAX || *fp < (double)X_INT_MIN)
                    (*range_error)++;
                *ip++ = *fp++;
            }
            break;
        case NC_INT64:
            for (fp = (float *)src, lip = dest; count < len; count++)
            {
                if (*fp > X_INT64_MAX || *fp <X_INT64_MIN)
                    (*range_error)++;
                *lip++ = *fp++;
            }
            break;
        case NC_UINT64:
            for (fp = (float *)src, lip = dest; count < len; count++)
            {
                if (*fp > X_UINT64_MAX || *fp < 0)
                    (*range_error)++;
                *lip++ = *fp++;
            }
            break;
        case NC_FLOAT:
            for (fp = (float *)src, fp1 = dest; count < len; count++)
            {
                /*                if (*fp > X_FLOAT_MAX || *fp < X_FLOAT_MIN)
                                  (*range_error)++;*/
                *fp1++ = *fp++;
            }
            break;
        case NC_DOUBLE:
            for (fp = (float *)src, dp = dest; count < len; count++)
                *dp++ = *fp++;
            break;
        default:
            LOG((0, "%s: unexpected dest type. src_type %d, dest_type %d",
                 __func__, src_type, dest_type));
            return NC_EBADTYPE;
        }
        break;

    case NC_DOUBLE:
        switch (dest_type)
        {
        case NC_UBYTE:
            for (dp = (double *)src, ubp = dest; count < len; count++)
            {
                if (*dp > X_UCHAR_MAX || *dp < 0)
                    (*range_error)++;
                *ubp++ = *dp++;
            }
            break;
        case NC_BYTE:
            for (dp = (double *)src, bp = dest; count < len; count++)
            {
                if (*dp > X_SCHAR_MAX || *dp < X_SCHAR_MIN)
                    (*range_error)++;
                *bp++ = *dp++;
            }
            break;
        case NC_SHORT:
            for (dp = (double *)src, sp = dest; count < len; count++)
            {
                if (*dp > X_SHORT_MAX || *dp < X_SHORT_MIN)
                    (*range_error)++;
                *sp++ = *dp++;
            }
            break;
        case NC_USHORT:
            for (dp = (double *)src, usp = dest; count < len; count++)
            {
                if (*dp > X_USHORT_MAX || *dp < 0)
                    (*range_error)++;
                *usp++ = *dp++;
            }
            break;
        case NC_UINT:
            for (dp = (double *)src, uip = dest; count < len; count++)
            {
                if (*dp > X_UINT_MAX || *dp < 0)
                    (*range_error)++;
                *uip++ = *dp++;
            }
            break;
        case NC_INT:
            for (dp = (double *)src, ip = dest; count < len; count++)
            {
                if (*dp > X_INT_MAX || *dp < X_INT_MIN)
                    (*range_error)++;
                *ip++ = *dp++;
            }
            break;
        case NC_INT64:
            for (dp = (double *)src, lip = dest; count < len; count++)
            {
                if (*dp > X_INT64_MAX || *dp < X_INT64_MIN)
                    (*range_error)++;
                *lip++ = *dp++;
            }
            break;
        case NC_UINT64:
            for (dp = (double *)src, lip = dest; count < len; count++)
            {
                if (*dp > X_UINT64_MAX || *dp < 0)
                    (*range_error)++;
                *lip++ = *dp++;
            }
            break;
        case NC_FLOAT:
            for (dp = (double *)src, fp = dest; count < len; count++)
            {
                if (isgreater(*dp, X_FLOAT_MAX) || isless(*dp, X_FLOAT_MIN))
                    (*range_error)++;
                *fp++ = *dp++;
            }
            break;
        case NC_DOUBLE:
            for (dp = (double *)src, dp1 = dest; count < len; count++)
            {
                /* if (*dp > X_DOUBLE_MAX || *dp < X_DOUBLE_MIN) */
                /*    (*range_error)++; */
                *dp1++ = *dp++;
            }
            break;
        default:
            LOG((0, "%s: unexpected dest type. src_type %d, dest_type %d",
                 __func__, src_type, dest_type));
            return NC_EBADTYPE;
        }
        break;

    default:
        LOG((0, "%s: unexpected src type. src_type %d, dest_type %d",
             __func__, src_type, dest_type));
        return NC_EBADTYPE;
    }
    return NC_NOERR;
}

/**
 * @internal Get the default fill value for an atomic type. Memory for
 * fill_value must already be allocated, or you are DOOMED!
 *
 * @param type_info Pointer to type info struct.
 * @param fill_value Pointer that gets the default fill value.
 *
 * @returns NC_NOERR No error.
 * @returns NC_EINVAL Can't find atomic type.
 * @author Ed Hartnett
 */
int
nc4_get_default_fill_value(const NC_TYPE_INFO_T *type_info, void *fill_value)
{
    switch (type_info->hdr.id)
    {
    case NC_CHAR:
        *(char *)fill_value = NC_FILL_CHAR;
        break;

    case NC_STRING:
        *(char **)fill_value = strdup(NC_FILL_STRING);
        break;

    case NC_BYTE:
        *(signed char *)fill_value = NC_FILL_BYTE;
        break;

    case NC_SHORT:
        *(short *)fill_value = NC_FILL_SHORT;
        break;

    case NC_INT:
        *(int *)fill_value = NC_FILL_INT;
        break;

    case NC_UBYTE:
        *(unsigned char *)fill_value = NC_FILL_UBYTE;
        break;

    case NC_USHORT:
        *(unsigned short *)fill_value = NC_FILL_USHORT;
        break;

    case NC_UINT:
        *(unsigned int *)fill_value = NC_FILL_UINT;
        break;

    case NC_INT64:
        *(long long *)fill_value = NC_FILL_INT64;
        break;

    case NC_UINT64:
        *(unsigned long long *)fill_value = NC_FILL_UINT64;
        break;

    case NC_FLOAT:
        *(float *)fill_value = NC_FILL_FLOAT;
        break;

    case NC_DOUBLE:
        *(double *)fill_value = NC_FILL_DOUBLE;
        break;

    default:
        return NC_EINVAL;
    }

    return NC_NOERR;
}

/**
 * @internal Get the length, in bytes, of one element of a type in
 * memory.
 *
 * @param h5 Pointer to HDF5 file info struct.
 * @param xtype NetCDF type ID.
 * @param len Pointer that gets length in bytes.
 *
 * @returns NC_NOERR No error.
 * @returns NC_EBADTYPE Type not found.
 * @author Ed Hartnett
 */
int
nc4_get_typelen_mem(NC_FILE_INFO_T *h5, nc_type xtype, size_t *len)
{
    NC_TYPE_INFO_T *type;
    int retval;

    LOG((4, "%s xtype: %d", __func__, xtype));
    assert(len);

    /* If this is an atomic type, the answer is easy. */
    switch (xtype)
    {
    case NC_BYTE:
    case NC_CHAR:
    case NC_UBYTE:
        *len = sizeof(char);
        return NC_NOERR;
    case NC_SHORT:
    case NC_USHORT:
        *len = sizeof(short);
        return NC_NOERR;
    case NC_INT:
    case NC_UINT:
        *len = sizeof(int);
        return NC_NOERR;
    case NC_FLOAT:
        *len = sizeof(float);
        return NC_NOERR;
    case NC_DOUBLE:
        *len = sizeof(double);
        return NC_NOERR;
    case NC_INT64:
    case NC_UINT64:
        *len = sizeof(long long);
        return NC_NOERR;
    case NC_STRING:
        *len = sizeof(char *);
        return NC_NOERR;
    }

    /* See if var is compound type. */
    if ((retval = nc4_find_type(h5, xtype, &type)))
        return retval;

    if (!type)
        return NC_EBADTYPE;

    *len = type->size;

    LOG((5, "type->size: %d", type->size));

    return NC_NOERR;
}
