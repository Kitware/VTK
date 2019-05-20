/** \file
  This header file contains the definitions of structs used to hold
  netCDF file metadata in memory.

  Copyright 2005-2011 University Corporation for Atmospheric
  Research/Unidata.
*/

#ifndef _NC4INTERNAL_
#define _NC4INTERNAL_

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <vtk_hdf5.h>


#include "ncdimscale.h"
#include "nc_logging.h"

#ifdef USE_PARALLEL
#include "netcdf_par.h"
#endif /* USE_PARALLEL */
#include "netcdf.h"
#include "netcdf_f.h"

/* Always needed */
#include "nc.h"

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
#define X_INT64_MIN	(-9223372036854775807LL-1LL)
#define X_INT64_MAX	9223372036854775807LL
#define X_UINT64_MAX	18446744073709551615ULL
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

/* Boolean type, to make the code easier to read */
typedef enum {NC_FALSE = 0, NC_TRUE = 1} nc_bool_t;

/*Forward*/
struct NCFILEINFO;

/* Generic doubly-linked list node */
typedef struct NC_LIST_NODE
{
   void *next;
   void *prev;
} NC_LIST_NODE_T;

/* This is a struct to handle the dim metadata. */
typedef struct NC_DIM_INFO
{
   NC_LIST_NODE_T l;            /* Use generic doubly-linked list (must be first) */
   char *name;
   size_t len;
   uint32_t hash;
   int dimid;
   nc_bool_t unlimited;         /* True if the dimension is unlimited */
   nc_bool_t extended;          /* True if the dimension needs to be extended */
   nc_bool_t too_long;          /* True if len is too big to fit in local size_t. */
   hid_t hdf_dimscaleid;
   HDF5_OBJID_T hdf5_objid;
   struct NC_VAR_INFO *coord_var; /* The coord var, if it exists. */
} NC_DIM_INFO_T;

typedef struct NC_ATT_INFO
{
   NC_LIST_NODE_T l;            /* Use generic doubly-linked list (must be first) */
   char *name;
   int len;
   nc_bool_t dirty;             /* True if attribute modified */
   nc_bool_t created;           /* True if attribute already created */
   nc_type nc_typeid;           /* netCDF type of attribute's data */
   hid_t native_hdf_typeid;     /* Native HDF5 datatype for attribute's data */
   int attnum;
   void *data;
   nc_vlen_t *vldata; /* only used for vlen */
   char **stdata; /* only for string type. */
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
   uint32_t hash;
   nc_bool_t is_new_var;        /* True if variable is newly created */
   nc_bool_t was_coord_var;     /* True if variable was a coordinate var, but either the dim or var has been renamed */
   nc_bool_t became_coord_var;  /* True if variable _became_ a coordinate var, because either the dim or var has been renamed */
   nc_bool_t fill_val_changed;  /* True if variable's fill value changes after it has been created */
   nc_bool_t attr_dirty;        /* True if variable's attributes are dirty and should be rewritten */
   nc_bool_t created;           /* Variable has already been created (_not_ that it was just created) */
   nc_bool_t written_to;        /* True if variable has data written to it */
   struct NC_TYPE_INFO *type_info;
   hid_t hdf_datasetid;
   NC_ATT_INFO_T *att;
   nc_bool_t no_fill;           /* True if no fill value is defined for var */
   void *fill_value;
   size_t *chunksizes;
   nc_bool_t contiguous;        /* True if variable is stored contiguously in HDF5 file */
   int parallel_access;         /* Type of parallel access for I/O on variable (collective or independent) */
   nc_bool_t dimscale;          /* True if var is a dimscale */
   nc_bool_t *dimscale_attached;        /* Array of flags that are true if dimscale is attached for that dim index */
   HDF5_OBJID_T *dimscale_hdf5_objids;
   nc_bool_t deflate;           /* True if var has deflate filter applied */
   int deflate_level;
   nc_bool_t shuffle;           /* True if var has shuffle filter applied */
   nc_bool_t fletcher32;        /* True if var has fletcher32 filter applied */
   nc_bool_t szip;              /* True if var has szip filter applied */
   int options_mask;
   int pixels_per_block;
   size_t chunk_cache_size, chunk_cache_nelems;
   float chunk_cache_preemption;
#ifdef USE_HDF4
   /* Stuff below is for hdf4 files. */
   int sdsid;
   int hdf4_data_type;
#endif /* USE_HDF4 */
   /* Stuff below for diskless data files. */
   void *diskless_data;
} NC_VAR_INFO_T;

typedef struct NC_FIELD_INFO
{
   NC_LIST_NODE_T l;            /* Use generic doubly-linked list (must be first) */
   nc_type nc_typeid;
   hid_t hdf_typeid;
   hid_t native_hdf_typeid;
   size_t offset;
   char *name;
   int fieldid;                 /* ID (index?) of field */
   int ndims;
   int *dim_size;
} NC_FIELD_INFO_T;

typedef struct NC_ENUM_MEMBER_INFO
{
   NC_LIST_NODE_T l;            /* Use generic doubly-linked list (must be first) */
   char *name;
   void *value;
} NC_ENUM_MEMBER_INFO_T;

typedef struct NC_TYPE_INFO
{
   NC_LIST_NODE_T l;            /* Use generic doubly-linked list (must be first) */
   char *name;
   nc_type nc_typeid;           /* netCDF type ID, equivalent to a pre-defined type
                                 * for atomic types, but a dynamically
                                 * defined value for user-defined types (stored
                                 * as named datatypes in the HDF5 file).
                                 */
   unsigned rc;                 /* Ref. count of objects using this type */
   hid_t hdf_typeid;            /* HDF5 type ID, in the file */
   hid_t native_hdf_typeid;     /* HDF5 type ID, in memory */
   int endianness;              /* What endianness for the type? */
                                /* (Set for integer types as well as "complex"
                                 *  types, like compound/enum/vlen, used for the
                                 *  endianness of the fields and/or base type)
                                 */
   size_t size;                 /* Size of the type in memory, in bytes */
   nc_bool_t committed;         /* True when datatype is committed in the file */
   nc_type nc_type_class;       /* NC_VLEN, NC_COMPOUND, NC_OPAQUE, or NC_ENUM
                                 * NOTE: NC_INT is used for all integer types,
                                 *      NC_FLOAT is used for all floating-point
                                 *      types, and NC_STRING is also used for
                                 *      fixed- and variable-length strings.
                                 *      (NC_CHAR is used for characters though)
                                 *
                                 *      This is somewhat redundant with the
                                 *      nc_type field, but allows the code to
                                 *      have a single location to look at for
                                 *      the "kind" of a type.
                                 */

   /* Information for each type or class */
   union {
      struct {
         int num_members;
         NC_ENUM_MEMBER_INFO_T *enum_member;
         nc_type base_nc_typeid;
         hid_t base_hdf_typeid;
      } e;                      /* Enum */
      struct {
         int num_fields;
         NC_FIELD_INFO_T *field;
      } c;                      /* Compound */
      struct {
         nc_type base_nc_typeid;
         hid_t base_hdf_typeid;
      } v;                      /* Variable-length */
   } u;                         /* Union of structs, for each type/class */
} NC_TYPE_INFO_T;

typedef struct NC_VAR_ARRAY_T {
	size_t nalloc;		/* number allocated >= nelems */
	size_t nelems;		/* length of the array */
	NC_VAR_INFO_T **value;
} NC_VAR_ARRAY_T;

/* This holds information for one group. Groups reproduce with
 * parthenogenesis. */
typedef struct NC_GRP_INFO
{
   NC_LIST_NODE_T l;            /* Use generic doubly-linked list (must be first) */
   char *name;
   hid_t hdf_grpid;
   int nc_grpid;
   struct NC_HDF5_FILE_INFO *nc4_info;
   struct NC_GRP_INFO *parent;
   struct NC_GRP_INFO *children;
   NC_VAR_ARRAY_T vars;
   NC_DIM_INFO_T *dim;
   NC_ATT_INFO_T *att;
   NC_TYPE_INFO_T *type;
   int nvars;
   int natts;
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
typedef struct  NC_HDF5_FILE_INFO
{
   NC* controller;
   hid_t hdfid;
#ifdef USE_PARALLEL4
   MPI_Comm comm;    /* Copy of MPI Communicator used to open the file */
   MPI_Info info;    /* Copy of MPI Information Object used to open the file */
#endif
   int flags;
   int cmode;
   nc_bool_t parallel;          /* True if file is open for parallel access */
   nc_bool_t redef;             /* True if redefining an existing file */
   int fill_mode;               /* Fill mode for vars - Unused internally currently */
   nc_bool_t no_write;          /* true if nc_open has mode NC_NOWRITE. */
   NC_GRP_INFO_T *root_grp;
   short next_nc_grpid;
   NC_TYPE_INFO_T *type;
   int next_typeid;
   int next_dimid;
#ifdef USE_HDF4
   nc_bool_t hdf4;              /* True for HDF4 file */
   int sdid;
#endif /* USE_HDF4 */
   struct NCFILEINFO* fileinfo;
} NC_HDF5_FILE_INFO_T;


/* Defined in lookup3.c */
extern uint32_t hash_fast(const void *key, size_t length);

/* These functions convert between netcdf and HDF5 types. */
int nc4_get_typelen_mem(NC_HDF5_FILE_INFO_T *h5, nc_type xtype,
			int is_long, size_t *len);
int nc4_convert_type(const void *src, void *dest,
		     const nc_type src_type, const nc_type dest_type,
		     const size_t len, int *range_error,
		     const void *fill_value, int strict_nc3, int src_long,
		     int dest_long);

/* These functions do HDF5 things. */
int rec_detach_scales(NC_GRP_INFO_T *grp, int dimid, hid_t dimscaleid);
int rec_reattach_scales(NC_GRP_INFO_T *grp, int dimid, hid_t dimscaleid);
int nc4_open_var_grp2(NC_GRP_INFO_T *grp, int varid, hid_t *dataset);
int nc4_put_vara(NC *nc, int ncid, int varid, const size_t *startp,
		 const size_t *countp, nc_type xtype, int is_long, void *op);
int nc4_get_vara(NC *nc, int ncid, int varid, const size_t *startp,
		 const size_t *countp, nc_type xtype, int is_long, void *op);
int nc4_rec_match_dimscales(NC_GRP_INFO_T *grp);
int nc4_rec_detect_need_to_preserve_dimids(NC_GRP_INFO_T *grp, nc_bool_t *bad_coord_orderp);
int nc4_rec_write_metadata(NC_GRP_INFO_T *grp, nc_bool_t bad_coord_order);
int nc4_rec_write_groups_types(NC_GRP_INFO_T *grp);
int nc4_enddef_netcdf4_file(NC_HDF5_FILE_INFO_T *h5);
int nc4_reopen_dataset(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *var);
int nc4_adjust_var_cache(NC_GRP_INFO_T *grp, NC_VAR_INFO_T * var);

/* The following functions manipulate the in-memory linked list of
   metadata, without using HDF calls. */
int nc4_find_nc_grp_h5(int ncid, NC **nc, NC_GRP_INFO_T **grp,
		       NC_HDF5_FILE_INFO_T **h5);
int nc4_find_grp_h5(int ncid, NC_GRP_INFO_T **grp, NC_HDF5_FILE_INFO_T **h5);
int nc4_find_nc4_grp(int ncid, NC_GRP_INFO_T **grp);
NC_GRP_INFO_T *nc4_find_nc_grp(int ncid);
NC_GRP_INFO_T *nc4_rec_find_grp(NC_GRP_INFO_T *start_grp, int target_nc_grpid);
NC *nc4_find_nc_file(int ncid, NC_HDF5_FILE_INFO_T**);
int nc4_find_dim(NC_GRP_INFO_T *grp, int dimid, NC_DIM_INFO_T **dim, NC_GRP_INFO_T **dim_grp);
int nc4_find_var(NC_GRP_INFO_T *grp, const char *name, NC_VAR_INFO_T **var);
int nc4_find_dim_len(NC_GRP_INFO_T *grp, int dimid, size_t **len);
int nc4_find_type(const NC_HDF5_FILE_INFO_T *h5, int typeid1, NC_TYPE_INFO_T **type);
NC_TYPE_INFO_T *nc4_rec_find_nc_type(const NC_GRP_INFO_T *start_grp, nc_type target_nc_typeid);
NC_TYPE_INFO_T *nc4_rec_find_hdf_type(NC_GRP_INFO_T *start_grp, hid_t target_hdf_typeid);
NC_TYPE_INFO_T *nc4_rec_find_named_type(NC_GRP_INFO_T *start_grp, char *name);
NC_TYPE_INFO_T *nc4_rec_find_equal_type(NC_GRP_INFO_T *start_grp, int ncid1, NC_TYPE_INFO_T *type);
int nc4_find_nc_att(int ncid, int varid, const char *name, int attnum,
		    NC_ATT_INFO_T **att);
int nc4_find_g_var_nc(NC *nc, int ncid, int varid,
		      NC_GRP_INFO_T **grp, NC_VAR_INFO_T **var);
int nc4_find_grp_att(NC_GRP_INFO_T *grp, int varid, const char *name, int attnum,
		     NC_ATT_INFO_T **att);
int nc4_get_hdf_typeid(NC_HDF5_FILE_INFO_T *h5, nc_type xtype,
		       hid_t *hdf_typeid, int endianness);
int nc4_get_typeclass(const NC_HDF5_FILE_INFO_T *h5, nc_type xtype,
                      int *type_class);

/* Free various types */
int nc4_type_free(NC_TYPE_INFO_T *type);

/* These list functions add and delete vars, atts. */
int nc4_nc4f_list_add(NC *nc, const char *path, int mode);
int nc4_var_add(NC_VAR_INFO_T **var);
int nc4_var_del(NC_VAR_INFO_T *var);
int nc4_dim_list_add(NC_DIM_INFO_T **list, NC_DIM_INFO_T **dim);
int nc4_dim_list_del(NC_DIM_INFO_T **list, NC_DIM_INFO_T *dim);
int nc4_att_list_add(NC_ATT_INFO_T **list, NC_ATT_INFO_T **att);
int nc4_type_list_add(NC_GRP_INFO_T *grp, size_t size, const char *name,
                  NC_TYPE_INFO_T **type);
int nc4_field_list_add(NC_FIELD_INFO_T **list, int fieldid, const char *name,
		       size_t offset, hid_t field_hdf_typeid, hid_t native_typeid,
		       nc_type xtype, int ndims, const int *dim_sizesp);
void nc4_file_list_del(NC *nc);
int nc4_att_list_del(NC_ATT_INFO_T **list, NC_ATT_INFO_T *att);
int nc4_grp_list_add(NC_GRP_INFO_T **list, int new_nc_grpid, NC_GRP_INFO_T *parent_grp,
		     NC *nc, char *name, NC_GRP_INFO_T **grp);
int nc4_rec_grp_del(NC_GRP_INFO_T **list, NC_GRP_INFO_T *grp);
int nc4_enum_member_add(NC_ENUM_MEMBER_INFO_T **list, size_t size,
			const char *name, const void *value);

/* Break & reform coordinate variables */
int nc4_break_coord_var(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *coord_var, NC_DIM_INFO_T *dim);
int nc4_reform_coord_var(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *coord_var, NC_DIM_INFO_T *dim);

/* Check and normalize names. */
int NC_check_name(const char *name);
int nc4_check_name(const char *name, char *norm_name);
int nc4_normalize_name(const char *name, char *norm_name);
int nc4_check_dup_name(NC_GRP_INFO_T *grp, char *norm_name);

/* HDF5 initialization */
extern int nc4_hdf5_initialized;
extern void nc4_hdf5_initialize(void);

/* This is only included if --enable-logging is used for configure; it
   prints info about the metadata to stderr. */
#ifdef LOGGING
int log_metadata_nc(NC *nc);
#endif

/* Define accessors for the dispatchdata */
#define NC4_DATA(nc) ((NC_HDF5_FILE_INFO_T*)(nc)->dispatchdata)
#define NC4_DATA_SET(nc,data) ((nc)->dispatchdata = (void*)(data))

/* Reserved Attributes */
extern const char* NC_RESERVED_VARATT_LIST[];
extern const char* NC_RESERVED_ATT_LIST[];
extern const char* NC_RESERVED_SPECIAL_LIST[];
#define NC_ATT_REFERENCE_LIST "REFERENCE_LIST"
#define NC_ATT_CLASS "CLASS"
#define NC_ATT_DIMENSION_LIST "DIMENSION_LIST"
#define NC_ATT_NAME "NAME"
#define NC_ATT_COORDINATES COORDINATES /*defined above*/
#define NC_ATT_FORMAT "_Format"

/**************************************************/
/**
For netcdf4 files, capture state information about the following:
1. Global: netcdf library version
2. Global: hdf5 library version
3. Per file: superblock version
4. Per File: was it created by netcdf-4?
5. Per file: _NCProperties attribute
*/

#define NCPROPS "_NCProperties"
#define NCPROPS_VERSION (1)
#define NCPROPSSEP  '|'

/* Currently used properties */
#define NCPVERSION "version" /* Of the properties format */
#define NCPHDF5LIBVERSION "hdf5libversion"
#define NCPNCLIBVERSION "netcdflibversion"

/* Other hidden attributes */
#define ISNETCDF4ATT "_IsNetcdf4"
#define SUPERBLOCKATT "_SuperblockVersion"

struct NCFILEINFO {
    int superblockversion;
    /* Following is filled from NCPROPS attribute or from global version */
    struct NCPROPINFO {
        int version; /* 0 => not defined */
        char hdf5ver[NC_MAX_NAME+1];
        char netcdfver[NC_MAX_NAME+1];
    } propattr;
};

extern struct NCPROPINFO globalpropinfo;

extern int NC4_fileinfo_init(void); /*libsrc4/ncinfo.c*/
extern int NC4_get_fileinfo(struct NC_HDF5_FILE_INFO* info, struct NCPROPINFO*); /*libsrc4/ncinfo.c*/
extern int NC4_put_propattr(struct NC_HDF5_FILE_INFO* info); /*libsrc4/ncinfo.c*/
extern int NC4_buildpropinfo(struct NCPROPINFO* info,char** propdatap);

extern int NC4_hdf5get_libversion(unsigned*,unsigned*,unsigned*);/*libsrc4/nc4hdf.c*/
extern int NC4_hdf5get_superblock(struct NC_HDF5_FILE_INFO*, int*);/*libsrc4/nc4hdf.c*/
extern int NC4_isnetcdf4(struct NC_HDF5_FILE_INFO*); /*libsrc4/nc4hdf.c*/

#endif /* _NETCDF4_ */
