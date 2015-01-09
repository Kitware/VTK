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
 * Module Info: This module contains most of the "core" functionality of
 *      the H5T interface, including the API initialization code, etc.
 *      Many routines that are infrequently used, or are specialized for
 *      one particular datatype class are in another module.
 */

/****************/
/* Module Setup */
/****************/

#define H5T_PACKAGE		/*suppress error about including H5Tpkg	  */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5T_init_interface


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5ACprivate.h"        /* Metadata cache                       */
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* Files				*/
#include "H5FLprivate.h"	/* Free Lists				*/
#include "H5FOprivate.h"	/* File objects				*/
#include "H5Gprivate.h"		/* Groups				*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Pprivate.h"         /* Property lists                       */
#include "H5Tpkg.h"		/* Datatypes 				*/

/* Check for header needed for SGI floating-point code */
#ifdef H5_HAVE_SYS_FPU_H
#include <sys/fpu.h>
#endif /* H5_HAVE_SYS_FPU_H */


/****************/
/* Local Macros */
/****************/

#define H5T_ENCODE_VERSION      0

/*
 * Type initialization macros
 *
 * These use the "template macro" technique to reduce the amount of gratuitous
 * duplicated code when initializing the datatypes for the library.  The main
 * template macro is the H5T_INIT_TYPE() macro below.
 *
 */

/* Define the code template for types which need no extra initialization for the "GUTS" in the H5T_INIT_TYPE macro */
#define H5T_INIT_TYPE_NONE_CORE {					      \
}

/* Define the code template for bitfields for the "GUTS" in the H5T_INIT_TYPE macro */
#define H5T_INIT_TYPE_BITFIELD_CORE {					      \
    dt->shared->type = H5T_BITFIELD;					      \
}

/* Define the code template for times for the "GUTS" in the H5T_INIT_TYPE macro */
#define H5T_INIT_TYPE_TIME_CORE {					      \
    dt->shared->type = H5T_TIME;					      \
}

/* Define the code template for types which reset the offset for the "GUTS" in the H5T_INIT_TYPE macro */
#define H5T_INIT_TYPE_OFFSET_CORE {					      \
    dt->shared->u.atomic.offset = 0;					      \
}

/* Define common code for all numeric types (floating-point & int, signed & unsigned) */
#define H5T_INIT_TYPE_NUM_COMMON(ENDIANNESS) {				      \
    dt->shared->u.atomic.order = ENDIANNESS;				      \
    dt->shared->u.atomic.offset = 0;					      \
    dt->shared->u.atomic.lsb_pad = H5T_PAD_ZERO;			      \
    dt->shared->u.atomic.msb_pad = H5T_PAD_ZERO;			      \
}

/* Define the code templates for standard floats for the "GUTS" in the H5T_INIT_TYPE macro */
#define H5T_INIT_TYPE_FLOAT_COMMON(ENDIANNESS) {			      \
    H5T_INIT_TYPE_NUM_COMMON(ENDIANNESS)				      \
    dt->shared->u.atomic.u.f.sign = 31;					      \
    dt->shared->u.atomic.u.f.epos = 23;					      \
    dt->shared->u.atomic.u.f.esize = 8;					      \
    dt->shared->u.atomic.u.f.ebias = 0x7f;				      \
    dt->shared->u.atomic.u.f.mpos = 0;					      \
    dt->shared->u.atomic.u.f.msize = 23;				      \
    dt->shared->u.atomic.u.f.norm = H5T_NORM_IMPLIED;			      \
    dt->shared->u.atomic.u.f.pad = H5T_PAD_ZERO;			      \
}

#define H5T_INIT_TYPE_FLOATLE_CORE {					      \
    H5T_INIT_TYPE_FLOAT_COMMON(H5T_ORDER_LE)				      \
}

#define H5T_INIT_TYPE_FLOATBE_CORE {					      \
    H5T_INIT_TYPE_FLOAT_COMMON(H5T_ORDER_BE)				      \
}

/* Define the code templates for standard doubles for the "GUTS" in the H5T_INIT_TYPE macro */
#define H5T_INIT_TYPE_DOUBLE_COMMON(ENDIANNESS) {			      \
    H5T_INIT_TYPE_NUM_COMMON(ENDIANNESS)				      \
    dt->shared->u.atomic.u.f.sign = 63;					      \
    dt->shared->u.atomic.u.f.epos = 52;					      \
    dt->shared->u.atomic.u.f.esize = 11;				      \
    dt->shared->u.atomic.u.f.ebias = 0x03ff;				      \
    dt->shared->u.atomic.u.f.mpos = 0;					      \
    dt->shared->u.atomic.u.f.msize = 52;				      \
    dt->shared->u.atomic.u.f.norm = H5T_NORM_IMPLIED;			      \
    dt->shared->u.atomic.u.f.pad = H5T_PAD_ZERO;			      \
}

#define H5T_INIT_TYPE_DOUBLELE_CORE {					      \
    H5T_INIT_TYPE_DOUBLE_COMMON(H5T_ORDER_LE)				      \
}

#define H5T_INIT_TYPE_DOUBLEBE_CORE {					      \
    H5T_INIT_TYPE_DOUBLE_COMMON(H5T_ORDER_BE)				      \
}

/* Define the code templates for VAX float for the "GUTS" in the H5T_INIT_TYPE macro */
#define H5T_INIT_TYPE_FLOATVAX_CORE {			                      \
    H5T_INIT_TYPE_NUM_COMMON(H5T_ORDER_VAX)				      \
    dt->shared->u.atomic.u.f.sign = 31;					      \
    dt->shared->u.atomic.u.f.epos = 23;					      \
    dt->shared->u.atomic.u.f.esize = 8;					      \
    dt->shared->u.atomic.u.f.ebias = 0x81;				      \
    dt->shared->u.atomic.u.f.mpos = 0;					      \
    dt->shared->u.atomic.u.f.msize = 23;				      \
    dt->shared->u.atomic.u.f.norm = H5T_NORM_IMPLIED;			      \
    dt->shared->u.atomic.u.f.pad = H5T_PAD_ZERO;			      \
    dt->shared->version = H5O_DTYPE_VERSION_3;				      \
}

/* Define the code templates for VAX double for the "GUTS" in the H5T_INIT_TYPE macro */
#define H5T_INIT_TYPE_DOUBLEVAX_CORE {                                        \
    H5T_INIT_TYPE_NUM_COMMON(H5T_ORDER_VAX)				      \
    dt->shared->u.atomic.u.f.sign = 63;					      \
    dt->shared->u.atomic.u.f.epos = 52;					      \
    dt->shared->u.atomic.u.f.esize = 11;				      \
    dt->shared->u.atomic.u.f.ebias = 0x0401;				      \
    dt->shared->u.atomic.u.f.mpos = 0;					      \
    dt->shared->u.atomic.u.f.msize = 52;				      \
    dt->shared->u.atomic.u.f.norm = H5T_NORM_IMPLIED;			      \
    dt->shared->u.atomic.u.f.pad = H5T_PAD_ZERO;			      \
    dt->shared->version = H5O_DTYPE_VERSION_3;				      \
}

/* Define the code templates for standard signed integers for the "GUTS" in the H5T_INIT_TYPE macro */
#define H5T_INIT_TYPE_SINT_COMMON(ENDIANNESS) {				      \
    H5T_INIT_TYPE_NUM_COMMON(ENDIANNESS)				      \
    dt->shared->u.atomic.u.i.sign = H5T_SGN_2;				      \
}

#define H5T_INIT_TYPE_SINTLE_CORE {					      \
    H5T_INIT_TYPE_SINT_COMMON(H5T_ORDER_LE)				      \
}

#define H5T_INIT_TYPE_SINTBE_CORE {					      \
    H5T_INIT_TYPE_SINT_COMMON(H5T_ORDER_BE)				      \
}

/* Define the code templates for standard unsigned integers for the "GUTS" in the H5T_INIT_TYPE macro */
#define H5T_INIT_TYPE_UINT_COMMON(ENDIANNESS) {				      \
    H5T_INIT_TYPE_NUM_COMMON(ENDIANNESS)				      \
    dt->shared->u.atomic.u.i.sign = H5T_SGN_NONE;				      \
}

#define H5T_INIT_TYPE_UINTLE_CORE {					      \
    H5T_INIT_TYPE_UINT_COMMON(H5T_ORDER_LE)				      \
}

#define H5T_INIT_TYPE_UINTBE_CORE {					      \
    H5T_INIT_TYPE_UINT_COMMON(H5T_ORDER_BE)				      \
}

/* Define a macro for common code for all newly allocate datatypes */
#define H5T_INIT_TYPE_ALLOC_COMMON(TYPE) {				      \
    dt->sh_loc.type = H5O_SHARE_TYPE_UNSHARED;				      \
    dt->shared->type = TYPE;						      \
}

/* Define the code templates for opaque for the "GUTS" in the H5T_INIT_TYPE macro */
#define H5T_INIT_TYPE_OPAQ_CORE {					      \
    H5T_INIT_TYPE_ALLOC_COMMON(H5T_OPAQUE)				      \
    dt->shared->u.opaque.tag = H5MM_xstrdup("");			      \
}

/* Define the code templates for strings for the "GUTS" in the H5T_INIT_TYPE macro */
#define H5T_INIT_TYPE_STRING_COMMON {					      \
    H5T_INIT_TYPE_ALLOC_COMMON(H5T_STRING)				      \
    H5T_INIT_TYPE_NUM_COMMON(H5T_ORDER_NONE)				      \
    dt->shared->u.atomic.u.s.cset = H5F_DEFAULT_CSET;			      \
}

#define H5T_INIT_TYPE_CSTRING_CORE {					      \
    H5T_INIT_TYPE_STRING_COMMON						      \
    dt->shared->u.atomic.u.s.pad = H5T_STR_NULLTERM;			      \
}

#define H5T_INIT_TYPE_FORSTRING_CORE {					      \
    H5T_INIT_TYPE_STRING_COMMON						      \
    dt->shared->u.atomic.u.s.pad = H5T_STR_SPACEPAD;			      \
}

/* Define the code templates for references for the "GUTS" in the H5T_INIT_TYPE macro */
#define H5T_INIT_TYPE_REF_COMMON {					      \
    H5T_INIT_TYPE_ALLOC_COMMON(H5T_REFERENCE)				      \
    H5T_INIT_TYPE_NUM_COMMON(H5T_ORDER_NONE)				      \
}

#define H5T_INIT_TYPE_OBJREF_CORE {					      \
    H5T_INIT_TYPE_REF_COMMON						      \
    dt->shared->force_conv = TRUE;					      \
    dt->shared->u.atomic.u.r.rtype = H5R_OBJECT;			      \
    dt->shared->u.atomic.u.r.loc = H5T_LOC_MEMORY;			      \
}

#define H5T_INIT_TYPE_REGREF_CORE {					      \
    H5T_INIT_TYPE_REF_COMMON						      \
    dt->shared->u.atomic.u.r.rtype = H5R_DATASET_REGION;		      \
}

/* Define the code templates for the "SIZE_TMPL" in the H5T_INIT_TYPE macro */
#define H5T_INIT_TYPE_SET_SIZE(SIZE) {					      \
    dt->shared->size = SIZE;						      \
    dt->shared->u.atomic.prec = 8 * SIZE;				      \
}

#define H5T_INIT_TYPE_NOSET_SIZE(SIZE) {				      \
}

/* Define the code templates for the "CRT_TMPL" in the H5T_INIT_TYPE macro */
#define H5T_INIT_TYPE_COPY_CREATE(BASE) {				      \
    /* Base off of existing datatype */					      \
    if(NULL == (dt = H5T_copy(BASE, H5T_COPY_TRANSIENT)))		      \
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, FAIL, "duplicating base type failed") \
}

#define H5T_INIT_TYPE_ALLOC_CREATE(BASE) {				      \
    /* Allocate new datatype info */					      \
    if(NULL == (dt = H5T__alloc()))				              \
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTALLOC, FAIL, "memory allocation failed") \
}


#define H5T_INIT_TYPE(GUTS,GLOBAL,CRT_TMPL,BASE,SIZE_TMPL,SIZE) {	      \
    /* Get new datatype struct */					      \
    H5_GLUE3(H5T_INIT_TYPE_,CRT_TMPL,_CREATE)(BASE)			      \
									      \
    /* Adjust information for all types */				      \
    dt->shared->state = H5T_STATE_IMMUTABLE;				      \
    H5_GLUE3(H5T_INIT_TYPE_,SIZE_TMPL,_SIZE)(SIZE)			      \
									      \
    /* Adjust information for this type */				      \
    H5_GLUE3(H5T_INIT_TYPE_, GUTS, _CORE)				      \
									      \
    /* Atomize result */						      \
    if((GLOBAL = H5I_register(H5I_DATATYPE, dt, FALSE)) < 0)			      \
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register datatype atom") \
}


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Local Prototypes */
/********************/
static herr_t H5T_unregister(H5T_pers_t pers, const char *name, H5T_t *src,
                H5T_t *dst, H5T_conv_t func, hid_t dxpl_id);
static herr_t H5T_register(H5T_pers_t pers, const char *name, H5T_t *src,
        H5T_t *dst, H5T_conv_t func, hid_t dxpl_id, hbool_t api_call);
static htri_t H5T_compiler_conv(H5T_t *src, H5T_t *dst);
static herr_t H5T_encode(H5T_t *obj, unsigned char *buf, size_t *nalloc);
static H5T_t *H5T_decode(const unsigned char *buf);
static herr_t H5T_set_size(H5T_t *dt, size_t size);


/*****************************/
/* Library Private Variables */
/*****************************/

/* The native endianess of the platform */
H5T_order_t H5T_native_order_g = H5T_ORDER_ERROR;


/*********************/
/* Package Variables */
/*********************/

/*
 * Predefined data types. These are initialized at runtime in H5Tinit.c and
 * by H5T_init_interface() in this source file.
 *
 * If more of these are added, the new ones must be added to the list of
 * types to reset in H5T_term_interface().
 */
hid_t H5T_IEEE_F32BE_g			= FAIL;
hid_t H5T_IEEE_F32LE_g			= FAIL;
hid_t H5T_IEEE_F64BE_g			= FAIL;
hid_t H5T_IEEE_F64LE_g			= FAIL;

hid_t H5T_VAX_F32_g			= FAIL;
hid_t H5T_VAX_F64_g			= FAIL;

hid_t H5T_STD_I8BE_g			= FAIL;
hid_t H5T_STD_I8LE_g			= FAIL;
hid_t H5T_STD_I16BE_g			= FAIL;
hid_t H5T_STD_I16LE_g			= FAIL;
hid_t H5T_STD_I32BE_g			= FAIL;
hid_t H5T_STD_I32LE_g			= FAIL;
hid_t H5T_STD_I64BE_g			= FAIL;
hid_t H5T_STD_I64LE_g			= FAIL;
hid_t H5T_STD_U8BE_g			= FAIL;
hid_t H5T_STD_U8LE_g			= FAIL;
hid_t H5T_STD_U16BE_g			= FAIL;
hid_t H5T_STD_U16LE_g			= FAIL;
hid_t H5T_STD_U32BE_g			= FAIL;
hid_t H5T_STD_U32LE_g			= FAIL;
hid_t H5T_STD_U64BE_g			= FAIL;
hid_t H5T_STD_U64LE_g			= FAIL;
hid_t H5T_STD_B8BE_g			= FAIL;
hid_t H5T_STD_B8LE_g			= FAIL;
hid_t H5T_STD_B16BE_g			= FAIL;
hid_t H5T_STD_B16LE_g			= FAIL;
hid_t H5T_STD_B32BE_g			= FAIL;
hid_t H5T_STD_B32LE_g			= FAIL;
hid_t H5T_STD_B64BE_g			= FAIL;
hid_t H5T_STD_B64LE_g 			= FAIL;
hid_t H5T_STD_REF_OBJ_g 		= FAIL;
hid_t H5T_STD_REF_DSETREG_g 		= FAIL;

hid_t H5T_UNIX_D32BE_g			= FAIL;
hid_t H5T_UNIX_D32LE_g			= FAIL;
hid_t H5T_UNIX_D64BE_g			= FAIL;
hid_t H5T_UNIX_D64LE_g 			= FAIL;

hid_t H5T_C_S1_g			= FAIL;

hid_t H5T_FORTRAN_S1_g			= FAIL;

hid_t H5T_NATIVE_SCHAR_g		= FAIL;
hid_t H5T_NATIVE_UCHAR_g		= FAIL;
hid_t H5T_NATIVE_SHORT_g		= FAIL;
hid_t H5T_NATIVE_USHORT_g		= FAIL;
hid_t H5T_NATIVE_INT_g			= FAIL;
hid_t H5T_NATIVE_UINT_g			= FAIL;
hid_t H5T_NATIVE_LONG_g			= FAIL;
hid_t H5T_NATIVE_ULONG_g		= FAIL;
hid_t H5T_NATIVE_LLONG_g		= FAIL;
hid_t H5T_NATIVE_ULLONG_g		= FAIL;
hid_t H5T_NATIVE_FLOAT_g		= FAIL;
hid_t H5T_NATIVE_DOUBLE_g		= FAIL;
#if H5_SIZEOF_LONG_DOUBLE !=0
hid_t H5T_NATIVE_LDOUBLE_g		= FAIL;
#endif
hid_t H5T_NATIVE_B8_g			= FAIL;
hid_t H5T_NATIVE_B16_g			= FAIL;
hid_t H5T_NATIVE_B32_g			= FAIL;
hid_t H5T_NATIVE_B64_g			= FAIL;
hid_t H5T_NATIVE_OPAQUE_g		= FAIL;
hid_t H5T_NATIVE_HADDR_g		= FAIL;
hid_t H5T_NATIVE_HSIZE_g		= FAIL;
hid_t H5T_NATIVE_HSSIZE_g		= FAIL;
hid_t H5T_NATIVE_HERR_g			= FAIL;
hid_t H5T_NATIVE_HBOOL_g		= FAIL;

hid_t H5T_NATIVE_INT8_g			= FAIL;
hid_t H5T_NATIVE_UINT8_g		= FAIL;
hid_t H5T_NATIVE_INT_LEAST8_g		= FAIL;
hid_t H5T_NATIVE_UINT_LEAST8_g		= FAIL;
hid_t H5T_NATIVE_INT_FAST8_g		= FAIL;
hid_t H5T_NATIVE_UINT_FAST8_g		= FAIL;

hid_t H5T_NATIVE_INT16_g		= FAIL;
hid_t H5T_NATIVE_UINT16_g		= FAIL;
hid_t H5T_NATIVE_INT_LEAST16_g		= FAIL;
hid_t H5T_NATIVE_UINT_LEAST16_g		= FAIL;
hid_t H5T_NATIVE_INT_FAST16_g		= FAIL;
hid_t H5T_NATIVE_UINT_FAST16_g		= FAIL;

hid_t H5T_NATIVE_INT32_g		= FAIL;
hid_t H5T_NATIVE_UINT32_g		= FAIL;
hid_t H5T_NATIVE_INT_LEAST32_g		= FAIL;
hid_t H5T_NATIVE_UINT_LEAST32_g		= FAIL;
hid_t H5T_NATIVE_INT_FAST32_g		= FAIL;
hid_t H5T_NATIVE_UINT_FAST32_g		= FAIL;

hid_t H5T_NATIVE_INT64_g		= FAIL;
hid_t H5T_NATIVE_UINT64_g		= FAIL;
hid_t H5T_NATIVE_INT_LEAST64_g		= FAIL;
hid_t H5T_NATIVE_UINT_LEAST64_g		= FAIL;
hid_t H5T_NATIVE_INT_FAST64_g		= FAIL;
hid_t H5T_NATIVE_UINT_FAST64_g		= FAIL;

/*
 * Alignment constraints for native types. These are initialized at run time
 * in H5Tinit.c.  These alignments are mainly for offsets in HDF5 compound
 * datatype or C structures, which are different from the alignments for memory
 * address below this group of variables.
 */
size_t H5T_NATIVE_SCHAR_COMP_ALIGN_g		= 0;
size_t H5T_NATIVE_UCHAR_COMP_ALIGN_g		= 0;
size_t H5T_NATIVE_SHORT_COMP_ALIGN_g		= 0;
size_t H5T_NATIVE_USHORT_COMP_ALIGN_g   	= 0;
size_t H5T_NATIVE_INT_COMP_ALIGN_g		= 0;
size_t H5T_NATIVE_UINT_COMP_ALIGN_g		= 0;
size_t H5T_NATIVE_LONG_COMP_ALIGN_g		= 0;
size_t H5T_NATIVE_ULONG_COMP_ALIGN_g		= 0;
size_t H5T_NATIVE_LLONG_COMP_ALIGN_g		= 0;
size_t H5T_NATIVE_ULLONG_COMP_ALIGN_g	        = 0;
size_t H5T_NATIVE_FLOAT_COMP_ALIGN_g		= 0;
size_t H5T_NATIVE_DOUBLE_COMP_ALIGN_g	        = 0;
#if H5_SIZEOF_LONG_DOUBLE !=0
size_t H5T_NATIVE_LDOUBLE_COMP_ALIGN_g	        = 0;
#endif

size_t H5T_POINTER_COMP_ALIGN_g	                = 0;
size_t H5T_HVL_COMP_ALIGN_g	                = 0;
size_t H5T_HOBJREF_COMP_ALIGN_g	                = 0;
size_t H5T_HDSETREGREF_COMP_ALIGN_g	        = 0;

/*
 * Alignment constraints for native types. These are initialized at run time
 * in H5Tinit.c
 */
size_t H5T_NATIVE_SCHAR_ALIGN_g		= 0;
size_t H5T_NATIVE_UCHAR_ALIGN_g		= 0;
size_t H5T_NATIVE_SHORT_ALIGN_g		= 0;
size_t H5T_NATIVE_USHORT_ALIGN_g	= 0;
size_t H5T_NATIVE_INT_ALIGN_g		= 0;
size_t H5T_NATIVE_UINT_ALIGN_g		= 0;
size_t H5T_NATIVE_LONG_ALIGN_g		= 0;
size_t H5T_NATIVE_ULONG_ALIGN_g		= 0;
size_t H5T_NATIVE_LLONG_ALIGN_g		= 0;
size_t H5T_NATIVE_ULLONG_ALIGN_g	= 0;
size_t H5T_NATIVE_FLOAT_ALIGN_g		= 0;
size_t H5T_NATIVE_DOUBLE_ALIGN_g	= 0;
#if H5_SIZEOF_LONG_DOUBLE !=0
size_t H5T_NATIVE_LDOUBLE_ALIGN_g	= 0;
#endif

/*
 * Alignment constraints for C9x types. These are initialized at run time in
 * H5Tinit.c if the types are provided by the system. Otherwise we set their
 * values to 0 here (no alignment calculated).
 */
size_t H5T_NATIVE_INT8_ALIGN_g		= 0;
size_t H5T_NATIVE_UINT8_ALIGN_g		= 0;
size_t H5T_NATIVE_INT_LEAST8_ALIGN_g	= 0;
size_t H5T_NATIVE_UINT_LEAST8_ALIGN_g	= 0;
size_t H5T_NATIVE_INT_FAST8_ALIGN_g	= 0;
size_t H5T_NATIVE_UINT_FAST8_ALIGN_g	= 0;

size_t H5T_NATIVE_INT16_ALIGN_g		= 0;
size_t H5T_NATIVE_UINT16_ALIGN_g	= 0;
size_t H5T_NATIVE_INT_LEAST16_ALIGN_g	= 0;
size_t H5T_NATIVE_UINT_LEAST16_ALIGN_g	= 0;
size_t H5T_NATIVE_INT_FAST16_ALIGN_g	= 0;
size_t H5T_NATIVE_UINT_FAST16_ALIGN_g	= 0;

size_t H5T_NATIVE_INT32_ALIGN_g		= 0;
size_t H5T_NATIVE_UINT32_ALIGN_g	= 0;
size_t H5T_NATIVE_INT_LEAST32_ALIGN_g	= 0;
size_t H5T_NATIVE_UINT_LEAST32_ALIGN_g	= 0;
size_t H5T_NATIVE_INT_FAST32_ALIGN_g	= 0;
size_t H5T_NATIVE_UINT_FAST32_ALIGN_g	= 0;

size_t H5T_NATIVE_INT64_ALIGN_g		= 0;
size_t H5T_NATIVE_UINT64_ALIGN_g	= 0;
size_t H5T_NATIVE_INT_LEAST64_ALIGN_g	= 0;
size_t H5T_NATIVE_UINT_LEAST64_ALIGN_g	= 0;
size_t H5T_NATIVE_INT_FAST64_ALIGN_g	= 0;
size_t H5T_NATIVE_UINT_FAST64_ALIGN_g	= 0;

/* Useful floating-point values for conversion routines */
/* (+/- Inf for all floating-point types) */
float H5T_NATIVE_FLOAT_POS_INF_g        = 0.0f;
float H5T_NATIVE_FLOAT_NEG_INF_g        = 0.0f;
double H5T_NATIVE_DOUBLE_POS_INF_g      = (double)0.0f;
double H5T_NATIVE_DOUBLE_NEG_INF_g      = (double)0.0f;

/* Declare the free list for H5T_t's and H5T_shared_t's */
H5FL_DEFINE(H5T_t);
H5FL_DEFINE(H5T_shared_t);


/*******************/
/* Local Variables */
/*******************/

/*
 * The path database. Each path has a source and destination data type pair
 * which is used as the key by which the `entries' array is sorted.
 */
static struct {
    int	npaths;		/*number of paths defined		*/
    size_t	apaths;		/*number of paths allocated		*/
    H5T_path_t	**path;		/*sorted array of path pointers		*/
    int	nsoft;		/*number of soft conversions defined	*/
    size_t	asoft;		/*number of soft conversions allocated	*/
    H5T_soft_t	*soft;		/*unsorted array of soft conversions	*/
} H5T_g;

/* Declare the free list for H5T_path_t's */
H5FL_DEFINE_STATIC(H5T_path_t);

/* Datatype ID class */
static const H5I_class_t H5I_DATATYPE_CLS[1] = {{
    H5I_DATATYPE,		/* ID class value */
    0,				/* Class flags */
    8,				/* # of reserved IDs for class */
    (H5I_free_t)H5T_close	/* Callback routine for closing objects of this class */
}};



/*-------------------------------------------------------------------------
 * Function:	H5T_init
 *
 * Purpose:	Initialize the interface from some other package.
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, December 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_init(void)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(FAIL)
    /* FUNC_ENTER() does all the work */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5T_init_inf
 *
 * Purpose:	Initialize the +/- Infinity floating-poing values for type
 *              conversion.
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Saturday, November 22, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T_init_inf(void)
{
    H5T_t	*dst_p;		/* Datatype type operate on */
    H5T_atomic_t *dst;		/* Datatype's atomic info */
    uint8_t	*d;             /* Pointer to value to set */
    size_t	half_size;	/* Half the type size */
    size_t      u;              /* Local index value */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Get the float datatype */
    if(NULL == (dst_p = (H5T_t *)H5I_object(H5T_NATIVE_FLOAT_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    dst = &dst_p->shared->u.atomic;

    /* Check that we can re-order the bytes correctly */
    if(H5T_ORDER_LE != H5T_native_order_g && H5T_ORDER_BE != H5T_native_order_g)
        HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unsupported byte order")

    /* +Inf */
    d = (uint8_t *)&H5T_NATIVE_FLOAT_POS_INF_g;
    H5T__bit_set(d, dst->u.f.sign, (size_t)1, FALSE);
    H5T__bit_set(d, dst->u.f.epos, dst->u.f.esize, TRUE);
    H5T__bit_set(d, dst->u.f.mpos, dst->u.f.msize, FALSE);

    /* Swap the bytes if the machine architecture is big-endian */
    if (H5T_ORDER_BE == H5T_native_order_g) {
        half_size = dst_p->shared->size / 2;
        for(u = 0; u < half_size; u++) {
            uint8_t tmp = d[dst_p->shared->size - (u + 1)];
            d[dst_p->shared->size - (u + 1)] = d[u];
            d[u] = tmp;
        } /* end for */
    } /* end if */

    /* -Inf */
    d = (uint8_t *)&H5T_NATIVE_FLOAT_NEG_INF_g;
    H5T__bit_set(d, dst->u.f.sign, (size_t)1, TRUE);
    H5T__bit_set(d, dst->u.f.epos, dst->u.f.esize, TRUE);
    H5T__bit_set(d, dst->u.f.mpos, dst->u.f.msize, FALSE);

    /* Swap the bytes if the machine architecture is big-endian */
    if(H5T_ORDER_BE == H5T_native_order_g) {
        half_size = dst_p->shared->size / 2;
        for(u = 0; u < half_size; u++) {
            uint8_t tmp = d[dst_p->shared->size - (u + 1)];
            d[dst_p->shared->size - (u + 1)] = d[u];
            d[u] = tmp;
        } /* end for */
    } /* end if */

    /* Get the double datatype */
    if(NULL == (dst_p = (H5T_t *)H5I_object(H5T_NATIVE_DOUBLE_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    dst = &dst_p->shared->u.atomic;

    /* Check that we can re-order the bytes correctly */
    if(H5T_ORDER_LE != H5T_native_order_g && H5T_ORDER_BE != H5T_native_order_g)
        HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unsupported byte order")

    /* +Inf */
    d = (uint8_t *)&H5T_NATIVE_DOUBLE_POS_INF_g;
    H5T__bit_set(d, dst->u.f.sign, (size_t)1, FALSE);
    H5T__bit_set(d, dst->u.f.epos, dst->u.f.esize, TRUE);
    H5T__bit_set(d, dst->u.f.mpos, dst->u.f.msize, FALSE);

    /* Swap the bytes if the machine architecture is big-endian */
    if(H5T_ORDER_BE == H5T_native_order_g) {
        half_size = dst_p->shared->size / 2;
        for(u = 0; u < half_size; u++) {
            uint8_t tmp = d[dst_p->shared->size - (u + 1)];
            d[dst_p->shared->size - (u + 1)] = d[u];
            d[u] = tmp;
        } /* end for */
    } /* end if */

    /* -Inf */
    d = (uint8_t *)&H5T_NATIVE_DOUBLE_NEG_INF_g;
    H5T__bit_set(d, dst->u.f.sign, (size_t)1, TRUE);
    H5T__bit_set(d, dst->u.f.epos, dst->u.f.esize, TRUE);
    H5T__bit_set(d, dst->u.f.mpos, dst->u.f.msize, FALSE);

    /* Swap the bytes if the machine architecture is big-endian */
    if(H5T_ORDER_BE == H5T_native_order_g) {
        half_size = dst_p->shared->size / 2;
        for(u = 0; u < half_size; u++) {
            uint8_t tmp = d[dst_p->shared->size - (u + 1)];
            d[dst_p->shared->size - (u + 1)] = d[u];
            d[u] = tmp;
        } /* end for */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_init_inf() */


/*-------------------------------------------------------------------------
 * Function:	H5T_init_hw
 *
 * Purpose:	Perform hardware specific [floating-point] initialization
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, November 24, 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T_init_hw(void)
{
#ifdef H5_HAVE_GET_FPC_CSR
    union fpc_csr csr;          /* Union to hold results of floating-point status register query */
#endif /* H5_HAVE_GET_FPC_CSR */
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

#ifdef H5_HAVE_GET_FPC_CSR
    /* [This code is specific to SGI machines] */

    /* Get the floating-point status register */
    csr.fc_word = get_fpc_csr();

    /* If the "flush denormalized values to zero" flag is set, unset it */
    if(csr.fc_struct.flush) {
        csr.fc_struct.flush = 0;
        set_fpc_csr(csr.fc_word);
    } /* end if */
#endif /* H5_HAVE_GET_FPC_CSR */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_init_hw() */


/*--------------------------------------------------------------------------
NAME
   H5T_init_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.

--------------------------------------------------------------------------*/
static herr_t
H5T_init_interface(void)
{
    H5T_t       *native_schar=NULL;     /* Datatype structure for native signed char */
    H5T_t       *native_uchar=NULL;     /* Datatype structure for native unsigned char */
    H5T_t       *native_short=NULL;     /* Datatype structure for native short */
    H5T_t       *native_ushort=NULL;    /* Datatype structure for native unsigned short */
    H5T_t       *native_int=NULL;       /* Datatype structure for native int */
    H5T_t       *native_uint=NULL;      /* Datatype structure for native unsigned int */
    H5T_t       *native_long=NULL;      /* Datatype structure for native long */
    H5T_t       *native_ulong=NULL;     /* Datatype structure for native unsigned long */
    H5T_t       *native_llong=NULL;     /* Datatype structure for native long long */
    H5T_t       *native_ullong=NULL;    /* Datatype structure for native unsigned long long */
    H5T_t       *native_float=NULL;     /* Datatype structure for native float */
    H5T_t       *native_double=NULL;    /* Datatype structure for native double */
#if H5_SIZEOF_LONG_DOUBLE !=0
    H5T_t       *native_ldouble=NULL;   /* Datatype structure for native long double */
#endif
    H5T_t       *std_u8le=NULL;         /* Datatype structure for unsigned 8-bit little-endian integer */
    H5T_t       *std_u8be=NULL;         /* Datatype structure for unsigned 8-bit big-endian integer */
    H5T_t       *std_u16le=NULL;        /* Datatype structure for unsigned 16-bit little-endian integer */
    H5T_t       *std_u16be=NULL;        /* Datatype structure for unsigned 16-bit big-endian integer */
    H5T_t       *std_u32le=NULL;        /* Datatype structure for unsigned 32-bit little-endian integer */
    H5T_t       *std_u32be=NULL;        /* Datatype structure for unsigned 32-bit big-endian integer */
    H5T_t       *std_u64le=NULL;        /* Datatype structure for unsigned 64-bit little-endian integer */
    H5T_t       *std_u64be=NULL;        /* Datatype structure for unsigned 64-bit big-endian integer */
    H5T_t	*dt = NULL;
    H5T_t	*fixedpt=NULL;          /* Datatype structure for native int */
    H5T_t	*floatpt=NULL;          /* Datatype structure for native float */
    H5T_t	*string=NULL;           /* Datatype structure for C string */
    H5T_t	*bitfield=NULL;         /* Datatype structure for bitfield */
    H5T_t	*compound=NULL;         /* Datatype structure for compound objects */
    H5T_t	*enum_type=NULL;        /* Datatype structure for enum objects */
    H5T_t	*vlen=NULL;             /* Datatype structure for vlen objects */
    H5T_t	*array=NULL;            /* Datatype structure for array objects */
    H5T_t	*objref=NULL;           /* Datatype structure for object reference objects */
    hsize_t     dim[1]={1};             /* Dimension info for array datatype */
    herr_t	status;
    unsigned    copied_dtype=1;         /* Flag to indicate whether datatype was copied or allocated (for error cleanup) */
    H5P_genclass_t  *crt_pclass;        /* Property list class for datatype creation properties */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Initialize the atom group for the file IDs */
    if(H5I_register_type(H5I_DATATYPE_CLS) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to initialize interface")

    /* Make certain there aren't too many classes of datatypes defined */
    /* Only 16 (numbered 0-15) are supported in the current file format */
    HDassert(H5T_NCLASSES < 16);

    /* Perform any necessary hardware initializations */
    if(H5T_init_hw() < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to initialize interface")

    /*
     * Initialize pre-defined native datatypes from code generated during
     * the library configuration by H5detect.
     */
    if(H5TN_init_interface() < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to initialize interface")

    /* Get the atomic datatype structures needed by the initialization code below */
    if(NULL == (native_schar = (H5T_t *)H5I_object(H5T_NATIVE_SCHAR_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object")
    if(NULL == (native_uchar = (H5T_t *)H5I_object(H5T_NATIVE_UCHAR_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object")
    if(NULL == (native_short = (H5T_t *)H5I_object(H5T_NATIVE_SHORT_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object")
    if(NULL == (native_ushort = (H5T_t *)H5I_object(H5T_NATIVE_USHORT_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object")
    if(NULL == (native_int = (H5T_t *)H5I_object(H5T_NATIVE_INT_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object")
    if(NULL == (native_uint = (H5T_t *)H5I_object(H5T_NATIVE_UINT_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object")
    if(NULL == (native_long = (H5T_t *)H5I_object(H5T_NATIVE_LONG_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object")
    if(NULL == (native_ulong = (H5T_t *)H5I_object(H5T_NATIVE_ULONG_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object")
    if(NULL == (native_llong = (H5T_t *)H5I_object(H5T_NATIVE_LLONG_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object")
    if(NULL == (native_ullong = (H5T_t *)H5I_object(H5T_NATIVE_ULLONG_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object")
    if(NULL == (native_float = (H5T_t *)H5I_object(H5T_NATIVE_FLOAT_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object")
    if(NULL == (native_double = (H5T_t *)H5I_object(H5T_NATIVE_DOUBLE_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object")
#if H5_SIZEOF_LONG_DOUBLE !=0
    if(NULL == (native_ldouble = (H5T_t *)H5I_object(H5T_NATIVE_LDOUBLE_g)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object")
#endif

    /*------------------------------------------------------------
     * Derived native types
     *------------------------------------------------------------
     */

    /* 1-byte bit field */
    H5T_INIT_TYPE(BITFIELD,H5T_NATIVE_B8_g,COPY,native_uint,SET,1)

    /* 2-byte bit field */
    H5T_INIT_TYPE(BITFIELD,H5T_NATIVE_B16_g,COPY,native_uint,SET,2)

    /* 4-byte bit field */
    H5T_INIT_TYPE(BITFIELD,H5T_NATIVE_B32_g,COPY,native_uint,SET,4)

    /* 8-byte bit field */
    H5T_INIT_TYPE(BITFIELD,H5T_NATIVE_B64_g,COPY,native_uint,SET,8)

    /* haddr_t */
    H5T_INIT_TYPE(OFFSET,H5T_NATIVE_HADDR_g,COPY,native_uint,SET,sizeof(haddr_t))

    /* hsize_t */
    H5T_INIT_TYPE(OFFSET,H5T_NATIVE_HSIZE_g,COPY,native_uint,SET,sizeof(hsize_t))

    /* hssize_t */
    H5T_INIT_TYPE(OFFSET,H5T_NATIVE_HSSIZE_g,COPY,native_int,SET,sizeof(hssize_t))

    /* herr_t */
    H5T_INIT_TYPE(OFFSET,H5T_NATIVE_HERR_g,COPY,native_int,SET,sizeof(herr_t))

    /* hbool_t */
    H5T_INIT_TYPE(OFFSET,H5T_NATIVE_HBOOL_g,COPY,native_int,SET,sizeof(hbool_t))

    /*------------------------------------------------------------
     * IEEE Types
     *------------------------------------------------------------
     */

    /* IEEE 4-byte little-endian float */
    H5T_INIT_TYPE(FLOATLE,H5T_IEEE_F32LE_g,COPY,native_double,SET,4)

    /* IEEE 4-byte big-endian float */
    H5T_INIT_TYPE(FLOATBE,H5T_IEEE_F32BE_g,COPY,native_double,SET,4)

    /* IEEE 8-byte little-endian float */
    H5T_INIT_TYPE(DOUBLELE,H5T_IEEE_F64LE_g,COPY,native_double,SET,8)

    /* IEEE 8-byte big-endian float */
    H5T_INIT_TYPE(DOUBLEBE,H5T_IEEE_F64BE_g,COPY,native_double,SET,8)

    /*------------------------------------------------------------
     * VAX Types
     *------------------------------------------------------------
     */

    /* VAX 4-byte float */
    H5T_INIT_TYPE(FLOATVAX,H5T_VAX_F32_g,COPY,native_double,SET,4)

    /* VAX 8-byte double */
    H5T_INIT_TYPE(DOUBLEVAX,H5T_VAX_F64_g,COPY,native_double,SET,8)

    /*------------------------------------------------------------
     * C99 types
     *------------------------------------------------------------
     */

    /* 1-byte little-endian (endianness is irrelevant) signed integer */
    H5T_INIT_TYPE(SINTLE,H5T_STD_I8LE_g,COPY,native_int,SET,1)

    /* 1-byte big-endian (endianness is irrelevant) signed integer */
    H5T_INIT_TYPE(SINTBE,H5T_STD_I8BE_g,COPY,native_int,SET,1)

    /* 2-byte little-endian signed integer */
    H5T_INIT_TYPE(SINTLE,H5T_STD_I16LE_g,COPY,native_int,SET,2)

    /* 2-byte big-endian signed integer */
    H5T_INIT_TYPE(SINTBE,H5T_STD_I16BE_g,COPY,native_int,SET,2)

    /* 4-byte little-endian signed integer */
    H5T_INIT_TYPE(SINTLE,H5T_STD_I32LE_g,COPY,native_int,SET,4)

    /* 4-byte big-endian signed integer */
    H5T_INIT_TYPE(SINTBE,H5T_STD_I32BE_g,COPY,native_int,SET,4)

    /* 8-byte little-endian signed integer */
    H5T_INIT_TYPE(SINTLE,H5T_STD_I64LE_g,COPY,native_int,SET,8)

    /* 8-byte big-endian signed integer */
    H5T_INIT_TYPE(SINTBE,H5T_STD_I64BE_g,COPY,native_int,SET,8)

    /* 1-byte little-endian (endianness is irrelevant) unsigned integer */
    H5T_INIT_TYPE(UINTLE,H5T_STD_U8LE_g,COPY,native_uint,SET,1)
    std_u8le=dt;    /* Keep type for later */

    /* 1-byte big-endian (endianness is irrelevant) unsigned integer */
    H5T_INIT_TYPE(UINTBE,H5T_STD_U8BE_g,COPY,native_uint,SET,1)
    std_u8be=dt;    /* Keep type for later */

    /* 2-byte little-endian unsigned integer */
    H5T_INIT_TYPE(UINTLE,H5T_STD_U16LE_g,COPY,native_uint,SET,2)
    std_u16le=dt;    /* Keep type for later */

    /* 2-byte big-endian unsigned integer */
    H5T_INIT_TYPE(UINTBE,H5T_STD_U16BE_g,COPY,native_uint,SET,2)
    std_u16be=dt;    /* Keep type for later */

    /* 4-byte little-endian unsigned integer */
    H5T_INIT_TYPE(UINTLE,H5T_STD_U32LE_g,COPY,native_uint,SET,4)
    std_u32le=dt;    /* Keep type for later */

    /* 4-byte big-endian unsigned integer */
    H5T_INIT_TYPE(UINTBE,H5T_STD_U32BE_g,COPY,native_uint,SET,4)
    std_u32be=dt;    /* Keep type for later */

    /* 8-byte little-endian unsigned integer */
    H5T_INIT_TYPE(UINTLE,H5T_STD_U64LE_g,COPY,native_uint,SET,8)
    std_u64le=dt;    /* Keep type for later */

    /* 8-byte big-endian unsigned integer */
    H5T_INIT_TYPE(UINTBE,H5T_STD_U64BE_g,COPY,native_uint,SET,8)
    std_u64be=dt;    /* Keep type for later */

    /*------------------------------------------------------------
     * Native, Little- & Big-endian bitfields
     *------------------------------------------------------------
     */

    /* little-endian (order is irrelevant) 8-bit bitfield */
    H5T_INIT_TYPE(BITFIELD, H5T_STD_B8LE_g, COPY, std_u8le, NOSET, -)
    bitfield=dt;    /* Keep type for later */

    /* big-endian (order is irrelevant) 8-bit bitfield */
    H5T_INIT_TYPE(BITFIELD, H5T_STD_B8BE_g, COPY, std_u8be, NOSET, -)

    /* Little-endian 16-bit bitfield */
    H5T_INIT_TYPE(BITFIELD, H5T_STD_B16LE_g, COPY, std_u16le, NOSET, -)

    /* Big-endian 16-bit bitfield */
    H5T_INIT_TYPE(BITFIELD, H5T_STD_B16BE_g, COPY, std_u16be, NOSET, -)

    /* Little-endian 32-bit bitfield */
    H5T_INIT_TYPE(BITFIELD, H5T_STD_B32LE_g, COPY, std_u32le, NOSET, -)

    /* Big-endian 32-bit bitfield */
    H5T_INIT_TYPE(BITFIELD, H5T_STD_B32BE_g, COPY, std_u32be, NOSET, -)

    /* Little-endian 64-bit bitfield */
    H5T_INIT_TYPE(BITFIELD, H5T_STD_B64LE_g, COPY, std_u64le, NOSET, -)

    /* Big-endian 64-bit bitfield */
    H5T_INIT_TYPE(BITFIELD, H5T_STD_B64BE_g, COPY, std_u64be, NOSET, -)

    /*------------------------------------------------------------
     * The Unix architecture for dates and times.
     *------------------------------------------------------------
     */

    /* Little-endian 32-bit UNIX time_t */
    H5T_INIT_TYPE(TIME, H5T_UNIX_D32LE_g, COPY, std_u32le, NOSET, -)

    /* Big-endian 32-bit UNIX time_t */
    H5T_INIT_TYPE(TIME, H5T_UNIX_D32BE_g, COPY, std_u32be, NOSET, -)

    /* Little-endian 64-bit UNIX time_t */
    H5T_INIT_TYPE(TIME, H5T_UNIX_D64LE_g, COPY, std_u64le, NOSET, -)

    /* Big-endian 64-bit UNIX time_t */
    H5T_INIT_TYPE(TIME, H5T_UNIX_D64BE_g, COPY, std_u64be, NOSET, -)


    /* Indicate that the types that are created from here down are allocated
     * H5FL_ALLOC(), not copied with H5T_copy()
     */
    copied_dtype = FALSE;

    /* Opaque data */
    H5T_INIT_TYPE(OPAQ, H5T_NATIVE_OPAQUE_g, ALLOC, -, SET, 1)

    /*------------------------------------------------------------
     * The `C' architecture
     *------------------------------------------------------------
     */

    /* One-byte character string */
    H5T_INIT_TYPE(CSTRING, H5T_C_S1_g, ALLOC, -, SET, 1)
    string=dt;    /* Keep type for later */

    /*------------------------------------------------------------
     * The `Fortran' architecture
     *------------------------------------------------------------
     */

    /* One-byte character string */
    H5T_INIT_TYPE(FORSTRING, H5T_FORTRAN_S1_g, ALLOC, -, SET, 1)

    /*------------------------------------------------------------
     * Reference types
     *------------------------------------------------------------
     */

    /* Object reference (i.e. object header address in file) */
    H5T_INIT_TYPE(OBJREF, H5T_STD_REF_OBJ_g, ALLOC, -, SET, H5R_OBJ_REF_BUF_SIZE)
    objref=dt;    /* Keep type for later */

    /* Dataset Region reference (i.e. selection inside a dataset) */
    H5T_INIT_TYPE(REGREF, H5T_STD_REF_DSETREG_g, ALLOC, -, SET, H5R_DSET_REG_REF_BUF_SIZE)

    /*
     * Register conversion functions beginning with the most general and
     * ending with the most specific.
     */
    fixedpt = native_int;
    floatpt = native_float;
    if (NULL == (compound = H5T__create(H5T_COMPOUND, (size_t)1)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    if (NULL == (enum_type = H5T__create(H5T_ENUM, (size_t)1)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    if (NULL == (vlen = H5T__vlen_create(native_int)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    if (NULL == (array = H5T__array_create(native_int, 1, dim)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    status = 0;

    status |= H5T_register(H5T_PERS_SOFT, "i_i", fixedpt, fixedpt, H5T__conv_i_i, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_SOFT, "f_f", floatpt, floatpt, H5T__conv_f_f, H5AC_dxpl_id, FALSE);

    status |= H5T_register(H5T_PERS_SOFT, "i_f", fixedpt, floatpt, H5T__conv_i_f, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_SOFT, "f_i", floatpt, fixedpt, H5T__conv_f_i, H5AC_dxpl_id, FALSE);

    status |= H5T_register(H5T_PERS_SOFT, "s_s", string, string, H5T__conv_s_s, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_SOFT, "b_b", bitfield, bitfield, H5T__conv_b_b, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_SOFT, "ibo", fixedpt, fixedpt, H5T__conv_order, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_SOFT, "ibo(opt)", fixedpt, fixedpt, H5T__conv_order_opt, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_SOFT, "fbo", floatpt, floatpt, H5T__conv_order, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_SOFT, "fbo(opt)", floatpt, floatpt, H5T__conv_order_opt, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_SOFT, "struct(no-opt)", compound, compound, H5T__conv_struct, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_SOFT, "struct(opt)", compound, compound, H5T__conv_struct_opt, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_SOFT, "enum", enum_type, enum_type, H5T__conv_enum, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_SOFT, "enum_i", enum_type, fixedpt, H5T__conv_enum_numeric, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_SOFT, "enum_f", enum_type, floatpt, H5T__conv_enum_numeric, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_SOFT, "vlen", vlen, vlen, H5T__conv_vlen, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_SOFT, "array", array, array, H5T__conv_array, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_SOFT, "objref", objref, objref, H5T__conv_order_opt, H5AC_dxpl_id, FALSE);

    /*
     * Native conversions should be listed last since we can use hardware to
     * perform the conversion.  We list the odd types like `llong', `long',
     * and `short' before the usual types like `int' and `char' so that when
     * diagnostics are printed we favor the usual names over the odd names
     * when two or more types are the same size.
     */

    /* floating point */
#if H5T_CONV_INTERNAL_FP_FP
    status |= H5T_register(H5T_PERS_HARD, "flt_dbl", native_float, native_double, H5T__conv_float_double, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "dbl_flt", native_double, native_float, H5T__conv_double_float, H5AC_dxpl_id, FALSE);
#endif /*H5T_CONV_INTERNAL_FP_FP*/
#if H5T_CONV_INTERNAL_FP_LDOUBLE
    status |= H5T_register(H5T_PERS_HARD, "flt_ldbl", native_float, native_ldouble, H5T__conv_float_ldouble, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "dbl_ldbl", native_double, native_ldouble, H5T__conv_double_ldouble, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ldbl_flt", native_ldouble, native_float, H5T__conv_ldouble_float, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ldbl_dbl", native_ldouble, native_double, H5T__conv_ldouble_double, H5AC_dxpl_id, FALSE);
#endif /*H5T_CONV_INTERNAL_FP_LDOUBLE*/

    /* from long long */
    status |= H5T_register(H5T_PERS_HARD, "llong_ullong", native_llong, native_ullong, H5T__conv_llong_ullong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ullong_llong", native_ullong, native_llong, H5T__conv_ullong_llong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "llong_long", native_llong, native_long, H5T__conv_llong_long, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "llong_ulong", native_llong, native_ulong, H5T__conv_llong_ulong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ullong_long", native_ullong, native_long, H5T__conv_ullong_long, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ullong_ulong", native_ullong, native_ulong, H5T__conv_ullong_ulong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "llong_short", native_llong, native_short, H5T__conv_llong_short, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "llong_ushort", native_llong, native_ushort, H5T__conv_llong_ushort, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ullong_short", native_ullong, native_short, H5T__conv_ullong_short, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ullong_ushort", native_ullong, native_ushort, H5T__conv_ullong_ushort, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "llong_int", native_llong, native_int, H5T__conv_llong_int, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "llong_uint", native_llong, native_uint, H5T__conv_llong_uint, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ullong_int", native_ullong, native_int, H5T__conv_ullong_int, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ullong_uint", native_ullong, native_uint, H5T__conv_ullong_uint, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "llong_schar", native_llong, native_schar, H5T__conv_llong_schar, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "llong_uchar", native_llong, native_uchar, H5T__conv_llong_uchar, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ullong_schar", native_ullong, native_schar, H5T__conv_ullong_schar, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ullong_uchar", native_ullong, native_uchar, H5T__conv_ullong_uchar, H5AC_dxpl_id, FALSE);

    /* From long */
    status |= H5T_register(H5T_PERS_HARD, "long_llong", native_long, native_llong, H5T__conv_long_llong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "long_ullong", native_long, native_ullong, H5T__conv_long_ullong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ulong_llong", native_ulong, native_llong, H5T__conv_ulong_llong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ulong_ullong", native_ulong, native_ullong, H5T__conv_ulong_ullong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "long_ulong", native_long, native_ulong, H5T__conv_long_ulong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ulong_long", native_ulong, native_long, H5T__conv_ulong_long, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "long_short", native_long, native_short, H5T__conv_long_short, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "long_ushort", native_long, native_ushort, H5T__conv_long_ushort, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ulong_short", native_ulong, native_short, H5T__conv_ulong_short, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ulong_ushort", native_ulong, native_ushort, H5T__conv_ulong_ushort, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "long_int", native_long, native_int, H5T__conv_long_int, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "long_uint", native_long, native_uint, H5T__conv_long_uint, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ulong_int", native_ulong, native_int, H5T__conv_ulong_int, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ulong_uint", native_ulong, native_uint, H5T__conv_ulong_uint, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "long_schar", native_long, native_schar, H5T__conv_long_schar, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "long_uchar", native_long, native_uchar, H5T__conv_long_uchar, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ulong_schar", native_ulong, native_schar, H5T__conv_ulong_schar, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ulong_uchar", native_ulong, native_uchar, H5T__conv_ulong_uchar, H5AC_dxpl_id, FALSE);

    /* From short */
    status |= H5T_register(H5T_PERS_HARD, "short_llong", native_short, native_llong, H5T__conv_short_llong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "short_ullong", native_short, native_ullong, H5T__conv_short_ullong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ushort_llong", native_ushort, native_llong, H5T__conv_ushort_llong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ushort_ullong", native_ushort, native_ullong, H5T__conv_ushort_ullong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "short_long", native_short, native_long, H5T__conv_short_long, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "short_ulong", native_short, native_ulong, H5T__conv_short_ulong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ushort_long", native_ushort, native_long, H5T__conv_ushort_long, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ushort_ulong", native_ushort, native_ulong, H5T__conv_ushort_ulong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "short_ushort", native_short, native_ushort, H5T__conv_short_ushort, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ushort_short", native_ushort, native_short, H5T__conv_ushort_short, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "short_int", native_short, native_int, H5T__conv_short_int, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "short_uint", native_short, native_uint, H5T__conv_short_uint, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ushort_int", native_ushort, native_int, H5T__conv_ushort_int, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ushort_uint", native_ushort, native_uint, H5T__conv_ushort_uint, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "short_schar", native_short, native_schar, H5T__conv_short_schar, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "short_uchar", native_short, native_uchar, H5T__conv_short_uchar, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ushort_schar", native_ushort, native_schar, H5T__conv_ushort_schar, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ushort_uchar", native_ushort, native_uchar, H5T__conv_ushort_uchar, H5AC_dxpl_id, FALSE);

    /* From int */
    status |= H5T_register(H5T_PERS_HARD, "int_llong", native_int, native_llong, H5T__conv_int_llong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "int_ullong", native_int, native_ullong, H5T__conv_int_ullong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uint_llong", native_uint, native_llong, H5T__conv_uint_llong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uint_ullong", native_uint, native_ullong, H5T__conv_uint_ullong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "int_long", native_int, native_long, H5T__conv_int_long, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "int_ulong", native_int, native_ulong, H5T__conv_int_ulong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uint_long", native_uint, native_long, H5T__conv_uint_long, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uint_ulong", native_uint, native_ulong, H5T__conv_uint_ulong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "int_short", native_int, native_short, H5T__conv_int_short, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "int_ushort", native_int, native_ushort, H5T__conv_int_ushort, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uint_short", native_uint, native_short, H5T__conv_uint_short, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uint_ushort", native_uint, native_ushort, H5T__conv_uint_ushort, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "int_uint", native_int, native_uint, H5T__conv_int_uint, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uint_int", native_uint, native_int, H5T__conv_uint_int, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "int_schar", native_int, native_schar, H5T__conv_int_schar, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "int_uchar", native_int, native_uchar, H5T__conv_int_uchar, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uint_schar", native_uint, native_schar, H5T__conv_uint_schar, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uint_uchar", native_uint, native_uchar, H5T__conv_uint_uchar, H5AC_dxpl_id, FALSE);

    /* From char */
    status |= H5T_register(H5T_PERS_HARD, "schar_llong", native_schar, native_llong, H5T__conv_schar_llong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "schar_ullong", native_schar, native_ullong, H5T__conv_schar_ullong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uchar_llong", native_uchar, native_llong, H5T__conv_uchar_llong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uchar_ullong", native_uchar, native_ullong, H5T__conv_uchar_ullong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "schar_long", native_schar, native_long, H5T__conv_schar_long, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "schar_ulong", native_schar, native_ulong, H5T__conv_schar_ulong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uchar_long", native_uchar, native_long, H5T__conv_uchar_long, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uchar_ulong", native_uchar, native_ulong, H5T__conv_uchar_ulong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "schar_short", native_schar, native_short, H5T__conv_schar_short, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "schar_ushort", native_schar, native_ushort, H5T__conv_schar_ushort, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uchar_short", native_uchar, native_short, H5T__conv_uchar_short, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uchar_ushort", native_uchar, native_ushort, H5T__conv_uchar_ushort, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "schar_int", native_schar, native_int, H5T__conv_schar_int, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "schar_uint", native_schar, native_uint, H5T__conv_schar_uint, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uchar_int", native_uchar, native_int, H5T__conv_uchar_int, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uchar_uint", native_uchar, native_uint, H5T__conv_uchar_uint, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "schar_uchar", native_schar, native_uchar, H5T__conv_schar_uchar, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uchar_schar", native_uchar, native_schar, H5T__conv_uchar_schar, H5AC_dxpl_id, FALSE);

    /* From char to floats */
    status |= H5T_register(H5T_PERS_HARD, "schar_flt", native_schar, native_float, H5T__conv_schar_float, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "schar_dbl", native_schar, native_double, H5T__conv_schar_double, H5AC_dxpl_id, FALSE);
#if H5T_CONV_INTERNAL_INTEGER_LDOUBLE
    status |= H5T_register(H5T_PERS_HARD, "schar_ldbl", native_schar, native_ldouble, H5T__conv_schar_ldouble, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_INTEGER_LDOUBLE */

    /* From unsigned char to floats */
    status |= H5T_register(H5T_PERS_HARD, "uchar_flt", native_uchar, native_float, H5T__conv_uchar_float, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uchar_dbl", native_uchar, native_double, H5T__conv_uchar_double, H5AC_dxpl_id, FALSE);
#if H5T_CONV_INTERNAL_INTEGER_LDOUBLE
    status |= H5T_register(H5T_PERS_HARD, "uchar_ldbl", native_uchar, native_ldouble, H5T__conv_uchar_ldouble, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_INTEGER_LDOUBLE */

    /* From short to floats */
    status |= H5T_register(H5T_PERS_HARD, "short_flt", native_short, native_float, H5T__conv_short_float, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "short_dbl", native_short, native_double, H5T__conv_short_double, H5AC_dxpl_id, FALSE);
#if H5T_CONV_INTERNAL_INTEGER_LDOUBLE
    status |= H5T_register(H5T_PERS_HARD, "short_ldbl", native_short, native_ldouble, H5T__conv_short_ldouble, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_INTEGER_LDOUBLE */

    /* From unsigned short to floats */
    status |= H5T_register(H5T_PERS_HARD, "ushort_flt", native_ushort, native_float, H5T__conv_ushort_float, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ushort_dbl", native_ushort, native_double, H5T__conv_ushort_double, H5AC_dxpl_id, FALSE);
#if H5T_CONV_INTERNAL_INTEGER_LDOUBLE
    status |= H5T_register(H5T_PERS_HARD, "ushort_ldbl", native_ushort, native_ldouble, H5T__conv_ushort_ldouble, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_INTEGER_LDOUBLE */

    /* From int to floats */
    status |= H5T_register(H5T_PERS_HARD, "int_flt", native_int, native_float, H5T__conv_int_float, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "int_dbl", native_int, native_double, H5T__conv_int_double, H5AC_dxpl_id, FALSE);
#if H5T_CONV_INTERNAL_INTEGER_LDOUBLE
    status |= H5T_register(H5T_PERS_HARD, "int_ldbl", native_int, native_ldouble, H5T__conv_int_ldouble, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_INTEGER_LDOUBLE */

    /* From unsigned int to floats */
    status |= H5T_register(H5T_PERS_HARD, "uint_flt", native_uint, native_float, H5T__conv_uint_float, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "uint_dbl", native_uint, native_double, H5T__conv_uint_double, H5AC_dxpl_id, FALSE);
#if H5T_CONV_INTERNAL_INTEGER_LDOUBLE
    status |= H5T_register(H5T_PERS_HARD, "uint_ldbl", native_uint, native_ldouble, H5T__conv_uint_ldouble, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_INTEGER_LDOUBLE */

    /* From long to floats */
    status |= H5T_register(H5T_PERS_HARD, "long_flt", native_long, native_float, H5T__conv_long_float, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "long_dbl", native_long, native_double, H5T__conv_long_double, H5AC_dxpl_id, FALSE);
#if H5T_CONV_INTERNAL_INTEGER_LDOUBLE
    status |= H5T_register(H5T_PERS_HARD, "long_ldbl", native_long, native_ldouble, H5T__conv_long_ldouble, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_INTEGER_LDOUBLE */

    /* From unsigned long to floats */
#if H5T_CONV_INTERNAL_ULONG_FLT
    status |= H5T_register(H5T_PERS_HARD, "ulong_flt", native_ulong, native_float, H5T__conv_ulong_float, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_ULONG_FLT */
#if H5T_CONV_INTERNAL_ULONG_DBL
    status |= H5T_register(H5T_PERS_HARD, "ulong_dbl", native_ulong, native_double, H5T__conv_ulong_double, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_ULONG_DBL */
#if H5T_CONV_INTERNAL_ULONG_LDOUBLE
    status |= H5T_register(H5T_PERS_HARD, "ulong_ldbl", native_ulong, native_ldouble, H5T__conv_ulong_ldouble, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_ULONG_LDOUBLE */

    /* From long long to floats */
    status |= H5T_register(H5T_PERS_HARD, "llong_flt", native_llong, native_float, H5T__conv_llong_float, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "llong_dbl", native_llong, native_double, H5T__conv_llong_double, H5AC_dxpl_id, FALSE);
#ifdef H5T_CONV_INTERNAL_LLONG_LDOUBLE
    status |= H5T_register(H5T_PERS_HARD, "llong_ldbl", native_llong, native_ldouble, H5T__conv_llong_ldouble, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_LLONG_LDOUBLE */

    /* From unsigned long long to floats */
#if H5T_CONV_INTERNAL_ULLONG_FP
    status |= H5T_register(H5T_PERS_HARD, "ullong_flt", native_ullong, native_float, H5T__conv_ullong_float, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "ullong_dbl", native_ullong, native_double, H5T__conv_ullong_double, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_ULLONG_FP */
#ifdef H5T_CONV_INTERNAL_ULLONG_LDOUBLE
    status |= H5T_register(H5T_PERS_HARD, "ullong_ldbl", native_ullong, native_ldouble, H5T__conv_ullong_ldouble, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_ULLONG_LDOUBLE */

    /* From floats to char */
    status |= H5T_register(H5T_PERS_HARD, "flt_schar", native_float, native_schar, H5T__conv_float_schar, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "dbl_schar", native_double, native_schar, H5T__conv_double_schar, H5AC_dxpl_id, FALSE);
#if H5T_CONV_INTERNAL_LDOUBLE_INTEGER
    status |= H5T_register(H5T_PERS_HARD, "ldbl_schar", native_ldouble, native_schar, H5T__conv_ldouble_schar, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_LDOUBLE_INTEGER */

    /* From floats to unsigned char */
    status |= H5T_register(H5T_PERS_HARD, "flt_uchar", native_float, native_uchar, H5T__conv_float_uchar, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "dbl_uchar", native_double, native_uchar, H5T__conv_double_uchar, H5AC_dxpl_id, FALSE);
#if H5T_CONV_INTERNAL_LDOUBLE_INTEGER
    status |= H5T_register(H5T_PERS_HARD, "ldbl_uchar", native_ldouble, native_uchar, H5T__conv_ldouble_uchar, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_LDOUBLE_INTEGER */

    /* From floats to short */
    status |= H5T_register(H5T_PERS_HARD, "flt_short", native_float, native_short, H5T__conv_float_short, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "dbl_short", native_double, native_short, H5T__conv_double_short, H5AC_dxpl_id, FALSE);
#if H5T_CONV_INTERNAL_LDOUBLE_INTEGER
    status |= H5T_register(H5T_PERS_HARD, "ldbl_short", native_ldouble, native_short, H5T__conv_ldouble_short, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_LDOUBLE_INTEGER */

    /* From floats to unsigned short */
    status |= H5T_register(H5T_PERS_HARD, "flt_ushort", native_float, native_ushort, H5T__conv_float_ushort, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "dbl_ushort", native_double, native_ushort, H5T__conv_double_ushort, H5AC_dxpl_id, FALSE);
#if H5T_CONV_INTERNAL_LDOUBLE_INTEGER
    status |= H5T_register(H5T_PERS_HARD, "ldbl_ushort", native_ldouble, native_ushort, H5T__conv_ldouble_ushort, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_LDOUBLE_INTEGER */

    /* From floats to int */
    status |= H5T_register(H5T_PERS_HARD, "flt_int", native_float, native_int, H5T__conv_float_int, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "dbl_int", native_double, native_int, H5T__conv_double_int, H5AC_dxpl_id, FALSE);
#if H5T_CONV_INTERNAL_LDOUBLE_INTEGER
    status |= H5T_register(H5T_PERS_HARD, "ldbl_int", native_ldouble, native_int, H5T__conv_ldouble_int, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_LDOUBLE_INTEGER */

    /* From floats to unsigned int */
    status |= H5T_register(H5T_PERS_HARD, "flt_uint", native_float, native_uint, H5T__conv_float_uint, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "dbl_uint", native_double, native_uint, H5T__conv_double_uint, H5AC_dxpl_id, FALSE);
#if H5T_CONV_INTERNAL_LDOUBLE_UINT
    status |= H5T_register(H5T_PERS_HARD, "ldbl_uint", native_ldouble, native_uint, H5T__conv_ldouble_uint, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_LDOUBLE_UINT */

    status |= H5T_register(H5T_PERS_HARD, "flt_long", native_float, native_long, H5T__conv_float_long, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "dbl_long", native_double, native_long, H5T__conv_double_long, H5AC_dxpl_id, FALSE);
#if H5T_CONV_INTERNAL_LDOUBLE_INTEGER
    status |= H5T_register(H5T_PERS_HARD, "ldbl_long", native_ldouble, native_long, H5T__conv_ldouble_long, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_LDOUBLE_INTEGER */

    /* From floats to unsigned long */
    status |= H5T_register(H5T_PERS_HARD, "flt_ulong", native_float, native_ulong, H5T__conv_float_ulong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "dbl_ulong", native_double, native_ulong, H5T__conv_double_ulong, H5AC_dxpl_id, FALSE);
#if H5T_CONV_INTERNAL_LDOUBLE_INTEGER
    status |= H5T_register(H5T_PERS_HARD, "ldbl_ulong", native_ldouble, native_ulong, H5T__conv_ldouble_ulong, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_LDOUBLE_INTEGER */

    /* From floats to long long */
#if H5T_CONV_INTERNAL_FP_LLONG
    status |= H5T_register(H5T_PERS_HARD, "flt_llong", native_float, native_llong, H5T__conv_float_llong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "dbl_llong", native_double, native_llong, H5T__conv_double_llong, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_FP_LLONG */
#ifdef H5T_CONV_INTERNAL_LDOUBLE_LLONG
    status |= H5T_register(H5T_PERS_HARD, "ldbl_llong", native_ldouble, native_llong, H5T__conv_ldouble_llong, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_LDOUBLE_LLONG */

    /* From floats to unsigned long long */
#if H5T_CONV_INTERNAL_FP_ULLONG
    status |= H5T_register(H5T_PERS_HARD, "flt_ullong", native_float, native_ullong, H5T__conv_float_ullong, H5AC_dxpl_id, FALSE);
    status |= H5T_register(H5T_PERS_HARD, "dbl_ullong", native_double, native_ullong, H5T__conv_double_ullong, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_FP_ULLONG */
#if H5T_CONV_INTERNAL_LDOUBLE_ULLONG
    status |= H5T_register(H5T_PERS_HARD, "ldbl_ullong", native_ldouble, native_ullong, H5T__conv_ldouble_ullong, H5AC_dxpl_id, FALSE);
#endif /* H5T_CONV_INTERNAL_LDOUBLE_ULLONG */

    /*
     * The special no-op conversion is the fastest, so we list it last. The
     * data types we use are not important as long as the source and
     * destination are equal.
     */
    status |= H5T_register(H5T_PERS_HARD, "no-op", native_int, native_int, H5T__conv_noop, H5AC_dxpl_id, FALSE);

    /* Initialize the +/- Infinity values for floating-point types */
    status |= H5T_init_inf();

    if (status<0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to register conversion function(s)")

    /* ========== Datatype Creation Property Class Initialization ============*/
    HDassert(H5P_CLS_DATATYPE_CREATE_g!=-1);

    /* Get the pointer to group creation class */
    if(NULL == (crt_pclass = (H5P_genclass_t *)H5I_object(H5P_CLS_DATATYPE_CREATE_g)))
         HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list class")

    /* Register datatype creation property class properties here.  See similar
     * code in H5D_init_interface(), etc. for example.
     */

    /* Only register the default property list if it hasn't been created yet */
    if(H5P_LST_DATATYPE_CREATE_g == (-1)) {
        /* Register the default datatype creation property list */
        if((H5P_LST_DATATYPE_CREATE_g = H5P_create_id(crt_pclass, FALSE)) < 0)
             HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "can't insert property into class")
    } /* end if */

done:
    /* General cleanup */
    if(compound != NULL)
        H5T_close(compound);
    if(enum_type != NULL)
        H5T_close(enum_type);
    if(vlen != NULL)
        H5T_close(vlen);
    if(array != NULL)
        H5T_close(array);

    /* Error cleanup */
    if(ret_value < 0) {
        if(dt) {
            /* Check if we should call H5T_close or H5FL_FREE */
            if(copied_dtype)
                H5T_close(dt);
            else {
                dt->shared = H5FL_FREE(H5T_shared_t, dt->shared);
                dt = H5FL_FREE(H5T_t, dt);
            } /* end else */
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_init_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5T_unlock_cb
 *
 * Purpose:	Clear the immutable flag for a datatype.  This function is
 *		called when the library is closing in order to unlock all
 *		registered datatypes and thus make them free-able.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, April 27, 1998
 *
 *-------------------------------------------------------------------------
 */
static int
H5T_unlock_cb(void *_dt, hid_t UNUSED id, void UNUSED *key)
{
    H5T_t	*dt = (H5T_t *)_dt;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert (dt && dt->shared);
    if (H5T_STATE_IMMUTABLE==dt->shared->state)
	dt->shared->state = H5T_STATE_RDONLY;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5T_unlock_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5T_term_interface
 *
 * Purpose:	Close this interface.
 *
 * Return:	Success:	Positive if any action might have caused a
 *				change in some other interface; zero
 *				otherwise.
 *
 * 		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Friday, November 20, 1998
 *
 *-------------------------------------------------------------------------
 */
int
H5T_term_interface(void)
{
    int	i, nprint=0, n=0;
    H5T_path_t	*path = NULL;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(H5_interface_initialize_g) {
	/* Unregister all conversion functions */
	for(i = 0; i < H5T_g.npaths; i++) {
	    path = H5T_g.path[i];
	    HDassert(path);
	    if(path->func) {
		H5T__print_stats(path, &nprint/*in,out*/);
		path->cdata.command = H5T_CONV_FREE;
		if((path->func)(FAIL, FAIL, &(path->cdata), (size_t)0,
                        (size_t)0, (size_t)0, NULL, NULL,H5AC_dxpl_id) < 0) {
#ifdef H5T_DEBUG
		    if (H5DEBUG(T)) {
			fprintf(H5DEBUG(T), "H5T: conversion function "
				 "0x%08lx failed to free private data for "
				 "%s (ignored)\n",
				 (unsigned long)(path->func), path->name);
		    } /* end if */
#endif
		    H5E_clear_stack(NULL); /*ignore the error*/
		} /* end if */
	    } /* end if */

            if(path->src)
                H5T_close(path->src);
            if(path->dst)
                H5T_close(path->dst);
            path = H5FL_FREE(H5T_path_t, path);
	    H5T_g.path[i] = NULL;
	} /* end for */

	/* Clear conversion tables */
	H5T_g.path = (H5T_path_t **)H5MM_xfree(H5T_g.path);
	H5T_g.npaths = 0;
	H5T_g.apaths = 0;
	H5T_g.soft = (H5T_soft_t *)H5MM_xfree(H5T_g.soft);
	H5T_g.nsoft = 0;
	H5T_g.asoft = 0;

	/* Unlock all datatypes, then free them */
	/* note that we are ignoring the return value from H5I_iterate() */
	H5I_iterate(H5I_DATATYPE, H5T_unlock_cb, NULL, FALSE);

        /* Close deprecated interface */
        n += H5T__term_deprec_interface();

	H5I_dec_type_ref(H5I_DATATYPE);

        /* Reset all the datatype IDs */
        H5T_IEEE_F32BE_g			= FAIL;
        H5T_IEEE_F32LE_g			= FAIL;
        H5T_IEEE_F64BE_g			= FAIL;
        H5T_IEEE_F64LE_g			= FAIL;

        H5T_STD_I8BE_g			= FAIL;
        H5T_STD_I8LE_g			= FAIL;
        H5T_STD_I16BE_g			= FAIL;
        H5T_STD_I16LE_g			= FAIL;
        H5T_STD_I32BE_g			= FAIL;
        H5T_STD_I32LE_g			= FAIL;
        H5T_STD_I64BE_g			= FAIL;
        H5T_STD_I64LE_g			= FAIL;
        H5T_STD_U8BE_g			= FAIL;
        H5T_STD_U8LE_g			= FAIL;
        H5T_STD_U16BE_g			= FAIL;
        H5T_STD_U16LE_g			= FAIL;
        H5T_STD_U32BE_g			= FAIL;
        H5T_STD_U32LE_g			= FAIL;
        H5T_STD_U64BE_g			= FAIL;
        H5T_STD_U64LE_g			= FAIL;
        H5T_STD_B8BE_g			= FAIL;
        H5T_STD_B8LE_g			= FAIL;
        H5T_STD_B16BE_g			= FAIL;
        H5T_STD_B16LE_g			= FAIL;
        H5T_STD_B32BE_g			= FAIL;
        H5T_STD_B32LE_g			= FAIL;
        H5T_STD_B64BE_g			= FAIL;
        H5T_STD_B64LE_g 			= FAIL;
        H5T_STD_REF_OBJ_g 		= FAIL;
        H5T_STD_REF_DSETREG_g 		= FAIL;

        H5T_UNIX_D32BE_g			= FAIL;
        H5T_UNIX_D32LE_g			= FAIL;
        H5T_UNIX_D64BE_g			= FAIL;
        H5T_UNIX_D64LE_g 			= FAIL;

        H5T_C_S1_g			= FAIL;

        H5T_FORTRAN_S1_g			= FAIL;

        H5T_NATIVE_SCHAR_g		= FAIL;
        H5T_NATIVE_UCHAR_g		= FAIL;
        H5T_NATIVE_SHORT_g		= FAIL;
        H5T_NATIVE_USHORT_g		= FAIL;
        H5T_NATIVE_INT_g			= FAIL;
        H5T_NATIVE_UINT_g			= FAIL;
        H5T_NATIVE_LONG_g			= FAIL;
        H5T_NATIVE_ULONG_g		= FAIL;
        H5T_NATIVE_LLONG_g		= FAIL;
        H5T_NATIVE_ULLONG_g		= FAIL;
        H5T_NATIVE_FLOAT_g		= FAIL;
        H5T_NATIVE_DOUBLE_g		= FAIL;
#if H5_SIZEOF_LONG_DOUBLE !=0
        H5T_NATIVE_LDOUBLE_g		= FAIL;
#endif
        H5T_NATIVE_B8_g			= FAIL;
        H5T_NATIVE_B16_g			= FAIL;
        H5T_NATIVE_B32_g			= FAIL;
        H5T_NATIVE_B64_g			= FAIL;
        H5T_NATIVE_OPAQUE_g		= FAIL;
        H5T_NATIVE_HADDR_g		= FAIL;
        H5T_NATIVE_HSIZE_g		= FAIL;
        H5T_NATIVE_HSSIZE_g		= FAIL;
        H5T_NATIVE_HERR_g			= FAIL;
        H5T_NATIVE_HBOOL_g		= FAIL;

        H5T_NATIVE_INT8_g			= FAIL;
        H5T_NATIVE_UINT8_g		= FAIL;
        H5T_NATIVE_INT_LEAST8_g		= FAIL;
        H5T_NATIVE_UINT_LEAST8_g		= FAIL;
        H5T_NATIVE_INT_FAST8_g		= FAIL;
        H5T_NATIVE_UINT_FAST8_g		= FAIL;

        H5T_NATIVE_INT16_g		= FAIL;
        H5T_NATIVE_UINT16_g		= FAIL;
        H5T_NATIVE_INT_LEAST16_g		= FAIL;
        H5T_NATIVE_UINT_LEAST16_g		= FAIL;
        H5T_NATIVE_INT_FAST16_g		= FAIL;
        H5T_NATIVE_UINT_FAST16_g		= FAIL;

        H5T_NATIVE_INT32_g		= FAIL;
        H5T_NATIVE_UINT32_g		= FAIL;
        H5T_NATIVE_INT_LEAST32_g		= FAIL;
        H5T_NATIVE_UINT_LEAST32_g		= FAIL;
        H5T_NATIVE_INT_FAST32_g		= FAIL;
        H5T_NATIVE_UINT_FAST32_g		= FAIL;

        H5T_NATIVE_INT64_g		= FAIL;
        H5T_NATIVE_UINT64_g		= FAIL;
        H5T_NATIVE_INT_LEAST64_g		= FAIL;
        H5T_NATIVE_UINT_LEAST64_g		= FAIL;
        H5T_NATIVE_INT_FAST64_g		= FAIL;
        H5T_NATIVE_UINT_FAST64_g		= FAIL;

	/* Mark interface as closed */
	H5_interface_initialize_g = 0;
	n = 1; /*H5I*/
    } /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* end H5T_term_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Tcreate
 *
 * Purpose:	Create a new type and initialize it to reasonable values.
 *		The type is a member of type class TYPE and is SIZE bytes.
 *
 * Return:	Success:	A new type identifier.
 *
 *		Failure:	Negative
 *
 * Errors:
 *		ARGS	  BADVALUE	Invalid size.
 *		DATATYPE  CANTINIT	Can't create type.
 *		DATATYPE  CANTREGISTER	Can't register datatype atom.
 *
 * Programmer:	Robb Matzke
 *		Friday, December  5, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tcreate(H5T_class_t type, size_t size)
{
    H5T_t	*dt = NULL;     /* New datatype constructed */
    hid_t	ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("i", "Ttz", type, size);

    /* check args. We support string (fixed-size or variable-length) now. */
    if(size <= 0 && size != H5T_VARIABLE)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "size must be positive")

    /* create the type */
    if(NULL == (dt = H5T__create(type, size)))
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to create type")

    /* Get an ID for the datatype */
    if((ret_value = H5I_register(H5I_DATATYPE, dt, TRUE)) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register datatype ID")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tcreate() */


/*-------------------------------------------------------------------------
 * Function:	H5Tcopy
 *
 * Purpose:	Copies a datatype.  The resulting datatype is not locked.
 *		The datatype should be closed when no longer needed by
 *		calling H5Tclose().
 *
 * Return:	Success:	The ID of a new datatype.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		Tuesday, December  9, 1997
 *
 * Modifications:
 *
 * 	Robb Matzke, 4 Jun 1998
 *	The returned type is always transient and unlocked.  If the TYPE_ID
 *	argument is a dataset instead of a datatype then this function
 *	returns a transient, modifiable datatype which is a copy of the
 *	dataset's datatype.
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tcopy(hid_t type_id)
{
    H5T_t	*dt;                    /* Pointer to the datatype to copy */
    H5T_t	*new_dt = NULL;
    hid_t	ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("i", "i", type_id);

    switch(H5I_get_type(type_id)) {
        case H5I_DATATYPE:
            /* The argument is a datatype handle */
            if(NULL == (dt = (H5T_t *)H5I_object(type_id)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
            break;

        case H5I_DATASET:
            {
                H5D_t	*dset;          /* Dataset for datatype */

                /* The argument is a dataset handle */
                if(NULL == (dset = (H5D_t *)H5I_object(type_id)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
                if(NULL == (dt = H5D_typeof(dset)))
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to get the dataset datatype")
            }
            break;

        case H5I_UNINIT:
        case H5I_BADID:
        case H5I_FILE:
        case H5I_GROUP:
        case H5I_DATASPACE:
        case H5I_ATTR:
        case H5I_REFERENCE:
        case H5I_VFL:
        case H5I_GENPROP_CLS:
        case H5I_GENPROP_LST:
        case H5I_ERROR_CLASS:
        case H5I_ERROR_MSG:
        case H5I_ERROR_STACK:
        case H5I_NTYPES:
        default:
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype or dataset")
    } /* end switch */

    /* Copy datatype */
    if(NULL == (new_dt = H5T_copy(dt, H5T_COPY_TRANSIENT)))
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to copy");

    /* Atomize result */
    if((ret_value = H5I_register(H5I_DATATYPE, new_dt, TRUE)) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register datatype atom")

done:
    if(ret_value < 0)
        if(new_dt && H5T_close(new_dt) < 0)
            HDONE_ERROR(H5E_DATATYPE, H5E_CANTRELEASE, FAIL, "unable to release datatype info")

    FUNC_LEAVE_API(ret_value)
} /* end H5Tcopy() */


/*-------------------------------------------------------------------------
 * Function:	H5Tclose
 *
 * Purpose:	Frees a datatype and all associated memory.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tclose(hid_t type_id)
{
    H5T_t	*dt;                    /* Pointer to datatype to close */
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", type_id);

    /* Check args */
    if(NULL == (dt = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    if(H5T_STATE_IMMUTABLE == dt->shared->state)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "immutable datatype")

    /* When the reference count reaches zero the resources are freed */
    if(H5I_dec_app_ref(type_id) < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "problem freeing id")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tclose() */


/*-------------------------------------------------------------------------
 * Function:	H5Tequal
 *
 * Purpose:	Determines if two datatypes are equal.
 *
 * Return:	Success:	TRUE if equal, FALSE if unequal
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		Wednesday, December 10, 1997
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Tequal(hid_t type1_id, hid_t type2_id)
{
    const H5T_t	*dt1;           /* Pointer to first datatype */
    const H5T_t	*dt2;           /* Pointer to second datatype */
    htri_t		ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("t", "ii", type1_id, type2_id);

    /* check args */
    if(NULL == (dt1 = (H5T_t *)H5I_object_verify(type1_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    if(NULL == (dt2 = (H5T_t *)H5I_object_verify(type2_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

    ret_value = (0 == H5T_cmp(dt1, dt2, FALSE)) ? TRUE : FALSE;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tequal() */


/*-------------------------------------------------------------------------
 * Function:	H5Tlock
 *
 * Purpose:	Locks a type, making it read only and non-destructable.	 This
 *		is normally done by the library for predefined datatypes so
 *		the application doesn't inadvertently change or delete a
 *		predefined type.
 *
 *		Once a datatype is locked it can never be unlocked unless
 *		the entire library is closed.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Friday, January	 9, 1998
 *
 * Modifications:
 *
 * 	Robb Matzke, 1 Jun 1998
 *	It is illegal to lock a named datatype since we must allow named
 *	types to be closed (to release file resources) but locking a type
 *	prevents that.
 *-------------------------------------------------------------------------
 */
herr_t
H5Tlock(hid_t type_id)
{
    H5T_t	*dt;                    /* Datatype to operate on */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", type_id);

    /* Check args */
    if(NULL == (dt = (H5T_t *)H5I_object_verify(type_id,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    if(H5T_STATE_NAMED==dt->shared->state || H5T_STATE_OPEN==dt->shared->state)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to lock named datatype")

    if(H5T_lock(dt, TRUE) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to lock transient datatype")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tlock() */


/*-------------------------------------------------------------------------
 * Function:	H5Tget_class
 *
 * Purpose:	Returns the datatype class identifier for datatype TYPE_ID.
 *
 * Return:	Success:	One of the non-negative datatype class
 *				constants.
 *
 *		Failure:	H5T_NO_CLASS (Negative)
 *
 * Programmer:	Robb Matzke
 *		Monday, December  8, 1997
 *
 *-------------------------------------------------------------------------
 */
H5T_class_t
H5Tget_class(hid_t type_id)
{
    H5T_t	*dt;            /* Pointer to datatype */
    H5T_class_t ret_value;      /* Return value */

    FUNC_ENTER_API(H5T_NO_CLASS)
    H5TRACE1("Tt", "i", type_id);

    /* Check args */
    if(NULL == (dt = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_NO_CLASS, "not a datatype")

    /* Set return value */
    ret_value = H5T_get_class(dt, FALSE);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tget_class() */


/*-------------------------------------------------------------------------
 * Function:	H5T_get_class
 *
 * Purpose:	Returns the data type class identifier for a datatype ptr.
 *
 * Return:	Success:	One of the non-negative data type class
 *				constants.
 *
 *		Failure:	H5T_NO_CLASS (Negative)
 *
 * Programmer:	Robb Matzke
 *		Monday, December  8, 1997
 *
 * Modifications:
 *      Broke out from H5Tget_class - QAK - 6/4/99
 *
 *-------------------------------------------------------------------------
 */
H5T_class_t
H5T_get_class(const H5T_t *dt, htri_t internal)
{
    H5T_class_t ret_value;

    FUNC_ENTER_NOAPI(H5T_NO_CLASS)

    HDassert(dt);

    /* Externally, a VL string is a string; internally, a VL string is a VL. */
    if(internal) {
        ret_value=dt->shared->type;
    } else {
        if(H5T_IS_VL_STRING(dt->shared))
            ret_value=H5T_STRING;
        else
            ret_value=dt->shared->type;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5T_get_class() */


/*-------------------------------------------------------------------------
 * Function:	H5Tdetect_class
 *
 * Purpose:	Check whether a datatype contains (or is) a certain type of
 *		datatype.
 *
 * Return:	TRUE (1) or FALSE (0) on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, November 29, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Tdetect_class(hid_t type, H5T_class_t cls)
{
    H5T_t	*dt;            /* Datatype to query */
    htri_t      ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("t", "iTt", type, cls);

    /* Check args */
    if(NULL == (dt = (H5T_t *)H5I_object_verify(type,H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_NO_CLASS, "not a datatype")
    if(!(cls > H5T_NO_CLASS && cls < H5T_NCLASSES))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_NO_CLASS, "not a datatype class")

    /* Set return value */
    if((ret_value = H5T_detect_class(dt, cls, TRUE)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, H5T_NO_CLASS, "can't get datatype class")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tdetect_class() */


/*-------------------------------------------------------------------------
 * Function:	H5T_detect_class
 *
 * Purpose:	Check whether a datatype contains (or is) a certain type of
 *		datatype.
 *
 * Return:	TRUE (1) or FALSE (0) on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, November 29, 2000
 *
 * Modifications:
 *              Raymond Lu
 *              4 December 2009
 *              Added a flag as a parameter to indicate whether the caller is
 *              H5Tdetect_class.  I also added the check for VL string type
 *              just like the public function.  Because we want to tell users
 *              VL string is a string type but we treat it as a VL type
 *              internally, H5T_detect_class needs to know where the caller
 *              is from.
 *-------------------------------------------------------------------------
 */
htri_t
H5T_detect_class(const H5T_t *dt, H5T_class_t cls, hbool_t from_api)
{
    unsigned	i;
    htri_t      ret_value = FALSE;        /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(dt);
    HDassert(cls > H5T_NO_CLASS && cls < H5T_NCLASSES);

    /* Consider VL string as a string for API, as a VL for internal use. */
    /* (note that this check must be performed before checking if the VL
     *  string belongs to the H5T_VLEN class, which would otherwise return
     *  true. -QAK)
     */
    if(from_api && H5T_IS_VL_STRING(dt->shared))
        HGOTO_DONE(H5T_STRING == cls);

    /* Check if this type is the correct type */
    if(dt->shared->type == cls)
        HGOTO_DONE(TRUE);

    /* check for types that might have the correct type as a component */
    switch(dt->shared->type) {
        case H5T_COMPOUND:
            for(i = 0; i < dt->shared->u.compnd.nmembs; i++) {
                htri_t nested_ret;      /* Return value from nested call */

                /* Check if this field's type is the correct type */
                if(dt->shared->u.compnd.memb[i].type->shared->type == cls)
                    HGOTO_DONE(TRUE);

                /* Recurse if it's VL, compound, enum or array */
                if(H5T_IS_COMPLEX(dt->shared->u.compnd.memb[i].type->shared->type))
                    if((nested_ret = H5T_detect_class(dt->shared->u.compnd.memb[i].type, cls, from_api)) != FALSE)
                        HGOTO_DONE(nested_ret);
            } /* end for */
            break;

        case H5T_ARRAY:
        case H5T_VLEN:
        case H5T_ENUM:
            HGOTO_DONE(H5T_detect_class(dt->shared->parent, cls, from_api));

        case H5T_NO_CLASS:
        case H5T_INTEGER:
        case H5T_FLOAT:
        case H5T_TIME:
        case H5T_STRING:
        case H5T_BITFIELD:
        case H5T_OPAQUE:
        case H5T_REFERENCE:
        case H5T_NCLASSES:
        default:
            break;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_detect_class() */


/*-------------------------------------------------------------------------
 * Function:	H5Tis_variable_str
 *
 * Purpose:	Check whether a datatype is a variable-length string
 *
 * Return:	TRUE (1) or FALSE (0) on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *		November 4, 2002
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Tis_variable_str(hid_t dtype_id)
{
    H5T_t	*dt;            /* Datatype to query */
    htri_t      ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("t", "i", dtype_id);

    /* Check args */
    if(NULL == (dt = (H5T_t *)H5I_object_verify(dtype_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

    /* Set return value */
    if((ret_value = H5T_is_variable_str(dt)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "can't determine if datatype is VL-string")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tis_variable_str() */


/*-------------------------------------------------------------------------
 * Function:	H5T_is_variable_str
 *
 * Purpose:	Check whether a datatype is a variable-length string
 *
 * Return:	TRUE (1) or FALSE (0) on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		October 17, 2007
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5T_is_variable_str(const H5T_t *dt)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    FUNC_LEAVE_NOAPI(H5T_IS_VL_STRING(dt->shared))
} /* end H5T_is_variable_str() */


/*-------------------------------------------------------------------------
 * Function:	H5Tget_size
 *
 * Purpose:	Determines the total size of a datatype in bytes.
 *
 * Return:	Success:	Size of the datatype in bytes.	 The size of
 *				datatype is the size of an instance of that
 *				datatype.
 *
 *		Failure:	0 (valid datatypes are never zero size)
 *
 * Programmer:	Robb Matzke
 *		Monday, December  8, 1997
 *
 *-------------------------------------------------------------------------
 */
size_t
H5Tget_size(hid_t type_id)
{
    H5T_t	*dt;            /* Datatype to query */
    size_t	ret_value;      /* Return value */

    FUNC_ENTER_API(0)
    H5TRACE1("z", "i", type_id);

    /* Check args */
    if(NULL == (dt = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not a datatype")

    /* size */
    ret_value = H5T_GET_SIZE(dt);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tget_size() */


/*-------------------------------------------------------------------------
 * Function:	H5Tset_size
 *
 * Purpose:	Sets the total size in bytes for a datatype (this operation
 *		is not permitted on reference datatypes).  If the size is
 *		decreased so that the significant bits of the datatype
 *		extend beyond the edge of the new size, then the `offset'
 *		property is decreased toward zero.  If the `offset' becomes
 *		zero and the significant bits of the datatype still hang
 *		over the edge of the new size, then the number of significant
 *		bits is decreased.
 *
 *		Adjusting the size of an H5T_STRING automatically sets the
 *		precision to 8*size.
 *
 *		All datatypes have a positive size.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_size(hid_t type_id, size_t size)
{
    H5T_t	*dt;                    /* Datatype to modify */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "iz", type_id, size);

    /* Check args */
    if(NULL == (dt = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    if(H5T_STATE_TRANSIENT!=dt->shared->state)
	HGOTO_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "datatype is read-only")
    if(size <= 0 && size != H5T_VARIABLE)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "size must be positive")
    if(size == H5T_VARIABLE && !H5T_IS_STRING(dt->shared))
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "only strings may be variable length")
    if(H5T_ENUM == dt->shared->type && dt->shared->u.enumer.nmembs > 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "operation not allowed after members are defined")
    if(H5T_REFERENCE == dt->shared->type)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "operation not defined for this datatype")

    /* Modify the datatype */
    if(H5T_set_size(dt, size) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to set size for datatype")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tset_size() */


/*-------------------------------------------------------------------------
 * Function:	H5Tget_super
 *
 * Purpose:	Returns the type from which TYPE is derived. In the case of
 *		an enumeration type the return value is an integer type.
 *
 * Return:	Success:	Type ID for base datatype.
 *
 *		Failure:	negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, December 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tget_super(hid_t type)
{
    H5T_t	*dt;            /* Datatype to query */
    H5T_t      *super = NULL;  /* Supertype */
    hid_t	ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("i", "i", type);

    if(NULL == (dt = (H5T_t *)H5I_object_verify(type,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    if(NULL == (super = H5T_get_super(dt)))
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "not a datatype")
    if((ret_value = H5I_register(H5I_DATATYPE, super, TRUE)) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register parent datatype")

done:
    if(ret_value < 0)
        if(super && H5T_close(super) < 0)
            HDONE_ERROR(H5E_DATATYPE, H5E_CANTRELEASE, FAIL, "unable to release super datatype info")

    FUNC_LEAVE_API(ret_value)
} /* end H5Tget_super() */


/*-------------------------------------------------------------------------
 * Function:	H5T_get_super
 *
 * Purpose:	Private function for H5Tget_super.  Returns the type from
 *              which TYPE is derived. In the case of an enumeration type
 *              the return value is an integer type.
 *
 * Return:	Success:	Data type for base data type.
 *
 *		Failure:        NULL
 *
 * Programmer:	Raymond Lu
 *              October 9, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T_get_super(const H5T_t *dt)
{
    H5T_t	*ret_value=NULL;

    FUNC_ENTER_NOAPI(NULL)

    HDassert(dt);

    if (!dt->shared->parent)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "not a derived data type");
    if (NULL==(ret_value=H5T_copy(dt->shared->parent, H5T_COPY_ALL)))
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to copy parent data type");

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5T_register
 *
 * Purpose:	Register a hard or soft conversion function for a data type
 *		conversion path.  The path is specified by the source and
 *		destination data types SRC_ID and DST_ID (for soft functions
 *		only the class of these types is important). If FUNC is a
 *		hard function then it replaces any previous path; if it's a
 *		soft function then it replaces all existing paths to which it
 *		applies and is used for any new path to which it applies as
 *		long as that path doesn't have a hard function.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Friday, January	 9, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T_register(H5T_pers_t pers, const char *name, H5T_t *src, H5T_t *dst,
	    H5T_conv_t func, hid_t dxpl_id, hbool_t api_call)
{
    hid_t	tmp_sid=-1, tmp_did=-1;/*temporary data type IDs	*/
    H5T_path_t	*old_path=NULL;		/*existing conversion path	*/
    H5T_path_t	*new_path=NULL;		/*new conversion path		*/
    H5T_cdata_t	cdata;			/*temporary conversion data	*/
    int	nprint=0;		/*number of paths shut down	*/
    int	i;			/*counter			*/
    herr_t	ret_value=SUCCEED;		/*return value			*/

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(src);
    HDassert(dst);
    HDassert(func);
    HDassert(H5T_PERS_HARD==pers || H5T_PERS_SOFT==pers);
    HDassert(name && *name);

    if(H5T_PERS_HARD == pers) {
        /* Only bother to register the path if it's not a no-op path (for this machine) */
        if(H5T_cmp(src, dst, FALSE)) {
            /* Locate or create a new conversion path */
            if(NULL == (new_path = H5T_path_find(src, dst, name, func, dxpl_id, api_call)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to locate/allocate conversion path")

            /*
             * Notify all other functions to recalculate private data since some
             * functions might cache a list of conversion functions.  For
             * instance, the compound type converter caches a list of conversion
             * functions for the members, so adding a new function should cause
             * the list to be recalculated to use the new function.
             */
            for(i = 0; i < H5T_g.npaths; i++)
                if(new_path != H5T_g.path[i])
                    H5T_g.path[i]->cdata.recalc = TRUE;
	} /* end if */
    } /* end if */
    else {
        /* Add function to end of soft list */
        if((size_t)H5T_g.nsoft >= H5T_g.asoft) {
            size_t na = MAX(32, 2 * H5T_g.asoft);
            H5T_soft_t *x;

            if(NULL == (x = (H5T_soft_t *)H5MM_realloc(H5T_g.soft, na * sizeof(H5T_soft_t))))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
            H5T_g.asoft = na;
            H5T_g.soft = x;
        } /* end if */
        HDstrncpy(H5T_g.soft[H5T_g.nsoft].name, name, (size_t)H5T_NAMELEN);
        H5T_g.soft[H5T_g.nsoft].name[H5T_NAMELEN - 1] = '\0';
        H5T_g.soft[H5T_g.nsoft].src = src->shared->type;
        H5T_g.soft[H5T_g.nsoft].dst = dst->shared->type;
        H5T_g.soft[H5T_g.nsoft].func = func;
        H5T_g.nsoft++;

        /*
         * Any existing path (except the no-op path) to which this new soft
         * conversion function applies should be replaced by a new path that
         * uses this function.
         */
        for (i=1; i<H5T_g.npaths; i++) {
            old_path = H5T_g.path[i];
            HDassert(old_path);

            /* Does the new soft conversion function apply to this path? */
            if (old_path->is_hard ||
                    old_path->src->shared->type!=src->shared->type ||
                    old_path->dst->shared->type!=dst->shared->type) {
                continue;
            }
            if((tmp_sid = H5I_register(H5I_DATATYPE, H5T_copy(old_path->src, H5T_COPY_ALL), FALSE)) < 0 ||
                    (tmp_did = H5I_register(H5I_DATATYPE, H5T_copy(old_path->dst, H5T_COPY_ALL), FALSE)) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register data types for conv query")
            HDmemset(&cdata, 0, sizeof cdata);
            cdata.command = H5T_CONV_INIT;
            if((func)(tmp_sid, tmp_did, &cdata, (size_t)0, (size_t)0, (size_t)0,
                    NULL, NULL, dxpl_id) < 0) {
                H5I_dec_ref(tmp_sid);
                H5I_dec_ref(tmp_did);
                tmp_sid = tmp_did = -1;
                H5E_clear_stack(NULL);
                continue;
            } /* end if */

            /* Create a new conversion path */
            if(NULL == (new_path = H5FL_CALLOC(H5T_path_t)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
            HDstrncpy(new_path->name, name, (size_t)H5T_NAMELEN);
            new_path->name[H5T_NAMELEN - 1] = '\0';
            if(NULL == (new_path->src = H5T_copy(old_path->src, H5T_COPY_ALL)) ||
                    NULL == (new_path->dst=H5T_copy(old_path->dst, H5T_COPY_ALL)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to copy data types")
            new_path->func = func;
            new_path->is_hard = FALSE;
            new_path->cdata = cdata;

            /* Replace previous path */
            H5T_g.path[i] = new_path;
            new_path = NULL; /*so we don't free it on error*/

            /* Free old path */
            H5T__print_stats(old_path, &nprint);
            old_path->cdata.command = H5T_CONV_FREE;
            if ((old_path->func)(tmp_sid, tmp_did, &(old_path->cdata),
                    (size_t)0, (size_t)0, (size_t)0, NULL, NULL, dxpl_id)<0) {
#ifdef H5T_DEBUG
		if (H5DEBUG(T)) {
		    fprintf (H5DEBUG(T), "H5T: conversion function 0x%08lx "
			     "failed to free private data for %s (ignored)\n",
			     (unsigned long)(old_path->func), old_path->name);
		}
#endif
            } /* end if */
            H5T_close(old_path->src);
            H5T_close(old_path->dst);
            old_path = H5FL_FREE(H5T_path_t, old_path);

            /* Release temporary atoms */
            H5I_dec_ref(tmp_sid);
            H5I_dec_ref(tmp_did);
            tmp_sid = tmp_did = -1;

            /* We don't care about any failures during the freeing process */
	    H5E_clear_stack(NULL);
        } /* end for */
    } /* end else */

done:
    if(ret_value < 0) {
	if(new_path) {
	    if(new_path->src)
                H5T_close(new_path->src);
	    if(new_path->dst)
                H5T_close(new_path->dst);
            new_path = H5FL_FREE(H5T_path_t, new_path);
	} /* end if */
	if(tmp_sid >= 0)
            H5I_dec_ref(tmp_sid);
	if(tmp_did >= 0)
            H5I_dec_ref(tmp_did);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_register() */


/*-------------------------------------------------------------------------
 * Function:	H5Tregister
 *
 * Purpose:	Register a hard or soft conversion function for a data type
 *		conversion path.  The path is specified by the source and
 *		destination data types SRC_ID and DST_ID (for soft functions
 *		only the class of these types is important). If FUNC is a
 *		hard function then it replaces any previous path; if it's a
 *		soft function then it replaces all existing paths to which it
 *		applies and is used for any new path to which it applies as
 *		long as that path doesn't have a hard function.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Friday, January	 9, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tregister(H5T_pers_t pers, const char *name, hid_t src_id, hid_t dst_id,
	    H5T_conv_t func)
{
    H5T_t	*src;		        /*source data type descriptor	*/
    H5T_t	*dst;		        /*destination data type desc	*/
    herr_t	ret_value = SUCCEED;	/*return value			*/

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "Te*siix", pers, name, src_id, dst_id, func);

    /* Check args */
    if(H5T_PERS_HARD != pers && H5T_PERS_SOFT != pers)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid function persistence")
    if(!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "conversion must have a name for debugging")
    if(NULL == (src = (H5T_t *)H5I_object_verify(src_id,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type")
    if(NULL == (dst = (H5T_t *)H5I_object_verify(dst_id,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type")
    if(!func)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no conversion function specified")

    /* Go register the function */
    if(H5T_register(pers, name, src, dst, func, H5AC_ind_dxpl_id, TRUE) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "can't register conversion function")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tregister() */


/*-------------------------------------------------------------------------
 * Function:	H5T_unregister
 *
 * Purpose:	Removes conversion paths that match the specified criteria.
 *		All arguments are optional. Missing arguments are wild cards.
 *		The special no-op path cannot be removed.
 *
 * Return:	Succeess:	non-negative
 *
 * 		Failure:	negative
 *
 * Programmer:	Robb Matzke
 *		Tuesday, January 13, 1998
 *
 * Modifications:
 *      Adapted to non-API function - QAK, 11/17/99
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T_unregister(H5T_pers_t pers, const char *name, H5T_t *src, H5T_t *dst,
	      H5T_conv_t func, hid_t dxpl_id)
{
    H5T_path_t	*path = NULL;		/*conversion path		*/
    H5T_soft_t	*soft = NULL;		/*soft conversion information	*/
    int	nprint = 0;		/*number of paths shut down	*/
    int	i;			/*counter			*/

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Remove matching entries from the soft list */
    if(H5T_PERS_DONTCARE == pers || H5T_PERS_SOFT == pers) {
        for(i = H5T_g.nsoft - 1; i >= 0; --i) {
            soft = H5T_g.soft + i;
            HDassert(soft);
            if(name && *name && HDstrcmp(name, soft->name))
                continue;
            if(src && src->shared->type != soft->src)
                continue;
            if(dst && dst->shared->type != soft->dst)
                continue;
            if(func && func != soft->func)
                continue;

            HDmemmove(H5T_g.soft + i, H5T_g.soft + i + 1, (size_t)(H5T_g.nsoft - (i + 1)) * sizeof(H5T_soft_t));
            --H5T_g.nsoft;
        } /* end for */
    } /* end if */

    /* Remove matching conversion paths, except no-op path */
    for(i = H5T_g.npaths - 1; i > 0; --i) {
        path = H5T_g.path[i];
        HDassert(path);

        /* Not a match */
        if(((H5T_PERS_SOFT == pers && path->is_hard) ||
                    (H5T_PERS_HARD == pers && !path->is_hard)) ||
                (name && *name && HDstrcmp(name, path->name)) ||
                (src && H5T_cmp(src, path->src, FALSE)) ||
                (dst && H5T_cmp(dst, path->dst, FALSE)) ||
                (func && func!=path->func)) {
            /*
             * Notify all other functions to recalculate private data since some
             * functions might cache a list of conversion functions.  For
             * instance, the compound type converter caches a list of conversion
             * functions for the members, so removing a function should cause
             * the list to be recalculated to avoid the removed function.
             */
            path->cdata.recalc = TRUE;
        } /* end if */
        else {
            /* Remove from table */
            HDmemmove(H5T_g.path + i, H5T_g.path + i + 1, (size_t)(H5T_g.npaths - (i + 1)) * sizeof(H5T_path_t*));
            --H5T_g.npaths;

            /* Shut down path */
            H5T__print_stats(path, &nprint);
            path->cdata.command = H5T_CONV_FREE;
            if((path->func)(FAIL, FAIL, &(path->cdata),
                    (size_t)0, (size_t)0, (size_t)0, NULL, NULL, dxpl_id) < 0) {
#ifdef H5T_DEBUG
                if(H5DEBUG(T)) {
                    fprintf(H5DEBUG(T), "H5T: conversion function 0x%08lx failed "
                            "to free private data for %s (ignored)\n",
                            (unsigned long)(path->func), path->name);
                }
#endif
            }
            H5T_close(path->src);
            H5T_close(path->dst);
            path = H5FL_FREE(H5T_path_t, path);
            H5E_clear_stack(NULL); /*ignore all shutdown errors*/
        } /* end else */
    } /* end for */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5T_unregister() */


/*-------------------------------------------------------------------------
 * Function:	H5Tunregister
 *
 * Purpose:	Removes conversion paths that match the specified criteria.
 *		All arguments are optional. Missing arguments are wild cards.
 *		The special no-op path cannot be removed.
 *
 * Return:	Succeess:	non-negative
 *
 * 		Failure:	negative
 *
 * Programmer:	Robb Matzke
 *		Tuesday, January 13, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tunregister(H5T_pers_t pers, const char *name, hid_t src_id, hid_t dst_id,
	      H5T_conv_t func)
{
    H5T_t	*src = NULL, *dst = NULL;	/* Datatype descriptors	*/
    herr_t      ret_value = SUCCEED;            /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "Te*siix", pers, name, src_id, dst_id, func);

    /* Check arguments */
    if(src_id > 0 && (NULL == (src = (H5T_t *)H5I_object_verify(src_id, H5I_DATATYPE))))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "src is not a data type")
    if(dst_id > 0 && (NULL == (dst = (H5T_t *)H5I_object_verify(dst_id, H5I_DATATYPE))))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dst is not a data type")

    if(H5T_unregister(pers, name, src, dst, func, H5AC_ind_dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTDELETE, FAIL, "internal unregister function failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tunregister() */


/*-------------------------------------------------------------------------
 * Function:	H5Tfind
 *
 * Purpose:	Finds a conversion function that can handle a conversion from
 *		type SRC_ID to type DST_ID.  The PCDATA argument is a pointer
 *		to a pointer to type conversion data which was created and
 *		initialized by the type conversion function of this path
 *		when the conversion function was installed on the path.
 *
 * Return:	Success:	A pointer to a suitable conversion function.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		Tuesday, January 13, 1998
 *
 *-------------------------------------------------------------------------
 */
H5T_conv_t
H5Tfind(hid_t src_id, hid_t dst_id, H5T_cdata_t **pcdata)
{
    H5T_t	*src, *dst;
    H5T_path_t	*path;
    H5T_conv_t	ret_value;      /* Return value */

    FUNC_ENTER_API(NULL)
    H5TRACE3("x", "ii**x", src_id, dst_id, pcdata);

    /* Check args */
    if(NULL == (src = (H5T_t *)H5I_object_verify(src_id, H5I_DATATYPE)) ||
            NULL == (dst = (H5T_t *)H5I_object_verify(dst_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a data type")
    if(!pcdata)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "no address to receive cdata pointer")

    /* Find it */
    if(NULL == (path = H5T_path_find(src, dst, NULL, NULL, H5AC_ind_dxpl_id, FALSE)))
	HGOTO_ERROR(H5E_DATATYPE, H5E_NOTFOUND, NULL, "conversion function not found")

    if(pcdata)
        *pcdata = &(path->cdata);

    /* Set return value */
    ret_value = path->func;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tfind() */


/*-------------------------------------------------------------------------
 * Function:	H5Tcompiler_conv
 *
 * Purpose:	Finds out whether the library's conversion function from
 *              type src_id to type dst_id is a compiler (hard) conversion.
 *              A hard conversion uses compiler's casting; a soft conversion
 *              uses the library's own conversion function.
 *
 * Return:	TRUE:           hard conversion.
 *		FALSE:          soft conversion.
 *		FAIL:           failed.
 *
 * Programmer:	Raymond Lu
 *		Friday, Sept 2, 2005
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Tcompiler_conv(hid_t src_id, hid_t dst_id)
{
    H5T_t	*src, *dst;
    htri_t	ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("t", "ii", src_id, dst_id);

    /* Check args */
    if(NULL == (src = (H5T_t *)H5I_object_verify(src_id, H5I_DATATYPE)) ||
            NULL == (dst = (H5T_t *)H5I_object_verify(dst_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type")

    /* Find it */
    if((ret_value = H5T_compiler_conv(src, dst)) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_NOTFOUND, FAIL, "conversion function not found")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tcompiler_conv() */


/*-------------------------------------------------------------------------
 * Function:	H5Tconvert
 *
 * Purpose:	Convert NELMTS elements from type SRC_ID to type DST_ID.  The
 *		source elements are packed in BUF and on return the
 *		destination will be packed in BUF.  That is, the conversion
 *		is performed in place.  The optional background buffer is an
 *		array of NELMTS values of destination type which are merged
 *		with the converted values to fill in cracks (for instance,
 *		BACKGROUND might be an array of structs with the `a' and `b'
 *		fields already initialized and the conversion of BUF supplies
 *		the `c' and `d' field values).  The PLIST_ID a dataset transfer
 *      property list which is passed to the conversion functions.  (It's
 *      currently only used to pass along the VL datatype custom allocation
 *      information -QAK 7/1/99)
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Wednesday, June 10, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tconvert(hid_t src_id, hid_t dst_id, size_t nelmts, void *buf,
	    void *background, hid_t dxpl_id)
{
    H5T_path_t	*tpath;			/*type conversion info	*/
    H5T_t	*src, *dst;		/*unatomized types	*/
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "iiz*x*xi", src_id, dst_id, nelmts, buf, background, dxpl_id);

    /* Check args */
    if(NULL == (src = (H5T_t *)H5I_object_verify(src_id, H5I_DATATYPE)) ||
            NULL == (dst = (H5T_t *)H5I_object_verify(dst_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type")
    if(H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else
        if(TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not dataset transfer property list")

    /* Find the conversion function */
    if(NULL == (tpath = H5T_path_find(src, dst, NULL, NULL, dxpl_id, FALSE)))
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to convert between src and dst data types")

    if(H5T_convert(tpath, src_id, dst_id, nelmts, (size_t)0, (size_t)0, buf, background, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "data type conversion failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tconvert() */


/*-------------------------------------------------------------------------
 * Function:	H5Tencode
 *
 * Purpose:	Given an datatype ID, converts the object description into
 *              binary in a buffer.
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Raymond Lu
 *              slu@ncsa.uiuc.edu
 *              July 14, 2004
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tencode(hid_t obj_id, void *buf, size_t *nalloc)
{
    H5T_t       *dtype;
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*x*z", obj_id, buf, nalloc);

    /* Check argument and retrieve object */
    if(NULL == (dtype = (H5T_t *)H5I_object_verify(obj_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    if(nalloc == NULL)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL pointer for buffer size")

    /* Go encode the datatype */
    if(H5T_encode(dtype, (unsigned char *)buf, nalloc) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTENCODE, FAIL, "can't encode datatype")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tencode() */


/*-------------------------------------------------------------------------
 * Function:	H5Tdecode
 *
 * Purpose:	Decode a binary object description and return a new object
 *              handle.
 *
 * Return:	Success:	datatype ID(non-negative)
 *
 *		Failure:	negative
 *
 * Programmer:	Raymond Lu
 *              slu@ncsa.uiuc.edu
 *              July 14, 2004
 *
 * Modification:Raymond Lu
 *              songyulu@hdfgroup.org
 *              17 February 2011
 *              I changed the value for the APP_REF parameter of H5I_register
 *              from FALSE to TRUE. 
 *-------------------------------------------------------------------------
 */
hid_t
H5Tdecode(const void *buf)
{
    H5T_t       *dt;
    hid_t       ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("i", "*x", buf);

    /* Check args */
    if(buf == NULL)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "empty buffer")

    /* Create datatype by decoding buffer */
    if(NULL == (dt = H5T_decode((const unsigned char *)buf)))
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTDECODE, FAIL, "can't decode object")

    /* Register the type and return the ID */
    if((ret_value = H5I_register(H5I_DATATYPE, dt, TRUE)) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register data type")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tdecode() */

/*-------------------------------------------------------------------------
 * API functions are above; library-private functions are below...
 *-------------------------------------------------------------------------
 */


/*-------------------------------------------------------------------------
 * Function:	H5T_encode
 *
 * Purpose:	Private function for H5Tencode.  Converts an object
 *              description into binary in a buffer.
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Raymond Lu
 *              slu@ncsa.uiuc.edu
 *              July 14, 2004
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T_encode(H5T_t *obj, unsigned char *buf, size_t *nalloc)
{
    size_t      buf_size;               /* Encoded size of datatype */
    H5F_t       *f = NULL;              /* Fake file structure*/
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

    /* Allocate "fake" file structure */
    if(NULL == (f = H5F_fake_alloc((uint8_t)0)))
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTALLOC, FAIL, "can't allocate fake file struct")

    /* Find out the size of buffer needed */
    if((buf_size = H5O_msg_raw_size(f, H5O_DTYPE_ID, TRUE, obj)) == 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_BADSIZE, FAIL, "can't find datatype size")

    /* Don't encode if buffer size isn't big enough or buffer is empty */
    if(!buf || *nalloc < (buf_size + 1 + 1))
        *nalloc = buf_size + 1 + 1;
    else {
        /* Encode the type of the information */
        *buf++ = H5O_DTYPE_ID;

        /* Encode the version of the dataspace information */
        *buf++ = H5T_ENCODE_VERSION;

        /* Encode into user's buffer */
        if(H5O_msg_encode(f, H5O_DTYPE_ID, TRUE, buf, obj) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTENCODE, FAIL, "can't encode object")
    } /* end else */

done:
    /* Release fake file structure */
    if(f && H5F_fake_free(f) < 0)
        HDONE_ERROR(H5E_DATATYPE, H5E_CANTRELEASE, FAIL, "unable to release fake file struct")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_encode() */


/*-------------------------------------------------------------------------
 * Function:	H5T_decode
 *
 * Purpose:	Private function for H5Tdecode.  Reconstructs a binary
 *              description of datatype and returns a new object handle.
 *
 * Return:	Success:	datatype ID(non-negative)
 *
 *		Failure:	negative
 *
 * Programmer:	Raymond Lu
 *              slu@ncsa.uiuc.edu
 *              July 14, 2004
 *
 *-------------------------------------------------------------------------
 */
static H5T_t *
H5T_decode(const unsigned char *buf)
{
    H5F_t       *f = NULL;      /* Fake file structure*/
    H5T_t       *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Allocate "fake" file structure */
    if(NULL == (f = H5F_fake_alloc((uint8_t)0)))
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTALLOC, NULL, "can't allocate fake file struct")

    /* Decode the type of the information */
    if(*buf++ != H5O_DTYPE_ID)
	HGOTO_ERROR(H5E_DATATYPE, H5E_BADMESG, NULL, "not an encoded datatype")

    /* Decode the version of the datatype information */
    if(*buf++ != H5T_ENCODE_VERSION)
	HGOTO_ERROR(H5E_DATATYPE, H5E_VERSION, NULL, "unknown version of encoded datatype")

    /* Decode the serialized datatype message */
    if(NULL == (ret_value = (H5T_t *)H5O_msg_decode(f, H5AC_dxpl_id, NULL, H5O_DTYPE_ID, buf)))
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTDECODE, NULL, "can't decode object")

    /* Mark datatype as being in memory now */
    if(H5T_set_loc(ret_value, NULL, H5T_LOC_MEMORY) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "invalid datatype location")

done:
    /* Release fake file structure */
    if(f && H5F_fake_free(f) < 0)
        HDONE_ERROR(H5E_DATATYPE, H5E_CANTRELEASE, NULL, "unable to release fake file struct")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_decode() */


/*-------------------------------------------------------------------------
 * Function:	H5T__create
 *
 * Purpose:	Creates a new data type and initializes it to reasonable
 *		values.	 The new data type is SIZE bytes and an instance of
 *		the class TYPE.
 *
 * Return:	Success:	Pointer to the new type.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		Friday, December  5, 1997
 *
 * Modifications:
 *              Raymond Lu
 *              19 May 2011
 *              We support fixed size or variable-length string now.
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T__create(H5T_class_t type, size_t size)
{
    H5T_t  *dt = NULL;
    H5T_t  *ret_value = NULL;

    FUNC_ENTER_PACKAGE

    switch(type) {
        case H5T_INTEGER:
        case H5T_FLOAT:
        case H5T_TIME:
        case H5T_STRING:
            {
                H5T_t *origin_dt = NULL;

		if(NULL == (origin_dt = (H5T_t *)H5I_object(H5T_C_S1)))
		    HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, NULL, "can't get structure for string type")

		/* Copy the default string datatype */
		if(NULL == (dt = H5T_copy(origin_dt, H5T_COPY_TRANSIENT)))
		    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to copy");

		/* Modify the datatype */
		if(H5T_set_size(dt, size) < 0)
		    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to set size for string type")
            }
            break;

        case H5T_BITFIELD:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, NULL, "type class is not appropriate - use H5Tcopy()")

        case H5T_OPAQUE:
        case H5T_COMPOUND:
            if(NULL == (dt = H5T__alloc()))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
            dt->shared->type = type;

            if(type == H5T_COMPOUND) {
                dt->shared->u.compnd.packed=FALSE;       /* Start out unpacked */
                dt->shared->u.compnd.memb_size=0;
            } /* end if */
            else if(type == H5T_OPAQUE)
                /* Initialize the tag in case it's not set later.  A null tag will
                 * cause problems for later operations. */
                dt->shared->u.opaque.tag = H5MM_strdup("");
            break;

        case H5T_ENUM:
            {
                hid_t  subtype;
                H5T_t  *sub_t_obj;

                if(sizeof(char) == size)
                    subtype = H5T_NATIVE_SCHAR_g;
                else if(sizeof(short) == size)
                    subtype = H5T_NATIVE_SHORT_g;
                else if(sizeof(int) == size)
                    subtype = H5T_NATIVE_INT_g;
                else if(sizeof(long) == size)
                    subtype = H5T_NATIVE_LONG_g;
                else if(sizeof(long long) == size)
                    subtype = H5T_NATIVE_LLONG_g;
                else
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "no applicable native integer type")
                if(NULL == (dt = H5T__alloc()))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
                dt->shared->type = type;
                if(NULL == (sub_t_obj = (H5T_t *)H5I_object(subtype)))
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, NULL, "unable to get datatype object")
                if(NULL == (dt->shared->parent = H5T_copy(sub_t_obj, H5T_COPY_ALL)))
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, NULL, "unable to copy base datatype")
            }
            break;

        case H5T_VLEN:  /* Variable length datatype */
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, NULL, "base type required - use H5Tvlen_create()")

        case H5T_ARRAY:  /* Array datatype */
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, NULL, "base type required - use H5Tarray_create2()")

        case H5T_NO_CLASS:
        case H5T_REFERENCE:
        case H5T_NCLASSES:
        default:
            HGOTO_ERROR(H5E_INTERNAL, H5E_UNSUPPORTED, NULL, "unknown data type class")
    } /* end switch */

    /* Set the size except VL string */
    if(H5T_STRING != type || H5T_VARIABLE != size)
        dt->shared->size = size;

    /* Set return value */
    ret_value = dt;

done:
    if(NULL == ret_value) {
        if(dt) {
            dt->shared = H5FL_FREE(H5T_shared_t, dt->shared);
            dt = H5FL_FREE(H5T_t, dt);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__create() */


/*-------------------------------------------------------------------------
 * Function:	H5T_copy
 *
 * Purpose:	Copies datatype OLD_DT.	 The resulting data type is not
 *		locked and is a transient type.
 *
 * Return:	Success:	Pointer to a new copy of the OLD_DT argument.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		Thursday, December  4, 1997
 *
 * Modifications:
 *
 * 	Robb Matzke, 4 Jun 1998
 *	Added the METHOD argument.  If it's H5T_COPY_TRANSIENT then the
 *	result will be an unlocked transient type.  Otherwise if it's
 *	H5T_COPY_ALL then the result is a named type if the original is a
 *	named type, but the result is not opened.  Finally, if it's
 *	H5T_COPY_REOPEN and the original type is a named type then the result
 *	is a named type and the type object header is opened again.  The
 *	H5T_COPY_REOPEN method is used when returning a named type to the
 *	application.
 *
 * 	Robb Matzke, 22 Dec 1998
 *	Now able to copy enumeration data types.
 *
 *      Robb Matzke, 20 May 1999
 *	Now able to copy opaque types.
 *
 *      Pedro Vicente, <pvn@ncsa.uiuc.edu> 21 Sep 2002
 *      Added a deep copy of the symbol table entry
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T_copy(H5T_t *old_dt, H5T_copy_t method)
{
    H5T_t	*new_dt = NULL, *tmp = NULL;
    H5T_shared_t    *reopened_fo = NULL;
    unsigned	i;
    char	*s;
    H5T_t	*ret_value;

    FUNC_ENTER_NOAPI(NULL)

    /* check args */
    HDassert(old_dt);

    /* Allocate space */
    if(NULL == (new_dt = H5FL_MALLOC(H5T_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    if(NULL == (new_dt->shared = H5FL_MALLOC(H5T_shared_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Copy shared information (entry information is copied last) */
    *(new_dt->shared) = *(old_dt->shared);

    /* Check what sort of copy we are making */
    switch (method) {
        case H5T_COPY_TRANSIENT:
            /*
             * Return an unlocked transient type.
             */
            new_dt->shared->state = H5T_STATE_TRANSIENT;
            break;

        case H5T_COPY_ALL:
            /*
             * Return a transient type (locked or unlocked) or an unopened named
             * type.  Immutable transient types are degraded to read-only.
             */
            if(H5T_STATE_OPEN==old_dt->shared->state)
                new_dt->shared->state = H5T_STATE_NAMED;
            else if(H5T_STATE_IMMUTABLE==old_dt->shared->state)
                new_dt->shared->state = H5T_STATE_RDONLY;
            break;

        case H5T_COPY_REOPEN:
            /*
             * Return a transient type (locked or unlocked) or an opened named
             * type.  Immutable transient types are degraded to read-only.
             */
            if(old_dt->sh_loc.type == H5O_SHARE_TYPE_COMMITTED) {
                /* Check if the object is already open */
                if(NULL == (reopened_fo = (H5T_shared_t *)H5FO_opened(old_dt->sh_loc.file, old_dt->sh_loc.u.loc.oh_addr))) {
                    /* Clear any errors from H5FO_opened() */
                    H5E_clear_stack(NULL);

                    /* Open named datatype again */
                    if(H5O_open(&old_dt->oloc) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTOPENOBJ, NULL, "unable to reopen named data type");

                    /* Insert opened named datatype into opened object list for the file */
                    if(H5FO_insert(old_dt->sh_loc.file, old_dt->sh_loc.u.loc.oh_addr, new_dt->shared, FALSE)<0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINSERT, NULL, "can't insert datatype into list of open objects")

                    /* Increment object count for the object in the top file */
                    if(H5FO_top_incr(old_dt->sh_loc.file, old_dt->sh_loc.u.loc.oh_addr) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINC, NULL, "can't increment object count")

                    new_dt->shared->fo_count = 1;
                } /* end if */
                else {
                    /* The object is already open.  Free the H5T_shared_t struct
                     * we had been using and use the one that already exists.
                     * Not terribly efficient. */
                    new_dt->shared = H5FL_FREE(H5T_shared_t, new_dt->shared);
                    new_dt->shared = reopened_fo;

                    reopened_fo->fo_count++;

                    /* Check if the object has been opened through the top file yet */
                    if(H5FO_top_count(old_dt->sh_loc.file, old_dt->sh_loc.u.loc.oh_addr) == 0) {
                        /* Open the object through this top file */
                        if(H5O_open(&old_dt->oloc) < 0)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTOPENOBJ, NULL, "unable to open object header")
                    } /* end if */

                    /* Increment object count for the object in the top file */
                    if(H5FO_top_incr(old_dt->sh_loc.file, old_dt->sh_loc.u.loc.oh_addr) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINC, NULL, "can't increment object count")
                } /* end else */
                new_dt->shared->state = H5T_STATE_OPEN;
            } else if(H5T_STATE_IMMUTABLE == old_dt->shared->state) {
                new_dt->shared->state = H5T_STATE_RDONLY;
            }
            break;
        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, NULL, "invalid copy method type")
    } /* end switch */

    /* Update fields in the new struct, if we aren't sharing an already opened
     * committed datatype */
    if(!reopened_fo) {
        /* Copy parent information */
        if(old_dt->shared->parent)
            new_dt->shared->parent = H5T_copy(old_dt->shared->parent, method);

        switch(new_dt->shared->type) {
            case H5T_COMPOUND:
                {
                int accum_change = 0;    /* Amount of change in the offset of the fields */

                /*
                * Copy all member fields to new type, then overwrite the
                * name and type fields of each new member with copied values.
                * That is, H5T_copy() is a deep copy.
                */
                /* Only malloc if space has been allocated for members - NAF */
                if(new_dt->shared->u.compnd.nalloc > 0) {
                    new_dt->shared->u.compnd.memb = (H5T_cmemb_t *)H5MM_malloc(new_dt->shared->u.compnd.nalloc *
                                        sizeof(H5T_cmemb_t));
                    if (NULL==new_dt->shared->u.compnd.memb)
                        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

                    HDmemcpy(new_dt->shared->u.compnd.memb, old_dt->shared->u.compnd.memb,
                        new_dt->shared->u.compnd.nmembs * sizeof(H5T_cmemb_t));
                } /* end if */

                for(i = 0; i < new_dt->shared->u.compnd.nmembs; i++) {
                    unsigned	j;
                    int    old_match;

                    s = new_dt->shared->u.compnd.memb[i].name;
                    new_dt->shared->u.compnd.memb[i].name = H5MM_xstrdup(s);
                    tmp = H5T_copy (old_dt->shared->u.compnd.memb[i].type, method);
                    new_dt->shared->u.compnd.memb[i].type = tmp;
                    HDassert(tmp != NULL);

                    /* Apply the accumulated size change to the offset of the field */
                    new_dt->shared->u.compnd.memb[i].offset += accum_change;

                    if(old_dt->shared->u.compnd.sorted != H5T_SORT_VALUE) {
                        for(old_match = -1, j = 0; j < old_dt->shared->u.compnd.nmembs; j++) {
                            if(!HDstrcmp(new_dt->shared->u.compnd.memb[i].name, old_dt->shared->u.compnd.memb[j].name)) {
                                old_match = j;
                                break;
                            } /* end if */
                        } /* end for */

                        /* check if we couldn't find a match */
                        if(old_match < 0)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, NULL, "fields in datatype corrupted");
                    } /* end if */
                    else
                        old_match = i;

                    /* If the field changed size, add that change to the accumulated size change */
                    if(new_dt->shared->u.compnd.memb[i].type->shared->size != old_dt->shared->u.compnd.memb[old_match].type->shared->size) {
                        /* Adjust the size of the member */
                        new_dt->shared->u.compnd.memb[i].size = (old_dt->shared->u.compnd.memb[old_match].size*tmp->shared->size)/old_dt->shared->u.compnd.memb[old_match].type->shared->size;

                        accum_change += (new_dt->shared->u.compnd.memb[i].type->shared->size - old_dt->shared->u.compnd.memb[old_match].type->shared->size);
                    } /* end if */
                } /* end for */

                /* Apply the accumulated size change to the size of the compound struct */
                new_dt->shared->size += accum_change;

                }
                break;

            case H5T_ENUM:
                /*
                * Copy all member fields to new type, then overwrite the name fields
                * of each new member with copied values. That is, H5T_copy() is a
                * deep copy.
                */
                new_dt->shared->u.enumer.name = (char **)H5MM_malloc(new_dt->shared->u.enumer.nalloc *
                                    sizeof(char*));
                new_dt->shared->u.enumer.value = (uint8_t *)H5MM_malloc(new_dt->shared->u.enumer.nalloc *
                                    new_dt->shared->size);
                if(NULL == new_dt->shared->u.enumer.value)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
                HDmemcpy(new_dt->shared->u.enumer.value, old_dt->shared->u.enumer.value,
                    new_dt->shared->u.enumer.nmembs * new_dt->shared->size);
                for(i = 0; i < new_dt->shared->u.enumer.nmembs; i++) {
                    s = old_dt->shared->u.enumer.name[i];
                    new_dt->shared->u.enumer.name[i] = H5MM_xstrdup(s);
                } /* end for */
                break;

            case H5T_VLEN:
            case H5T_REFERENCE:
                if(method == H5T_COPY_TRANSIENT || method == H5T_COPY_REOPEN) {
                    /* H5T_copy converts any type into a memory type */
                    if(H5T_set_loc(new_dt, NULL, H5T_LOC_MEMORY) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "invalid datatype location");
                } /* end if */
                break;

            case H5T_OPAQUE:
                /*
                * Copy the tag name.
                */
                new_dt->shared->u.opaque.tag = H5MM_xstrdup(new_dt->shared->u.opaque.tag);
                break;

            case H5T_ARRAY:
                /* Re-compute the array's size, in case it's base type changed size */
                new_dt->shared->size=new_dt->shared->u.array.nelem*new_dt->shared->parent->shared->size;
                break;

            default:
                break;
        } /* end switch */
    } /* end if */

    /* Set the cached location & name path if the original type was a named
     * type and the new type is also named.
     */
    if(H5O_loc_reset(&new_dt->oloc) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTRESET, NULL, "unable to initialize location")
    if(H5G_name_reset(&new_dt->path) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTOPENOBJ, NULL, "unable to reset path")

    if(new_dt->shared->state == H5T_STATE_NAMED || new_dt->shared->state == H5T_STATE_OPEN) {
        if(H5O_loc_copy(&(new_dt->oloc), &(old_dt->oloc), H5_COPY_DEEP) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, NULL, "can't copy object location")
        if(H5G_name_copy(&(new_dt->path), &(old_dt->path), H5_COPY_DEEP) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTOPENOBJ, NULL, "unable to copy path")
    } /* end if */

    /* Copy shared location information if the new type is named or if it is
     * shared in the heap.
     */
    if((old_dt->sh_loc.type == H5O_SHARE_TYPE_SOHM || old_dt->sh_loc.type == H5O_SHARE_TYPE_HERE) ||
            new_dt->shared->state == H5T_STATE_NAMED || new_dt->shared->state == H5T_STATE_OPEN) {
        if(H5O_set_shared(&(new_dt->sh_loc), &(old_dt->sh_loc)) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, NULL, "can't copy shared information")
    } /* end if */
    else
        /* Reset shared component info */
        H5O_msg_reset_share(H5O_DTYPE_ID, new_dt);

    /* Set return value */
    ret_value = new_dt;

done:
    if(ret_value == NULL) {
        if(new_dt) {
            if(new_dt->shared)
                new_dt->shared = H5FL_FREE(H5T_shared_t, new_dt->shared);
            new_dt = H5FL_FREE(H5T_t, new_dt);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_copy() */


/*-------------------------------------------------------------------------
 * Function:	H5T_lock
 *
 * Purpose:	Lock a transient data type making it read-only.  If IMMUTABLE
 *		is set then the type cannot be closed except when the library
 *		itself closes.
 *
 *		This function is a no-op if the type is not transient or if
 *		the type is already read-only or immutable.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, June  4, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_lock (H5T_t *dt, hbool_t immutable)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert (dt);

    switch (dt->shared->state) {
        case H5T_STATE_TRANSIENT:
            dt->shared->state = immutable ? H5T_STATE_IMMUTABLE : H5T_STATE_RDONLY;
            break;
        case H5T_STATE_RDONLY:
            if (immutable) dt->shared->state = H5T_STATE_IMMUTABLE;
            break;
        case H5T_STATE_IMMUTABLE:
        case H5T_STATE_NAMED:
        case H5T_STATE_OPEN:
            /*void*/
            break;
        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "invalid datatype state")
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5T__alloc
 *
 * Purpose:	Allocates a new H5T_t structure, initializing it correctly.
 *
 * Return:	Pointer to new H5T_t on success/NULL on failure
 *
 * Programmer:	Quincey Koziol
 *		Monday, August 29, 2005
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T__alloc(void)
{
    H5T_t *dt = NULL;           /* Pointer to datatype allocated */
    H5T_t *ret_value;           /* Return value */

    FUNC_ENTER_PACKAGE

    /* Allocate & initialize datatype wrapper info */
    if(NULL == (dt = H5FL_CALLOC(H5T_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
    H5O_loc_reset(&(dt->oloc));
    H5G_name_reset(&(dt->path));
    H5O_msg_reset_share(H5O_DTYPE_ID, dt);

    /* Allocate & initialize shared datatype structure */
    if(NULL == (dt->shared = H5FL_CALLOC(H5T_shared_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
    dt->shared->version = H5O_DTYPE_VERSION_1;

    /* Assign return value */
    ret_value = dt;

done:
    if(ret_value == NULL)
        if(dt) {
            if(dt->shared)
                dt->shared = H5FL_FREE(H5T_shared_t, dt->shared);
            dt = H5FL_FREE(H5T_t, dt);
        } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5T__free
 *
 * Purpose:	Frees all memory associated with a datatype, but does not
 *              free the H5T_t or H5T_shared_t structures (which should
 *              be done in H5T_close).
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Monday, January  6, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__free(H5T_t *dt)
{
    unsigned	i;
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_PACKAGE

    HDassert(dt && dt->shared);

    /*
     * If a named type is being closed then close the object header and
     * remove from the list of open objects in the file.
     */
    if(H5T_STATE_OPEN == dt->shared->state) {
        HDassert(dt->sh_loc.type == H5O_SHARE_TYPE_COMMITTED);
        HDassert(H5F_addr_defined(dt->sh_loc.u.loc.oh_addr));
        HDassert(H5F_addr_defined(dt->oloc.addr));

        /* Remove the datatype from the list of opened objects in the file */
        if(H5FO_top_decr(dt->sh_loc.file, dt->sh_loc.u.loc.oh_addr) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTRELEASE, FAIL, "can't decrement count for object")
        if(H5FO_delete(dt->sh_loc.file, H5AC_dxpl_id, dt->sh_loc.u.loc.oh_addr) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTRELEASE, FAIL, "can't remove datatype from list of open objects")
        if(H5O_close(&dt->oloc) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to close data type object header")
        dt->shared->state = H5T_STATE_NAMED;
    } /* end if */

    /* Free the ID to name info */
    H5G_name_free(&(dt->path));

    /*
     * Don't free locked datatypes.
     */
    if(H5T_STATE_IMMUTABLE==dt->shared->state)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CLOSEERROR, FAIL, "unable to close immutable datatype")

    /* Close the datatype */
    switch(dt->shared->type) {
        case H5T_COMPOUND:
            for(i = 0; i < dt->shared->u.compnd.nmembs; i++) {
                H5MM_xfree(dt->shared->u.compnd.memb[i].name);
                H5T_close(dt->shared->u.compnd.memb[i].type);
            } /* end for */
            H5MM_xfree(dt->shared->u.compnd.memb);
            break;

        case H5T_ENUM:
            for(i = 0; i < dt->shared->u.enumer.nmembs; i++)
                H5MM_xfree(dt->shared->u.enumer.name[i]);
            H5MM_xfree(dt->shared->u.enumer.name);
            H5MM_xfree(dt->shared->u.enumer.value);
            break;

        case H5T_OPAQUE:
            H5MM_xfree(dt->shared->u.opaque.tag);
            break;

        default:
            break;
    } /* end switch */

    /* Close the parent */
    HDassert(dt->shared->parent != dt);
    if(dt->shared->parent && H5T_close(dt->shared->parent) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCLOSEOBJ, FAIL, "unable to close parent data type")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__free() */


/*-------------------------------------------------------------------------
 * Function:	H5T_close
 *
 * Purpose:	Frees a data type and all associated memory.  If the data
 *		type is locked then nothing happens.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Monday, December  8, 1997
 *
 * Modifications:
 *      Robb Matzke, 1999-04-27
 *      This function fails if the datatype state is IMMUTABLE.
 *
 *      Robb Matzke, 1999-05-20
 *      Closes opaque types also.
 *
 *      Pedro Vicente, <pvn@ncsa.uiuc.edu> 22 Aug 2002
 *      Added "ID to name" support
 *
 *      Quincey Koziol, 2003-01-06
 *      Moved "guts" of function to H5T__free()
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_close(H5T_t *dt)
{
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(dt && dt->shared);

    if(dt->shared->state == H5T_STATE_OPEN)
        dt->shared->fo_count--;

    if(dt->shared->state != H5T_STATE_OPEN || dt->shared->fo_count == 0) {
        if(H5T__free(dt) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "unable to free datatype");

        dt->shared = H5FL_FREE(H5T_shared_t, dt->shared);
    } else {
        /*
         * If a named type is being closed then close the object header and
         * remove from the list of open objects in the file.
         */
        if(H5T_STATE_OPEN == dt->shared->state) {
            HDassert(dt->sh_loc.type == H5O_SHARE_TYPE_COMMITTED);

            /* Decrement the ref. count for this object in the top file */
            if(H5FO_top_decr(dt->sh_loc.file, dt->sh_loc.u.loc.oh_addr) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTRELEASE, FAIL, "can't decrement count for object")

            /* Check reference count for this object in the top file */
            if(H5FO_top_count(dt->sh_loc.file, dt->sh_loc.u.loc.oh_addr) == 0) {
                /* Close object location for named datatype */
                if(H5O_close(&dt->oloc) < 0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to close")
            } /* end if */
            else
                /* Free object location (i.e. "unhold" the file if appropriate)
                 */
                if(H5O_loc_free(&(dt->oloc)) < 0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTRELEASE, FAIL, "problem attempting to free location")
        } /* end if */

        /* Free the group hier. path since we're not calling H5T__free*/
        H5G_name_free(&(dt->path));
    } /* end else */

    /* Free the datatype struct */
    dt = H5FL_FREE(H5T_t, dt);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_close() */


/*-------------------------------------------------------------------------
 * Function:	H5T_set_size
 *
 * Purpose:	Sets the total size in bytes for a data type (this operation
 *		is not permitted on reference data types).  If the size is
 *		decreased so that the significant bits of the data type
 *		extend beyond the edge of the new size, then the `offset'
 *		property is decreased toward zero.  If the `offset' becomes
 *		zero and the significant bits of the data type still hang
 *		over the edge of the new size, then the number of significant
 *		bits is decreased.
 *
 *		Adjusting the size of an H5T_STRING automatically sets the
 *		precision to 8*size.
 *
 *		All data types have a positive size.
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Robb Matzke
 *              Tuesday, December 22, 1998
 *
 * Modifications:
 * 	Robb Matzke, 22 Dec 1998
 *	Also works with derived data types.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T_set_size(H5T_t *dt, size_t size)
{
    size_t	prec, offset;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(dt);
    HDassert(size!=0);
    HDassert(H5T_REFERENCE!=dt->shared->type);
    HDassert(!(H5T_ENUM==dt->shared->type && 0==dt->shared->u.enumer.nmembs));

    if (dt->shared->parent) {
        if (H5T_set_size(dt->shared->parent, size)<0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to set size for parent data type");

        /* Adjust size of datatype appropriately */
        if(dt->shared->type==H5T_ARRAY)
            dt->shared->size = dt->shared->parent->shared->size * dt->shared->u.array.nelem;
        else if(dt->shared->type!=H5T_VLEN)
            dt->shared->size = dt->shared->parent->shared->size;
    } else {
        if (H5T_IS_ATOMIC(dt->shared)) {
            offset = dt->shared->u.atomic.offset;
            prec = dt->shared->u.atomic.prec;

            /* Decrement the offset and precision if necessary */
            if (prec > 8*size)
                offset = 0;
            else
                if (offset+prec > 8*size)
                    offset = 8 * size - prec;
            if (prec > 8*size)
                prec = 8 * size;
        } else {
            prec = offset = 0;
        }

        switch (dt->shared->type) {
            case H5T_INTEGER:
            case H5T_TIME:
            case H5T_BITFIELD:
            case H5T_OPAQUE:
                /* nothing to check */
                break;

            case H5T_COMPOUND:
                /* If decreasing size, check the last member isn't being cut. */
                if(size<dt->shared->size) {
                    int         num_membs = 0;
                    unsigned    i, max_index=0;
                    size_t      memb_offset, max_offset=0;
                    size_t      max_size;

                    if((num_membs = H5T_get_nmembers(dt))<0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to get number of members");

                    if(num_membs) {
			for(i=0; i<(unsigned)num_membs; i++) {
			    memb_offset = H5T_get_member_offset(dt, i);
			    if(memb_offset > max_offset) {
			        max_offset = memb_offset;
				max_index = i;
			    }
			}

			max_size = H5T__get_member_size(dt, max_index);

			if(size<(max_offset+max_size))
			    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "size shrinking will cut off last member ");
                    }

                    /* Compound must not have been packed previously */
                    /* We will check if resizing changed the packed state of
                     * this type at the end of this function */
                    HDassert(!dt->shared->u.compnd.packed);
                }

                break;

            case H5T_STRING:
                /* Convert string to variable-length datatype */
                if(size==H5T_VARIABLE) {
                    H5T_t	*base = NULL;		/* base data type */
                    H5T_cset_t  tmp_cset;               /* Temp. cset info */
                    H5T_str_t   tmp_strpad;             /* Temp. strpad info */

                    /* Get a copy of unsigned char type as the base/parent type */
                    if(NULL == (base = (H5T_t *)H5I_object(H5T_NATIVE_UCHAR)))
                        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid base datatype");
                    dt->shared->parent=H5T_copy(base,H5T_COPY_ALL);

                    /* change this datatype into a VL string */
                    dt->shared->type = H5T_VLEN;

                    /*
                     * Force conversions (i.e. memory to memory conversions
		     * should duplicate data, not point to the same VL strings)
                     */
                    dt->shared->force_conv = TRUE;

		    /* Before we mess with the info in the union, extract the
		     * values we need */
                    tmp_cset=dt->shared->u.atomic.u.s.cset;
                    tmp_strpad=dt->shared->u.atomic.u.s.pad;

                    /* This is a string, not a sequence */
                    dt->shared->u.vlen.type = H5T_VLEN_STRING;

                    /* Set character set and padding information */
                    dt->shared->u.vlen.cset = tmp_cset;
                    dt->shared->u.vlen.pad  = tmp_strpad;

                    /* Set up VL information */
                    if (H5T_set_loc(dt, NULL, H5T_LOC_MEMORY)<0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid datatype location");

                } else {
                    prec = 8 * size;
                    offset = 0;
                } /* end else */
                break;

            case H5T_FLOAT:
                /*
                 * The sign, mantissa, and exponent fields should be adjusted
                 * first when decreasing the size of a floating point type.
                 */
                if (dt->shared->u.atomic.u.f.sign >= prec+offset ||
                        dt->shared->u.atomic.u.f.epos + dt->shared->u.atomic.u.f.esize > prec+offset ||
                        dt->shared->u.atomic.u.f.mpos + dt->shared->u.atomic.u.f.msize > prec+offset) {
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "adjust sign, mantissa, and exponent fields first");
                }
                break;

            case H5T_ENUM:
            case H5T_VLEN:
            case H5T_ARRAY:
            case H5T_REFERENCE:
                HDassert("can't happen" && 0);
            case H5T_NO_CLASS:
            case H5T_NCLASSES:
                HDassert("invalid type" && 0);
            default:
                HDassert("not implemented yet" && 0);
        }

        /* Commit (if we didn't convert this type to a VL string) */
        if(dt->shared->type!=H5T_VLEN) {
            dt->shared->size = size;
            if (H5T_IS_ATOMIC(dt->shared)) {
                dt->shared->u.atomic.offset = offset;
                dt->shared->u.atomic.prec = prec;
            }
        } /* end if */

        /* Check if the new compound type is packed */
        if(dt->shared->type == H5T_COMPOUND)
            H5T__update_packed(dt);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5T_get_size
 *
 * Purpose:	Determines the total size of a data type in bytes.
 *
 * Return:	Success:	Size of the data type in bytes.	 The size of
 *				the data type is the size of an instance of
 *				that data type.
 *
 *		Failure:	0 (valid data types are never zero size)
 *
 * Programmer:	Robb Matzke
 *		Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
size_t
H5T_get_size(const H5T_t *dt)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(dt);

    FUNC_LEAVE_NOAPI(dt->shared->size)
}


/*-------------------------------------------------------------------------
 * Function:	H5T_cmp
 *
 * Purpose:	Compares two data types.
 *
 * Return:	Success:	0 if DT1 and DT2 are equal.
 *				<0 if DT1 is less than DT2.
 *				>0 if DT1 is greater than DT2.
 *
 *		Failure:	0, never fails
 *
 * Programmer:	Robb Matzke
 *		Wednesday, December 10, 1997
 *
 *-------------------------------------------------------------------------
 */
int
H5T_cmp(const H5T_t *dt1, const H5T_t *dt2, hbool_t superset)
{
    unsigned	*idx1 = NULL, *idx2 = NULL;
    size_t	base_size;
    hbool_t	swapped;
    int	i, j;
    unsigned u;
    int	tmp;
    int	ret_value = 0;

    FUNC_ENTER_NOAPI(0)

    /* Sanity check */
    HDassert(dt1);
    HDassert(dt2);

    /* the easy case */
    if(dt1 == dt2)
        HGOTO_DONE(0);

    /* compare */
    if(dt1->shared->type < dt2->shared->type)
        HGOTO_DONE(-1);
    if(dt1->shared->type > dt2->shared->type)
        HGOTO_DONE(1);

    if(dt1->shared->size < dt2->shared->size)
        HGOTO_DONE(-1);
    if(dt1->shared->size > dt2->shared->size)
        HGOTO_DONE(1);

    if(dt1->shared->parent && !dt2->shared->parent)
        HGOTO_DONE(-1);
    if(!dt1->shared->parent && dt2->shared->parent)
        HGOTO_DONE(1);
    if(dt1->shared->parent) {
	tmp = H5T_cmp(dt1->shared->parent, dt2->shared->parent, superset);
	if(tmp < 0)
            HGOTO_DONE(-1);
	if(tmp > 0)
            HGOTO_DONE(1);
    } /* end if */

    switch(dt1->shared->type) {
        case H5T_COMPOUND:
            /*
             * Compound data types...
             */
            if(dt1->shared->u.compnd.nmembs < dt2->shared->u.compnd.nmembs)
                HGOTO_DONE(-1);
            if(dt1->shared->u.compnd.nmembs > dt2->shared->u.compnd.nmembs)
                HGOTO_DONE(1);

            /* Build an index for each type so the names are sorted */
            if(NULL == (idx1 = (unsigned *)H5MM_malloc(dt1->shared->u.compnd.nmembs * sizeof(unsigned))) ||
                    NULL == (idx2 = (unsigned *)H5MM_malloc(dt2->shared->u.compnd.nmembs * sizeof(unsigned))))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "memory allocation failed");
            for(u = 0; u < dt1->shared->u.compnd.nmembs; u++)
                idx1[u] = idx2[u] = u;
            if(dt1->shared->u.enumer.nmembs > 1) {
                for(i = dt1->shared->u.compnd.nmembs - 1, swapped = TRUE; swapped && i >= 0; --i)
                    for(j = 0, swapped=FALSE; j < i; j++)
                        if(HDstrcmp(dt1->shared->u.compnd.memb[idx1[j]].name,
                                 dt1->shared->u.compnd.memb[idx1[j + 1]].name) > 0) {
                            tmp = idx1[j];
                            idx1[j] = idx1[j + 1];
                            idx1[j + 1] = tmp;
                            swapped = TRUE;
                        }
                for(i = dt2->shared->u.compnd.nmembs - 1, swapped = TRUE; swapped && i >= 0; --i)
                    for(j = 0, swapped = FALSE; j<i; j++)
                        if(HDstrcmp(dt2->shared->u.compnd.memb[idx2[j]].name,
                                 dt2->shared->u.compnd.memb[idx2[j + 1]].name) > 0) {
                            tmp = idx2[j];
                            idx2[j] = idx2[j + 1];
                            idx2[j + 1] = tmp;
                            swapped = TRUE;
                        }
            } /* end if */

#ifdef H5T_DEBUG
            /* I don't quite trust the code above yet :-)  --RPM */
            for (u=0; u<dt1->shared->u.compnd.nmembs-1; u++) {
                HDassert(HDstrcmp(dt1->shared->u.compnd.memb[idx1[u]].name,
                        dt1->shared->u.compnd.memb[idx1[u + 1]].name));
                HDassert(HDstrcmp(dt2->shared->u.compnd.memb[idx2[u]].name,
                        dt2->shared->u.compnd.memb[idx2[u + 1]].name));
            }
#endif

            /* Compare the members */
            for (u=0; u<dt1->shared->u.compnd.nmembs; u++) {
                tmp = HDstrcmp(dt1->shared->u.compnd.memb[idx1[u]].name,
                       dt2->shared->u.compnd.memb[idx2[u]].name);
                if (tmp < 0)
                    HGOTO_DONE(-1);
                if (tmp > 0)
                    HGOTO_DONE(1);

                if (dt1->shared->u.compnd.memb[idx1[u]].offset < dt2->shared->u.compnd.memb[idx2[u]].offset) HGOTO_DONE(-1);
                if (dt1->shared->u.compnd.memb[idx1[u]].offset > dt2->shared->u.compnd.memb[idx2[u]].offset) HGOTO_DONE(1);

                if (dt1->shared->u.compnd.memb[idx1[u]].size < dt2->shared->u.compnd.memb[idx2[u]].size) HGOTO_DONE(-1);
                if (dt1->shared->u.compnd.memb[idx1[u]].size > dt2->shared->u.compnd.memb[idx2[u]].size) HGOTO_DONE(1);

                tmp = H5T_cmp(dt1->shared->u.compnd.memb[idx1[u]].type,
                      dt2->shared->u.compnd.memb[idx2[u]].type, superset);
                if (tmp < 0) HGOTO_DONE(-1);
                if (tmp > 0) HGOTO_DONE(1);
            }
            break;

        case H5T_ENUM:
            /*
             * Enumeration data types...
             */

            /* If we are doing a "superset" comparison, dt2 is allowed to have
             * more members than dt1
             */
            if(superset) {
                if (dt1->shared->u.enumer.nmembs > dt2->shared->u.enumer.nmembs)
                    HGOTO_DONE(1);
            } /* end if */
            else {
                if (dt1->shared->u.enumer.nmembs < dt2->shared->u.enumer.nmembs)
                    HGOTO_DONE(-1);
                if (dt1->shared->u.enumer.nmembs > dt2->shared->u.enumer.nmembs)
                    HGOTO_DONE(1);
            } /* end else */

            /* Build an index for each type so the names are sorted */
            if(NULL == (idx1 = (unsigned *)H5MM_malloc(dt1->shared->u.enumer.nmembs * sizeof(unsigned))) ||
                    NULL == (idx2 = (unsigned *)H5MM_malloc(dt2->shared->u.enumer.nmembs * sizeof(unsigned))))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "memory allocation failed");
            for (u=0; u<dt1->shared->u.enumer.nmembs; u++)
                idx1[u] = u;
            if(dt1->shared->u.enumer.nmembs > 1)
                for (i=dt1->shared->u.enumer.nmembs-1, swapped=TRUE; swapped && i>=0; --i)
                    for (j=0, swapped=FALSE; j<i; j++)
                        if (HDstrcmp(dt1->shared->u.enumer.name[idx1[j]],
                                 dt1->shared->u.enumer.name[idx1[j+1]]) > 0) {
                            tmp = idx1[j];
                            idx1[j] = idx1[j+1];
                            idx1[j+1] = tmp;
                            swapped = TRUE;
                        }
            for (u=0; u<dt2->shared->u.enumer.nmembs; u++)
                idx2[u] = u;
            if(dt2->shared->u.enumer.nmembs > 1)
                for (i=dt2->shared->u.enumer.nmembs-1, swapped=TRUE; swapped && i>=0; --i)
                    for (j=0, swapped=FALSE; j<i; j++)
                        if (HDstrcmp(dt2->shared->u.enumer.name[idx2[j]],
                                 dt2->shared->u.enumer.name[idx2[j+1]]) > 0) {
                            tmp = idx2[j];
                            idx2[j] = idx2[j+1];
                            idx2[j+1] = tmp;
                            swapped = TRUE;
                        }

#ifdef H5T_DEBUG
            /* I don't quite trust the code above yet :-)  --RPM */
            for (u=0; u<dt1->shared->u.enumer.nmembs-1; u++) {
                HDassert(HDstrcmp(dt1->shared->u.enumer.name[idx1[u]],
                        dt1->shared->u.enumer.name[idx1[u+1]]));
                HDassert(HDstrcmp(dt2->shared->u.enumer.name[idx2[u]],
                        dt2->shared->u.enumer.name[idx2[u+1]]));
            }
#endif

            /* Compare the members */
            base_size = dt1->shared->parent->shared->size;
            for (u=0; u<dt1->shared->u.enumer.nmembs; u++) {
                unsigned idx = 0;

                if(superset) {
                    unsigned    lt = 0, rt;        /* Final, left & right key indices */
                    int	        cmp = 1;                /* Key comparison value */

                    /* If a superset is allowed, dt2 may have more members
                     * than dt1, so binary search for matching member name in
                     * dt2
                     */
                    rt = dt2->shared->u.enumer.nmembs;

                    while (lt < rt && cmp) {
                        idx = (lt + rt) / 2;

                        /* compare */
                        if ((cmp = HDstrcmp(dt1->shared->u.enumer.name[idx1[u]],
                                dt2->shared->u.enumer.name[idx2[idx]] ) ) < 0)
                            rt = idx;
                        else
                            lt = idx+1;
                    }
                    /* Leave, if we couldn't find match */
                    if (cmp)
                        HGOTO_DONE(-1);
                } /* end if */
                else {
                    /* Check for exact member name match when not doing
                     * "superset" comparison
                     */
                    tmp = HDstrcmp(dt1->shared->u.enumer.name[idx1[u]],
                           dt2->shared->u.enumer.name[idx2[u]]);
                    if (tmp<0) HGOTO_DONE(-1);
                    if (tmp>0) HGOTO_DONE(1);

                    /* Set index value appropriately */
                    idx = u;
                } /* end else */

                tmp = HDmemcmp(dt1->shared->u.enumer.value+idx1[u]*base_size,
                       dt2->shared->u.enumer.value+idx2[idx]*base_size,
                       base_size);
                if (tmp<0) HGOTO_DONE(-1);
                if (tmp>0) HGOTO_DONE(1);
            }
            break;

        case H5T_VLEN:
            HDassert(dt1->shared->u.vlen.type>H5T_VLEN_BADTYPE && dt1->shared->u.vlen.type<H5T_VLEN_MAXTYPE);
            HDassert(dt2->shared->u.vlen.type>H5T_VLEN_BADTYPE && dt2->shared->u.vlen.type<H5T_VLEN_MAXTYPE);
            HDassert(dt1->shared->u.vlen.loc>=H5T_LOC_BADLOC && dt1->shared->u.vlen.loc<H5T_LOC_MAXLOC);
            HDassert(dt2->shared->u.vlen.loc>=H5T_LOC_BADLOC && dt2->shared->u.vlen.loc<H5T_LOC_MAXLOC);

            /* Arbitrarily sort sequence VL datatypes before string VL datatypes */
            if (dt1->shared->u.vlen.type==H5T_VLEN_SEQUENCE &&
                    dt2->shared->u.vlen.type==H5T_VLEN_STRING) {
                HGOTO_DONE(-1);
            } else if (dt1->shared->u.vlen.type==H5T_VLEN_STRING &&
                    dt2->shared->u.vlen.type==H5T_VLEN_SEQUENCE) {
                HGOTO_DONE(1);
            }
            /* Arbitrarily sort VL datatypes in memory before disk */
            if (dt1->shared->u.vlen.loc==H5T_LOC_MEMORY &&
                    dt2->shared->u.vlen.loc==H5T_LOC_DISK) {
                HGOTO_DONE(-1);
            } else if (dt1->shared->u.vlen.loc==H5T_LOC_DISK &&
                    dt2->shared->u.vlen.loc==H5T_LOC_MEMORY) {
                HGOTO_DONE(1);
            } else if (dt1->shared->u.vlen.loc==H5T_LOC_BADLOC &&
                    dt2->shared->u.vlen.loc!=H5T_LOC_BADLOC) {
                HGOTO_DONE(1);
            }

            /* Don't allow VL types in different files to compare as equal */
            if (dt1->shared->u.vlen.f < dt2->shared->u.vlen.f)
                HGOTO_DONE(-1);
            if (dt1->shared->u.vlen.f > dt2->shared->u.vlen.f)
                HGOTO_DONE(1);
            break;

        case H5T_OPAQUE:
            if(dt1->shared->u.opaque.tag && dt2->shared->u.opaque.tag)
                HGOTO_DONE(HDstrcmp(dt1->shared->u.opaque.tag,dt2->shared->u.opaque.tag));
            break;

        case H5T_ARRAY:
            if (dt1->shared->u.array.ndims < dt2->shared->u.array.ndims)
                HGOTO_DONE(-1);
            if (dt1->shared->u.array.ndims > dt2->shared->u.array.ndims)
                HGOTO_DONE(1);

            for (u=0; u<dt1->shared->u.array.ndims; u++) {
                if (dt1->shared->u.array.dim[u] < dt2->shared->u.array.dim[u])
                    HGOTO_DONE(-1);
                if (dt1->shared->u.array.dim[u] > dt2->shared->u.array.dim[u])
                    HGOTO_DONE(1);
            }

            tmp = H5T_cmp(dt1->shared->parent, dt2->shared->parent, superset);
            if (tmp < 0)
                HGOTO_DONE(-1);
            if (tmp > 0)
                HGOTO_DONE(1);
            break;

        default:
            /*
             * Atomic datatypes...
             */
            if (dt1->shared->u.atomic.order < dt2->shared->u.atomic.order) HGOTO_DONE(-1);
            if (dt1->shared->u.atomic.order > dt2->shared->u.atomic.order) HGOTO_DONE(1);

            if (dt1->shared->u.atomic.prec < dt2->shared->u.atomic.prec) HGOTO_DONE(-1);
            if (dt1->shared->u.atomic.prec > dt2->shared->u.atomic.prec) HGOTO_DONE(1);

            if (dt1->shared->u.atomic.offset < dt2->shared->u.atomic.offset) HGOTO_DONE(-1);
            if (dt1->shared->u.atomic.offset > dt2->shared->u.atomic.offset) HGOTO_DONE(1);

            if (dt1->shared->u.atomic.lsb_pad < dt2->shared->u.atomic.lsb_pad) HGOTO_DONE(-1);
            if (dt1->shared->u.atomic.lsb_pad > dt2->shared->u.atomic.lsb_pad) HGOTO_DONE(1);

            if (dt1->shared->u.atomic.msb_pad < dt2->shared->u.atomic.msb_pad) HGOTO_DONE(-1);
            if (dt1->shared->u.atomic.msb_pad > dt2->shared->u.atomic.msb_pad) HGOTO_DONE(1);

            switch (dt1->shared->type) {
                case H5T_INTEGER:
                    if (dt1->shared->u.atomic.u.i.sign < dt2->shared->u.atomic.u.i.sign)
                        HGOTO_DONE(-1);
                    if (dt1->shared->u.atomic.u.i.sign > dt2->shared->u.atomic.u.i.sign)
                        HGOTO_DONE(1);
                    break;

                case H5T_FLOAT:
                    if (dt1->shared->u.atomic.u.f.sign < dt2->shared->u.atomic.u.f.sign)
                        HGOTO_DONE(-1);
                    if (dt1->shared->u.atomic.u.f.sign > dt2->shared->u.atomic.u.f.sign)
                        HGOTO_DONE(1);

                    if (dt1->shared->u.atomic.u.f.epos < dt2->shared->u.atomic.u.f.epos)
                        HGOTO_DONE(-1);
                    if (dt1->shared->u.atomic.u.f.epos > dt2->shared->u.atomic.u.f.epos)
                        HGOTO_DONE(1);

                    if (dt1->shared->u.atomic.u.f.esize < dt2->shared->u.atomic.u.f.esize) HGOTO_DONE(-1);
                    if (dt1->shared->u.atomic.u.f.esize > dt2->shared->u.atomic.u.f.esize) HGOTO_DONE(1);

                    if (dt1->shared->u.atomic.u.f.ebias < dt2->shared->u.atomic.u.f.ebias) HGOTO_DONE(-1);
                    if (dt1->shared->u.atomic.u.f.ebias > dt2->shared->u.atomic.u.f.ebias) HGOTO_DONE(1);

                    if (dt1->shared->u.atomic.u.f.mpos < dt2->shared->u.atomic.u.f.mpos)
                        HGOTO_DONE(-1);
                    if (dt1->shared->u.atomic.u.f.mpos > dt2->shared->u.atomic.u.f.mpos)
                        HGOTO_DONE(1);

                    if (dt1->shared->u.atomic.u.f.msize < dt2->shared->u.atomic.u.f.msize) HGOTO_DONE(-1);
                    if (dt1->shared->u.atomic.u.f.msize > dt2->shared->u.atomic.u.f.msize) HGOTO_DONE(1);

                    if (dt1->shared->u.atomic.u.f.norm < dt2->shared->u.atomic.u.f.norm)
                        HGOTO_DONE(-1);
                    if (dt1->shared->u.atomic.u.f.norm > dt2->shared->u.atomic.u.f.norm)
                        HGOTO_DONE(1);

                    if (dt1->shared->u.atomic.u.f.pad < dt2->shared->u.atomic.u.f.pad)
                        HGOTO_DONE(-1);
                    if (dt1->shared->u.atomic.u.f.pad > dt2->shared->u.atomic.u.f.pad)
                        HGOTO_DONE(1);

                    break;

                case H5T_TIME:  /* order and precision are checked above */
                    /*void */
                    break;

                case H5T_STRING:
                    if (dt1->shared->u.atomic.u.s.cset < dt2->shared->u.atomic.u.s.cset)
                        HGOTO_DONE(-1);
                    if (dt1->shared->u.atomic.u.s.cset > dt2->shared->u.atomic.u.s.cset)
                        HGOTO_DONE(1);

                    if (dt1->shared->u.atomic.u.s.pad < dt2->shared->u.atomic.u.s.pad)
                        HGOTO_DONE(-1);
                    if (dt1->shared->u.atomic.u.s.pad > dt2->shared->u.atomic.u.s.pad)
                        HGOTO_DONE(1);

                    break;

                case H5T_BITFIELD:
                    /*void */
                    break;

                case H5T_REFERENCE:
                    if (dt1->shared->u.atomic.u.r.rtype < dt2->shared->u.atomic.u.r.rtype)
                        HGOTO_DONE(-1);
                    if (dt1->shared->u.atomic.u.r.rtype > dt2->shared->u.atomic.u.r.rtype)
                        HGOTO_DONE(1);

                    switch(dt1->shared->u.atomic.u.r.rtype) {
                        case H5R_OBJECT:
                            if (dt1->shared->u.atomic.u.r.loc < dt2->shared->u.atomic.u.r.loc)
                                HGOTO_DONE(-1);
                            if (dt1->shared->u.atomic.u.r.loc > dt2->shared->u.atomic.u.r.loc)
                                HGOTO_DONE(1);
                            break;

                        case H5R_DATASET_REGION:
                    /* Does this need more to distinguish it? -QAK 11/30/98 */
                            /*void */
                            break;

                        case H5R_BADTYPE:
                        case H5R_MAXTYPE:
                            HDassert("invalid type" && 0);
                        default:
                            HDassert("not implemented yet" && 0);
                    }
                    break;

                default:
                    HDassert("not implemented yet" && 0);
            }
        break;
    } /* end switch */

done:
    if(NULL != idx1)
        H5MM_xfree(idx1);
    if(NULL != idx2)
        H5MM_xfree(idx2);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_cmp() */


/*-------------------------------------------------------------------------
 * Function:	H5T_path_find
 *
 * Purpose:	Finds the path which converts type SRC_ID to type DST_ID,
 *		creating a new path if necessary.  If FUNC is non-zero then
 *		it is set as the hard conversion function for that path
 *		regardless of whether the path previously existed. Changing
 *		the conversion function of a path causes statistics to be
 *		reset to zero after printing them.  The NAME is used only
 *		when creating a new path and is just for debugging.
 *
 *		If SRC and DST are both null pointers then the special no-op
 *		conversion path is used.  This path is always stored as the
 *		first path in the path table.
 *
 * Return:	Success:	Pointer to the path, valid until the path
 *				database is modified.
 *
 *		Failure:	NULL if the path does not exist and no
 *				function can be found to apply to the new
 *				path.
 *
 * Programmer:	Robb Matzke
 *		Tuesday, January 13, 1998
 *
 * Modifications:
 *              Added a parameter IS_API to indicate whether to an API
 *              function issued a call to this function.  If a API
 *              function like H5Tregister() is calling this function to
 *              register a new hard conversion function, IS_API is TRUE
 *              and the old path is replaced.  If a private function like
 *              H5T_init_interface() is trying to register hard conversions,
 *              IS_API is FALSE and the old hard path is not replaced.
 *              Tuesday, Sept 13, 2005
 *
 *-------------------------------------------------------------------------
 */
H5T_path_t *
H5T_path_find(const H5T_t *src, const H5T_t *dst, const char *name,
    H5T_conv_t func, hid_t dxpl_id, hbool_t is_api)
{
    int	lt, rt;			/*left and right edges		*/
    int	md;			/*middle			*/
    int	cmp;			/*comparison result		*/
    int old_npaths;             /* Previous number of paths in table */
    H5T_path_t	*table = NULL;		/*path existing in the table	*/
    H5T_path_t	*path = NULL;		/*new path			*/
    hid_t	src_id = -1, dst_id = -1;	/*src and dst type identifiers	*/
    int	i;			/*counter			*/
    int	nprint = 0;		/*lines of output printed	*/
    H5T_path_t	*ret_value;	/*return value			*/

    FUNC_ENTER_NOAPI(NULL)

    /* Sanity check */
    HDassert(src);
    HDassert(dst);

    /*
     * Make sure the first entry in the table is the no-op conversion path.
     */
    if(0 == H5T_g.npaths) {
	if(NULL == (H5T_g.path = (H5T_path_t **)H5MM_malloc(128 * sizeof(H5T_path_t *))))
	    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for type conversion path table")
	H5T_g.apaths = 128;
	if(NULL == (H5T_g.path[0] = H5FL_CALLOC(H5T_path_t)))
	    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for no-op conversion path")
        HDsnprintf(H5T_g.path[0]->name, sizeof(H5T_g.path[0]->name), "no-op");
	H5T_g.path[0]->func = H5T__conv_noop;
	H5T_g.path[0]->cdata.command = H5T_CONV_INIT;
	if(H5T__conv_noop(FAIL, FAIL, &(H5T_g.path[0]->cdata), (size_t)0, (size_t)0, (size_t)0, NULL, NULL, dxpl_id) < 0) {
#ifdef H5T_DEBUG
	    if(H5DEBUG(T))
		fprintf(H5DEBUG(T), "H5T: unable to initialize no-op conversion function (ignored)\n");
#endif
	    H5E_clear_stack(NULL); /*ignore the error*/
	} /* end if */
	H5T_g.path[0]->is_noop = TRUE;
	H5T_g.npaths = 1;
    } /* end if */

    /*
     * Find the conversion path.  If source and destination types are equal
     * then use entry[0], otherwise do a binary search over the
     * remaining entries.
     *
     * Quincey Koziol, 2 July, 1999
     * Only allow the no-op conversion to occur if no "force conversion" flags
     * are set
     */
    if(src->shared->force_conv == FALSE && dst->shared->force_conv == FALSE && 0 == H5T_cmp(src, dst, TRUE)) {
	table = H5T_g.path[0];
	cmp = 0;
	md = 0;
    } /* end if */
    else {
	lt = md = 1;
	rt = H5T_g.npaths;
	cmp = -1;

	while(cmp && lt < rt) {
	    md = (lt + rt) / 2;
	    HDassert(H5T_g.path[md]);
	    cmp = H5T_cmp(src, H5T_g.path[md]->src, FALSE);
	    if(0 == cmp)
                cmp = H5T_cmp(dst, H5T_g.path[md]->dst, FALSE);
	    if(cmp < 0)
		rt = md;
	    else if(cmp > 0)
		lt = md + 1;
	    else
		table = H5T_g.path[md];
	} /* end while */
    } /* end else */

    /* Keep a record of the number of paths in the table, in case one of the
     * initialization calls below (hard or soft) causes more entries to be
     * added to the table - QAK, 1/26/02
     */
    old_npaths = H5T_g.npaths;

    /*
     * If we didn't find the path, if the caller is an API function specifying
     * a new hard conversion function, or if the caller is a private function
     * specifying a new hard conversion and the path is a soft conversion, then
     * create a new path and add the new function to the path.
     */
    if(!table || (table && func && is_api) || (table && !table->is_hard && func && !is_api)) {
	if(NULL == (path = H5FL_CALLOC(H5T_path_t)))
	    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for type conversion path")
	if(name && *name) {
	    HDstrncpy(path->name, name, (size_t)H5T_NAMELEN);
	    path->name[H5T_NAMELEN - 1] = '\0';
        } /* end if */
	else
	    HDsnprintf(path->name, sizeof(path->name), "NONAME");
	if(NULL == (path->src = H5T_copy(src, H5T_COPY_ALL)))
	    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to copy datatype for conversion path")
        if(NULL == (path->dst = H5T_copy(dst, H5T_COPY_ALL)))
	    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to copy datatype for conversion path")
    } /* end if */
    else
	path = table;

    /*
     * If a hard conversion function is specified and none is defined for the
     * path, or the caller is an API function, or the caller is a private function but
     * the existing path is a soft function, then add the new conversion to the path
     * and initialize its conversion data.
     */
    if(func && (!table || (table && is_api) || (table && !table->is_hard && !is_api))) {
	HDassert(path != table);
	HDassert(NULL == path->func);
	if(path->src && (src_id = H5I_register(H5I_DATATYPE, H5T_copy(path->src, H5T_COPY_ALL), FALSE)) < 0)
	    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, NULL, "unable to register source conversion type for query")
	if(path->dst && (dst_id = H5I_register(H5I_DATATYPE, H5T_copy(path->dst, H5T_COPY_ALL), FALSE)) < 0)
	    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, NULL, "unable to register destination conversion type for query")
	path->cdata.command = H5T_CONV_INIT;
	if((func)(src_id, dst_id, &(path->cdata), (size_t)0, (size_t)0, (size_t)0, NULL, NULL, dxpl_id) < 0)
	    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to initialize conversion function")
	if(src_id >= 0)
            H5I_dec_ref(src_id);
	if(dst_id >= 0)
            H5I_dec_ref(dst_id);
	src_id = dst_id = -1;
	path->func = func;
	path->is_hard = TRUE;
    } /* end if */

    /*
     * If the path doesn't have a function by now (because it's a new path
     * and the caller didn't supply a hard function) then scan the soft list
     * for an applicable function and add it to the path.  This can't happen
     * for the no-op conversion path.
     */
    HDassert(path->func || (src && dst));
    for(i = H5T_g.nsoft - 1; i >= 0 && !path->func; --i) {
	if(src->shared->type != H5T_g.soft[i].src || dst->shared->type != H5T_g.soft[i].dst)
	    continue;
	if((src_id = H5I_register(H5I_DATATYPE, H5T_copy(path->src, H5T_COPY_ALL), FALSE)) < 0)
	    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, NULL, "unable to register src conversion type for query")
        if((dst_id = H5I_register(H5I_DATATYPE, H5T_copy(path->dst, H5T_COPY_ALL), FALSE)) < 0)
	    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, NULL, "unable to register dst conversion type for query")
	path->cdata.command = H5T_CONV_INIT;
	if((H5T_g.soft[i].func)(src_id, dst_id, &(path->cdata), (size_t)0, (size_t)0, (size_t)0, NULL, NULL, dxpl_id) < 0) {
	    HDmemset(&(path->cdata), 0, sizeof(H5T_cdata_t));
	    H5E_clear_stack(H5E_DEFAULT); /*ignore the error*/
	} /* end if */
        else {
	    HDstrncpy(path->name, H5T_g.soft[i].name, (size_t)H5T_NAMELEN);
	    path->name[H5T_NAMELEN - 1] = '\0';
	    path->func = H5T_g.soft[i].func;
	    path->is_hard = FALSE;
	} /* end else */
	H5I_dec_ref(src_id);
	H5I_dec_ref(dst_id);
	src_id = dst_id = -1;
    } /* end for */
    if(!path->func)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "no appropriate function for conversion path")

    /* Check if paths were inserted into the table through a recursive call
     * and re-compute the correct location for this path if so. - QAK, 1/26/02
     */
    if(old_npaths != H5T_g.npaths) {
        lt = md = 1;
        rt = H5T_g.npaths;
        cmp = -1;

        while(cmp && lt < rt) {
            md = (lt + rt) / 2;
            HDassert(H5T_g.path[md]);
            cmp = H5T_cmp(src, H5T_g.path[md]->src, FALSE);
            if(0 == cmp)
                cmp = H5T_cmp(dst, H5T_g.path[md]->dst, FALSE);
            if(cmp < 0)
                rt = md;
            else if(cmp > 0)
                lt = md + 1;
            else
                table = H5T_g.path[md];
        } /* end while */
    } /* end if */

    /* Replace an existing table entry or add a new entry */
    if(table && path != table) {
	HDassert(table == H5T_g.path[md]);
	H5T__print_stats(table, &nprint/*in,out*/);
	table->cdata.command = H5T_CONV_FREE;
	if((table->func)(FAIL, FAIL, &(table->cdata), (size_t)0, (size_t)0, (size_t)0, NULL, NULL, dxpl_id) < 0) {
#ifdef H5T_DEBUG
	    if(H5DEBUG(T)) {
		fprintf(H5DEBUG(T), "H5T: conversion function 0x%08lx free "
			"failed for %s (ignored)\n",
			(unsigned long)(path->func), path->name);
	    } /* end if */
#endif
	    H5E_clear_stack(NULL); /*ignore the failure*/
	} /* end if */
	if(table->src)
            H5T_close(table->src);
	if(table->dst)
            H5T_close(table->dst);
        table = H5FL_FREE(H5T_path_t, table);
	table = path;
	H5T_g.path[md] = path;
    } else if(path != table) {
	HDassert(cmp);
        if((size_t)H5T_g.npaths >= H5T_g.apaths) {
            size_t na = MAX(128, 2 * H5T_g.apaths);
            H5T_path_t **x;

            if(NULL == (x = (H5T_path_t **)H5MM_realloc(H5T_g.path, na * sizeof(H5T_path_t*))))
		HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
            H5T_g.apaths = na;
            H5T_g.path = x;
        } /* end if */
        if(cmp > 0)
            md++;
        HDmemmove(H5T_g.path + md + 1, H5T_g.path + md, (H5T_g.npaths - md) * sizeof(H5T_path_t*));
        H5T_g.npaths++;
	H5T_g.path[md] = path;
	table = path;
    } /* end else-if */

    /* Set the flag to indicate both source and destination types are compound types
     * for the optimization of data reading (in H5Dio.c). */
    if(H5T_COMPOUND == H5T_get_class(src, TRUE) && H5T_COMPOUND == H5T_get_class(dst, TRUE))
        path->are_compounds = TRUE;

    /* Set return value */
    ret_value = path;

done:
    if(!ret_value && path && path != table) {
	if(path->src)
            H5T_close(path->src);
	if(path->dst)
            H5T_close(path->dst);
        path = H5FL_FREE(H5T_path_t, path);
    } /* end if */
    if(src_id >= 0)
        H5I_dec_ref(src_id);
    if(dst_id >= 0)
        H5I_dec_ref(dst_id);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_path_find() */


/*-------------------------------------------------------------------------
 * Function:	H5T_path_noop
 *
 * Purpose:	Is the path the special no-op path? The no-op function can be
 *              set by the application and there might be more than one no-op
 *              path in a multi-threaded application if one thread is using
 *              the no-op path when some other thread changes its definition.
 *
 * Return:	TRUE/FALSE (can't fail)
 *
 * Programmer:	Quincey Koziol
 *		Thursday, May  8, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hbool_t
H5T_path_noop(const H5T_path_t *p)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(p);

    FUNC_LEAVE_NOAPI(p->is_noop || (p->is_hard && 0==H5T_cmp(p->src, p->dst, FALSE)))
} /* end H5T_path_noop() */


/*-------------------------------------------------------------------------
 * Function:	H5T_path_compound_subset
 *
 * Purpose:	Checks if the source and destination types are both compound.
 *              Tells whether whether the source members are a subset of
 *              destination, and the order is the same, and no conversion
 *              is needed.  For example:
 *                  struct source {            struct destination {
 *                      TYPE1 A;      -->          TYPE1 A;
 *                      TYPE2 B;      -->          TYPE2 B;
 *                      TYPE3 C;      -->          TYPE3 C;
 *                  };                             TYPE4 D;
 *                                                 TYPE5 E;
 *                                             };
 *
 * Return:	A pointer to the subset info struct in p, or NULL if there are
 *          no compounds.  Points directly into the H5T_path_t structure.
 *
 * Programmer:	Raymond Lu
 *		8 June 2007
 *
 * Modifications:  Neil Fortner
 *      19 September 2008
 *      Changed return value to H5T_subset_info_t
 *      (to allow it to return copy_size)
 *
 *-------------------------------------------------------------------------
 */
H5T_subset_info_t *
H5T_path_compound_subset(const H5T_path_t *p)
{
    H5T_subset_info_t *ret_value = NULL;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(p);

    if(p->are_compounds)
        ret_value = H5T__conv_struct_subset(&(p->cdata));

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_path_compound_subset */


/*-------------------------------------------------------------------------
 * Function:	H5T_path_bkg
 *
 * Purpose:	Get the "background" flag for the conversion path.
 *
 * Return:	Background flag (can't fail)
 *
 * Programmer:	Quincey Koziol
 *		Thursday, May  8, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_bkg_t
H5T_path_bkg(const H5T_path_t *p)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(p);

    FUNC_LEAVE_NOAPI(p->cdata.need_bkg)
} /* end H5T_path_bkg() */


/*-------------------------------------------------------------------------
 * Function:	H5T_compiler_conv
 *
 * Purpose:	Private function for H5Tcompiler_conv.  Finds out whether the
 *              library's conversion function from type SRC to type DST
 *              is a hard conversion.
 *
 * Return:	TRUE:           hard conversion.
 *		FALSE:          soft conversion.
 *		FAIL:           function failed.
 *
 * Programmer:	Raymond Lu
 *		Friday, Sept 2, 2005
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5T_compiler_conv(H5T_t *src, H5T_t *dst)
{
    H5T_path_t	*path;
    htri_t	ret_value;

    FUNC_ENTER_NOAPI_NOINIT

    /* Find it */
    if (NULL==(path=H5T_path_find(src, dst, NULL, NULL, H5AC_ind_dxpl_id, FALSE)))
	HGOTO_ERROR(H5E_DATATYPE, H5E_NOTFOUND, FAIL, "conversion function not found")

    ret_value = (htri_t)path->is_hard;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5T_convert
 *
 * Purpose:	Call a conversion function to convert from source to
 *		destination data type and accumulate timing statistics.
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Robb Matzke
 *              Tuesday, December 15, 1998
 *
 * Modifications:
 * 		Robb Matzke, 1999-06-16
 *		The timers are updated only if H5T debugging is enabled at
 *		runtime in addition to compile time.
 *
 *		Robb Matzke, 1999-06-16
 *		Added support for non-zero strides. If BUF_STRIDE is non-zero
 *		then convert one value at each memory location advancing
 *		BUF_STRIDE bytes each time; otherwise assume both source and
 *		destination values are packed.
 *
 *              Quincey Koziol, 1999-07-01
 *              Added dataset transfer properties, to allow custom VL
 *              datatype allocation function to be passed down to VL
 *              conversion routine.
 *
 *              Robb Matzke, 2000-05-17
 *              Added the BKG_STRIDE argument which gets passed to all the
 *              conversion functions. If BUF_STRIDE is non-zero then each
 *              data element is at a multiple of BUF_STRIDE bytes in BUF
 *              (on both input and output). If BKG_STRIDE is also set then
 *              the BKG buffer is used in such a way that temporary space
 *              for each element is aligned on a BKG_STRIDE byte boundary.
 *              If either BUF_STRIDE or BKG_STRIDE are zero then the BKG
 *              buffer will be accessed as though it were a packed array
 *              of destination datatype.
 *-------------------------------------------------------------------------
 */
herr_t
H5T_convert(H5T_path_t *tpath, hid_t src_id, hid_t dst_id, size_t nelmts,
	    size_t buf_stride, size_t bkg_stride, void *buf, void *bkg,
            hid_t dset_xfer_plist)
{
#ifdef H5T_DEBUG
    H5_timer_t		timer;
#endif
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

#ifdef H5T_DEBUG
    if (H5DEBUG(T)) H5_timer_begin(&timer);
#endif
    tpath->cdata.command = H5T_CONV_CONV;
    if ((tpath->func)(src_id, dst_id, &(tpath->cdata), nelmts, buf_stride,
                      bkg_stride, buf, bkg, dset_xfer_plist)<0)
	HGOTO_ERROR(H5E_ATTR, H5E_CANTENCODE, FAIL, "data type conversion failed");
#ifdef H5T_DEBUG
    if (H5DEBUG(T)) {
	H5_timer_end(&(tpath->stats.timer), &timer);
	tpath->stats.ncalls++;
	tpath->stats.nelmts += nelmts;
    }
#endif

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5T_oloc
 *
 * Purpose:	Returns a pointer to the object location for a named datatype.
 *
 * Return:	Success:	Ptr directly into named datatype
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Friday, June  5, 1998
 *
 *-------------------------------------------------------------------------
 */
H5O_loc_t *
H5T_oloc(H5T_t *dt)
{
    H5O_loc_t *ret_value = NULL;

    FUNC_ENTER_NOAPI(NULL)

    HDassert(dt);

    switch(dt->shared->state) {
        case H5T_STATE_TRANSIENT:
        case H5T_STATE_RDONLY:
        case H5T_STATE_IMMUTABLE:
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "not a named datatype")
        case H5T_STATE_NAMED:
        case H5T_STATE_OPEN:
            HDassert(dt->sh_loc.type == H5O_SHARE_TYPE_COMMITTED);
            ret_value = &dt->oloc;
            break;
        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, NULL, "invalid datatype state")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_oloc() */


/*-------------------------------------------------------------------------
 * Function:	H5T_nameof
 *
 * Purpose:	Returns a pointer to the path for a named datatype.
 *
 * Return:	Success:	Ptr directly into named datatype
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 12, 2005
 *
 *-------------------------------------------------------------------------
 */
H5G_name_t *
H5T_nameof(H5T_t *dt)
{
    H5G_name_t *ret_value = NULL;

    FUNC_ENTER_NOAPI(NULL)

    HDassert(dt);

    switch(dt->shared->state) {
        case H5T_STATE_TRANSIENT:
        case H5T_STATE_RDONLY:
        case H5T_STATE_IMMUTABLE:
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "not a named datatype")
        case H5T_STATE_NAMED:
        case H5T_STATE_OPEN:
            ret_value = &(dt->path);
            break;
        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, NULL, "invalid datatype state")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_nameof() */


/*-------------------------------------------------------------------------
 * Function:    H5T_is_immutable
 *
 * Purpose:     Check if a datatype is immutable.
 *
 * Return:      TRUE
 *
 *              FALSE
 *
 * Programmer:  Raymond Lu
 *              Friday, Dec 7, 2001
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5T_is_immutable(const H5T_t *dt)
{
    htri_t ret_value = FALSE;

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(dt);

    if(dt->shared->state == H5T_STATE_IMMUTABLE)
        ret_value = TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5T_is_named
 *
 * Purpose:     Check if a datatype is named.
 *
 * Return:      TRUE
 *
 *              FALSE
 *
 * Programmer:  Pedro Vicente
 *              Tuesday, Sep 3, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5T_is_named(const H5T_t *dt)
{
    htri_t ret_value = FALSE;

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(dt);

    if(dt->shared->state == H5T_STATE_OPEN || dt->shared->state == H5T_STATE_NAMED)
        ret_value = TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*--------------------------------------------------------------------------
 NAME
    H5T_get_ref_type
 PURPOSE
    Retrieves the type of reference for a datatype
 USAGE
    H5R_type_t H5Tget_ref_type(dt)
        H5T_t *dt;  IN: datatype pointer for the reference datatype

 RETURNS
    Success:	A reference type defined in H5Rpublic.h
    Failure:	H5R_BADTYPE
 DESCRIPTION
    Given a reference datatype object, this function returns the reference type
        of the datatype.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5R_type_t
H5T_get_ref_type(const H5T_t *dt)
{
    H5R_type_t ret_value = H5R_BADTYPE;

    FUNC_ENTER_NOAPI(H5R_BADTYPE)

    HDassert(dt);

    if(dt->shared->type==H5T_REFERENCE)
        ret_value=dt->shared->u.atomic.u.r.rtype;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5T_get_ref_type() */


/*-------------------------------------------------------------------------
 * Function:	H5T_is_sensible
 *
 * Purpose:	Determines if a data type is sensible to store on disk
 *              (i.e. not partially initialized)
 *
 * Return:	Success:	TRUE, FALSE
 *
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, June 11, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5T_is_sensible(const H5T_t *dt)
{
    htri_t	ret_value;

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(dt);

    switch(dt->shared->type) {
        case H5T_COMPOUND:
            /* Only allow compound datatypes with at least one member to be stored on disk */
            if(dt->shared->u.compnd.nmembs > 0)
                ret_value=TRUE;
            else
                ret_value=FALSE;
            break;

        case H5T_ENUM:
            /* Only allow enum datatypes with at least one member to be stored on disk */
            if(dt->shared->u.enumer.nmembs > 0)
                ret_value=TRUE;
            else
                ret_value=FALSE;
            break;

        default:
            /* Assume all other datatype are sensible to store on disk */
            ret_value=TRUE;
            break;
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*--------------------------------------------------------------------------
 NAME
    H5T_set_loc
 PURPOSE
    Recursively mark any datatypes as on disk/in memory
 USAGE
    htri_t H5T_set_loc(dt,f,loc)
        H5T_t *dt;              IN/OUT: Pointer to the datatype to mark
        H5F_t *f;               IN: Pointer to the file the datatype is in
        H5T_vlen_type_t loc     IN: location of type

 RETURNS
    One of two values on success:
        TRUE - If the location of any vlen types changed
        FALSE - If the location of any vlen types is the same
    <0 is returned on failure
 DESCRIPTION
    Recursively descends any VL or compound datatypes to mark all VL datatypes
    as either on disk or in memory.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5T_set_loc(H5T_t *dt, H5F_t *f, H5T_loc_t loc)
{
    htri_t changed;    /* Whether H5T_set_loc changed the type (even if the size didn't change) */
    htri_t ret_value = 0;   /* Indicate that success, but no location change */
    unsigned i;             /* Local index variable */
    int accum_change;       /* Amount of change in the offset of the fields */
    size_t old_size;        /* Previous size of a field */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(dt);
    HDassert(loc>=H5T_LOC_BADLOC && loc<H5T_LOC_MAXLOC);

    /* Datatypes can't change in size if the force_conv flag is not set */
    if(dt->shared->force_conv) {
        /* Check the datatype of this element */
        switch(dt->shared->type) {
            case H5T_ARRAY:  /* Recurse on VL, compound and array base element type */
                /* Recurse if it's VL, compound, enum or array */
                /* (If the force_conv flag is _not_ set, the type cannot change in size, so don't recurse) */
                if(dt->shared->parent->shared->force_conv && H5T_IS_COMPLEX(dt->shared->parent->shared->type)) {
                    /* Keep the old base element size for later */
                    old_size=dt->shared->parent->shared->size;

                    /* Mark the VL, compound or array type */
                    if((changed=H5T_set_loc(dt->shared->parent,f,loc))<0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "Unable to set VL location");
                    if(changed>0)
                        ret_value=changed;

                    /* Check if the field changed size */
                    if(old_size != dt->shared->parent->shared->size) {
                        /* Adjust the size of the array */
                        dt->shared->size = dt->shared->u.array.nelem*dt->shared->parent->shared->size;
                    } /* end if */
                } /* end if */
                break;

            case H5T_COMPOUND:  /* Check each field and recurse on VL, compound and array type */
                /* Sort the fields based on offsets */
                H5T__sort_value(dt, NULL);

                for (i=0,accum_change=0; i<dt->shared->u.compnd.nmembs; i++) {
                    H5T_t *memb_type;   /* Member's datatype pointer */

                    /* Apply the accumulated size change to the offset of the field */
                    dt->shared->u.compnd.memb[i].offset += accum_change;

                    /* Set the member type pointer (for convenience) */
                    memb_type=dt->shared->u.compnd.memb[i].type;

                    /* Recurse if it's VL, compound, enum or array */
                    /* (If the force_conv flag is _not_ set, the type cannot change in size, so don't recurse) */
                    if(memb_type->shared->force_conv && H5T_IS_COMPLEX(memb_type->shared->type)) {
                        /* Keep the old field size for later */
                        old_size=memb_type->shared->size;

                        /* Mark the VL, compound, enum or array type */
                        if((changed=H5T_set_loc(memb_type,f,loc))<0)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "Unable to set VL location");
                        if(changed>0)
                            ret_value=changed;

                        /* Check if the field changed size */
                        if(old_size != memb_type->shared->size) {
                            /* Adjust the size of the member */
                            dt->shared->u.compnd.memb[i].size = (dt->shared->u.compnd.memb[i].size*memb_type->shared->size)/old_size;

                            /* Add that change to the accumulated size change */
                            accum_change += (memb_type->shared->size - (int)old_size);
                        } /* end if */
                    } /* end if */
                } /* end for */

                /* Apply the accumulated size change to the datatype */
                dt->shared->size = (size_t)(dt->shared->size + accum_change);
                break;

            case H5T_VLEN: /* Recurse on the VL information if it's VL, compound or array, then free VL sequence */
                /* Recurse if it's VL, compound, enum or array */
                /* (If the force_conv flag is _not_ set, the type cannot change in size, so don't recurse) */
                if(dt->shared->parent->shared->force_conv && H5T_IS_COMPLEX(dt->shared->parent->shared->type)) {
                    if((changed=H5T_set_loc(dt->shared->parent,f,loc))<0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "Unable to set VL location");
                    if(changed>0)
                        ret_value=changed;
                } /* end if */

                /* Mark this VL sequence */
                if((changed = H5T__vlen_set_loc(dt, f, loc)) < 0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "Unable to set VL location");
                if(changed>0)
                    ret_value=changed;
                break;

            case H5T_REFERENCE:
                /* Only need to change location of object references */
                if(dt->shared->u.atomic.u.r.rtype==H5R_OBJECT) {
                    /* Mark this reference */
                    if(loc!=dt->shared->u.atomic.u.r.loc) {
                        /* Set the location */
                        dt->shared->u.atomic.u.r.loc = loc;

                        /* Indicate that the location changed */
                        ret_value=TRUE;
                    } /* end if */
                } /* end if */
                break;

            default:
                break;
        } /* end switch */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5T_set_loc() */


/*-------------------------------------------------------------------------
 * Function:    H5T_is_relocatable
 *
 * Purpose:     Check if a datatype will change between disk and memory.
 *
 * Notes:       Currently, only variable-length and object references change
 *              between disk & memory (see cases where things are changed in
 *              the H5T_set_loc() code above).
 *
 * Return:
 *  One of two values on success:
 *      TRUE - If the location of any vlen types changed
 *      FALSE - If the location of any vlen types is the same
 *  <0 is returned on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, June 24, 2004
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5T_is_relocatable(const H5T_t *dt)
{
    htri_t ret_value = FALSE;

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(dt);

    /* VL and reference datatypes are relocatable */
    if(H5T_detect_class(dt, H5T_VLEN, FALSE) || H5T_detect_class(dt, H5T_REFERENCE, FALSE))
        ret_value = TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_is_relocatable() */


/*-------------------------------------------------------------------------
 * Function:    H5T_upgrade_version_cb
 *
 * Purpose:     H5T__visit callback to Upgrade the version of a datatype
 *              (if there's any benefit to doing so)
 *
 * Note:	The behavior below is tightly coupled with the "better"
 *              encodings for datatype messages in the datatype message
 *              encoding routine.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, July 19, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T_upgrade_version_cb(H5T_t *dt, void *op_value)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(dt);
    HDassert(op_value);

    /* Special behavior for each type of datatype */
    switch(dt->shared->type) {
        case H5T_COMPOUND:
        case H5T_ARRAY:
        case H5T_ENUM:
            /* These types benefit from "upgrading" their version */
            if(*(unsigned *)op_value > dt->shared->version)
                dt->shared->version = *(unsigned *)op_value;
            break;

        case H5T_VLEN:
            if(dt->shared->parent->shared->version > dt->shared->version)
                dt->shared->version = dt->shared->parent->shared->version;
            break;

        default:
            break;
    } /* end switch */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5T_upgrade_version_cb() */


/*-------------------------------------------------------------------------
 * Function:    H5T__upgrade_version
 *
 * Purpose:     Upgrade the version of a datatype (if there's any benefit to
 *              doing so) and recursively apply to compound members and/or
 *              parent datatypes.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, July 19, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__upgrade_version(H5T_t *dt, unsigned new_version)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(dt);

    /* Iterate over entire datatype, upgrading the version of components, if it's useful */
    if(H5T__visit(dt, (H5T_VISIT_SIMPLE | H5T_VISIT_COMPLEX_LAST), H5T_upgrade_version_cb, &new_version) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_BADITER, FAIL, "iteration to upgrade datatype encoding version failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__upgrade_version() */


/*-------------------------------------------------------------------------
 * Function:    H5T_set_latest_version
 *
 * Purpose:     Set the encoding for a datatype to the latest version.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, July 19, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_set_latest_version(H5T_t *dt)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(dt);

    /* Upgrade the format version for the datatype to the latest */
    if(H5T__upgrade_version(dt, H5O_DTYPE_VERSION_LATEST) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTSET, FAIL, "can't upgrade datatype encoding")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_set_latest_version() */


/*-------------------------------------------------------------------------
 * Function:    H5T_patch_file
 *
 * Purpose:     Patch the top-level file pointers contained in dt to point
 *              to f, if dt is a committed type.  This is possible because
 *              the top-level file pointer can be closed out from under
 *              dt while dt is contained in the shared file's cache.
 *
 * Return:      SUCCEED
 *
 * Programmer:  Neil Fortner
 *              Thursday, July 14, 2011
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_patch_file(H5T_t *dt, H5F_t *f)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(dt);
    HDassert(f);

    if(H5T_STATE_OPEN == dt->shared->state || H5T_STATE_NAMED == dt->shared->state) {
        dt->oloc.file = f;
        dt->sh_loc.file = f;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_patch_file() */

