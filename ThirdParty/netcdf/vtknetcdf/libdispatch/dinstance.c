/*
Copyright (c) 1998-2018 University Corporation for Atmospheric Research/Unidata
See COPYRIGHT for license information.
*/

/*
This file contains various instance operations that operate
on a deep level rather than the shallow level of e.g. nc_free_vlen_t.
Currently two operations are defined:
1. reclaim a vector of instances
2. copy a vector of instances
*/

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "netcdf.h"
#include "nc4internal.h"
#include "nc4dispatch.h"
#include "ncoffsets.h"
#include "ncbytes.h"

#undef REPORT
#undef DEBUG


/* It is helpful to have a structure that contains memory and an offset */
typedef struct Position{char* memory; ptrdiff_t offset;} Position;

static int type_alignment_initialized = 0;

/* Forward */
#ifdef USE_NETCDF4
static int reclaim_datar(int ncid, nc_type xtype, Position*);
static int reclaim_compound(int ncid, nc_type xtype, size_t size, size_t nfields, Position* offset);
static int reclaim_vlen(int ncid, nc_type xtype, nc_type basetype, Position* offset);
static int reclaim_enum(int ncid, nc_type xtype, nc_type basetype, Position* offset);
static int reclaim_opaque(int ncid, nc_type xtype, size_t size, Position* offset);

static int copy_datar(int ncid, nc_type xtype, Position* src, Position* dst);
static int copy_compound(int ncid, nc_type xtype, size_t size, size_t nfields, Position* src, Position* dst);
static int copy_vlen(int ncid, nc_type xtype, nc_type basetype, Position* src, Position* dst);
static int copy_enum(int ncid, nc_type xtype, nc_type basetype, Position* src, Position* dst);
static int copy_opaque(int ncid, nc_type xtype, size_t size, Position* src,Position* dst);

static int dump_datar(int ncid, nc_type xtype, Position*, NCbytes* buf);
static int dump_compound(int ncid, nc_type xtype, size_t size, size_t nfields, Position* offset, NCbytes* buf);
static int dump_vlen(int ncid, nc_type xtype, nc_type basetype, Position* offset, NCbytes* buf);
static int dump_enum(int ncid, nc_type xtype, nc_type basetype, Position* offset, NCbytes* buf);
static int dump_opaque(int ncid, nc_type xtype, size_t size, Position* offset, NCbytes* buf);

static ptrdiff_t read_align(ptrdiff_t offset, size_t alignment);
#endif

static int NC_inq_any_type(int ncid, nc_type typeid, char *name, size_t *size, nc_type *basetypep, size_t *nfieldsp, int *classp);

/**

Reclaim a vector of instances of a type.  This improves upon
e.g. nc_free_vlen.  This recursively walks the top-level
instances to reclaim any nested data such as vlen or strings or such.

Assumes it is passed a pointer to count instances of xtype.
Reclaims any nested data.

WARNING: This needs access to the type metadata of the file, so
a valid ncid and typeid must be available, which means the file
must not have been closed or aborted.

WARNING: DOES NOT RECLAIM THE TOP-LEVEL MEMORY (see the
nc_reclaim_data_all function).  The reason is that we do not
know how it was allocated (e.g. static vs dynamic); only the
caller can know that.  But note that it assumes all memory
blocks other than the top were dynamically allocated, so they
will be free'd.

Should work for any netcdf format.

@param ncid root id
@param xtype type id
@param memory ptr to top-level memory to reclaim
@param count number of instances of the type in memory block
@return error code
*/

int
nc_reclaim_data(int ncid, nc_type xtype, void* memory, size_t count)
{
    int stat = NC_NOERR;
    size_t i;
    Position offset;
    int isf;

    if(ncid < 0 || xtype <= 0)
        {stat = NC_EINVAL; goto done;}
    if(memory == NULL && count > 0)
        {stat = NC_EINVAL; goto done;}
    if(memory == NULL || count == 0)
        goto done; /* ok, do nothing */
#ifdef REPORT
    fprintf(stderr,">>> reclaim: memory=%p count=%lu ncid=%d xtype=%d\n",memory,(unsigned long)count,ncid,xtype);
#endif

    /* Optimizations */
    /* 1. Vector of fixed size types */
    if((stat = NC4_inq_type_fixed_size(ncid,xtype,&isf))) goto done;
    if(isf) goto done; /* no need to reclaim anything */

#ifdef USE_NETCDF4
    /* 2.Vector of strings */
    if(xtype == NC_STRING) {
	char** ss = (char**)memory;
        for(i=0;i<count;i++) {
	    nullfree(ss[i]);
	}
        goto done;
    }
    offset.memory = (char*)memory; /* use char* so we can do pointer arithmetic */
    offset.offset = 0;
    for(i=0;i<count;i++) {
        if((stat=reclaim_datar(ncid,xtype,&offset))) /* reclaim one instance */
	    break;
    }
#else
    stat = NC_EBADTYPE;
#endif

done:
    return stat;
}

/* Alternate entry point: includes recovering the top-level memory */
int
nc_reclaim_data_all(int ncid, nc_type xtypeid, void* memory, size_t count)
{
    int stat = NC_NOERR;
    stat = nc_reclaim_data(ncid,xtypeid,memory,count);
    if(stat == NC_NOERR && memory != NULL)
        free(memory);
    return stat;
}

#ifdef USE_NETCDF4
/* Recursive type walker: reclaim a single instance */
static int
reclaim_datar(int ncid, nc_type xtype, Position* offset)
{
    int stat = NC_NOERR;
    size_t xsize;
    nc_type basetype;
    size_t nfields;
    int klass, isf;

    if((stat = NC4_inq_type_fixed_size(ncid,xtype,&isf))) goto done;

    /* Get relevant type info */
    if((stat = NC_inq_any_type(ncid,xtype,NULL,&xsize,&basetype,&nfields,&klass))) goto done;

    if(isf) { /* no need to reclaim anything */
	offset->offset += xsize;
	goto done;
    }

    switch  (xtype) {
    case NC_STRING: {
        char** sp = (char**)(offset->memory + offset->offset);
        /* Need to reclaim string */
	if(*sp != NULL) free(*sp);
	offset->offset += xsize;
	} break;
    default:
    	/* reclaim a user type */
        switch (klass) {
        case NC_OPAQUE: stat = reclaim_opaque(ncid,xtype,xsize,offset); break;
        case NC_ENUM: stat = reclaim_enum(ncid,xtype,basetype,offset); break;
        case NC_COMPOUND: stat = reclaim_compound(ncid,xtype,xsize,nfields,offset); break;
        case NC_VLEN:
	    stat = reclaim_vlen(ncid,xtype,basetype,offset);
	    break;
        default:
            stat = NC_EINVAL;
	    break;
        }
	break;
    }
done:
    return stat;
}

static int
reclaim_vlen(int ncid, nc_type xtype, nc_type basetype, Position* offset)
{
    int stat = NC_NOERR;
    size_t i;
    nc_vlen_t* vl = (nc_vlen_t*)(offset->memory+offset->offset);

    if(vl->len > 0 && vl->p == NULL)
        {stat = NC_EINVAL; goto done;}

    /* Free up each entry in the vlen list */
    if(vl->len > 0) {
	Position voffset;
	size_t alignment = 0;
	if((stat = NC_type_alignment(ncid,basetype,&alignment))) goto done;;
	voffset.memory = vl->p;
	voffset.offset = 0;
        for(i=0;i<vl->len;i++) {
	    voffset.offset = read_align(voffset.offset,alignment);
	    if((stat = reclaim_datar(ncid,basetype,&voffset))) goto done;
	}
	free(vl->p);
    }
    offset->offset += sizeof(nc_vlen_t);

done:
    return stat;
}

static int
reclaim_enum(int ncid, nc_type xtype, nc_type basetype, Position* offset)
{
    int stat = NC_NOERR;
abort();

    /* basically same as an instance of the enum's integer basetype */
    stat = reclaim_datar(ncid,basetype,offset);
    return stat;
}

static int
reclaim_opaque(int ncid, nc_type xtype, size_t size, Position* offset)
{
abort();
    /* basically a fixed size sequence of bytes */
    offset->offset += size;
    return NC_NOERR;
}

static int
reclaim_compound(int ncid, nc_type xtype, size_t size, size_t nfields, Position* offset)
{
    int stat = NC_NOERR;
    size_t fid, i, arraycount;
    ptrdiff_t saveoffset;
    int ndims;
    int dimsizes[NC_MAX_VAR_DIMS];

    saveoffset = offset->offset;

    /* Get info about each field in turn and reclaim it */
    for(fid=0;fid<nfields;fid++) {
	size_t fieldalignment;
	nc_type fieldtype;

	/* Get all relevant info about the field */
	if((stat = nc_inq_compound_field(ncid,xtype,fid,NULL,&fieldalignment,&fieldtype,&ndims,dimsizes))) goto done;

	if(ndims == 0) {ndims=1; dimsizes[0]=1;} /* fake the scalar case */
	/* Align to this field */
	offset->offset = saveoffset + fieldalignment;
	/* compute the total number of elements in the field array */
	arraycount = 1;
	for(i=0;i<ndims;i++) arraycount *= dimsizes[i];
	for(i=0;i<arraycount;i++) {
	    if((stat = reclaim_datar(ncid, fieldtype, offset))) goto done;
	}
    }
    /* Return to beginning of the compound and move |compound| */
    offset->offset = saveoffset;
    offset->offset += size;

done:
    return stat;
}
#endif

/**************************************************/

/**
Copy a vector of instances of a type.  This recursively walks
the top-level instances to copy any nested data such as vlen or
strings or such.

Assumes it is passed a pointer to count instances of xtype and a
space into which to copy the instance.  Copys any nested data.

WARNING: This needs access to the type metadata of the file, so
a valid ncid and typeid must be available, which means the file
must not have been closed or aborted.

WARNING: DOES NOT ALLOCATE THE TOP-LEVEL MEMORY (see the
nc_copy_data_all function).  Note that all memory blocks other
than the top are dynamically allocated.

Should work for any netcdf format.

@param ncid root id
@param xtype type id
@param memory ptr to top-level memory to copy
@param count number of instances of the type in memory block
@param copy top-level space into which to copy the instance
@return error code
*/

int
nc_copy_data(int ncid, nc_type xtype, const void* memory, size_t count, void* copy)
{
    int stat = NC_NOERR;
    size_t i;
    Position src;
    Position dst;
    size_t xsize;
    int isf;

    if(ncid < 0 || xtype <= 0)
        {stat = NC_EINVAL; goto done;}
    if(memory == NULL && count > 0)
        {stat = NC_EINVAL; goto done;}
    if(copy == NULL && count > 0)
        {stat = NC_EINVAL; goto done;}
    if(memory == NULL || count == 0)
        goto done; /* ok, do nothing */

#ifdef REPORT
    fprintf(stderr,">>> copy   : copy  =%p memory=%p count=%lu ncid=%d xtype=%d\n",copy,memory,(unsigned long)count,ncid,xtype);
#endif

    /* Get type size */
    if((stat = NC_inq_any_type(ncid,xtype,NULL,&xsize,NULL,NULL,NULL))) goto done;

    /* Optimizations */
    /* 1. Vector of fixed sized objects */
    if((stat = NC4_inq_type_fixed_size(ncid,xtype,&isf))) goto done;
    if(isf) {
	memcpy(copy,memory,xsize*count);
	goto done;
    }

#ifdef USE_NETCDF4
    src.memory = (char*)memory; /* use char* so we can do pointer arithmetic */
    src.offset = 0;
    dst.memory = (char*)copy; /* use char* so we can do pointer arithmetic */
    dst.offset = 0;
    for(i=0;i<count;i++) {
        if((stat=copy_datar(ncid,xtype,&src,&dst))) /* copy one instance copy_datar will increment src and dst*/
	    break;
    }
#else
    stat = NC_EBADTYPE;
#endif
done:
    return stat;
}

/* Alternate entry point: includes recovering the top-level memory */
int
nc_copy_data_all(int ncid, nc_type xtype, const void* memory, size_t count, void** copyp)
{
    int stat = NC_NOERR;
    size_t xsize = 0;
    void* copy = NULL;

    /* Get type size */
    if((stat = NC_inq_any_type(ncid,xtype,NULL,&xsize,NULL,NULL,NULL))) goto done;

    /* allocate the top-level */
    if(count > 0) {
        if((copy = calloc(xsize,count))==NULL)
	    {stat = NC_ENOMEM; goto done;}
    }
    stat = nc_copy_data(ncid,xtype,memory,count,copy);
    if(copyp) {*copyp = copy; copy = NULL;}

done:
    if(copy)
        stat = nc_reclaim_data_all(ncid,xtype,copy,count);

    return stat;
}

#ifdef USE_NETCDF4
/* Recursive type walker: copy a single instance */
static int
copy_datar(int ncid, nc_type xtype, Position* src, Position* dst)
{
    int stat = NC_NOERR;
    size_t xsize;
    nc_type basetype;
    size_t nfields;
    int xclass,isf;

    if((stat = NC_inq_any_type(ncid,xtype,NULL,&xsize,&basetype,&nfields,&xclass))) goto done;

    /* Optimizations */
    /* 1. Vector of fixed size types */
    if((stat = NC4_inq_type_fixed_size(ncid,xtype,&isf))) goto done;
    if(isf) {
	memcpy(dst->memory+dst->offset,src->memory+src->offset,xsize*1);
        src->offset += xsize;
        dst->offset += xsize;
	goto done;
    }

    switch  (xtype) {
    case NC_STRING: {
        char** sp = (char**)(src->memory + src->offset);
	char* copy = NULL;
        /* Need to copy string */
	if(*sp != NULL) {
	    if((copy = strdup(*sp))==NULL) {stat = NC_ENOMEM; goto done;}
	}
	memcpy(dst->memory+dst->offset,(void*)&copy,sizeof(char*));
	src->offset += xsize;
	dst->offset += xsize;
	} break;
    default:
    	/* copy a user type */
        switch (xclass) {
        case NC_OPAQUE: stat = copy_opaque(ncid,xtype,xsize,src,dst); break;
        case NC_ENUM: stat = copy_enum(ncid,xtype,basetype,src,dst); break;
        case NC_COMPOUND: stat = copy_compound(ncid,xtype,xsize,nfields,src,dst); break;
        case NC_VLEN: stat = copy_vlen(ncid,xtype,basetype,src,dst); break;
        default: stat = NC_EINVAL; break;
        }
	break;
    }
done:
    return stat;
}

static int
copy_vlen(int ncid, nc_type xtype, nc_type basetype, Position* src, Position* dst)
{
    int stat = NC_NOERR;
    size_t i, basetypesize;
    nc_vlen_t* vl = (nc_vlen_t*)(src->memory+src->offset);
    nc_vlen_t copy = {0,NULL};

    if(vl->len > 0 && vl->p == NULL)
        {stat = NC_EINVAL; goto done;}

    /* Get basetype info */
    if((stat = NC_inq_any_type(ncid,basetype,NULL,&basetypesize,NULL,NULL,NULL))) goto done;

    /* Make space in the copy vlen */
    if(vl->len > 0) {
        copy.len = vl->len;
        if((copy.p = calloc(copy.len,basetypesize))==NULL) {stat = NC_ENOMEM; goto done;}
    } 
    /* Copy each entry in the vlen list */
    if(vl->len > 0) {
	Position vsrc, vdst;
	size_t alignment = 0;
	if((stat = NC_type_alignment(ncid,basetype,&alignment))) goto done;;
	vsrc.memory = vl->p;
	vsrc.offset = 0;
	vdst.memory = copy.p;
	vdst.offset = 0;
        for(i=0;i<vl->len;i++) {
	    vsrc.offset= read_align(vsrc.offset,alignment);
    	    vdst.offset= read_align(vdst.offset,alignment);
	    if((stat = copy_datar(ncid,basetype,&vsrc,&vdst))) goto done;
	}
    }
    /* Move into place */
    memcpy(dst->memory+dst->offset,&copy,sizeof(nc_vlen_t));
    src->offset += sizeof(nc_vlen_t);
    dst->offset += sizeof(nc_vlen_t);

done:
    if(stat) {
	nullfree(copy.p);
    }
    return stat;
}

static int
copy_enum(int ncid, nc_type xtype, nc_type basetype, Position* src, Position* dst)
{
    int stat = NC_NOERR;
abort();
    /* basically same as an instance of the enum's integer basetype */
    stat = copy_datar(ncid,basetype,src,dst);
    return stat;
}

static int
copy_opaque(int ncid, nc_type xtype, size_t size, Position* src, Position* dst)
{
abort();
    /* basically a fixed size sequence of bytes */
    memcpy(dst->memory+dst->offset,src->memory+src->offset,size);
    src->offset += size;
    dst->offset += size;
    return NC_NOERR;
}

static int
copy_compound(int ncid, nc_type xtype, size_t size, size_t nfields, Position* src, Position* dst)
{
    int stat = NC_NOERR;
    size_t fid, i, arraycount;
    ptrdiff_t savesrcoffset, savedstoffset;
    int ndims;
    int dimsizes[NC_MAX_VAR_DIMS];

    savesrcoffset = src->offset;
    savedstoffset = dst->offset;

    /* Get info about each field in turn and copy it */
    for(fid=0;fid<nfields;fid++) {
	size_t fieldoffset;
	nc_type fieldtype;
char name[NC_MAX_NAME];

	/* Get all relevant info about the field */
	if((stat = nc_inq_compound_field(ncid,xtype,fid,name,&fieldoffset,&fieldtype,&ndims,dimsizes))) goto done;

	if(ndims == 0) {ndims=1; dimsizes[0]=1;} /* fake the scalar case */
	/* Set offset for this field */
#ifdef DEBUG
fprintf(stderr,"before: offset = %d after: offset = %d\n",(int)src->offset,(int)(savesrcoffset+fieldoffset));
#endif
	src->offset = savesrcoffset+fieldoffset;
	dst->offset = savedstoffset+fieldoffset;
#ifdef DEBUG
fprintf(stderr,"field %s(%d) = %d\n",name,(int)fieldoffset,(int)src->offset);
#endif
	/* compute the total number of elements in the field array */
	arraycount = 1;
	for(i=0;i<ndims;i++) arraycount *= dimsizes[i];
	for(i=0;i<arraycount;i++) {
	    if((stat = copy_datar(ncid, fieldtype, src, dst))) goto done;
	}
#ifdef DEBUG
fprintf(stderr,"src=(%d,%p)\n",(int)src->offset,src->memory);
#endif
    }
#ifdef DEBUG
fprintf(stderr,"\n");
#endif
    /* Return to beginning of the compound and move |compound| */
    src->offset = savesrcoffset;
    dst->offset = savedstoffset;
    src->offset += size;
    dst->offset += size;

done:
    return stat;
}
#endif

/**************************************************/
/* Alignment functions */

#ifdef USE_NETCDF4
static ptrdiff_t
read_align(ptrdiff_t offset, size_t alignment)
{
  size_t loc_align = (alignment == 0 ? 1 : alignment);
  size_t delta = (offset % loc_align);
  if(delta == 0) return offset;
  return offset + (alignment - delta);
}


/**
 @param ncid - only needed for a compound type
 @param xtype - type for which alignment is requested
 @return 0 if not found
*/
int
NC_type_alignment(int ncid, nc_type xtype, size_t* alignp)
{
    int stat = NC_NOERR;
    size_t align = 0;
    int klass;

    if(!type_alignment_initialized) {
	NC_compute_alignments();
	type_alignment_initialized = 1;
    }
    if(xtype <= NC_MAX_ATOMIC_TYPE)
        {stat = NC_class_alignment(xtype,&align); goto done;}
    else {/* Presumably a user type */
        if((stat = NC_inq_any_type(ncid,xtype,NULL,NULL,NULL,NULL,&klass))) goto done;
	switch(klass) {
        case NC_VLEN: stat = NC_class_alignment(klass,&align); break;
        case NC_OPAQUE: stat = NC_class_alignment(klass,&align); break;
        case NC_COMPOUND: {/* get alignment of the first field of the compound */
	    nc_type fieldtype;
	    /* Get all relevant info about the first field */
	    if((stat = nc_inq_compound_field(ncid,xtype,0,NULL,NULL,&fieldtype,NULL,NULL))) goto done;
	    stat =  NC_type_alignment(ncid,fieldtype,&align); /* may recurse repeatedly */
	} break;
        default: break;
	}
    }
    if(alignp) *alignp = align;

done:
#if 0
Why was this here?
    if(stat == NC_NOERR && align == 0) stat = NC_EINVAL;
#endif
    return stat;
}
#endif

/**************************************************/
/* Dump an instance into a bytebuffer

@param ncid root id
@param xtype type id
@param memory ptr to top-level memory to dump
@param count number of instances of the type in memory block
@return error code
*/

int
nc_dump_data(int ncid, nc_type xtype, void* memory, size_t count, char** bufp)
{
    int stat = NC_NOERR;
    size_t i;
    Position offset;
    NCbytes* buf = ncbytesnew();

    if(ncid < 0 || xtype <= 0)
        {stat = NC_EINVAL; goto done;}
    if(memory == NULL && count > 0)
        {stat = NC_EINVAL; goto done;}
    if(memory == NULL || count == 0)
        goto done; /* ok, do nothing */
#ifdef REPORT
    fprintf(stderr,">>> dump: memory=%p count=%lu ncid=%d xtype=%d\n",memory,(unsigned long)count,ncid,xtype);
#endif
    offset.memory = (char*)memory; /* use char* so we can do pointer arithmetic */
    offset.offset = 0;
    for(i=0;i<count;i++) {
	if(i > 0) ncbytescat(buf," ");
        if((stat=dump_datar(ncid,xtype,&offset,buf))) /* dump one instance */
	    break;
    }

    if(bufp) *bufp = ncbytesextract(buf);

done:
    ncbytesfree(buf);
    return stat;
}

int
nc_print_data(int ncid, nc_type xtype, void* memory, size_t count)
{
    char* s = NULL;
    int stat = NC_NOERR;
    if((stat=nc_dump_data(ncid,xtype,memory,count,&s))) return stat;
    fprintf(stderr,"%s\n",s);
    nullfree(s)
    return stat;
}

/* Recursive type walker: dump a single instance */
static int
dump_datar(int ncid, nc_type xtype, Position* offset, NCbytes* buf)
{
    int stat = NC_NOERR;
    size_t xsize;
    nc_type basetype;
    size_t nfields;
    int klass;
    char s[128];

    /* Get relevant type info */
    if((stat = NC_inq_any_type(ncid,xtype,NULL,&xsize,&basetype,&nfields,&klass))) goto done;

    switch  (xtype) {
    case NC_CHAR:
	snprintf(s,sizeof(s),"'%c'",*(char*)(offset->memory+offset->offset));
	ncbytescat(buf,s);
	break;
    case NC_BYTE:
	snprintf(s,sizeof(s),"%d",*(char*)(offset->memory+offset->offset));
	ncbytescat(buf,s);
	break;
    case NC_UBYTE:
	snprintf(s,sizeof(s),"%u",*(unsigned char*)(offset->memory+offset->offset));
	ncbytescat(buf,s);
	break;
    case NC_SHORT:
	snprintf(s,sizeof(s),"%d",*(short*)(offset->memory+offset->offset));
	ncbytescat(buf,s);
	break;
    case NC_USHORT:
	snprintf(s,sizeof(s),"%d",*(unsigned short*)(offset->memory+offset->offset));
	ncbytescat(buf,s);
	break;
    case NC_INT:
	snprintf(s,sizeof(s),"%d",*(int*)(offset->memory+offset->offset));
	ncbytescat(buf,s);
	break;
    case NC_UINT:
	snprintf(s,sizeof(s),"%d",*(unsigned int*)(offset->memory+offset->offset));
	ncbytescat(buf,s);
	break;
    case NC_FLOAT:
	snprintf(s,sizeof(s),"%f",*(float*)(offset->memory+offset->offset));
	ncbytescat(buf,s);
	break;
    case NC_INT64:
	snprintf(s,sizeof(s),"%lld",*(long long*)(offset->memory+offset->offset));
	ncbytescat(buf,s);
	break;
    case NC_UINT64:
	snprintf(s,sizeof(s),"%llu",*(unsigned long long*)(offset->memory+offset->offset));
	ncbytescat(buf,s);
	break;
    case NC_DOUBLE:
	snprintf(s,sizeof(s),"%lf",*(double*)(offset->memory+offset->offset));
	ncbytescat(buf,s);
	break;
#ifdef USE_NETCDF4
    case NC_STRING: {
        char* s = *(char**)(offset->memory + offset->offset);
	ncbytescat(buf,"\"");
	ncbytescat(buf,s);
	ncbytescat(buf,"\"");
	} break;
#endif
    default:
#ifdef USE_NETCDF4
    	/* dump a user type */
        switch (klass) {
        case NC_OPAQUE: stat = dump_opaque(ncid,xtype,xsize,offset,buf); break;
        case NC_ENUM: stat = dump_enum(ncid,xtype,basetype,offset,buf); break;
        case NC_COMPOUND: stat = dump_compound(ncid,xtype,xsize,nfields,offset,buf); break;
        case NC_VLEN: stat = dump_vlen(ncid,xtype,basetype,offset,buf); break;
        default: stat = NC_EBADTYPE; break;
        }
#else
	stat = NC_EBADTYPE;
#endif
	break;
    }
    if(xtype <= NC_MAX_ATOMIC_TYPE)
	offset->offset += xsize;

done:
    return stat;
}

#ifdef USE_NETCDF4
static int
dump_vlen(int ncid, nc_type xtype, nc_type basetype, Position* offset, NCbytes* buf)
{
    int stat = NC_NOERR;
    size_t i;
    nc_vlen_t* vl = (nc_vlen_t*)(offset->memory+offset->offset);
    char s[128];

    if(vl->len > 0 && vl->p == NULL)
        {stat = NC_EINVAL; goto done;}

    snprintf(s,sizeof(s),"{len=%u,p=(",(unsigned)vl->len);
    ncbytescat(buf,s);
    /* dump each entry in the vlen list */
    if(vl->len > 0) {
	Position voffset;
	size_t alignment = 0;
	if((stat = NC_type_alignment(ncid,basetype,&alignment))) goto done;;
	voffset.memory = vl->p;
	voffset.offset = 0;
        for(i=0;i<vl->len;i++) {
	    if(i > 0) ncbytescat(buf," ");
	    voffset.offset = read_align(voffset.offset,alignment);
	    if((stat = dump_datar(ncid,basetype,&voffset,buf))) goto done;
	}
    } 
    ncbytescat(buf,")}");
    offset->offset += sizeof(nc_vlen_t);
    
done:
    return stat;
}

static int
dump_enum(int ncid, nc_type xtype, nc_type basetype, Position* offset, NCbytes* buf)
{
    int stat = NC_NOERR;

    /* basically same as an instance of the enum's integer basetype */
    stat = dump_datar(ncid,basetype,offset,buf);
    return stat;
}

static int
dump_opaque(int ncid, nc_type xtype, size_t size, Position* offset, NCbytes* buf)
{
    size_t i;
    char sx[16];
    /* basically a fixed size sequence of bytes */
    ncbytescat(buf,"|");
    for(i=0;i<size;i++) {
	unsigned char x = *(offset->memory+offset->offset+i);
	snprintf(sx,sizeof(sx),"%2x",x);
	ncbytescat(buf,sx);
    }
    ncbytescat(buf,"|");
    offset->offset += size;
    return NC_NOERR;
}

static int
dump_compound(int ncid, nc_type xtype, size_t size, size_t nfields, Position* offset, NCbytes* buf)
{
    int stat = NC_NOERR;
    size_t fid, i, arraycount;
    ptrdiff_t saveoffset;
    int ndims;
    int dimsizes[NC_MAX_VAR_DIMS];

    saveoffset = offset->offset;

    ncbytescat(buf,"<");

    /* Get info about each field in turn and dump it */
    for(fid=0;fid<nfields;fid++) {
	size_t fieldalignment;
	nc_type fieldtype;
	char name[NC_MAX_NAME];
	char sd[128];

	/* Get all relevant info about the field */
	if((stat = nc_inq_compound_field(ncid,xtype,fid,name,&fieldalignment,&fieldtype,&ndims,dimsizes))) goto done;
	if(fid > 0) ncbytescat(buf,";");
	ncbytescat(buf,name);
        if(ndims > 0) {
	    int j;
	    for(j=0;j<ndims;j++) {
		snprintf(sd,sizeof(sd),"[%d]",(int)dimsizes[j]);
		ncbytescat(buf,sd);
	    }
	}
	if(ndims == 0) {ndims=1; dimsizes[0]=1;} /* fake the scalar case */
	/* Align to this field */
	offset->offset = saveoffset + fieldalignment;
	/* compute the total number of elements in the field array */
	arraycount = 1;
	for(i=0;i<ndims;i++) arraycount *= dimsizes[i];
	for(i=0;i<arraycount;i++) {
	    if(i > 0) ncbytescat(buf," ");
	    if((stat = dump_datar(ncid, fieldtype, offset,buf))) goto done;
	}
    }
    ncbytescat(buf,">");
    /* Return to beginning of the compound and move |compound| */
    offset->offset = saveoffset;
    offset->offset += size;

done:
    return stat;
}
#endif

/* Extended version that can handle atomic typeids */
int
NC_inq_any_type(int ncid, nc_type typeid, char *name, size_t *size,
                  nc_type *basetypep, size_t *nfieldsp, int *classp)
{
    int stat = NC_NOERR;
#ifdef USE_NETCDF4
    if(typeid >= NC_FIRSTUSERTYPEID) {
        stat = nc_inq_user_type(ncid,typeid,name,size,basetypep,nfieldsp,classp);
    } else
#endif
    if(typeid > NC_NAT && typeid <= NC_MAX_ATOMIC_TYPE) {
	if(basetypep) *basetypep = NC_NAT;
	if(nfieldsp) *nfieldsp = 0;
	if(classp) *classp = typeid;
	stat = NC4_inq_atomic_type(typeid,name,size);
    } else
        stat = NC_EBADTYPE;
    return stat;
}
