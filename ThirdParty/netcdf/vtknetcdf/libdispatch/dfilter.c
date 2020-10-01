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

#ifndef H5Z_FILTER_SZIP
/** ID of HDF SZIP filter. */
#define H5Z_FILTER_SZIP 4
#endif

#define LPAREN '('
#define RPAREN ')'
#define LBRACK '['
#define RBRACK ']'

#define NUMCHAR "0123456789"
#define NAMECHAR1 "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define NAMECHARN (NAMECHAR1 NUMCHAR "_-")

/* Forward */
static int gettype(const int q0, const int q1, int* unsignedp);

const struct LegalFormat {
    const char* tag;
    int format;
} legalformats[] = {
    {"hdf5", NC_FILTER_FORMAT_HDF5},
    {NULL, 0},
};

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
/*
Parse a filter spec string into a NC_FILTER_SPEC*

@param txt - a string containing the spec as a sequence of
              constants separated by commas.
@param specp - store the parsed filter here -- caller frees
@return NC_NOERR if parse succeeded
@return NC_EINVAL otherwise
*/

EXTERNL int
NC_parsefilterspec(const char* txt, int format, NC_Filterspec** specp)
{
    int stat = NC_NOERR;
    int sstat; /* for scanf */
    char* p;
    char* sdata0 = NULL; /* what to free */
    char* sdata = NULL; /* sdata0 with leading prefix skipped */
    unsigned int id;
    size_t count; /* no. of comma delimited params */
    size_t nparams; /* final no. of unsigned ints */
    size_t len;
    int i;
    unsigned int* ulist = NULL;
    unsigned char mem[8];

    if(txt == NULL) goto fail;
    len = strlen(txt);
    if(len == 0) goto fail;

    sdata0 = (char*)calloc(1,len+1+1);
    if(sdata0 == NULL) return 0;
    memcpy(sdata0,txt,len);
    sdata = sdata0;

    /* Count number of parameters + id and delimit */
    p=sdata;
    for(count=0;;count++) {
        char* q = strchr(p,',');
	if(q == NULL) break;
	*q++ = '\0';
	p = q;
    }
    count++; /* for final piece */

    if(count == 0)
	goto fail; /* no id and no parameters */

    /* Extract the filter id */
    p = sdata;
    if(strchr(NAMECHAR1,*p) != NULL) {
	const struct FilterName* candidate = known_filters;
	char* q = p+1;
	while(*q && strchr(NAMECHARN,*q) != NULL) {
	    q++;
	}
	if(*q) goto fail; /*name has bad char*/
	/* Lookup name */
	id = 0; /* => not found */
	for(;candidate->name;candidate++) {
	    if(strcasecmp(candidate->name,p)==0) {id = candidate->id; break;}
	}
	if(id == 0) goto fail; /* unknown name */
    } else if(strchr(NUMCHAR,*p) != NULL) {
	sstat = sscanf(p,"%u",&id);
	if(sstat != 1) goto fail;
    } else goto fail; /* unparseable id */
    count--;

    /* skip past the filter id */
    p = p + strlen(p) + 1;

    /* Allocate the max needed space; *2 in case the params are all doubles */
    ulist = (unsigned int*)malloc(sizeof(unsigned int)*(count)*2);
    if(ulist == NULL) goto fail;

    /* walk and convert */
    nparams = 0; /* actual count */
    for(i=0;i<count;i++) { /* step thru param strings */
	unsigned long long val64u;
	unsigned int val32u;
	double vald;
	float valf;
	unsigned int *vector;
	int isunsigned = 0;
	int isnegative = 0;
	int type = 0;
	char* q;

	len = strlen(p);
	/* skip leading white space */
	while(strchr(" 	",*p) != NULL) {p++; len--;}
	/* Get leading sign character, if any */
	if(*p == '-') isnegative = 1;
        /* Get trailing type tag characters */
	switch (len) {
	case 0:
	    goto fail; /* empty parameter */
	case 1:
	case 2:
	    q = (p + len) - 1; /* point to last char */
	    type = gettype(*q,'\0',&isunsigned);
	    break;
	default: /* > 2 => we might have a two letter tag */
	    q = (p + len) - 2;
	    type = gettype(*q,*(q+1),&isunsigned);
	    break;
	}

	/* Now parse */
	switch (type) {
	case 'b':
	case 's':
	case 'i':
 	    /* special case for a positive integer;for back compatibility.*/
	    if(!isnegative)
	        sstat = sscanf(p,"%u",&val32u);
	    else
                sstat = sscanf(p,"%d",(int*)&val32u);
	    if(sstat != 1) goto fail;
	    switch(type) {
	    case 'b': val32u = (val32u & 0xFF); break;
	    case 's': val32u = (val32u & 0xFFFF); break;
	    }
	    ulist[nparams++] = val32u;
	    break;

	case 'f':
	    sstat = sscanf(p,"%lf",&vald);
	    if(sstat != 1) goto fail;
	    valf = (float)vald;
	    ulist[nparams++] = *(unsigned int*)&valf;
	    break;

	/* The following are 8-byte values, so we must swap pieces if this
           is a little endian machine */	
	case 'd':
	    sstat = sscanf(p,"%lf",&vald);
	    if(sstat != 1) goto fail;
	    memcpy(mem,&vald,sizeof(mem));
	    NC4_filterfix8(mem,0);
	    vector = (unsigned int*)mem;
	    ulist[nparams++] = vector[0];
	    ulist[nparams++] = vector[1];
	    break;
	case 'l': /* long long */
	    if(isunsigned)
	        sstat = sscanf(p,"%llu",&val64u);
	    else
                sstat = sscanf(p,"%lld",(long long*)&val64u);
	    if(sstat != 1) goto fail;
	    memcpy(mem,&val64u,sizeof(mem));
	    NC4_filterfix8(mem,0);
	    vector = (unsigned int*)&mem;
	    ulist[nparams++] = vector[0];
	    ulist[nparams++] = vector[1];
	    break;
	default:
	    goto fail;
	}
        p = p + strlen(p) + 1; /* move to next param */
    }
    /* Now return results */
    if(specp != NULL) {
        NC4_Filterspec* pfs = calloc(1,sizeof(NC4_Filterspec));
        if(pfs == NULL) {stat = NC_ENOMEM; goto done;}
        pfs->hdr.hdr.format = format;
        pfs->filterid = id;
        pfs->nparams = nparams;
        pfs->params = ulist; ulist = NULL;
	*specp = (NC_Filterspec*)pfs;
    }

done:
    if(sdata) free(sdata);
    if(ulist) free(ulist);
    return stat;
fail:
    stat = NC_EINVAL;
    goto done;
}

/*
Parse a string containing multiple '|' separated filter specs.

@param spec0 - a string containing the list of filter specs.
@param nspecsp - # of parsed specs
@param specsp - pointer to hold vector of parsed specs. Caller frees
@return NC_NOERR if parse succeeded
@return NC_EINVAL if bad parameters or parse failed
*/

EXTERNL int
NC_parsefilterlist(const char* txt0, int* formatp, size_t* nspecsp, NC_Filterspec*** vectorp)
{
    int stat = NC_NOERR;
    int format = NC_FILTER_FORMAT_HDF5; /* default */
    size_t len = 0;
    size_t nspecs = 0;
    NC4_Filterspec** vector = NULL;
    char* spec0 = NULL; /* with prefix */
    char* spec = NULL; /* without prefix */
    char* p = NULL;
    char* q = NULL;

    if(txt0  == NULL) return NC_EINVAL;
    /* Duplicate txt0 so we can modify it */
    len = strlen(txt0);
    if((spec = calloc(1,len+1+1)) == NULL) return NC_ENOMEM;
    memcpy(spec,txt0,len); /* Note double ending nul */
    spec0 = spec; /* Save for later free */

    /* See if there is a prefix '[format]' tag */
    if(spec[0] == LBRACK) {
	int found = 0;
        const struct LegalFormat* candidates = legalformats;
	p = spec + 1;
	q = strchr(p,RBRACK);
	if(q == NULL) {stat = NC_EINVAL; goto done;}
	*q++ = '\0'; /* delimit tag */
	while(candidates->tag != NULL) {
	    if(strcasecmp(p,candidates->tag) == 0) {
		found = 1;
		format = candidates->format;
	    }
	}
        if(found == 0) {stat = NC_EINVAL; goto done;}
	spec = q; /* skip tag wrt later processing */
    }
    if(formatp) *formatp = format;

    if(format != NC_FILTER_FORMAT_HDF5) {stat = NC_EINVAL; goto done;} /* for now */

    /* pass 1: count number of specs */
    p = spec;
    nspecs = 0;
    while(*p) {
	q = strchr(p,'|');
	if(q == NULL) q = p + strlen(p); /* fake it */
	nspecs++;
	p = q + 1;
    }
    if(nspecs >  0) {
	int count = 0;
	if((vector = (NC4_Filterspec**)malloc(sizeof(NC4_Filterspec*)*nspecs)) == NULL)
	    {stat = NC_ENOMEM; goto done;}
	/* pass 2: parse */
	p = spec;
	for(count=0;count<nspecs;count++) {
	    NC4_Filterspec* aspec = NULL;
	    q = strchr(p,'|');
	    if(q == NULL) q = p + strlen(p); /* fake it */
	    *q = '\0';
	    if(NC_parsefilterspec(p,format,(NC_Filterspec**)&aspec))
	        {stat = NC_EINVAL; goto done;}
	    vector[count] = aspec; aspec = NULL;
	    p = q+1; /* ok because of double nul */
	}
    }
    if(nspecsp) *nspecsp = nspecs;
    if(vectorp) *vectorp = (nspecs == 0 ? NULL : (NC_Filterspec**)vector);
    vector = NULL;
done:
    nullfree(spec0);
    if(vector != NULL) {
	int k;
	for(k=0;k<nspecs;k++) {
	    NC4_Filterspec* nfs = vector[k];
	    if(nfs->params) free(nfs->params);
	    nullfree(nfs);
	}
	free(vector);
    }
    return stat;
}

EXTERNL void
NC4_filterfix8(unsigned char* mem, int decode)
{
#ifdef WORDS_BIGENDIAN
    if(decode) { /* Apply inverse of the encode case */
	byteswap4(mem); /* step 1: byte-swap each piece */
	byteswap4(mem+4);
	byteswap8(mem); /* step 2: convert to little endian format */
    } else { /* encode */
	byteswap8(mem); /* step 1: convert to little endian format */
	byteswap4(mem); /* step 2: byte-swap each piece */
	byteswap4(mem+4);
    }
#else /* Little endian */
    /* No action is necessary */
#endif	    
}


/* Support direct user defined filters */

/* Use void* to avoid having to include hdf.h*/
EXTERNL int
nc_filter_client_register(unsigned int id, void* info)
{
    int stat = NC_NOERR;
    if(id == 0 ||info == NULL)
	return NC_EINVAL;
#ifdef USE_HDF5
    NC_FILTER_OBJ_HDF5 client;
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
nc_inq_var_filterids(int ncid, int varid, size_t* nfiltersp, unsigned int* ids)
{
   NC* ncp;
   int stat = NC_check_id(ncid,&ncp);
   NC_FILTER_OBJ_HDF5 ncids;

   if(stat != NC_NOERR) return stat;
   TRACE(nc_inq_var_filterids);

   memset(&ncids,0,sizeof(ncids));
   ncids.hdr.format = NC_FILTER_FORMAT_HDF5;
   ncids.sort = NC_FILTER_SORT_IDS;
   ncids.u.ids.nfilters = (nfiltersp?*nfiltersp:0);
   ncids.u.ids.filterids = ids;
   
   if((stat = ncp->dispatch->filter_actions(ncid,varid, NCFILTER_FILTERIDS, (NC_Filterobject*)&ncids)) == NC_NOERR) {
       if(nfiltersp) *nfiltersp = ncids.u.ids.nfilters;
   }
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
nc_inq_var_filter_info(int ncid, int varid, unsigned int id, size_t* nparamsp, unsigned int* params)
{
   NC* ncp;
   int stat = NC_check_id(ncid,&ncp);
   NC_FILTER_OBJ_HDF5 spec;

   if(stat != NC_NOERR) return stat;
   TRACE(nc_inq_var_filter_info_hdf5);

   memset(&spec,0,sizeof(spec));
   spec.hdr.format = NC_FILTER_FORMAT_HDF5;
   spec.sort = NC_FILTER_SORT_SPEC;
   spec.u.spec.filterid = id;
   spec.u.spec.nparams = (nparamsp?*nparamsp:0);
   spec.u.spec.params = params;

   if((stat = ncp->dispatch->filter_actions(ncid,varid,NCFILTER_INFO,(NC_Filterobject*)&spec)) == NC_NOERR) {
       if(nparamsp) *nparamsp = spec.u.spec.nparams;
   }
   return stat;
}

/**
Find the first filter (if any) associated with a variable.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param idp Storage which will get the filter id; a return value of zero means no filter

\param nparamsp Storage which will get the number of parameters to the
filter

\param params Storage which will get associated parameters.
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
nc_inq_var_filter(int ncid, int varid, unsigned int* idp, size_t* nparamsp, unsigned int* params)
{
   NC* ncp;
   NC_FILTER_OBJ_HDF5 spec;
   int stat = NC_check_id(ncid,&ncp);

   if(stat != NC_NOERR) return stat;
   TRACE(nc_inq_var_filter);

   memset(&spec,0,sizeof(spec));
   spec.hdr.format = NC_FILTER_FORMAT_HDF5;
   spec.sort = NC_FILTER_SORT_SPEC;
   spec.u.spec.filterid = (idp?*idp:0);
   spec.u.spec.nparams = (nparamsp?*nparamsp:0);
   spec.u.spec.params = params;

   if((stat=ncp->dispatch->filter_actions(ncid,varid,NCFILTER_INQ,(NC_Filterobject*)&spec)))
      return stat;
   if(idp) *idp = spec.u.spec.filterid;
   if(nparamsp) *nparamsp = spec.u.spec.nparams;
   return stat;
}

/**
   Define a new variable hdf5 filter.

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
    NC_FILTER_OBJ_HDF5 spec;
    int stat = NC_check_id(ncid,&ncp);
   
    if(stat != NC_NOERR) return stat;
    TRACE(nc_def_var_filter_hdf5);

    memset(&spec,0,sizeof(spec));
    spec.hdr.format = NC_FILTER_FORMAT_HDF5;
    spec.sort = NC_FILTER_SORT_SPEC;
    spec.u.spec.filterid = id;
    spec.u.spec.nparams = nparams;
    spec.u.spec.params = (unsigned int*)params; /* discard const */
    return ncp->dispatch->filter_actions(ncid,varid,NCFILTER_DEF,(NC_Filterobject*)&spec);
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
nc_var_filter_remove(int ncid, int varid, unsigned int id)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    NC_FILTER_OBJ_HDF5 spec;
    
    if(stat != NC_NOERR) return stat;
    TRACE(nc_var_filter_hdf5_remove);

    memset(&spec,0,sizeof(spec));
    spec.hdr.format = NC_FILTER_FORMAT_HDF5;
    spec.sort = NC_FILTER_SORT_SPEC;
    spec.u.spec.filterid = id;
    return ncp->dispatch->filter_actions(ncid,varid,NCFILTER_REMOVE,(NC_Filterobject*)&spec);
}

/**************************************************/
/* Utilities */

/* Look at q0 and q1) to determine type */
static int
gettype(const int q0, const int q1, int* isunsignedp)
{
    int type = 0;
    int isunsigned = 0;
    char typechar;
    
    isunsigned = (q0 == 'u' || q0 == 'U');
    if(q1 == '\0')
	typechar = q0; /* we were given only a single char */
    else if(isunsigned)
	typechar = q1; /* we have something like Ux as the tag */
    else
	typechar = q1; /* look at last char for tag */
    switch (typechar) {
    case 'f': case 'F': case '.': type = 'f'; break; /* float */
    case 'd': case 'D': type = 'd'; break; /* double */
    case 'b': case 'B': type = 'b'; break; /* byte */
    case 's': case 'S': type = 's'; break; /* short */
    case 'l': case 'L': type = 'l'; break; /* long long */
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': type = 'i'; break;
    case 'u': case 'U': type = 'i'; isunsigned = 1; break; /* unsigned int */
    case '\0': type = 'i'; break;
    default: break;
    }
    if(isunsignedp) *isunsignedp = isunsigned;
    return type;
}

#ifdef WORDS_BIGENDIAN
/* Byte swap an 8-byte integer in place */
static void
byteswap8(unsigned char* mem)
{
    unsigned char c;
    c = mem[0];
    mem[0] = mem[7];
    mem[7] = c;
    c = mem[1];
    mem[1] = mem[6];
    mem[6] = c;
    c = mem[2];
    mem[2] = mem[5];
    mem[5] = c;
    c = mem[3];
    mem[3] = mem[4];
    mem[4] = c;
}

/* Byte swap an 8-byte integer in place */
static void
byteswap4(unsigned char* mem)
{
    unsigned char c;
    c = mem[0];
    mem[0] = mem[3];
    mem[3] = c;
    c = mem[1];
    mem[1] = mem[2];
    mem[2] = c;
}
#endif


