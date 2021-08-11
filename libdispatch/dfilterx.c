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
#include "ncfilter.h"

#ifdef USE_HDF5
#include "hdf5internal.h"
#endif

/*
Unified filter related code
*/

#ifndef H5Z_FILTER_SZIP
/** ID of HDF SZIP filter. */
#define H5Z_FILTER_SZIP 4
#endif

/*Mnemonic*/
#define USENAME 1

#define LPAREN '('
#define RPAREN ')'
#define LBRACK '['
#define RBRACK ']'

#define NUMCHAR "0123456789"
#define NAMECHAR1 "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define NAMECHARN (NAMECHAR1 NUMCHAR "_-")

const struct FilterName {
    const char* name; /* name or alias as assigned by HDF group*/
    unsigned int id;  /* id as assigned by HDF group*/
} known_filters[] = {
{"zip", 2}, /* Standard zlib compression */
{"zlib", 2}, /* alias */
{"deflate", 2}, /* alias */
{"szip", 4}, /* Standard szip compression */
{"bzip2", 307}, /* BZIP2 lossless compression used by PyTables */
{"lzf", 32000}, /* LZF lossless compression used by H5Py project */
{"blosc", 32001}, /* Blosc lossless compression used by PyTables */
{"mafisc", 32002}, /* Modified LZMA compression filter, MAFISC (Multidimensional Adaptive Filtering Improved Scientific data Compression) */
{"snappy", 32003}, /* Snappy lossless compression. */
{"lz4", 32004}, /* LZ4 fast lossless compression algorithm */
{"apax", 32005}, /* Samplify's APAX Numerical Encoding Technology */
{"cbf", 32006}, /* All imgCIF/CBF compressions and decompressions, including Canonical, Packed, Packed Vesrsion 2, Byte Offset and Nibble Offset. */
{"jpeg-xr", 32007}, /* Enables images to be compressed/decompressed with JPEG-XR compression */
{"bitshuffle", 32008}, /* Extreme version of shuffle filter that shuffles data at bit level instead of byte level. */
{"spdp", 32009}, /* SPDP fast lossless compression algorithm for single- and double-precision floating-point data. */
{"lpc-rice", 32010}, /* LPC-Rice multi-threaded lossless compression */
{"ccsds-123", 32011}, /* ESA CCSDS-123 multi-threaded compression filter */
{"jpeg-ls", 32012}, /* CharLS JPEG-LS multi-threaded compression filter */
{"zfp", 32013}, /* Rate, accuracy or precision bounded compression for floating-point arrays */
{"fpzip", 32014}, /* Fast and Efficient Lossy or Lossless Compressor for Floating-Point Data */
{"zstandard", 32015}, /* Real-time compression algorithm with wide range of compression / speed trade-off and fast decoder */
{"b3d", 32016}, /* GPU based image compression method developed for light-microscopy applications */
{"sz", 32017}, /* An error-bounded lossy compressor for scientific floating-point data */
{"fcidecomp", 32018}, /* EUMETSAT CharLS compression filter for use with netCDF */
{"user-defined", 32768}, /* First user-defined filter */
{NULL,0}
};

/**************************************************/
/*Forward*/
static unsigned int NC_filterx_lookup(const char* filtername);
static const char* NC_filterx_toname(unsigned int id);
static int NC_filterx_transferstringvec(size_t n, char** vec, char** copy);

/**************************************************/
/* Per-variable filters extended string-based */

/**
Find the set of filters (if any) associated with a variable.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID
\param nfilters return no. of filters
\param ids return the filter ids (caller allocates)

\returns ::NC_NOERR No error.
\returns ::NC_ENOTNC4 Not a netCDF-4 file.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Invalid variable ID.
\returns ::NC_EINVAL Invalid arguments
\ingroup variables
\author Dennis Heimbigner
*/
EXTERNL int
nc_inq_var_filterx_ids(int ncid, int varid, size_t* nfiltersp, char** ids)
{
   NC* ncp;
   int stat = NC_NOERR;
   NC_FILTERX_OBJ ncids;

   if((stat = NC_check_id(ncid,&ncp))) goto done;
   TRACE(ncx_inq_var_filterids);

   memset(&ncids,0,sizeof(ncids));
   ncids.usort = NC_FILTER_UNION_IDS;
   ncids.u.ids.nfilters = 0;
   ncids.u.ids.filterids = NULL;
   
   if((stat = ncp->dispatch->filter_actions(ncid,varid, NCFILTER_FILTERIDS, &ncids))) goto done;
   if(nfiltersp) *nfiltersp = ncids.u.ids.nfilters;
   if(ids) {
	if((stat = NC_filterx_transferstringvec(ncids.u.ids.nfilters, ncids.u.ids.filterids, ids))) goto done;
   }

done:
   NC_filterx_freestringvec(ncids.u.ids.nfilters,ncids.u.ids.filterids);
   return stat;
 }

/**
Find the the param info about filter (if any)
associated with a variable and with specified id.

This is a wrapper for nc_inq_var_all().

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID
\param id The filter id of interest
\param formatp (Out) Storage for the filter format
\param nparamsp (Out) Storage which will get the number of parameters to the filter
\param params (Out) Storage which will get associated parameters.
Note: the caller must allocate and free.

\returns ::NC_NOERR No error.
\returns ::NC_ENOTNC4 Not a netCDF-4 file.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Invalid variable ID.
\returns ::NC_ENOFILTER No filter defined.
\ingroup variables
\author Dennis Heimbigner
*/
EXTERNL int
nc_inq_var_filterx_info(int ncid, int varid, const char* id, size_t* nparamsp, char** params)
{
   NC* ncp;
   int stat = NC_NOERR;
   NC_FILTERX_OBJ spec;

   if((stat = NC_check_id(ncid,&ncp))) goto done;
   TRACE(ncx_inq_var_filter_info);

   memset(&spec,0,sizeof(spec));
   spec.usort = NC_FILTER_UNION_SPEC;
   spec.u.spec.filterid = (char*)id;
   spec.u.spec.params = NULL;

   if((stat = ncp->dispatch->filter_actions(ncid,varid,NCFILTER_INFO,&spec))) goto done;
   if(nparamsp) *nparamsp = spec.u.spec.nparams;
   if(params) {
	if((stat = NC_filterx_transferstringvec(spec.u.spec.nparams, spec.u.spec.params, params))) goto done;
   }
done:
   NC_filterx_freestringvec(spec.u.spec.nparams,spec.u.spec.params);
   return stat;
}

/**
   Define a new variable filter.

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
nc_def_var_filterx(int ncid, int varid, const char* id, size_t nparams, const char** params)
{
   NC* ncp;
   NC_FILTERX_OBJ spec;
   int stat = NC_NOERR;
   
   if((stat = NC_check_id(ncid,&ncp))) goto done;
   TRACE(ncx_def_var_filter);

    memset(&spec,0,sizeof(spec));
    spec.usort = NC_FILTER_UNION_SPEC;
    spec.u.spec.filterid = (char*)id;
    spec.u.spec.nparams = nparams;
    spec.u.spec.params = (char**)params; /* discard const */
    if((stat=ncp->dispatch->filter_actions(ncid,varid,NCFILTER_DEF,&spec))) goto done;
done:
    return stat;
}

/**
   Remove all filters with specified id from a variable

   @param ncid File and group ID.
   @param varid Variable ID.
   @param id filter to remove

   @return ::NC_NOERR No error.
   @return ::NC_EBADID Bad ID.
   @author Dennis Heimbigner
*/
EXTERNL int
nc_var_filterx_remove(int ncid, int varid, const char* id)
{
    NC* ncp;
    int stat = NC_NOERR;
    NC_FILTERX_OBJ spec;
    
   if((stat = NC_check_id(ncid,&ncp))) goto done;
   TRACE(ncx_var_filter_remove);

    memset(&spec,0,sizeof(spec));
    spec.usort = NC_FILTER_UNION_SPEC;
    spec.u.spec.filterid = (char*)id;
    if((stat=ncp->dispatch->filter_actions(ncid,varid,NCFILTER_REMOVE,&spec))) goto done;
done:
    return stat;
}

/**************************************************/
/* Utilities */

int
NC_cvtX2I_idlist(size_t n, const char** xidlist, unsigned int* ids)
{
    int i,stat = NC_NOERR;

    for(i=0;i<n;i++) {
	if((stat = NC_cvtX2I_id(xidlist[i],&ids[i]))) break;
    }
    return stat;
}

int
NC_cvtX2I_params(size_t nparams, const char** xparamslist, unsigned int* params)
{
    int i,stat = NC_NOERR;
    unsigned int id;

    for(i=0;i<nparams;i++) {
	/* See if this param looks like an unsigned int */
	if(sscanf(xparamslist[i],"%u",&id) == 1) {
	    params[i] = id;
	}
    }
    return stat;
}

int
NC_cvtI2X_idlist(int n, const unsigned int* ids, char** xid)
{
    int i,stat = NC_NOERR;
    char sid[1024];
    
    /* For now, do not attempt a name conversion */
    for(i=0;i<n;i++) {
	snprintf(sid,sizeof(sid),"%u",ids[i]);
	if((xid[i] = strdup(sid)) == NULL)
	    {stat = NC_ENOMEM; goto done;}
    }
done:
    return stat;
}

int
NC_cvtI2X_params(int n, const unsigned int* ids, char** params)
{
    int i,stat = NC_NOERR;
    char sid[1024];
    
    for(i=0;i<n;i++) {
	snprintf(sid,sizeof(sid),"%u",ids[i]);
	if((params[i] = strdup(sid))==NULL)
	    {stat = NC_ENOMEM; goto done;}

    }
done:
    return stat;
}

/* Convert an xid; allow a name */
int
NC_cvtX2I_id(const char* xid, unsigned int* idp)
{
    unsigned int id;

    /* See if this id looks like an unsigned int */
    if(sscanf(xid,"%u",&id) != 1) {
	id = NC_filterx_lookup(xid);
    }
    if(idp) *idp = id;
    return (id > 0 ? NC_NOERR : NC_EINVAL);
}

/* Convert an int id to a string; optional name conversion */
int
NC_cvtI2X_id(unsigned int id, char** xidp, int usename)
{
    char xid[NC_MAX_NAME];

    snprintf(xid,sizeof(xid),"%u",id);
    if(usename) {/* See if this id has a name */
        const char* name = NC_filterx_toname(id);		
	if(name != NULL) {xid[0] = '\0'; strlcat(xid,name,sizeof(xid));}
    }
    if(xidp) {
    	if((*xidp = strdup(xid))==NULL) return NC_ENOMEM;
    }
    return NC_NOERR;
}

static unsigned int
NC_filterx_lookup(const char* filtername)
{
    const struct FilterName* p = known_filters;
    /* Try a name lookup */
    for(;p->name;p++) {
        if(strcasecmp(p->name,filtername)==0)
	    return p->id;
    }
    return 0; /* no match */
}

static const char*
NC_filterx_toname(unsigned int id)
{
    const struct FilterName* p = known_filters;
    for(;p->name;p++) {
        if(p->id == id)
	    return p->name;
    }
    return NULL; /* no match */
}

void
NC_filterx_freestringvec(size_t n, char** vec)
{
    int i;
    if(vec != NULL) {
        for(i=0;i<n;i++) {
	    if(vec[i]) free(vec[i]);
	}
	free(vec);
    }
}

static int
NC_filterx_transferstringvec(size_t n, char** vec, char** copy)
{
    int i, stat = NC_NOERR;

    for(i=0;i<n;i++) {
	copy[i] = vec[i];
	vec[i] = NULL;
    }
    return stat;
}

int
NC_filterx_copy(size_t n, const char** vec, char*** copyp)
{
    char** copy = NULL;
    int i, stat = NC_NOERR;

    if((copy = calloc(sizeof(char*),n+1))==NULL)
        {stat = NC_ENOMEM; goto done;}
    
    for(i=0;i<n;i++) {
	char* s = nulldup(vec[i]);
	if(s == NULL) {stat = NC_ENOMEM; goto done;}
	copy[i] = s;
    }
    if(copyp) {*copyp = copy; copy = NULL;}
done:
    if(copy) NC_filterx_freestringvec(n,copy);
    return stat;
}

