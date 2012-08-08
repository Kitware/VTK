/*
 * Copyright 1993-2010 University Corporation for Atmospheric Research/Unidata
 * 
 * Portions of this software were developed by the Unidata Program at the 
 * University Corporation for Atmospheric Research.
 * 
 * Access and use of this software shall impose the following obligations
 * and understandings on the user. The user is granted the right, without
 * any fee or cost, to use, copy, modify, alter, enhance and distribute
 * this software, and any derivative works thereof, and its supporting
 * documentation for any purpose whatsoever, provided that this entire
 * notice appears in all copies of the software, derivative works and
 * supporting documentation.  Further, UCAR requests that the user credit
 * UCAR/Unidata in any publications that result from the use of this
 * software or in any product that includes this software. The names UCAR
 * and/or Unidata, however, may not be used in any advertising or publicity
 * to endorse or promote any products or commercial entity unless specific
 * written permission is obtained from UCAR/Unidata. The user also
 * understands that UCAR/Unidata is not obligated to provide the user with
 * any support, consulting, training or assistance of any kind with regard
 * to the use, operation and performance of this software nor to provide
 * the user with any updates, revisions, new versions or "bug fixes."
 * 
 * THIS SOFTWARE IS PROVIDED BY UCAR/UNIDATA "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL UCAR/UNIDATA BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE ACCESS, USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/* "$Id: netcdf.h,v 1.1 2010/06/01 15:46:49 ed Exp $" */

#ifndef _NETCDF_
#define _NETCDF_

#include <stddef.h> /* size_t, ptrdiff_t */
#include <errno.h>  /* netcdf functions sometimes return system errors */

#include "vtk_netcdf_config.h"
#include "vtk_netcdf_mangle.h"

/* The nc_type type is just an int. */
typedef int nc_type;

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *  The netcdf external data types
 */
#define	NC_NAT 	        0	/* NAT = 'Not A Type' (c.f. NaN) */
#define	NC_BYTE         1	/* signed 1 byte integer */
#define	NC_CHAR 	2	/* ISO/ASCII character */
#define	NC_SHORT 	3	/* signed 2 byte integer */
#define	NC_INT 	        4	/* signed 4 byte integer */
#define NC_LONG         NC_INT  /* deprecated, but required for backward compatibility. */
#define	NC_FLOAT 	5	/* single precision floating point number */
#define	NC_DOUBLE 	6	/* double precision floating point number */
#define	NC_UBYTE 	7	/* unsigned 1 byte int */
#define	NC_USHORT 	8	/* unsigned 2-byte int */
#define	NC_UINT 	9	/* unsigned 4-byte int */
#define	NC_INT64 	10	/* signed 8-byte int */
#define	NC_UINT64 	11	/* unsigned 8-byte int */
#define	NC_STRING 	12	/* string */

#define NC_MAX_ATOMIC_TYPE NC_STRING

/* The following are use internally in support of user-defines
 * types. They are also the class returned by nc_inq_user_type. */
#define	NC_VLEN 	13	/* used internally for vlen types */
#define	NC_OPAQUE 	14	/* used internally for opaque types */
#define	NC_ENUM 	15	/* used internally for enum types */
#define	NC_COMPOUND 	16	/* used internally for compound types */

/* Define the first user defined type id (leave some room) */
#define NC_FIRSTUSERTYPEID 32

/*
 * 	Default fill values, used unless _FillValue attribute is set.
 * These values are stuffed into newly allocated space as appropriate.
 * The hope is that one might use these to notice that a particular datum
 * has not been set.
 */
#define NC_FILL_BYTE	((signed char)-127)
#define NC_FILL_CHAR	((char)0)
#define NC_FILL_SHORT	((short)-32767)
#define NC_FILL_INT	(-2147483647L)
#define NC_FILL_FLOAT	(9.9692099683868690e+36f) /* near 15 * 2^119 */
#define NC_FILL_DOUBLE	(9.9692099683868690e+36)
#define NC_FILL_UBYTE   (255)
#define NC_FILL_USHORT  (65535)
#define NC_FILL_UINT    (4294967295U)
#define NC_FILL_INT64   ((long long)-9223372036854775806LL)
#define NC_FILL_UINT64  ((unsigned long long)18446744073709551614ULL)
#define NC_FILL_STRING  ""

/* These represent the max and min values that can be stored in a
 * netCDF file for their associated types. Recall that a C compiler
 * may define int to be any length it wants, but a NC_INT is *always*
 * a 4 byte signed int. On a platform with has 64 bit ints, there will
 * be many ints which are outside the range supported by NC_INT. But
 * since NC_INT is an external format, it has to mean the same thing
 * everywhere. */
#define NC_MAX_BYTE 127
#define NC_MIN_BYTE (-NC_MAX_BYTE-1)
#define NC_MAX_CHAR 255
#define NC_MAX_SHORT 32767
#define NC_MIN_SHORT (-NC_MAX_SHORT - 1)
#define NC_MAX_INT 2147483647
#define NC_MIN_INT (-NC_MAX_INT - 1)
#define NC_MAX_FLOAT 3.402823466e+38f
#define NC_MIN_FLOAT (-NC_MAX_FLOAT)
#define NC_MAX_DOUBLE 1.7976931348623157e+308 
#define NC_MIN_DOUBLE (-NC_MAX_DOUBLE)
#define NC_MAX_UBYTE NC_MAX_CHAR
#define NC_MAX_USHORT 65535U
#define NC_MAX_UINT 4294967295U
#define NC_MAX_INT64 (9223372036854775807LL)
#define NC_MIN_INT64 (-9223372036854775807LL-1)
#define NC_MAX_UINT64 (18446744073709551615ULL)
#define X_INT64_MAX     (9223372036854775807LL)
#define X_INT64_MIN     (-X_INT64_MAX - 1)
#define X_UINT64_MAX    (18446744073709551615ULL)

/*
 * The above values are defaults.  If you wish a variable to use a
 * different value than the above defaults, create an attribute with
 * the same type as the variable and the following reserved name. The
 * value you give the attribute will be used as the fill value for
 * that variable.
 */
#define _FillValue	"_FillValue"
#define NC_FILL		0	/* argument to ncsetfill to clear NC_NOFILL */
#define NC_NOFILL	0x100	/* Don't fill data section an records */

/*
 * Use these 'mode' flags for nc_open.
 */
#define NC_NOWRITE	0	/* default is read only */
#define NC_WRITE    	0x0001	/* read & write */

/*
 * Use these 'mode' flags for nc_create.
 */
#define NC_CLOBBER	0
#define NC_NOCLOBBER	0x0004	/* Don't destroy existing file on create */
#define NC_64BIT_OFFSET 0x0200  /* Use large (64-bit) file offsets */
#define NC_NETCDF4      0x1000  /* Use netCDF-4/HDF5 format */
#define NC_CLASSIC_MODEL 0x0100 /* Enforce classic model when used with NC_NETCDF4. */

/*
 * Use these 'mode' flags for both nc_create and nc_open.
 */
#define NC_SHARE       0x0800	/* Share updates, limit cacheing */
#define NC_MPIIO       0x2000 
#define NC_MPIPOSIX    0x4000
#define NC_PNETCDF     0x8000

/* The following flag currently is ignored, but use in
 * nc_open() or nc_create() may someday support use of advisory
 * locking to prevent multiple writers from clobbering a file 
 */
#define NC_LOCK		0x0400	/* Use locking if available */

/*
 * Starting with version 3.6, there are different format netCDF
 * files. 4.0 introduces the third one. These defines are only for
 * the nc_set_default_format function.
 */
#define NC_FORMAT_CLASSIC (1)
#define NC_FORMAT_64BIT   (2)
#define NC_FORMAT_NETCDF4 (3)
#define NC_FORMAT_NETCDF4_CLASSIC  (4) /* create netcdf-4 files, with NC_CLASSIC_MODEL. */

/*
 * Let nc__create() or nc__open() figure out
 * as suitable chunk size.
 */
#define NC_SIZEHINT_DEFAULT 0

/*
 * In nc__enddef(), align to the chunk size.
 */
#define NC_ALIGN_CHUNK ((size_t)(-1))

/*
 * 'size' argument to ncdimdef for an unlimited dimension
 */
#define NC_UNLIMITED 0L

/*
 * attribute id to put/get a global attribute
 */
#define NC_GLOBAL -1

/*
 * These maximums are enforced by the interface, to facilitate writing
 * applications and utilities.  However, nothing is statically allocated to
 * these sizes internally.
 */
#define NC_MAX_DIMS	1024	 /* max dimensions per file */
#define NC_MAX_ATTRS	8192	 /* max global or per variable attributes */
#define NC_MAX_VARS	8192	 /* max variables per file */
#define NC_MAX_NAME	256	 /* max length of a name */
/* As a rule, NC_MAX_VAR_DIMS <= NC_MAX_DIMS*/
#define NC_MAX_VAR_DIMS	1024 /* max per variable dimensions */

/* This is the max size of an SD dataset name in HDF4. */
#define NC_MAX_HDF4_NAME 64 /* From HDF4 documentation. */

/* In HDF5 files you can set the endianness of variables with
 * nc_def_var_endian(). These defines are used there. */   
#define NC_ENDIAN_NATIVE 0
#define NC_ENDIAN_LITTLE 1
#define NC_ENDIAN_BIG    2

/* In HDF5 files you can set storage for each variable to be either
 * contiguous or chunked, with nc_def_var_chunking().  These defines
 * are used there. */
#define NC_CHUNKED    0
#define NC_CONTIGUOUS 1

/* In HDF5 files you can set check-summing for each variable.
 * Currently the only checksum available is Fletcher-32, which can be
 * set with the function nc_def_var_fletcher32.  These defines are used
 * there. */
#define NC_NOCHECKSUM 0
#define NC_FLETCHER32 1

/* In HDF5 files you can specify that a shuffle filter should be used
 * on each chunk of a variable to improve compression for that
 * variable.  This per-variable shuffle property can be set with the
 * function nc_def_var_deflate.  These defines are used there. */
#define NC_NOSHUFFLE 0
#define NC_SHUFFLE   1

/*
 * The netcdf version 3 functions all return integer error status.
 * These are the possible values, in addition to certain
 * values from the system errno.h.
 */

#define NC_ISSYSERR(err)	((err) > 0)

#define	NC_NOERR	0	/* No Error */

#define NC2_ERR         (-1)    /* Returned for all errors in the v2 API. */
#define	NC_EBADID	(-33)	/* Not a netcdf id */
#define	NC_ENFILE	(-34)	/* Too many netcdfs open */
#define	NC_EEXIST	(-35)	/* netcdf file exists && NC_NOCLOBBER */
#define	NC_EINVAL	(-36)	/* Invalid Argument */
#define	NC_EPERM	(-37)	/* Write to read only */
#define NC_ENOTINDEFINE	(-38)	/* Operation not allowed in data mode */
#define	NC_EINDEFINE	(-39)	/* Operation not allowed in define mode */
#define	NC_EINVALCOORDS	(-40)	/* Index exceeds dimension bound */
#define	NC_EMAXDIMS	(-41)	/* NC_MAX_DIMS exceeded */
#define	NC_ENAMEINUSE	(-42)	/* String match to name in use */
#define NC_ENOTATT	(-43)	/* Attribute not found */
#define	NC_EMAXATTS	(-44)	/* NC_MAX_ATTRS exceeded */
#define NC_EBADTYPE	(-45)	/* Not a netcdf data type */
#define NC_EBADDIM	(-46)	/* Invalid dimension id or name */
#define NC_EUNLIMPOS	(-47)	/* NC_UNLIMITED in the wrong index */
#define	NC_EMAXVARS	(-48)	/* NC_MAX_VARS exceeded */
#define NC_ENOTVAR	(-49)	/* Variable not found */
#define NC_EGLOBAL	(-50)	/* Action prohibited on NC_GLOBAL varid */
#define NC_ENOTNC	(-51)	/* Not a netcdf file */
#define NC_ESTS        	(-52)	/* In Fortran, string too short */
#define NC_EMAXNAME    	(-53)	/* NC_MAX_NAME exceeded */
#define NC_EUNLIMIT    	(-54)	/* NC_UNLIMITED size already in use */
#define NC_ENORECVARS  	(-55)	/* nc_rec op when there are no record vars */
#define NC_ECHAR	(-56)	/* Attempt to convert between text & numbers */
#define NC_EEDGE	(-57)	/* Start+count exceeds dimension bound */
#define NC_ESTRIDE	(-58)	/* Illegal stride */
#define NC_EBADNAME	(-59)	/* Attribute or variable name
                                         contains illegal characters */
/* N.B. following must match value in ncx.h */
#define NC_ERANGE	(-60)	/* Math result not representable */
#define NC_ENOMEM	(-61)	/* Memory allocation (malloc) failure */

#define NC_EVARSIZE     (-62)   /* One or more variable sizes violate
				   format constraints */ 
#define NC_EDIMSIZE     (-63)   /* Invalid dimension size */
#define NC_ETRUNC       (-64)   /* File likely truncated or possibly corrupted */

#define NC_EAXISTYPE    (-65)   /* Unknown axis type. */

/* Following errors are added for DAP */
#define NC_EDAP         (-66)   /* Generic DAP error */
#define NC_ECURL        (-67)   /* Generic libcurl error */
#define NC_EIO          (-68)   /* Generic IO error */
#define NC_ENODATA      (-69)   /* Attempt to access variable with no data */
#define NC_EDAPSVC      (-70)   /* DAP Server side error */
#define NC_EDAS		(-71)   /* Malformed or inaccessible DAS */
#define NC_EDDS		(-72)   /* Malformed or inaccessible DDS */
#define NC_EDATADDS	(-73)   /* Malformed or inaccessible DATADDS */
#define NC_EDAPURL	(-74)   /* Malformed DAP URL */
#define NC_EDAPCONSTRAINT (-75)   /* Malformed DAP Constraint*/

/* The following was added in support of netcdf-4. Make all netcdf-4
   error codes < -100 so that errors can be added to netcdf-3 if
   needed. */
#define NC4_FIRST_ERROR  (-100)
#define NC_EHDFERR       (-101) /* Error at HDF5 layer. */
#define NC_ECANTREAD     (-102) /* Can't read. */
#define NC_ECANTWRITE    (-103) /* Can't write. */
#define NC_ECANTCREATE   (-104) /* Can't create. */
#define NC_EFILEMETA     (-105) /* Problem with file metadata. */
#define NC_EDIMMETA      (-106) /* Problem with dimension metadata. */
#define NC_EATTMETA      (-107) /* Problem with attribute metadata. */
#define NC_EVARMETA      (-108) /* Problem with variable metadata. */
#define NC_ENOCOMPOUND   (-109) /* Not a compound type. */
#define NC_EATTEXISTS    (-110) /* Attribute already exists. */
#define NC_ENOTNC4       (-111) /* Attempting netcdf-4 operation on netcdf-3 file. */  
#define NC_ESTRICTNC3    (-112) /* Attempting netcdf-4 operation on strict nc3 netcdf-4 file. */  
#define NC_ENOTNC3       (-113) /* Attempting netcdf-3 operation on netcdf-4 file. */  
#define NC_ENOPAR        (-114) /* Parallel operation on file opened for non-parallel access. */  
#define NC_EPARINIT      (-115) /* Error initializing for parallel access. */  
#define NC_EBADGRPID     (-116) /* Bad group ID. */  
#define NC_EBADTYPID     (-117) /* Bad type ID. */  
#define NC_ETYPDEFINED   (-118) /* Type has already been defined and may not be edited. */
#define NC_EBADFIELD     (-119) /* Bad field ID. */  
#define NC_EBADCLASS     (-120) /* Bad class. */  
#define NC_EMAPTYPE      (-121) /* Mapped access for atomic types only. */  
#define NC_ELATEFILL     (-122) /* Attempt to define fill value when data already exists. */
#define NC_ELATEDEF      (-123) /* Attempt to define var properties, like deflate, after enddef. */
#define NC_EDIMSCALE     (-124) /* Probem with HDF5 dimscales. */
#define NC_ENOGRP        (-125) /* No group found. */
#define NC_ESTORAGE      (-126) /* Can't specify both contiguous and chunking. */
#define NC_EBADCHUNK     (-127) /* Bad chunksize. */
#define NC_ENOTBUILT     (-128) /* Attempt to use feature that was not turned on when netCDF was built. */
#define NC4_LAST_ERROR   (-128) 

/* This is used in netCDF-4 files for dimensions without coordinate
 * vars. */
#define DIM_WITHOUT_VARIABLE "This is a netCDF dimension but not a netCDF variable."

/* This is here at the request of the NCO team to support the stupid
 * mistake of having chunksizes be first ints, then size_t. Doh! */
#define NC_HAVE_NEW_CHUNKING_API 1


/*Errors for all remote access methods(e.g. DAP and CDMREMOTE)*/
#define NC_EURL		(NC_EDAPURL)   /* Malformed URL */
#define NC_ECONSTRAINT  (NC_EDAPCONSTRAINT)   /* Malformed Constraint*/


/*
 * The Interface
 */

/* Declaration modifiers for DLL support (MSC et al) */

#if defined(DLL_NETCDF) /* define when library is a DLL */
#  if defined(vtkNetCDF_EXPORTS) /* define when building the library */
#   define MSC_EXTRA __declspec(dllexport)
#  else
#   define MSC_EXTRA __declspec(dllimport)
#  endif
#  if defined(vtkNetCDF_cxx_EXPORTS)
#   define MSCPP_EXTRA __declspec(dllexport)
#  else
#   define MSCPP_EXTRA __declspec(dllimport)
#  endif
#include <io.h>
/*#define lseek _lseeki64
  #define off_t __int64*/
#else
#define MSC_EXTRA
#define MSCPP_EXTRA
#endif	/* defined(DLL_NETCDF) */

# define EXTERNL extern MSC_EXTRA

/* When netCDF is built as a DLL, this will export ncerr and
 * ncopts. When it is used as a DLL, it will import them. */
#if defined(DLL_NETCDF)
EXTERNL int ncerr;
EXTERNL int ncopts;
#endif

EXTERNL const char *
nc_inq_libvers(void);

EXTERNL const char *
nc_strerror(int ncerr);

EXTERNL int
nc__create(const char *path, int cmode, size_t initialsz,
	 size_t *chunksizehintp, int *ncidp);

EXTERNL int
nc_create(const char *path, int cmode, int *ncidp);

EXTERNL int
nc__open(const char *path, int mode, 
	size_t *chunksizehintp, int *ncidp);

EXTERNL int
nc_open(const char *path, int mode, int *ncidp);

/* Learn the path used to open/create the file. */
EXTERNL int
nc_inq_path(int ncid, size_t *pathlen, char *path);

/* Use these with nc_var_par_access(). */
#define NC_INDEPENDENT 0
#define NC_COLLECTIVE 1

/* Set parallel access for a variable to independent (the default) or
 * collective. */
EXTERNL int
nc_var_par_access(int ncid, int varid, int par_access);

/* Given an ncid and group name (NULL gets root group), return
 * locid. */
EXTERNL int
nc_inq_ncid(int ncid, const char *name, int *grp_ncid);

/* Given a location id, return the number of groups it contains, and
 * an array of their locids. */
EXTERNL int
nc_inq_grps(int ncid, int *numgrps, int *ncids);

/* Given locid, find name of group. (Root group is named "/".) */
EXTERNL int
nc_inq_grpname(int ncid, char *name);

/* Given ncid, find full name and len of full name. (Root group is
 * named "/", with length 1.) */
EXTERNL int
nc_inq_grpname_full(int ncid, size_t *lenp, char *full_name);

/* Given ncid, find len of full name. */
EXTERNL int
nc_inq_grpname_len(int ncid, size_t *lenp);

/* Given an ncid, find the ncid of its parent group. */
EXTERNL int
nc_inq_grp_parent(int ncid, int *parent_ncid);

/* Given a name and parent ncid, find group ncid. */
EXTERNL int
nc_inq_grp_ncid(int ncid, const char *grp_name, int *grp_ncid);

/* Given a full name and ncid, find group ncid. */
EXTERNL int
nc_inq_grp_full_ncid(int ncid, const char *full_name, int *grp_ncid);

/* Get a list of ids for all the variables in a group. */
EXTERNL int 
nc_inq_varids(int ncid, int *nvars, int *varids);

/* Find all dimids for a location. This finds all dimensions in a
 * group, or any of its parents. */
EXTERNL int 
nc_inq_dimids(int ncid, int *ndims, int *dimids, int include_parents);

/* Find all user-defined types for a location. This finds all
 * user-defined types in a group. */
EXTERNL int 
nc_inq_typeids(int ncid, int *ntypes, int *typeids);

/* Are two types equal? */
EXTERNL int
nc_inq_type_equal(int ncid1, nc_type typeid1, int ncid2, 
		  nc_type typeid2, int *equal);

/* Create a group. its ncid is returned in the new_ncid pointer. */
EXTERNL int
nc_def_grp(int parent_ncid, const char *name, int *new_ncid);

/* Here are functions for dealing with compound types. */

/* Create a compound type. */
EXTERNL int
nc_def_compound(int ncid, size_t size, const char *name, nc_type *typeidp);

/* Insert a named field into a compound type. */
EXTERNL int
nc_insert_compound(int ncid, nc_type xtype, const char *name, 
		   size_t offset, nc_type field_typeid);

/* Insert a named array into a compound type. */
EXTERNL int
nc_insert_array_compound(int ncid, nc_type xtype, const char *name, 
			 size_t offset, nc_type field_typeid,
			 int ndims, const int *dim_sizes);

/* Get the name and size of a type. */
EXTERNL int
nc_inq_type(int ncid, nc_type xtype, char *name, size_t *size);

/* Get the id of a type from the name. */
EXTERNL int
nc_inq_typeid(int ncid, const char *name, nc_type *typeidp);

/* Get the name, size, and number of fields in a compound type. */
EXTERNL int
nc_inq_compound(int ncid, nc_type xtype, char *name, size_t *sizep, 
		size_t *nfieldsp);

/* Get the name of a compound type. */
EXTERNL int
nc_inq_compound_name(int ncid, nc_type xtype, char *name);

/* Get the size of a compound type. */
EXTERNL int
nc_inq_compound_size(int ncid, nc_type xtype, size_t *sizep);

/* Get the number of fields in this compound type. */
EXTERNL int
nc_inq_compound_nfields(int ncid, nc_type xtype, size_t *nfieldsp);

/* Given the xtype and the fieldid, get all info about it. */
EXTERNL int
nc_inq_compound_field(int ncid, nc_type xtype, int fieldid, char *name, 
		      size_t *offsetp, nc_type *field_typeidp, int *ndimsp, 
		      int *dim_sizesp);

/* Given the typeid and the fieldid, get the name. */
EXTERNL int
nc_inq_compound_fieldname(int ncid, nc_type xtype, int fieldid, 
			  char *name);

/* Given the xtype and the name, get the fieldid. */
EXTERNL int
nc_inq_compound_fieldindex(int ncid, nc_type xtype, const char *name, 
			   int *fieldidp);

/* Given the xtype and fieldid, get the offset. */
EXTERNL int
nc_inq_compound_fieldoffset(int ncid, nc_type xtype, int fieldid, 
			    size_t *offsetp);

/* Given the xtype and the fieldid, get the type of that field. */
EXTERNL int
nc_inq_compound_fieldtype(int ncid, nc_type xtype, int fieldid, 
			  nc_type *field_typeidp);

/* Given the xtype and the fieldid, get the number of dimensions for
 * that field (scalars are 0). */
EXTERNL int
nc_inq_compound_fieldndims(int ncid, nc_type xtype, int fieldid, 
			   int *ndimsp);

/* Given the xtype and the fieldid, get the sizes of dimensions for
 * that field. User must have allocated storage for the dim_sizes. */
EXTERNL int
nc_inq_compound_fielddim_sizes(int ncid, nc_type xtype, int fieldid, 
			       int *dim_sizes);

/* This is the type of arrays of vlens. */
typedef struct {
    size_t len; /* Length of VL data (in base type units) */
    void *p;    /* Pointer to VL data */
} nc_vlen_t;

/* This is used when creating a compound type. It calls a mysterious C
 * macro which was found carved into one of the blocks of the
 * Newgrange passage tomb in County Meath, Ireland. This code has been
 * carbon dated to 3200 B.C.E. */
#define NC_COMPOUND_OFFSET(S,M)    (offsetof(S,M))

/* Create a variable length type. */
EXTERNL int
nc_def_vlen(int ncid, const char *name, nc_type base_typeid, nc_type *xtypep);

/* Find out about a vlen. */
EXTERNL int
nc_inq_vlen(int ncid, nc_type xtype, char *name, size_t *datum_sizep, 
	    nc_type *base_nc_typep);

/* When you read VLEN type the library will actually allocate the
 * storage space for the data. This storage space must be freed, so
 * pass the pointer back to this function, when you're done with the
 * data, and it will free the vlen memory. */
EXTERNL int
nc_free_vlen(nc_vlen_t *vl);

EXTERNL int
nc_free_vlens(size_t len, nc_vlen_t vlens[]);

/* Put or get one element in a vlen array. */
EXTERNL int
nc_put_vlen_element(int ncid, int typeid1, void *vlen_element, 
		    size_t len, const void *data);

EXTERNL int
nc_get_vlen_element(int ncid, int typeid1, const void *vlen_element, 
		    size_t *len, void *data);
   
/* When you read the string type the library will allocate the storage
 * space for the data. This storage space must be freed, so pass the
 * pointer back to this function, when you're done with the data, and
 * it will free the string memory. */
EXTERNL int
nc_free_string(size_t len, char **data);

/* Find out about a user defined type. */
EXTERNL int
nc_inq_user_type(int ncid, nc_type xtype, char *name, size_t *size, 
		 nc_type *base_nc_typep, size_t *nfieldsp, int *classp);

/* Write an attribute of any type. */
EXTERNL int
nc_put_att(int ncid, int varid, const char *name, nc_type xtype, 
	   size_t len, const void *op);

/* Read an attribute of any type. */
EXTERNL int
nc_get_att(int ncid, int varid, const char *name, void *ip);

/* Enum type. */

/* Create an enum type. Provide a base type and a name. At the moment
 * only ints are accepted as base types. */
EXTERNL int
nc_def_enum(int ncid, nc_type base_typeid, const char *name, 
	    nc_type *typeidp);

/* Insert a named value into an enum type. The value must fit within
 * the size of the enum type, the name size must be <= NC_MAX_NAME. */
EXTERNL int
nc_insert_enum(int ncid, nc_type xtype, const char *name, 
	       const void *value);

/* Get information about an enum type: its name, base type and the
 * number of members defined. */
EXTERNL int
nc_inq_enum(int ncid, nc_type xtype, char *name, nc_type *base_nc_typep, 
	    size_t *base_sizep, size_t *num_membersp);

/* Get information about an enum member: a name and value. Name size
 * will be <= NC_MAX_NAME. */
EXTERNL int
nc_inq_enum_member(int ncid, nc_type xtype, int idx, char *name, 
		   void *value);


/* Get enum name from enum value. Name size will be <= NC_MAX_NAME. */
EXTERNL int
nc_inq_enum_ident(int ncid, nc_type xtype, long long value, char *identifier);

/* Opaque type. */

/* Create an opaque type. Provide a size and a name. */
EXTERNL int
nc_def_opaque(int ncid, size_t size, const char *name, nc_type *xtypep);

/* Get information about an opaque type. */
EXTERNL int
nc_inq_opaque(int ncid, nc_type xtype, char *name, size_t *sizep);

/* Write entire var of any type. */
EXTERNL int
nc_put_var(int ncid, int varid,  const void *op);

/* Read entire var of any type. */
EXTERNL int
nc_get_var(int ncid, int varid,  void *ip);

/* Write one value. */
EXTERNL int
nc_put_var1(int ncid, int varid,  const size_t *indexp,
	    const void *op);

/* Read one value. */
EXTERNL int
nc_get_var1(int ncid, int varid,  const size_t *indexp, void *ip);

/* Write an array of values. */
EXTERNL int
nc_put_vara(int ncid, int varid,  const size_t *startp, 
	    const size_t *countp, const void *op);

/* Read an array of values. */
EXTERNL int
nc_get_vara(int ncid, int varid,  const size_t *startp, 
	    const size_t *countp, void *ip);

/* Write slices of an array of values. */
EXTERNL int
nc_put_vars(int ncid, int varid,  const size_t *startp, 
	    const size_t *countp, const ptrdiff_t *stridep,
	    const void *op);

/* Read slices of an array of values. */
EXTERNL int
nc_get_vars(int ncid, int varid,  const size_t *startp, 
	    const size_t *countp, const ptrdiff_t *stridep,
	    void *ip);

/* Write mapped slices of an array of values. */
EXTERNL int
nc_put_varm(int ncid, int varid,  const size_t *startp, 
	    const size_t *countp, const ptrdiff_t *stridep,
	    const ptrdiff_t *imapp, const void *op);

/* Read mapped slices of an array of values. */
EXTERNL int
nc_get_varm(int ncid, int varid,  const size_t *startp, 
	    const size_t *countp, const ptrdiff_t *stridep,
	    const ptrdiff_t *imapp, void *ip);

/* Extra netcdf-4 stuff. */

/* Set compression settings for a variable. Lower is faster, higher is
 * better. Must be called after nc_def_var and before nc_enddef. */
EXTERNL int
nc_def_var_deflate(int ncid, int varid, int shuffle, int deflate, 
		   int deflate_level);

/* Find out compression settings of a var. */
EXTERNL int
nc_inq_var_deflate(int ncid, int varid, int *shufflep, 
		   int *deflatep, int *deflate_levelp);

/* Find out szip settings of a var. */
EXTERNL int
nc_inq_var_szip(int ncid, int varid, int *options_maskp, int *pixels_per_blockp);

/* Set fletcher32 checksum for a var. This must be done after nc_def_var
   and before nc_enddef. */
EXTERNL int
nc_def_var_fletcher32(int ncid, int varid, int fletcher32);
   
/* Inquire about fletcher32 checksum for a var. */
EXTERNL int
nc_inq_var_fletcher32(int ncid, int varid, int *fletcher32p);

/* Define chunking for a variable. This must be done after nc_def_var
   and before nc_enddef. */
EXTERNL int
nc_def_var_chunking(int ncid, int varid, int storage, const size_t *chunksizesp);

/* Inq chunking stuff for a var. */
EXTERNL int
nc_inq_var_chunking(int ncid, int varid, int *storagep, size_t *chunksizesp);

/* Define fill value behavior for a variable. This must be done after
   nc_def_var and before nc_enddef. */
EXTERNL int
nc_def_var_fill(int ncid, int varid, int no_fill, const void *fill_value);

/* Inq fill value setting for a var. */
EXTERNL int
nc_inq_var_fill(int ncid, int varid, int *no_fill, void *fill_value);

/* Define the endianness of a variable. */
EXTERNL int
nc_def_var_endian(int ncid, int varid, int endian);

/* Learn about the endianness of a variable. */
EXTERNL int
nc_inq_var_endian(int ncid, int varid, int *endianp);

/* Set the fill mode (classic or 64-bit offset files only). */
EXTERNL int
nc_set_fill(int ncid, int fillmode, int *old_modep);

/* Set the default nc_create format to NC_FORMAT_CLASSIC,
 * NC_FORMAT_64BIT, NC_FORMAT_NETCDF4, NC_FORMAT_NETCDF4_CLASSIC. */
EXTERNL int
nc_set_default_format(int format, int *old_formatp);

/* Set the cache size, nelems, and preemption policy. */
EXTERNL int
nc_set_chunk_cache(size_t size, size_t nelems, float preemption);

/* Get the cache size, nelems, and preemption policy. */
EXTERNL int
nc_get_chunk_cache(size_t *sizep, size_t *nelemsp, float *preemptionp);

/* Set the per-variable cache size, nelems, and preemption policy. */
EXTERNL int
nc_set_var_chunk_cache(int ncid, int varid, size_t size, size_t nelems, 
		       float preemption);

/* Set the per-variable cache size, nelems, and preemption policy. */
EXTERNL int
nc_get_var_chunk_cache(int ncid, int varid, size_t *sizep, size_t *nelemsp, 
		       float *preemptionp);

EXTERNL int
nc_redef(int ncid);

EXTERNL int
nc__enddef(int ncid, size_t h_minfree, size_t v_align,
	size_t v_minfree, size_t r_align);

EXTERNL int
nc_enddef(int ncid);

EXTERNL int
nc_sync(int ncid);

EXTERNL int
nc_abort(int ncid);

EXTERNL int
nc_close(int ncid);

EXTERNL int
nc_inq(int ncid, int *ndimsp, int *nvarsp, int *nattsp, int *unlimdimidp);

EXTERNL int 
nc_inq_ndims(int ncid, int *ndimsp);

EXTERNL int 
nc_inq_nvars(int ncid, int *nvarsp);

EXTERNL int 
nc_inq_natts(int ncid, int *nattsp);

EXTERNL int 
nc_inq_unlimdim(int ncid, int *unlimdimidp);

/* The next function is for NetCDF-4 only */
EXTERNL int 
nc_inq_unlimdims(int ncid, int *nunlimdimsp, int *unlimdimidsp);

/* Added in 3.6.1 to return format of netCDF file. */
EXTERNL int
nc_inq_format(int ncid, int *formatp);

/* Begin _dim */

EXTERNL int
nc_def_dim(int ncid, const char *name, size_t len, int *idp);

EXTERNL int
nc_inq_dimid(int ncid, const char *name, int *idp);

EXTERNL int
nc_inq_dim(int ncid, int dimid, char *name, size_t *lenp);

EXTERNL int 
nc_inq_dimname(int ncid, int dimid, char *name);

EXTERNL int 
nc_inq_dimlen(int ncid, int dimid, size_t *lenp);

EXTERNL int
nc_rename_dim(int ncid, int dimid, const char *name);

/* End _dim */
/* Begin _att */

EXTERNL int
nc_inq_att(int ncid, int varid, const char *name,
	   nc_type *xtypep, size_t *lenp);

EXTERNL int 
nc_inq_attid(int ncid, int varid, const char *name, int *idp);

EXTERNL int 
nc_inq_atttype(int ncid, int varid, const char *name, nc_type *xtypep);

EXTERNL int 
nc_inq_attlen(int ncid, int varid, const char *name, size_t *lenp);

EXTERNL int
nc_inq_attname(int ncid, int varid, int attnum, char *name);

EXTERNL int
nc_copy_att(int ncid_in, int varid_in, const char *name, int ncid_out, int varid_out);

EXTERNL int
nc_rename_att(int ncid, int varid, const char *name, const char *newname);

EXTERNL int
nc_del_att(int ncid, int varid, const char *name);

/* End _att */
/* Begin {put,get}_att */

EXTERNL int
nc_put_att_text(int ncid, int varid, const char *name,
		size_t len, const char *op);

EXTERNL int
nc_get_att_text(int ncid, int varid, const char *name, char *ip);

EXTERNL int
nc_put_att_uchar(int ncid, int varid, const char *name, nc_type xtype,
		 size_t len, const unsigned char *op);

EXTERNL int
nc_get_att_uchar(int ncid, int varid, const char *name, unsigned char *ip);

EXTERNL int
nc_put_att_schar(int ncid, int varid, const char *name, nc_type xtype,
		 size_t len, const signed char *op);

EXTERNL int
nc_get_att_schar(int ncid, int varid, const char *name, signed char *ip);

EXTERNL int
nc_put_att_short(int ncid, int varid, const char *name, nc_type xtype,
		 size_t len, const short *op);

EXTERNL int
nc_get_att_short(int ncid, int varid, const char *name, short *ip);

EXTERNL int
nc_put_att_int(int ncid, int varid, const char *name, nc_type xtype,
	       size_t len, const int *op);

EXTERNL int
nc_get_att_int(int ncid, int varid, const char *name, int *ip);

EXTERNL int
nc_put_att_long(int ncid, int varid, const char *name, nc_type xtype,
		size_t len, const long *op);

EXTERNL int
nc_get_att_long(int ncid, int varid, const char *name, long *ip);

EXTERNL int
nc_put_att_float(int ncid, int varid, const char *name, nc_type xtype,
		 size_t len, const float *op);

EXTERNL int
nc_get_att_float(int ncid, int varid, const char *name, float *ip);

EXTERNL int
nc_put_att_double(int ncid, int varid, const char *name, nc_type xtype,
		  size_t len, const double *op);

EXTERNL int
nc_get_att_double(int ncid, int varid, const char *name, double *ip);

EXTERNL int
nc_put_att_ushort(int ncid, int varid, const char *name, nc_type xtype,
		  size_t len, const unsigned short *op);

EXTERNL int
nc_get_att_ushort(int ncid, int varid, const char *name, unsigned short *ip);

EXTERNL int
nc_put_att_uint(int ncid, int varid, const char *name, nc_type xtype,
		size_t len, const unsigned int *op);

EXTERNL int
nc_get_att_uint(int ncid, int varid, const char *name, unsigned int *ip);

EXTERNL int
nc_put_att_longlong(int ncid, int varid, const char *name, nc_type xtype,
		 size_t len, const long long *op);

EXTERNL int
nc_get_att_longlong(int ncid, int varid, const char *name, long long *ip);

EXTERNL int
nc_put_att_ulonglong(int ncid, int varid, const char *name, nc_type xtype,
		     size_t len, const unsigned long long *op);

EXTERNL int
nc_get_att_ulonglong(int ncid, int varid, const char *name, 
		     unsigned long long *ip);

EXTERNL int
nc_put_att_string(int ncid, int varid, const char *name, 
		  size_t len, const char **op);

EXTERNL int
nc_get_att_string(int ncid, int varid, const char *name, char **ip);

/* End {put,get}_att */
/* Begin _var */

EXTERNL int
nc_def_var(int ncid, const char *name, nc_type xtype, int ndims, 
	   const int *dimidsp, int *varidp);

EXTERNL int
nc_inq_var(int ncid, int varid, char *name, nc_type *xtypep, 
	   int *ndimsp, int *dimidsp, int *nattsp);

EXTERNL int
nc_inq_varid(int ncid, const char *name, int *varidp);

EXTERNL int 
nc_inq_varname(int ncid, int varid, char *name);

EXTERNL int 
nc_inq_vartype(int ncid, int varid, nc_type *xtypep);

EXTERNL int 
nc_inq_varndims(int ncid, int varid, int *ndimsp);

EXTERNL int 
nc_inq_vardimid(int ncid, int varid, int *dimidsp);

EXTERNL int 
nc_inq_varnatts(int ncid, int varid, int *nattsp);

EXTERNL int
nc_rename_var(int ncid, int varid, const char *name);

EXTERNL int
nc_copy_var(int ncid_in, int varid, int ncid_out);

#ifndef ncvarcpy
/* support the old name for now */
#define ncvarcpy(ncid_in, varid, ncid_out) ncvarcopy((ncid_in), (varid), (ncid_out))
#endif

/* End _var */
/* Begin {put,get}_var1 */

EXTERNL int
nc_put_var1_text(int ncid, int varid, const size_t *indexp, const char *op);

EXTERNL int
nc_get_var1_text(int ncid, int varid, const size_t *indexp, char *ip);

EXTERNL int
nc_put_var1_uchar(int ncid, int varid, const size_t *indexp,
		  const unsigned char *op);

EXTERNL int
nc_get_var1_uchar(int ncid, int varid, const size_t *indexp,
		  unsigned char *ip);

EXTERNL int
nc_put_var1_schar(int ncid, int varid, const size_t *indexp,
		  const signed char *op);

EXTERNL int
nc_get_var1_schar(int ncid, int varid, const size_t *indexp,
		  signed char *ip);

EXTERNL int
nc_put_var1_short(int ncid, int varid, const size_t *indexp,
		  const short *op);

EXTERNL int
nc_get_var1_short(int ncid, int varid, const size_t *indexp,
		  short *ip);

EXTERNL int
nc_put_var1_int(int ncid, int varid, const size_t *indexp, const int *op);

EXTERNL int
nc_get_var1_int(int ncid, int varid, const size_t *indexp, int *ip);

EXTERNL int
nc_put_var1_long(int ncid, int varid, const size_t *indexp, const long *op);

EXTERNL int
nc_get_var1_long(int ncid, int varid, const size_t *indexp, long *ip);

EXTERNL int
nc_put_var1_float(int ncid, int varid, const size_t *indexp, const float *op);

EXTERNL int
nc_get_var1_float(int ncid, int varid, const size_t *indexp, float *ip);

EXTERNL int
nc_put_var1_double(int ncid, int varid, const size_t *indexp, const double *op);

EXTERNL int
nc_get_var1_double(int ncid, int varid, const size_t *indexp, double *ip);

EXTERNL int
nc_put_var1_ushort(int ncid, int varid, const size_t *indexp, 
		   const unsigned short *op);

EXTERNL int
nc_get_var1_ushort(int ncid, int varid, const size_t *indexp, 
		   unsigned short *ip);

EXTERNL int
nc_put_var1_uint(int ncid, int varid, const size_t *indexp, 
		 const unsigned int *op);

EXTERNL int
nc_get_var1_uint(int ncid, int varid, const size_t *indexp, 
		 unsigned int *ip);

EXTERNL int
nc_put_var1_longlong(int ncid, int varid, const size_t *indexp, 
		     const long long *op);

EXTERNL int
nc_get_var1_longlong(int ncid, int varid, const size_t *indexp, 
		  long long *ip);

EXTERNL int
nc_put_var1_ulonglong(int ncid, int varid, const size_t *indexp, 
		   const unsigned long long *op);

EXTERNL int
nc_get_var1_ulonglong(int ncid, int varid, const size_t *indexp, 
		   unsigned long long *ip);

EXTERNL int
nc_put_var1_string(int ncid, int varid, const size_t *indexp, 
		   const char **op);

EXTERNL int
nc_get_var1_string(int ncid, int varid, const size_t *indexp, 
		   char **ip);

/* End {put,get}_var1 */
/* Begin {put,get}_vara */

EXTERNL int
nc_put_vara_text(int ncid, int varid,
	const size_t *startp, const size_t *countp, const char *op);

EXTERNL int
nc_get_vara_text(int ncid, int varid,
	const size_t *startp, const size_t *countp, char *ip);

EXTERNL int
nc_put_vara_uchar(int ncid, int varid,
	const size_t *startp, const size_t *countp, const unsigned char *op);

EXTERNL int
nc_get_vara_uchar(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, unsigned char *ip);

EXTERNL int
nc_put_vara_schar(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const signed char *op);

EXTERNL int
nc_get_vara_schar(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, signed char *ip);

EXTERNL int
nc_put_vara_short(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const short *op);

EXTERNL int
nc_get_vara_short(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, short *ip);

EXTERNL int
nc_put_vara_int(int ncid, int varid, const size_t *startp, 
		const size_t *countp, const int *op);

EXTERNL int
nc_get_vara_int(int ncid, int varid, const size_t *startp, 
		const size_t *countp, int *ip);

EXTERNL int
nc_put_vara_long(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const long *op);

EXTERNL int
nc_get_vara_long(int ncid, int varid,
	const size_t *startp, const size_t *countp, long *ip);

EXTERNL int
nc_put_vara_float(int ncid, int varid,
	const size_t *startp, const size_t *countp, const float *op);

EXTERNL int
nc_get_vara_float(int ncid, int varid,
	const size_t *startp, const size_t *countp, float *ip);

EXTERNL int
nc_put_vara_double(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const double *op);

EXTERNL int
nc_get_vara_double(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, double *ip);

EXTERNL int
nc_put_vara_ushort(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const unsigned short *op);

EXTERNL int
nc_get_vara_ushort(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, unsigned short *ip);

EXTERNL int
nc_put_vara_uint(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const unsigned int *op);

EXTERNL int
nc_get_vara_uint(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, unsigned int *ip);

EXTERNL int
nc_put_vara_longlong(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const long long *op);

EXTERNL int
nc_get_vara_longlong(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, long long *ip);

EXTERNL int
nc_put_vara_ulonglong(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const unsigned long long *op);

EXTERNL int
nc_get_vara_ulonglong(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, unsigned long long *ip);

EXTERNL int
nc_put_vara_string(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const char **op);

EXTERNL int
nc_get_vara_string(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, char **ip);

/* End {put,get}_vara */
/* Begin {put,get}_vars */

EXTERNL int
nc_put_vars_text(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const char *op);

EXTERNL int
nc_get_vars_text(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	char *ip);

EXTERNL int
nc_put_vars_uchar(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const unsigned char *op);

EXTERNL int
nc_get_vars_uchar(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	unsigned char *ip);

EXTERNL int
nc_put_vars_schar(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const signed char *op);

EXTERNL int
nc_get_vars_schar(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	signed char *ip);

EXTERNL int
nc_put_vars_short(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const short *op);

EXTERNL int
nc_get_vars_short(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  short *ip);

EXTERNL int
nc_put_vars_int(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const int *op);

EXTERNL int
nc_get_vars_int(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	int *ip);

EXTERNL int
nc_put_vars_long(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const long *op);

EXTERNL int
nc_get_vars_long(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	long *ip);

EXTERNL int
nc_put_vars_float(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const float *op);

EXTERNL int
nc_get_vars_float(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	float *ip);

EXTERNL int
nc_put_vars_double(int ncid, int varid,
	const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
	const double *op);

EXTERNL int
nc_get_vars_double(int ncid, int varid,	const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep,
		   double *ip);

EXTERNL int
nc_put_vars_ushort(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const unsigned short *op);

EXTERNL int
nc_get_vars_ushort(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   unsigned short *ip);

EXTERNL int
nc_put_vars_uint(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep, 
		 const unsigned int *op);

EXTERNL int
nc_get_vars_uint(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep, 
		 unsigned int *ip);

EXTERNL int
nc_put_vars_longlong(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  const long long *op);

EXTERNL int
nc_get_vars_longlong(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  long long *ip);

EXTERNL int
nc_put_vars_ulonglong(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const unsigned long long *op);

EXTERNL int
nc_get_vars_ulonglong(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   unsigned long long *ip);

EXTERNL int
nc_put_vars_string(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const char **op);

EXTERNL int
nc_get_vars_string(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   char **ip);

/* End {put,get}_vars */
/* Begin {put,get}_varm */

EXTERNL int
nc_put_varm_text(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep,
		 const ptrdiff_t *imapp, const char *op);

EXTERNL int
nc_get_varm_text(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep,
		 const ptrdiff_t *imapp, char *ip);

EXTERNL int
nc_put_varm_uchar(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, const unsigned char *op);

EXTERNL int
nc_get_varm_uchar(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, unsigned char *ip);

EXTERNL int
nc_put_varm_schar(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, const signed char *op);

EXTERNL int
nc_get_varm_schar(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, signed char *ip);

EXTERNL int
nc_put_varm_short(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, const short *op);

EXTERNL int
nc_get_varm_short(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, short *ip);

EXTERNL int
nc_put_varm_int(int ncid, int varid, const size_t *startp, 
		const size_t *countp, const ptrdiff_t *stridep,
		const ptrdiff_t *imapp, const int *op);

EXTERNL int
nc_get_varm_int(int ncid, int varid, const size_t *startp, 
		const size_t *countp, const ptrdiff_t *stridep,
		const ptrdiff_t *imapp, int *ip);

EXTERNL int
nc_put_varm_long(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep,
		 const ptrdiff_t *imapp, const long *op);

EXTERNL int
nc_get_varm_long(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep,
		 const ptrdiff_t *imapp, long *ip);

EXTERNL int
nc_put_varm_float(int ncid, int varid,const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, const float *op);

EXTERNL int
nc_get_varm_float(int ncid, int varid,const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep,
		  const ptrdiff_t *imapp, float *ip);

EXTERNL int
nc_put_varm_double(int ncid, int varid,	const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep,
		   const ptrdiff_t *imapp, const double *op);

EXTERNL int
nc_get_varm_double(int ncid, int varid,	const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep,
		   const ptrdiff_t * imapp, double *ip);

EXTERNL int
nc_put_varm_ushort(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const ptrdiff_t * imapp, const unsigned short *op);

EXTERNL int
nc_get_varm_ushort(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const ptrdiff_t * imapp, unsigned short *ip);

EXTERNL int
nc_put_varm_uint(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep, 
		 const ptrdiff_t * imapp, const unsigned int *op);

EXTERNL int
nc_get_varm_uint(int ncid, int varid, const size_t *startp, 
		 const size_t *countp, const ptrdiff_t *stridep, 
		 const ptrdiff_t * imapp, unsigned int *ip);

EXTERNL int
nc_put_varm_longlong(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  const ptrdiff_t * imapp, const long long *op);

EXTERNL int
nc_get_varm_longlong(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  const ptrdiff_t * imapp, long long *ip);

EXTERNL int
nc_put_varm_ulonglong(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const ptrdiff_t * imapp, const unsigned long long *op);

EXTERNL int
nc_get_varm_ulonglong(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const ptrdiff_t * imapp, unsigned long long *ip);

EXTERNL int
nc_put_varm_string(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const ptrdiff_t * imapp, const char **op);

EXTERNL int
nc_get_varm_string(int ncid, int varid, const size_t *startp, 
		   const size_t *countp, const ptrdiff_t *stridep, 
		   const ptrdiff_t * imapp, char **ip);

/* End {put,get}_varm */
/* Begin {put,get}_var */

EXTERNL int
nc_put_var_text(int ncid, int varid, const char *op);

EXTERNL int
nc_get_var_text(int ncid, int varid, char *ip);

EXTERNL int
nc_put_var_uchar(int ncid, int varid, const unsigned char *op);

EXTERNL int
nc_get_var_uchar(int ncid, int varid, unsigned char *ip);

EXTERNL int
nc_put_var_schar(int ncid, int varid, const signed char *op);

EXTERNL int
nc_get_var_schar(int ncid, int varid, signed char *ip);

EXTERNL int
nc_put_var_short(int ncid, int varid, const short *op);

EXTERNL int
nc_get_var_short(int ncid, int varid, short *ip);

EXTERNL int
nc_put_var_int(int ncid, int varid, const int *op);

EXTERNL int
nc_get_var_int(int ncid, int varid, int *ip);

EXTERNL int
nc_put_var_long(int ncid, int varid, const long *op);

EXTERNL int
nc_get_var_long(int ncid, int varid, long *ip);

EXTERNL int
nc_put_var_float(int ncid, int varid, const float *op);

EXTERNL int
nc_get_var_float(int ncid, int varid, float *ip);

EXTERNL int
nc_put_var_double(int ncid, int varid, const double *op);

EXTERNL int
nc_get_var_double(int ncid, int varid, double *ip);

EXTERNL int
nc_put_var_ushort(int ncid, int varid, const unsigned short *op);

EXTERNL int
nc_get_var_ushort(int ncid, int varid, unsigned short *ip);

EXTERNL int
nc_put_var_uint(int ncid, int varid, const unsigned int *op);

EXTERNL int
nc_get_var_uint(int ncid, int varid, unsigned int *ip);

EXTERNL int
nc_put_var_longlong(int ncid, int varid, const long long *op);

EXTERNL int
nc_get_var_longlong(int ncid, int varid, long long *ip);

EXTERNL int
nc_put_var_ulonglong(int ncid, int varid, const unsigned long long *op);

EXTERNL int
nc_get_var_ulonglong(int ncid, int varid, unsigned long long *ip);

EXTERNL int
nc_put_var_string(int ncid, int varid, const char **op);

EXTERNL int
nc_get_var_string(int ncid, int varid, char **ip);

/* Begin Deprecated, same as functions with "_ubyte" replaced by "_uchar" */
EXTERNL int
nc_put_att_ubyte(int ncid, int varid, const char *name, nc_type xtype,
		 size_t len, const unsigned char *op);
EXTERNL int
nc_get_att_ubyte(int ncid, int varid, const char *name, 
		 unsigned char *ip);
EXTERNL int
nc_put_var1_ubyte(int ncid, int varid, const size_t *indexp, 
		  const unsigned char *op);
EXTERNL int
nc_get_var1_ubyte(int ncid, int varid, const size_t *indexp, 
		  unsigned char *ip);
EXTERNL int
nc_put_vara_ubyte(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const unsigned char *op);
EXTERNL int
nc_get_vara_ubyte(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, unsigned char *ip);
EXTERNL int
nc_put_vars_ubyte(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  const unsigned char *op);
EXTERNL int
nc_get_vars_ubyte(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  unsigned char *ip);
EXTERNL int
nc_put_varm_ubyte(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  const ptrdiff_t * imapp, const unsigned char *op);
EXTERNL int
nc_get_varm_ubyte(int ncid, int varid, const size_t *startp, 
		  const size_t *countp, const ptrdiff_t *stridep, 
		  const ptrdiff_t * imapp, unsigned char *ip);
EXTERNL int
nc_put_var_ubyte(int ncid, int varid, const unsigned char *op);
EXTERNL int
nc_get_var_ubyte(int ncid, int varid, unsigned char *ip);
/* End Deprecated */

#ifdef LOGGING

/* Set the log level. 0 shows only errors, 1 only major messages,
 * etc., to 5, which shows way too much information. */
EXTERNL int
nc_set_log_level(int new_level);

/* Use this to turn off logging by calling
   nc_log_level(NC_TURN_OFF_LOGGING) */
#define NC_TURN_OFF_LOGGING (-1)

#else /* not LOGGING */

#define nc_set_log_level(e)

#endif /* LOGGING */

/* Show the netCDF library's in-memory metadata for a file. */
EXTERNL int 
nc_show_metadata(int ncid);

/* End {put,get}_var */

/* #ifdef _CRAYMPP */
/*
 * Public interfaces to better support
 * CRAY multi-processor systems like T3E.
 * A tip of the hat to NERSC.
 */
/*
 * It turns out we need to declare and define
 * these public interfaces on all platforms
 * or things get ugly working out the
 * FORTRAN interface. On !_CRAYMPP platforms,
 * these functions work as advertised, but you
 * can only use "processor element" 0.
 */

EXTERNL int
nc__create_mp(const char *path, int cmode, size_t initialsz, int basepe,
	 size_t *chunksizehintp, int *ncidp);

EXTERNL int
nc__open_mp(const char *path, int mode, int basepe,
	size_t *chunksizehintp, int *ncidp);

EXTERNL int
nc_delete(const char *path);

EXTERNL int
nc_delete_mp(const char *path, int basepe);

EXTERNL int
nc_set_base_pe(int ncid, int pe);

EXTERNL int
nc_inq_base_pe(int ncid, int *pe);

/* #endif _CRAYMPP */

/* This v2 function is used in the nc_test program. */
EXTERNL int
nctypelen(nc_type datatype);

/* Begin v2.4 backward compatiblity */
/*
 * defining NO_NETCDF_2 to the preprocessor
 * turns off backward compatiblity declarations.
 */
#ifndef NO_NETCDF_2

/*
 * Backward compatible aliases
 */
#define FILL_BYTE	NC_FILL_BYTE
#define FILL_CHAR	NC_FILL_CHAR
#define FILL_SHORT	NC_FILL_SHORT
#define FILL_LONG	NC_FILL_INT
#define FILL_FLOAT	NC_FILL_FLOAT
#define FILL_DOUBLE	NC_FILL_DOUBLE

#define MAX_NC_DIMS	NC_MAX_DIMS
#define MAX_NC_ATTRS	NC_MAX_ATTRS
#define MAX_NC_VARS	NC_MAX_VARS
#define MAX_NC_NAME	NC_MAX_NAME
#define MAX_VAR_DIMS	NC_MAX_VAR_DIMS


/*
 * Global error status
 */
EXTERNL int ncerr;

#define NC_ENTOOL       NC_EMAXNAME   /* Backward compatibility */
#define	NC_EXDR		(-32)	/* */
#define	NC_SYSERR	(-31)

/*
 * Global options variable.
 * Used to determine behavior of error handler.
 */
#define	NC_FATAL	1
#define	NC_VERBOSE	2

EXTERNL int ncopts;	/* default is (NC_FATAL | NC_VERBOSE) */

EXTERNL void
nc_advise(const char *cdf_routine_name, int err, const char *fmt,...);

/*
 * C data type corresponding to a netCDF NC_LONG argument,
 * a signed 32 bit object.
 * 
 * This is the only thing in this file which architecture dependent.
 */
typedef int nclong;

EXTERNL int
nccreate(const char* path, int cmode);

EXTERNL int
ncopen(const char* path, int mode);

EXTERNL int
ncsetfill(int ncid, int fillmode);

EXTERNL int
ncredef(int ncid);

EXTERNL int
ncendef(int ncid);

EXTERNL int
ncsync(int ncid);

EXTERNL int
ncabort(int ncid);

EXTERNL int
ncclose(int ncid);

EXTERNL int
ncinquire(int ncid, int *ndimsp, int *nvarsp, int *nattsp, int *unlimdimp);

EXTERNL int
ncdimdef(int ncid, const char *name, long len);

EXTERNL int
ncdimid(int ncid, const char *name);

EXTERNL int
ncdiminq(int ncid, int dimid, char *name, long *lenp);

EXTERNL int
ncdimrename(int ncid, int dimid, const char *name);

EXTERNL int
ncattput(int ncid, int varid, const char *name, nc_type xtype,
	int len, const void *op);

EXTERNL int
ncattinq(int ncid, int varid, const char *name, nc_type *xtypep, int *lenp);

EXTERNL int
ncattget(int ncid, int varid, const char *name, void *ip);

EXTERNL int
ncattcopy(int ncid_in, int varid_in, const char *name, int ncid_out,
	int varid_out);

EXTERNL int
ncattname(int ncid, int varid, int attnum, char *name);

EXTERNL int
ncattrename(int ncid, int varid, const char *name, const char *newname);

EXTERNL int
ncattdel(int ncid, int varid, const char *name);

EXTERNL int
ncvardef(int ncid, const char *name, nc_type xtype,
	int ndims, const int *dimidsp);

EXTERNL int
ncvarid(int ncid, const char *name);

EXTERNL int
ncvarinq(int ncid, int varid, char *name, nc_type *xtypep,
	int *ndimsp, int *dimidsp, int *nattsp);

EXTERNL int
ncvarput1(int ncid, int varid, const long *indexp, const void *op);

EXTERNL int
ncvarget1(int ncid, int varid, const long *indexp, void *ip);

EXTERNL int
ncvarput(int ncid, int varid, const long *startp, const long *countp,
	const void *op);

EXTERNL int
ncvarget(int ncid, int varid, const long *startp, const long *countp, 
	void *ip);

EXTERNL int
ncvarputs(int ncid, int varid, const long *startp, const long *countp,
	const long *stridep, const void *op);

EXTERNL int
ncvargets(int ncid, int varid, const long *startp, const long *countp,
	const long *stridep, void *ip);

EXTERNL int
ncvarputg(int ncid, int varid, const long *startp, const long *countp,
	const long *stridep, const long *imapp, const void *op);

EXTERNL int
ncvargetg(int ncid, int varid, const long *startp, const long *countp,
	const long *stridep, const long *imapp, void *ip);

EXTERNL int
ncvarrename(int ncid, int varid, const char *name);

EXTERNL int
ncrecinq(int ncid, int *nrecvarsp, int *recvaridsp, long *recsizesp);

EXTERNL int
ncrecget(int ncid, long recnum, void **datap);

EXTERNL int
ncrecput(int ncid, long recnum, void *const *datap);

/* End v2.4 backward compatiblity */
#endif /*!NO_NETCDF_2*/

#if defined(__cplusplus)
}
#endif

#endif /* _NETCDF_ */
