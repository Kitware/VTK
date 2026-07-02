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
 * Purpose: Datatype conversion functions for floating-point datatypes
 */

/****************/
/* Module Setup */
/****************/
#include "H5Tmodule.h" /* This source code file is part of the H5T module */

/***********/
/* Headers */
/***********/
#include "H5private.h" /* Generic Functions                    */
#include "H5Tconv.h"   /* Datatype conversions                 */
#include "H5Tconv_macros.h"
#include "H5Tconv_complex.h"
#include "H5Tconv_integer.h"
#include "H5Tconv_float.h"

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_f_f
 *
 * Purpose:     Convert one floating point type to another. This is a catch
 *              all for floating point conversions and is probably not
 *              particularly fast!
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_f_f(const H5T_t *src_p, const H5T_t *dst_p, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
              size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
              void H5_ATTR_UNUSED *bkg)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT: {
            H5T_atomic_t src_atomic; /* source datatype atomic info      */
            H5T_atomic_t dst_atomic; /* destination datatype atomic info */

            if (NULL == src_p || NULL == dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            src_atomic = src_p->shared->u.atomic;
            dst_atomic = dst_p->shared->u.atomic;
            if (H5T_ORDER_LE != src_atomic.order && H5T_ORDER_BE != src_atomic.order &&
                H5T_ORDER_VAX != src_atomic.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unsupported byte order");
            if (H5T_ORDER_LE != dst_atomic.order && H5T_ORDER_BE != dst_atomic.order &&
                H5T_ORDER_VAX != dst_atomic.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unsupported byte order");
            if (dst_p->shared->size > TEMP_FLOAT_CONV_BUFFER_SIZE)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "destination size is too large");
            if (8 * sizeof(int64_t) - 1 < src_atomic.u.f.esize ||
                8 * sizeof(int64_t) - 1 < dst_atomic.u.f.esize)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "exponent field is too large");
            cdata->need_bkg = H5T_BKG_NO;

            break;
        }

        case H5T_CONV_FREE:
            break;

        case H5T_CONV_CONV:
            if (NULL == src_p || NULL == dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (NULL == conv_ctx)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid datatype conversion context pointer");
            if (H5T__conv_f_f_loop(src_p, dst_p, conv_ctx, nelmts, buf_stride, buf) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "unable to convert data values");
            break;

        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_f_f() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_f_f_loop
 *
 * Purpose:     Implements the body of the conversion loop when converting
 *              floating-point values to another floating-point type
 *              (including complex number types). Encapsulates common
 *              code that is shared between the H5T__conv_f_f conversion
 *              function and other functions where the logic is nearly
 *              identical, such as H5T__conv_f_complex and
 *              H5T__conv_complex_f.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_f_f_loop(const H5T_t *src_p, const H5T_t *dst_p, const H5T_conv_ctx_t *conv_ctx, size_t nelmts,
                   size_t buf_stride, void *buf)
{
    H5T_atomic_t src_atomic;                        /* source datatype atomic info      */
    H5T_atomic_t dst_atomic;                        /* destination datatype atomic info */
    hssize_t     expo_max;                          /* maximum possible dst exponent    */
    ssize_t      src_delta, dst_delta;              /* source & destination stride      */
    uint8_t     *s, *sp, *d, *dp;                   /* source and dest traversal ptrs   */
    uint8_t     *src_rev = NULL;                    /* order-reversed source buffer     */
    uint8_t      dbuf[TEMP_FLOAT_CONV_BUFFER_SIZE]; /* temp destination buffer          */
    size_t       olap;                              /* num overlapping elements         */
    int          direction;                         /* forward or backward traversal    */
    herr_t       ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(src_p);
    assert(src_p->shared->type == H5T_FLOAT || src_p->shared->type == H5T_COMPLEX);
    assert(dst_p);
    assert(dst_p->shared->type == H5T_FLOAT || dst_p->shared->type == H5T_COMPLEX);
    assert(conv_ctx);
    assert(buf);

    if (src_p->shared->type == H5T_COMPLEX)
        src_atomic = src_p->shared->parent->shared->u.atomic;
    else
        src_atomic = src_p->shared->u.atomic;
    if (dst_p->shared->type == H5T_COMPLEX)
        dst_atomic = dst_p->shared->parent->shared->u.atomic;
    else
        dst_atomic = dst_p->shared->u.atomic;

    expo_max = ((hssize_t)1 << dst_atomic.u.f.esize) - 1;

#ifndef NDEBUG
    /* Are we converting between a floating-point type and a complex number
     * type consisting of the same floating-point type? This function is
     * only intended for converting between different floating-point types
     * and will produce incorrect results otherwise.
     */
    if ((src_p->shared->type == H5T_COMPLEX && dst_p->shared->type == H5T_FLOAT) ||
        (src_p->shared->type == H5T_FLOAT && dst_p->shared->type == H5T_COMPLEX)) {
        const H5T_t *src_base = (src_p->shared->type == H5T_FLOAT) ? src_p : src_p->shared->parent;
        const H5T_t *dst_base = (dst_p->shared->type == H5T_FLOAT) ? dst_p : dst_p->shared->parent;
        assert(0 != (H5T_cmp(src_base, dst_base, false)));
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
        H5T_conv_float_specval_t specval_type; /* floating-point value type (regular, +/-Inf, +/-0, NaN) */
        H5T_conv_ret_t except_ret = H5T_CONV_UNHANDLED; /* return of conversion exception callback function */
        ssize_t        bitno      = 0;                  /* bit number                                       */
        int64_t        expo;                            /* exponent                                         */
        size_t         implied;                         /* destination implied bits                         */
        size_t         mpos;                            /* offset to useful mant in src                     */
        size_t         msize = 0;                       /* useful size of mantissa in src                   */
        size_t         mrsh;                            /* amount to right shift mantissa                   */
        bool           reverse      = true;             /* if reversed the order of destination             */
        bool           denormalized = false;            /* is either source or destination denormalized?    */
        bool           carry        = false;            /* carry after rounding mantissa                    */

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
            /* Temporary solution to handle VAX special values.
             * Note that even though we don't support VAX anymore, we
             * still need to handle legacy VAX files so this code must
             * remain in place.
             */
        }

        /*
         * Get the exponent as an unsigned quantity from the section of
         * the source bit field where it's located. Don't worry about
         * the exponent bias yet.
         */
        expo = (int64_t)H5T__bit_get_d(s, src_atomic.u.f.epos, src_atomic.u.f.esize);

        if (expo == 0)
            denormalized = true;

        /*
         * Set markers for the source mantissa, excluding the leading `1'
         * (might be implied).
         */
        implied = 1;
        mpos    = src_atomic.u.f.mpos;
        mrsh    = 0;
        if (0 == expo || H5T_NORM_NONE == src_atomic.u.f.norm) {
            if ((bitno = H5T__bit_find(s, src_atomic.u.f.mpos, src_atomic.u.f.msize, H5T_BIT_MSB, true)) >
                0) {
                msize = (size_t)bitno;
            }
            else if (0 == bitno) {
                msize = 1;
                H5T__bit_set(s, src_atomic.u.f.mpos, (size_t)1, false);
            }
        }
        else if (H5T_NORM_IMPLIED == src_atomic.u.f.norm) {
            msize = src_atomic.u.f.msize;
        }
        else {
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "normalization method not implemented yet");
        }

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
            assert(bitno >= 0);
            expo -= (int64_t)((src_atomic.u.f.ebias - 1) + (src_atomic.u.f.msize - (size_t)bitno));
        }
        else if (H5T_NORM_IMPLIED == src_atomic.u.f.norm) {
            expo -= (int64_t)src_atomic.u.f.ebias;
        }
        else {
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "normalization method not implemented yet");
        }

        /*
         * If the destination is not normalized then right shift the
         * mantissa by one.
         */
        if (H5T_NORM_NONE == dst_atomic.u.f.norm)
            mrsh++;

        /*
         * Calculate the destination exponent by adding the destination
         * bias and clipping by the minimum and maximum possible
         * destination exponent values.
         */
        expo += (int64_t)dst_atomic.u.f.ebias;

        if (expo < -(hssize_t)(dst_atomic.u.f.msize)) {
            /* The exponent is way too small.  Result is zero. */
            expo = 0;
            H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
            msize = 0;
        }
        else if (expo <= 0) {
            /*
             * The exponent is too small to fit in the exponent field,
             * but by shifting the mantissa to the right we can
             * accommodate that value.  The mantissa of course is no
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
                            H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                            conv_ctx->u.conv.dst_type_id, src_rev, d, conv_ctx->u.conv.cb_struct.user_data);
                    }
                H5_AFTER_USER_CB(FAIL)
            }

            if (except_ret == H5T_CONV_UNHANDLED) {
                expo = expo_max;
                H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
                msize = 0;
            }
            else if (except_ret == H5T_CONV_HANDLED) {
                reverse = false;
                goto next;
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
        if (msize > 0 && mrsh <= dst_atomic.u.f.msize && mrsh + msize > dst_atomic.u.f.msize) {
            bitno = (ssize_t)(mrsh + msize - dst_atomic.u.f.msize);
            assert(bitno >= 0 && (size_t)bitno <= msize);
            /* If the 1st bit being cut off is set and source isn't denormalized. */
            if (H5T__bit_get_d(s, (mpos + (size_t)bitno) - 1, (size_t)1) && !denormalized) {
                /* Don't do rounding if exponent is 111...110 and mantissa is 111...11.
                 * To do rounding and increment exponent in this case will create an infinity value. */
                if ((H5T__bit_find(s, mpos + (size_t)bitno, msize - (size_t)bitno, H5T_BIT_LSB, false) >= 0 ||
                     expo < expo_max - 1)) {
                    carry = H5T__bit_inc(s, mpos + (size_t)bitno - 1, 1 + msize - (size_t)bitno);
                    if (carry)
                        implied = 2;
                }
            }
            else if (H5T__bit_get_d(s, (mpos + (size_t)bitno) - 1, (size_t)1) && denormalized)
                /* For either source or destination, denormalized value doesn't increment carry. */
                H5T__bit_inc(s, mpos + (size_t)bitno - 1, 1 + msize - (size_t)bitno);
        }
        else
            carry = false;

        /*
         * Write the mantissa to the destination
         */
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
                H5T__bit_set_d(d, dst_atomic.u.f.mpos + dst_atomic.u.f.msize - mrsh, (size_t)2,
                               (hsize_t)implied);
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
                                conv_ctx->u.conv.dst_type_id, src_rev, d,
                                conv_ctx->u.conv.cb_struct.user_data);
                        }
                    H5_AFTER_USER_CB(FAIL)
                }

                if (except_ret == H5T_CONV_UNHANDLED) {
                    expo = expo_max;
                    H5T__bit_set(d, dst_atomic.u.f.mpos, dst_atomic.u.f.msize, false);
                }
                else if (except_ret == H5T_CONV_HANDLED) {
                    reverse = false;
                    goto next;
                }
                else if (except_ret == H5T_CONV_ABORT)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");
            }
        }

        carry = false;

        H5_CHECK_OVERFLOW(expo, hssize_t, hsize_t);
        H5T__bit_set_d(d, dst_atomic.u.f.epos, dst_atomic.u.f.esize, (hsize_t)expo);

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
} /* end H5T__conv_f_f_loop() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_find_special
 *
 * Purpose:     Helper function to inspect the bits of a floating-point
 *              value during data conversions and determine if that value
 *              is a special value (+/-Inf, +/-0, NaN).
 *
 *              If `sign_out` is non-NULL, it is set to the value of the
 *              sign bit of the floating-point value.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
H5T_conv_float_specval_t
H5T__conv_float_find_special(const uint8_t *src_buf, const H5T_atomic_t *src_atomic, uint64_t *sign_out)
{
    uint64_t                 sign; /* sign bit value */
    H5T_conv_float_specval_t ret_value = H5T_CONV_FLOAT_SPECVAL_REGULAR;

    FUNC_ENTER_PACKAGE_NOERR

    assert(src_buf);
    assert(src_atomic);

    /* Find the sign bit value of the source. */
    sign = H5T__bit_get_d(src_buf, src_atomic->u.f.sign, (size_t)1);

    /* Is the mantissa all 0 bits? */
    if (H5T__bit_find(src_buf, src_atomic->u.f.mpos, src_atomic->u.f.msize, H5T_BIT_LSB, true) < 0) {
        /* Is the exponent all 0 bits? */
        if (H5T__bit_find(src_buf, src_atomic->u.f.epos, src_atomic->u.f.esize, H5T_BIT_LSB, true) < 0)
            /* +0 or -0 */
            ret_value = sign ? H5T_CONV_FLOAT_SPECVAL_NEGZERO : H5T_CONV_FLOAT_SPECVAL_POSZERO;
        /* Is the exponent all 1 bits? */
        else if (H5T__bit_find(src_buf, src_atomic->u.f.epos, src_atomic->u.f.esize, H5T_BIT_LSB, false) < 0)
            /* +Inf or -Inf */
            ret_value = sign ? H5T_CONV_FLOAT_SPECVAL_NEGINF : H5T_CONV_FLOAT_SPECVAL_POSINF;
    }
    else {
        bool exp_all_ones =
            (H5T__bit_find(src_buf, src_atomic->u.f.epos, src_atomic->u.f.esize, H5T_BIT_LSB, false) < 0);

        /* For a source value with no implied mantissa bit, if the exponent bits
         * are all 1s and only the 1st bit of the mantissa is set to 1, the value
         * is infinity. The Intel-Linux "long double" is this case.
         */
        if (H5T_NORM_NONE == src_atomic->u.f.norm && exp_all_ones &&
            H5T__bit_find(src_buf, src_atomic->u.f.mpos, src_atomic->u.f.msize - 1, H5T_BIT_LSB, true) < 0)
            ret_value = sign ? H5T_CONV_FLOAT_SPECVAL_NEGINF : H5T_CONV_FLOAT_SPECVAL_POSINF;
        else if (exp_all_ones)
            ret_value = H5T_CONV_FLOAT_SPECVAL_NAN;
    }

    if (sign_out)
        *sign_out = sign;

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5T__conv_float_find_special() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_f_i
 *
 * Purpose:     Convert one floating-point type to an integer. This is
 *              the catch-all function for float-integer conversions and
 *              is probably not particularly fast.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_f_i(const H5T_t *src_p, const H5T_t *dst_p, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
              size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
              void H5_ATTR_UNUSED *bkg)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT: {
            H5T_atomic_t src_atomic; /* source datatype atomic info      */
            H5T_atomic_t dst_atomic; /* destination datatype atomic info */

            if (NULL == src_p || NULL == dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            src_atomic = src_p->shared->u.atomic;
            dst_atomic = dst_p->shared->u.atomic;
            if (H5T_ORDER_LE != src_atomic.order && H5T_ORDER_BE != src_atomic.order &&
                H5T_ORDER_VAX != src_atomic.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unsupported byte order");
            if (H5T_ORDER_LE != dst_atomic.order && H5T_ORDER_BE != dst_atomic.order &&
                H5T_ORDER_VAX != dst_atomic.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unsupported byte order");
            if (dst_p->shared->size > TEMP_INT_CONV_BUFFER_SIZE)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "destination size is too large");
            if (8 * sizeof(hssize_t) - 1 < src_atomic.u.f.esize)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "exponent field is too large");
            cdata->need_bkg = H5T_BKG_NO;

            break;
        }

        case H5T_CONV_FREE:
            break;

        case H5T_CONV_CONV:
            if (NULL == src_p || NULL == dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (NULL == conv_ctx)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid datatype conversion context pointer");

            if (H5T__conv_f_i_loop(src_p, dst_p, conv_ctx, nelmts, buf_stride, buf) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "unable to convert data values");
            break;

        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_f_i() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_f_i_loop
 *
 * Purpose:     Implements the body of the conversion loop when converting
 *              floating-point values (including complex number values) to
 *              integer values. Encapsulates common code that is shared
 *              between the H5T__conv_f_i conversion function and other
 *              functions where the logic is nearly identical, such as
 *              H5T__conv_complex_i.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_f_i_loop(const H5T_t *src_p, const H5T_t *dst_p, const H5T_conv_ctx_t *conv_ctx, size_t nelmts,
                   size_t buf_stride, void *buf)
{
    H5T_atomic_t src_atomic;                      /* source datatype atomic info      */
    H5T_atomic_t dst_atomic;                      /* destination datatype atomic info */
    ssize_t      src_delta, dst_delta;            /* source & destination stride      */
    uint8_t     *s, *sp, *d, *dp;                 /* source and dest traversal ptrs   */
    uint8_t     *int_buf = NULL;                  /* buffer for temporary value       */
    uint8_t     *src_rev = NULL;                  /* order-reversed source buffer     */
    uint8_t      dbuf[TEMP_INT_CONV_BUFFER_SIZE]; /* temp destination buffer          */
    size_t       int_buf_size;                    /* buffer size for temporary value  */
    size_t       src_base_size;                   /* size of source base datatype     */
    size_t       olap;                            /* num overlapping elements         */
    int          direction;                       /* forward or backward traversal    */
    herr_t       ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(src_p);
    assert(src_p->shared->type == H5T_FLOAT || src_p->shared->type == H5T_COMPLEX);
    assert(dst_p);
    assert(dst_p->shared->type == H5T_INTEGER);
    assert(conv_ctx);
    assert(buf);

    if (src_p->shared->type == H5T_COMPLEX)
        src_atomic = src_p->shared->parent->shared->u.atomic;
    else
        src_atomic = src_p->shared->u.atomic;
    dst_atomic = dst_p->shared->u.atomic;

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

    /* Allocate enough space for the buffer holding temporary converted value */
    src_base_size =
        (H5T_FLOAT == src_p->shared->type) ? src_p->shared->size : src_p->shared->parent->shared->size;
    if (dst_atomic.prec / 8 > src_base_size)
        int_buf_size = (dst_atomic.prec + 7) / 8;
    else
        int_buf_size = src_base_size;
    if (NULL == (int_buf = H5MM_calloc(int_buf_size)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTALLOC, FAIL, "couldn't allocate temporary buffer");

    /* Allocate space for order-reversed source buffer */
    if (conv_ctx->u.conv.cb_struct.func)
        if (NULL == (src_rev = H5MM_calloc(src_p->shared->size)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTALLOC, FAIL, "couldn't allocate temporary buffer");

    /* The conversion loop */
    for (size_t elmtno = 0; elmtno < nelmts; elmtno++) {
        H5T_conv_float_specval_t specval_type; /* floating-point value type (regular, +/-Inf, +/-0, NaN) */
        H5T_conv_ret_t except_ret = H5T_CONV_UNHANDLED; /* return of conversion exception callback function */
        uint64_t       sign;              /* source sign bit value                                  */
        hssize_t       expo;              /* source exponent                                        */
        hssize_t       shift_val;         /* shift value when shifting mantissa by exponent         */
        ssize_t        msb_pos_s;         /* first bit(MSB) in an integer                           */
        ssize_t        new_msb_pos;       /* MSB position after shifting mantissa by exponent       */
        bool           truncated = false; /* if fraction value is dropped                           */
        bool           reverse   = true;  /* if reversed the order of destination                   */

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
         * complicated.  We'll do all the conversion stuff assuming
         * little endian and then we'll fix the order at the end.
         */
        if (H5T_ORDER_BE == src_atomic.order) {
            size_t half_size = src_p->shared->size / 2;

            if (H5T_FLOAT == src_p->shared->type) {
                for (size_t i = 0; i < half_size; i++)
                    H5_SWAP_BYTES(s, i, src_p->shared->size - (i + 1));
            }
            else {
                uint8_t *cur_part = s;
                /* Swap real part of complex number element */
                for (size_t i = 0; i < half_size / 2; i++)
                    H5_SWAP_BYTES(cur_part, i, half_size - (i + 1));
                /* Swap imaginary part of complex number element */
                cur_part += half_size;
                for (size_t i = 0; i < half_size / 2; i++)
                    H5_SWAP_BYTES(cur_part, i, half_size - (i + 1));
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

        /* zero-set all destination bits */
        H5T__bit_set(d, dst_atomic.offset, dst_atomic.prec, false);

        /* Check for special cases: +0, -0, +Inf, -Inf, NaN */
        specval_type = H5T__conv_float_find_special(s, &src_atomic, &sign);
        if (specval_type == H5T_CONV_FLOAT_SPECVAL_POSZERO ||
            specval_type == H5T_CONV_FLOAT_SPECVAL_NEGZERO) {
            /* +0 or -0; Set all bits to zero */
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
                if (specval_type == H5T_CONV_FLOAT_SPECVAL_NAN)
                    goto padding; /* Just set all bits to zero. */
                else if (specval_type == H5T_CONV_FLOAT_SPECVAL_POSINF) {
                    if (H5T_SGN_NONE == dst_atomic.u.i.sign)
                        H5T__bit_set(d, dst_atomic.offset, dst_atomic.prec, true);
                    else if (H5T_SGN_2 == dst_atomic.u.i.sign)
                        H5T__bit_set(d, dst_atomic.offset, dst_atomic.prec - 1, true);
                }
                else if (specval_type == H5T_CONV_FLOAT_SPECVAL_NEGINF) {
                    if (H5T_SGN_2 == dst_atomic.u.i.sign)
                        H5T__bit_set(d, dst_atomic.prec - 1, (size_t)1, true);
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

        /*
         * Get the exponent as an unsigned quantity from the section of
         * the source bit field where it's located. Not expecting
         * exponent to be greater than the maximal value of hssize_t.
         */
        expo = (hssize_t)H5T__bit_get_d(s, src_atomic.u.f.epos, src_atomic.u.f.esize);

        /*
         * Calculate the true source exponent by adjusting according to
         * the source exponent bias.
         */
        if (0 == expo || H5T_NORM_NONE == src_atomic.u.f.norm)
            expo -= (hssize_t)(src_atomic.u.f.ebias - 1);
        else if (H5T_NORM_IMPLIED == src_atomic.u.f.norm)
            expo -= (hssize_t)src_atomic.u.f.ebias;
        else
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "normalization method not implemented yet");

        /*
         * Get the mantissa as bit vector from the section of
         * the source bit field where it's located.
         * Keep the little-endian order in the buffer.
         * A sequence 0x01020304 will be like in the buffer,
         *      04      03      02      01
         *      |       |       |       |
         *      V       V       V       V
         *    buf[0]  buf[1]  buf[2]  buf[3]
         */
        H5T__bit_copy(int_buf, (size_t)0, s, src_atomic.u.f.mpos, src_atomic.u.f.msize);

        /*
         * Restore the implicit bit for mantissa if it's implied.
         * Equivalent to mantissa |= (hsize_t)1 << src_atomic.u.f.msize.
         */
        if (H5T_NORM_IMPLIED == src_atomic.u.f.norm)
            H5T__bit_inc(int_buf, src_atomic.u.f.msize, 8 * int_buf_size - src_atomic.u.f.msize);

        /*
         * What is the bit position for the most significant bit(MSB) of S
         * which is set? This is checked before shifting and before possibly
         * converting to a negative integer. Note that later use of this value
         * assumes that H5T__bit_shift will always shift in 0 during a right
         * shift.
         */
        msb_pos_s = H5T__bit_find(int_buf, (size_t)0, src_atomic.prec, H5T_BIT_MSB, true);

        /* The temporary buffer has no bits set and must therefore be zero; nothing to do. */
        if (msb_pos_s < 0)
            goto padding;

        /*
         * Shift mantissa part by exponent minus mantissa size(right shift),
         * or by mantissa size minus exponent(left shift).  Example: Sequence
         * 10...010111, expo=20, expo-msize=-3.  Right-shift the sequence, we get
         * 00010...10.  The last three bits were dropped.
         */
        shift_val = expo - (ssize_t)src_atomic.u.f.msize;
        H5T__bit_shift(int_buf, shift_val, (size_t)0, int_buf_size * 8);

        /* Calculate the new position of the MSB after shifting and
         * skip to the padding section if we shifted exactly to 0
         * (MSB position is -1)
         */
        new_msb_pos = msb_pos_s + shift_val;
        if (new_msb_pos == -1)
            goto padding;

        /*
         * If expo is less than mantissa size, the fractional value is dropped off
         * during conversion. Set exception type to be "truncate"
         */
        if ((size_t)expo < src_atomic.u.f.msize && conv_ctx->u.conv.cb_struct.func)
            truncated = true;

        if (H5T_SGN_NONE == dst_atomic.u.i.sign) { /* destination is unsigned */
            /*
             * Destination is unsigned. Library's default way: If the source value
             * is greater than the maximal destination value then it overflows, the
             * destination will be set to the maximum possible value. When the
             * source is negative, underflow happens. Set the destination to be
             * zero (do nothing). If user's exception handler is set, call it and
             * let user handle it.
             */
            if (sign) { /* source is negative */
                /* If user's exception handler is present, use it */
                if (conv_ctx->u.conv.cb_struct.func) {
                    /* Reverse source buffer order first */
                    H5T__reverse_order(src_rev, s, src_p);

                    /* Prepare & restore library for user callback */
                    H5_BEFORE_USER_CB(FAIL)
                        {
                            except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                H5T_CONV_EXCEPT_RANGE_LOW, conv_ctx->u.conv.src_type_id,
                                conv_ctx->u.conv.dst_type_id, src_rev, d,
                                conv_ctx->u.conv.cb_struct.user_data);
                        }
                    H5_AFTER_USER_CB(FAIL)

                    if (except_ret == H5T_CONV_HANDLED) {
                        /* No need to reverse the order of destination because user handles it */
                        reverse = false;
                        goto next;
                    }
                    else if (except_ret == H5T_CONV_ABORT)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");
                }
            }
            else { /* source is positive */
                if (new_msb_pos >= (ssize_t)dst_atomic.prec) {
                    /* overflow - if user's exception handler is present, use it */
                    if (conv_ctx->u.conv.cb_struct.func) {
                        /* Reverse source buffer order first */
                        H5T__reverse_order(src_rev, s, src_p);

                        /* Prepare & restore library for user callback */
                        H5_BEFORE_USER_CB(FAIL)
                            {
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                            }
                        H5_AFTER_USER_CB(FAIL)
                    }

                    if (except_ret == H5T_CONV_UNHANDLED)
                        H5T__bit_set(d, dst_atomic.offset, dst_atomic.prec, true);
                    else if (except_ret == H5T_CONV_HANDLED) {
                        /* No need to reverse the order of destination because user handles it */
                        reverse = false;
                        goto next;
                    }
                    else if (except_ret == H5T_CONV_ABORT)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");
                }
                else {
                    /* If user's exception handler is present, use it */
                    if (truncated && conv_ctx->u.conv.cb_struct.func) {
                        /* Reverse source buffer order first */
                        H5T__reverse_order(src_rev, s, src_p);

                        /* Prepare & restore library for user callback */
                        H5_BEFORE_USER_CB(FAIL)
                            {
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_TRUNCATE, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                            }
                        H5_AFTER_USER_CB(FAIL)
                    }

                    if (except_ret == H5T_CONV_UNHANDLED) {
                        /* copy source value into it if case is ignored by user handler */
                        if (new_msb_pos >= 0)
                            H5T__bit_copy(d, dst_atomic.offset, int_buf, (size_t)0, (size_t)new_msb_pos + 1);
                    }
                    else if (except_ret == H5T_CONV_HANDLED) {
                        /* No need to reverse the order of destination because user handles it */
                        reverse = false;
                        goto next;
                    }
                    else if (except_ret == H5T_CONV_ABORT)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");
                }
            }
        }
        else if (H5T_SGN_2 == dst_atomic.u.i.sign) { /* Destination is signed */
            if (sign) {                              /* source is negative */
                if ((new_msb_pos >= 0) && ((size_t)new_msb_pos < dst_atomic.prec - 1)) {
                    /* If user's exception handler is present, use it */
                    if (truncated && conv_ctx->u.conv.cb_struct.func) {
                        /* Reverse source buffer order first */
                        H5T__reverse_order(src_rev, s, src_p);

                        /* Prepare & restore library for user callback */
                        H5_BEFORE_USER_CB(FAIL)
                            {
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_TRUNCATE, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                            }
                        H5_AFTER_USER_CB(FAIL)
                    }

                    if (except_ret == H5T_CONV_UNHANDLED) { /* If this case ignored by user handler */
                        /* Convert to integer representation. Equivalent to ~(value - 1). */
                        H5T__bit_dec(int_buf, (size_t)0, dst_atomic.prec);
                        H5T__bit_neg(int_buf, (size_t)0, dst_atomic.prec);

                        /* copy source value into destination */
                        H5T__bit_copy(d, dst_atomic.offset, int_buf, (size_t)0, dst_atomic.prec - 1);
                        H5T__bit_set(d, (dst_atomic.offset + dst_atomic.prec - 1), (size_t)1, true);
                    }
                    else if (except_ret == H5T_CONV_HANDLED) {
                        /* No need to reverse the order of destination because user handles it */
                        reverse = false;
                        goto next;
                    }
                    else if (except_ret == H5T_CONV_ABORT)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");
                }
                else {
                    /* if underflows and no callback, do nothing except turn on
                     * the sign bit because 0x80...00 is the biggest negative value.
                     * If user's exception handler is present, use it
                     */
                    if (conv_ctx->u.conv.cb_struct.func) {
                        /* Reverse source buffer order first */
                        H5T__reverse_order(src_rev, s, src_p);

                        /* Prepare & restore library for user callback */
                        H5_BEFORE_USER_CB(FAIL)
                            {
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_RANGE_LOW, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                            }
                        H5_AFTER_USER_CB(FAIL)
                    }

                    if (except_ret == H5T_CONV_UNHANDLED)
                        H5T__bit_set(d, (dst_atomic.offset + dst_atomic.prec - 1), (size_t)1, true);
                    else if (except_ret == H5T_CONV_HANDLED) {
                        /* No need to reverse the order of destination because user handles it */
                        reverse = false;
                        goto next;
                    }
                    else if (except_ret == H5T_CONV_ABORT)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");
                }
            }
            else { /* source is positive */
                if (new_msb_pos >= (ssize_t)dst_atomic.prec - 1) {
                    /* overflow - if user's exception handler is present, use it */
                    if (conv_ctx->u.conv.cb_struct.func) {
                        /* Reverse source buffer order first */
                        H5T__reverse_order(src_rev, s, src_p);

                        /* Prepare & restore library for user callback */
                        H5_BEFORE_USER_CB(FAIL)
                            {
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                            }
                        H5_AFTER_USER_CB(FAIL)
                    }

                    if (except_ret == H5T_CONV_UNHANDLED)
                        H5T__bit_set(d, dst_atomic.offset, dst_atomic.prec - 1, true);
                    else if (except_ret == H5T_CONV_HANDLED) {
                        /* No need to reverse the order of destination because user handles it */
                        reverse = false;
                        goto next;
                    }
                    else if (except_ret == H5T_CONV_ABORT)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");
                }
                else if (new_msb_pos < (ssize_t)dst_atomic.prec - 1) {
                    /* If user's exception handler is present, use it */
                    if (truncated && conv_ctx->u.conv.cb_struct.func) {
                        /* Reverse source buffer order first */
                        H5T__reverse_order(src_rev, s, src_p);

                        /* Prepare & restore library for user callback */
                        H5_BEFORE_USER_CB(FAIL)
                            {
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_TRUNCATE, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                            }
                        H5_AFTER_USER_CB(FAIL)
                    }

                    if (except_ret == H5T_CONV_UNHANDLED) {
                        /* copy source value into it if case is ignored by user handler */
                        if (new_msb_pos >= 0)
                            H5T__bit_copy(d, dst_atomic.offset, int_buf, (size_t)0, (size_t)new_msb_pos + 1);
                    }
                    else if (except_ret == H5T_CONV_HANDLED) {
                        /* No need to reverse the order of destination because user handles it */
                        reverse = false;
                        goto next;
                    }
                    else if (except_ret == H5T_CONV_ABORT)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");
                }
            }
        }

padding:
        /* Set padding areas in destination. */
        if (dst_atomic.offset > 0) {
            assert(H5T_PAD_ZERO == dst_atomic.lsb_pad || H5T_PAD_ONE == dst_atomic.lsb_pad);
            H5T__bit_set(d, (size_t)0, dst_atomic.offset, (bool)(H5T_PAD_ONE == dst_atomic.lsb_pad));
        }
        if (dst_atomic.offset + dst_atomic.prec != 8 * dst_p->shared->size) {
            assert(H5T_PAD_ZERO == dst_atomic.msb_pad || H5T_PAD_ONE == dst_atomic.msb_pad);
            H5T__bit_set(d, dst_atomic.offset + dst_atomic.prec,
                         8 * dst_p->shared->size - (dst_atomic.offset + dst_atomic.prec),
                         (bool)(H5T_PAD_ONE == dst_atomic.msb_pad));
        }

        /*
         * Put the destination in the correct byte order. See note at
         * beginning of loop.
         */
        if (H5T_ORDER_BE == dst_atomic.order && reverse)
            for (size_t i = 0; i < dst_p->shared->size / 2; i++)
                H5_SWAP_BYTES(d, i, dst_p->shared->size - (i + 1));

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

        memset(int_buf, 0, int_buf_size);
    } /* end conversion loop */

done:
    H5MM_free(src_rev);
    H5MM_free(int_buf);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5T__conv_f_i_loop() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_f_complex
 *
 * Purpose:     Convert floating-point values to complex number values.
 *              This is the catch-all function for float-complex number
 *              conversions and is probably not particularly fast.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_f_complex(const H5T_t *src_p, const H5T_t *dst_p, H5T_cdata_t *cdata,
                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                    size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    bool   equal_cplx_conv = false; /* if converting between complex and matching float */
    herr_t ret_value       = SUCCEED;

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT: {
            H5T_atomic_t src_atomic; /* source datatype atomic info      */
            H5T_atomic_t dst_atomic; /* destination datatype atomic info */

            if (!src_p || !dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (!H5T_IS_ATOMIC(dst_p->shared->parent->shared))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid complex number datatype");
            src_atomic = src_p->shared->u.atomic;
            dst_atomic = dst_p->shared->parent->shared->u.atomic;
            if (H5T_ORDER_LE != src_atomic.order && H5T_ORDER_BE != src_atomic.order &&
                H5T_ORDER_VAX != src_atomic.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                            "unsupported byte order for source datatype");
            if (H5T_ORDER_LE != dst_atomic.order && H5T_ORDER_BE != dst_atomic.order)
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
            equal_cplx_conv = (0 == H5T_cmp(src_p, dst_p->shared->parent, false));
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
} /* end H5T__conv_f_complex() */

#ifdef H5_HAVE__FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_schar
 *
 * Purpose:     Converts `_Float16' to `signed char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(FLOAT16, SCHAR, H5__Float16, signed char, SCHAR_MIN, SCHAR_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_uchar
 *
 * Purpose:     Converts `_Float16' to `unsigned char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(FLOAT16, UCHAR, H5__Float16, unsigned char, 0, UCHAR_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_short
 *
 * Purpose:     Converts `_Float16' to `signed short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(FLOAT16, SHORT, H5__Float16, short, SHRT_MIN, SHRT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_ushort
 *
 * Purpose:     Converts `_Float16' to `unsigned short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fX(FLOAT16, USHORT, H5__Float16, unsigned short, 0, USHRT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_int
 *
 * Purpose:     Converts `_Float16' to `signed int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fX(FLOAT16, INT, H5__Float16, int, INT_MIN, INT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_uint
 *
 * Purpose:     Converts `_Float16' to `unsigned int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fX(FLOAT16, UINT, H5__Float16, unsigned int, 0, UINT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_long
 *
 * Purpose:     Converts `_Float16' to `signed long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fX(FLOAT16, LONG, H5__Float16, long, LONG_MIN, LONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_ulong
 *
 * Purpose:     Converts `_Float16' to `unsigned long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fX(FLOAT16, ULONG, H5__Float16, unsigned long, 0, ULONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_llong
 *
 * Purpose:     Converts `_Float16' to `signed long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fX(FLOAT16, LLONG, H5__Float16, long long, LLONG_MIN, LLONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_ullong
 *
 * Purpose:     Converts `_Float16' to `unsigned long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fX(FLOAT16, ULLONG, H5__Float16, unsigned long long, 0, ULLONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_float
 *
 * Purpose:     Converts `_Float16' to `float'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fF(FLOAT16, FLOAT, H5__Float16, float, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_double
 *
 * Purpose:     Converts `_Float16' to `double'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fF(FLOAT16, DOUBLE, H5__Float16, double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_ldouble
 *
 * Purpose:     Converts `_Float16' to `long double'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                           const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                           size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fF(FLOAT16, LDOUBLE, H5__Float16, long double, -, -);
}

#ifdef H5_HAVE_COMPLEX_NUMBERS
/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_fcomplex
 *
 * Purpose:     Converts `_Float16' to `float _Complex' / `_Fcomplex'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_fcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fZ(FLOAT16, FLOAT_COMPLEX, H5__Float16, H5_float_complex, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_dcomplex
 *
 * Purpose:     Converts `_Float16' to `double _Complex' / `_Dcomplex'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_dcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fZ(FLOAT16, DOUBLE_COMPLEX, H5__Float16, H5_double_complex, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv__Float16_lcomplex
 *
 * Purpose:     Converts `_Float16' to `long double _Complex' / `_Lcomplex'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv__Float16_lcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fZ(FLOAT16, LDOUBLE_COMPLEX, H5__Float16, H5_ldouble_complex, -, -);
}
#endif
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_schar
 *
 * Purpose:     Convert native float to native signed char using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(FLOAT, SCHAR, float, signed char, SCHAR_MIN, SCHAR_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_uchar
 *
 * Purpose:     Convert native float to native unsigned char using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(FLOAT, UCHAR, float, unsigned char, 0, UCHAR_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_short
 *
 * Purpose:     Convert native float to native short using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(FLOAT, SHORT, float, short, SHRT_MIN, SHRT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_ushort
 *
 * Purpose:     Convert native float to native unsigned short using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(FLOAT, USHORT, float, unsigned short, 0, USHRT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_int
 *
 * Purpose:     Convert native float to native int using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                    size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                    void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(FLOAT, INT, float, int, INT_MIN, INT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_uint
 *
 * Purpose:     Convert native float to native unsigned int using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(FLOAT, UINT, float, unsigned int, 0, UINT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_long
 *
 * Purpose:     Convert native float to native long using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(FLOAT, LONG, float, long, LONG_MIN, LONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_ulong
 *
 * Purpose:     Convert native float to native unsigned long using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(FLOAT, ULONG, float, unsigned long, 0, ULONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_llong
 *
 * Purpose:     Convert native float to native long long using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(FLOAT, LLONG, float, long long, LLONG_MIN, LLONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_ullong
 *
 * Purpose:     Convert native float to native unsigned long long using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(FLOAT, ULLONG, float, unsigned long long, 0, ULLONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

#ifdef H5_HAVE__FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float__Float16
 *
 * Purpose:     Convert native float to native _Float16 using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    /* Suppress warning about non-standard floating-point literal suffix */
    H5_WARN_NONSTD_SUFFIX_OFF
    H5T_CONV_Ff(FLOAT, FLOAT16, float, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_WARN_NONSTD_SUFFIX_ON
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_double
 *
 * Purpose:     Convert native `float' to native `double' using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fF(FLOAT, DOUBLE, float, double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_ldouble
 *
 * Purpose:     Convert native `float' to native `long double' using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fF(FLOAT, LDOUBLE, float, long double, -, -);
}

#ifdef H5_HAVE_COMPLEX_NUMBERS
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_fcomplex
 *
 * Purpose:     Convert native `float' to native
 *              `float _Complex' / `_Fcomplex' using hardware. This is a
 *              fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float_fcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fz(FLOAT, FLOAT_COMPLEX, float, H5_float_complex, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_dcomplex
 *
 * Purpose:     Convert native `float' to native
 *              `double _Complex' / `_Dcomplex' using hardware. This is a
 *              fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float_dcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fZ(FLOAT, DOUBLE_COMPLEX, float, H5_double_complex, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_float_lcomplex
 *
 * Purpose:     Convert native `float' to native
 *              `long double _Complex' / `_Lcomplex' using hardware. This
 *              is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_float_lcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fZ(FLOAT, LDOUBLE_COMPLEX, float, H5_ldouble_complex, -, -);
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double_schar
 *
 * Purpose:     Convert native double to native signed char using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(DOUBLE, SCHAR, double, signed char, SCHAR_MIN, SCHAR_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double_uchar
 *
 * Purpose:     Convert native double to native unsigned char using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(DOUBLE, UCHAR, double, unsigned char, 0, UCHAR_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double_short
 *
 * Purpose:     Convert native double to native short using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(DOUBLE, SHORT, double, short, SHRT_MIN, SHRT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double_ushort
 *
 * Purpose:     Convert native double to native unsigned short using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(DOUBLE, USHORT, double, unsigned short, 0, USHRT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double_int
 *
 * Purpose:     Convert native double to native int using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(DOUBLE, INT, double, int, INT_MIN, INT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double_uint
 *
 * Purpose:     Convert native double to native unsigned int using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(DOUBLE, UINT, double, unsigned int, 0, UINT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double_long
 *
 * Purpose:     Convert native double to native long using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(DOUBLE, LONG, double, long, LONG_MIN, LONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double_ulong
 *
 * Purpose:     Convert native double to native unsigned long using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(DOUBLE, ULONG, double, unsigned long, 0, ULONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double_llong
 *
 * Purpose:     Convert native double to native long long using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(DOUBLE, LLONG, double, long long, LLONG_MIN, LLONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double_ullong
 *
 * Purpose:     Convert native double to native unsigned long long using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(DOUBLE, ULLONG, double, unsigned long long, 0, ULLONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

#ifdef H5_HAVE__FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double__Float16
 *
 * Purpose:     Convert native double to native _Float16 using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    /* Suppress warning about non-standard floating-point literal suffix */
    H5_WARN_NONSTD_SUFFIX_OFF
    H5T_CONV_Ff(DOUBLE, FLOAT16, double, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_WARN_NONSTD_SUFFIX_ON
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double_float
 *
 * Purpose:     Convert native `double' to native `float' using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Ff(DOUBLE, FLOAT, double, float, -FLT_MAX, FLT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double_ldouble
 *
 * Purpose:     Convert native `double' to native `long double' using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fF(DOUBLE, LDOUBLE, double, long double, -, -);
}

#ifdef H5_HAVE_COMPLEX_NUMBERS
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double_fcomplex
 *
 * Purpose:     Convert native `double' to native
 *              `float _Complex' / `_Fcomplex' using hardware. This is a
 *              fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double_fcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Fz(DOUBLE, FLOAT_COMPLEX, double, H5_float_complex, -FLT_MAX, FLT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double_dcomplex
 *
 * Purpose:     Convert native `double' to native
 *              `double _Complex' / `_Dcomplex' using hardware. This is a
 *              fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double_dcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fz(DOUBLE, DOUBLE_COMPLEX, double, H5_double_complex, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_double_lcomplex
 *
 * Purpose:     Convert native `double' to native
 *              `long double _Complex' / `_Lcomplex' using hardware. This
 *              is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_double_lcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fZ(DOUBLE, LDOUBLE_COMPLEX, double, H5_ldouble_complex, -, -);
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble_schar
 *
 * Purpose:     Convert native long double to native signed char using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ldouble_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(LDOUBLE, SCHAR, long double, signed char, SCHAR_MIN, SCHAR_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble_uchar
 *
 * Purpose:     Convert native long double to native unsigned char using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ldouble_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(LDOUBLE, UCHAR, long double, unsigned char, 0, UCHAR_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble_short
 *
 * Purpose:     Convert native long double to native short using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ldouble_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(LDOUBLE, SHORT, long double, short, SHRT_MIN, SHRT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble_ushort
 *
 * Purpose:     Convert native long double to native unsigned short using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ldouble_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(LDOUBLE, USHORT, long double, unsigned short, 0, USHRT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble_int
 *
 * Purpose:     Convert native long double to native int using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ldouble_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(LDOUBLE, INT, long double, int, INT_MIN, INT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble_uint
 *
 * Purpose:     Convert native long double to native unsigned int using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ldouble_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(LDOUBLE, UINT, long double, unsigned int, 0, UINT_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble_long
 *
 * Purpose:     Convert native long double to native long using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ldouble_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(LDOUBLE, LONG, long double, long, LONG_MIN, LONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble_ulong
 *
 * Purpose:     Convert native long double to native unsigned long using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ldouble_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(LDOUBLE, ULONG, long double, unsigned long, 0, ULONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble_llong
 *
 * Purpose:     Convert native long double to native long long using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5T_CONV_INTERNAL_LDOUBLE_LLONG
herr_t
H5T__conv_ldouble_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(LDOUBLE, LLONG, long double, long long, LLONG_MIN, LLONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}
#endif /*H5T_CONV_INTERNAL_LDOUBLE_LLONG*/

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble_ullong
 *
 * Purpose:     Convert native long double to native unsigned long long using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5T_CONV_INTERNAL_LDOUBLE_ULLONG
herr_t
H5T__conv_ldouble_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5_WARN_FLOAT_EQUAL_OFF
    H5T_CONV_Fx(LDOUBLE, ULLONG, long double, unsigned long long, 0, ULLONG_MAX);
    H5_WARN_FLOAT_EQUAL_ON
}
#endif /*H5T_CONV_INTERNAL_LDOUBLE_ULLONG*/

#ifdef H5_HAVE__FLOAT16
#ifdef H5T_CONV_INTERNAL_LDOUBLE_FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble__Float16
 *
 * Purpose:     Convert native long double to native _Float16 using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ldouble__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                           const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                           size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    /* Suppress warning about non-standard floating-point literal suffix */
    H5_WARN_NONSTD_SUFFIX_OFF
    H5T_CONV_Ff(LDOUBLE, FLOAT16, long double, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_WARN_NONSTD_SUFFIX_ON
}
#endif
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble_float
 *
 * Purpose:     Convert native `long double' to native `float' using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ldouble_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Ff(LDOUBLE, FLOAT, long double, float, -FLT_MAX, FLT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble_double
 *
 * Purpose:     Convert native `long double' to native `double' using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ldouble_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Ff(LDOUBLE, DOUBLE, long double, double, -DBL_MAX, DBL_MAX);
}

#ifdef H5_HAVE_COMPLEX_NUMBERS
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble_fcomplex
 *
 * Purpose:     Convert native `long double' to native
 *              `float _Complex' / `_Fcomplex' using hardware. This is a
 *              fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ldouble_fcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                           const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                           size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Fz(LDOUBLE, FLOAT_COMPLEX, long double, H5_float_complex, -FLT_MAX, FLT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble_dcomplex
 *
 * Purpose:     Convert native `long double' to native
 *              `double _Complex' / `_Dcomplex' using hardware. This is a
 *              fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ldouble_dcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                           const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                           size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Fz(LDOUBLE, DOUBLE_COMPLEX, long double, H5_double_complex, -DBL_MAX, DBL_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ldouble_lcomplex
 *
 * Purpose:     Convert native `long double' to native
 *              `long double _Complex' / `_Lcomplex' using hardware. This
 *              is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ldouble_lcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                           const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                           size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_fz(LDOUBLE, LDOUBLE_COMPLEX, long double, H5_ldouble_complex, -, -);
}
#endif
