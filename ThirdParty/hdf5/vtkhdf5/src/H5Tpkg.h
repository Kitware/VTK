/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose: This file contains declarations which are visible only within
 *          the H5T package.  Source files outside the H5T package should
 *          include H5Tprivate.h instead.
 */
#if !(defined H5T_FRIEND || defined H5T_MODULE)
#error "Do not include this file outside the H5T package!"
#endif

#ifndef H5Tpkg_H
#define H5Tpkg_H

/*
 * Define this to enable debugging.
 */
#ifdef NDEBUG
#undef H5T_DEBUG
#endif

/* Get package's private header */
#include "H5Tprivate.h"

/* Other private headers needed by this file */
#include "H5Fprivate.h"  /* Files				*/
#include "H5FLprivate.h" /* Free Lists				*/
#include "H5Oprivate.h"  /* Object headers		  	*/
#include "H5VLprivate.h" /* Virtual Object Layer                     */

/* Other public headers needed by this file */
#include "H5Spublic.h" /* Dataspace functions			*/

/* Macro to ease detecting "complex" datatypes (i.e. those with base types or fields) */
#define H5T_IS_COMPLEX(t)                                                                                    \
    ((t) == H5T_COMPOUND || (t) == H5T_ENUM || (t) == H5T_VLEN || (t) == H5T_ARRAY || (t) == H5T_REFERENCE)

/* Macro to ease detecting fixed "string" datatypes */
#define H5T_IS_FIXED_STRING(dt) (H5T_STRING == (dt)->type)

/* Macro to ease detecting variable-length "string" datatypes */
#define H5T_IS_VL_STRING(dt) (H5T_VLEN == (dt)->type && H5T_VLEN_STRING == (dt)->u.vlen.type)

/* Macro to ease detecting fixed or variable-length "string" datatypes */
#define H5T_IS_STRING(dt) (H5T_IS_FIXED_STRING(dt) || H5T_IS_VL_STRING(dt))

/* Macro to ease detecting atomic datatypes */
#define H5T_IS_ATOMIC(dt) (!(H5T_IS_COMPLEX((dt)->type) || (dt)->type == H5T_OPAQUE))

/* Macro to ease retrieving class of shared datatype */
/* (Externally, a VL string is a string; internally, a VL string is a VL.  Lie
 *      to the user if they have a VL string and tell them it's in the string
 *      class)
 */
#define H5T_GET_CLASS(shared, internal)                                                                      \
    ((internal) ? (shared)->type : (H5T_IS_VL_STRING(shared) ? H5T_STRING : (shared)->type))

/*
 * Datatype encoding versions
 */

/* This is the version to create all datatypes which don't contain
 * array datatypes (atomic types, compound datatypes without array fields,
 * vlen sequences of objects which aren't arrays, etc.) or VAX byte-ordered
 * objects.
 */
#define H5O_DTYPE_VERSION_1 1

/* This is the version to create all datatypes which contain H5T_ARRAY
 * class objects (array definitely, potentially compound & vlen sequences also),
 * but not VAX byte-ordered objects.
 */
#define H5O_DTYPE_VERSION_2 2

/* This is the version to create all datatypes which contain VAX byte-ordered
 * objects (floating-point types, currently).
 */
/* This version also packs compound & enum field names without padding */
/* This version also encodes the member offset of compound fields more efficiently */
/* This version also encodes array types more efficiently */
#define H5O_DTYPE_VERSION_3 3

/* This is the version that adds support for new reference types and prevents
 * older versions of the library to attempt reading unknown types.
 */
#define H5O_DTYPE_VERSION_4 4

/* The latest version of the format.  Look through the 'encode helper' routine
 *      and 'size' callback for places to change when updating this. */
#define H5O_DTYPE_VERSION_LATEST H5O_DTYPE_VERSION_4

/* Flags for visiting datatype */
#define H5T_VISIT_COMPLEX_FIRST 0x01 /* Visit complex datatype before visiting member/parent datatypes */
#define H5T_VISIT_COMPLEX_LAST  0x02 /* Visit complex datatype after visiting member/parent datatypes */
                                     /* (setting both flags will mean visiting complex type twice) */
#define H5T_VISIT_SIMPLE 0x04        /* Visit simple datatypes (at all) */
/* (setting H5T_VISIT_SIMPLE and _not_ setting either H5T_VISIT_COMPLEX_FIRST or H5T_VISIT_COMPLEX_LAST will
 * mean visiting _only_ "simple" "leafs" in the "tree" */
/* (_not_ setting H5T_VISIT_SIMPLE and setting either H5T_VISIT_COMPLEX_FIRST or H5T_VISIT_COMPLEX_LAST will
 * mean visiting all nodes _except_ "simple" "leafs" in the "tree" */

/* Define an internal macro for converting long long to long double.  Mac OS 10.4 gives some
 * incorrect conversions. */
#if (H5_WANT_DATA_ACCURACY && defined(H5_LLONG_TO_LDOUBLE_CORRECT)) || (!H5_WANT_DATA_ACCURACY)
#define H5T_CONV_INTERNAL_LLONG_LDOUBLE 1
#endif

/* Define an internal macro for converting unsigned long long to long double.  SGI compilers give
 * some incorrect conversion.  64-bit Solaris does different rounding.   Windows Visual Studio 6 does
 * not support unsigned long long.  For FreeBSD(sleipnir), the last 2 bytes of mantissa are lost when
 * compiler tries to do the conversion.  For Cygwin, compiler doesn't do rounding correctly.
 * Mac OS 10.4 gives some incorrect result. */
#if (H5_WANT_DATA_ACCURACY && defined(H5_LLONG_TO_LDOUBLE_CORRECT)) || (!H5_WANT_DATA_ACCURACY)
#define H5T_CONV_INTERNAL_ULLONG_LDOUBLE 1
#endif

/* Define an internal macro for converting long double to long long.  SGI compilers give some incorrect
 * conversions. Mac OS 10.4 gives incorrect conversions. HP-UX 11.00 compiler generates floating exception.
 * The hard conversion on Windows .NET 2003 has a bug and gives wrong exception value. */
#if (H5_WANT_DATA_ACCURACY && defined(H5_LDOUBLE_TO_LLONG_ACCURATE)) || (!H5_WANT_DATA_ACCURACY)
#define H5T_CONV_INTERNAL_LDOUBLE_LLONG 1
#endif

/* Define an internal macro for converting long double to unsigned long long.  SGI compilers give some
 * incorrect conversions.  Mac OS 10.4 gives incorrect conversions. HP-UX 11.00 compiler generates
 * floating exception. */
#if (H5_WANT_DATA_ACCURACY && defined(H5_LDOUBLE_TO_LLONG_ACCURATE)) || (!H5_WANT_DATA_ACCURACY)
#define H5T_CONV_INTERNAL_LDOUBLE_ULLONG 1
#else
#define H5T_CONV_INTERNAL_LDOUBLE_ULLONG 0
#endif

/* Define an internal macro for converting long double to _Float16. Mac OS 13
 * gives incorrect conversions that appear to be resolved in Mac OS 14. */
#ifdef H5_HAVE__FLOAT16
#if (H5_WANT_DATA_ACCURACY && defined(H5_LDOUBLE_TO_FLOAT16_CORRECT)) || (!H5_WANT_DATA_ACCURACY)
#define H5T_CONV_INTERNAL_LDOUBLE_FLOAT16 1
#endif
#endif

/* Reference function pointers */
typedef herr_t (*H5T_ref_isnullfunc_t)(const H5VL_object_t *file, const void *src_buf, bool *isnull);
typedef herr_t (*H5T_ref_setnullfunc_t)(H5VL_object_t *file, void *dst_buf, void *bg_buf);
typedef size_t (*H5T_ref_getsizefunc_t)(H5VL_object_t *src_file, const void *src_buf, size_t src_size,
                                        H5VL_object_t *dst_file, bool *dst_copy);
typedef herr_t (*H5T_ref_readfunc_t)(H5VL_object_t *src_file, const void *src_buf, size_t src_size,
                                     H5VL_object_t *dst_file, void *dst_buf, size_t dst_size);
typedef herr_t (*H5T_ref_writefunc_t)(H5VL_object_t *src_file, const void *src_buf, size_t src_size,
                                      H5R_type_t src_type, H5VL_object_t *dst_file, void *dst_buf,
                                      size_t dst_size, void *bg_buf);

typedef struct H5T_ref_class_t {
    H5T_ref_isnullfunc_t  isnull;  /* check if reference value is NIL */
    H5T_ref_setnullfunc_t setnull; /* set a reference value to NIL */
    H5T_ref_getsizefunc_t getsize; /* get reference size (bytes)   */
    H5T_ref_readfunc_t    read;    /* read reference into buffer   */
    H5T_ref_writefunc_t   write;   /* write reference from buffer  */
} H5T_ref_class_t;

typedef struct H5T_atomic_t {
    H5T_order_t order;   /* byte order                           */
    size_t      prec;    /* precision in bits                    */
    size_t      offset;  /* bit position of lsb of value         */
    H5T_pad_t   lsb_pad; /* type of lsb padding                  */
    H5T_pad_t   msb_pad; /* type of msb padding                  */
    union {
        struct {
            H5T_sign_t sign; /* type of integer sign                 */
        } i;                 /* integer; integer types */

        struct {
            size_t     sign;  /* bit position of sign bit             */
            size_t     epos;  /* position of lsb of exponent          */
            size_t     esize; /* size of exponent in bits             */
            uint64_t   ebias; /* exponent bias                        */
            size_t     mpos;  /* position of lsb of mantissa          */
            size_t     msize; /* size of mantissa                     */
            H5T_norm_t norm;  /* normalization                        */
            H5T_pad_t  pad;   /* type of padding for internal bits    */
        } f;                  /* floating-point types */

        struct {
            H5T_cset_t cset; /* character set                        */
            H5T_str_t  pad;  /* space or null padding of extra bytes */
        } s;                 /* string types */

        struct {
            H5R_type_t             rtype;   /* type of reference stored             */
            unsigned               version; /* version of encoded reference         */
            bool                   opaque;  /* opaque reference type                */
            H5T_loc_t              loc;     /* location of data in buffer           */
            H5VL_object_t         *file;    /* file VOL pointer (if data is on disk) */
            const H5T_ref_class_t *cls;     /* Pointer to ref class callbacks */
        } r;                                /* reference types */
    } u;
} H5T_atomic_t;

/* How members are sorted for compound or enum datatypes */
typedef enum H5T_sort_t {
    H5T_SORT_NONE  = 0, /*not sorted			     */
    H5T_SORT_NAME  = 1, /*sorted by member name		     */
    H5T_SORT_VALUE = 2  /*sorted by memb offset or enum value*/
} H5T_sort_t;

/* A compound datatype member */
typedef struct H5T_cmemb_t {
    char         *name;   /*name of this member		     */
    size_t        offset; /*offset from beginning of struct    */
    size_t        size;   /*size of this member		     */
    struct H5T_t *type;   /*type of this member		     */
} H5T_cmemb_t;

/* A compound datatype */
typedef struct H5T_compnd_t {
    unsigned     nalloc;    /*num entries allocated in MEMB array*/
    unsigned     nmembs;    /*number of members defined in struct*/
    H5T_sort_t   sorted;    /*how are members sorted?	     */
    bool         packed;    /*are members packed together?       */
    H5T_cmemb_t *memb;      /*array of struct members	     */
    size_t       memb_size; /*total of all member sizes          */
} H5T_compnd_t;

/* An enumeration datatype */
typedef struct H5T_enum_t {
    unsigned   nalloc; /*num entries allocated		     */
    unsigned   nmembs; /*number of members defined in enum  */
    H5T_sort_t sorted; /*how are members sorted?	     */
    void      *value;  /*array of values		     */
    char     **name;   /*array of symbol names		     */
} H5T_enum_t;

/* VL types */
typedef enum {
    H5T_VLEN_BADTYPE  = -1, /* invalid VL Type */
    H5T_VLEN_SEQUENCE = 0,  /* VL sequence */
    H5T_VLEN_STRING,        /* VL string */
    H5T_VLEN_MAXTYPE        /* highest type (Invalid as true type) */
} H5T_vlen_type_t;

/* VL function pointers */
typedef herr_t (*H5T_vlen_getlen_func_t)(H5VL_object_t *file, const void *vl_addr, size_t *len);
typedef void *(*H5T_vlen_getptr_func_t)(void *vl_addr);
typedef herr_t (*H5T_vlen_isnull_func_t)(const H5VL_object_t *file, void *vl_addr, bool *isnull);
typedef herr_t (*H5T_vlen_setnull_func_t)(H5VL_object_t *file, void *_vl, void *_bg);
typedef herr_t (*H5T_vlen_read_func_t)(H5VL_object_t *file, void *_vl, void *buf, size_t len);
typedef herr_t (*H5T_vlen_write_func_t)(H5VL_object_t *file, const H5T_vlen_alloc_info_t *vl_alloc_info,
                                        void *_vl, void *buf, void *_bg, size_t seq_len, size_t base_size);
typedef herr_t (*H5T_vlen_delete_func_t)(H5VL_object_t *file, void *_vl);

/* VL datatype callbacks */
typedef struct H5T_vlen_class_t {
    H5T_vlen_getlen_func_t  getlen;  /* Function to get VL sequence size (in element units, not bytes) */
    H5T_vlen_getptr_func_t  getptr;  /* Function to get VL sequence pointer */
    H5T_vlen_isnull_func_t  isnull;  /* Function to check if VL value is NIL */
    H5T_vlen_setnull_func_t setnull; /* Function to set a VL value to NIL */
    H5T_vlen_read_func_t    read;    /* Function to read VL sequence into buffer */
    H5T_vlen_write_func_t   write;   /* Function to write VL sequence from buffer */
    H5T_vlen_delete_func_t  del;     /* Function to delete VL sequence */
} H5T_vlen_class_t;

/* A VL datatype */
typedef struct H5T_vlen_t {
    H5T_vlen_type_t type;         /* Type of VL data in buffer */
    H5T_loc_t       loc;          /* Location of VL data in buffer */
    H5T_cset_t      cset;         /* For VL string: character set */
    H5T_str_t       pad;          /* For VL string: space or null padding of
                                   * extra bytes */
    H5VL_object_t          *file; /* File object (if VL data is on disk) */
    const H5T_vlen_class_t *cls;  /* Pointer to VL class callbacks */
} H5T_vlen_t;

/* An opaque datatype */
typedef struct H5T_opaque_t {
    char *tag; /*short type description string	     */
} H5T_opaque_t;

/* An array datatype */
typedef struct H5T_array_t {
    size_t   nelem;             /* total number of elements in array */
    unsigned ndims;             /* member dimensionality        */
    size_t   dim[H5S_MAX_RANK]; /* size in each dimension       */
} H5T_array_t;

typedef enum H5T_state_t {
    H5T_STATE_TRANSIENT, /*type is a modifiable, closable transient */
    H5T_STATE_RDONLY,    /*transient, not modifiable, closable */
    H5T_STATE_IMMUTABLE, /*transient, not modifiable, not closable */
    H5T_STATE_NAMED,     /*named constant, not open	     */
    H5T_STATE_OPEN       /*named constant, open object header */
} H5T_state_t;

/* This struct is shared between all occurrences of an open named type */
typedef struct H5T_shared_t {
    hsize_t     fo_count; /* number of references to this file object */
    H5T_state_t state;    /*current state of the type		     */
    H5T_class_t type;     /*which class of type is this?		     */
    size_t      size;     /*total size of an instance of this type     */
    unsigned    version;  /* Version of object header message to encode this object with */
    bool force_conv; /* Set if this type always needs to be converted and H5T__conv_noop cannot be called */
    struct H5T_t  *parent;        /*parent type for derived datatypes	     */
    H5VL_object_t *owned_vol_obj; /* Vol object owned by this type (free on close) */
    union {
        H5T_atomic_t atomic; /* an atomic datatype              */
        H5T_compnd_t compnd; /* a compound datatype (struct)    */
        H5T_enum_t   enumer; /* an enumeration type (enum)       */
        H5T_vlen_t   vlen;   /* a variable-length datatype       */
        H5T_opaque_t opaque; /* an opaque datatype              */
        H5T_array_t  array;  /* an array datatype                */
    } u;
} H5T_shared_t;

struct H5T_t {
    H5O_shared_t sh_loc; /* Shared message info (must be first) */

    H5T_shared_t  *shared;  /* all other information */
    H5O_loc_t      oloc;    /* Object location, if the type is a named type */
    H5G_name_t     path;    /* group hier. path if the type is a named type */
    H5VL_object_t *vol_obj; /* pointer to VOL object when working with committed datatypes */
};

/* Bit search direction */
typedef enum H5T_sdir_t {
    H5T_BIT_LSB, /*search lsb toward msb		     */
    H5T_BIT_MSB  /*search msb toward lsb		     */
} H5T_sdir_t;

/* Typedef for named datatype creation operation */
typedef struct {
    H5T_t *dt;      /* Datatype to commit */
    hid_t  tcpl_id; /* Named datatype creation property list */
} H5T_obj_create_t;

/* Typedef for datatype iteration operations */
typedef herr_t (*H5T_operator_t)(H5T_t *dt, void *op_data /*in,out*/);

/*  Array of versions for Datatype */
H5_DLLVAR const unsigned H5O_dtype_ver_bounds[H5F_LIBVER_NBOUNDS];

/*
 * Alignment constraints for HDF5 types.  Accessing objects of these
 * types with improper alignment invokes C undefined behavior, so the
 * library lays out objects with correct alignment, always.
 *
 * A value of N indicates that the data must be aligned on an address
 * ADDR such that 0 == ADDR mod N. When N=1 no alignment is required;
 * N=0 implies that alignment constraints were not calculated.  These
 * values are used for structure alignment.
 *
 * This alignment info is only for H5Tget_native_type.
 */
H5_DLLVAR size_t H5T_POINTER_ALIGN_g;
H5_DLLVAR size_t H5T_HVL_ALIGN_g;
H5_DLLVAR size_t H5T_HOBJREF_ALIGN_g;
H5_DLLVAR size_t H5T_HDSETREGREF_ALIGN_g;
H5_DLLVAR size_t H5T_REF_ALIGN_g;

/*
 * Alignment information for native types. A value of N indicates that the
 * data must be aligned on an address ADDR such that 0 == ADDR mod N. When
 * N=1 no alignment is required; N=0 implies that alignment constraints were
 * not calculated.
 */
H5_DLLVAR size_t H5T_NATIVE_SCHAR_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_UCHAR_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_SHORT_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_USHORT_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_INT_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_UINT_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_LONG_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_ULONG_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_LLONG_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_ULLONG_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_FLOAT16_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_FLOAT_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_DOUBLE_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_LDOUBLE_ALIGN_g;

/* C9x alignment constraints */
H5_DLLVAR size_t H5T_NATIVE_INT8_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_UINT8_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_INT_LEAST8_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_UINT_LEAST8_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_INT_FAST8_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_UINT_FAST8_ALIGN_g;

H5_DLLVAR size_t H5T_NATIVE_INT16_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_UINT16_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_INT_LEAST16_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_UINT_LEAST16_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_INT_FAST16_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_UINT_FAST16_ALIGN_g;

H5_DLLVAR size_t H5T_NATIVE_INT32_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_UINT32_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_INT_LEAST32_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_UINT_LEAST32_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_INT_FAST32_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_UINT_FAST32_ALIGN_g;

H5_DLLVAR size_t H5T_NATIVE_INT64_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_UINT64_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_INT_LEAST64_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_UINT_LEAST64_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_INT_FAST64_ALIGN_g;
H5_DLLVAR size_t H5T_NATIVE_UINT_FAST64_ALIGN_g;

/* Useful floating-point values for conversion routines */
/* (+/- Inf for all floating-point types) */
#ifdef H5_HAVE__FLOAT16
H5_DLLVAR H5__Float16 H5T_NATIVE_FLOAT16_POS_INF_g;
H5_DLLVAR H5__Float16 H5T_NATIVE_FLOAT16_NEG_INF_g;
#endif
H5_DLLVAR float  H5T_NATIVE_FLOAT_POS_INF_g;
H5_DLLVAR float  H5T_NATIVE_FLOAT_NEG_INF_g;
H5_DLLVAR double H5T_NATIVE_DOUBLE_POS_INF_g;
H5_DLLVAR double H5T_NATIVE_DOUBLE_NEG_INF_g;
H5_DLLVAR double H5T_NATIVE_LDOUBLE_POS_INF_g;
H5_DLLVAR double H5T_NATIVE_LDOUBLE_NEG_INF_g;

/* Declare extern the free lists for H5T_t's and H5T_shared_t's */
H5FL_EXTERN(H5T_t);
H5FL_EXTERN(H5T_shared_t);

/* Common functions */
H5_DLL herr_t H5T__init_native_float_types(void);
H5_DLL herr_t H5T__init_native_internal(void);
H5_DLL H5T_t *H5T__create(H5T_class_t type, size_t size);
H5_DLL H5T_t *H5T__alloc(void);
H5_DLL herr_t H5T__free(H5T_t *dt);
H5_DLL herr_t H5T__visit(H5T_t *dt, unsigned visit_flags, H5T_operator_t op, void *op_value);
H5_DLL herr_t H5T__upgrade_version(H5T_t *dt, unsigned new_version);

/* Committed / named datatype routines */
H5_DLL herr_t H5T__commit_anon(H5F_t *file, H5T_t *type, hid_t tcpl_id);
H5_DLL herr_t H5T__commit(H5F_t *file, H5T_t *type, hid_t tcpl_id);
H5_DLL herr_t H5T__commit_named(const H5G_loc_t *loc, const char *name, H5T_t *dt, hid_t lcpl_id,
                                hid_t tcpl_id);
H5_DLL H5T_t *H5T__open_name(const H5G_loc_t *loc, const char *name);
H5_DLL hid_t  H5T__get_create_plist(const H5T_t *type);

/* Bit twiddling functions */
H5_DLL void     H5T__bit_copy(uint8_t *dst, size_t dst_offset, const uint8_t *src, size_t src_offset,
                              size_t size);
H5_DLL herr_t   H5T__bit_shift(uint8_t *buf, ssize_t shift_dist, size_t offset, size_t size);
H5_DLL void     H5T__bit_set(uint8_t *buf, size_t offset, size_t size, bool value);
H5_DLL uint64_t H5T__bit_get_d(uint8_t *buf, size_t offset, size_t size);
H5_DLL void     H5T__bit_set_d(uint8_t *buf, size_t offset, size_t size, uint64_t val);
H5_DLL ssize_t  H5T__bit_find(const uint8_t *buf, size_t offset, size_t size, H5T_sdir_t direction,
                              bool value);
H5_DLL bool     H5T__bit_inc(uint8_t *buf, size_t start, size_t size);
H5_DLL bool     H5T__bit_dec(uint8_t *buf, size_t start, size_t size);
H5_DLL void     H5T__bit_neg(uint8_t *buf, size_t start, size_t size);

/* VL functions */
H5_DLL H5T_t *H5T__vlen_create(const H5T_t *base);
H5_DLL herr_t H5T__vlen_reclaim(void *elem, const H5T_t *dt, H5T_vlen_alloc_info_t *alloc_info);
H5_DLL htri_t H5T__vlen_set_loc(H5T_t *dt, H5VL_object_t *file, H5T_loc_t loc);

/* Array functions */
H5_DLL H5T_t *H5T__array_create(H5T_t *base, unsigned ndims, const hsize_t dim[/* ndims */]);
H5_DLL int    H5T__get_array_ndims(const H5T_t *dt);
H5_DLL int    H5T__get_array_dims(const H5T_t *dt, hsize_t dims[]);

/* Reference functions */
H5_DLL herr_t H5T__ref_reclaim(void *elem, const H5T_t *dt);
H5_DLL htri_t H5T__ref_set_loc(H5T_t *dt, H5VL_object_t *file, H5T_loc_t loc);

/* Compound functions */
H5_DLL herr_t H5T__insert(H5T_t *parent, const char *name, size_t offset, const H5T_t *member);
H5_DLL size_t H5T__get_member_size(const H5T_t *dt, unsigned membno);
H5_DLL void   H5T__update_packed(const H5T_t *dt);

/* Enumerated type functions */
H5_DLL H5T_t *H5T__enum_create(const H5T_t *parent);
H5_DLL herr_t H5T__enum_insert(const H5T_t *dt, const char *name, const void *value);
H5_DLL herr_t H5T__get_member_value(const H5T_t *dt, unsigned membno, void *value);

/* Field functions (for both compound & enumerated types) */
H5_DLL char  *H5T__get_member_name(H5T_t const *dt, unsigned membno) H5_ATTR_MALLOC;
H5_DLL herr_t H5T__sort_value(const H5T_t *dt, int *map);
H5_DLL herr_t H5T__sort_name(const H5T_t *dt, int *map);

#endif /* H5Tpkg_H */
