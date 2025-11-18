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

#ifndef H5Tconv_macros_H
#define H5Tconv_macros_H

/*
 * Purpose: Contains the macros and infrastructure that make up the atomic
 * datatype conversion mechanism
 */

/***********/
/* Headers */
/***********/
#include "H5private.h"  /* Generic Functions                        */
#include "H5Eprivate.h" /* Error Handling                           */
#include "H5Tprivate.h" /* Datatypes                                */

#ifdef H5T_DEBUG

/* Conversion debugging data for the hardware conversion functions */
typedef struct H5T_conv_hw_t {
    size_t s_aligned; /*number source elements aligned     */
    size_t d_aligned; /*number destination elements aligned*/
} H5T_conv_hw_t;

#endif

/*
 * These macros are for the bodies of functions that convert buffers of one
 * atomic type to another using hardware.
 *
 * They all start with `H5T_CONV_' and end with two letters that represent the
 * source and destination types, respectively. The letters `s' and `S' refer to
 * signed integers while the letters `u' and `U' refer to unsigned integers, and
 * the letters `f' and `F' refer to floating-point values.
 *
 * The letter which is capitalized indicates that the corresponding type
 * (source or destination) is at least as large as the other type.
 *
 * Certain conversions may experience overflow conditions which arise when the
 * source value has a magnitude that cannot be represented by the destination
 * type.
 *
 * Suffix    Description
 * ------    -----------
 * sS:       Signed integers to signed integers where the destination is
 *           at least as wide as the source. This case cannot generate
 *           overflows.
 *
 * sU:       Signed integers to unsigned integers where the destination is
 *           at least as wide as the source. This case experiences
 *           overflows when the source value is negative.
 *
 * uS:       Unsigned integers to signed integers where the destination is
 *           at least as wide as the source. This case can experience
 *           overflows when the source and destination are the same size.
 *
 * uU:       Unsigned integers to unsigned integers where the destination
 *           is at least as wide as the source. Overflows are not
 *           possible in this case.
 *
 * Ss:       Signed integers to signed integers where the source is at
 *           least as large as the destination. Overflows can occur when
 *           the destination is narrower than the source.
 *
 * Su:       Signed integers to unsigned integers where the source is at
 *           least as large as the destination. Overflows occur when the
 *           source value is negative and can also occur if the
 *           destination is narrower than the source.
 *
 * Us:       Unsigned integers to signed integers where the source is at
 *           least as large as the destination. Overflows can occur for
 *           all sizes.
 *
 * Uu:       Unsigned integers to unsigned integers where the source is at
 *           least as large as the destination. Overflows can occur if the
 *           destination is narrower than the source.
 *
 * su:       Conversion from signed integers to unsigned integers where
 *           the source and destination are the same size. Overflow occurs
 *           when the source value is negative.
 *
 * us:       Conversion from unsigned integers to signed integers where
 *           the source and destination are the same size. Overflow
 *           occurs when the source magnitude is too large for the
 *           destination.
 *
 * fF:       Floating-point values to floating-point values where the
 *           destination is at least as wide as the source. This case
 *           cannot generate overflows.
 *
 * Ff:       Floating-point values to floating-point values the source is at
 *           least as large as the destination. Overflows can occur when
 *           the destination is narrower than the source.
 *
 * xF:       Integers to float-point(float or double) values where the destination
 *           is at least as wide as the source. This case cannot generate
 *           overflows.
 *
 * Fx:       Float-point(float or double) values to integer where the source is
 *           at least as wide as the destination. Overflow can occur
 *           when the source magnitude is too large for the destination.
 *
 * fX:       Floating-point values to integers where the destination is at least
 *           as wide as the source. This case cannot generate overflows.
 *
 * Xf:       Integers to floating-point values where the source is at least as
 *           wide as the destination. Overflows can occur when the destination is
 *           narrower than the source.
 *
 *
 * The macros take a subset of these arguments in the order listed here:
 *
 * CDATA:    A pointer to the H5T_cdata_t structure that was passed to the
 *           conversion function.
 *
 * STYPE:    The hid_t value for the source datatype.
 *
 * DTYPE:    The hid_t value for the destination datatype.
 *
 * BUF:      A pointer to the conversion buffer.
 *
 * NELMTS:   The number of values to be converted.
 *
 * ST:       The C name for source datatype (e.g., int)
 *
 * DT:       The C name for the destination datatype (e.g., signed char)
 *
 * D_MIN:    The minimum possible destination value. For unsigned
 *           destination types this should be zero. For signed destination
 *           types it's a negative value with a magnitude that is usually
 *           one greater than D_MAX. Source values which are smaller than
 *           D_MIN generate overflows.
 *
 * D_MAX:    The maximum possible destination value. Source values which
 *           are larger than D_MAX generate overflows.
 *
 * The macros are implemented with a generic programming technique, similar
 * to templates in C++. The macro which defines the "core" part of the
 * conversion (which actually moves the data from the source to the destination)
 * is invoked inside the H5T_CONV "template" macro by "gluing" it together,
 * which allows the core conversion macro to be invoked as necessary.
 *
 * "Core" macros come in two flavors: one which calls the exception handling
 * routine and one which doesn't (the "_NOEX" variant). The presence of the
 * exception handling routine is detected before the loop over the values and
 * the appropriate core routine loop is executed.
 *
 * The generic "core" macros are: (others are specific to particular conversion)
 *
 * Suffix    Description
 * ------    -----------
 * xX:       Generic Conversion where the destination is at least as
 *           wide as the source. This case cannot generate overflows.
 *
 * Xx:       Generic signed conversion where the source is at least as large
 *           as the destination. Overflows can occur when the destination is
 *           narrower than the source.
 *
 * Ux:       Generic conversion for the `Us', `Uu' & `us' cases.
 *           Overflow occurs when the source magnitude is too large for the
 *           destination.
 *
 */
#define H5T_CONV_xX_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                           \
    {                                                                                                        \
        *(D) = (DT)(*(S));                                                                                   \
    }
#define H5T_CONV_xX_NOEX_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                      \
    {                                                                                                        \
        *(D) = (DT)(*(S));                                                                                   \
    }

/* Added a condition branch(else if (*(S) == (DT)(D_MAX))) which seems redundant.
 * It handles a special situation when the source is "float" and assigned the value
 * of "INT_MAX".  A compiler may do roundup making this value "INT_MAX+1".  However,
 * when do comparison "if (*(S) > (DT)(D_MAX))", the compiler may consider them
 * equal. In this case, do not return exception but make sure the maximum is assigned
 * to the destination.   SLU - 2005/06/29
 */
#define H5T_CONV_Xx_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                           \
    {                                                                                                        \
        if (*(S) > (ST)(D_MAX)) {                                                                            \
            H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                   \
                H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D,  \
                conv_ctx->u.conv.cb_struct.user_data);                                                       \
            if (except_ret == H5T_CONV_UNHANDLED)                                                            \
                /* Let compiler convert if case is ignored by user handler*/                                 \
                *(D) = (DT)(D_MAX);                                                                          \
            else if (except_ret == H5T_CONV_ABORT)                                                           \
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");       \
            /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                            \
        }                                                                                                    \
        else if (*(S) < (ST)(D_MIN)) {                                                                       \
            H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                   \
                H5T_CONV_EXCEPT_RANGE_LOW, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D, \
                conv_ctx->u.conv.cb_struct.user_data);                                                       \
            if (except_ret == H5T_CONV_UNHANDLED)                                                            \
                /* Let compiler convert if case is ignored by user handler*/                                 \
                *(D) = (DT)(D_MIN);                                                                          \
            else if (except_ret == H5T_CONV_ABORT)                                                           \
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");       \
            /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                            \
        }                                                                                                    \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }
#define H5T_CONV_Xx_NOEX_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                      \
    {                                                                                                        \
        if (*(S) > (ST)(D_MAX)) {                                                                            \
            *(D) = (DT)(D_MAX);                                                                              \
        }                                                                                                    \
        else if (*(S) < (ST)(D_MIN)) {                                                                       \
            *(D) = (DT)(D_MIN);                                                                              \
        }                                                                                                    \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }

#define H5T_CONV_Ux_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                           \
    {                                                                                                        \
        if (*(S) > (ST)(D_MAX)) {                                                                            \
            H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                   \
                H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D,  \
                conv_ctx->u.conv.cb_struct.user_data);                                                       \
            if (except_ret == H5T_CONV_UNHANDLED)                                                            \
                /* Let compiler convert if case is ignored by user handler*/                                 \
                *(D) = (DT)(D_MAX);                                                                          \
            else if (except_ret == H5T_CONV_ABORT)                                                           \
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");       \
            /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                            \
        }                                                                                                    \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }
#define H5T_CONV_Ux_NOEX_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                      \
    {                                                                                                        \
        if (*(S) > (ST)(D_MAX)) {                                                                            \
            *(D) = (DT)(D_MAX);                                                                              \
        }                                                                                                    \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }

#define H5T_CONV_sS(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        HDcompile_assert(sizeof(ST) <= sizeof(DT));                                                          \
        H5T_CONV(H5T_CONV_xX, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, N)                                         \
    } while (0)

#define H5T_CONV_sU_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                           \
    {                                                                                                        \
        if (*(S) < 0) {                                                                                      \
            H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                   \
                H5T_CONV_EXCEPT_RANGE_LOW, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D, \
                conv_ctx->u.conv.cb_struct.user_data);                                                       \
            if (except_ret == H5T_CONV_UNHANDLED)                                                            \
                /* Let compiler convert if case is ignored by user handler*/                                 \
                *(D) = 0;                                                                                    \
            else if (except_ret == H5T_CONV_ABORT)                                                           \
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");       \
            /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                            \
        }                                                                                                    \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }
#define H5T_CONV_sU_NOEX_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                      \
    {                                                                                                        \
        if (*(S) < 0)                                                                                        \
            *(D) = 0;                                                                                        \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }

#define H5T_CONV_sU(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        HDcompile_assert(sizeof(ST) <= sizeof(DT));                                                          \
        H5T_CONV(H5T_CONV_sU, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, N)                                         \
    } while (0)

/* Define to 1 if overflow is possible during conversion, 0 otherwise
 * Because destination is at least as wide as the source, this should only
 * occur between types of equal size */
#define H5T_CONV_uS_UCHAR_SHORT 0
#define H5T_CONV_uS_UCHAR_INT   0
#define H5T_CONV_uS_UCHAR_LONG  0
#define H5T_CONV_uS_UCHAR_LLONG 0
#if H5_SIZEOF_SHORT == H5_SIZEOF_INT
#define H5T_CONV_uS_USHORT_INT 1
#else
#define H5T_CONV_uS_USHORT_INT 0
#endif
#define H5T_CONV_uS_USHORT_LONG  0
#define H5T_CONV_uS_USHORT_LLONG 0
#if H5_SIZEOF_INT == H5_SIZEOF_LONG
#define H5T_CONV_uS_UINT_LONG 1
#else
#define H5T_CONV_uS_UINT_LONG 0
#endif
#define H5T_CONV_uS_UINT_LLONG 0
#if H5_SIZEOF_LONG == H5_SIZEOF_LONG_LONG
#define H5T_CONV_uS_ULONG_LLONG 1
#else
#define H5T_CONV_uS_ULONG_LLONG 0
#endif

/* Note. If an argument is stringified or concatenated, the prescan does not
 * occur. To expand the macro, then stringify or concatenate its expansion,
 * one macro must call another macro that does the stringification or
 * concatenation. */
#define H5T_CONV_uS_EVAL_TYPES(STYPE, DTYPE) H5_GLUE4(H5T_CONV_uS_, STYPE, _, DTYPE)

/* Called if overflow is possible */
#define H5T_CONV_uS_CORE_1(S, D, ST, DT, D_MIN, D_MAX)                                                       \
    if (*(S) > (DT)(D_MAX)) {                                                                                \
        H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                       \
            H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D,      \
            conv_ctx->u.conv.cb_struct.user_data);                                                           \
        if (except_ret == H5T_CONV_UNHANDLED)                                                                \
            /* Let compiler convert if case is ignored by user handler */                                    \
            *(D) = (DT)(D_MAX);                                                                              \
        else if (except_ret == H5T_CONV_ABORT)                                                               \
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");           \
        /* if (except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                               \
    }                                                                                                        \
    else                                                                                                     \
        *(D) = (DT)(*(S));

/* Called if no overflow is possible */
#define H5T_CONV_uS_CORE_0(S, D, ST, DT, D_MIN, D_MAX) *(D) = (DT)(*(S));

#define H5T_CONV_uS_CORE_I(over, S, D, ST, DT, D_MIN, D_MAX)                                                 \
    H5_GLUE(H5T_CONV_uS_CORE_, over)(S, D, ST, DT, D_MIN, D_MAX)

#define H5T_CONV_uS_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                           \
    {                                                                                                        \
        H5T_CONV_uS_CORE_I(H5T_CONV_uS_EVAL_TYPES(STYPE, DTYPE), S, D, ST, DT, D_MIN, D_MAX)                 \
    }

/* Called if overflow is possible */
#define H5T_CONV_uS_NOEX_CORE_1(S, D, ST, DT, D_MIN, D_MAX)                                                  \
    if (*(S) > (DT)(D_MAX))                                                                                  \
        *(D) = (D_MAX);                                                                                      \
    else                                                                                                     \
        *(D) = (DT)(*(S));

/* Called if no overflow is possible */
#define H5T_CONV_uS_NOEX_CORE_0(S, D, ST, DT, D_MIN, D_MAX) *(D) = (DT)(*(S));

#define H5T_CONV_uS_NOEX_CORE_I(over, S, D, ST, DT, D_MIN, D_MAX)                                            \
    H5_GLUE(H5T_CONV_uS_NOEX_CORE_, over)(S, D, ST, DT, D_MIN, D_MAX)

#define H5T_CONV_uS_NOEX_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                      \
    {                                                                                                        \
        H5T_CONV_uS_NOEX_CORE_I(H5T_CONV_uS_EVAL_TYPES(STYPE, DTYPE), S, D, ST, DT, D_MIN, D_MAX)            \
    }

#define H5T_CONV_uS(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        HDcompile_assert(sizeof(ST) <= sizeof(DT));                                                          \
        H5T_CONV(H5T_CONV_uS, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, N)                                         \
    } while (0)

#define H5T_CONV_uU(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        HDcompile_assert(sizeof(ST) <= sizeof(DT));                                                          \
        H5T_CONV(H5T_CONV_xX, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, N)                                         \
    } while (0)

#define H5T_CONV_Ss(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        HDcompile_assert(sizeof(ST) >= sizeof(DT));                                                          \
        H5T_CONV(H5T_CONV_Xx, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, N)                                         \
    } while (0)

#define H5T_CONV_Su_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                           \
    {                                                                                                        \
        if (*(S) < 0) {                                                                                      \
            H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                   \
                H5T_CONV_EXCEPT_RANGE_LOW, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D, \
                conv_ctx->u.conv.cb_struct.user_data);                                                       \
            if (except_ret == H5T_CONV_UNHANDLED)                                                            \
                /* Let compiler convert if case is ignored by user handler*/                                 \
                *(D) = 0;                                                                                    \
            else if (except_ret == H5T_CONV_ABORT)                                                           \
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");       \
            /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                            \
        }                                                                                                    \
        else if (sizeof(ST) > sizeof(DT) && *(S) > (ST)(D_MAX)) {                                            \
            H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                   \
                H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D,  \
                conv_ctx->u.conv.cb_struct.user_data);                                                       \
            if (except_ret == H5T_CONV_UNHANDLED)                                                            \
                /* Let compiler convert if case is ignored by user handler*/                                 \
                *(D) = (DT)(D_MAX);                                                                          \
            else if (except_ret == H5T_CONV_ABORT)                                                           \
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");       \
            /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                            \
        }                                                                                                    \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }
#define H5T_CONV_Su_NOEX_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                      \
    {                                                                                                        \
        if (*(S) < 0)                                                                                        \
            *(D) = 0;                                                                                        \
        else if (sizeof(ST) > sizeof(DT) && *(S) > (ST)(D_MAX))                                              \
            *(D) = (DT)(D_MAX);                                                                              \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }

#define H5T_CONV_Su(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        HDcompile_assert(sizeof(ST) >= sizeof(DT));                                                          \
        H5T_CONV(H5T_CONV_Su, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, N)                                         \
    } while (0)

#define H5T_CONV_Us(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        HDcompile_assert(sizeof(ST) >= sizeof(DT));                                                          \
        H5T_CONV(H5T_CONV_Ux, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, N)                                         \
    } while (0)

#define H5T_CONV_Uu(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        HDcompile_assert(sizeof(ST) >= sizeof(DT));                                                          \
        H5T_CONV(H5T_CONV_Ux, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, N)                                         \
    } while (0)

#define H5T_CONV_su_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                           \
    {                                                                                                        \
        /* Assumes memory format of unsigned & signed integers is same */                                    \
        if (*(S) < 0) {                                                                                      \
            H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                   \
                H5T_CONV_EXCEPT_RANGE_LOW, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D, \
                conv_ctx->u.conv.cb_struct.user_data);                                                       \
            if (except_ret == H5T_CONV_UNHANDLED)                                                            \
                /* Let compiler convert if case is ignored by user handler*/                                 \
                *(D) = 0;                                                                                    \
            else if (except_ret == H5T_CONV_ABORT)                                                           \
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");       \
            /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                            \
        }                                                                                                    \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }
#define H5T_CONV_su_NOEX_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                      \
    {                                                                                                        \
        /* Assumes memory format of unsigned & signed integers is same */                                    \
        if (*(S) < 0)                                                                                        \
            *(D) = 0;                                                                                        \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }

#define H5T_CONV_su(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        HDcompile_assert(sizeof(ST) == sizeof(DT));                                                          \
        H5T_CONV(H5T_CONV_su, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, N)                                         \
    } while (0)

#define H5T_CONV_us_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                           \
    {                                                                                                        \
        /* Assumes memory format of unsigned & signed integers is same */                                    \
        if (*(S) > (ST)(D_MAX)) {                                                                            \
            H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                   \
                H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D,  \
                conv_ctx->u.conv.cb_struct.user_data);                                                       \
            if (except_ret == H5T_CONV_UNHANDLED)                                                            \
                /* Let compiler convert if case is ignored by user handler*/                                 \
                *(D) = (DT)(D_MAX);                                                                          \
            else if (except_ret == H5T_CONV_ABORT)                                                           \
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");       \
            /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                            \
        }                                                                                                    \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }
#define H5T_CONV_us_NOEX_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                      \
    {                                                                                                        \
        /* Assumes memory format of unsigned & signed integers is same */                                    \
        if (*(S) > (ST)(D_MAX))                                                                              \
            *(D) = (DT)(D_MAX);                                                                              \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }

#define H5T_CONV_us(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        HDcompile_assert(sizeof(ST) == sizeof(DT));                                                          \
        H5T_CONV(H5T_CONV_us, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, N)                                         \
    } while (0)

#define H5T_CONV_fF(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        HDcompile_assert(sizeof(ST) <= sizeof(DT));                                                          \
        H5T_CONV(H5T_CONV_xX, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, N)                                         \
    } while (0)

/* Same as H5T_CONV_Xx_CORE, except that instead of using D_MAX and D_MIN
 * when an overflow occurs, use the 'float' infinity values.
 */
#define H5T_CONV_Ff_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                           \
    {                                                                                                        \
        if (*(S) > (ST)(D_MAX)) {                                                                            \
            H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                   \
                H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D,  \
                conv_ctx->u.conv.cb_struct.user_data);                                                       \
            if (except_ret == H5T_CONV_UNHANDLED)                                                            \
                /* Let compiler convert if case is ignored by user handler*/                                 \
                *(D) = H5_GLUE3(H5T_NATIVE_, DTYPE, _POS_INF_g);                                             \
            else if (except_ret == H5T_CONV_ABORT)                                                           \
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");       \
            /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                            \
        }                                                                                                    \
        else if (*(S) < (ST)(D_MIN)) {                                                                       \
            H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                   \
                H5T_CONV_EXCEPT_RANGE_LOW, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D, \
                conv_ctx->u.conv.cb_struct.user_data);                                                       \
            if (except_ret == H5T_CONV_UNHANDLED)                                                            \
                /* Let compiler convert if case is ignored by user handler*/                                 \
                *(D) = H5_GLUE3(H5T_NATIVE_, DTYPE, _NEG_INF_g);                                             \
            else if (except_ret == H5T_CONV_ABORT)                                                           \
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");       \
            /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                            \
        }                                                                                                    \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }
#define H5T_CONV_Ff_NOEX_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                      \
    {                                                                                                        \
        if (*(S) > (ST)(D_MAX))                                                                              \
            *(D) = H5_GLUE3(H5T_NATIVE_, DTYPE, _POS_INF_g);                                                 \
        else if (*(S) < (ST)(D_MIN))                                                                         \
            *(D) = H5_GLUE3(H5T_NATIVE_, DTYPE, _NEG_INF_g);                                                 \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }

#define H5T_CONV_Ff(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        HDcompile_assert(sizeof(ST) >= sizeof(DT));                                                          \
        H5T_CONV(H5T_CONV_Ff, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, N)                                         \
    } while (0)

#define H5T_HI_LO_BIT_SET(TYP, V, LO, HI)                                                                    \
    {                                                                                                        \
        unsigned      count;                                                                                 \
        unsigned char p;                                                                                     \
        unsigned      u;                                                                                     \
                                                                                                             \
        count = 0;                                                                                           \
        for (u = 0; u < sizeof(TYP); u++) {                                                                  \
            count = (((unsigned)sizeof(TYP) - 1) - u) * 8;                                                   \
            p     = (unsigned char)((V) >> count);                                                           \
            if (p > 0) {                                                                                     \
                if (p & 0x80)                                                                                \
                    count += 7;                                                                              \
                else if (p & 0x40)                                                                           \
                    count += 6;                                                                              \
                else if (p & 0x20)                                                                           \
                    count += 5;                                                                              \
                else if (p & 0x10)                                                                           \
                    count += 4;                                                                              \
                else if (p & 0x08)                                                                           \
                    count += 3;                                                                              \
                else if (p & 0x04)                                                                           \
                    count += 2;                                                                              \
                else if (p & 0x02)                                                                           \
                    count += 1;                                                                              \
                break;                                                                                       \
            } /* end if */                                                                                   \
        }     /* end for */                                                                                  \
                                                                                                             \
        HI = count;                                                                                          \
                                                                                                             \
        count = 0;                                                                                           \
        for (u = 0; u < sizeof(TYP); u++) {                                                                  \
            p = (unsigned char)((V) >> (u * 8));                                                             \
            if (p > 0) {                                                                                     \
                count = u * 8;                                                                               \
                                                                                                             \
                if (p & 0x01)                                                                                \
                    ;                                                                                        \
                else if (p & 0x02)                                                                           \
                    count += 1;                                                                              \
                else if (p & 0x04)                                                                           \
                    count += 2;                                                                              \
                else if (p & 0x08)                                                                           \
                    count += 3;                                                                              \
                else if (p & 0x10)                                                                           \
                    count += 4;                                                                              \
                else if (p & 0x20)                                                                           \
                    count += 5;                                                                              \
                else if (p & 0x40)                                                                           \
                    count += 6;                                                                              \
                else if (p & 0x80)                                                                           \
                    count += 7;                                                                              \
                break;                                                                                       \
            } /* end if */                                                                                   \
        }     /* end for */                                                                                  \
                                                                                                             \
        LO = count;                                                                                          \
    }

#define H5T_CONV_xF_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                           \
    {                                                                                                        \
        if (sprec > dprec) {                                                                                 \
            unsigned low_bit_pos, high_bit_pos;                                                              \
                                                                                                             \
            /* Detect high & low bits set in source */                                                       \
            H5T_HI_LO_BIT_SET(ST, *(S), low_bit_pos, high_bit_pos)                                           \
                                                                                                             \
            /* Check for more bits of precision in src than available in dst */                              \
            if ((high_bit_pos - low_bit_pos) >= dprec) {                                                     \
                H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                               \
                    H5T_CONV_EXCEPT_PRECISION, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id,   \
                    S, D, conv_ctx->u.conv.cb_struct.user_data);                                             \
                if (except_ret == H5T_CONV_UNHANDLED)                                                        \
                    /* Let compiler convert if case is ignored by user handler*/                             \
                    *(D) = (DT)(*(S));                                                                       \
                else if (except_ret == H5T_CONV_ABORT)                                                       \
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");   \
                /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                        \
            }                                                                                                \
            else                                                                                             \
                *(D) = (DT)(*(S));                                                                           \
        }                                                                                                    \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }
#define H5T_CONV_xF_NOEX_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                      \
    {                                                                                                        \
        *(D) = (DT)(*(S));                                                                                   \
    }

#define H5T_CONV_xF(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        H5T_CONV(H5T_CONV_xF, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, Y)                                         \
    } while (0)

/* Quincey added the condition branch (else if (*(S) != (ST)((DT)(*(S))))).
 * It handles a special situation when the source is "float" and assigned the value
 * of "INT_MAX".  Compilers do roundup making this value "INT_MAX+1".  This branch
 * is to check that situation and return exception for some compilers, mainly GCC.
 * The branch if (*(S) > (DT)(D_MAX) || (sprec < dprec && *(S) ==
 * (ST)(D_MAX))) is for some compilers like Sun, HP, IBM, and SGI where under
 * the same situation the "int" doesn't overflow.  SLU - 2005/9/12
 */
#define H5T_CONV_Fx_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                           \
    {                                                                                                        \
        if (*(S) > (ST)(D_MAX) || (sprec < dprec && *(S) == (ST)(D_MAX))) {                                  \
            H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                   \
                H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D,  \
                conv_ctx->u.conv.cb_struct.user_data);                                                       \
            if (except_ret == H5T_CONV_UNHANDLED)                                                            \
                /* Let compiler convert if case is ignored by user handler*/                                 \
                *(D) = (DT)(D_MAX);                                                                          \
            else if (except_ret == H5T_CONV_ABORT)                                                           \
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");       \
            /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                            \
        }                                                                                                    \
        else if (*(S) < (ST)(D_MIN)) {                                                                       \
            H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                   \
                H5T_CONV_EXCEPT_RANGE_LOW, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D, \
                conv_ctx->u.conv.cb_struct.user_data);                                                       \
            if (except_ret == H5T_CONV_UNHANDLED)                                                            \
                /* Let compiler convert if case is ignored by user handler*/                                 \
                *(D) = (DT)(D_MIN);                                                                          \
            else if (except_ret == H5T_CONV_ABORT)                                                           \
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");       \
            /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                            \
        }                                                                                                    \
        else if (*(S) != (ST)((DT)(*(S)))) {                                                                 \
            H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                   \
                H5T_CONV_EXCEPT_TRUNCATE, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D,  \
                conv_ctx->u.conv.cb_struct.user_data);                                                       \
            if (except_ret == H5T_CONV_UNHANDLED)                                                            \
                /* Let compiler convert if case is ignored by user handler*/                                 \
                *(D) = (DT)(*(S));                                                                           \
            else if (except_ret == H5T_CONV_ABORT)                                                           \
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");       \
            /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                            \
        }                                                                                                    \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }
#define H5T_CONV_Fx_NOEX_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                      \
    {                                                                                                        \
        if (*(S) > (ST)(D_MAX))                                                                              \
            *(D) = (DT)(D_MAX);                                                                              \
        else if (*(S) < (ST)(D_MIN))                                                                         \
            *(D) = (DT)(D_MIN);                                                                              \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }

#define H5T_CONV_Fx(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        H5T_CONV(H5T_CONV_Fx, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, Y)                                         \
    } while (0)

#define H5T_CONV_fX(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        HDcompile_assert(sizeof(ST) <= sizeof(DT));                                                          \
        H5T_CONV(H5T_CONV_xX, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, N)                                         \
    } while (0)

#define H5T_CONV_Xf_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                           \
    {                                                                                                        \
        if (*(S) > (ST)(D_MAX) || (sprec < dprec && *(S) == (ST)(D_MAX))) {                                  \
            H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                   \
                H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D,  \
                conv_ctx->u.conv.cb_struct.user_data);                                                       \
            if (except_ret == H5T_CONV_UNHANDLED)                                                            \
                /* Let compiler convert if case is ignored by user handler*/                                 \
                *(D) = H5_GLUE3(H5T_NATIVE_, DTYPE, _POS_INF_g);                                             \
            else if (except_ret == H5T_CONV_ABORT)                                                           \
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");       \
            /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                            \
        }                                                                                                    \
        else if (*(S) < (ST)(D_MIN)) {                                                                       \
            H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                                   \
                H5T_CONV_EXCEPT_RANGE_LOW, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, S, D, \
                conv_ctx->u.conv.cb_struct.user_data);                                                       \
            if (except_ret == H5T_CONV_UNHANDLED)                                                            \
                /* Let compiler convert if case is ignored by user handler*/                                 \
                *(D) = H5_GLUE3(H5T_NATIVE_, DTYPE, _NEG_INF_g);                                             \
            else if (except_ret == H5T_CONV_ABORT)                                                           \
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");       \
            /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                            \
        }                                                                                                    \
        else if (sprec > dprec) {                                                                            \
            unsigned low_bit_pos, high_bit_pos;                                                              \
                                                                                                             \
            /* Detect high & low bits set in source */                                                       \
            H5T_HI_LO_BIT_SET(ST, *(S), low_bit_pos, high_bit_pos)                                           \
                                                                                                             \
            /* Check for more bits of precision in src than available in dst */                              \
            if ((high_bit_pos - low_bit_pos) >= dprec) {                                                     \
                H5T_conv_ret_t except_ret = (conv_ctx->u.conv.cb_struct.func)(                               \
                    H5T_CONV_EXCEPT_PRECISION, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id,   \
                    S, D, conv_ctx->u.conv.cb_struct.user_data);                                             \
                if (except_ret == H5T_CONV_UNHANDLED)                                                        \
                    /* Let compiler convert if case is ignored by user handler*/                             \
                    *(D) = (DT)(*(S));                                                                       \
                else if (except_ret == H5T_CONV_ABORT)                                                       \
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");   \
                /* if(except_ret==H5T_CONV_HANDLED): Fall through, user handled it */                        \
            }                                                                                                \
            else                                                                                             \
                *(D) = (DT)(*(S));                                                                           \
        }                                                                                                    \
        else                                                                                                 \
            *(D) = (DT)(*(S));                                                                               \
    }
#define H5T_CONV_Xf_NOEX_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                      \
    {                                                                                                        \
        if (*(S) > (ST)(D_MAX))                                                                              \
            *(D) = H5_GLUE3(H5T_NATIVE_, DTYPE, _POS_INF_g);                                                 \
        else {                                                                                               \
            intmax_t s_cast = (intmax_t)(*(S));                                                              \
            intmax_t d_cast = (intmax_t)(D_MAX);                                                             \
                                                                                                             \
            /* Check if source value would underflow destination. Do NOT do this                             \
             * by comparing against D_MIN casted to type ST here, as this will                               \
             * generally be undefined behavior (casting negative float value <= 1.0                          \
             * to integer) for all floating point types and some compilers optimize                          \
             * this in a way that causes unexpected behavior. Instead, grab the                              \
             * absolute value of the source value first, then compare it to D_MAX.                           \
             */                                                                                              \
            if (s_cast != INTMAX_MIN)                                                                        \
                s_cast = imaxabs(s_cast);                                                                    \
            else {                                                                                           \
                /* Handle two's complement integer representations where abs(INTMAX_MIN)                     \
                 * can't be represented. Other representations will fall here as well,                       \
                 * but this should be fine.                                                                  \
                 */                                                                                          \
                s_cast = INTMAX_MAX;                                                                         \
                d_cast -= 1;                                                                                 \
            }                                                                                                \
                                                                                                             \
            if (s_cast > d_cast)                                                                             \
                *(D) = H5_GLUE3(H5T_NATIVE_, DTYPE, _NEG_INF_g);                                             \
            else                                                                                             \
                *(D) = (DT)(*(S));                                                                           \
        }                                                                                                    \
    }

#define H5T_CONV_Xf(STYPE, DTYPE, ST, DT, D_MIN, D_MAX)                                                      \
    do {                                                                                                     \
        HDcompile_assert(sizeof(ST) >= sizeof(DT));                                                          \
        H5T_CONV(H5T_CONV_Xf, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, Y)                                         \
    } while (0)

/* Since all "no exception" cores do the same thing (assign the value in the
 * source location to the destination location, using casting), use one "core"
 * to do them all.
 */
#ifndef H5_WANT_DCONV_EXCEPTION
#define H5T_CONV_NO_EXCEPT_CORE(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                    \
    {                                                                                                        \
        *(D) = (DT)(*(S));                                                                                   \
    }
#endif /* H5_WANT_DCONV_EXCEPTION */

/* Declare the source & destination precision variables */
#define H5T_CONV_DECL_PREC(PREC) H5_GLUE(H5T_CONV_DECL_PREC_, PREC)

#define H5T_CONV_DECL_PREC_Y                                                                                 \
    size_t      sprec;  /*source precision        */                                                         \
    size_t      dprec;  /*destination precision        */                                                    \
    H5T_class_t tclass; /*datatype's class        */

#define H5T_CONV_DECL_PREC_N /*no precision variables        */

/* Initialize the source & destination precision variables */
#define H5T_CONV_SET_PREC(PREC) H5_GLUE(H5T_CONV_SET_PREC_, PREC)

#define H5T_CONV_SET_PREC_Y                                                                                  \
    /* Get source & destination precisions into a variable */                                                \
    tclass = st->shared->type;                                                                               \
    assert(tclass == H5T_INTEGER || tclass == H5T_FLOAT);                                                    \
    if (tclass == H5T_INTEGER)                                                                               \
        sprec = st->shared->u.atomic.prec;                                                                   \
    else                                                                                                     \
        sprec = 1 + st->shared->u.atomic.u.f.msize;                                                          \
    tclass = dt->shared->type;                                                                               \
    assert(tclass == H5T_INTEGER || tclass == H5T_FLOAT);                                                    \
    if (tclass == H5T_INTEGER)                                                                               \
        dprec = dt->shared->u.atomic.prec;                                                                   \
    else                                                                                                     \
        dprec = 1 + dt->shared->u.atomic.u.f.msize;

#define H5T_CONV_SET_PREC_N /*don't init precision variables */

/* Macro defining action on source data which needs to be aligned (before main action) */
#define H5T_CONV_LOOP_PRE_SALIGN(ST)                                                                         \
    {                                                                                                        \
        /* The uint8_t * cast is required to avoid tripping over undefined behavior.                         \
         *                                                                                                   \
         * The typed pointer arrives via a void pointer, which may have any alignment.                       \
         * We then cast it to a pointer to a type that is assumed to be aligned, which                       \
         * is undefined behavior (section 6.3.2.3 paragraph 7 of the C99 standard).                          \
         * In the past this hasn't caused many problems, but in some cases (e.g.                             \
         * converting long doubles on macOS), an optimizing compiler might do the                            \
         * wrong thing (in the macOS case, the conversion uses SSE, which has stricter                       \
         * requirements about alignment).                                                                    \
         */                                                                                                  \
        H5MM_memcpy(&src_aligned, (const uint8_t *)src, sizeof(ST));                                         \
    }

/* Macro defining action on source data which doesn't need to be aligned (before main action) */
#define H5T_CONV_LOOP_PRE_SNOALIGN(ST)                                                                       \
    {                                                                                                        \
    }

/* Macro defining action on destination data which needs to be aligned (before main action) */
#define H5T_CONV_LOOP_PRE_DALIGN(DT)                                                                         \
    {                                                                                                        \
        d = &dst_aligned;                                                                                    \
    }

/* Macro defining action on destination data which doesn't need to be aligned (before main action) */
#define H5T_CONV_LOOP_PRE_DNOALIGN(DT)                                                                       \
    {                                                                                                        \
    }

/* Macro defining action on source data which needs to be aligned (after main action) */
#define H5T_CONV_LOOP_POST_SALIGN(ST)                                                                        \
    {                                                                                                        \
    }

/* Macro defining action on source data which doesn't need to be aligned (after main action) */
#define H5T_CONV_LOOP_POST_SNOALIGN(ST)                                                                      \
    {                                                                                                        \
    }

/* Macro defining action on destination data which needs to be aligned (after main action) */
#define H5T_CONV_LOOP_POST_DALIGN(DT)                                                                        \
    {                                                                                                        \
        /* The uint8_t * cast is required to avoid tripping over undefined behavior.                         \
         *                                                                                                   \
         * The typed pointer arrives via a void pointer, which may have any alignment.                       \
         * We then cast it to a pointer to a type that is assumed to be aligned, which                       \
         * is undefined behavior (section 6.3.2.3 paragraph 7 of the C99 standard).                          \
         * In the past this hasn't caused many problems, but in some cases (e.g.                             \
         * converting long doubles on macOS), an optimizing compiler might do the                            \
         * wrong thing (in the macOS case, the conversion uses SSE, which has stricter                       \
         * requirements about alignment).                                                                    \
         */                                                                                                  \
        H5MM_memcpy((uint8_t *)dst, &dst_aligned, sizeof(DT));                                               \
    }

/* Macro defining action on destination data which doesn't need to be aligned (after main action) */
#define H5T_CONV_LOOP_POST_DNOALIGN(DT)                                                                      \
    {                                                                                                        \
    }

/* The outer wrapper for the type conversion loop, to check for an exception handling routine */
#define H5T_CONV_LOOP_OUTER(PRE_SALIGN_GUTS, PRE_DALIGN_GUTS, POST_SALIGN_GUTS, POST_DALIGN_GUTS, GUTS,      \
                            STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                        \
    if (conv_ctx->u.conv.cb_struct.func) {                                                                   \
        H5T_CONV_LOOP(PRE_SALIGN_GUTS, PRE_DALIGN_GUTS, POST_SALIGN_GUTS, POST_DALIGN_GUTS, GUTS, STYPE,     \
                      DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                                     \
    }                                                                                                        \
    else {                                                                                                   \
        H5T_CONV_LOOP(PRE_SALIGN_GUTS, PRE_DALIGN_GUTS, POST_SALIGN_GUTS, POST_DALIGN_GUTS,                  \
                      H5_GLUE(GUTS, _NOEX), STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                        \
    }

/* Macro to call the actual "guts" of the type conversion, or call the "no exception" guts */
#ifdef H5_WANT_DCONV_EXCEPTION
#define H5T_CONV_LOOP_GUTS(GUTS, STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                   \
    /* ... user-defined stuff here -- the conversion ... */                                                  \
    H5_GLUE(GUTS, _CORE)(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)
#else /* H5_WANT_DCONV_EXCEPTION */
#define H5T_CONV_LOOP_GUTS(GUTS, STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                   \
    H5_GLUE(H5T_CONV_NO_EXCEPT, _CORE)(STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)
#endif /* H5_WANT_DCONV_EXCEPTION */

/* The inner loop of the type conversion macro, actually converting the elements */
#define H5T_CONV_LOOP(PRE_SALIGN_GUTS, PRE_DALIGN_GUTS, POST_SALIGN_GUTS, POST_DALIGN_GUTS, GUTS, STYPE,     \
                      DTYPE, S, D, ST, DT, D_MIN, D_MAX)                                                     \
    for (elmtno = 0; elmtno < safe; elmtno++) {                                                              \
        /* Handle source pre-alignment */                                                                    \
        H5_GLUE(H5T_CONV_LOOP_, PRE_SALIGN_GUTS)                                                             \
        (ST)                                                                                                 \
                                                                                                             \
            /* Handle destination pre-alignment */                                                           \
            H5_GLUE(H5T_CONV_LOOP_, PRE_DALIGN_GUTS)(DT)                                                     \
                                                                                                             \
            /* ... user-defined stuff here -- the conversion ... */                                          \
            H5T_CONV_LOOP_GUTS(GUTS, STYPE, DTYPE, S, D, ST, DT, D_MIN, D_MAX)                               \
                                                                                                             \
            /* Handle source post-alignment */                                                               \
            H5_GLUE(H5T_CONV_LOOP_, POST_SALIGN_GUTS)(ST)                                                    \
                                                                                                             \
            /* Handle destination post-alignment */                                                          \
            H5_GLUE(H5T_CONV_LOOP_, POST_DALIGN_GUTS)(DT)                                                    \
                                                                                                             \
            /* Advance pointers */                                                                           \
            src_buf = (void *)((uint8_t *)src_buf + s_stride);                                               \
        src         = (ST *)src_buf;                                                                         \
        dst_buf     = (void *)((uint8_t *)dst_buf + d_stride);                                               \
        dst         = (DT *)dst_buf;                                                                         \
    }

/* The main part of every integer hardware conversion macro */
#define H5T_CONV(GUTS, STYPE, DTYPE, ST, DT, D_MIN, D_MAX, PREC)                                             \
    {                                                                                                        \
        herr_t ret_value = SUCCEED; /* Return value         */                                               \
                                                                                                             \
        FUNC_ENTER_PACKAGE                                                                                   \
                                                                                                             \
        {                                                                                                    \
            size_t elmtno;              /*element number        */                                           \
            H5T_CONV_DECL_PREC(PREC)    /*declare precision variables, or not */                             \
            void   *src_buf;            /*'raw' source buffer        */                                      \
            void   *dst_buf;            /*'raw' destination buffer    */                                     \
            ST     *src, *s;            /*source buffer            */                                        \
            DT     *dst, *d;            /*destination buffer        */                                       \
            ST      src_aligned;        /*source aligned type        */                                      \
            DT      dst_aligned;        /*destination aligned type    */                                     \
            bool    s_mv, d_mv;         /*move data to align it?    */                                       \
            ssize_t s_stride, d_stride; /*src and dst strides        */                                      \
            size_t  safe;               /*how many elements are safe to process in each pass */              \
                                                                                                             \
            switch (cdata->command) {                                                                        \
                case H5T_CONV_INIT:                                                                          \
                    /* Sanity check and initialize statistics */                                             \
                    cdata->need_bkg = H5T_BKG_NO;                                                            \
                    if (NULL == st || NULL == dt)                                                            \
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid datatype");                   \
                    if (st->shared->size != sizeof(ST) || dt->shared->size != sizeof(DT))                    \
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "disagreement about datatype size");   \
                    CI_ALLOC_PRIV                                                                            \
                    break;                                                                                   \
                                                                                                             \
                case H5T_CONV_FREE:                                                                          \
                    /* Print and free statistics */                                                          \
                    CI_PRINT_STATS(STYPE, DTYPE);                                                            \
                    CI_FREE_PRIV                                                                             \
                    break;                                                                                   \
                                                                                                             \
                case H5T_CONV_CONV:                                                                          \
                    if (NULL == st || NULL == dt)                                                            \
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid datatype");                   \
                    if (NULL == conv_ctx)                                                                    \
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,                                        \
                                    "invalid datatype conversion context pointer");                          \
                                                                                                             \
                    /* Initialize source & destination strides */                                            \
                    if (buf_stride) {                                                                        \
                        assert(buf_stride >= sizeof(ST));                                                    \
                        assert(buf_stride >= sizeof(DT));                                                    \
                        s_stride = d_stride = (ssize_t)buf_stride;                                           \
                    }                                                                                        \
                    else {                                                                                   \
                        s_stride = sizeof(ST);                                                               \
                        d_stride = sizeof(DT);                                                               \
                    }                                                                                        \
                                                                                                             \
                    /* Is alignment required for source or dest? */                                          \
                    s_mv = H5T_NATIVE_##STYPE##_ALIGN_g > 1 &&                                               \
                           ((size_t)buf % H5T_NATIVE_##STYPE##_ALIGN_g ||                                    \
                            /* Cray */ ((size_t)((ST *)buf) != (size_t)buf) ||                               \
                            (size_t)s_stride % H5T_NATIVE_##STYPE##_ALIGN_g);                                \
                    d_mv = H5T_NATIVE_##DTYPE##_ALIGN_g > 1 &&                                               \
                           ((size_t)buf % H5T_NATIVE_##DTYPE##_ALIGN_g ||                                    \
                            /* Cray */ ((size_t)((DT *)buf) != (size_t)buf) ||                               \
                            (size_t)d_stride % H5T_NATIVE_##DTYPE##_ALIGN_g);                                \
                    CI_INC_SRC(s_mv)                                                                         \
                    CI_INC_DST(d_mv)                                                                         \
                                                                                                             \
                    H5T_CONV_SET_PREC(PREC) /*init precision variables, or not */                            \
                                                                                                             \
                    /* The outer loop of the type conversion macro, controlling which */                     \
                    /* direction the buffer is walked */                                                     \
                    while (nelmts > 0) {                                                                     \
                        /* Check if we need to go backwards through the buffer */                            \
                        if (d_stride > s_stride) {                                                           \
                            /* Compute the number of "safe" destination elements at */                       \
                            /* the end of the buffer (Those which don't overlap with */                      \
                            /* any source elements at the beginning of the buffer) */                        \
                            safe = nelmts - (((nelmts * (size_t)s_stride) + (size_t)(d_stride - 1)) /        \
                                             (size_t)d_stride);                                              \
                                                                                                             \
                            /* If we're down to the last few elements, just wrap up */                       \
                            /* with a "real" reverse copy */                                                 \
                            if (safe < 2) {                                                                  \
                                src      = (ST *)(src_buf = (void *)((uint8_t *)buf +                        \
                                                                (nelmts - 1) * (size_t)s_stride));      \
                                dst      = (DT *)(dst_buf = (void *)((uint8_t *)buf +                        \
                                                                (nelmts - 1) * (size_t)d_stride));      \
                                s_stride = -s_stride;                                                        \
                                d_stride = -d_stride;                                                        \
                                                                                                             \
                                safe = nelmts;                                                               \
                            } /* end if */                                                                   \
                            else {                                                                           \
                                src = (ST *)(src_buf = (void *)((uint8_t *)buf +                             \
                                                                (nelmts - safe) * (size_t)s_stride));        \
                                dst = (DT *)(dst_buf = (void *)((uint8_t *)buf +                             \
                                                                (nelmts - safe) * (size_t)d_stride));        \
                            } /* end else */                                                                 \
                        }     /* end if */                                                                   \
                        else {                                                                               \
                            /* Single forward pass over all data */                                          \
                            src  = (ST *)(src_buf = buf);                                                    \
                            dst  = (DT *)(dst_buf = buf);                                                    \
                            safe = nelmts;                                                                   \
                        } /* end else */                                                                     \
                                                                                                             \
                        /* Perform loop over elements to convert */                                          \
                        if (s_mv && d_mv) {                                                                  \
                            /* Alignment is required for both source and dest */                             \
                            s = &src_aligned;                                                                \
                            H5T_CONV_LOOP_OUTER(PRE_SALIGN, PRE_DALIGN, POST_SALIGN, POST_DALIGN, GUTS,      \
                                                STYPE, DTYPE, s, d, ST, DT, D_MIN, D_MAX)                    \
                        }                                                                                    \
                        else if (s_mv) {                                                                     \
                            /* Alignment is required only for source */                                      \
                            s = &src_aligned;                                                                \
                            H5T_CONV_LOOP_OUTER(PRE_SALIGN, PRE_DNOALIGN, POST_SALIGN, POST_DNOALIGN, GUTS,  \
                                                STYPE, DTYPE, s, dst, ST, DT, D_MIN, D_MAX)                  \
                        }                                                                                    \
                        else if (d_mv) {                                                                     \
                            /* Alignment is required only for destination */                                 \
                            H5T_CONV_LOOP_OUTER(PRE_SNOALIGN, PRE_DALIGN, POST_SNOALIGN, POST_DALIGN, GUTS,  \
                                                STYPE, DTYPE, src, d, ST, DT, D_MIN, D_MAX)                  \
                        }                                                                                    \
                        else {                                                                               \
                            /* Alignment is not required for both source and destination */                  \
                            H5T_CONV_LOOP_OUTER(PRE_SNOALIGN, PRE_DNOALIGN, POST_SNOALIGN, POST_DNOALIGN,    \
                                                GUTS, STYPE, DTYPE, src, dst, ST, DT, D_MIN, D_MAX)          \
                        }                                                                                    \
                                                                                                             \
                        /* Decrement number of elements left to convert */                                   \
                        nelmts -= safe;                                                                      \
                    } /* end while */                                                                        \
                    break;                                                                                   \
                                                                                                             \
                default:                                                                                     \
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");          \
            }                                                                                                \
        }                                                                                                    \
                                                                                                             \
done:                                                                                                        \
        FUNC_LEAVE_NOAPI(ret_value)                                                                          \
    }

#ifdef H5T_DEBUG

/* Print alignment statistics */
#define CI_PRINT_STATS(STYPE, DTYPE)                                                                         \
    do {                                                                                                     \
        if (H5DEBUG(T) && ((H5T_conv_hw_t *)cdata->priv)->s_aligned) {                                       \
            fprintf(H5DEBUG(T), "      %zu src elements aligned on %zu-byte boundaries\n",                   \
                    ((H5T_conv_hw_t *)cdata->priv)->s_aligned, H5T_NATIVE_##STYPE##_ALIGN_g);                \
        }                                                                                                    \
        if (H5DEBUG(T) && ((H5T_conv_hw_t *)cdata->priv)->d_aligned) {                                       \
            fprintf(H5DEBUG(T), "      %zu dst elements aligned on %zu-byte boundaries\n",                   \
                    ((H5T_conv_hw_t *)cdata->priv)->d_aligned, H5T_NATIVE_##DTYPE##_ALIGN_g);                \
        }                                                                                                    \
    } while (0)

/* Allocate private alignment structure for atomic types */
#define CI_ALLOC_PRIV                                                                                        \
    if (NULL == (cdata->priv = H5MM_calloc(sizeof(H5T_conv_hw_t)))) {                                        \
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");                            \
    }

/* Free private alignment structure for atomic types */
#define CI_FREE_PRIV                                                                                         \
    if (cdata->priv != NULL)                                                                                 \
        cdata->priv = H5MM_xfree(cdata->priv);

/* Increment source alignment counter */
#define CI_INC_SRC(s)                                                                                        \
    if (s)                                                                                                   \
        ((H5T_conv_hw_t *)cdata->priv)->s_aligned += nelmts;

/* Increment destination alignment counter */
#define CI_INC_DST(d)                                                                                        \
    if (d)                                                                                                   \
        ((H5T_conv_hw_t *)cdata->priv)->d_aligned += nelmts;
#else                                /* H5T_DEBUG */
#define CI_PRINT_STATS(STYPE, DTYPE) /*void*/
#define CI_ALLOC_PRIV                cdata->priv = NULL;
#define CI_FREE_PRIV                 /* void */
#define CI_INC_SRC(s)                /* void */
#define CI_INC_DST(d)                /* void */
#endif                               /* H5T_DEBUG */

#endif /* H5Tconv_macros_H */
