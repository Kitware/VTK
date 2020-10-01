/* Copyright 2003-2018, University Corporation for Atmospheric
 * Research. See the COPYRIGHT file for copying and redistribution
 * conditions.
 */
/**
 * @file @internal Internal netcdf-4 functions for filters
 *
 * This file contains functions internal to the netcdf4 library. None of
 * the functions in this file are exposed in the exetnal API. These
 * functions all relate to the manipulation of netcdf-4 filters
 *
 * @author Dennis Heimbigner
 */

#include "config.h"
#include <stdlib.h>
#include "hdf5internal.h"
#include "hdf5debug.h"

#define HAVE_H5_DEFLATE

/* Mnemonic */
#define FILTERACTIVE 1


/* WARNING: GLOBAL VARIABLE */

/* Define list of registered filters */
static NClist* NC4_registeredfilters = NULL; /** List<NC_FILTER_CLIENT_HDF5*> */

/**************************************************/
/* Filter registration support */

static int
filterlookup(unsigned int id)
{
    int i;
    if(NC4_registeredfilters == NULL)
	NC4_registeredfilters = nclistnew();
    for(i=0;i<nclistlength(NC4_registeredfilters);i++) {
	NC_FILTER_CLIENT_HDF5* x = nclistget(NC4_registeredfilters,i);
	if(x != NULL && x->id == id) return i; /* return position */
    }
    return -1;
}

static void
reclaiminfo(NC_FILTER_CLIENT_HDF5* info)
{
    nullfree(info);
}

static int
filterremove(int pos)
{
    NC_FILTER_CLIENT_HDF5* info = NULL;
    if(NC4_registeredfilters == NULL)
	return THROW(NC_EINVAL);
    if(pos < 0 || pos >= nclistlength(NC4_registeredfilters))
	return THROW(NC_EINVAL);
    info = nclistget(NC4_registeredfilters,pos);
    reclaiminfo(info);
    nclistremove(NC4_registeredfilters,pos);
    return NC_NOERR;
}

static NC_FILTER_CLIENT_HDF5*
dupfilterinfo(NC_FILTER_CLIENT_HDF5* info)
{
    NC_FILTER_CLIENT_HDF5* dup = NULL;
    if(info == NULL) goto fail;
    if((dup = calloc(1,sizeof(NC_FILTER_CLIENT_HDF5))) == NULL) goto fail;
    *dup = *info;
    return dup;
fail:
    reclaiminfo(dup);
    return NULL;
}

int
NC4_hdf5_addfilter(NC_VAR_INFO_T* var, int active, unsigned int id, size_t nparams, unsigned int* inparams)
{
    int stat = NC_NOERR;
    NC_FILTER_SPEC_HDF5* fi = NULL;
    unsigned int* params = NULL;

    if(var->filters == NULL) {
	if((var->filters = nclistnew())==NULL) return THROW(NC_ENOMEM);
    }

    if(nparams > 0 && inparams == NULL)
        return THROW(NC_EINVAL);
    if(inparams != NULL) {
        if((params = malloc(sizeof(unsigned int)*nparams)) == NULL)
	    return THROW(NC_ENOMEM);
        memcpy(params,inparams,sizeof(unsigned int)*nparams);
    }
    
    if((fi = calloc(1,sizeof(NC_FILTER_SPEC_HDF5))) == NULL)
    	{nullfree(params); return THROW(NC_ENOMEM);}

    fi->active = active;
    fi->filterid = id;
    fi->nparams = nparams;
    fi->params = params;
    nclistpush(var->filters,fi);
    return THROW(stat);
}

int
nc4_global_filter_action(int op, unsigned int id, NC_FILTER_OBJ_HDF5* infop)
{
    int stat = NC_NOERR;
    H5Z_class2_t* h5filterinfo = NULL;
    herr_t herr;
    int pos = -1;
    NC_FILTER_CLIENT_HDF5* dup = NULL;
    NC_FILTER_CLIENT_HDF5* elem = NULL;
    NC_FILTER_CLIENT_HDF5 ncf;

    switch (op) {
    case NCFILTER_CLIENT_REG: /* Ignore id argument */
        if(infop == NULL) {stat = NC_EINVAL; goto done;}
	assert(NC_FILTER_FORMAT_HDF5 == infop->hdr.format);
	assert(NC_FILTER_SORT_CLIENT == infop->sort);
	elem = (NC_FILTER_CLIENT_HDF5*)&infop->u.client;
        h5filterinfo = elem->info;
        /* Another sanity check */
        if(id != h5filterinfo->id)
	    {stat = NC_EINVAL; goto done;}
	/* See if this filter is already defined */
	if((pos = filterlookup(id)) >= 0)
	    {stat = NC_ENAMEINUSE; goto done;} /* Already defined */
	if((herr = H5Zregister(h5filterinfo)) < 0)
	    {stat = NC_EFILTER; goto done;}
	/* Save a copy of the passed in info */
	ncf.id = id;
	ncf.info = elem->info;
	if((dup=dupfilterinfo(&ncf)) == NULL)
	    {stat = NC_ENOMEM; goto done;}		
	nclistpush(NC4_registeredfilters,dup);	
	break;
    case NCFILTER_CLIENT_UNREG:
	if(id <= 0)
	    {stat = NC_ENOTNC4; goto done;}
	/* See if this filter is already defined */
	if((pos = filterlookup(id)) < 0)
	    {stat = NC_ENOFILTER; goto done;} /* Not defined */
	if((herr = H5Zunregister(id)) < 0)
	    {stat = NC_EFILTER; goto done;}
	if((stat=filterremove(pos))) goto done;
	break;
    case NCFILTER_CLIENT_INQ:
	if(infop == NULL) goto done;
        /* Look up the id in our local table */
   	if((pos = filterlookup(id)) < 0)
	    {stat = NC_ENOFILTER; goto done;} /* Not defined */
        elem = (NC_FILTER_CLIENT_HDF5*)nclistget(NC4_registeredfilters,pos);
	if(elem == NULL) {stat = NC_EINTERNAL; goto done;}
	if(infop != NULL) {
	    infop->u.client = *elem;
	}
	break;
    default:
	{stat = NC_EINTERNAL; goto done;}	
    }
done:
    return THROW(stat);
} 

/**
 * @internal Define filter settings. Called by nc_def_var_filter().
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param id Filter ID
 * @param nparams Number of parameters for filter.
 * @param parms Filter parameters.
 *
 * @returns ::NC_NOERR for success
 * @returns ::NC_EBADID Bad ncid.
 * @returns ::NC_ENOTVAR Invalid variable ID.
 * @returns ::NC_ENOTNC4 Attempting netcdf-4 operation on file that is
 * not netCDF-4/HDF5.
 * @returns ::NC_ELATEDEF Too late to change settings for this variable.
 * @returns ::NC_EFILTER Filter error.
 * @returns ::NC_EINVAL Invalid input
 * @author Dennis Heimbigner
 */
int
NC4_filter_actions(int ncid, int varid, int op, NC_Filterobject* args)
{
    int stat = NC_NOERR;
    NC_GRP_INFO_T *grp = NULL;
    NC_FILE_INFO_T *h5 = NULL;
    NC_VAR_INFO_T *var = NULL;
    NC_FILTER_OBJ_HDF5* obj = (NC_FILTER_OBJ_HDF5*)args;
    unsigned int id = 0;
    size_t nparams = 0;
    unsigned int* idp = NULL;
    size_t* nparamsp = NULL;
    size_t* nfiltersp = NULL;
    unsigned int* params = NULL;
    size_t nfilters = 0;
    unsigned int* filterids = NULL;

    LOG((2, "%s: ncid 0x%x varid %d op=%d", __func__, ncid, varid, op));

    if(args == NULL) return THROW(NC_EINVAL);
    if(args->format != NC_FILTER_FORMAT_HDF5) return THROW(NC_EFILTER);

    /* Find info for this file and group and var, and set pointer to each. */
    if ((stat = nc4_hdf5_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
	return THROW(stat);

    assert(h5 && var && var->hdr.id == varid);

    nfilters = nclistlength(var->filters);

    switch (op) {
    case NCFILTER_DEF: {
        if(obj->sort != NC_FILTER_SORT_SPEC) return THROW(NC_EFILTER);
        /* If the HDF5 dataset has already been created, then it is too
         * late to set all the extra stuff. */
        if (!(h5->flags & NC_INDEF)) return THROW(NC_EINDEFINE);
        if (!var->ndims) return NC_EINVAL; /* For scalars, complain */
        if (var->created)
             return THROW(NC_ELATEDEF);
        /* Can't turn on parallel and szip before HDF5 1.10.2. */
#ifdef USE_PARALLEL
#ifndef HDF5_SUPPORTS_PAR_FILTERS
        if (h5->parallel == NC_TRUE)
            return THROW(NC_EINVAL);
#endif /* HDF5_SUPPORTS_PAR_FILTERS */
#endif /* USE_PARALLEL */
	id = obj->u.spec.filterid;
	nparams = obj->u.spec.nparams;
	params = obj->u.spec.params;
#ifdef HAVE_H5_DEFLATE
        if(id == H5Z_FILTER_DEFLATE) {
	    int k;
	    int level;
            if(nparams != 1)
                return THROW(NC_EFILTER); /* incorrect no. of parameters */
	    level = (int)params[0];
            if (level < NC_MIN_DEFLATE_LEVEL ||
                level > NC_MAX_DEFLATE_LEVEL)
                return THROW(NC_EINVAL);
            /* If szip compression is already applied, return error. */
	    for(k=0;k<nclistlength(var->filters);k++) {
		NC_FILTER_SPEC_HDF5* f = nclistget(var->filters,k);
                if (f->filterid == H5Z_FILTER_SZIP)
                 return THROW(NC_EINVAL);
	    }
        }
#else /*!HAVE_H5_DEFLATE*/
        if(id == H5Z_FILTER_DEFLATE)
            return THROW(NC_EFILTER); /* Not allowed */
#endif
#ifdef HAVE_H5Z_SZIP
        if(id == H5Z_FILTER_SZIP) { /* Do error checking */
	    int k;
            if(nparams != 2)
                return THROW(NC_EFILTER); /* incorrect no. of parameters */
            /* Pixels per block must be an even number, < 32. */
            if (params[1] % 2 || params[1] > NC_MAX_PIXELS_PER_BLOCK)
                return THROW(NC_EINVAL);
            /* If zlib compression is already applied, return error. */
	    for(k=0;k<nclistlength(var->filters);k++) {
		NC_FILTER_SPEC_HDF5* f = nclistget(var->filters,k);
                if (f->filterid == H5Z_FILTER_DEFLATE)
                 return THROW(NC_EINVAL);
	    }
        }
#else /*!HAVE_H5Z_SZIP*/
        if(id == H5Z_FILTER_SZIP)
            return THROW(NC_EFILTER); /* Not allowed */
#endif
        /* Filter => chunking */
	var->storage = NC_CHUNKED;
        /* Determine default chunksizes for this variable unless already specified */
        if(var->chunksizes && !var->chunksizes[0]) {
	    /* Should this throw error? */
            if((stat = nc4_find_default_chunksizes2(grp, var)))
	        goto done;
            /* Adjust the cache. */
            if ((stat = nc4_adjust_var_cache(grp, var)))
                goto done;
        }
#ifdef HAVE_H5Z_SZIP
        if(id == H5Z_FILTER_SZIP) { /* szip X chunking error checking */
	    /* For szip, the pixels_per_block parameter must not be greater
	     * than the number of elements in a chunk of data. */
            size_t num_elem = 1;
            int d;
            for (d = 0; d < var->ndims; d++)
                num_elem *= var->dim[d]->len;
            /* Pixels per block must be <= number of elements. */
            if (params[1] > num_elem)
                return THROW(NC_EINVAL);
        }
#endif
	if((stat = NC4_hdf5_addfilter(var,!FILTERACTIVE,id,nparams,params)))
  	    goto done;
#ifdef USE_PARALLEL
#ifdef HDF5_SUPPORTS_PAR_FILTERS
        /* Switch to collective access. HDF5 requires collevtive access
         * for filter use with parallel I/O. */
        if (h5->parallel)
            var->parallel_access = NC_COLLECTIVE;
#else
        if (h5->parallel)
            return NC_EINVAL;
#endif /* HDF5_SUPPORTS_PAR_FILTERS */
#endif /* USE_PARALLEL */
	} break;
    case NCFILTER_INQ: {
        if (!var->ndims) return THROW(NC_EINVAL); /* For scalars, fail */
        if(obj->sort != NC_FILTER_SORT_SPEC) return THROW(NC_EFILTER);
	idp = &obj->u.spec.filterid;
	nparamsp = &obj->u.spec.nparams;
	params = obj->u.spec.params;
	if(nfilters > 0) {
	    /* Return info about var->filters[0] */
	    NC_FILTER_SPEC_HDF5* f = (NC_FILTER_SPEC_HDF5*)nclistget(var->filters,0);
	    if(idp) *idp = f->filterid;
	    if(nparamsp) {
		*nparamsp = (f->params == NULL ? 0 : f->nparams);
		if(params && f->params != NULL && f->nparams > 0)
		    memcpy(params,f->params,f->nparams*sizeof(unsigned int));
	    }
	} else {stat = NC_ENOFILTER; goto done;}
	} break;
    case NCFILTER_FILTERIDS: {
        if(obj->sort != NC_FILTER_SORT_IDS) return THROW(NC_EFILTER);
	nfiltersp = &obj->u.ids.nfilters;
	filterids = obj->u.ids.filterids;
        if(nfiltersp) *nfiltersp = nfilters;
	if(filterids) filterids[0] = 0;
        if(nfilters > 0 && filterids != NULL) {
	    int k;
	    for(k=0;k<nfilters;k++) {
		NC_FILTER_SPEC_HDF5* f = (NC_FILTER_SPEC_HDF5*)nclistget(var->filters,k);
		filterids[k] = f->filterid;
	    }
	}
	} break;
    case NCFILTER_INFO: {
	int k,found;
        if(obj->sort != NC_FILTER_SORT_SPEC) return THROW(NC_EFILTER);
	id = obj->u.spec.filterid;
        for(found=0,k=0;k<nfilters;k++) {
	    NC_FILTER_SPEC_HDF5* f = (NC_FILTER_SPEC_HDF5*)nclistget(var->filters,k);
	    if(f->filterid == id) {
	        obj->u.spec.nparams = f->nparams;
		if(obj->u.spec.params != NULL && f->params != NULL && f->nparams > 0)
		    memcpy(obj->u.spec.params,f->params,f->nparams*sizeof(unsigned int));
		found = 1;
		break;
	    }
	}
	if(!found) {stat = NC_ENOFILTER; goto done;}
	} break;
    case NCFILTER_REMOVE: {
	int k;
        if (!(h5->flags & NC_INDEF)) return THROW(NC_EINDEFINE);
        if(obj->sort != NC_FILTER_SORT_SPEC) return THROW(NC_EFILTER);
	id = obj->u.spec.filterid;
	/* Walk backwards */
        for(k=nfilters-1;k>=0;k--) {
	    NC_FILTER_SPEC_HDF5* f = (NC_FILTER_SPEC_HDF5*)nclistget(var->filters,k);
	    if(f->filterid == id) {
		if(f->active) {
		    /* Remove from variable */
		    if((stat = NC4_hdf5_remove_filter(var,id))) {stat = NC_ENOFILTER; goto done;}
		}
		nclistremove(var->filters,k);
		NC4_freefilterspec(f);
	    }
	}
	} break;
    default:
	{stat = NC_EINTERNAL; goto done;}	
    }

done:
    return THROW(stat);
}

void
NC4_freefilterspec(NC_FILTER_SPEC_HDF5* f)
{
    if(f) {
        if(f->params != NULL) {free(f->params);}
	free(f);
    }
}
