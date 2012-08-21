/*
 *	Copyright 1996, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: nc.h,v 2.106 2010/06/01 15:34:50 ed Exp $ */
#ifndef _NC_H_
#define _NC_H_

/*
 *	netcdf library 'private' data structures, objects and interfaces
 */
#include <config.h>
#include <stddef.h>	/* size_t */
#ifndef HAVE_STDINT_H
#  include "pstdint.h"	/* attempts to define uint32_t etc portably */
#else
#  include <stdint.h>
#endif /* HAVE_STDINT_H */
#include <sys/types.h>	/* off_t */
#ifdef USE_PARALLEL
#include <netcdf_par.h>
#else
#include <netcdf.h>
#endif /* USE_PARALLEL */
#include "ncio.h"	
#include "fbits.h"

/*#ifndef HAVE_SSIZE_T
#define ssize_t int
#endif*/

#ifdef _MSC_VER
#  pragma warning ( disable : 4127 )
#  pragma warning ( disable : 4130 )
#endif

#if defined(__BORLANDC__)
#pragma warn -8004 /* "assigned a value that is never used" */
#pragma warn -8008 /* "condition always true" */
#endif

#ifndef NC_ARRAY_GROWBY
#define NC_ARRAY_GROWBY 4
#endif

/*
 * The extern size of an empty
 * netcdf version 1 file.
 * The initial value of ncp->xsz.
 */
#define MIN_NC_XSZ 32

typedef struct NC NC; /* forward reference */

/*
 *  The internal data types
 */
typedef enum {
	NC_UNSPECIFIED = 0,
/* future	NC_BITFIELD = 7, */
/*	NC_STRING =	8,	*/
	NC_DIMENSION =	10,
	NC_VARIABLE =	11,
	NC_ATTRIBUTE =	12
} NCtype;


/*
 * Counted string for names and such
 */
typedef struct {
	/* all xdr'd */
	size_t nchars;
	char *cp;
} NC_string;

/* Begin defined in string.c */
extern void
free_NC_string(NC_string *ncstrp);

extern int
NC_check_name(const char *name);

extern NC_string *
new_NC_string(size_t slen, const char *str);

extern int
set_NC_string(NC_string *ncstrp, const char *str);

/* End defined in string.c */

/*
 * NC dimension stucture
 */
typedef struct {
	/* all xdr'd */
	NC_string *name;
 	uint32_t hash;
	size_t size;
} NC_dim;

typedef struct NC_dimarray {
	size_t nalloc;		/* number allocated >= nelems */
	/* below gets xdr'd */
	/* NCtype type = NC_DIMENSION */
	size_t nelems;		/* length of the array */
	NC_dim **value;
} NC_dimarray;

/* Begin defined in dim.c */

extern void
free_NC_dim(NC_dim *dimp);

extern NC_dim *
new_x_NC_dim(NC_string *name);

extern int
find_NC_Udim(const NC_dimarray *ncap, NC_dim **dimpp);

/* dimarray */

extern void
free_NC_dimarrayV0(NC_dimarray *ncap);

extern void
free_NC_dimarrayV(NC_dimarray *ncap);

extern int
dup_NC_dimarrayV(NC_dimarray *ncap, const NC_dimarray *ref);

extern NC_dim *
elem_NC_dimarray(const NC_dimarray *ncap, size_t elem);

/* End defined in dim.c */

/*
 * NC attribute
 */
typedef struct {
	size_t xsz;		/* amount of space at xvalue */
	/* below gets xdr'd */
	NC_string *name;
	nc_type type;		/* the discriminant */
	size_t nelems;		/* length of the array */
	void *xvalue;		/* the actual data, in external representation */
} NC_attr;

typedef struct NC_attrarray {
	size_t nalloc;		/* number allocated >= nelems */
	/* below gets xdr'd */
	/* NCtype type = NC_ATTRIBUTE */
	size_t nelems;		/* length of the array */
	NC_attr **value;
} NC_attrarray;

/* Begin defined in attr.c */

extern void
free_NC_attr(NC_attr *attrp);

extern NC_attr *
new_x_NC_attr(
	NC_string *strp,
	nc_type type,
	size_t nelems);

extern NC_attr **
NC_findattr(const NC_attrarray *ncap, const char *name);

/* attrarray */

extern void
free_NC_attrarrayV0(NC_attrarray *ncap);

extern void
free_NC_attrarrayV(NC_attrarray *ncap);

extern int
dup_NC_attrarrayV(NC_attrarray *ncap, const NC_attrarray *ref);

extern NC_attr *
elem_NC_attrarray(const NC_attrarray *ncap, size_t elem);

/* End defined in attr.c */


/*
 * NC variable: description and data
 */
typedef struct NC_var {
	size_t xsz;		/* xszof 1 element */
	size_t *shape; /* compiled info: dim->size of each dim */
	off_t *dsizes; /* compiled info: the right to left product of shape */
	/* below gets xdr'd */
	NC_string *name;
 	uint32_t hash;
	/* next two: formerly NC_iarray *assoc */ /* user definition */
	size_t ndims;	/* assoc->count */
	int *dimids;	/* assoc->value */
	NC_attrarray attrs;
	nc_type type;		/* the discriminant */
	size_t len;		/* the total length originally allocated */
	off_t begin;
} NC_var;

typedef struct NC_vararray {
	size_t nalloc;		/* number allocated >= nelems */
	/* below gets xdr'd */
	/* NCtype type = NC_VARIABLE */
	size_t nelems;		/* length of the array */
	NC_var **value;
} NC_vararray;

/* Begin defined in lookup3.c */

extern uint32_t
hash_fast(const void *key, size_t length);

/* End defined in lookup3.c */

/* Begin defined in var.c */

extern void
free_NC_var(NC_var *varp);

extern NC_var *
new_x_NC_var(
	NC_string *strp,
	size_t ndims);

/* vararray */

extern void
free_NC_vararrayV0(NC_vararray *ncap);

extern void
free_NC_vararrayV(NC_vararray *ncap);

extern int
dup_NC_vararrayV(NC_vararray *ncap, const NC_vararray *ref);

extern int
NC_var_shape(NC_var *varp, const NC_dimarray *dims);

extern int
NC_findvar(const NC_vararray *ncap, const char *name, NC_var **varpp);

extern int
NC_check_vlen(NC_var *varp, size_t vlen_max);

extern NC_var *
NC_lookupvar(NC *ncp, int varid);

/* End defined in var.c */

#define IS_RECVAR(vp) \
	((vp)->shape != NULL ? (*(vp)->shape == NC_UNLIMITED) : 0 )

#ifdef LOCKNUMREC
/*
 * typedef SHMEM type
 * for whenever the SHMEM functions can handle other than shorts
 */
typedef unsigned short int	ushmem_t;
typedef short int		 shmem_t;
#endif

/* Warning: fields from BEGIN COMMON to END COMMON must be same for:
	1. NC (libsrc/nc.h)
	2. NC_FILE_INFO (libsrc4/nc4internal.h)
	3. NCDAP3 (libncdap3/ncdap3.h)
	4. NCDAP4 (libncdap4/ncdap4.h)
*/
struct NC {
/*BEGIN COMMON*/
	int ext_ncid; /* uid << 16 */
	int int_ncid; /* unspecified other id */
	struct NC_Dispatch* dispatch;	
        struct NC_Dispatch3* dapdispatch;
	char* path; /* as specified at open or create */
/*END COMMON*/
	/* contains the previous NC during redef. */
	struct NC *old;
	/* flags */
#define NC_CREAT 2	/* in create phase, cleared by ncendef */
#define NC_INDEF 8	/* in define mode, cleared by ncendef */
#define NC_NSYNC 0x10	/* synchronise numrecs on change */
#define NC_HSYNC 0x20	/* synchronise whole header on change */
#define NC_NDIRTY 0x40	/* numrecs has changed */
#define NC_HDIRTY 0x80  /* header info has changed */
/*	NC_NOFILL in netcdf.h, historical interface */
	int flags;
	ncio *nciop;
	size_t chunk;	/* largest extent this layer will request from ncio->get() */
	size_t xsz;	/* external size of this header, == var[0].begin */
	off_t begin_var; /* position of the first (non-record) var */
	off_t begin_rec; /* position of the first 'record' */
        /* Don't constrain maximum size of record unnecessarily */
#if SIZEOF_OFF_T > SIZEOF_SIZE_T
        off_t recsize;   /* length of 'record' */
#else
	size_t recsize;  /* length of 'record' */
#endif
	/* below gets xdr'd */
	size_t numrecs; /* number of 'records' allocated */
	NC_dimarray dims;
	NC_attrarray attrs;
	NC_vararray vars;
#ifdef LOCKNUMREC
/* size and named indexes for the lock array protecting NC.numrecs */
#  define LOCKNUMREC_DIM	4
#  define LOCKNUMREC_VALUE	0
#  define LOCKNUMREC_LOCK	1
#  define LOCKNUMREC_SERVING	2
#  define LOCKNUMREC_BASEPE	3
	/* Used on Cray T3E MPP to maintain the
	 * integrity of numrecs for an unlimited dimension
	 */
	ushmem_t lock[LOCKNUMREC_DIM];
#endif
};

#define NC_readonly(ncp) \
	(!fIsSet(ncp->nciop->ioflags, NC_WRITE))

#define NC_IsNew(ncp) \
	fIsSet((ncp)->flags, NC_CREAT)

#define NC_indef(ncp) \
	(NC_IsNew(ncp) || fIsSet((ncp)->flags, NC_INDEF)) 

#define set_NC_ndirty(ncp) \
	fSet((ncp)->flags, NC_NDIRTY)

#define NC_ndirty(ncp) \
	fIsSet((ncp)->flags, NC_NDIRTY)

#define set_NC_hdirty(ncp) \
	fSet((ncp)->flags, NC_HDIRTY)

#define NC_hdirty(ncp) \
	fIsSet((ncp)->flags, NC_HDIRTY)

#define NC_dofill(ncp) \
	(!fIsSet((ncp)->flags, NC_NOFILL))

#define NC_doHsync(ncp) \
	fIsSet((ncp)->flags, NC_HSYNC)

#define NC_doNsync(ncp) \
	fIsSet((ncp)->flags, NC_NSYNC)

#ifndef LOCKNUMREC
#  define NC_get_numrecs(ncp) \
	((ncp)->numrecs)

#  define NC_set_numrecs(ncp, nrecs) \
	{((ncp)->numrecs = (nrecs));}

#  define NC_increase_numrecs(ncp, nrecs) \
	{if((nrecs) > (ncp)->numrecs) ((ncp)->numrecs = (nrecs));}
#else
	size_t NC_get_numrecs(const NC *ncp);
	void   NC_set_numrecs(NC *ncp, size_t nrecs);
	void   NC_increase_numrecs(NC *ncp, size_t nrecs);
#endif

/* Begin defined in nc.c */

extern int
NC_check_id(int ncid, NC **ncpp);

extern int
nc_cktype(nc_type datatype);

extern size_t
ncx_howmany(nc_type type, size_t xbufsize);

extern int
read_numrecs(NC *ncp);

extern int
write_numrecs(NC *ncp);

extern int
NC_sync(NC *ncp);

extern int
NC_calcsize(const NC *ncp, off_t *filesizep);

/* End defined in nc.c */
/* Begin defined in v1hpg.c */

extern size_t
ncx_len_NC(const NC *ncp, size_t sizeof_off_t);

extern int
ncx_put_NC(const NC *ncp, void **xpp, off_t offset, size_t extent);

extern int
nc_get_NC( NC *ncp);

/* End defined in v1hpg.c */
/* Begin defined in putget.c */

extern int
fill_NC_var(NC *ncp, const NC_var *varp, size_t varsize, size_t recno);

extern int
nc_inq_rec(int ncid, size_t *nrecvars, int *recvarids, size_t *recsizes);

extern int
nc_get_rec(int ncid, size_t recnum, void **datap);

extern int
nc_put_rec(int ncid, size_t recnum, void *const *datap);

/* End defined in putget.c */

extern int add_to_NCList(NC*);
extern void del_from_NCList(NC*);/* does not free object */
extern NC* find_in_NCList(int ext_ncid);
extern void free_NCList(void);/* reclaim whole list */
extern int count_NCList(void); /* return # of entries in NClist */

#endif /* _NC_H_ */
