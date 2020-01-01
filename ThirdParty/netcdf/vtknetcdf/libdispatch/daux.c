/*
Copyright (c) 1998-2018 University Corporation for Atmospheric Research/Unidata
See COPYRIGHT for license information.
*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "config.h"
#include "netcdf.h"
#include "netcdf_aux.h"
#include "ncoffsets.h"
#include "nclog.h"

struct NCAUX_FIELD {
    char* name;
    nc_type fieldtype;
    size_t ndims;
    int dimsizes[NC_MAX_VAR_DIMS];
    size_t size;
    size_t offset;
    size_t alignment;
};

struct NCAUX_CMPD {
    int ncid;
    int mode;
    char* name;
    size_t nfields;
    struct NCAUX_FIELD* fields;
    size_t size;
    size_t offset; /* cumulative as fields are added */
    size_t alignment;
};


/* It is helpful to have a structure that contains memory and an offset */
typedef struct Position{char* memory; ptrdiff_t offset;} Position;

/* Forward */
static int reclaim_datar(int ncid, int xtype, size_t typesize, Position*);

static int ncaux_initialized = 0;

#ifdef USE_NETCDF4
static int reclaim_usertype(int ncid, int xtype, Position* offset);
static int reclaim_compound(int ncid, int xtype, size_t size, size_t nfields, Position* offset);
static int reclaim_vlen(int ncid, int xtype, int basetype, Position* offset);
static int reclaim_enum(int ncid, int xtype, int basetype, size_t, Position* offset);
static int reclaim_opaque(int ncid, int xtype, size_t size, Position* offset);

static int computefieldinfo(struct NCAUX_CMPD* cmpd);
#endif /* USE_NETCDF4 */

/**************************************************/

/**
Reclaim the output tree of data from a call
to e.g. nc_get_vara or the input to e.g. nc_put_vara.
This recursively walks the top-level instances to
reclaim any nested data such as vlen or strings or such.

Assumes it is passed a pointer to count instances of xtype.
Reclaims any nested data.
WARNING: does not reclaim the top-level memory because
we do not know how it was allocated.

Should work for any netcdf format.

@param ncid file ncid
@param xtype type id
@param memory to reclaim
@param count number of instances of the type in memory
@return error code
*/

EXTERNL int
ncaux_reclaim_data(int ncid, int xtype, void* memory, size_t count)
{
    int stat = NC_NOERR;
    size_t typesize = 0;
    size_t i;
    Position offset;

    if(ncid < 0 || xtype < 0
       || (memory == NULL && count > 0)
       || xtype == NC_NAT)
        {stat = NC_EINVAL; goto done;}
    if(memory == NULL || count == 0)
        goto done; /* ok, do nothing */
    if((stat=nc_inq_type(ncid,xtype,NULL,&typesize))) goto done;
    offset.memory = (char*)memory; /* use char* so we can do pointer arithmetic */
    offset.offset = 0;
    for(i=0;i<count;i++) {
	if((stat=reclaim_datar(ncid,xtype,typesize,&offset))) /* reclaim one instance */
	    break;
    }

done:
    return stat;
}

/* Recursive type walker: reclaim a single instance */
static int
reclaim_datar(int ncid, int xtype, size_t typesize, Position* offset)
{
    int stat = NC_NOERR;

    switch  (xtype) {
    case NC_CHAR: case NC_BYTE: case NC_UBYTE:
    case NC_SHORT: case NC_USHORT:
    case NC_INT: case NC_UINT: case NC_FLOAT:
    case NC_INT64: case NC_UINT64: case NC_DOUBLE:
        offset->offset += typesize;
	break;

#ifdef USE_NETCDF4
    case NC_STRING: {
        char** sp = (char**)(offset->memory + offset->offset);
        /* Need to reclaim string */
	if(*sp != NULL) free(*sp);
	offset->offset += typesize;
	} break;
    default:
    	/* reclaim a user type */
	stat = reclaim_usertype(ncid,xtype,offset);
#else
    default:
	stat = NC_ENOTNC4;
#endif
	break;
    }
    return stat;
}

#ifdef USE_NETCDF4

static ptrdiff_t
read_align(ptrdiff_t offset, size_t alignment)
{
  size_t loc_align = (alignment == 0 ? 1 : alignment);
  size_t delta = (offset % loc_align);
  if(delta == 0) return offset;
  return offset + (alignment - delta);
}

static int
reclaim_usertype(int ncid, int xtype, Position* offset)
{
    int stat = NC_NOERR;
    size_t size;
    nc_type basetype;
    size_t nfields;
    int klass;

    /* Get info about the xtype */
    stat = nc_inq_user_type(ncid, xtype, NULL, &size, &basetype, &nfields, &klass);
    switch (klass) {
    case NC_OPAQUE: stat = reclaim_opaque(ncid,xtype,size,offset); break;
    case NC_ENUM: stat = reclaim_enum(ncid,xtype,basetype,size,offset); break;
    case NC_COMPOUND: stat = reclaim_compound(ncid,xtype,size,nfields,offset); break;
    case NC_VLEN: stat = reclaim_vlen(ncid,xtype,basetype,offset); break;
    default:
        stat = NC_EINVAL;
	break;
    }
    return stat;
}

static int
reclaim_vlen(int ncid, int xtype, int basetype, Position* offset)
{
    int stat = NC_NOERR;
    size_t i, basesize;
    nc_vlen_t* vl = (nc_vlen_t*)(offset->memory+offset->offset);

    /* Get size of the basetype */
    if((stat=nc_inq_type(ncid,basetype,NULL,&basesize))) goto done;
    /* Free up each entry in the vlen list */
    if(vl->p != NULL) {
	Position voffset;
	unsigned int alignment = ncaux_type_alignment(basetype,ncid);
	voffset.memory = vl->p;
	voffset.offset = 0;
        for(i=0;i<vl->len;i++) {
	    voffset.offset = read_align(voffset.offset,alignment);
	    if((stat = reclaim_datar(ncid,basetype,basesize,&voffset))) goto done;
	}
	offset->offset += sizeof(nc_vlen_t);
	free(vl->p);
    }

done:
    return stat;
}

static int
reclaim_enum(int ncid, int xtype, int basetype, size_t basesize, Position* offset)
{
    /* basically same as an instance of the enum's integer basetype */
    return reclaim_datar(ncid,basetype,basesize,offset);
}

static int
reclaim_opaque(int ncid, int xtype, size_t opsize, Position* offset)
{
    /* basically a fixed size sequence of bytes */
    offset->offset += opsize;
    return NC_NOERR;
}

static int
reclaim_compound(int ncid, int xtype, size_t cmpdsize, size_t nfields, Position* offset)
{
    int stat = NC_NOERR;
    size_t fid, fieldoffset, i, fieldsize, arraycount;
    int dimsizes[NC_MAX_VAR_DIMS];
    int ndims;
    nc_type fieldtype;
    ptrdiff_t saveoffset;

    saveoffset = offset->offset;

    /* Get info about each field in turn and reclaim it */
    for(fid=0;fid<nfields;fid++) {
	unsigned int fieldalignment;
	/* Get all relevant info about the field */
        if((stat = nc_inq_compound_field(ncid,xtype,fid,NULL,&fieldoffset, &fieldtype, &ndims, dimsizes))) goto done;
	fieldalignment = ncaux_type_alignment(fieldtype,ncid);
        if((stat = nc_inq_type(ncid,fieldtype,NULL,&fieldsize))) goto done;
	if(ndims == 0) {ndims=1; dimsizes[0]=1;} /* fake the scalar case */
	/* Align to this field */
	offset->offset = read_align(offset->offset,fieldalignment);
	/* compute the total number of elements in the field array */
	arraycount = 1;
	for(i=0;i<ndims;i++) arraycount *= dimsizes[i];
	for(i=0;i<arraycount;i++) {
	    if((stat = reclaim_datar(ncid, fieldtype, fieldsize, offset))) goto done;
	}
    }
    /* Return to beginning of the compound and move |compound| */
    offset->offset = saveoffset;
    offset->offset += cmpdsize;

done:
    return stat;
}

#endif /*USE_NETCDF4*/


/**************************************************/

/*
This code is a variant of the H5detect.c code from HDF5.
Author: D. Heimbigner 10/7/2008
*/

EXTERNL int
ncaux_begin_compound(int ncid, const char *name, int alignmode, void** tagp)
{
#ifdef USE_NETCDF4
    int status = NC_NOERR;
    struct NCAUX_CMPD* cmpd = NULL;

    if(tagp) *tagp = NULL;

    cmpd = (struct NCAUX_CMPD*)calloc(1,sizeof(struct NCAUX_CMPD));
    if(cmpd == NULL) {status = NC_ENOMEM; goto fail;}
    cmpd->ncid = ncid;
    cmpd->mode = alignmode;
    cmpd->nfields = 0;
    cmpd->name = strdup(name);
    if(cmpd->name == NULL) {status = NC_ENOMEM; goto fail;}

    if(tagp) {
      *tagp = (void*)cmpd;
    } else { /* Error, free cmpd to avoid memory leak. */
      free(cmpd);
    }
    return status;

fail:
    ncaux_abort_compound((void*)cmpd);
    return status;
#else
    return NC_ENOTBUILT;
#endif
}

EXTERNL int
ncaux_abort_compound(void* tag)
{
#ifdef USE_NETCDF4
    int i;
    struct NCAUX_CMPD* cmpd = (struct NCAUX_CMPD*)tag;
    if(cmpd == NULL) goto done;
    if(cmpd->name) free(cmpd->name);
    for(i=0;i<cmpd->nfields;i++) {
	struct NCAUX_FIELD* field = &cmpd->fields[i];
	if(field->name) free(field->name);
    }
    if(cmpd->fields) free(cmpd->fields);
    free(cmpd);

done:
    return NC_NOERR;
#else
    return NC_ENOTBUILT;
#endif
}

EXTERNL int
ncaux_add_field(void* tag,  const char *name, nc_type field_type,
			   int ndims, const int* dimsizes)
{
#ifdef USE_NETCDF4
    int i;
    int status = NC_NOERR;
    struct NCAUX_CMPD* cmpd = (struct NCAUX_CMPD*)tag;
    struct NCAUX_FIELD* newfields = NULL;
    struct NCAUX_FIELD* field = NULL;

    if(cmpd == NULL) goto done;
    if(ndims < 0) {status = NC_EINVAL; goto done;}
    for(i=0;i<ndims;i++) {
	if(dimsizes[i] <= 0) {status = NC_EINVAL; goto done;}
    }
    if(cmpd->fields == NULL) {
        newfields = (struct NCAUX_FIELD*)calloc(1,sizeof(struct NCAUX_FIELD));
    } else {
        newfields = (struct NCAUX_FIELD*)realloc(cmpd->fields,cmpd->nfields+1*sizeof(struct NCAUX_FIELD));
    }
    if(cmpd->fields == NULL) {status = NC_ENOMEM; goto done;}
    cmpd->fields = newfields;
    field = &cmpd->fields[cmpd->nfields+1];
    field->name = strdup(name);
    field->fieldtype = field_type;
    if(field->name == NULL) {status = NC_ENOMEM; goto done;}
    field->ndims = (size_t)ndims;
    memcpy(field->dimsizes,dimsizes,sizeof(int)*field->ndims);
    cmpd->nfields++;

done:
    if(newfields)
      free(newfields);
    return status;
#else
    return NC_ENOTBUILT;
#endif
}

EXTERNL int
ncaux_end_compound(void* tag, nc_type* idp)
{
#ifdef USE_NETCDF4
    int i;
    int status = NC_NOERR;
    struct NCAUX_CMPD* cmpd = (struct NCAUX_CMPD*)tag;

    if(cmpd == NULL) {status = NC_EINVAL; goto done;}

    /* Compute field and compound info */
    status = computefieldinfo(cmpd);
    if(status != NC_NOERR) goto done;

    status = nc_def_compound(cmpd->ncid, cmpd->size, cmpd->name, idp);
    if(status != NC_NOERR) goto done;

    for(i=0;i<cmpd->nfields;i++) {
	struct NCAUX_FIELD* field = &cmpd->fields[i];
	if(field->ndims > 0) {
            status = nc_insert_compound(cmpd->ncid, *idp, field->name,
					field->offset, field->fieldtype);
	} else {
            status = nc_insert_array_compound(cmpd->ncid, *idp, field->name,
					field->offset, field->fieldtype,
					(int)field->ndims,field->dimsizes);
	}
        if(status != NC_NOERR) goto done;
    }

done:
    return status;
#else
    return NC_ENOTBUILT;
#endif
}

/**************************************************/

/**
 @param ncclass - type class for which alignment is requested; excludes ENUM|COMPOUND
*/
EXTERNL size_t
ncaux_class_alignment(int ncclass)
{
    if(ncclass <= NC_MAX_ATOMIC_TYPE || ncclass == NC_VLEN || ncclass == NC_OPAQUE)
        return NC_class_alignment(ncclass);
    nclog(NCLOGERR,"ncaux_class_alignment: class %d; alignment cannot be determermined",ncclass);
    return 0;
}

/**
 @param ncid - only needed for a compound type
 @param xtype - type for which alignment is requested
*/
EXTERNL size_t
ncaux_type_alignment(int xtype, int ncid)
{
    if(!ncaux_initialized) {
	NC_compute_alignments();
	ncaux_initialized = 1;
    }
    if(xtype <= NC_MAX_ATOMIC_TYPE)
        return NC_class_alignment(xtype); /* type == class */
#ifdef USE_NETCDF4
    else {/* Presumably a user type */
	int klass = NC_NAT;
        int stat = nc_inq_user_type(ncid,xtype,NULL,NULL,NULL,NULL,&klass);
	if(stat) goto done;
	switch(klass) {
        case NC_VLEN: return NC_class_alignment(klass);
        case NC_OPAQUE: return NC_class_alignment(klass);
        case NC_COMPOUND: {/* get alignment of the first field of the compound */
	   int fieldtype = NC_NAT;
	   if((stat=nc_inq_compound_fieldtype(ncid,xtype,0,&fieldtype))) goto done;
	   return ncaux_type_alignment(fieldtype,ncid); /* may recurse repeatedly */
	} break;
        default: break;
	}
    }

done:
#endif /*USE_NETCDF4 */
    return 0; /* fail */
}

#ifdef USE_NETCDF4
/* Find first primitive field of a possibly nested sequence of compounds */
static nc_type
findfirstfield(int ncid, nc_type xtype)
{
    int status = NC_NOERR;
    nc_type fieldtype = xtype;
    if(xtype <= NC_MAX_ATOMIC_TYPE) goto done;

    status = nc_inq_compound_fieldtype(ncid, xtype, 0, &fieldtype);
    if(status != NC_NOERR) goto done;
    fieldtype = findfirstfield(ncid,fieldtype);

done:
    return (status == NC_NOERR?fieldtype:NC_NAT);
}

static size_t
getpadding(size_t offset, size_t alignment)
{
    size_t rem = (alignment==0?0:(offset % alignment));
    size_t pad = (rem==0?0:(alignment - rem));
    return pad;
}

static size_t
dimproduct(size_t ndims, int* dimsizes)
{
    int i;
    size_t product = 1;
    for(i=0;i<ndims;i++) product *= (size_t)dimsizes[i];
    return product;
}

static int
computefieldinfo(struct NCAUX_CMPD* cmpd)
{
    int i;
    int status = NC_NOERR;
    size_t offset = 0;
    size_t totaldimsize;

    /* Assign the sizes for the fields */
    for(i=0;i<cmpd->nfields;i++) {
	struct NCAUX_FIELD* field = &cmpd->fields[i];
	status = nc_inq_type(cmpd->ncid,field->fieldtype,NULL,&field->size);
        if(status != NC_NOERR) goto done;
	totaldimsize = dimproduct(field->ndims,field->dimsizes);
	field->size *= totaldimsize;
    }

    for(offset=0,i=0;i<cmpd->nfields;i++) {
        struct NCAUX_FIELD* field = &cmpd->fields[i];
	int alignment = 0;
	nc_type firsttype = findfirstfield(cmpd->ncid,field->fieldtype);

        /* only support 'c' alignment for now*/
	switch (field->fieldtype) {
	case NC_OPAQUE:
	    field->alignment = 1;
	    break;
	case NC_ENUM:
            field->alignment = ncaux_type_alignment(firsttype,cmpd->ncid);
	    break;
	case NC_VLEN: /*fall thru*/
	case NC_COMPOUND:
            field->alignment = ncaux_type_alignment(firsttype,cmpd->ncid);
	    break;
	default:
            field->alignment = ncaux_type_alignment(field->fieldtype,cmpd->ncid);
	    break;

	}
        offset += getpadding(offset,alignment);
        field->offset = offset;
        offset += field->size;
    }
    cmpd->size = offset;
    cmpd->alignment = cmpd->fields[0].alignment;

done:
    return status;
}

#endif /*USE_NETCDF4*/
