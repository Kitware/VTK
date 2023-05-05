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

#include "ncjson.h"

/*
NCZarr filter API
*/

/**************************************************/
/* Per-variable filters */

/**
Find the set of filters (if any) associated with a variable.
Assumes NCZarr format using json

\param ncid NetCDF or group ID, from a previous call to nc_open(),
\param varid Variable ID
\param jsonp a JSON formatted string is returned in this argument

\returns ::NC_NOERR No error.
\returns ::NC_ENOTNC4 Not a netCDF-4 file.
\returns ::NC_EBADID Bad ncid
\returns ::NC_ENOTVAR Invalid variable ID.
\returns ::NC_EINVAL Invalid arguments
\ingroup variables
\author Dennis Heimbigner
*/
EXTERNL int
nc_inq_var_filterx_ids(int ncid, int varid, char** textp)
{
    NC* ncp;
    int stat = NC_NOERR;

    TRACE(nc_inq_var_filterx_ids);
    if((stat = NC_check_id(ncid,&ncp))) return stat;
    if((stat = ncp->dispatch->inq_var_filterx_ids(ncid,varid,textp))) goto done;

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
nc_inq_var_filterx_info(int ncid, int varid, const char* id, char** textp)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);

    TRACE(nc_inq_var_filterx_info);
    if(stat != NC_NOERR) return stat;
    if((stat = ncp->dispatch->inq_var_filterx_info(ncid,varid,id,textp))) goto done;

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
nc_def_var_filterx(int ncid, int varid, const char* json)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);

    TRACE(nc_def_var_filterx);
    if(stat != NC_NOERR) return stat;
    if((stat = ncp->dispatch->def_var_filterx(ncid,varid,json))) goto done;

done:
    return stat;
}

/**
Find the first filter (if any) associated with a variable.
   
\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param textp Storage which will get the filter info (id + parameters) in json format

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
nc_inq_var_filterx(int ncid, int varid, char** textp)
{
    NC* ncp;
    int stat = NC_NOERR;
    char* text = NULL;
    NCjson* json = NULL;
    NCjson* jid = NULL;
    
    TRACE(nc_inq_var_filterx);
    if((stat = NC_check_id(ncid,&ncp))) goto done;

    /* Get the filters on this variable */
    if((stat = nc_inq_var_filterx_ids(ncid,varid,&text))) goto done;
    /* Parse it */
    if((stat = NCJparse(text,0,&json))) goto done;
    if(json->sort != NCJ_ARRAY)
        {stat = NC_EFILTER; goto done;}
    if(NCJlength(json) == 0 || NCJcontents(json) == NULL)
        {stat = NC_ENOFILTER; goto done;}    
    jid = NCJith(json,0);
    if(jid->sort == NCJ_DICT || jid->sort == NCJ_ARRAY)
        {stat = NC_EFILTER; goto done;}
    /* Get info about the first filter */
    if((stat = nc_inq_var_filterx_info(ncid,varid,NCJstring(jid),textp)))
        {stat = NC_ENOFILTER; goto done;}
 done:
    NCJreclaim(json);
    return stat;
}

/**************************************************/
/* Support direct user defined filters */

#ifdef ENABLE_CLIENTSIDE_FILTERS

/* Use void* to avoid having to include hdf.h*/
EXTERNL int
nc_filterx_client_register(unsigned int id, void* info)
{
    int stat = NC_NOERR;
#ifdef USE_HDF5
    NC_FILTERX_OBJ_HDF5 client;
    if(id == 0 ||info == NULL)
	return NC_EINVAL;
    memset(&client,0,sizeof(client));
    client.hdr.format = NC_FILTERX_FORMAT_HDF5;
    client.sort = NC_FILTERX_SORT_CLIENT;
    client.u.client.id = id;
    client.u.client.info = info;
    /* Note use of a global function, not part of the dispatch table */
    stat = nc4_global_filterx_action(NCFILTER_CLIENT_REG, id, &client);
#else
    stat = NC_ENOTBUILT;
#endif
    return stat;
}

EXTERNL int
nc_filterx_client_unregister(unsigned int id)
{
int stat = NC_NOERR;
#ifdef USE_HDF5
    stat = nc4_global_filterx_action(NCFILTER_CLIENT_UNREG, id, NULL);
#else
    stat = NC_ENOTBUILT;
#endif
    return stat;
}

/* Use void* to avoid having to include hdf.h*/
EXTERNL int
nc_filterx_client_inq(unsigned int id, void* infop)
{
int stat = NC_NOERR;
#ifdef USE_HDF5
    H5Z_class2_t* hct = (H5Z_class2_t*)infop;
    NC_FILTERX_OBJ_HDF5 client;
    if(id == 0 ||infop == NULL)
	return NC_EINVAL;
    memset(&client,0,sizeof(client));
    client.hdr.format = NC_FILTERX_FORMAT_HDF5;
    client.sort = NC_FILTERX_SORT_CLIENT;
    client.u.client.id = id;
    client.u.client.info = hct;
    /* Note use of a global function, not part of the dispatch table */
    stat = nc4_global_filterx_action(NCFILTER_CLIENT_INQ, id, &client);
    if(stat == NC_NOERR) {
	*hct = *(H5Z_class2_t*)client.u.client.info;
    }
#else
    stat = NC_ENOTBUILT;
#endif
    return stat;
}
#endif /*ENABLE_CLIENTSIDE_FILTERS*/
