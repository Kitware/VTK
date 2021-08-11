/*
 * Copyright 2018, University Corporation for Atmospheric Research
 * See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _MSC_VER
#include <io.h>
#endif

#include "netcdf.h"
#include "netcdf_filter.h"
#include "ncdispatch.h"
#include "nc4internal.h"

#ifdef USE_HDF5
#include "hdf5internal.h"
#endif

/*
Unified filter related code
*/

/**************************************************/
/* Per-variable filters */
/* The original HDF5-based functions are left
   but are now wrappers around the filterx functions.
*/

/**
Find the set of filters (if any) associated with a variable.
Assumes HDF5 format using unsigned ints.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID
\param nfilters return no. of filters; may be zero
\param ids return the filter ids (caller allocates)

\returns ::NC_NOERR No error.
\returns ::NC_ENOTNC4 Not a netCDF-4 file.
\returns ::NC_EBADID Bad ncid
\returns ::NC_ENOTVAR Invalid variable ID.
\returns ::NC_EINVAL Invalid arguments
\ingroup variables
\author Dennis Heimbigner
*/
EXTERNL int
nc_inq_var_filter_ids(int ncid, int varid, size_t* nfiltersp, unsigned int* ids)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    TRACE(nc_inq_var_filter_ids);
    if((stat = ncp->dispatch->inq_var_filter_ids(ncid,varid,nfiltersp,ids))) goto done;

done:
   return stat;
}

/**
Find the the param info about filter (if any)
associated with a variable and with specified id.
Assumes HDF5 format using unsigned ints.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID
\param id The filter id of interest
\param nparamsp (Out) Storage which will get the number of parameters to the filter
\param params (Out) Storage which will get associated parameters.
Note: the caller must allocate and free.

\returns ::NC_NOERR No error.
\returns ::NC_ENOTNC4 Not a netCDF-4 file.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Invalid variable ID.
\returns ::NC_ENOFILTER Specified filter not defined for this variable.
\ingroup variables
\author Dennis Heimbigner
*/
EXTERNL int
nc_inq_var_filter_info(int ncid, int varid, unsigned int id, size_t* nparamsp, unsigned int* params)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    TRACE(nc_inq_var_filter_info);
    if((stat = ncp->dispatch->inq_var_filter_info(ncid,varid,id,nparamsp,params))) goto done;

done:
     return stat;
}

/**
   Define a new variable filter
   Assumes HDF5 format using unsigned ints.
   Only variables with chunked storage can use filters.

   @param ncid File and group ID.
   @param varid Variable ID.
   @param id Filter ID.
   @param nparams Number of filter parameters.
   @param parms Filter parameters.

   @return ::NC_NOERR No error.
   @return ::NC_EINVAL Variable must be chunked.
   @return ::NC_EBADID Bad ID.
   @author Dennis Heimbigner
*/

EXTERNL int
nc_def_var_filter(int ncid, int varid, unsigned int id, size_t nparams, const unsigned int* params)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    TRACE(nc_inq_var_filter_info);
    if((stat = ncp->dispatch->def_var_filter(ncid,varid,id,nparams,params))) goto done;
done:
    return stat;
}

/**
Find the first filter (if any) associated with a variable.
Assumes HDF5 format using unsigned int.
   
\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param idp Storage which will get the filter id; a return value of zero means variable has no filters.

\param nparamsp Storage which will get the number of parameters to the filter

\param params Storage which will get associated parameters (call allocates and frees).

This is redundant over the multi-filter API, so
it can be implemented in terms of those functions.

\returns ::NC_NOERR No error.
\returns ::NC_ENOTNC4 Not a netCDF-4 file.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Invalid variable ID.

\ingroup variables
\author Dennis Heimbigner
*/
EXTERNL int
nc_inq_var_filter(int ncid, int varid, unsigned int* idp, size_t* nparamsp, unsigned int* params)
{
    NC* ncp;
    size_t nfilters;
    unsigned int* ids = NULL;
    int stat = NC_check_id(ncid,&ncp);

    if(stat != NC_NOERR) return stat;
    TRACE(nc_inq_var_filter);

    /* Get the number of filters on this variable */
    if((stat = nc_inq_var_filter_ids(ncid,varid,&nfilters, NULL))) goto done;
    /* If no filters, then return zero */
    if(nfilters == 0) {
	if(idp) *idp = 0;
	goto done;
    }
    /* Get the filter ids */
    if((ids = calloc(sizeof(unsigned int),nfilters)) == NULL) {stat = NC_ENOMEM; goto done;}
    if((stat = nc_inq_var_filter_ids(ncid,varid,&nfilters, ids))) goto done;
    /* Get params for the first filter */
    if((stat = nc_inq_var_filter_info(ncid,varid,ids[0],nparamsp,params))) goto done;
    if(idp) *idp = ids[0];
 done:
    nullfree(ids);        
    return stat;
}

/**************************************************/
/* Support direct user defined filters */

#ifdef ENABLE_CLIENTSIDE_FILTERS

/* Use void* to avoid having to include hdf.h*/
EXTERNL int
nc_filter_client_register(unsigned int id, void* info)
{
    int stat = NC_NOERR;
#ifdef USE_HDF5
    NC_FILTER_OBJ_HDF5 client;
    if(id == 0 ||info == NULL)
	return NC_EINVAL;
    memset(&client,0,sizeof(client));
    client.hdr.format = NC_FILTER_FORMAT_HDF5;
    client.sort = NC_FILTER_SORT_CLIENT;
    client.u.client.id = id;
    client.u.client.info = info;
    /* Note use of a global function, not part of the dispatch table */
    stat = nc4_global_filter_action(NCFILTER_CLIENT_REG, id, &client);
#else
    stat = NC_ENOTBUILT;
#endif
    return stat;
}

EXTERNL int
nc_filter_client_unregister(unsigned int id)
{
int stat = NC_NOERR;
#ifdef USE_HDF5
    stat = nc4_global_filter_action(NCFILTER_CLIENT_UNREG, id, NULL);
#else
    stat = NC_ENOTBUILT;
#endif
    return stat;
}

/* Use void* to avoid having to include hdf.h*/
EXTERNL int
nc_filter_client_inq(unsigned int id, void* infop)
{
int stat = NC_NOERR;
#ifdef USE_HDF5
    H5Z_class2_t* hct = (H5Z_class2_t*)infop;
    NC_FILTER_OBJ_HDF5 client;
    if(id == 0 ||infop == NULL)
	return NC_EINVAL;
    memset(&client,0,sizeof(client));
    client.hdr.format = NC_FILTER_FORMAT_HDF5;
    client.sort = NC_FILTER_SORT_CLIENT;
    client.u.client.id = id;
    client.u.client.info = hct;
    /* Note use of a global function, not part of the dispatch table */
    stat = nc4_global_filter_action(NCFILTER_CLIENT_INQ, id, &client);
    if(stat == NC_NOERR) {
	*hct = *(H5Z_class2_t*)client.u.client.info;
    }
#else
    stat = NC_ENOTBUILT;
#endif
    return stat;
}
#endif /*ENABLE_CLIENTSIDE_FILTERS*/

