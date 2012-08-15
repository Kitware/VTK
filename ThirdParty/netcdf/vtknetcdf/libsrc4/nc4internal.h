/*
  This file is part of netcdf-4, a netCDF-like interface for HDF5, or a
  HDF5 backend for netCDF, depending on your point of view.

  This header file contains the definitions of structs used to hold
  netCDF file metadata in memory.

  Copyright 2005 University Corporation for Atmospheric Research/Unidata.

  $Id: nc4internal.h,v 1.137 2010/06/01 15:34:51 ed Exp $ */

#ifndef _NC4INTERNAL_
#define _NC4INTERNAL_

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <vtk_hdf5.h>
#include <ncdimscale.h>
#include <nc_logging.h>

#ifdef USE_PARALLEL
#include <netcdf_par.h>
#else
#define MPI_Info int
#define MPI_Comm int
#include <netcdf.h>
#endif /* USE_PARALLEL */
#include <netcdf_f.h>

#ifdef USE_HDF4
#include <mfhdf.h>
#endif

#define FILE_ID_MASK (0xffff0000)
#define GRP_ID_MASK (0x0000ffff)
#define ID_SHIFT (16)

typedef enum {GET, PUT} NC_PG_T;
typedef enum {VAR, DIM, ATT} NC_OBJ_T;

#define NC_MAX_HDF5_NAME (NC_MAX_NAME + 10)
#define NC_V2_ERR (-1)

/* The name of the root group. */
#define NC_GROUP_NAME "/"

#define MEGABYTE 1048576

/*
 * limits of the external representation
 */
#define X_SCHAR_MIN	(-128)
#define X_SCHAR_MAX	127
#define X_UCHAR_MAX	255U
#define X_SHORT_MIN	(-32768)
#define X_SHRT_MIN	X_SHORT_MIN	/* alias compatible with limits.h */
#define X_SHORT_MAX	32767
#define X_SHRT_MAX	X_SHORT_MAX	/* alias compatible with limits.h */
#define X_USHORT_MAX	65535U
#define X_USHRT_MAX	X_USHORT_MAX	/* alias compatible with limits.h */
#define X_INT_MIN	(-2147483647-1)
#define X_INT_MAX	2147483647
#define X_LONG_MIN	X_INT_MIN
#define X_LONG_MAX	X_INT_MAX
#define X_UINT_MAX	4294967295U
#ifdef WIN32 /* Windows, of course, has to be a *little* different. */
#define X_FLOAT_MAX	3.402823466e+38f
#else
#define X_FLOAT_MAX	3.40282347e+38f
#endif /* WIN32 */
#define X_FLOAT_MIN	(-X_FLOAT_MAX)
#define X_DOUBLE_MAX	1.7976931348623157e+308 
#define X_DOUBLE_MIN	(-X_DOUBLE_MAX)

/* These have to do with creating chuncked datasets in HDF5. */
#define NC_HDF5_UNLIMITED_DIMSIZE (0)
#define NC_HDF5_CHUNKSIZE_FACTOR (10)
#define NC_HDF5_MIN_CHUNK_SIZE (2)

#define NC_EMPTY_SCALE "NC_EMPTY_SCALE"

/* This is an attribute I had to add to handle multidimensional
 * coordinate variables. */
#define COORDINATES "_Netcdf4Coordinates"
#define COORDINATES_LEN (NC_MAX_NAME * 5)

/* This is used when the user defines a non-coordinate variable with
 * same name as a dimension. */
#define NON_COORD_PREPEND "_nc4_non_coord_"

/* An attribute in the HDF5 root group of this name means that the
 * file must follow strict netCDF classic format rules. */
#define NC3_STRICT_ATT_NAME "_nc3_strict"

/* If this attribute is present on a dimscale variable, use the value
 * as the netCDF dimid. */
#define NC_DIMID_ATT_NAME "_Netcdf4Dimid"

/* This is a struct to handle the dim metadata. */
typedef struct NC_DIM_INFO
{
   char *name;
   size_t len;
   int dimid;
   int unlimited;
   int extended;
   struct NC_DIM_INFO *next;
   struct NC_DIM_INFO *prev;
   hid_t hdf_dimscaleid;
   char *old_name; /* only used to rename dim */
   int dirty;
   unsigned char coord_var_in_grp;
   struct NC_VAR_INFO *coord_var; /* The coord var, if it exists. */
   int too_long; /* True if len it too big to fit in local size_t. */
} NC_DIM_INFO_T;

typedef struct NC_ATT_INFO
{
   int len;
   char *name;
   struct NC_ATT_INFO *next;
   struct NC_ATT_INFO *prev;
   int dirty;
   int created;
   nc_type xtype;
   hid_t native_typeid;
   int attnum;
   void *data;
   nc_vlen_t *vldata; /* only used for vlen */
   char **stdata; /* only for string type. */
   int class;
} NC_ATT_INFO_T;

/* This is a struct to handle the var metadata. */
typedef struct NC_VAR_INFO
{
   char *name;
   char *hdf5_name; /* used if different from name */
   int ndims;
   int *dimids;
   NC_DIM_INFO_T **dim;
   int varid;
   int natts;
   struct NC_VAR_INFO *next;
   struct NC_VAR_INFO *prev;
   int dirty;
   int created;
   int written_to;
   int *dimscale_attached;
   struct NC_TYPE_INFO *type_info;
   nc_type xtype;
   hid_t hdf_datasetid;
   NC_ATT_INFO_T *att;
   int no_fill;
   void *fill_value;
   size_t *chunksizes;
   int contiguous;
   int parallel_access;
   int dimscale;
   HDF5_OBJID_T *dimscale_hdf5_objids;
   int deflate;
   int deflate_level;
   int shuffle;
   int fletcher32;
   int options_mask;
   int pixels_per_block;
   size_t chunk_cache_size, chunk_cache_nelems;
   float chunk_cache_preemption;
   /* Stuff below is for hdf4 files. */
   int sdsid;
   int hdf4_data_type;
} NC_VAR_INFO_T;

typedef struct NC_FIELD_INFO
{
   struct NC_FIELD_INFO *next;
   struct NC_FIELD_INFO *prev;
   nc_type nctype;
   hid_t hdf_typeid;
   hid_t native_typeid;
   size_t offset;
   char *name;
   int fieldid;
   int ndims;
   int *dim_size;
} NC_FIELD_INFO_T;

typedef struct NC_ENUM_MEMBER_INFO
{
   struct NC_ENUM_MEMBER_INFO *next;
   struct NC_ENUM_MEMBER_INFO *prev;
   char *name;
   void *value;
} NC_ENUM_MEMBER_INFO_T;

typedef struct NC_TYPE_INFO
{
   struct NC_TYPE_INFO *next;
   struct NC_TYPE_INFO *prev;
   nc_type nc_typeid;
   hid_t hdf_typeid;
   hid_t native_typeid;
   size_t size;
   int committed; /* What the pig is, but the hen isn't, at breakfast. */
   char *name;
   int class; /* NC_VLEN, NC_COMPOUND, NC_OPAQUE, or NC_ENUM */
   int num_enum_members;
   NC_ENUM_MEMBER_INFO_T *enum_member;
   NC_FIELD_INFO_T *field; /* Used for compound types. */
   int num_fields;
   nc_type base_nc_type; /* for VLEN and ENUM only */
   hid_t base_hdf_typeid; /* for VLEN only */
   int close_hdf_typeid; /* True when hdf_typeid must be H5Tclosed. */
   int endianness;
} NC_TYPE_INFO_T;

/* This holds information for one group. Groups reproduce with
 * parthenogenesis. */
typedef struct NC_GRP_INFO
{
   int nc_grpid;
   struct NC_GRP_INFO *parent;
   struct NC_GRP_INFO *children;
   struct NC_GRP_INFO *next; /* points to siblings */
   struct NC_GRP_INFO *prev; /* points to siblings */
   NC_VAR_INFO_T *var;
   NC_DIM_INFO_T *dim;
   NC_ATT_INFO_T *att;
   int nvars;
   int ndims;
   int natts;
   struct NC_FILE_INFO *file;
   char *name;
   hid_t hdf_grpid;
   NC_TYPE_INFO_T *type;
} NC_GRP_INFO_T;

/* These constants apply to the cmode parameter in the
 * HDF5_FILE_INFO_T defined below. */
#define NC_CREAT 2	/* in create phase, cleared by ncendef */
#define NC_INDEF 8	/* in define mode, cleared by ncendef */
#define NC_NSYNC 0x10	/* synchronise numrecs on change */
#define NC_HSYNC 0x20	/* synchronise whole header on change */
#define NC_NDIRTY 0x40	/* numrecs has changed */
#define NC_HDIRTY 0x80  /* header info has changed */

/* This is the metadata we need to keep track of for each
   netcdf-4/HDF5 file. */
typedef struct 
{
   hid_t hdfid;
   int flags;
   int cmode;
   int nvars;
   int ndims;
   int natts;
   int parallel;  /* true if file is open for parallel access */
   int redef;
   char *path;
   int fill_mode;
   int no_write; /* true if nc_open has mode NC_NOWRITE. */
   NC_GRP_INFO_T *root_grp;
   short next_nc_grpid;
   NC_TYPE_INFO_T *type;
   int next_typeid;
   int next_dimid;
   int ignore_creationorder;
   int hdf4;
   int sdid;
} NC_HDF5_FILE_INFO_T;

/* In the nc_file array there will be one entry for each open file.*/

   /* There's an external ncid (ext_ncid) and an internal ncid
    * (int_ncid). The ext_ncid is the ncid returned to the user. If
    * the user has opened or created a netcdf-4 file, then the
    * ext_ncid is the same as the int_ncid. If he has opened or
    * created a netcdf-3 file ext_ncid (which the user sees) is
    * different from the int_ncid, which is the ncid returned by the
    * netcdf-3 layer, which insists on inventing its own ncids,
    * regardless of what is already in use due to previously opened
    * netcdf-4 files. The ext_ncid contains the ncid for the root
    * group (i.e. group zero). */

/* Warning: fields from BEGIN COMMON to END COMMON must be same for:
	1. NC (libsrc/nc.h)
	2. NC_FILE_INFO (libsrc4/nc4internal.h)
	3. NCDAP3 (libncdap3/ncdap3.h)
	4. NCDAP4 (libncdap4/ncdap4.h)
*/
typedef struct NC_FILE_INFO
{
/*BEGIN COMMON*/
   int ext_ncid;
   int int_ncid;
   struct NC_Dispatch* dispatch;	
   struct NC_Dispatch4* dapdispatch;
   char* path;
/*END COMMON*/

#ifdef USE_PNETCDF
   /* pnetcdf_file will be true if the file is created/opened with the
    * parallel-netcdf library. pnetcdf_access_mode keeps track of
    * whether independpent or collective mode is
    * desired. pnetcdf_ndims keeps track of how many dims each var
    * has, which I need to know to convert start, count, and stride
    * arrays from size_t to MPI_Offset. (I can't use an inq function
    * to find out the number of dims, because these are collective in
    * pnetcdf.) */
   int pnetcdf_file;
   int pnetcdf_access_mode;
   int pnetcdf_ndims[NC_MAX_VARS];
#endif /* USE_PNETCDF */

   /* The nc4_info pointer will remain NULL for netcdf3 files,
    * otherwise it points to information about the netcdf-4 file. */
   NC_HDF5_FILE_INFO_T *nc4_info;
} NC_FILE_INFO_T;

/* These functions only use the netcdf API calls, so they will work on
   both new format and old format files. */
/*int copy_dataset(int ncid_in, int ncid_out);*/


/* These functions convert beteen netcdf and HDF5 types. */
int nc4_get_typelen_mem(NC_HDF5_FILE_INFO_T *h5, nc_type xtype, 
			int is_long, size_t *len);
int nc4_convert_type(const void *src, void *dest, 
		     const nc_type src_type, const nc_type dest_type, 
		     const size_t len, int *range_error, 
		     const void *fill_value, int strict_nc3, int src_long,
		     int dest_long);

/* These functions do HDF5 things. */
int nc4_open_var_grp2(NC_GRP_INFO_T *grp, int varid, hid_t *dataset);
int pg_var(NC_PG_T pg, NC_FILE_INFO_T *nc, int ncid, int varid, nc_type xtype, int is_long, 
	   void *ip);
int nc4_pg_var1(NC_PG_T pg, NC_FILE_INFO_T *nc, int ncid, int varid, const size_t *indexp, 
		nc_type xtype, int is_long, void *ip);
int nc4_put_vara(NC_FILE_INFO_T *nc, int ncid, int varid, const size_t *startp, 
		 const size_t *countp, nc_type xtype, int is_long, void *op);
int nc4_get_vara(NC_FILE_INFO_T *nc, int ncid, int varid, const size_t *startp, 
		 const size_t *countp, nc_type xtype, int is_long, void *op);
int nc4_pg_varm(NC_PG_T pg, NC_FILE_INFO_T *nc, int ncid, int varid, const size_t *startp, 
		const size_t *countp, const ptrdiff_t *stridep,
		const ptrdiff_t *imapp, nc_type xtype, int is_long, void *op);
int nc4_rec_match_dimscales(NC_GRP_INFO_T *grp);
int nc4_rec_write_metadata(NC_GRP_INFO_T *grp);
int nc4_rec_write_types(NC_GRP_INFO_T *grp);
int nc4_enddef_netcdf4_file(NC_HDF5_FILE_INFO_T *h5);
int nc4_reopen_dataset(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *var);
int nc4_adjust_var_cache(NC_GRP_INFO_T *grp, NC_VAR_INFO_T * var);

/* The following functions manipulate the in-memory linked list of
   metadata, without using HDF calls. */
int nc4_find_nc_grp_h5(int ncid, NC_FILE_INFO_T **nc, NC_GRP_INFO_T **grp, 
		       NC_HDF5_FILE_INFO_T **h5);
int nc4_find_grp_h5(int ncid, NC_GRP_INFO_T **grp, NC_HDF5_FILE_INFO_T **h5);
int nc4_find_nc4_grp(int ncid, NC_GRP_INFO_T **grp);
NC_GRP_INFO_T *nc4_find_nc_grp(int ncid);
NC_GRP_INFO_T *nc4_rec_find_grp(NC_GRP_INFO_T *start_grp, int target_nc_grpid);
NC_FILE_INFO_T *nc4_find_nc_file(int ncid);
int nc4_find_dim(NC_GRP_INFO_T *grp, int dimid, NC_DIM_INFO_T **dim, NC_GRP_INFO_T **dim_grp);
int nc4_find_dim_len(NC_GRP_INFO_T *grp, int dimid, size_t **len);
int nc4_find_type(NC_HDF5_FILE_INFO_T *h5, int typeid, NC_TYPE_INFO_T **type);
NC_TYPE_INFO_T *nc4_rec_find_nc_type(NC_GRP_INFO_T *start_grp, hid_t target_nc_typeid);
NC_TYPE_INFO_T *nc4_rec_find_hdf_type(NC_GRP_INFO_T *start_grp, hid_t target_hdf_typeid);
NC_TYPE_INFO_T *nc4_rec_find_named_type(NC_GRP_INFO_T *start_grp, char *name);
NC_TYPE_INFO_T *nc4_rec_find_equal_type(NC_GRP_INFO_T *start_grp, int ncid1, NC_TYPE_INFO_T *type);
int nc4_find_nc_att(int ncid, int varid, const char *name, int attnum,
		    NC_ATT_INFO_T **att);
int nc4_find_g_var_nc(NC_FILE_INFO_T *nc, int ncid, int varid, 
		      NC_GRP_INFO_T **grp, NC_VAR_INFO_T **var);
int nc4_find_grp_att(NC_GRP_INFO_T *grp, int varid, const char *name, int attnum,
		     NC_ATT_INFO_T **att);
int nc4_get_hdf_typeid(NC_HDF5_FILE_INFO_T *h5, nc_type xtype, 
		       hid_t *hdf_typeid, int endianness);
/*int var_info_nc(NC_PG_T pg, hid_t dataset, NC_VAR_INFO_T *var_info);*/

/* These list functions add and delete vars, atts, and files. */
int nc4_file_list_add(NC_FILE_INFO_T**, struct NC_Dispatch*);
void nc4_file_list_free(void);

int nc4_nc4f_list_add(NC_FILE_INFO_T *nc, const char *path, int mode);
int nc4_var_list_add(NC_VAR_INFO_T **list, NC_VAR_INFO_T **var);
int nc4_dim_list_add(NC_DIM_INFO_T **list);
int nc4_dim_list_del(NC_DIM_INFO_T **list, NC_DIM_INFO_T *dim);
int nc4_att_list_add(NC_ATT_INFO_T **list);
int nc4_type_list_add(NC_TYPE_INFO_T **list, NC_TYPE_INFO_T **new_type);
int nc4_field_list_add(NC_FIELD_INFO_T **list, int fieldid, const char *name,
		       size_t offset, hid_t field_hdf_typeid, hid_t native_typeid, 
		       nc_type xtype, int ndims, const int *dim_sizesp);
void nc4_file_list_del(NC_FILE_INFO_T *nc);
int nc4_att_list_del(NC_ATT_INFO_T **list, NC_ATT_INFO_T *att);
int nc4_grp_list_add(NC_GRP_INFO_T **list, int new_nc_grpid, NC_GRP_INFO_T *parent_grp, 
		     NC_FILE_INFO_T *nc, char *name, NC_GRP_INFO_T **grp);
int nc4_rec_grp_del(NC_GRP_INFO_T **list, NC_GRP_INFO_T *grp);
int nc4_enum_member_add(NC_ENUM_MEMBER_INFO_T **list, size_t size,
			const char *name, const void *value);

int NC_check_name(const char *name);

/* Check and normalize names. */
int nc4_check_name(const char *name, char *norm_name);
int nc4_normalize_name(const char *name, char *norm_name);

/* Check for name collisions. */
int nc4_check_dup_name(NC_GRP_INFO_T *grp, char *norm_name);

/* Insert and read one element into an already allocated vlen array
 * element (this is for F77). */
/*int nc_put_vlen_element(int ncid, int typeid, void *vlen_element, size_t len, const void *data);
  int nc_get_vlen_element(int ncid, int typeid, const void *vlen_element, size_t *len, void *data);*/

/* This is only included if --enable-logging is used for configure; it
   prints info about the metadata to stderr. */
#ifdef LOGGING
int log_metadata_nc(NC_FILE_INFO_T *nc);
#endif

#endif /* _NETCDF4_ */
