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

/*
This code is a variant of the H5detect.c code from HDF5.
Author: D. Heimbigner 10/7/2008
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <netcdf.h>
#include "config.h"
#include "ncaux.h"

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

static int ncaux_initialized = 0;

static void compute_alignments(void);
static int computefieldinfo(struct NCAUX_CMPD* cmpd);

int
ncaux_begin_compound(int ncid, const char *name, int alignmode, void** tagp)
{
    int status = NC_NOERR;
    struct NCAUX_CMPD* cmpd = NULL;

    if(!ncaux_initialized) {
	compute_alignments();
	ncaux_initialized = 1;
    }

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
}

int
ncaux_abort_compound(void* tag)
{
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
}

int
ncaux_add_field(void* tag,  const char *name, nc_type field_type,
			   int ndims, const int* dimsizes)
{
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
}

static size_t
dimproduct(size_t ndims, int* dimsizes)
{
    int i;
    size_t product = 1;
    for(i=0;i<ndims;i++) product *= (size_t)dimsizes[i];
    return product;
}

int
ncaux_end_compound(void* tag, nc_type* idp)
{
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
}

/**************************************************/

/*
The heart of this is the following macro,
which computes the offset of a field x
when preceded by a char field.
The assumptions appear to be as follows:
1. the offset produced in this situation indicates
   the alignment for x relative in such a way that it
   depends only on the types that precede it in the struct.
2. the compiler does not reorder fields.
3. arrays are tightly packed.
4. nested structs are alignd according to their first member
   (this actually follows from C language requirement that
    a struct can legally be cast to an instance of its first member).
Given the alignments for the various common primitive types,
it is assumed that one can use them anywhere to construct
the layout of a struct of such types.
It seems to work for HDF5 for a wide variety of machines.
*/

#define COMP_ALIGNMENT(DST,TYPE)  {\
    struct {char f1; TYPE x;} tmp; \
    DST.typename = #TYPE ;        \
    DST.alignment = (size_t)((char*)(&(tmp.x)) - (char*)(&tmp));}

/* Define indices for every primitive C type */
/* NAT => NOT-A-TYPE*/
#define NATINDEX       0
#define CHARINDEX      1
#define UCHARINDEX     2
#define SHORTINDEX     3
#define USHORTINDEX    4
#define INTINDEX       5
#define UINTINDEX      6
#define LONGINDEX      7
#define ULONGINDEX     8
#define LONGLONGINDEX  9
#define ULONGLONGINDEX 10
#define FLOATINDEX     11
#define DOUBLEINDEX    12
#define PTRINDEX       13
#define NCVLENINDEX    14

#define NCTYPES        15

typedef struct Alignment {
    char* typename;
    size_t alignment;
} Alignment;

typedef Alignment Typealignvec;

/* Capture in struct and in a vector*/
typedef struct Typealignset {
    Alignment charalign;	/* char*/
    Alignment ucharalign;	/* unsigned char*/
    Alignment shortalign;	/* short*/
    Alignment ushortalign;	/* unsigned short*/
    Alignment intalign;		/* int*/
    Alignment uintalign;	/* unsigned int*/
    Alignment longalign;	/* long*/
    Alignment ulongalign;	/* unsigned long*/
    Alignment longlongalign;	/* long long*/
    Alignment ulonglongalign;	/* unsigned long long*/
    Alignment floatalign;	/* float*/
    Alignment doublealign;	/* double*/
    Alignment ptralign;		/* void**/
    Alignment ncvlenalign;	/* nc_vlen_t*/
} Typealignset;

static Typealignvec vec[NCTYPES];
static Typealignset set;

static void
compute_alignments(void)
{
    /* Compute the alignments for all the common C data types*/
    /* First for the struct*/
    /* initialize*/
    memset((void*)&set,0,sizeof(set));
    memset((void*)vec,0,sizeof(vec));

    COMP_ALIGNMENT(set.charalign,char);
    COMP_ALIGNMENT(set.ucharalign,unsigned char);
    COMP_ALIGNMENT(set.shortalign,short);
    COMP_ALIGNMENT(set.ushortalign,unsigned short);
    COMP_ALIGNMENT(set.intalign,int);
    COMP_ALIGNMENT(set.uintalign,unsigned int);
    COMP_ALIGNMENT(set.longalign,long);
    COMP_ALIGNMENT(set.ulongalign,unsigned long);
    COMP_ALIGNMENT(set.longlongalign,long long);
    COMP_ALIGNMENT(set.ulonglongalign,unsigned long long);
    COMP_ALIGNMENT(set.floatalign,float);
    COMP_ALIGNMENT(set.doublealign,double);
    COMP_ALIGNMENT(set.ptralign,void*);
    COMP_ALIGNMENT(set.ncvlenalign,nc_vlen_t);

    /* Then the vector*/
    COMP_ALIGNMENT(vec[CHARINDEX],char);
    COMP_ALIGNMENT(vec[UCHARINDEX],unsigned char); 
    COMP_ALIGNMENT(vec[SHORTINDEX],short);
    COMP_ALIGNMENT(vec[USHORTINDEX],unsigned short);
    COMP_ALIGNMENT(vec[INTINDEX],int);
    COMP_ALIGNMENT(vec[UINTINDEX],unsigned int);
    COMP_ALIGNMENT(vec[LONGINDEX],long);
    COMP_ALIGNMENT(vec[ULONGINDEX],unsigned long);
    COMP_ALIGNMENT(vec[LONGLONGINDEX],long long);
    COMP_ALIGNMENT(vec[ULONGLONGINDEX],unsigned long long);
    COMP_ALIGNMENT(vec[FLOATINDEX],float);
    COMP_ALIGNMENT(vec[DOUBLEINDEX],double);
    COMP_ALIGNMENT(vec[PTRINDEX],void*);
    COMP_ALIGNMENT(vec[NCVLENINDEX],nc_vlen_t);
}

static size_t
nctypealignment(nc_type nctype)
{
    Alignment* align = NULL;
    int index = 0;
    switch (nctype) {
      case NC_BYTE: index = UCHARINDEX; break;
      case NC_CHAR: index = CHARINDEX; break;
      case NC_SHORT: index = SHORTINDEX; break;
      case NC_INT: index = INTINDEX; break;
      case NC_FLOAT: index = FLOATINDEX; break;
      case NC_DOUBLE: index = DOUBLEINDEX; break;
      case NC_UBYTE: index = UCHARINDEX; break;
      case NC_USHORT: index = USHORTINDEX; break;
      case NC_UINT: index = UINTINDEX; break;
      case NC_INT64: index = LONGLONGINDEX; break;
      case NC_UINT64: index = ULONGLONGINDEX; break;
      case NC_STRING: index = PTRINDEX; break;
      case NC_VLEN: index = NCVLENINDEX; break;
      case NC_OPAQUE: index = UCHARINDEX; break;
      default: assert(0);
    }
    align = &vec[index];
    return align->alignment;
}

static size_t
getpadding(size_t offset, size_t alignment)
{
    size_t rem = (alignment==0?0:(offset % alignment));
    size_t pad = (rem==0?0:(alignment - rem));
    return pad;
}

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
            field->alignment = nctypealignment(firsttype);
	    break;	
	case NC_VLEN: /*fall thru*/
	case NC_COMPOUND:
            field->alignment = nctypealignment(firsttype);
	    break;
	default:
            field->alignment = nctypealignment(field->fieldtype);
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


