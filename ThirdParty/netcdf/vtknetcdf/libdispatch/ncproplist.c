/*********************************************************************
 *   Copyright 2018, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Header$
 *********************************************************************/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "ncdispatch.h"
#include "nccrc.h"
#include "ncproplist.h"

#undef DEBUG
#define ASSERTIONS

#ifdef ASSERTIONS
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif

/**************************************************/
			
#define MINPROPS 2
#define EXPANDFACTOR 1

#define hasspace(plist,nelems) ((plist)->alloc >= ((plist)->count + (nelems)))

#define emptyprop {"                               ",0,0,0,NULL}

/**************************************************/

/* Forward */
static int ncproplistinit(NCproplist* plist);
static int extendplist(NCproplist* plist, size_t nprops);

/* Static'ize everything for plugins */
#ifdef NETCDF_PROPLIST_H
#define OPTSTATIC static
static NCproplist* ncproplistnew(void);
static int ncproplistfree(NCproplist* plist);
static int ncproplistadd(NCproplist* plist, const char* key, uintptr_t value);
static int ncproplistaddbytes(NCproplist* plist, const char* key, void* value, uintptr_t size);
static int ncproplistaddstring(NCproplist* plist, const char* key, const char* str);
static int ncproplistaddx(NCproplist* plist, const char* key, void* value, uintptr_t size, uintptr_t userdata, NCPreclaimfcn fcn);
static int ncproplistclone(const NCproplist* src, NCproplist* clone);
static int ncproplistget(const NCproplist* plist, const char* key, uintptr_t* valuep, uintptr_t* sizep);
static int ncproplistith(const NCproplist* plist, size_t i, char* const * keyp, uintptr_t const * valuep, uintptr_t* sizep);
#else /*!NETCDF_PROPLIST_H*/
#define OPTSTATIC
#endif /*NETCDF_PROPLIST_H*/


/**
 * Create new property list
 * @return pointer to the created property list.
 */
OPTSTATIC NCproplist*
ncproplistnew(void)
{
   NCproplist* plist = NULL;
   plist = calloc(1,sizeof(NCproplist));
   if(ncproplistinit(plist) != NC_NOERR)
       {ncproplistfree(plist); plist = NULL;}
   return plist;
}

/**
 * Reclaim memory used by a property list
 * @param plist to reclaim
 * @return NC_NOERR if succeed, NC_EXXX otherwise.
 */
OPTSTATIC int
ncproplistfree(NCproplist* plist)
{
    int stat = NC_NOERR;
    size_t i;
    if(plist == NULL) goto done;
    if(plist->properties != NULL) {
        for(i=0;i<plist->count;i++) {
            NCProperty* prop = &plist->properties[i];
	    void* ptr = (void*)prop->value; /* convert to ptr */
	    assert(prop->flags & (NCPF_SIMPLE|NCPF_BYTES|NCPF_COMPLEX));
	    if(prop->flags & NCPF_SIMPLE) continue; /* no reclaim needed */
    	    if(prop->flags & NCPF_BYTES) {
		if(ptr != NULL) free(ptr);
	    } else { /* (prop->flags & NCPF_COMPLEX) */
		int ok;
		assert(prop->reclaim != NULL);
		ok = prop->reclaim(prop->userdata, prop->key, ptr, prop->size);
		if(!ok && stat == NC_NOERR) stat = NC_EINVAL;
	    }	
	}
	free(plist->properties);
    }
    free(plist);
done:
    return stat;
}

/**
 * Add a non-reclaimable entry to the property list
 * @param plist into which the value is be inserted.
 * @param key
 * @param value
 * @return NC_NOERR if succeed, NC_EXXX otherwise.
 */
OPTSTATIC int
ncproplistadd(NCproplist* plist, const char* key, uintptr_t value)
{
    int stat = NC_NOERR;
    NCProperty* prop = NULL;
    size_t keylen;
    if(plist == NULL) goto done;
    if(!hasspace(plist,1)) {if((stat = extendplist(plist,(plist->count+1)*EXPANDFACTOR))) goto done;} /* extra space */
    prop = &plist->properties[plist->count];
    keylen = strlen(key);
    if(keylen > NCPROPSMAXKEY) keylen = NCPROPSMAXKEY; /* truncate */
    memcpy(prop->key,key,keylen);
    prop->key[keylen] = '\0';
    prop->value = value;
    prop->flags = NCPF_SIMPLE;
    plist->count++;
done:
    return stat;
}

/**
 * Add a reclaimable entry to the property list, where the value
 * can be reclaimed using a simple free();
 * @param plist into which the value is be inserted.
 * @param key
 * @param value ptr to memory chunk
 * @param size |*value|
 * @return NC_NOERR if succeed, NC_EXXX otherwise.
 */
OPTSTATIC int
ncproplistaddbytes(NCproplist* plist, const char* key, void* value, uintptr_t size)
{
    int stat = NC_NOERR;
    NCProperty* prop = NULL;
    size_t keylen;
    if(plist == NULL) goto done;
    if(!hasspace(plist,1)) {if((stat = extendplist(plist,(plist->count+1)*EXPANDFACTOR))) goto done;} /* extra space */
    prop = &plist->properties[plist->count];
    keylen = strlen(key);
    if(keylen > NCPROPSMAXKEY) keylen = NCPROPSMAXKEY; /* truncate */
    memcpy(prop->key,key,keylen);
    prop->key[keylen] = '\0';
    prop->value = (uintptr_t)value;
    prop->flags = NCPF_BYTES;
    plist->count++;
done:
    return stat;
}

/**
 * Add a reclaimable entry to the property list, where the value
 * can be reclaimed using a simple free();
 * @param plist into which the value is be inserted.
 * @param key
 * @param value ptr to memory chunk
 * @param size |*value|
 * @return NC_NOERR if succeed, NC_EXXX otherwise.
 */
OPTSTATIC int
ncproplistaddstring(NCproplist* plist, const char* key, const char* str)
{
    uintptr_t size = 0;
    if(str) size = (uintptr_t)strlen(str);
    return ncproplistaddbytes(plist,key,(void*)str,size);
}

/**
 * Most general case for adding a property.
 * @param plist into which the value is be inserted.
 * @param key
 * @param value
 * @param size
 * @param userdata extra environment data for the reclaim function.
 * @param fcn the reclaim function
 * @return NC_NOERR if succeed, NC_EXXX otherwise.
 */
OPTSTATIC int
ncproplistaddx(NCproplist* plist, const char* key, void* value, uintptr_t size, uintptr_t userdata, NCPreclaimfcn fcn)
{
    int stat = NC_NOERR;
    NCProperty* prop = NULL;
    size_t keylen;
    if(plist == NULL) goto done;
    if(!hasspace(plist,1)) {if((stat = extendplist(plist,(plist->count+1)*EXPANDFACTOR))) goto done;} /* extra space */
    prop = &plist->properties[plist->count];
    keylen = strlen(key);
    if(keylen > NCPROPSMAXKEY) keylen = NCPROPSMAXKEY; /* truncate */
    memcpy(prop->key,key,keylen);
    prop->key[keylen] = '\0';
    prop->value = (uintptr_t)value;
    prop->size = size;
    prop->reclaim = fcn;
    prop->userdata = userdata;
    prop->flags = NCPF_COMPLEX;
    plist->count++;
done:
    return stat;
}

OPTSTATIC int
ncproplistclone(const NCproplist* src, NCproplist* clone)
{
    int stat = NC_NOERR;
    size_t i;
    NCProperty* srcprops;
    NCProperty* cloneprops;

    if(src == NULL || clone == NULL) {stat = NC_EINVAL; goto done;}
    if((stat=ncproplistinit(clone))) goto done;
    if((stat=extendplist(clone,src->count))) goto done;
    srcprops = src->properties;
    cloneprops = clone->properties;
    for(i=0;i<src->count;i++) {
	cloneprops[i] = srcprops[i];
	strncpy(cloneprops[i].key,srcprops[i].key,sizeof(cloneprops[i].key));
#if 0
	cloneprops[i]->flags = srcprops[i]->flags;
	cloneprops[i]->value = srcprops[i]->value;
	cloneprops[i]->size = srcprops[i]->size;
	cloneprops[i]->userdata = srcprops[i]->userdata;
	cloneprops[i]->reclaim = srcprops->reclaim;
#endif
    }
done:
    return stat;
}

/* Increase size of a plist to be at lease nprops properties */
static int
extendplist(NCproplist* plist, size_t nprops)
{
    int stat = NC_NOERR;
    size_t newsize = plist->count + nprops;
    NCProperty* newlist = NULL;
    if((plist->alloc >= newsize) || (nprops == 0))
	goto done; /* Already enough space */
    newlist = realloc(plist->properties,newsize*sizeof(NCProperty));
    if(newlist == NULL) {stat = NC_ENOMEM; goto done;}
    plist->properties = newlist; newlist = NULL;    
    plist->alloc = newsize;
done:
    return stat;
}

/**
 * Lookup key and return value and size
 * @param plist to search
 * @param key for which to search
 * @param valuep returned value
 * @param sizep returned size
 * @return NC_NOERR if key found, NC_ENOOBJECT if key not found; NC_EXXX otherwise
 */
OPTSTATIC int
ncproplistget(const NCproplist* plist, const char* key, uintptr_t* valuep, uintptr_t* sizep)
{
    int stat = NC_ENOOBJECT; /* assume not found til proven otherwise */
    size_t i;
    NCProperty* props;
    uintptr_t value = 0;
    uintptr_t size = 0;
    if(plist == NULL || key == NULL) goto done;
    for(i=0,props=plist->properties;i<plist->count;i++,props++) {
	if(strcmp(props->key,key)==0) {
	    value = props->value;
	    size = props->size;	    
	    stat = NC_NOERR; /* found */
	    break;
	}
    }
    if(valuep) *valuep = value;
    if(sizep) *sizep = size;
done:
    return stat;
}

/* Iteration support */

/**
 * Get the ith key+value.a
 * @param plist to search
 * @param i which property to get.
 * @param keyp return i'th key
 * @param valuep return i'th value
 * @param valuep return i'th size
 * @return NC_NOERR if success, NC_EINVAL otherwise
 */
OPTSTATIC int
ncproplistith(const NCproplist* plist, size_t i, char* const * keyp, uintptr_t const * valuep, uintptr_t* sizep)
{
    int stat = NC_NOERR;
    NCProperty* prop = NULL;    
    if(plist == NULL) goto done;
    if(i >= plist->count) {stat = NC_EINVAL; goto done;}
    prop = &plist->properties[i];
    if(keyp) *((char**)keyp) = (char*)prop->key;
    if(valuep) *((uintptr_t*)valuep) = (uintptr_t)prop->value;
    if(sizep) *sizep = prop->size;
done:
    return stat;
}

/**************************************************/
/* Support Functions */

/**
 * Initialize a new property list 
 */
static int
ncproplistinit(NCproplist* plist)
{
   /* Assume property list will hold at lease MINPROPS properties */
   plist->alloc = MINPROPS;
   plist->count = 0;
   plist->properties = (NCProperty*)calloc(MINPROPS,sizeof(NCProperty));
   return (plist->properties?NC_NOERR:NC_ENOMEM);
}

/* Suppress unused statics warning */
static void
ncproplist_unused(void)
{
    void* unused = ncproplist_unused;
    unused = ncproplistnew;
    unused = ncproplistfree;
    unused = ncproplistadd;
    unused = ncproplistaddbytes;
    unused = ncproplistaddstring;
    unused = ncproplistaddx;
    unused = ncproplistclone;
    unused = ncproplistget;
    unused = ncproplistith;
    unused = ncproplistinit;
    unused = (void*)ncproplistith;
    unused = unused;
}

