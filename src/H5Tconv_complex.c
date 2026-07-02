/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose: Datatype conversion functions for complex number datatypes
 */

/****************/
/* Module Setup */
/****************/
#include "H5Tmodule.h" /* This source code file is part of the H5T module */

/***********/
/* Headers */
/***********/
#include "H5private.h"  /*  Generic Functions                    */
#include "H5Eprivate.h" /* Error handling                       */
#include "H5Tconv.h"    /* Datatype conversions                 */
#include "H5Tconv_macros.h"
#include "H5Tconv_complex.h"
#include "H5Tconv_integer.h"
#include "H5Tconv_float.h"

/******************/
/* Local Typedefs */
/******************/

/********************/
/* Local Prototypes */
/********************/

static herr_t H5T__conv_complex_loop(const H5T_t *src_p, const H5T_t *dst_p, const H5T_conv_ctx_t *conv_ctx,
                                     size_t nelmts, size_t buf_stride, void *buf);
static herr_t H5T__conv_complex_part(const H5T_t *src_p, const H5T_t *dst_p, uint8_t *s, uint8_t *d,
                                     const H5T_conv_ctx_t *conv_ctx, uint8_t *src_rev,
                                     bool *exception_handled);

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_complex
 *
 * Purpose:     Convert one complex number type to another. This is the
 *              catch-all function for complex number conversions and is
 *              probably not particularly fast.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_complex(const H5T_t *src_p, const H5T_t *dst_p, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                  size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                  void H5_ATTR_UNUSED *bkg)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT: {
            H5T_atomic_t src_atomic; /* source datatype atomic info      */
            H5T_atomic_t dst_atomic; /* destination datatype atomic info */

            if (!src_p || !dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (!H5T_IS_ATOMIC(src_p->shared->parent->shared))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid source complex number datatype");
            if (!H5T_IS_ATOMIC(dst_p->shared->parent->shared))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid destination complex number datatype");
            src_atomic = src_p->shared->parent->shared->u.atomic;
            dst_atomic = dst_p->shared->parent->shared->u.atomic;
            if (H5T_ORDER_LE != src_atomic.order && H5T_ORDER_BE != src_atomic.order &&
                H5T_ORDER_VAX != src_atomic.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                            "unsupported byte order for source datatype");
            if (H5T_ORDER_LE != dst_atomic.order && H5T_ORDER_BE != dst_atomic.order &&
                H5T_ORDER_VAX != src_atomic.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                            "unsupported byte order for destination datatype");
            if (dst_p->shared->size > 2 * TEMP_FLOAT_CONV_BUFFER_SIZE)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "destination datatype size is too large");
            if (8 * sizeof(int64_t) - 1 < src_atomic.u.f.esize ||
                8 * sizeof(int64_t) - 1 < dst_atomic.u.f.esize)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "exponent field is too large");
            cdata->need_bkg = H5T_BKG_NO;

            break;
        }

        case H5T_CONV_FREE:
            break;

        case H5T_CONV_CONV:
            if (!src_p || !dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (NULL == conv_ctx)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid datatype conversion context pointer");

            if (H5T__conv_complex_loop(src_p, dst_p, conv_ctx, nelmts, buf_stride, buf) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "unable to convert data values");

            break;

        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_complex() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_complex_loop
 *
 * Purpose:     Implements the body of the conversion loop when converting
 *              complex number values to another complex number type.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T__conv_complex_loop(const H5T_t *src_p, const H5T_t *dst_p, const H5T_conv_ctx_t *conv_ctx, size_t nelmts,
                       size_t buf_stride, void *buf)
{
    H5T_conv_float_specval_t realval_type; /* floating-point value type (regular, +/-Inf, +/-0, NaN) */
    H5T_conv_float_specval_t imagval_type; /* floating-point value type (regular, +/-Inf, +/-0, NaN) */
    H5T_conv_ret_t           except_ret;   /* return of conversion exception callback function       */
    H5T_atomic_t             src_atomic;   /* source datatype atomic info                            */
    H5T_atomic_t             dst_atomic;   /* destination datatype atomic info                       */
    ssize_t  src_delta, dst_delta;         /* source & destination stride                            */
    uint8_t *s, *sp, *d, *dp;              /* source and dest traversal ptrs                         */
    uint8_t *src_rev = NULL;               /* order-reversed source buffer                           */
    uint8_t  dbuf[2 * TEMP_FLOAT_CONV_BUFFER_SIZE]; /* temp destination buffer */
    size_t   src_part_size; /* size of each complex number part                       */
    size_t   dst_part_size; /* size of each complex number part                       */
    size_t   olap;          /* num overlapping elements                               */
    int      direction;     /* forward or backward traversal                          */
    herr_t   ret_value = SUCCEED;

    assert(src_p);
    assert(src_p->shared->type == H5T_COMPLEX);
    assert(dst_p);
    assert(dst_p->shared->type == H5T_COMPLEX);
    assert(conv_ctx);
    assert(buf);

    FUNC_ENTER_PACKAGE

    src_atomic    = src_p->shared->parent->shared->u.atomic;
    dst_atomic    = dst_p->shared->parent->shared->u.atomic;
    src_part_size = src_p->shared->size / 2;
    dst_part_size = dst_p->shared->size / 2;

    /*
     * Do we process the values from beginning to end or vice versa? Also,
     * how many of the elements have the source and destination areas
     * overlapping?
     */
    if (src_p->shared->size == dst_p->shared->size || buf_stride) {
        sp = dp   = (uint8_t *)buf;
        direction = 1;
        olap      = nelmts;
    }
    else if (src_p->shared->size >= dst_p->shared->size) {
        double olap_d =
            ceil((double)(dst_p->shared->size) / (double)(src_p->shared->size - dst_p->shared->size));
        olap = (size_t)olap_d;
        sp = dp   = (uint8_t *)buf;
        direction = 1;
    }
    else {
        double olap_d =
            ceil((double)(src_p->shared->size) / (double)(dst_p->shared->size - src_p->shared->size));
        olap      = (size_t)olap_d;
        sp        = (uint8_t *)buf + (nelmts - 1) * src_p->shared->size;
        dp        = (uint8_t *)buf + (nelmts - 1) * dst_p->shared->size;
        direction = -1;
    }

    /* Direction & size of buffer traversal */
    H5_CHECK_OVERFLOW(buf_stride, size_t, ssize_t);
    H5_CHECK_OVERFLOW(src_p->shared->size, size_t, ssize_t);
    H5_CHECK_OVERFLOW(dst_p->shared->size, size_t, ssize_t);
    src_delta = (ssize_t)direction * (ssize_t)(buf_stride ? buf_stride : src_p->shared->size);
    dst_delta = (ssize_t)direction * (ssize_t)(buf_stride ? buf_stride : dst_p->shared->size);

    /* Allocate space for order-reversed source buffer */
    if (conv_ctx->u.conv.cb_struct.func)
        if (NULL == (src_rev = H5MM_calloc(src_p->shared->size)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTALLOC, FAIL, "couldn't allocate temporary buffer");

    /* The conversion loop */
    for (size_t elmtno = 0; elmtno < nelmts; elmtno++) {
        bool reverse     = true;  /* if reversed the order of destination            */
        bool real_zero   = false; /* if real part is +/-0                            */
        bool imag_zero   = false; /* if imaginary part is +/-0                       */
        bool real_except = false; /* if an exception happened for the real part      */
        bool imag_except = false; /* if an exception happened for the imaginary part */

        /*
         * If the source and destination buffers overlap then use a
         * temporary buffer for the destination.
         */
        s = sp;
        if (direction > 0)
            d = elmtno < olap ? dbuf : dp;
        else
            d = elmtno + olap >= nelmts ? dbuf : dp;
        if (d == dbuf)
            memset(dbuf, 0, sizeof(dbuf));

#ifndef NDEBUG
        if (d == dbuf) {
            assert((dp >= sp && dp < sp + src_p->shared->size) ||
                   (sp >= dp && sp < dp + dst_p->shared->size));
        }
        else {
            assert((dp < sp && dp + dst_p->shared->size <= sp) ||
                   (sp < dp && sp + src_p->shared->size <= dp));
        }
#endif

        /*
         * Put the data in little endian order so our loops aren't so
         * complicated. We'll do all the conversion stuff assuming
         * little endian and then we'll fix the order at the end.
         */
        if (H5T_ORDER_BE == src_atomic.order) {
            uint8_t *cur_part = s;
            /* Swap real part of complex number element */
            for (size_t j = 0; j < src_part_size / 2; j++)
                H5_SWAP_BYTES(cur_part, j, src_part_size - (j + 1));
            /* Swap imaginary part of complex number element */
            cur_part += src_part_size;
            for (size_t j = 0; j < src_part_size / 2; j++)
                H5_SWAP_BYTES(cur_part, j, src_part_size - (j + 1));
        }
        else if (H5T_ORDER_VAX == src_atomic.order)
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                        "VAX byte ordering is unsupported for complex number type conversions");

        /* Check for special cases: +0, -0, +Inf, -Inf, NaN */
        realval_type = H5T__conv_float_find_special(s, &src_atomic, NULL);
        imagval_type = H5T__conv_float_find_special(s + (src_p->shared->size / 2), &src_atomic, NULL);

        real_zero = (realval_type == H5T_CONV_FLOAT_SPECVAL_POSZERO ||
                     realval_type == H5T_CONV_FLOAT_SPECVAL_NEGZERO);
        imag_zero = (imagval_type == H5T_CONV_FLOAT_SPECVAL_POSZERO ||
                     imagval_type == H5T_CONV_FLOAT_SPECVAL_NEGZERO);
        real_except =
            (realval_type == H5T_CONV_FLOAT_SPECVAL_POSINF || realval_type == H5T_CONV_FLOAT_SPECVAL_NEGINF ||
             realval_type == H5T_CONV_FLOAT_SPECVAL_NAN);
        imag_except =
            (imagval_type == H5T_CONV_FLOAT_SPECVAL_POSINF || imagval_type == H5T_CONV_FLOAT_SPECVAL_NEGINF ||
             imagval_type == H5T_CONV_FLOAT_SPECVAL_NAN);

        /* A complex number is zero if both parts are +/-0 */
        if (real_zero && imag_zero) {
            H5T__bit_copy(d, dst_atomic.u.f.sign, s, src_atomic.u.f.sign, (size_t)1);
            H5T__bit_copy(d + dst_part_size, dst_atomic.u.f.sign, s + src_part_size, src_atomic.u.f.sign,
                          (size_t)1);
            H5T__bit_set(d, dst_atomic.u.f.epos, dst_atomic.u.f.esize, false);
            H5T__bit_set(d + dst_part_size, dst_atomic.u.f.epos, dst_atomic.u.f.esize, false);
            H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
            H5T__bit_set(d + dst_part_size, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
            goto padding;
        }
        else if (real_except || imag_except) {
            except_ret = H5T_CONV_UNHANDLED;

            /* If user's exception handler is present, use it */
            if (conv_ctx->u.conv.cb_struct.func) {
                H5T_conv_except_t except_type; /* type of conversion exception that occurred */

                /* Reverse source buffer order first */
                H5T__reverse_order(src_rev, s, src_p);

                /*
                 * A complex number is infinity if either part is infinity,
                 * even if the other part is NaN. If a part is infinity,
                 * since we can only throw one type of conversion exception,
                 * arbitrarily choose the exception type to throw based
                 * on the infinity type for the real part (if it's infinity),
                 * followed by the infinity type for the imaginary part. For
                 * now, it will be assumed that the conversion exception
                 * callback will inspect and handle both parts of the complex
                 * number value.
                 */
                if (realval_type == H5T_CONV_FLOAT_SPECVAL_POSINF)
                    except_type = H5T_CONV_EXCEPT_PINF;
                else if (realval_type == H5T_CONV_FLOAT_SPECVAL_NEGINF)
                    except_type = H5T_CONV_EXCEPT_NINF;
                else if (imagval_type == H5T_CONV_FLOAT_SPECVAL_POSINF)
                    except_type = H5T_CONV_EXCEPT_PINF;
                else if (imagval_type == H5T_CONV_FLOAT_SPECVAL_NEGINF)
                    except_type = H5T_CONV_EXCEPT_NINF;
                else {
                    assert(realval_type == H5T_CONV_FLOAT_SPECVAL_NAN ||
                           imagval_type == H5T_CONV_FLOAT_SPECVAL_NAN);
                    except_type = H5T_CONV_EXCEPT_NAN;
                }

                /* Prepare & restore library for user callback */
                H5_BEFORE_USER_CB(FAIL)
                    {
                        except_ret = (conv_ctx->u.conv.cb_struct.func)(
                            except_type, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, src_rev,
                            d, conv_ctx->u.conv.cb_struct.user_data);
                    }
                H5_AFTER_USER_CB(FAIL)
            }

            if (except_ret == H5T_CONV_UNHANDLED) {
                if (realval_type == H5T_CONV_FLOAT_SPECVAL_POSINF ||
                    realval_type == H5T_CONV_FLOAT_SPECVAL_NEGINF ||
                    imagval_type == H5T_CONV_FLOAT_SPECVAL_POSINF ||
                    imagval_type == H5T_CONV_FLOAT_SPECVAL_NEGINF) {
                    H5T__bit_copy(d, dst_atomic.u.f.sign, s, src_atomic.u.f.sign, (size_t)1);
                    H5T__bit_copy(d + dst_part_size, dst_atomic.u.f.sign, s + src_part_size,
                                  src_atomic.u.f.sign, (size_t)1);

                    if (realval_type == H5T_CONV_FLOAT_SPECVAL_POSINF ||
                        realval_type == H5T_CONV_FLOAT_SPECVAL_NEGINF) {
                        H5T__bit_set(d, dst_atomic.u.f.epos, dst_atomic.u.f.esize, true);
                        H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
                        /* If the destination has no implied mantissa bit, we'll need to set
                         * the 1st bit of mantissa to 1. The Intel-Linux "long double" is
                         * this case. */
                        if (H5T_NORM_NONE == dst_atomic.u.f.norm)
                            H5T__bit_set(d, dst_atomic.u.f.mpos + dst_atomic.u.f.msize - 1, (size_t)1, true);
                    }
                    if (imagval_type == H5T_CONV_FLOAT_SPECVAL_POSINF ||
                        imagval_type == H5T_CONV_FLOAT_SPECVAL_NEGINF) {
                        H5T__bit_set(d + dst_part_size, dst_atomic.u.f.epos, dst_atomic.u.f.esize, true);
                        H5T__bit_set(d + dst_part_size, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
                        /* If the destination has no implied mantissa bit, we'll need to set
                         * the 1st bit of mantissa to 1. The Intel-Linux "long double" is
                         * this case. */
                        if (H5T_NORM_NONE == dst_atomic.u.f.norm)
                            H5T__bit_set(d + dst_part_size, dst_atomic.u.f.mpos + dst_atomic.u.f.msize - 1,
                                         (size_t)1, true);
                    }
                }
                else {
                    /* There are many NaN values, so we just set all bits of the significand. */
                    if (realval_type == H5T_CONV_FLOAT_SPECVAL_NAN) {
                        H5T__bit_copy(d, dst_atomic.u.f.sign, s, src_atomic.u.f.sign, (size_t)1);
                        H5T__bit_set(d, dst_atomic.u.f.epos, dst_atomic.u.f.esize, true);
                        H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, true);
                    }
                    if (imagval_type == H5T_CONV_FLOAT_SPECVAL_NAN) {
                        H5T__bit_copy(d + dst_part_size, dst_atomic.u.f.sign, s + src_part_size,
                                      src_atomic.u.f.sign, (size_t)1);
                        H5T__bit_set(d + dst_part_size, dst_atomic.u.f.epos, dst_atomic.u.f.esize, true);
                        H5T__bit_set(d + dst_part_size, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, true);
                    }
                }
            }
            else if (except_ret == H5T_CONV_HANDLED) {
                /* No need to reverse the order of destination because user handles it */
                reverse = false;
                goto next;
            }
            else if (except_ret == H5T_CONV_ABORT)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");

            goto padding;
        }

        if (real_zero) {
            H5T__bit_copy(d, dst_atomic.u.f.sign, s, src_atomic.u.f.sign, (size_t)1);
            H5T__bit_set(d, dst_atomic.u.f.epos, dst_atomic.u.f.esize, false);
            H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
        }
        else {
            bool exception_handled = false;

            if (H5T__conv_complex_part(src_p, dst_p, s, d, conv_ctx, src_rev, &exception_handled) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't convert real part of complex number");

            /* If an exception was handled, go to the next element */
            if (exception_handled) {
                reverse = false;
                goto next;
            }
        }

        if (imag_zero) {
            H5T__bit_copy(d + dst_part_size, dst_atomic.u.f.sign, s + src_part_size, src_atomic.u.f.sign,
                          (size_t)1);
            H5T__bit_set(d + dst_part_size, dst_atomic.u.f.epos, dst_atomic.u.f.esize, false);
            H5T__bit_set(d + dst_part_size, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
        }
        else {
            bool exception_handled = false;

            if (H5T__conv_complex_part(src_p, dst_p, s + src_part_size, d + dst_part_size, conv_ctx, src_rev,
                                       &exception_handled) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                            "can't convert imaginary part of complex number");

            /* If an exception was handled, go to the next element */
            if (exception_handled) {
                reverse = false;
                goto next;
            }
        }

padding:
        /*
         * Set external padding areas
         */
        if (dst_atomic.offset > 0) {
            assert(H5T_PAD_ZERO == dst_atomic.lsb_pad || H5T_PAD_ONE == dst_atomic.lsb_pad);
            H5T__bit_set(d, (size_t)0, dst_atomic.offset, (bool)(H5T_PAD_ONE == dst_atomic.lsb_pad));
            H5T__bit_set(d + dst_part_size, (size_t)0, dst_atomic.offset,
                         (bool)(H5T_PAD_ONE == dst_atomic.lsb_pad));
        }
        {
            size_t type_size = dst_p->shared->parent->shared->size;

            if (dst_atomic.offset + dst_atomic.prec != 8 * type_size) {
                assert(H5T_PAD_ZERO == dst_atomic.msb_pad || H5T_PAD_ONE == dst_atomic.msb_pad);
                H5T__bit_set(d, dst_atomic.offset + dst_atomic.prec,
                             8 * type_size - (dst_atomic.offset + dst_atomic.prec),
                             (bool)(H5T_PAD_ONE == dst_atomic.msb_pad));
                H5T__bit_set(d + dst_part_size, dst_atomic.offset + dst_atomic.prec,
                             8 * type_size - (dst_atomic.offset + dst_atomic.prec),
                             (bool)(H5T_PAD_ONE == dst_atomic.msb_pad));
            }
        }

        /* Put the destination in the correct byte order. See note at beginning of loop. */
        if (H5T_ORDER_BE == dst_atomic.order && reverse) {
            uint8_t *cur_part = d;
            /* Swap real part of complex number element */
            for (size_t j = 0; j < dst_part_size / 2; j++)
                H5_SWAP_BYTES(cur_part, j, dst_part_size - (j + 1));
            /* Swap imaginary part of complex number element */
            cur_part += dst_part_size;
            for (size_t j = 0; j < dst_part_size / 2; j++)
                H5_SWAP_BYTES(cur_part, j, dst_part_size - (j + 1));
        }
        else if (H5T_ORDER_VAX == dst_atomic.order)
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                        "VAX byte ordering is unsupported for complex number type conversions");

next:
        /*
         * If we had used a temporary buffer for the destination then we
         * should copy the value to the true destination buffer.
         */
        if (d == dbuf)
            H5MM_memcpy(dp, d, dst_p->shared->size);

        /* Advance source & destination pointers by delta amounts */
        sp += src_delta;
        dp += dst_delta;
    } /* end conversion loop */

done:
    H5MM_free(src_rev);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_complex_loop() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_complex_part
 *
 * Purpose:     Helper function to convert a single part (real or
 *              imaginary) of a complex number.
 *
 * NOTE:        The conversion logic in this function is essentially
 *              identical to the logic in the H5T__conv_f_f_loop function.
 *              However, conversion has to be performed on both the real
 *              and imaginary parts of each complex number element. Since
 *              complex numbers have the same representation as an array
 *              of two elements of the base floating-point type, this could
 *              be simulated in some cases with the H5T__conv_f_f_loop
 *              function by doubling the number of elements to be converted
 *              and halving the sizes involved. However, overlapping
 *              elements or a non-zero `buf_stride` value would complicate
 *              the buffer pointer advancements since each part of the
 *              complex number value has to be processed before advancing
 *              the buffer pointer. Conversion exceptions also pose a
 *              problem since both parts of the complex number have to be
 *              taken into account when determining if an exception
 *              occurred. Application conversion exception callbacks would
 *              also expect to receive an entire complex number rather than
 *              part of one. Therefore, the H5T__conv_f_f_loop logic is
 *              mostly duplicated here and fixes to one function should be
 *              made to the other, if appropriate.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T__conv_complex_part(const H5T_t *src_p, const H5T_t *dst_p, uint8_t *s, uint8_t *d,
                       const H5T_conv_ctx_t *conv_ctx, uint8_t *src_rev, bool *exception_handled)
{
    H5T_conv_ret_t except_ret = H5T_CONV_UNHANDLED; /* return of conversion exception callback function */
    H5T_atomic_t   src_atomic;                      /* source datatype atomic info                      */
    H5T_atomic_t   dst_atomic;                      /* destination datatype atomic info                 */
    hssize_t       expo_max;                        /* maximum possible dst exponent                    */
    ssize_t        mant_msb = 0;                    /* most significant bit set in mantissa             */
    int64_t        expo;                            /* exponent                                         */
    size_t         msize = 0;                       /* useful size of mantissa in src                   */
    size_t         mpos;                            /* offset to useful mant in src                     */
    size_t         mrsh;                            /* amount to right shift mantissa                   */
    size_t         implied;                         /* destination implied bits                         */
    bool           denormalized = false;            /* is either source or destination denormalized?    */
    bool           carry        = false;            /* carry after rounding mantissa                    */
    herr_t         ret_value    = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(src_p);
    assert(dst_p);
    assert(s);
    assert(d);
    assert(conv_ctx);
    assert(exception_handled);

    if (conv_ctx->u.conv.cb_struct.func)
        assert(src_rev);

    *exception_handled = false;

    src_atomic = src_p->shared->parent->shared->u.atomic;
    dst_atomic = dst_p->shared->parent->shared->u.atomic;
    expo_max   = ((hssize_t)1 << dst_atomic.u.f.esize) - 1;

    /*
     * Get the exponent as an unsigned quantity from the section of
     * the source bit field where it's located. Don't worry about
     * the exponent bias yet.
     */
    expo = (int64_t)H5T__bit_get_d(s, src_atomic.u.f.epos, src_atomic.u.f.esize);

    if (expo == 0)
        denormalized = true;

    /* Determine size of mantissa */
    if (0 == expo || H5T_NORM_NONE == src_atomic.u.f.norm) {
        if ((mant_msb = H5T__bit_find(s, src_atomic.u.f.mpos, src_atomic.u.f.msize, H5T_BIT_MSB, true)) > 0)
            msize = (size_t)mant_msb;
        else if (0 == mant_msb) {
            msize = 1;
            H5T__bit_set(s, src_atomic.u.f.mpos, (size_t)1, false);
        }
    }
    else if (H5T_NORM_IMPLIED == src_atomic.u.f.norm)
        msize = src_atomic.u.f.msize;
    else
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "normalization method not implemented yet");

    /*
     * The sign for the destination is the same as the sign for the
     * source in all cases.
     */
    H5T__bit_copy(d, dst_atomic.u.f.sign, s, src_atomic.u.f.sign, (size_t)1);

    /*
     * Calculate the true source exponent by adjusting according to
     * the source exponent bias.
     */
    if (0 == expo || H5T_NORM_NONE == src_atomic.u.f.norm) {
        assert(mant_msb >= 0);
        expo -= (int64_t)((src_atomic.u.f.ebias - 1) + (src_atomic.u.f.msize - (size_t)mant_msb));
    }
    else if (H5T_NORM_IMPLIED == src_atomic.u.f.norm)
        expo -= (int64_t)src_atomic.u.f.ebias;
    else
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "normalization method not implemented yet");

    /*
     * If the destination is not normalized then right shift the
     * mantissa by one.
     */
    mrsh = 0;
    if (H5T_NORM_NONE == dst_atomic.u.f.norm)
        mrsh++;

    /*
     * Calculate the destination exponent by adding the destination
     * bias and clipping by the minimum and maximum possible
     * destination exponent values.
     */
    expo += (int64_t)dst_atomic.u.f.ebias;

    if (expo < -(hssize_t)(dst_atomic.u.f.msize)) {
        /* The exponent is way too small. Result is zero. */
        expo = 0;
        H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
        msize = 0;
    }
    else if (expo <= 0) {
        /*
         * The exponent is too small to fit in the exponent field,
         * but by shifting the mantissa to the right we can
         * accommodate that value. The mantissa of course is no
         * longer normalized.
         */
        mrsh += (size_t)(1 - expo);
        expo         = 0;
        denormalized = true;
    }
    else if (expo >= expo_max) {
        /*
         * The exponent is too large to fit in the available region
         * or it results in the maximum possible value. Use positive
         * or negative infinity instead unless the application
         * specifies something else. Before calling the overflow
         * handler make sure the source buffer we hand it is in the
         * original byte order.
         */
        if (conv_ctx->u.conv.cb_struct.func) { /* If user's exception handler is present, use it */
            /* Reverse source buffer order first */
            H5T__reverse_order(src_rev, s, src_p);

            /* Prepare & restore library for user callback */
            H5_BEFORE_USER_CB(FAIL)
                {
                    except_ret = (conv_ctx->u.conv.cb_struct.func)(
                        H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id,
                        src_rev, d, conv_ctx->u.conv.cb_struct.user_data);
                }
            H5_AFTER_USER_CB(FAIL)
        }

        if (except_ret == H5T_CONV_UNHANDLED) {
            expo = expo_max;
            H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
            msize = 0;
        }
        else if (except_ret == H5T_CONV_HANDLED) {
            *exception_handled = true;
            goto done;
        }
        else if (except_ret == H5T_CONV_ABORT)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");
    }

    /*
     * If the destination mantissa is smaller than the source
     * mantissa then round the source mantissa. Rounding may cause a
     * carry in which case the exponent has to be re-evaluated for
     * overflow. That is, if `carry' is clear then the implied
     * mantissa bit is `1', else it is `10' binary.
     */
    implied = 1;
    mpos    = src_atomic.u.f.mpos;
    if (msize > 0 && mrsh <= dst_atomic.u.f.msize && mrsh + msize > dst_atomic.u.f.msize) {
        mant_msb = (ssize_t)(mrsh + msize - dst_atomic.u.f.msize);
        assert(mant_msb >= 0 && (size_t)mant_msb <= msize);
        /* If the 1st bit being cut off is set and source isn't denormalized. */
        if (H5T__bit_get_d(s, (mpos + (size_t)mant_msb) - 1, (size_t)1) && !denormalized) {
            /* Don't do rounding if exponent is 111...110 and mantissa is 111...11.
             * To do rounding and increment exponent in this case will create an infinity value. */
            if ((H5T__bit_find(s, mpos + (size_t)mant_msb, msize - (size_t)mant_msb, H5T_BIT_LSB, false) >=
                     0 ||
                 expo < expo_max - 1)) {
                carry = H5T__bit_inc(s, mpos + (size_t)mant_msb - 1, 1 + msize - (size_t)mant_msb);
                if (carry)
                    implied = 2;
            }
        }
        else if (H5T__bit_get_d(s, (mpos + (size_t)mant_msb) - 1, (size_t)1) && denormalized)
            /* For either source or destination, denormalized value doesn't increment carry. */
            H5T__bit_inc(s, mpos + (size_t)mant_msb - 1, 1 + msize - (size_t)mant_msb);
    }
    else
        carry = false;

    /* Write the mantissa to the destination */
    if (mrsh > dst_atomic.u.f.msize + 1) {
        H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
    }
    else if (mrsh == dst_atomic.u.f.msize + 1) {
        H5T__bit_set(d, dst_atomic.u.f.mpos + 1, dst_atomic.u.f.msize - 1, false);
        H5T__bit_set(d, dst_atomic.u.f.mpos, (size_t)1, true);
    }
    else if (mrsh == dst_atomic.u.f.msize) {
        H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
        H5T__bit_set_d(d, dst_atomic.u.f.mpos, MIN(2, dst_atomic.u.f.msize), (hsize_t)implied);
    }
    else {
        if (mrsh > 0) {
            H5T__bit_set(d, dst_atomic.u.f.mpos + dst_atomic.u.f.msize - mrsh, mrsh, false);
            H5T__bit_set_d(d, dst_atomic.u.f.mpos + dst_atomic.u.f.msize - mrsh, (size_t)2, (hsize_t)implied);
        }
        if (mrsh + msize >= dst_atomic.u.f.msize) {
            H5T__bit_copy(d, dst_atomic.u.f.mpos, s, (mpos + msize + mrsh - dst_atomic.u.f.msize),
                          dst_atomic.u.f.msize - mrsh);
        }
        else {
            H5T__bit_copy(d, dst_atomic.u.f.mpos + dst_atomic.u.f.msize - (mrsh + msize), s, mpos, msize);
            H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize - (mrsh + msize), false);
        }
    }

    /* Write the exponent */
    if (carry) {
        expo++;
        if (expo >= expo_max) {
            /*
             * The exponent is too large to fit in the available
             * region or it results in the maximum possible value.
             * Use positive or negative infinity instead unless the
             * application specifies something else. Before calling
             * the overflow handler make sure the source buffer we
             * hand it is in the original byte order.
             */
            if (conv_ctx->u.conv.cb_struct.func) { /* If user's exception handler is present, use it */
                /* Reverse source buffer order first */
                H5T__reverse_order(src_rev, s, src_p);

                /* Prepare & restore library for user callback */
                H5_BEFORE_USER_CB(FAIL)
                    {
                        except_ret = (conv_ctx->u.conv.cb_struct.func)(
                            H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                            conv_ctx->u.conv.dst_type_id, src_rev, d, conv_ctx->u.conv.cb_struct.user_data);
                    }
                H5_AFTER_USER_CB(FAIL)
            }

            if (except_ret == H5T_CONV_UNHANDLED) {
                expo = expo_max;
                H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
            }
            else if (except_ret == H5T_CONV_HANDLED) {
                *exception_handled = true;
                goto done;
            }
            else if (except_ret == H5T_CONV_ABORT)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");
        }
    }

    H5_CHECK_OVERFLOW(expo, hssize_t, hsize_t);
    H5T__bit_set_d(d, dst_atomic.u.f.epos, dst_atomic.u.f.esize, (hsize_t)expo);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_complex_i
 *
 * Purpose:     Convert complex number values to integer values. This is
 *              the catch-all function for complex number -> integer
 *              conversions and is probably not particularly fast.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_complex_i(const H5T_t *src_p, const H5T_t *dst_p, H5T_cdata_t *cdata,
                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                    size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT: {
            H5T_atomic_t src_atomic; /* source datatype atomic info      */
            H5T_atomic_t dst_atomic; /* destination datatype atomic info */

            if (!src_p || !dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (!H5T_IS_ATOMIC(src_p->shared->parent->shared))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid complex number datatype");
            src_atomic = src_p->shared->parent->shared->u.atomic;
            dst_atomic = dst_p->shared->u.atomic;
            if (H5T_ORDER_LE != src_atomic.order && H5T_ORDER_BE != src_atomic.order &&
                H5T_ORDER_VAX != src_atomic.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                            "unsupported byte order for source datatype");
            if (H5T_ORDER_LE != dst_atomic.order && H5T_ORDER_BE != dst_atomic.order &&
                H5T_ORDER_VAX != dst_atomic.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                            "unsupported byte order for destination datatype");
            if (dst_p->shared->size > TEMP_INT_CONV_BUFFER_SIZE)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "destination datatype size is too large");
            if (8 * sizeof(hssize_t) - 1 < src_atomic.u.f.esize)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "exponent field is too large");
            cdata->need_bkg = H5T_BKG_NO;

            break;
        }

        case H5T_CONV_FREE:
            break;

        case H5T_CONV_CONV:
            if (!src_p || !dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (NULL == conv_ctx)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid datatype conversion context pointer");

            if (H5T__conv_f_i_loop(src_p, dst_p, conv_ctx, nelmts, buf_stride, buf) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "unable to convert data values");

            break;

        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_complex_i() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_complex_f
 *
 * Purpose:     Convert complex number values to floating-point values.
 *              This is the catch-all function for complex number -> float
 *              conversions and is probably not particularly fast.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_complex_f(const H5T_t *src_p, const H5T_t *dst_p, H5T_cdata_t *cdata,
                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                    size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    bool   equal_cplx_conv = false; /* if converting between complex and matching float */
    herr_t ret_value       = SUCCEED;

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT: {
            H5T_atomic_t src_atomic; /* source datatype atomic info                      */
            H5T_atomic_t dst_atomic; /* destination datatype atomic info                 */

            if (!src_p || !dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (!H5T_IS_ATOMIC(src_p->shared->parent->shared))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid complex number datatype");
            src_atomic = src_p->shared->parent->shared->u.atomic;
            dst_atomic = dst_p->shared->u.atomic;
            if (H5T_ORDER_LE != src_atomic.order && H5T_ORDER_BE != src_atomic.order &&
                H5T_ORDER_VAX != src_atomic.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                            "unsupported byte order for source datatype");
            if (H5T_ORDER_LE != dst_atomic.order && H5T_ORDER_BE != dst_atomic.order &&
                H5T_ORDER_VAX != dst_atomic.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                            "unsupported byte order for destination datatype");
            if (dst_p->shared->size > TEMP_FLOAT_CONV_BUFFER_SIZE)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "destination datatype size is too large");
            if (8 * sizeof(int64_t) - 1 < src_atomic.u.f.esize ||
                8 * sizeof(int64_t) - 1 < dst_atomic.u.f.esize)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "exponent field is too large");
            cdata->need_bkg = H5T_BKG_NO;

            break;
        }

        case H5T_CONV_FREE:
            break;

        case H5T_CONV_CONV:
            if (!src_p || !dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (NULL == conv_ctx)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid datatype conversion context pointer");

            /* Are we converting between a floating-point type and a complex number
             * type consisting of the same floating-point type?
             */
            equal_cplx_conv = (0 == H5T_cmp(src_p->shared->parent, dst_p, false));
            if (!equal_cplx_conv) {
                /* If floating-point types differ, use generic f_f loop */
                if (H5T__conv_f_f_loop(src_p, dst_p, conv_ctx, nelmts, buf_stride, buf) < 0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "unable to convert data values");
            }
            else {
                /* If floating-point types are the same, use specialized loop */
                if (H5T__conv_complex_f_matched(src_p, dst_p, conv_ctx, nelmts, buf_stride, buf) < 0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "unable to convert data values");
            }

            break;

        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_complex_f() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_complex_f_matched
 *
 * Purpose:     Implements the body of the conversion loop when converting
 *              between a floating-point type and a complex number type
 *              consisting of the same floating-point type. Encapsulates
 *              common code that is shared between the H5T__conv_complex_f
 *              and H5T__conv_f_complex functions. Values can be directly
 *              converted between the types after checking for conversion
 *              exceptions.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_complex_f_matched(const H5T_t *src_p, const H5T_t *dst_p, const H5T_conv_ctx_t *conv_ctx,
                            size_t nelmts, size_t buf_stride, void *buf)
{
    H5T_conv_float_specval_t specval_type;      /* floating-point value type (regular, +/-Inf, +/-0, NaN) */
    H5T_atomic_t             src_atomic;        /* source datatype atomic info                            */
    H5T_atomic_t             dst_atomic;        /* destination datatype atomic info                       */
    ssize_t  src_delta, dst_delta;              /* source & destination stride                            */
    uint8_t *s, *sp, *d, *dp;                   /* source and dest traversal ptrs                         */
    uint8_t *src_rev = NULL;                    /* order-reversed source buffer                           */
    uint8_t  dbuf[TEMP_FLOAT_CONV_BUFFER_SIZE]; /* temp destination buffer                                */
    size_t   olap;                              /* num overlapping elements                               */
    int      direction;                         /* forward or backward traversal                          */
    herr_t   ret_value = SUCCEED;

    assert(src_p);
    assert(src_p->shared->type == H5T_FLOAT || src_p->shared->type == H5T_COMPLEX);
    assert(dst_p);
    assert(dst_p->shared->type == H5T_FLOAT || dst_p->shared->type == H5T_COMPLEX);
    assert(conv_ctx);
    assert(buf);

    FUNC_ENTER_PACKAGE

    if (src_p->shared->type == H5T_COMPLEX)
        src_atomic = src_p->shared->parent->shared->u.atomic;
    else
        src_atomic = src_p->shared->u.atomic;
    if (dst_p->shared->type == H5T_COMPLEX)
        dst_atomic = dst_p->shared->parent->shared->u.atomic;
    else
        dst_atomic = dst_p->shared->u.atomic;

#ifndef NDEBUG
    {
        /* Make sure the floating-point types match */
        const H5T_t *src_base = (src_p->shared->type == H5T_FLOAT) ? src_p : src_p->shared->parent;
        const H5T_t *dst_base = (dst_p->shared->type == H5T_FLOAT) ? dst_p : dst_p->shared->parent;
        assert(0 == (H5T_cmp(src_base, dst_base, false)));
    }
#endif

    /*
     * Do we process the values from beginning to end or vice versa? Also,
     * how many of the elements have the source and destination areas
     * overlapping?
     */
    if (src_p->shared->size == dst_p->shared->size || buf_stride) {
        sp = dp   = (uint8_t *)buf;
        direction = 1;
        olap      = nelmts;
    }
    else if (src_p->shared->size >= dst_p->shared->size) {
        double olap_d =
            ceil((double)(dst_p->shared->size) / (double)(src_p->shared->size - dst_p->shared->size));
        olap = (size_t)olap_d;
        sp = dp   = (uint8_t *)buf;
        direction = 1;
    }
    else {
        double olap_d =
            ceil((double)(src_p->shared->size) / (double)(dst_p->shared->size - src_p->shared->size));
        olap      = (size_t)olap_d;
        sp        = (uint8_t *)buf + (nelmts - 1) * src_p->shared->size;
        dp        = (uint8_t *)buf + (nelmts - 1) * dst_p->shared->size;
        direction = -1;
    }

    /* Direction & size of buffer traversal */
    H5_CHECK_OVERFLOW(buf_stride, size_t, ssize_t);
    H5_CHECK_OVERFLOW(src_p->shared->size, size_t, ssize_t);
    H5_CHECK_OVERFLOW(dst_p->shared->size, size_t, ssize_t);
    src_delta = (ssize_t)direction * (ssize_t)(buf_stride ? buf_stride : src_p->shared->size);
    dst_delta = (ssize_t)direction * (ssize_t)(buf_stride ? buf_stride : dst_p->shared->size);

    /* Allocate space for order-reversed source buffer */
    if (conv_ctx->u.conv.cb_struct.func)
        if (NULL == (src_rev = H5MM_calloc(src_p->shared->size)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTALLOC, FAIL, "couldn't allocate temporary buffer");

    /* The conversion loop */
    for (size_t elmtno = 0; elmtno < nelmts; elmtno++) {
        H5T_conv_ret_t except_ret = H5T_CONV_UNHANDLED; /* return of conversion exception callback function */
        bool           reverse    = true;               /* if reversed the order of destination             */

        /*
         * If the source and destination buffers overlap then use a
         * temporary buffer for the destination.
         */
        s = sp;
        if (direction > 0)
            d = elmtno < olap ? dbuf : dp;
        else
            d = elmtno + olap >= nelmts ? dbuf : dp;
        if (d == dbuf)
            memset(dbuf, 0, sizeof(dbuf));

#ifndef NDEBUG
        if (d == dbuf) {
            assert((dp >= sp && dp < sp + src_p->shared->size) ||
                   (sp >= dp && sp < dp + dst_p->shared->size));
        }
        else {
            assert((dp < sp && dp + dst_p->shared->size <= sp) ||
                   (sp < dp && sp + src_p->shared->size <= dp));
        }
#endif

        /*
         * Put the data in little endian order so our loops aren't so
         * complicated. We'll do all the conversion stuff assuming
         * little endian and then we'll fix the order at the end.
         */
        if (H5T_ORDER_BE == src_atomic.order) {
            size_t half_size = src_p->shared->size / 2;

            if (H5T_FLOAT == src_p->shared->type) {
                for (size_t j = 0; j < half_size; j++)
                    H5_SWAP_BYTES(s, j, src_p->shared->size - (j + 1));
            }
            else {
                uint8_t *cur_part = s;
                /* Swap real part of complex number element */
                for (size_t j = 0; j < half_size / 2; j++)
                    H5_SWAP_BYTES(cur_part, j, half_size - (j + 1));
                /* Swap imaginary part of complex number element */
                cur_part += half_size;
                for (size_t j = 0; j < half_size / 2; j++)
                    H5_SWAP_BYTES(cur_part, j, half_size - (j + 1));
            }
        }
        else if (H5T_ORDER_VAX == src_atomic.order) {
            if (H5T_FLOAT == src_p->shared->type) {
                uint8_t tmp1, tmp2;
                size_t  tsize = src_p->shared->size;
                assert(0 == tsize % 2);

                for (size_t i = 0; i < tsize; i += 4) {
                    tmp1 = s[i];
                    tmp2 = s[i + 1];

                    s[i]     = s[(tsize - 2) - i];
                    s[i + 1] = s[(tsize - 1) - i];

                    s[(tsize - 2) - i] = tmp1;
                    s[(tsize - 1) - i] = tmp2;
                }
            }
            else
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                            "VAX byte ordering is unsupported for complex number type conversions");
        }

        /* Check for special cases: +0, -0, +Inf, -Inf, NaN */
        specval_type = H5T__conv_float_find_special(s, &src_atomic, NULL);
        if (specval_type == H5T_CONV_FLOAT_SPECVAL_POSZERO ||
            specval_type == H5T_CONV_FLOAT_SPECVAL_NEGZERO) {
            H5T__bit_copy(d, dst_atomic.u.f.sign, s, src_atomic.u.f.sign, (size_t)1);
            H5T__bit_set(d, dst_atomic.u.f.epos, dst_atomic.u.f.esize, false);
            H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
            goto padding;
        }
        else if (specval_type != H5T_CONV_FLOAT_SPECVAL_REGULAR) {
            /* If user's exception handler is present, use it */
            if (conv_ctx->u.conv.cb_struct.func) {
                H5T_conv_except_t except_type; /* type of conversion exception that occurred */

                /* Reverse source buffer order first */
                H5T__reverse_order(src_rev, s, src_p);

                if (specval_type == H5T_CONV_FLOAT_SPECVAL_POSINF)
                    except_type = H5T_CONV_EXCEPT_PINF;
                else if (specval_type == H5T_CONV_FLOAT_SPECVAL_NEGINF)
                    except_type = H5T_CONV_EXCEPT_NINF;
                else
                    except_type = H5T_CONV_EXCEPT_NAN;

                /* Prepare & restore library for user callback */
                H5_BEFORE_USER_CB(FAIL)
                    {
                        except_ret = (conv_ctx->u.conv.cb_struct.func)(
                            except_type, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id, src_rev,
                            d, conv_ctx->u.conv.cb_struct.user_data);
                    }
                H5_AFTER_USER_CB(FAIL)
            }

            if (except_ret == H5T_CONV_UNHANDLED) {
                H5T__bit_copy(d, dst_atomic.u.f.sign, s, src_atomic.u.f.sign, (size_t)1);
                H5T__bit_set(d, dst_atomic.u.f.epos, dst_atomic.u.f.esize, true);
                if (specval_type == H5T_CONV_FLOAT_SPECVAL_NAN)
                    /* There are many NaN values, so we just set all bits of the significand. */
                    H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, true);
                else {
                    /* +/-Inf */
                    H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
                    /* If the destination has no implied mantissa bit, we'll need to set
                     * the 1st bit of mantissa to 1. The Intel-Linux "long double" is
                     * this case. */
                    if (H5T_NORM_NONE == dst_atomic.u.f.norm)
                        H5T__bit_set(d, dst_atomic.u.f.mpos + dst_atomic.u.f.msize - 1, (size_t)1, true);
                }
            }
            else if (except_ret == H5T_CONV_HANDLED) {
                /* No need to reverse the order of destination because user handles it */
                reverse = false;
                goto next;
            }
            else if (except_ret == H5T_CONV_ABORT)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");

            goto padding;
        }

        /* Direct copy between complex number and floating-point type */
        if (H5T_FLOAT == src_p->shared->type)
            memcpy(d, s, src_p->shared->size);
        else
            memcpy(d, s, src_p->shared->size / 2);

padding:
        /*
         * Set external padding areas
         */
        if (dst_atomic.offset > 0) {
            assert(H5T_PAD_ZERO == dst_atomic.lsb_pad || H5T_PAD_ONE == dst_atomic.lsb_pad);
            H5T__bit_set(d, (size_t)0, dst_atomic.offset, (bool)(H5T_PAD_ONE == dst_atomic.lsb_pad));
        }
        {
            size_t type_size;

            if (dst_p->shared->type == H5T_FLOAT)
                type_size = dst_p->shared->size;
            else
                type_size = dst_p->shared->parent->shared->size;

            if (dst_atomic.offset + dst_atomic.prec != 8 * type_size) {
                assert(H5T_PAD_ZERO == dst_atomic.msb_pad || H5T_PAD_ONE == dst_atomic.msb_pad);
                H5T__bit_set(d, dst_atomic.offset + dst_atomic.prec,
                             8 * type_size - (dst_atomic.offset + dst_atomic.prec),
                             (bool)(H5T_PAD_ONE == dst_atomic.msb_pad));
            }
        }

        /*
         * Put the destination in the correct byte order. See note at
         * beginning of loop. Only the "real" part of a complex number
         * element is swapped. By the C standard, the "imaginary" part
         * should just be zeroed when converting a real value to a
         * complex value.
         */
        if (H5T_ORDER_BE == dst_atomic.order && reverse) {
            size_t half_size = dst_p->shared->size / 2;

            if (H5T_FLOAT == dst_p->shared->type) {
                for (size_t j = 0; j < half_size; j++)
                    H5_SWAP_BYTES(d, j, dst_p->shared->size - (j + 1));
            }
            else {
                for (size_t j = 0; j < half_size / 2; j++)
                    H5_SWAP_BYTES(d, j, half_size - (j + 1));
            }
        }
        else if (H5T_ORDER_VAX == dst_atomic.order && reverse) {
            if (H5T_FLOAT == dst_p->shared->type) {
                uint8_t tmp1, tmp2;
                size_t  tsize = dst_p->shared->size / 2;
                assert(0 == tsize % 2);

                for (size_t i = 0; i < tsize; i += 4) {
                    tmp1 = d[i];
                    tmp2 = d[i + 1];

                    d[i]     = d[(tsize - 2) - i];
                    d[i + 1] = d[(tsize - 1) - i];

                    d[(tsize - 2) - i] = tmp1;
                    d[(tsize - 1) - i] = tmp2;
                }
            }
            else
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                            "VAX byte ordering is unsupported for complex number type conversions");
        }

next:
        /*
         * If we had used a temporary buffer for the destination then we
         * should copy the value to the true destination buffer.
         */
        if (d == dbuf) {
            if (H5T_FLOAT == dst_p->shared->type)
                H5MM_memcpy(dp, d, dst_p->shared->size);
            else
                H5MM_memcpy(dp, d, dst_p->shared->size / 2);
        }

        /* Ensure imaginary part of complex number is zeroed */
        if (H5T_COMPLEX == dst_p->shared->type)
            memset(dp + (dst_p->shared->size / 2), 0, dst_p->shared->size / 2);

        /* Advance source & destination pointers by delta amounts */
        sp += src_delta;
        dp += dst_delta;
    } /* end conversion loop */

done:
    H5MM_free(src_rev);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_complex_f_matched() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_complex_compat
 *
 * Purpose:     Performs a no-op conversion between a complex number type
 *              and an equivalent datatype. Complex number types are
 *              considered equivalent to the following:
 *
 *              - An array datatype consisting of two elements where each
 *                element is of the same floating-point datatype as the
 *                complex number type's base floating-point datatype
 *
 *              - A compound datatype consisting of two fields where each
 *                field is of the same floating-point datatype as the
 *                complex number type's base floating-point datatype. The
 *                compound datatype must not have any leading or trailing
 *                structure padding or any padding between its two fields.
 *                The fields must also have compatible names, must have
 *                compatible offsets within the datatype and must be in
 *                the order of "real" part -> "imaginary" part, such that
 *                the compound datatype matches the following representation:
 *
 *                H5T_COMPOUND {
 *                    <float_type> "r(e)(a)(l)";                OFFSET 0
 *                    <float_type> "i(m)(a)(g)(i)(n)(a)(r)(y)"; OFFSET SIZEOF("r(e)(a)(l)")
 *                }
 *
 *                where "r(e)(a)(l)" means the field may be named any
 *                substring of "real", such as "r", or "re" and
 *                "i(m)(a)(g)(i)(n)(a)(r)(y)" means the field may be named
 *                any substring of "imaginary", such as "im" or "imag".
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_complex_compat(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                         const H5T_conv_ctx_t H5_ATTR_UNUSED *conv_ctx, size_t H5_ATTR_UNUSED nelmts,
                         size_t H5_ATTR_UNUSED buf_stride, size_t H5_ATTR_UNUSED bkg_stride,
                         void H5_ATTR_UNUSED *_buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_t *compound_copy = NULL;
    herr_t ret_value     = SUCCEED;

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT: {
            const H5T_t *complex_type;
            const H5T_t *other_type;

            if (src->shared->type == H5T_COMPLEX) {
                if (dst->shared->type != H5T_ARRAY && dst->shared->type != H5T_COMPOUND)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                "unsupported destination datatype for conversion");
                complex_type = src;
                other_type   = dst;
            }
            else {
                if (dst->shared->type != H5T_COMPLEX)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                "unsupported destination datatype for conversion");
                complex_type = dst;
                other_type   = src;
            }

            if (complex_type->shared->u.cplx.form != H5T_COMPLEX_RECTANGULAR)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                            "unsupported form of complex number datatype for conversion");

            if (other_type->shared->type == H5T_ARRAY) {
                if (other_type->shared->u.array.nelem != 2)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                "array datatype doesn't have the correct number of elements for conversion");
                if (H5T_cmp(other_type->shared->parent, complex_type->shared->parent, false))
                    HGOTO_ERROR(
                        H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                        "array datatype's base datatype doesn't match complex number type's base datatype");
            }
            else {
                H5T_cmemb_t *fields;
                size_t       name_len;

                assert(other_type->shared->type == H5T_COMPOUND);

                if (other_type->shared->u.compnd.nmembs != 2)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                "compound datatype doesn't have the correct number of fields for conversion");
                if (!other_type->shared->u.compnd.packed)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                "compound datatype fields aren't packed together");
                if (other_type->shared->u.compnd.memb_size != complex_type->shared->size)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                "compound datatype size doesn't match size of complex number datatype");

                /* Make sure members are unsorted or sorted according to
                 * their offsets before checking their names
                 */
                if (other_type->shared->u.compnd.sorted == H5T_SORT_NONE ||
                    other_type->shared->u.compnd.sorted == H5T_SORT_VALUE)
                    fields = other_type->shared->u.compnd.memb;
                else {
                    /* Make a copy so the sort order of the original type isn't disturbed */
                    if (NULL == (compound_copy = H5T_copy(other_type, H5T_COPY_TRANSIENT)))
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, FAIL, "can't copy datatype");

                    H5T__sort_value(compound_copy, NULL);
                    fields = compound_copy->shared->u.compnd.memb;
                }

                /* Check "real" part of compound datatype */
                if (fields[0].offset != 0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                "compound datatype's 'real' field is not at offset 0");
                if (fields[0].size != complex_type->shared->parent->shared->size)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                "compound datatype's 'real' field is not the same size as the complex number "
                                "datatype's base datatype");

                /* Match up to 5 characters (including the NUL terminator) from the
                 * field name to a substring of "real".
                 */
                name_len = strlen(fields[0].name);
                if (strncmp(fields[0].name, "real", (name_len < 5) ? name_len : 5))
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                "compound datatype's 'real' field name ('%s') didn't match an expected name "
                                "for conversion",
                                fields[0].name);

                if (H5T_cmp(fields[0].type, complex_type->shared->parent, false))
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                "compound datatype's 'real' field is not the same datatype as the complex "
                                "number datatype's base datatype");

                /* Check "imaginary" part of compound datatype */
                if (fields[1].offset != fields[0].size)
                    HGOTO_ERROR(
                        H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                        "compound datatype's 'imaginary' field is not at offset 'sizeof(real_field)'");
                if (fields[1].size != complex_type->shared->parent->shared->size)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                "compound datatype's 'imaginary' field is not the same size as the complex "
                                "number datatype's base datatype");

                /* Match up to 10 characters (including the NUL terminator) from the
                 * field name to a substring of "imaginary".
                 */
                name_len = strlen(fields[1].name);
                if (strncmp(fields[1].name, "imaginary", (name_len < 10) ? name_len : 10))
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                "compound datatype's 'imaginary' field name ('%s') didn't match an expected "
                                "name for conversion",
                                fields[1].name);

                if (H5T_cmp(fields[1].type, complex_type->shared->parent, false))
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                "compound datatype's 'imaginary' field is not the same datatype as the "
                                "complex number datatype's base datatype");
            }

            cdata->need_bkg = H5T_BKG_NO;

            break;
        }

        case H5T_CONV_FREE:
            break;

        case H5T_CONV_CONV:
            /* no-op as the types should be equivalent */
            break;

        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    }

done:
    if (compound_copy && H5T_close(compound_copy) < 0)
        HDONE_ERROR(H5E_DATATYPE, H5E_CANTCLOSEOBJ, FAIL, "can't close datatype");

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_complex_compat() */

#ifdef H5_HAVE_COMPLEX_NUMBERS
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex_schar
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to `signed char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(FLOAT_COMPLEX, SCHAR, H5_float_complex, signed char, SCHAR_MIN, SCHAR_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex_uchar
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to `unsigned char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(FLOAT_COMPLEX, UCHAR, H5_float_complex, unsigned char, 0, UCHAR_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex_short
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to `short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(FLOAT_COMPLEX, SHORT, H5_float_complex, short, SHRT_MIN, SHRT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex_ushort
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to `unsigned short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(FLOAT_COMPLEX, USHORT, H5_float_complex, unsigned short, 0, USHRT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex_int
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to `int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(FLOAT_COMPLEX, INT, H5_float_complex, int, INT_MIN, INT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex_uint
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to `unsigned int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(FLOAT_COMPLEX, UINT, H5_float_complex, unsigned int, 0, UINT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex_long
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to `long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(FLOAT_COMPLEX, LONG, H5_float_complex, long, LONG_MIN, LONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex_ulong
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to `unsigned long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(FLOAT_COMPLEX, ULONG, H5_float_complex, unsigned long, 0, ULONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex_llong
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to `long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(FLOAT_COMPLEX, LLONG, H5_float_complex, long long, LLONG_MIN, LLONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex_ullong
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to
 *              `unsigned long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(FLOAT_COMPLEX, ULLONG, H5_float_complex, unsigned long long, 0, ULLONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

#ifdef H5_HAVE__FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex__Float16
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to `_Float16'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    /* Suppress warning about non-standard floating-point literal suffix (F16) */
    H5_WARN_NONSTD_SUFFIX_OFF
    H5T_CONV_Zf(FLOAT_COMPLEX, FLOAT16, H5_float_complex, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_WARN_NONSTD_SUFFIX_ON
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex_float
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to `float'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_zf(FLOAT_COMPLEX, FLOAT, H5_float_complex, float, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex_double
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to `double'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_zF(FLOAT_COMPLEX, DOUBLE, H5_float_complex, double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex_ldouble
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to `long double'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                           const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                           size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_zF(FLOAT_COMPLEX, LDOUBLE, H5_float_complex, long double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex_dcomplex
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to
 *              `double _Complex' / `_Dcomplex'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex_dcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_zZ(FLOAT_COMPLEX, DOUBLE_COMPLEX, H5_float_complex, H5_double_complex, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_fcomplex_lcomplex
 *
 * Purpose:     Converts `float _Complex' / `_Fcomplex' to
 *              `long double _Complex' / `_Lcomplex'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_fcomplex_lcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_zZ(FLOAT_COMPLEX, LDOUBLE_COMPLEX, H5_float_complex, H5_ldouble_complex, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex_schar
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to `signed char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(DOUBLE_COMPLEX, SCHAR, H5_double_complex, signed char, SCHAR_MIN, SCHAR_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex_uchar
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to `unsigned char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(DOUBLE_COMPLEX, UCHAR, H5_double_complex, unsigned char, 0, UCHAR_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex_short
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to `short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(DOUBLE_COMPLEX, SHORT, H5_double_complex, short, SHRT_MIN, SHRT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex_ushort
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to
 *              `unsigned short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(DOUBLE_COMPLEX, USHORT, H5_double_complex, unsigned short, 0, USHRT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex_int
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to `int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(DOUBLE_COMPLEX, INT, H5_double_complex, int, INT_MIN, INT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex_uint
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to `unsigned int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(DOUBLE_COMPLEX, UINT, H5_double_complex, unsigned int, 0, UINT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex_long
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to `long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(DOUBLE_COMPLEX, LONG, H5_double_complex, long, LONG_MIN, LONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex_ulong
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to `unsigned long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(DOUBLE_COMPLEX, ULONG, H5_double_complex, unsigned long, 0, ULONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex_llong
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to `long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(DOUBLE_COMPLEX, LLONG, H5_double_complex, long long, LLONG_MIN, LLONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex_ullong
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to
 *              `unsigned long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(DOUBLE_COMPLEX, ULLONG, H5_double_complex, unsigned long long, 0, ULLONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

#ifdef H5_HAVE__FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex__Float16
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to `_Float16'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    /* Suppress warning about non-standard floating-point literal suffix */
    H5_WARN_NONSTD_SUFFIX_OFF
    H5T_CONV_Zf(DOUBLE_COMPLEX, FLOAT16, H5_double_complex, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_WARN_NONSTD_SUFFIX_ON
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex_float
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to `float'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Zf(DOUBLE_COMPLEX, FLOAT, H5_double_complex, float, -FLT_MAX, FLT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex_double
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to `double'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_zf(DOUBLE_COMPLEX, DOUBLE, H5_double_complex, double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex_ldouble
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to `long double'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                           const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                           size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_zF(DOUBLE_COMPLEX, LDOUBLE, H5_double_complex, long double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex_fcomplex
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to
 *              `float _Complex' / `_Fcomplex'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex_fcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Zz(DOUBLE_COMPLEX, FLOAT_COMPLEX, H5_double_complex, H5_float_complex, -FLT_MAX, FLT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_dcomplex_lcomplex
 *
 * Purpose:     Converts `double _Complex' / `_Dcomplex' to
 *              `long double _Complex' / `_Lcomplex'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_dcomplex_lcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_zZ(DOUBLE_COMPLEX, LDOUBLE_COMPLEX, H5_double_complex, H5_ldouble_complex, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex_schar
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to
 *              `signed char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_lcomplex_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(LDOUBLE_COMPLEX, SCHAR, H5_ldouble_complex, signed char, SCHAR_MIN, SCHAR_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex_uchar
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to
 *              `unsigned char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_lcomplex_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(LDOUBLE_COMPLEX, UCHAR, H5_ldouble_complex, unsigned char, 0, UCHAR_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex_short
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to `short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_lcomplex_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(LDOUBLE_COMPLEX, SHORT, H5_ldouble_complex, short, SHRT_MIN, SHRT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex_ushort
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to
 *              `unsigned short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_lcomplex_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(LDOUBLE_COMPLEX, USHORT, H5_ldouble_complex, unsigned short, 0, USHRT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex_int
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to `int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_lcomplex_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(LDOUBLE_COMPLEX, INT, H5_ldouble_complex, int, INT_MIN, INT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex_uint
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to
 *              `unsigned int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_lcomplex_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(LDOUBLE_COMPLEX, UINT, H5_ldouble_complex, unsigned int, 0, UINT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex_long
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to `long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_lcomplex_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(LDOUBLE_COMPLEX, LONG, H5_ldouble_complex, long, LONG_MIN, LONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex_ulong
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to
 *              `unsigned long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_lcomplex_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(LDOUBLE_COMPLEX, ULONG, H5_ldouble_complex, unsigned long, 0, ULONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex_llong
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to
 *              `long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5T_CONV_INTERNAL_LDOUBLE_LLONG
herr_t
H5T__conv_lcomplex_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(LDOUBLE_COMPLEX, LLONG, H5_ldouble_complex, long long, LLONG_MIN, LLONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}
#endif /* H5T_CONV_INTERNAL_LDOUBLE_LLONG */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex_ullong
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to
 *              `unsigned long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5T_CONV_INTERNAL_LDOUBLE_ULLONG
herr_t
H5T__conv_lcomplex_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Zx(LDOUBLE_COMPLEX, ULLONG, H5_ldouble_complex, unsigned long long, 0, ULLONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}
#endif /* H5T_CONV_INTERNAL_LDOUBLE_ULLONG */

#ifdef H5_HAVE__FLOAT16
#ifdef H5T_CONV_INTERNAL_LDOUBLE_FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex__Float16
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to `_Float16'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_lcomplex__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    /* Suppress warning about non-standard floating-point literal suffix */
    H5_WARN_NONSTD_SUFFIX_OFF
    H5T_CONV_Zf(LDOUBLE_COMPLEX, FLOAT16, H5_ldouble_complex, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_WARN_NONSTD_SUFFIX_ON
}
#endif
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex_float
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to `float'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_lcomplex_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Zf(LDOUBLE_COMPLEX, FLOAT, H5_ldouble_complex, float, -FLT_MAX, FLT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex_double
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to `double'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_lcomplex_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Zf(LDOUBLE_COMPLEX, DOUBLE, H5_ldouble_complex, double, -DBL_MAX, DBL_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex_ldouble
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to
 *              `long double'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_lcomplex_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                           const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                           size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_zf(LDOUBLE_COMPLEX, LDOUBLE, H5_ldouble_complex, long double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex_fcomplex
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to
 *              `float _Complex' / `_Fcomplex'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_lcomplex_fcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Zz(LDOUBLE_COMPLEX, FLOAT_COMPLEX, H5_ldouble_complex, H5_float_complex, -FLT_MAX, FLT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_lcomplex_dcomplex
 *
 * Purpose:     Converts `long double _Complex' / `_Lcomplex' to
 *              `double _Complex' / `_Dcomplex'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_lcomplex_dcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Zz(LDOUBLE_COMPLEX, DOUBLE_COMPLEX, H5_ldouble_complex, H5_double_complex, -DBL_MAX, DBL_MAX);
}
#endif /* H5_HAVE_COMPLEX_NUMBERS */
