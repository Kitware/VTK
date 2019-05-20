/*********************************************************************
 *   Copyright 2010, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *********************************************************************/

/* $Id: ncdispatch.h,v 1.18 2010/06/01 20:11:59 dmh Exp $ */
/* $Header: /upc/share/CVS/netcdf-3/libdispatch/ncdispatch.h,v 1.18 2010/06/01 20:11:59 dmh Exp $ */

#ifndef _DISPATCH_H
#define _DISPATCH_H

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#if defined(HDF5_PARALLEL) || defined(USE_PNETCDF)
#include <mpi.h>
#endif
#ifdef USE_PARALLEL
#include "netcdf_par.h"
#endif
#include "netcdf.h"
#include "nc.h"
#include "ncuri.h"

#define longtype ((sizeof(long) == sizeof(int) ? NC_INT : NC_INT64))

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
#if 0
/* Define the known classes of dispatchers */
/* Flags may be or'd => powers of 2*/
#define NC_DISPATCH_NC3    1
#define NC_DISPATCH_NC4    2
#define NC_DISPATCH_NCD    4
#define NC_DISPATCH_NCP    8
#endif

/* Define a type for use when doing e.g. nc_get_vara_long, etc. */
/* Should matche values in libsrc4/netcdf.h */
#ifndef NC_UINT64
#define	NC_UBYTE 	7	/* unsigned 1 byte int */
#define	NC_USHORT 	8	/* unsigned 2-byte int */
#define	NC_UINT 	9	/* unsigned 4-byte int */
#define	NC_INT64 	10	/* signed 8-byte int */
#define	NC_UINT64 	11	/* unsigned 8-byte int */
#define	NC_STRING 	12	/* char* */
#endif

/* Define the range of Atomic types */
#define ATOMICTYPEMAX4 NC_STRING
#define ATOMICTYPEMAX3 NC_DOUBLE
#define ATOMICTYPEMAX5 NC_UINT64

/* Define an alias for int to indicate an error return */
typedef int NCerror;

#if !defined HDF5_PARALLEL && !defined USE_PNETCDF
typedef int MPI_Comm;
typedef int MPI_Info;
#define MPI_COMM_WORLD 0
#define MPI_INFO_NULL 0
#endif

/* Define a struct to hold the MPI info so it can be passed down the
 * call stack. This is used internally by the netCDF library. It
 * should not be used by netcdf users. */
typedef struct NC_MPI_INFO {
    MPI_Comm comm;
    MPI_Info info;
} NC_MPI_INFO;

typedef struct NC_MEM_INFO {
    size_t size;
    void* memory;
} NC_MEM_INFO;

/* Define known dispatch tables and initializers */

/*Forward*/
typedef struct NC_Dispatch NC_Dispatch;

extern int NCDISPATCH_initialize(void);
extern int NCDISPATCH_finalize(void);

extern NC_Dispatch* NC3_dispatch_table;
extern int NC3_initialize(void);
extern int NC3_finalize(void);

#ifdef ENABLE_DAP
extern NC_Dispatch* NCD2_dispatch_table;
extern int NCD2_initialize(void);
extern int NCD2_finalize(void);
#endif
#ifdef ENABLE_DAP4
extern NC_Dispatch* NCD4_dispatch_table;
extern int NCD4_initialize(void);
extern int NCD4_finalize(void);
#endif

#ifdef USE_PNETCDF
extern NC_Dispatch* NCP_dispatch_table;
extern int NCP_initialize(void);
extern int NCP_finalize(void);
#endif

#ifdef USE_NETCDF4
extern NC_Dispatch* NC4_dispatch_table;
extern int NC4_initialize(void);
extern int NC4_finalize(void);
#endif

/* Vectors of ones and zeros */
extern size_t nc_sizevector0[NC_MAX_VAR_DIMS];
extern size_t nc_sizevector1[NC_MAX_VAR_DIMS];
extern ptrdiff_t nc_ptrdiffvector1[NC_MAX_VAR_DIMS];

/**************************************************/
/* Forward */
#ifndef USE_NETCDF4
/* Taken from libsrc4/netcdf.h */
struct nc_vlen_t;
#define NC_NETCDF4 0x1000
#define NC_CLASSIC_MODEL 0x0100
#define NC_ENOPAR (-114)
#endif /*!USE_NETCDF4*/

struct NC;


int NC_create(const char *path, int cmode,
	      size_t initialsz, int basepe, size_t *chunksizehintp,
	      int useparallel, void* parameters,
	      int *ncidp);
int NC_open(const char *path, int cmode,
	    int basepe, size_t *chunksizehintp,
	    int useparallel, void* parameters,
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

int model; /* one of the NC_FORMATX #'s */

int (*create)(const char *path, int cmode,
	  size_t initialsz, int basepe, size_t *chunksizehintp,
	  int use_parallel, void* parameters,
	  struct NC_Dispatch* table, NC* ncp);
int (*open)(const char *path, int mode,
	    int basepe, size_t *chunksizehintp,
	    int use_parallel, void* parameters,
	    struct NC_Dispatch* table, NC* ncp);

int (*redef)(int);
int (*_enddef)(int,size_t,size_t,size_t,size_t);
int (*sync)(int);
int (*abort)(int);
int (*close)(int);
int (*set_fill)(int,int,int*);
int (*inq_base_pe)(int,int*);
int (*set_base_pe)(int,int);
int (*inq_format)(int,int*);
int (*inq_format_extended)(int,int*,int*);

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

int (*var_par_access)(int, int, int);

/* Note the following may still be invoked by netcdf client code
   even when the file is a classic file; they will just return an error or
   be ignored.
*/
#ifdef USE_NETCDF4
int (*show_metadata)(int);
int (*inq_unlimdims)(int, int*, int*);
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
int (*rename_grp)(int, const char*);
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
void (*nc_advise)(const char*cdf_routine_name,interr,const char*fmt,...);
void (*nc_set_log_level)(int);
const char* (*nc_inq_libvers)(void);
const char* (*nc_strerror)(int);
int (*nc_delete)(const char*path);
int (*nc_delete_mp)(const char*path,intbasepe);
int (*nc_initialize)();
int (*nc_finalize)();
#endif /*NONDISPATCH*/

/* Define the common fields for NC and NC_FILE_INFO_T etc */
typedef struct NCcommon {
	int ext_ncid; /* uid << 16 */
	int int_ncid; /* unspecified other id */
	struct NC_Dispatch* dispatch;
	void* dispatchdata; /* per-protocol instance data */
	char* path; /* as specified at open or create */
} NCcommon;

extern size_t NC_atomictypelen(nc_type xtype);
extern char* NC_atomictypename(nc_type xtype);

#ifdef OBSOLETE
/* Provide a dispatch table overlay facility */
extern int NC_dispatch_overlay(const NC_Dispatch* overlay,
                                        const NC_Dispatch* base,
					NC_Dispatch* merge);

/* Get/set the override dispatch table */
extern NC_Dispatch* NC_get_dispatch_override(void);
extern void NC_set_dispatch_override(NC_Dispatch*);
#endif

/* Return model as specified by the url, if any;
   return a modified url suitable for passing to curl
*/
extern int NC_urlmodel(const char* path, int mode, char** newurl);

/* allow access url parse and params without exposing nc_url.h */
extern int NCDAP_urlparse(const char* s, void** dapurl);
extern void NCDAP_urlfree(void* dapurl);
extern const char* NCDAP_urllookup(void* dapurl, const char* param);

#if defined(DLL_NETCDF)
# if defined(DLL_EXPORT)
#  define NCC_EXTRA __declspec(dllexport)
#else
#  define NCC_EXTRA __declspec(dllimport)
# endif
NCC_EXTRA extern int nc__testurl(const char* path, char** basename);
#else
extern int
 nc__testurl(const char* parth, char** basename);
#endif

/* Ping a specific server */
extern int NCDAP2_ping(const char*);
extern int NCDAP4_ping(const char*);

/* Misc */

extern int NC_getshape(int ncid, int varid, int ndims, size_t* shape);
extern int NC_is_recvar(int ncid, int varid, size_t* nrecs);
extern int NC_inq_recvar(int ncid, int varid, int* nrecdims, int* is_recdim);

#define nullstring(s) (s==NULL?"(null)":s)


#undef TRACECALLS
#ifdef TRACECALLS
#include <stdio.h>
#define TRACE(fname) fprintf(stderr,"call: %s\n",#fname)
#else
#define TRACE(fname)
#endif

extern size_t NC_coord_zero[NC_MAX_VAR_DIMS];
extern size_t NC_coord_one[NC_MAX_VAR_DIMS];

extern int NC_argc;
extern char* NC_argv[];
extern int NC_initialized;

extern int nc_initialize();
extern int nc_finalize();



/**
Certain functions are in the dispatch table,
but not in the netcdf.h API. These need to
be exposed for use in delegation such as
in libdap2.
*/
extern int
NCDISPATCH_inq_var_all(int ncid, int varid, char *name, nc_type *xtypep,
               int *ndimsp, int *dimidsp, int *nattsp,
               int *shufflep, int *deflatep, int *deflate_levelp,
               int *fletcher32p, int *contiguousp, size_t *chunksizesp,
               int *no_fill, void *fill_valuep, int *endiannessp,
	       int *options_maskp, int *pixels_per_blockp);

extern int
NCDISPATCH_get_att(int ncid, int varid, const char* name, void* value, nc_type t);

#endif /* _DISPATCH_H */
