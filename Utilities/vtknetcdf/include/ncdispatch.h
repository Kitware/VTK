/*********************************************************************
 *   Copyright 2010, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *********************************************************************/

/* $Id: ncdispatch.h,v 1.18 2010/06/01 20:11:59 dmh Exp $ */
/* $Header: /upc/share/CVS/netcdf-3/libdispatch/ncdispatch.h,v 1.18 2010/06/01 20:11:59 dmh Exp $ */

#ifndef _DISPATCH_H
#define _DISPATCH_H

#include "ncconfig.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#ifdef USE_PARALLEL
#include "netcdf_par.h"
#endif
#include "netcdf.h"
#include "nc.h"
#include "nc_url.h"

extern int nc_get_vara_ubyte(int ncid, int varid,
                  const size_t* start, const size_t* count,
                  unsigned char* value);
extern int nc_get_vara_ushort(int ncid, int varid,
                  const size_t* start, const size_t* count,
                  unsigned short* value);
extern int nc_get_vara_uint(int ncid, int varid,
                  const size_t* start, const size_t* count,
                  unsigned int* value);
extern int nc_get_vara_ulonglong(int ncid, int varid,
                  const size_t* start, const size_t* count,
                  unsigned long long* value);

extern int nc_put_vara_ushort(int ncid, int varid,
                  const size_t* start, const size_t* count,
                  const unsigned short* value);
extern int nc_put_vara_uint(int ncid, int varid,
                  const size_t* start, const size_t* count,
                  const unsigned int* value);
extern int nc_put_vara_ulonglong(int ncid, int varid,
                  const size_t* start, const size_t* count,
                  const unsigned long long* value);

#define X_INT_MAX	2147483647

/* Given a filename, check its magic number */
#define MAGIC_NUMBER_LEN 4
#define MAGIC_HDF5_FILE 1
#define MAGIC_HDF4_FILE 2
#define MAGIC_CDF1_FILE 1 /* std classic format */
#define MAGIC_CDF2_FILE 2 /* classic 64 bit */

/* Define the mappings from fcn name types
   to corresponding NC types. */
#define T_text   NC_CHAR
#define T_schar  NC_BYTE
#define T_char   NC_CHAR
#define T_short  NC_SHORT
#define T_int    NC_INT
#define T_float  NC_FLOAT
#define T_double NC_DOUBLE
#define T_ubyte  NC_UBYTE
#define T_ushort NC_USHORT
#define T_uint   NC_UINT
#define T_longlong  NC_INT64
#define T_ulonglong  NC_UINT64
#ifdef USE_NETCDF4
#define T_string NC_STRING
#endif

/* Synthetic type to handle special memtypes */
#define T_uchar  NC_UBYTE
#define T_long   longtype
#define T_ulong   ulongtype

/**************************************************/
/* Define the known classes of dispatchers */
/* Flags may be or'd => powers of 2*/
#define NC_DISPATCH_NC3    1
#define NC_DISPATCH_NC4    2
#define NC_DISPATCH_NCD    4
#define NC_DISPATCH_NCR    8

/* Define a type for use when doing e.g. nc_get_vara_long, etc. */
/* Should matche values in libsrc4/netcdf.h */
#ifndef NC_UINT64
#define	NC_UBYTE        7	/* unsigned 1 byte int */
#define	NC_USHORT       8	/* unsigned 2-byte int */
#define	NC_UINT         9	/* unsigned 4-byte int */
#define	NC_INT64        10	/* signed 8-byte int */
#define	NC_UINT64       11	/* unsigned 8-byte int */
#define	NC_STRING       12	/* char* */
#endif

#ifndef HAVE_LONGLONG
#define longlong long long
#define ulonglong unsigned long long
#endif

/* Define the range of Atomic types */
#ifdef USE_NETCDF4
#define ATOMICTYPEMAX NC_STRING
#else
#define ATOMICTYPEMAX NC_DOUBLE
#endif

/* Define an alias for int to indicate an error return */
typedef int NCerror;

/* Define a struct to hold the MPI info so it can be passed down the
 * call stack. This is used internally by the netCDF library. It
 * should not be used by netcdf users. */
#ifdef USE_PARALLEL
typedef struct NC_MPI_INFO {
    MPI_Comm comm;
    MPI_Info info;
} NC_MPI_INFO;
#endif

/* Define known dispatch tables */
/*Forward*/
typedef struct NC_Dispatch NC_Dispatch;

extern NC_Dispatch* NC3_dispatch_table;

#ifdef USE_NETCDF4
extern NC_Dispatch* NC4_dispatch_table;
#endif

#ifdef USE_DAP
extern NC_Dispatch* NCD3_dispatch_table;
#endif

#if defined(USE_DAP) && defined(USE_NETCDF4)
extern NC_Dispatch* NCD4_dispatch_table;
#endif

#if defined(USE_CDMREMOTE) && defined(USE_NETCDF4)
extern NC_Dispatch* NCCR_dispatch_table;
#endif

/**************************************************/
/* Forward */
#ifndef USE_NETCDF4
/* Taken from libsrc4/netcdf.h */
struct nc_vlen_t;
#define NC_NETCDF4 0x1000
#define NC_CLASSIC_MODEL 0x0100
#define NC_ENOPAR (-114)
#endif /*USE_NETCDF4*/

struct NC;

/* WARNING: this must match libsrc4/netcdf.h */
#ifndef USE_PARALLEL
#ifndef MPI_Comm
#define MPI_Comm int
#define MPI_Info int
#define MPI_COMM_WORLD 0
#ifndef MPI_INFO_NULL
#define MPI_INFO_NULL 0
#endif
#endif
#endif

int NC_create(const char *path, int cmode,
              size_t initialsz, int basepe, size_t *chunksizehintp,
              int useparallel,void* mpi_info,
              int *ncidp);
int NC_open(const char *path, int cmode,
            int basepe, size_t *chunksizehintp,
            int useparallel, void* mpi_info,
            int *ncidp);

/* Expose the default vars and varm dispatch entries */
extern int NCDEFAULT_get_vars(int, int, const size_t*,
               const size_t*, const ptrdiff_t*, void*, nc_type);
extern int NCDEFAULT_put_vars(int, int, const size_t*,
               const size_t*, const ptrdiff_t*, const void*, nc_type);
extern int NCDEFAULT_get_varm(int, int, const size_t*,
               const size_t*, const ptrdiff_t*, const ptrdiff_t*,
               void*, nc_type);
extern int NCDEFAULT_put_varm(int, int, const size_t*,
               const size_t*, const ptrdiff_t*, const ptrdiff_t*,
               const void*, nc_type);

/**************************************************/
/* Forward */
struct NCHDR;

struct NC_Dispatch {

int model; /* one of the NC_DISPATCH #'s above */

int (*new_nc)(struct NC**); /* Create an nc instance;free is not needed,
                                because it can be done by close and abort*/

/* Warning: these two will create appropriate NC instances
   using new_nc dispatch function
*/
int (*create)(const char *path, int cmode,
          size_t initialsz, int basepe, size_t *chunksizehintp,
          int use_parallel, void* parameters,
          struct NC_Dispatch* table, NC** ncp);
int (*open)(const char *path, int mode,
            int basepe, size_t *chunksizehintp,
            int use_parallel, void* parameters,
            struct NC_Dispatch* table, NC** ncp);

int (*redef)(int);
int (*_enddef)(int,size_t,size_t,size_t,size_t);
int (*sync)(int);
int (*abort)(int);
int (*close)(int);
int (*set_fill)(int,int,int*);
int (*inq_base_pe)(int,int*);
int (*set_base_pe)(int,int);
int (*inq_format)(int,int*);

int (*inq)(int,int*,int*,int*,int*);
int (*inq_type)(int, nc_type, char*, size_t*);

int (*def_dim)(int, const char*, size_t, int*);
int (*inq_dimid)(int, const char*, int*);
int (*inq_dim)(int, int, char*, size_t*);
int (*inq_unlimdim)(int ncid,  int *unlimdimidp);
int (*rename_dim)(int, int, const char*);

int (*inq_att)(int, int, const char*, nc_type*, size_t*);
int (*inq_attid)(int, int, const char*, int*);
int (*inq_attname)(int, int, int, char*);
int (*rename_att)(int, int, const char*, const char*);
int (*del_att)(int, int, const char*);
int (*get_att)(int, int, const char*, void*, nc_type);
int (*put_att)(int, int, const char*, nc_type, size_t, const void*, nc_type);

int (*def_var)(int, const char*, nc_type, int, const int*, int*);
int (*inq_varid)(int, const char*, int*);
int (*rename_var)(int, int, const char*);

int (*get_vara)(int, int, const size_t*, const size_t*, void*, nc_type);
int (*put_vara)(int, int, const size_t*, const size_t*, const void*, nc_type);

/* Added to solve Ferret performance problem with Opendap */
int (*get_vars)(int, int, const size_t*, const size_t*, const ptrdiff_t*, void*, nc_type);
int (*put_vars)(int, int, const size_t*, const size_t*, const ptrdiff_t*, const void*, nc_type);

int (*get_varm)(int, int, const size_t*, const size_t*, const ptrdiff_t*, const ptrdiff_t*, void*, nc_type);
int (*put_varm)(int, int, const size_t*, const size_t*, const ptrdiff_t*, const ptrdiff_t*, const void*, nc_type);


int (*inq_var_all)(int ncid, int varid, char *name, nc_type *xtypep,
               int *ndimsp, int *dimidsp, int *nattsp,
               int *shufflep, int *deflatep, int *deflate_levelp,
               int *fletcher32p, int *contiguousp, size_t *chunksizesp,
               int *no_fill, void *fill_valuep, int *endiannessp,
               int *options_maskp, int *pixels_per_blockp);

/* Note the following may still be invoked by netcdf client code
   even when the file is a classic file
*/
#ifdef USE_NETCDF4
int (*show_metadata)(int);
int (*inq_unlimdims)(int, int*, int*);
int (*var_par_access)(int, int, int);
int (*inq_ncid)(int, const char*, int*);
int (*inq_grps)(int, int*, int*);
int (*inq_grpname)(int, char*);
int (*inq_grpname_full)(int, size_t*, char*);
int (*inq_grp_parent)(int, int*);
int (*inq_grp_full_ncid)(int, const char*, int*);
int (*inq_varids)(int, int* nvars, int*);
int (*inq_dimids)(int, int* ndims, int*, int);
int (*inq_typeids)(int, int* ntypes, int*);
int (*inq_type_equal)(int, nc_type, int, nc_type, int*);
int (*def_grp)(int, const char*, int*);
int (*inq_user_type)(int, nc_type, char*, size_t*, nc_type*, size_t*, int*);
int (*inq_typeid)(int, const char*, nc_type*);

int (*def_compound)(int, size_t, const char*, nc_type*);
int (*insert_compound)(int, nc_type, const char*, size_t, nc_type);
int (*insert_array_compound)(int, nc_type, const char*, size_t, nc_type, int, const int*);
int (*inq_compound_field)(int, nc_type, int, char*, size_t*, nc_type*, int*, int*);
int (*inq_compound_fieldindex)(int, nc_type, const char*, int*);
int (*def_vlen)(int, const char*, nc_type base_typeid, nc_type*);
int (*put_vlen_element)(int, int, void*, size_t, const void*);
int (*get_vlen_element)(int, int, const void*, size_t*, void*);
int (*def_enum)(int, nc_type, const char*, nc_type*);
int (*insert_enum)(int, nc_type, const char*, const void*);
int (*inq_enum_member)(int, nc_type, int, char*, void*);
int (*inq_enum_ident)(int, nc_type, long long, char*);
int (*def_opaque)(int, size_t, const char*, nc_type*);
int (*def_var_deflate)(int, int, int, int, int);
int (*def_var_fletcher32)(int, int, int);
int (*def_var_chunking)(int, int, int, const size_t*);
int (*def_var_fill)(int, int, int, const void*);
int (*def_var_endian)(int, int, int);
int (*set_var_chunk_cache)(int, int, size_t, size_t, float);
int (*get_var_chunk_cache)(int ncid, int varid, size_t *sizep, size_t *nelemsp, float *preemptionp);
#endif /*USE_NETCDF4*/

};

/* Following functions must be handled as non-dispatch */
#ifdef NONDISPATCH
void(*nc_advise)(const char*cdf_routine_name,interr,const char*fmt,...);
void(*nc_set_log_level)(int);
const char* (*nc_inq_libvers)(void);
const char* (*nc_strerror)(int);
int(*nc_delete)(const char*path);
int(*nc_delete_mp)(const char*path,intbasepe);
#endif /*NONDISPATCH*/

/* Define the common fields for NC and NC_FILE_INFO_T etc */
typedef struct NCcommon {
        int ext_ncid; /* uid << 16 */
        int int_ncid; /* unspecified other id */
        struct NC_Dispatch* dispatch;
#ifdef USE_DAP
        struct NCDRNO* drno;
#endif
} NCcommon;

extern int NC_atomictypelen(nc_type xtype);
extern char* NC_atomictypename(nc_type xtype);

/* Provide an initializer */
extern int NC_initialize(void);

/* Provide a dispatch table overlay facility */
extern int NC_dispatch_overlay(const NC_Dispatch* overlay,
                                        const NC_Dispatch* base,
                                        NC_Dispatch* merge);

/* Get/set the override dispatch table */
extern NC_Dispatch* NC_get_dispatch_override(void);
extern void NC_set_dispatch_override(NC_Dispatch*);

/* Does the path look like a url? */
extern int NC_testurl(const char* path);
/* Return model (0 or 3 or 4) as specified by the url */
extern int NC_urlmodel(const char* path);

/* allow access dapurlparse and params without exposing dapurl.h */
extern int NCDAP_urlparse(const char* s, void** dapurl);
extern void NCDAP_urlfree(void* dapurl);
extern const char* NCDAP_urllookup(void* dapurl, const char* param);

/* Misc */
/* Replacement for strdup (in libsrc) */
#ifdef HAVE_STRDUP
#define nulldup(s) ((s)==NULL?NULL:strdup(s))
#else
extern char* nulldup(const char*);
#endif

#define nulllen(s) (s==NULL?0:strlen(s))
#define nullstring(s) (s==NULL?"(null)":s)

#endif /* _DISPATCH_H */
