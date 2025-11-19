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
 * Purpose: Datatype conversion functions for floating-point datatypes
 */

/****************/
/* Module Setup */
/****************/
#include "H5Tmodule.h" /* This source code file is part of the H5T module */

/***********/
/* Headers */
/***********/
#include "H5Tconv.h"
#include "H5Tconv_macros.h"
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
    /* Traversal-related variables */
    H5T_atomic_t src;                  /*atomic source info        */
    H5T_atomic_t dst;                  /*atomic destination info    */
    ssize_t      src_delta, dst_delta; /*source & destination stride    */
    int          direction;            /*forward or backward traversal    */
    size_t       elmtno;               /*element number        */
    size_t       half_size;            /*half the type size        */
    size_t       tsize;                /*type size for swapping bytes  */
    size_t       olap;                 /*num overlapping elements    */
    ssize_t      bitno = 0;            /*bit number            */
    uint8_t     *s, *sp, *d, *dp;      /*source and dest traversal ptrs*/
    uint8_t     *src_rev  = NULL;      /*order-reversed source buffer  */
    uint8_t      dbuf[64] = {0};       /*temp destination buffer    */
    uint8_t      tmp1, tmp2;           /*temp variables for swapping bytes*/

    /* Conversion-related variables */
    int64_t        expo;                 /*exponent            */
    hssize_t       expo_max;             /*maximum possible dst exponent    */
    size_t         msize = 0;            /*useful size of mantissa in src*/
    size_t         mpos;                 /*offset to useful mant is src    */
    uint64_t       sign;                 /*source sign bit value         */
    size_t         mrsh;                 /*amount to right shift mantissa*/
    bool           carry = false;        /*carry after rounding mantissa    */
    size_t         i;                    /*miscellaneous counters    */
    size_t         implied;              /*destination implied bits    */
    bool           denormalized = false; /*is either source or destination denormalized?*/
    H5T_conv_ret_t except_ret;           /*return of callback function   */
    bool           reverse;              /*if reverse the order of destination        */
    herr_t         ret_value = SUCCEED;  /*return value                 */

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT:
            if (NULL == src_p || NULL == dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            src = src_p->shared->u.atomic;
            dst = dst_p->shared->u.atomic;
            if (H5T_ORDER_LE != src.order && H5T_ORDER_BE != src.order && H5T_ORDER_VAX != src.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unsupported byte order");
            if (H5T_ORDER_LE != dst.order && H5T_ORDER_BE != dst.order && H5T_ORDER_VAX != dst.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unsupported byte order");
            if (dst_p->shared->size > sizeof(dbuf))
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "destination size is too large");
            if (8 * sizeof(expo) - 1 < src.u.f.esize || 8 * sizeof(expo) - 1 < dst.u.f.esize)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "exponent field is too large");
            cdata->need_bkg = H5T_BKG_NO;
            break;

        case H5T_CONV_FREE:
            break;

        case H5T_CONV_CONV:
            if (NULL == src_p || NULL == dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (NULL == conv_ctx)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid datatype conversion context pointer");

            src      = src_p->shared->u.atomic;
            dst      = dst_p->shared->u.atomic;
            expo_max = ((hssize_t)1 << dst.u.f.esize) - 1;

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

            /*
             * Direction & size of buffer traversal.
             */
            H5_CHECK_OVERFLOW(buf_stride, size_t, ssize_t);
            H5_CHECK_OVERFLOW(src_p->shared->size, size_t, ssize_t);
            H5_CHECK_OVERFLOW(dst_p->shared->size, size_t, ssize_t);
            src_delta = (ssize_t)direction * (ssize_t)(buf_stride ? buf_stride : src_p->shared->size);
            dst_delta = (ssize_t)direction * (ssize_t)(buf_stride ? buf_stride : dst_p->shared->size);

            /* Allocate space for order-reversed source buffer */
            src_rev = (uint8_t *)H5MM_calloc(src_p->shared->size);

            /* The conversion loop */
            for (elmtno = 0; elmtno < nelmts; elmtno++) {
                /* Set these variables to default */
                except_ret = H5T_CONV_UNHANDLED;
                reverse    = true;

                /*
                 * If the source and destination buffers overlap then use a
                 * temporary buffer for the destination.
                 */
                if (direction > 0) {
                    s = sp;
                    d = elmtno < olap ? dbuf : dp;
                }
                else {
                    s = sp;
                    d = elmtno + olap >= nelmts ? dbuf : dp;
                }
#ifndef NDEBUG
                /* I don't quite trust the overlap calculations yet  */
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
                if (H5T_ORDER_BE == src.order) {
                    half_size = src_p->shared->size / 2;
                    for (i = 0; i < half_size; i++) {
                        tmp1                             = s[src_p->shared->size - (i + 1)];
                        s[src_p->shared->size - (i + 1)] = s[i];
                        s[i]                             = tmp1;
                    }
                }
                else if (H5T_ORDER_VAX == src.order) {
                    tsize = src_p->shared->size;
                    assert(0 == tsize % 2);

                    for (i = 0; i < tsize; i += 4) {
                        tmp1 = s[i];
                        tmp2 = s[i + 1];

                        s[i]     = s[(tsize - 2) - i];
                        s[i + 1] = s[(tsize - 1) - i];

                        s[(tsize - 2) - i] = tmp1;
                        s[(tsize - 1) - i] = tmp2;
                    }
                }

                /*
                 * Find the sign bit value of the source.
                 */
                sign = H5T__bit_get_d(s, src.u.f.sign, (size_t)1);

                /*
                 * Check for special cases: +0, -0, +Inf, -Inf, NaN
                 */
                if (H5T__bit_find(s, src.u.f.mpos, src.u.f.msize, H5T_BIT_LSB, true) < 0) {
                    if (H5T__bit_find(s, src.u.f.epos, src.u.f.esize, H5T_BIT_LSB, true) < 0) {
                        /* +0 or -0 */
                        H5T__bit_copy(d, dst.u.f.sign, s, src.u.f.sign, (size_t)1);
                        H5T__bit_set(d, dst.u.f.epos, dst.u.f.esize, false);
                        H5T__bit_set(d, dst.u.f.mpos, dst.u.f.msize, false);
                        goto padding;
                    }
                    else if (H5T__bit_find(s, src.u.f.epos, src.u.f.esize, H5T_BIT_LSB, false) < 0) {
                        /* +Inf or -Inf */
                        if (conv_ctx->u.conv.cb_struct
                                .func) { /*If user's exception handler is present, use it*/
                            /*reverse order first*/
                            H5T__reverse_order(src_rev, s, src_p->shared->size,
                                               src_p->shared->u.atomic.order);
                            if (sign)
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_NINF, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                            else
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_PINF, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                        }

                        if (except_ret == H5T_CONV_UNHANDLED) {
                            H5T__bit_copy(d, dst.u.f.sign, s, src.u.f.sign, (size_t)1);
                            H5T__bit_set(d, dst.u.f.epos, dst.u.f.esize, true);
                            H5T__bit_set(d, dst.u.f.mpos, dst.u.f.msize, false);
                            /*If the destination no implied mantissa bit, we'll need to set
                             *the 1st bit of mantissa to 1.  The Intel-Linux long double is
                             *this case.*/
                            if (H5T_NORM_NONE == dst.u.f.norm)
                                H5T__bit_set(d, dst.u.f.mpos + dst.u.f.msize - 1, (size_t)1, true);
                        }
                        else if (except_ret == H5T_CONV_HANDLED) {
                            /*No need to reverse the order of destination because user handles it*/
                            reverse = false;
                            goto next;
                        }
                        else if (except_ret == H5T_CONV_ABORT)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "can't handle conversion exception");

                        goto padding;
                    }
                }
                else if (H5T_NORM_NONE == src.u.f.norm &&
                         H5T__bit_find(s, src.u.f.mpos, src.u.f.msize - 1, H5T_BIT_LSB, true) < 0 &&
                         H5T__bit_find(s, src.u.f.epos, src.u.f.esize, H5T_BIT_LSB, false) < 0) {
                    /*This is a special case for the source of no implied mantissa bit.
                     *If the exponent bits are all 1s and only the 1st bit of mantissa
                     *is set to 1.  It's infinity. The Intel-Linux "long double" is this case.*/
                    /* +Inf or -Inf */
                    if (conv_ctx->u.conv.cb_struct.func) { /*If user's exception handler is present, use it*/
                        /*reverse order first*/
                        H5T__reverse_order(src_rev, s, src_p->shared->size, src_p->shared->u.atomic.order);
                        if (sign)
                            except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                H5T_CONV_EXCEPT_NINF, conv_ctx->u.conv.src_type_id,
                                conv_ctx->u.conv.dst_type_id, src_rev, d,
                                conv_ctx->u.conv.cb_struct.user_data);
                        else
                            except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                H5T_CONV_EXCEPT_PINF, conv_ctx->u.conv.src_type_id,
                                conv_ctx->u.conv.dst_type_id, src_rev, d,
                                conv_ctx->u.conv.cb_struct.user_data);
                    }

                    if (except_ret == H5T_CONV_UNHANDLED) {
                        H5T__bit_copy(d, dst.u.f.sign, s, src.u.f.sign, (size_t)1);
                        H5T__bit_set(d, dst.u.f.epos, dst.u.f.esize, true);
                        H5T__bit_set(d, dst.u.f.mpos, dst.u.f.msize, false);
                        /*If the destination no implied mantissa bit, we'll need to set
                         *the 1st bit of mantissa to 1.  The Intel-Linux long double is
                         *this case.*/
                        if (H5T_NORM_NONE == dst.u.f.norm)
                            H5T__bit_set(d, dst.u.f.mpos + dst.u.f.msize - 1, (size_t)1, true);
                    }
                    else if (except_ret == H5T_CONV_HANDLED) {
                        /*No need to reverse the order of destination because user handles it*/
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
                else if (H5T__bit_find(s, src.u.f.epos, src.u.f.esize, H5T_BIT_LSB, false) < 0) {
                    /* NaN */
                    if (conv_ctx->u.conv.cb_struct.func) { /*If user's exception handler is present, use it*/
                        /*reverse order first*/
                        H5T__reverse_order(src_rev, s, src_p->shared->size, src_p->shared->u.atomic.order);
                        except_ret = (conv_ctx->u.conv.cb_struct.func)(
                            H5T_CONV_EXCEPT_NAN, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id,
                            src_rev, d, conv_ctx->u.conv.cb_struct.user_data);
                    }

                    if (except_ret == H5T_CONV_UNHANDLED) {
                        /* There are many NaN values, so we just set all bits of
                         * the significand. */
                        H5T__bit_copy(d, dst.u.f.sign, s, src.u.f.sign, (size_t)1);
                        H5T__bit_set(d, dst.u.f.epos, dst.u.f.esize, true);
                        H5T__bit_set(d, dst.u.f.mpos, dst.u.f.msize, true);
                    }
                    else if (except_ret == H5T_CONV_HANDLED) {
                        /*No need to reverse the order of destination because user handles it*/
                        reverse = false;
                        goto next;
                    }
                    else if (except_ret == H5T_CONV_ABORT)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");

                    goto padding;
                }

                /*
                 * Get the exponent as an unsigned quantity from the section of
                 * the source bit field where it's located.     Don't worry about
                 * the exponent bias yet.
                 */
                expo = (int64_t)H5T__bit_get_d(s, src.u.f.epos, src.u.f.esize);

                if (expo == 0)
                    denormalized = true;

                /*
                 * Set markers for the source mantissa, excluding the leading `1'
                 * (might be implied).
                 */
                implied = 1;
                mpos    = src.u.f.mpos;
                mrsh    = 0;
                if (0 == expo || H5T_NORM_NONE == src.u.f.norm) {
                    if ((bitno = H5T__bit_find(s, src.u.f.mpos, src.u.f.msize, H5T_BIT_MSB, true)) > 0) {
                        msize = (size_t)bitno;
                    }
                    else if (0 == bitno) {
                        msize = 1;
                        H5T__bit_set(s, src.u.f.mpos, (size_t)1, false);
                    }
                }
                else if (H5T_NORM_IMPLIED == src.u.f.norm) {
                    msize = src.u.f.msize;
                }
                else {
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                "normalization method not implemented yet");
                }

                /*
                 * The sign for the destination is the same as the sign for the
                 * source in all cases.
                 */
                H5T__bit_copy(d, dst.u.f.sign, s, src.u.f.sign, (size_t)1);

                /*
                 * Calculate the true source exponent by adjusting according to
                 * the source exponent bias.
                 */
                if (0 == expo || H5T_NORM_NONE == src.u.f.norm) {
                    assert(bitno >= 0);
                    expo -= (int64_t)((src.u.f.ebias - 1) + (src.u.f.msize - (size_t)bitno));
                }
                else if (H5T_NORM_IMPLIED == src.u.f.norm) {
                    expo -= (int64_t)src.u.f.ebias;
                }
                else {
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                "normalization method not implemented yet");
                }

                /*
                 * If the destination is not normalized then right shift the
                 * mantissa by one.
                 */
                if (H5T_NORM_NONE == dst.u.f.norm)
                    mrsh++;

                /*
                 * Calculate the destination exponent by adding the destination
                 * bias and clipping by the minimum and maximum possible
                 * destination exponent values.
                 */
                expo += (int64_t)dst.u.f.ebias;

                if (expo < -(hssize_t)(dst.u.f.msize)) {
                    /* The exponent is way too small.  Result is zero. */
                    expo = 0;
                    H5T__bit_set(d, dst.u.f.mpos, dst.u.f.msize, false);
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
                     * or it results in the maximum possible value.     Use positive
                     * or negative infinity instead unless the application
                     * specifies something else.  Before calling the overflow
                     * handler make sure the source buffer we hand it is in the
                     * original byte order.
                     */
                    if (conv_ctx->u.conv.cb_struct.func) { /*If user's exception handler is present, use it*/
                        /*reverse order first*/
                        H5T__reverse_order(src_rev, s, src_p->shared->size, src_p->shared->u.atomic.order);
                        except_ret = (conv_ctx->u.conv.cb_struct.func)(
                            H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                            conv_ctx->u.conv.dst_type_id, src_rev, d, conv_ctx->u.conv.cb_struct.user_data);
                    }

                    if (except_ret == H5T_CONV_UNHANDLED) {
                        expo = expo_max;
                        H5T__bit_set(d, dst.u.f.mpos, dst.u.f.msize, false);
                        msize = 0;
                    }
                    else if (except_ret == H5T_CONV_ABORT)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");
                    else if (except_ret == H5T_CONV_HANDLED) {
                        reverse = false;
                        goto next;
                    }
                }

                /*
                 * If the destination mantissa is smaller than the source
                 * mantissa then round the source mantissa. Rounding may cause a
                 * carry in which case the exponent has to be re-evaluated for
                 * overflow.  That is, if `carry' is clear then the implied
                 * mantissa bit is `1', else it is `10' binary.
                 */
                if (msize > 0 && mrsh <= dst.u.f.msize && mrsh + msize > dst.u.f.msize) {
                    bitno = (ssize_t)(mrsh + msize - dst.u.f.msize);
                    assert(bitno >= 0 && (size_t)bitno <= msize);
                    /* If the 1st bit being cut off is set and source isn't denormalized.*/
                    if (H5T__bit_get_d(s, (mpos + (size_t)bitno) - 1, (size_t)1) && !denormalized) {
                        /* Don't do rounding if exponent is 111...110 and mantissa is 111...11.
                         * To do rounding and increment exponent in this case will create an infinity value.*/
                        if ((H5T__bit_find(s, mpos + (size_t)bitno, msize - (size_t)bitno, H5T_BIT_LSB,
                                           false) >= 0 ||
                             expo < expo_max - 1)) {
                            carry = H5T__bit_inc(s, mpos + (size_t)bitno - 1, 1 + msize - (size_t)bitno);
                            if (carry)
                                implied = 2;
                        }
                    }
                    else if (H5T__bit_get_d(s, (mpos + (size_t)bitno) - 1, (size_t)1) && denormalized)
                        /* For either source or destination, denormalized value doesn't increment carry.*/
                        H5T__bit_inc(s, mpos + (size_t)bitno - 1, 1 + msize - (size_t)bitno);
                }
                else
                    carry = false;

                /*
                 * Write the mantissa to the destination
                 */
                if (mrsh > dst.u.f.msize + 1) {
                    H5T__bit_set(d, dst.u.f.mpos, dst.u.f.msize, false);
                }
                else if (mrsh == dst.u.f.msize + 1) {
                    H5T__bit_set(d, dst.u.f.mpos + 1, dst.u.f.msize - 1, false);
                    H5T__bit_set(d, dst.u.f.mpos, (size_t)1, true);
                }
                else if (mrsh == dst.u.f.msize) {
                    H5T__bit_set(d, dst.u.f.mpos, dst.u.f.msize, false);
                    H5T__bit_set_d(d, dst.u.f.mpos, MIN(2, dst.u.f.msize), (hsize_t)implied);
                }
                else {
                    if (mrsh > 0) {
                        H5T__bit_set(d, dst.u.f.mpos + dst.u.f.msize - mrsh, mrsh, false);
                        H5T__bit_set_d(d, dst.u.f.mpos + dst.u.f.msize - mrsh, (size_t)2, (hsize_t)implied);
                    }
                    if (mrsh + msize >= dst.u.f.msize) {
                        H5T__bit_copy(d, dst.u.f.mpos, s, (mpos + msize + mrsh - dst.u.f.msize),
                                      dst.u.f.msize - mrsh);
                    }
                    else {
                        H5T__bit_copy(d, dst.u.f.mpos + dst.u.f.msize - (mrsh + msize), s, mpos, msize);
                        H5T__bit_set(d, dst.u.f.mpos, dst.u.f.msize - (mrsh + msize), false);
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
                         * application specifies something else.  Before
                         * calling the overflow handler make sure the source
                         * buffer we hand it is in the original byte order.
                         */
                        if (conv_ctx->u.conv.cb_struct
                                .func) { /*If user's exception handler is present, use it*/
                            /*reverse order first*/
                            H5T__reverse_order(src_rev, s, src_p->shared->size,
                                               src_p->shared->u.atomic.order);
                            except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                                conv_ctx->u.conv.dst_type_id, src_rev, d,
                                conv_ctx->u.conv.cb_struct.user_data);
                        }

                        if (except_ret == H5T_CONV_UNHANDLED) {
                            expo = expo_max;
                            H5T__bit_set(d, dst.u.f.mpos, dst.u.f.msize, false);
                        }
                        else if (except_ret == H5T_CONV_ABORT)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "can't handle conversion exception");
                        else if (except_ret == H5T_CONV_HANDLED) {
                            reverse = false;
                            goto next;
                        }
                    }
                }
                /*reset CARRY*/
                carry = false;

                H5_CHECK_OVERFLOW(expo, hssize_t, hsize_t);
                H5T__bit_set_d(d, dst.u.f.epos, dst.u.f.esize, (hsize_t)expo);

padding:

                /*
                 * Set external padding areas
                 */
                if (dst.offset > 0) {
                    assert(H5T_PAD_ZERO == dst.lsb_pad || H5T_PAD_ONE == dst.lsb_pad);
                    H5T__bit_set(d, (size_t)0, dst.offset, (bool)(H5T_PAD_ONE == dst.lsb_pad));
                }
                if (dst.offset + dst.prec != 8 * dst_p->shared->size) {
                    assert(H5T_PAD_ZERO == dst.msb_pad || H5T_PAD_ONE == dst.msb_pad);
                    H5T__bit_set(d, dst.offset + dst.prec, 8 * dst_p->shared->size - (dst.offset + dst.prec),
                                 (bool)(H5T_PAD_ONE == dst.msb_pad));
                }

                /*
                 * Put the destination in the correct byte order.  See note at
                 * beginning of loop.
                 */
                if (H5T_ORDER_BE == dst.order && reverse) {
                    half_size = dst_p->shared->size / 2;
                    for (i = 0; i < half_size; i++) {
                        uint8_t tmp                      = d[dst_p->shared->size - (i + 1)];
                        d[dst_p->shared->size - (i + 1)] = d[i];
                        d[i]                             = tmp;
                    }
                }
                else if (H5T_ORDER_VAX == dst.order && reverse) {
                    tsize = dst_p->shared->size;
                    assert(0 == tsize % 2);

                    for (i = 0; i < tsize; i += 4) {
                        tmp1 = d[i];
                        tmp2 = d[i + 1];

                        d[i]     = d[(tsize - 2) - i];
                        d[i + 1] = d[(tsize - 1) - i];

                        d[(tsize - 2) - i] = tmp1;
                        d[(tsize - 1) - i] = tmp2;
                    }
                }

                /*
                 * If we had used a temporary buffer for the destination then we
                 * should copy the value to the true destination buffer.
                 */
next:
                if (d == dbuf)
                    H5MM_memcpy(dp, d, dst_p->shared->size);

                /* Advance source & destination pointers by delta amounts */
                sp += src_delta;
                dp += dst_delta;
            }

            break;

        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    } /* end switch */

done:
    if (src_rev)
        H5MM_free(src_rev);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_f_f() */

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
    /* Traversal-related variables */
    H5T_atomic_t src;             /*atomic source info        */
    H5T_atomic_t dst;             /*atomic destination info    */
    int          direction;       /*forward or backward traversal    */
    size_t       elmtno;          /*element number        */
    size_t       half_size;       /*half the type size        */
    size_t       tsize;           /*type size for swapping bytes  */
    size_t       olap;            /*num overlapping elements    */
    uint8_t     *s, *sp, *d, *dp; /*source and dest traversal ptrs*/
    uint8_t     *src_rev  = NULL; /*order-reversed source buffer  */
    uint8_t      dbuf[64] = {0};  /*temp destination buffer    */
    uint8_t      tmp1, tmp2;      /*temp variables for swapping bytes*/

    /* Conversion-related variables */
    hssize_t       expo;                /*source exponent        */
    hssize_t       sign;                /*source sign bit value         */
    uint8_t       *int_buf = NULL;      /*buffer for temporary value    */
    size_t         buf_size;            /*buffer size for temporary value */
    size_t         i;                   /*miscellaneous counters    */
    ssize_t        msb_pos_s;           /*first bit(MSB) in an integer */
    ssize_t        new_msb_pos;         /*MSB position after shifting mantissa by exponent */
    hssize_t       shift_val;           /*shift value when shifting mantissa by exponent */
    bool           truncated;           /*if fraction value is dropped  */
    bool           reverse;             /*if reverse order of destination at the end */
    H5T_conv_ret_t except_ret;          /*return of callback function   */
    herr_t         ret_value = SUCCEED; /* Return value                 */

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT:
            if (NULL == src_p || NULL == dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            src = src_p->shared->u.atomic;
            dst = dst_p->shared->u.atomic;
            if (H5T_ORDER_LE != src.order && H5T_ORDER_BE != src.order && H5T_ORDER_VAX != src.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unsupported byte order");
            if (dst_p->shared->size > sizeof(dbuf))
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "destination size is too large");
            if (8 * sizeof(expo) - 1 < src.u.f.esize)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "exponent field is too large");
            cdata->need_bkg = H5T_BKG_NO;
            break;

        case H5T_CONV_FREE:
            break;

        case H5T_CONV_CONV:
            if (NULL == src_p || NULL == dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (NULL == conv_ctx)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid datatype conversion context pointer");

            src = src_p->shared->u.atomic;
            dst = dst_p->shared->u.atomic;

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

            /* Allocate enough space for the buffer holding temporary
             * converted value
             */
            if (dst.prec / 8 > src_p->shared->size)
                buf_size = (dst.prec + 7) / 8;
            else
                buf_size = src_p->shared->size;
            int_buf = (uint8_t *)H5MM_calloc(buf_size);

            /* Allocate space for order-reversed source buffer */
            src_rev = (uint8_t *)H5MM_calloc(src_p->shared->size);

            /* The conversion loop */
            for (elmtno = 0; elmtno < nelmts; elmtno++) {
                /* Set these variables to default */
                except_ret = H5T_CONV_UNHANDLED;
                truncated  = false;
                reverse    = true;

                /*
                 * If the source and destination buffers overlap then use a
                 * temporary buffer for the destination.
                 */
                if (direction > 0) {
                    s = sp;
                    d = elmtno < olap ? dbuf : dp;
                }
                else {
                    s = sp;
                    d = elmtno + olap >= nelmts ? dbuf : dp;
                }
#ifndef NDEBUG
                /* I don't quite trust the overlap calculations yet  */
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
                if (H5T_ORDER_BE == src.order) {
                    half_size = src_p->shared->size / 2;
                    for (i = 0; i < half_size; i++) {
                        tmp1                             = s[src_p->shared->size - (i + 1)];
                        s[src_p->shared->size - (i + 1)] = s[i];
                        s[i]                             = tmp1;
                    }
                }
                else if (H5T_ORDER_VAX == src.order) {
                    tsize = src_p->shared->size;
                    assert(0 == tsize % 2);

                    for (i = 0; i < tsize; i += 4) {
                        tmp1 = s[i];
                        tmp2 = s[i + 1];

                        s[i]     = s[(tsize - 2) - i];
                        s[i + 1] = s[(tsize - 1) - i];

                        s[(tsize - 2) - i] = tmp1;
                        s[(tsize - 1) - i] = tmp2;
                    }
                }

                /*zero-set all destination bits*/
                H5T__bit_set(d, dst.offset, dst.prec, false);

                /*
                 * Find the sign bit value of the source.
                 */
                sign = (hssize_t)H5T__bit_get_d(s, src.u.f.sign, (size_t)1);

                /*
                 * Check for special cases: +0, -0, +Inf, -Inf, NaN
                 */
                if (H5T__bit_find(s, src.u.f.mpos, src.u.f.msize, H5T_BIT_LSB, true) < 0) {
                    if (H5T__bit_find(s, src.u.f.epos, src.u.f.esize, H5T_BIT_LSB, true) < 0) {
                        /* +0 or -0 */
                        /* Set all bits to zero */
                        goto padding;
                    }
                    else if (H5T__bit_find(s, src.u.f.epos, src.u.f.esize, H5T_BIT_LSB, false) < 0) {
                        /* +Infinity or -Infinity */
                        if (sign) { /* -Infinity */
                            if (conv_ctx->u.conv.cb_struct
                                    .func) { /*If user's exception handler is present, use it*/
                                /*reverse order first*/
                                H5T__reverse_order(src_rev, s, src_p->shared->size,
                                                   src_p->shared->u.atomic.order);
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_NINF, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                            }

                            if (except_ret == H5T_CONV_UNHANDLED) {
                                if (H5T_SGN_2 == dst.u.i.sign)
                                    H5T__bit_set(d, dst.prec - 1, (size_t)1, true);
                            }
                            else if (except_ret == H5T_CONV_HANDLED) {
                                /*No need to reverse the order of destination because user handles it*/
                                reverse = false;
                                goto next;
                            }
                            else if (except_ret == H5T_CONV_ABORT)
                                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                            "can't handle conversion exception");
                        }
                        else { /* +Infinity */
                            if (conv_ctx->u.conv.cb_struct
                                    .func) { /*If user's exception handler is present, use it*/
                                /*reverse order first*/
                                H5T__reverse_order(src_rev, s, src_p->shared->size,
                                                   src_p->shared->u.atomic.order);
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_PINF, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                            }

                            if (except_ret == H5T_CONV_UNHANDLED) {
                                if (H5T_SGN_NONE == dst.u.i.sign)
                                    H5T__bit_set(d, dst.offset, dst.prec, true);
                                else if (H5T_SGN_2 == dst.u.i.sign)
                                    H5T__bit_set(d, dst.offset, dst.prec - 1, true);
                            }
                            else if (except_ret == H5T_CONV_HANDLED) {
                                /*No need to reverse the order of destination because user handles it*/
                                reverse = false;
                                goto next;
                            }
                            else if (except_ret == H5T_CONV_ABORT)
                                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                            "can't handle conversion exception");
                        }
                        goto padding;
                    }
                }
                else if (H5T_NORM_NONE == src.u.f.norm &&
                         H5T__bit_find(s, src.u.f.mpos, src.u.f.msize - 1, H5T_BIT_LSB, true) < 0 &&
                         H5T__bit_find(s, src.u.f.epos, src.u.f.esize, H5T_BIT_LSB, false) < 0) {
                    /*This is a special case for the source of no implied mantissa bit.
                     *If the exponent bits are all 1s and only the 1st bit of mantissa
                     *is set to 1.  It's infinity. The Intel-Linux "long double" is this case.*/
                    /* +Infinity or -Infinity */
                    if (sign) { /* -Infinity */
                        if (conv_ctx->u.conv.cb_struct
                                .func) { /*If user's exception handler is present, use it*/
                            /*reverse order first*/
                            H5T__reverse_order(src_rev, s, src_p->shared->size,
                                               src_p->shared->u.atomic.order);
                            except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                H5T_CONV_EXCEPT_NINF, conv_ctx->u.conv.src_type_id,
                                conv_ctx->u.conv.dst_type_id, src_rev, d,
                                conv_ctx->u.conv.cb_struct.user_data);
                        }

                        if (except_ret == H5T_CONV_UNHANDLED) {
                            if (H5T_SGN_2 == dst.u.i.sign)
                                H5T__bit_set(d, dst.prec - 1, (size_t)1, true);
                        }
                        else if (except_ret == H5T_CONV_HANDLED) {
                            /*No need to reverse the order of destination because user handles it*/
                            reverse = false;
                            goto next;
                        }
                        else if (except_ret == H5T_CONV_ABORT)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "can't handle conversion exception");
                    }
                    else { /* +Infinity */
                        if (conv_ctx->u.conv.cb_struct
                                .func) { /*If user's exception handler is present, use it*/
                            /*reverse order first*/
                            H5T__reverse_order(src_rev, s, src_p->shared->size,
                                               src_p->shared->u.atomic.order);
                            except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                H5T_CONV_EXCEPT_PINF, conv_ctx->u.conv.src_type_id,
                                conv_ctx->u.conv.dst_type_id, src_rev, d,
                                conv_ctx->u.conv.cb_struct.user_data);
                        }

                        if (except_ret == H5T_CONV_UNHANDLED) {
                            if (H5T_SGN_NONE == dst.u.i.sign)
                                H5T__bit_set(d, dst.offset, dst.prec, true);
                            else if (H5T_SGN_2 == dst.u.i.sign)
                                H5T__bit_set(d, dst.offset, dst.prec - 1, true);
                        }
                        else if (except_ret == H5T_CONV_HANDLED) {
                            /*No need to reverse the order of destination because user handles it*/
                            reverse = false;
                            goto next;
                        }
                        else if (except_ret == H5T_CONV_ABORT)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "can't handle conversion exception");
                    }
                    goto padding;
                }
                else if (H5T__bit_find(s, src.u.f.epos, src.u.f.esize, H5T_BIT_LSB, false) < 0) {
                    /* NaN */
                    if (conv_ctx->u.conv.cb_struct.func) { /*If user's exception handler is present, use it*/
                        /*reverse order first*/
                        H5T__reverse_order(src_rev, s, src_p->shared->size, src_p->shared->u.atomic.order);
                        except_ret = (conv_ctx->u.conv.cb_struct.func)(
                            H5T_CONV_EXCEPT_NAN, conv_ctx->u.conv.src_type_id, conv_ctx->u.conv.dst_type_id,
                            src_rev, d, conv_ctx->u.conv.cb_struct.user_data);
                    }

                    if (except_ret == H5T_CONV_UNHANDLED) {
                        /*Just set all bits to zero.*/
                        goto padding;
                    }
                    else if (except_ret == H5T_CONV_HANDLED) {
                        /*No need to reverse the order of destination because user handles it*/
                        reverse = false;
                        goto next;
                    }
                    else if (except_ret == H5T_CONV_ABORT)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");

                    goto padding;
                }

                /*
                 * Get the exponent as an unsigned quantity from the section of
                 * the source bit field where it's located.   Not expecting
                 * exponent to be greater than the maximal value of hssize_t.
                 */
                expo = (hssize_t)H5T__bit_get_d(s, src.u.f.epos, src.u.f.esize);

                /*
                 * Calculate the true source exponent by adjusting according to
                 * the source exponent bias.
                 */
                if (0 == expo || H5T_NORM_NONE == src.u.f.norm) {
                    expo -= (hssize_t)(src.u.f.ebias - 1);
                }
                else if (H5T_NORM_IMPLIED == src.u.f.norm) {
                    expo -= (hssize_t)src.u.f.ebias;
                }
                else {
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                "normalization method not implemented yet");
                }

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
                H5T__bit_copy(int_buf, (size_t)0, s, src.u.f.mpos, src.u.f.msize);

                /*
                 * Restore the implicit bit for mantissa if it's implied.
                 * Equivalent to mantissa |= (hsize_t)1<<src.u.f.msize.
                 */
                if (H5T_NORM_IMPLIED == src.u.f.norm)
                    H5T__bit_inc(int_buf, src.u.f.msize, 8 * buf_size - src.u.f.msize);

                /*
                 * What is the bit position for the most significant bit(MSB) of S
                 * which is set?  This is checked before shifting and before possibly
                 * converting to a negative integer. Note that later use of this value
                 * assumes that H5T__bit_shift will always shift in 0 during a right
                 * shift.
                 */
                msb_pos_s = H5T__bit_find(int_buf, (size_t)0, src.prec, H5T_BIT_MSB, true);

                /*
                 * The temporary buffer has no bits set and must therefore be
                 * zero; nothing to do.
                 */
                if (msb_pos_s < 0)
                    goto padding;

                /*
                 * Shift mantissa part by exponent minus mantissa size(right shift),
                 * or by mantissa size minus exponent(left shift).  Example: Sequence
                 * 10...010111, expo=20, expo-msize=-3.  Right-shift the sequence, we get
                 * 00010...10.  The last three bits were dropped.
                 */
                shift_val = expo - (ssize_t)src.u.f.msize;
                H5T__bit_shift(int_buf, shift_val, (size_t)0, buf_size * 8);

                /* Calculate the new position of the MSB after shifting and
                 * skip to the padding section if we shifted exactly to 0
                 * (MSB position is -1)
                 */
                new_msb_pos = msb_pos_s + shift_val;
                if (new_msb_pos == -1)
                    goto padding;

                /*
                 * If expo is less than mantissa size, the fractional value is dropped off
                 * during conversion.  Set exception type to be "truncate"
                 */
                if ((size_t)expo < src.u.f.msize && conv_ctx->u.conv.cb_struct.func)
                    truncated = true;

                if (H5T_SGN_NONE == dst.u.i.sign) { /*destination is unsigned*/
                    /*
                     * Destination is unsigned.  Library's default way: If the source value
                     * is greater than the maximal destination value then it overflows, the
                     * destination will be set to the maximum possible value.  When the
                     * source is negative, underflow happens.  Set the destination to be
                     * zero(do nothing).  If user's exception handler is set, call it and
                     * let user handle it.
                     */
                    if (sign) { /*source is negative*/
                        if (conv_ctx->u.conv.cb_struct
                                .func) { /*If user's exception handler is present, use it*/
                            /*reverse order first*/
                            H5T__reverse_order(src_rev, s, src_p->shared->size,
                                               src_p->shared->u.atomic.order);
                            except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                H5T_CONV_EXCEPT_RANGE_LOW, conv_ctx->u.conv.src_type_id,
                                conv_ctx->u.conv.dst_type_id, src_rev, d,
                                conv_ctx->u.conv.cb_struct.user_data);
                            if (except_ret == H5T_CONV_ABORT)
                                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                            "can't handle conversion exception");
                            else if (except_ret == H5T_CONV_HANDLED) {
                                /*No need to reverse the order of destination because user handles it*/
                                reverse = false;
                                goto next;
                            }
                        }
                    }
                    else { /*source is positive*/
                        if (new_msb_pos >= (ssize_t)dst.prec) {
                            /*overflow*/
                            if (conv_ctx->u.conv.cb_struct
                                    .func) { /*If user's exception handler is present, use it*/
                                /*reverse order first*/
                                H5T__reverse_order(src_rev, s, src_p->shared->size,
                                                   src_p->shared->u.atomic.order);
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                            }

                            if (except_ret == H5T_CONV_UNHANDLED)
                                H5T__bit_set(d, dst.offset, dst.prec, true);
                            else if (except_ret == H5T_CONV_HANDLED) {
                                /*No need to reverse the order of destination because user handles it*/
                                reverse = false;
                                goto next;
                            }
                            else if (except_ret == H5T_CONV_ABORT)
                                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                            "can't handle conversion exception");
                        }
                        else {
                            if (truncated && conv_ctx->u.conv.cb_struct
                                                 .func) { /*If user's exception handler is present, use it*/
                                /*reverse order first*/
                                H5T__reverse_order(src_rev, s, src_p->shared->size,
                                                   src_p->shared->u.atomic.order);
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_TRUNCATE, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                            }

                            if (except_ret == H5T_CONV_UNHANDLED) {
                                /*copy source value into it if case is ignored by user handler*/
                                if (new_msb_pos >= 0)
                                    H5T__bit_copy(d, dst.offset, int_buf, (size_t)0, (size_t)new_msb_pos + 1);
                            }
                            else if (except_ret == H5T_CONV_HANDLED) {
                                /*No need to reverse the order of destination because user handles it*/
                                reverse = false;
                                goto next;
                            }
                            else if (except_ret == H5T_CONV_ABORT)
                                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                            "can't handle conversion exception");
                        }
                    }
                }
                else if (H5T_SGN_2 == dst.u.i.sign) { /*Destination is signed*/
                    if (sign) {                       /*source is negative*/
                        if ((new_msb_pos >= 0) && ((size_t)new_msb_pos < dst.prec - 1)) {
                            if (truncated && conv_ctx->u.conv.cb_struct
                                                 .func) { /*If user's exception handler is present, use it*/
                                /*reverse order first*/
                                H5T__reverse_order(src_rev, s, src_p->shared->size,
                                                   src_p->shared->u.atomic.order);
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_TRUNCATE, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                            }

                            if (except_ret == H5T_CONV_UNHANDLED) { /*If this case ignored by user handler*/
                                /*Convert to integer representation.  Equivalent to ~(value - 1).*/
                                H5T__bit_dec(int_buf, (size_t)0, dst.prec);
                                H5T__bit_neg(int_buf, (size_t)0, dst.prec);

                                /*copy source value into destination*/
                                H5T__bit_copy(d, dst.offset, int_buf, (size_t)0, dst.prec - 1);
                                H5T__bit_set(d, (dst.offset + dst.prec - 1), (size_t)1, true);
                            }
                            else if (except_ret == H5T_CONV_ABORT)
                                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                            "can't handle conversion exception");
                            else if (except_ret == H5T_CONV_HANDLED) {
                                /*No need to reverse the order of destination because user handles it*/
                                reverse = false;
                                goto next;
                            }
                        }
                        else {
                            /* if underflows and no callback, do nothing except turn on
                             * the sign bit because 0x80...00 is the biggest negative value.
                             */
                            if (conv_ctx->u.conv.cb_struct
                                    .func) { /*If user's exception handler is present, use it*/
                                /*reverse order first*/
                                H5T__reverse_order(src_rev, s, src_p->shared->size,
                                                   src_p->shared->u.atomic.order);
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_RANGE_LOW, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                            }

                            if (except_ret == H5T_CONV_UNHANDLED)
                                H5T__bit_set(d, (dst.offset + dst.prec - 1), (size_t)1, true);
                            else if (except_ret == H5T_CONV_ABORT)
                                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                            "can't handle conversion exception");
                            else if (except_ret == H5T_CONV_HANDLED) {
                                /*No need to reverse the order of destination because user handles it*/
                                reverse = false;
                                goto next;
                            }
                        }
                    }
                    else { /*source is positive*/
                        if (new_msb_pos >= (ssize_t)dst.prec - 1) {
                            /*overflow*/
                            if (conv_ctx->u.conv.cb_struct
                                    .func) { /*If user's exception handler is present, use it*/
                                /*reverse order first*/
                                H5T__reverse_order(src_rev, s, src_p->shared->size,
                                                   src_p->shared->u.atomic.order);
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                            }

                            if (except_ret == H5T_CONV_UNHANDLED)
                                H5T__bit_set(d, dst.offset, dst.prec - 1, true);
                            else if (except_ret == H5T_CONV_ABORT)
                                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                            "can't handle conversion exception");
                            else if (except_ret == H5T_CONV_HANDLED) {
                                /*No need to reverse the order of destination because user handles it*/
                                reverse = false;
                                goto next;
                            }
                        }
                        else if (new_msb_pos < (ssize_t)dst.prec - 1) {
                            if (truncated && conv_ctx->u.conv.cb_struct
                                                 .func) { /*If user's exception handler is present, use it*/
                                /*reverse order first*/
                                H5T__reverse_order(src_rev, s, src_p->shared->size,
                                                   src_p->shared->u.atomic.order);
                                except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                    H5T_CONV_EXCEPT_TRUNCATE, conv_ctx->u.conv.src_type_id,
                                    conv_ctx->u.conv.dst_type_id, src_rev, d,
                                    conv_ctx->u.conv.cb_struct.user_data);
                            }

                            if (except_ret == H5T_CONV_UNHANDLED) {
                                /*copy source value into it if case is ignored by user handler*/
                                if (new_msb_pos >= 0)
                                    H5T__bit_copy(d, dst.offset, int_buf, (size_t)0, (size_t)new_msb_pos + 1);
                            }
                            else if (except_ret == H5T_CONV_ABORT)
                                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                            "can't handle conversion exception");
                            else if (except_ret == H5T_CONV_HANDLED) {
                                /*No need to reverse the order of destination because user handles it*/
                                reverse = false;
                                goto next;
                            }
                        }
                    }
                }

padding:
                /*
                 * Set padding areas in destination.
                 */
                if (dst.offset > 0) {
                    assert(H5T_PAD_ZERO == dst.lsb_pad || H5T_PAD_ONE == dst.lsb_pad);
                    H5T__bit_set(d, (size_t)0, dst.offset, (bool)(H5T_PAD_ONE == dst.lsb_pad));
                }
                if (dst.offset + dst.prec != 8 * dst_p->shared->size) {
                    assert(H5T_PAD_ZERO == dst.msb_pad || H5T_PAD_ONE == dst.msb_pad);
                    H5T__bit_set(d, dst.offset + dst.prec, 8 * dst_p->shared->size - (dst.offset + dst.prec),
                                 (bool)(H5T_PAD_ONE == dst.msb_pad));
                }

                /*
                 * Put the destination in the correct byte order.  See note at
                 * beginning of loop.
                 */
                if (H5T_ORDER_BE == dst.order && reverse) {
                    half_size = dst_p->shared->size / 2;
                    for (i = 0; i < half_size; i++) {
                        tmp1                             = d[dst_p->shared->size - (i + 1)];
                        d[dst_p->shared->size - (i + 1)] = d[i];
                        d[i]                             = tmp1;
                    }
                }

next:
                /*
                 * If we had used a temporary buffer for the destination then we
                 * should copy the value to the true destination buffer.
                 */
                if (d == dbuf)
                    H5MM_memcpy(dp, d, dst_p->shared->size);
                if (buf_stride) {
                    sp += direction * (ssize_t)buf_stride;
                    dp += direction * (ssize_t)buf_stride;
                }
                else {
                    sp += direction * (ssize_t)src_p->shared->size;
                    dp += direction * (ssize_t)dst_p->shared->size;
                }

                memset(int_buf, 0, buf_size);
            }

            break;

        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    } /* end switch */

done:
    if (int_buf)
        H5MM_xfree(int_buf);
    if (src_rev)
        H5MM_free(src_rev);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_f_i() */

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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(FLOAT16, SCHAR, H5__Float16, signed char, SCHAR_MIN, SCHAR_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(FLOAT16, UCHAR, H5__Float16, unsigned char, 0, UCHAR_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(FLOAT16, SHORT, H5__Float16, short, SHRT_MIN, SHRT_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(FLOAT, SCHAR, float, signed char, SCHAR_MIN, SCHAR_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(FLOAT, UCHAR, float, unsigned char, 0, UCHAR_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(FLOAT, SHORT, float, short, SHRT_MIN, SHRT_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(FLOAT, USHORT, float, unsigned short, 0, USHRT_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(FLOAT, INT, float, int, INT_MIN, INT_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(FLOAT, UINT, float, unsigned int, 0, UINT_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(FLOAT, LONG, float, long, LONG_MIN, LONG_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(FLOAT, ULONG, float, unsigned long, 0, ULONG_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(FLOAT, LLONG, float, long long, LLONG_MIN, LLONG_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(FLOAT, ULLONG, float, unsigned long long, 0, ULLONG_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("pedantic")
    H5T_CONV_Ff(FLOAT, FLOAT16, float, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_GCC_CLANG_DIAG_ON("pedantic")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(DOUBLE, SCHAR, double, signed char, SCHAR_MIN, SCHAR_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(DOUBLE, UCHAR, double, unsigned char, 0, UCHAR_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(DOUBLE, SHORT, double, short, SHRT_MIN, SHRT_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(DOUBLE, USHORT, double, unsigned short, 0, USHRT_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(DOUBLE, INT, double, int, INT_MIN, INT_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(DOUBLE, UINT, double, unsigned int, 0, UINT_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(DOUBLE, LONG, double, long, LONG_MIN, LONG_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(DOUBLE, ULONG, double, unsigned long, 0, ULONG_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(DOUBLE, LLONG, double, long long, LLONG_MIN, LLONG_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(DOUBLE, ULLONG, double, unsigned long long, 0, ULLONG_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("pedantic")
    H5T_CONV_Ff(DOUBLE, FLOAT16, double, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_GCC_CLANG_DIAG_ON("pedantic")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(LDOUBLE, SCHAR, long double, signed char, SCHAR_MIN, SCHAR_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(LDOUBLE, UCHAR, long double, unsigned char, 0, UCHAR_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(LDOUBLE, SHORT, long double, short, SHRT_MIN, SHRT_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(LDOUBLE, USHORT, long double, unsigned short, 0, USHRT_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(LDOUBLE, INT, long double, int, INT_MIN, INT_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(LDOUBLE, UINT, long double, unsigned int, 0, UINT_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(LDOUBLE, LONG, long double, long, LONG_MIN, LONG_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(LDOUBLE, ULONG, long double, unsigned long, 0, ULONG_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(LDOUBLE, LLONG, long double, long long, LLONG_MIN, LLONG_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("float-equal")
    H5T_CONV_Fx(LDOUBLE, ULLONG, long double, unsigned long long, 0, ULLONG_MAX);
    H5_GCC_CLANG_DIAG_ON("float-equal")
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
    H5_GCC_CLANG_DIAG_OFF("pedantic")
    H5T_CONV_Ff(LDOUBLE, FLOAT16, long double, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_GCC_CLANG_DIAG_ON("pedantic")
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
