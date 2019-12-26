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

#ifdef USE_HDF5
#include "hdf5internal.h"
#endif

/*
Common utilities related to filters.
*/


/* Forward */
static int gettype(const int q0, const int q1, int* unsignedp);

/**************************************************/
/*
Parse a filter spec string into a filter id + a vector
of unsigned ints.

@param spec0 - a string containing the spec as a sequence of
              constants separated by commas.
@param idp - store the parsed filter id here
@param nparamsp - store number of parsed filter params here
@param paramsp - store the vector of parsed filter params here
@return 1 if parse succeeded, 0 otherwise.
*/

EXTERNL int
NC_parsefilterspec(const char* spec, unsigned int* idp, size_t* nparamsp, unsigned int** paramsp)
{
    int stat = NC_NOERR;
    int sstat; /* for scanf */
    char* p;
    char* sdata = NULL;
    unsigned int id;
    size_t count; /* no. of comma delimited params */
    size_t nparams; /* final no. of unsigned ints */
    size_t len;
    int i;
    unsigned int* ulist = NULL;
    unsigned char mem[8];

    if(spec == NULL || strlen(spec) == 0) goto fail;
    sdata = strdup(spec);

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
    sstat = sscanf(p,"%u",&id);
    if(sstat != 1) goto fail;
    /* skip past the filter id */
    p = p + strlen(p) + 1;
    count--;

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
	    NC_filterfix8(mem,0);
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
	    NC_filterfix8(mem,0);
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
    if(idp) *idp = id;
    if(nparamsp) *nparamsp = nparams;
    if(paramsp) {
       *paramsp = ulist;
       ulist = NULL; /* avoid duplicate free */
    }
done:
    if(sdata) free(sdata);
    if(ulist) free(ulist);
    return stat;
fail:
    stat = NC_EFILTER;
    goto done;
}

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

EXTERNL void
NC_filterfix8(unsigned char* mem, int decode)
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

/**************************************************/
/* Support direct user defined filters */

EXTERNL int
nc_filter_register(NC_FILTER_INFO* filter)
{
    int stat = NC_NOERR;
    if(filter == NULL)
	return NC_EINVAL;
    switch (filter->format) {
    case NC_FILTER_FORMAT_HDF5:
#ifdef USE_HDF5
	stat = nc4_filter_action(FILTER_REG, filter->format, filter->id, filter);
#else
        stat = NC_ENOTBUILT;
#endif
	break;
    default:
	stat = NC_EINVAL;
    }
    return stat;
}

EXTERNL int
nc_filter_unregister(int fformat, int id)
{
    int stat = NC_NOERR;
    switch (fformat) {
    case NC_FILTER_FORMAT_HDF5:
#ifdef USE_HDF5
	stat = nc4_filter_action(FILTER_UNREG, fformat, id, NULL);
#else
        stat = NC_ENOTBUILT;
#endif
	break;
    default:
	stat = NC_EINVAL;
    }
    return stat;
}

EXTERNL int
nc_filter_inq(int fformat, int id, NC_FILTER_INFO* filter_info)
{
    int stat = NC_NOERR;
    switch (fformat) {
    case NC_FILTER_FORMAT_HDF5:
#ifdef USE_HDF5
	stat = nc4_filter_action(FILTER_INQ, fformat, id, filter_info);
#else
        stat = NC_ENOTBUILT;
#endif
	break;
    default:
	stat = NC_EINVAL;
    }
    return stat;
}
