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
 * Purpose: Datatype conversion functions for integer datatypes
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
#include "H5Tconv_integer.h"

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_i_i
 *
 * Purpose:     Convert one integer type to another. This is the catch-all
 *              function for integer conversions and is probably not
 *              particularly fast.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_i_i(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
              size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
              void H5_ATTR_UNUSED *bkg)
{
    ssize_t        src_delta, dst_delta; /*source & destination stride    */
    int            direction;            /*direction of traversal    */
    size_t         elmtno;               /*element number        */
    size_t         half_size;            /*half the type size        */
    size_t         olap;                 /*num overlapping elements    */
    uint8_t       *s, *sp, *d, *dp;      /*source and dest traversal ptrs*/
    uint8_t       *src_rev  = NULL;      /*order-reversed source buffer  */
    uint8_t        dbuf[64] = {0};       /*temp destination buffer    */
    size_t         first;
    ssize_t        sfirst;              /*a signed version of `first'    */
    size_t         i;                   /*Local index variables         */
    H5T_conv_ret_t except_ret;          /*return of callback function   */
    bool           reverse;             /*if reverse the order of destination        */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT:
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (H5T_ORDER_LE != src->shared->u.atomic.order && H5T_ORDER_BE != src->shared->u.atomic.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unsupported byte order");
            if (H5T_ORDER_LE != dst->shared->u.atomic.order && H5T_ORDER_BE != dst->shared->u.atomic.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unsupported byte order");
            if (dst->shared->size > sizeof dbuf)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "destination size is too large");
            cdata->need_bkg = H5T_BKG_NO;
            break;

        case H5T_CONV_FREE:
            break;

        case H5T_CONV_CONV:
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (NULL == conv_ctx)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid datatype conversion context pointer");

            /*
             * Do we process the values from beginning to end or vice versa? Also,
             * how many of the elements have the source and destination areas
             * overlapping?
             */
            if (src->shared->size == dst->shared->size || buf_stride) {
                sp = dp   = (uint8_t *)buf;
                direction = 1;
                olap      = nelmts;
            }
            else if (src->shared->size >= dst->shared->size) {
                double olap_d =
                    ceil((double)(dst->shared->size) / (double)(src->shared->size - dst->shared->size));

                olap = (size_t)olap_d;
                sp = dp   = (uint8_t *)buf;
                direction = 1;
            }
            else {
                double olap_d =
                    ceil((double)(src->shared->size) / (double)(dst->shared->size - src->shared->size));
                olap      = (size_t)olap_d;
                sp        = (uint8_t *)buf + (nelmts - 1) * src->shared->size;
                dp        = (uint8_t *)buf + (nelmts - 1) * dst->shared->size;
                direction = -1;
            }

            /*
             * Direction & size of buffer traversal.
             */
            H5_CHECK_OVERFLOW(buf_stride, size_t, ssize_t);
            H5_CHECK_OVERFLOW(src->shared->size, size_t, ssize_t);
            H5_CHECK_OVERFLOW(dst->shared->size, size_t, ssize_t);
            src_delta = (ssize_t)direction * (ssize_t)(buf_stride ? buf_stride : src->shared->size);
            dst_delta = (ssize_t)direction * (ssize_t)(buf_stride ? buf_stride : dst->shared->size);

            /* Allocate space for order-reversed source buffer */
            src_rev = (uint8_t *)H5MM_calloc(src->shared->size);

            /* The conversion loop */
            for (elmtno = 0; elmtno < nelmts; elmtno++) {

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
                    assert((dp >= sp && dp < sp + src->shared->size) ||
                           (sp >= dp && sp < dp + dst->shared->size));
                }
                else {
                    assert((dp < sp && dp + dst->shared->size <= sp) ||
                           (sp < dp && sp + src->shared->size <= dp));
                }
#endif

                /*
                 * Put the data in little endian order so our loops aren't so
                 * complicated.  We'll do all the conversion stuff assuming
                 * little endian and then we'll fix the order at the end.
                 */
                if (H5T_ORDER_BE == src->shared->u.atomic.order) {
                    half_size = src->shared->size / 2;
                    for (i = 0; i < half_size; i++) {
                        uint8_t tmp                    = s[src->shared->size - (i + 1)];
                        s[src->shared->size - (i + 1)] = s[i];
                        s[i]                           = tmp;
                    }
                }

                /*
                 * What is the bit number for the msb bit of S which is set? The
                 * bit number is relative to the significant part of the number.
                 */
                sfirst = H5T__bit_find(s, src->shared->u.atomic.offset, src->shared->u.atomic.prec,
                                       H5T_BIT_MSB, true);
                first  = (size_t)sfirst;

                /* Set these variables to default */
                except_ret = H5T_CONV_UNHANDLED;
                reverse    = true;

                if (sfirst < 0) {
                    /*
                     * The source has no bits set and must therefore be zero.
                     * Set the destination to zero.
                     */
                    H5T__bit_set(d, dst->shared->u.atomic.offset, dst->shared->u.atomic.prec, false);
                }
                else if (H5T_SGN_NONE == src->shared->u.atomic.u.i.sign &&
                         H5T_SGN_NONE == dst->shared->u.atomic.u.i.sign) {
                    /*
                     * Source and destination are both unsigned, but if the
                     * source has more precision bits than the destination then
                     * it's possible to overflow.  When overflow occurs the
                     * destination will be set to the maximum possible value.
                     */
                    if (src->shared->u.atomic.prec <= dst->shared->u.atomic.prec) {
                        H5T__bit_copy(d, dst->shared->u.atomic.offset, s, src->shared->u.atomic.offset,
                                      src->shared->u.atomic.prec);
                        H5T__bit_set(d, dst->shared->u.atomic.offset + src->shared->u.atomic.prec,
                                     dst->shared->u.atomic.prec - src->shared->u.atomic.prec, false);
                    }
                    else if (first >= dst->shared->u.atomic.prec) {
                        /*overflow*/
                        if (conv_ctx->u.conv.cb_struct
                                .func) { /*If user's exception handler is present, use it*/
                            H5T__reverse_order(src_rev, s, src->shared->size,
                                               src->shared->u.atomic.order); /*reverse order first*/
                            except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                                conv_ctx->u.conv.dst_type_id, src_rev, d,
                                conv_ctx->u.conv.cb_struct.user_data);
                        }

                        if (except_ret == H5T_CONV_UNHANDLED) {
                            H5T__bit_set(d, dst->shared->u.atomic.offset, dst->shared->u.atomic.prec, true);
                        }
                        else if (except_ret == H5T_CONV_ABORT)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "can't handle conversion exception");
                        else if (except_ret == H5T_CONV_HANDLED)
                            /*Don't reverse because user handles it already*/
                            reverse = false;
                    }
                    else {
                        H5T__bit_copy(d, dst->shared->u.atomic.offset, s, src->shared->u.atomic.offset,
                                      dst->shared->u.atomic.prec);
                    }
                }
                else if (H5T_SGN_2 == src->shared->u.atomic.u.i.sign &&
                         H5T_SGN_NONE == dst->shared->u.atomic.u.i.sign) {
                    /*
                     * If the source is signed and the destination isn't then we
                     * can have overflow if the source contains more bits than
                     * the destination (destination is set to the maximum
                     * possible value) or overflow if the source is negative
                     * (destination is set to zero).
                     */
                    if (first + 1 == src->shared->u.atomic.prec) {
                        /*overflow - source is negative*/
                        if (conv_ctx->u.conv.cb_struct
                                .func) { /*If user's exception handler is present, use it*/
                            H5T__reverse_order(src_rev, s, src->shared->size,
                                               src->shared->u.atomic.order); /*reverse order first*/
                            except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                H5T_CONV_EXCEPT_RANGE_LOW, conv_ctx->u.conv.src_type_id,
                                conv_ctx->u.conv.dst_type_id, src_rev, d,
                                conv_ctx->u.conv.cb_struct.user_data);
                        }

                        if (except_ret == H5T_CONV_UNHANDLED) {
                            H5T__bit_set(d, dst->shared->u.atomic.offset, dst->shared->u.atomic.prec, false);
                        }
                        else if (except_ret == H5T_CONV_ABORT)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "can't handle conversion exception");
                        else if (except_ret == H5T_CONV_HANDLED)
                            /*Don't reverse because user handles it already*/
                            reverse = false;
                    }
                    else if (src->shared->u.atomic.prec < dst->shared->u.atomic.prec) {
                        H5T__bit_copy(d, dst->shared->u.atomic.offset, s, src->shared->u.atomic.offset,
                                      src->shared->u.atomic.prec - 1);
                        H5T__bit_set(d, dst->shared->u.atomic.offset + src->shared->u.atomic.prec - 1,
                                     (dst->shared->u.atomic.prec - src->shared->u.atomic.prec) + 1, false);
                    }
                    else if (first >= dst->shared->u.atomic.prec) {
                        /*overflow - source is positive*/
                        if (conv_ctx->u.conv.cb_struct
                                .func) { /*If user's exception handler is present, use it*/
                            H5T__reverse_order(src_rev, s, src->shared->size,
                                               src->shared->u.atomic.order); /*reverse order first*/
                            except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                                conv_ctx->u.conv.dst_type_id, src_rev, d,
                                conv_ctx->u.conv.cb_struct.user_data);
                        }

                        if (except_ret == H5T_CONV_UNHANDLED)
                            H5T__bit_set(d, dst->shared->u.atomic.offset, dst->shared->u.atomic.prec, true);
                        else if (except_ret == H5T_CONV_ABORT)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "can't handle conversion exception");
                        else if (except_ret == H5T_CONV_HANDLED)
                            /*Don't reverse because user handles it already*/
                            reverse = false;
                    }
                    else {
                        H5T__bit_copy(d, dst->shared->u.atomic.offset, s, src->shared->u.atomic.offset,
                                      dst->shared->u.atomic.prec);
                    }
                }
                else if (H5T_SGN_NONE == src->shared->u.atomic.u.i.sign &&
                         H5T_SGN_2 == dst->shared->u.atomic.u.i.sign) {
                    /*
                     * If the source is not signed but the destination is then
                     * overflow can occur in which case the destination is set to
                     * the largest possible value (all bits set except the msb).
                     */
                    if (first + 1 >= dst->shared->u.atomic.prec) {
                        /*overflow*/
                        if (conv_ctx->u.conv.cb_struct
                                .func) { /*If user's exception handler is present, use it*/
                            H5T__reverse_order(src_rev, s, src->shared->size,
                                               src->shared->u.atomic.order); /*reverse order first*/
                            except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                                conv_ctx->u.conv.dst_type_id, src_rev, d,
                                conv_ctx->u.conv.cb_struct.user_data);
                        }

                        if (except_ret == H5T_CONV_UNHANDLED) {
                            H5T__bit_set(d, dst->shared->u.atomic.offset, dst->shared->u.atomic.prec - 1,
                                         true);
                            H5T__bit_set(d, (dst->shared->u.atomic.offset + dst->shared->u.atomic.prec - 1),
                                         (size_t)1, false);
                        }
                        else if (except_ret == H5T_CONV_ABORT)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "can't handle conversion exception");
                        else if (except_ret == H5T_CONV_HANDLED)
                            /*Don't reverse because user handles it already*/
                            reverse = false;
                    }
                    else if (src->shared->u.atomic.prec < dst->shared->u.atomic.prec) {
                        H5T__bit_copy(d, dst->shared->u.atomic.offset, s, src->shared->u.atomic.offset,
                                      src->shared->u.atomic.prec);
                        H5T__bit_set(d, dst->shared->u.atomic.offset + src->shared->u.atomic.prec,
                                     dst->shared->u.atomic.prec - src->shared->u.atomic.prec, false);
                    }
                    else {
                        H5T__bit_copy(d, dst->shared->u.atomic.offset, s, src->shared->u.atomic.offset,
                                      dst->shared->u.atomic.prec);
                    }
                }
                else if (first + 1 == src->shared->u.atomic.prec) {
                    /*
                     * Both the source and the destination are signed and the
                     * source value is negative.  We could experience overflow
                     * if the destination isn't wide enough in which case the
                     * destination is set to a negative number with the largest
                     * possible magnitude.
                     */
                    ssize_t sfz = H5T__bit_find(s, src->shared->u.atomic.offset,
                                                src->shared->u.atomic.prec - 1, H5T_BIT_MSB, false);
                    size_t  fz  = (size_t)sfz;

                    if (sfz >= 0 && fz + 1 >= dst->shared->u.atomic.prec) {
                        /*overflow*/
                        if (conv_ctx->u.conv.cb_struct
                                .func) { /*If user's exception handler is present, use it*/
                            H5T__reverse_order(src_rev, s, src->shared->size,
                                               src->shared->u.atomic.order); /*reverse order first*/
                            except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                H5T_CONV_EXCEPT_RANGE_LOW, conv_ctx->u.conv.src_type_id,
                                conv_ctx->u.conv.dst_type_id, src_rev, d,
                                conv_ctx->u.conv.cb_struct.user_data);
                        }

                        if (except_ret == H5T_CONV_UNHANDLED) {
                            H5T__bit_set(d, dst->shared->u.atomic.offset, dst->shared->u.atomic.prec - 1,
                                         false);
                            H5T__bit_set(d, (dst->shared->u.atomic.offset + dst->shared->u.atomic.prec - 1),
                                         (size_t)1, true);
                        }
                        else if (except_ret == H5T_CONV_ABORT)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "can't handle conversion exception");
                        else if (except_ret == H5T_CONV_HANDLED)
                            /*Don't reverse because user handles it already*/
                            reverse = false;
                    }
                    else if (src->shared->u.atomic.prec < dst->shared->u.atomic.prec) {
                        H5T__bit_copy(d, dst->shared->u.atomic.offset, s, src->shared->u.atomic.offset,
                                      src->shared->u.atomic.prec);
                        H5T__bit_set(d, dst->shared->u.atomic.offset + src->shared->u.atomic.prec,
                                     dst->shared->u.atomic.prec - src->shared->u.atomic.prec, true);
                    }
                    else {
                        H5T__bit_copy(d, dst->shared->u.atomic.offset, s, src->shared->u.atomic.offset,
                                      dst->shared->u.atomic.prec);
                    }
                }
                else {
                    /*
                     * Source and destination are both signed but the source
                     * value is positive.  We could have an overflow in which
                     * case the destination is set to the largest possible
                     * positive value.
                     */
                    if (first + 1 >= dst->shared->u.atomic.prec) {
                        /*overflow*/
                        if (conv_ctx->u.conv.cb_struct
                                .func) { /*If user's exception handler is present, use it*/
                            H5T__reverse_order(src_rev, s, src->shared->size,
                                               src->shared->u.atomic.order); /*reverse order first*/
                            except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                                conv_ctx->u.conv.dst_type_id, src_rev, d,
                                conv_ctx->u.conv.cb_struct.user_data);
                        }

                        if (except_ret == H5T_CONV_UNHANDLED) {
                            H5T__bit_set(d, dst->shared->u.atomic.offset, dst->shared->u.atomic.prec - 1,
                                         true);
                            H5T__bit_set(d, (dst->shared->u.atomic.offset + dst->shared->u.atomic.prec - 1),
                                         (size_t)1, false);
                        }
                        else if (except_ret == H5T_CONV_ABORT)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "can't handle conversion exception");
                        else if (except_ret == H5T_CONV_HANDLED)
                            /*Don't reverse because user handles it already*/
                            reverse = false;
                    }
                    else if (src->shared->u.atomic.prec < dst->shared->u.atomic.prec) {
                        H5T__bit_copy(d, dst->shared->u.atomic.offset, s, src->shared->u.atomic.offset,
                                      src->shared->u.atomic.prec);
                        H5T__bit_set(d, dst->shared->u.atomic.offset + src->shared->u.atomic.prec,
                                     dst->shared->u.atomic.prec - src->shared->u.atomic.prec, false);
                    }
                    else {
                        H5T__bit_copy(d, dst->shared->u.atomic.offset, s, src->shared->u.atomic.offset,
                                      dst->shared->u.atomic.prec);
                    }
                }

                /*
                 * Set padding areas in destination.
                 */
                if (dst->shared->u.atomic.offset > 0) {
                    assert(H5T_PAD_ZERO == dst->shared->u.atomic.lsb_pad ||
                           H5T_PAD_ONE == dst->shared->u.atomic.lsb_pad);
                    H5T__bit_set(d, (size_t)0, dst->shared->u.atomic.offset,
                                 (bool)(H5T_PAD_ONE == dst->shared->u.atomic.lsb_pad));
                }
                if (dst->shared->u.atomic.offset + dst->shared->u.atomic.prec != 8 * dst->shared->size) {
                    assert(H5T_PAD_ZERO == dst->shared->u.atomic.msb_pad ||
                           H5T_PAD_ONE == dst->shared->u.atomic.msb_pad);
                    H5T__bit_set(d, dst->shared->u.atomic.offset + dst->shared->u.atomic.prec,
                                 8 * dst->shared->size -
                                     (dst->shared->u.atomic.offset + dst->shared->u.atomic.prec),
                                 (bool)(H5T_PAD_ONE == dst->shared->u.atomic.msb_pad));
                }

                /*
                 * Put the destination in the correct byte order.  See note at
                 * beginning of loop.
                 */
                if (H5T_ORDER_BE == dst->shared->u.atomic.order && reverse) {
                    half_size = dst->shared->size / 2;
                    for (i = 0; i < half_size; i++) {
                        uint8_t tmp                    = d[dst->shared->size - (i + 1)];
                        d[dst->shared->size - (i + 1)] = d[i];
                        d[i]                           = tmp;
                    }
                }

                /*
                 * If we had used a temporary buffer for the destination then we
                 * should copy the value to the true destination buffer.
                 */
                if (d == dbuf)
                    H5MM_memcpy(dp, d, dst->shared->size);

                /* Advance source & destination pointers by delta amounts */
                sp += src_delta;
                dp += dst_delta;
            } /* end for */

            break;

        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    } /* end switch */

done:
    if (src_rev)
        H5MM_free(src_rev);
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_i_i() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_i_f
 *
 * Purpose:     Convert one integer type to a floating-point type. This is
 *              the catch-all function for integer-float conversions and
 *              is probably not particularly fast.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_i_f(const H5T_t *src_p, const H5T_t *dst_p, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
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
    hsize_t        expo;                /*destination exponent        */
    hsize_t        expo_max;            /*maximal possible exponent value       */
    size_t         sign;                /*source sign bit value         */
    bool           is_max_neg;          /*source is maximal negative value*/
    bool           do_round;            /*whether there is roundup      */
    uint8_t       *int_buf = NULL;      /*buffer for temporary value    */
    size_t         buf_size;            /*buffer size for temporary value */
    size_t         i;                   /*miscellaneous counters    */
    size_t         first;               /*first bit(MSB) in an integer  */
    ssize_t        sfirst;              /*a signed version of `first'    */
    H5T_conv_ret_t except_ret;          /*return of callback function   */
    bool           reverse;             /*if reverse the order of destination   */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT:
            if (NULL == src_p || NULL == dst_p)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            src = src_p->shared->u.atomic;
            dst = dst_p->shared->u.atomic;
            if (H5T_ORDER_LE != dst.order && H5T_ORDER_BE != dst.order && H5T_ORDER_VAX != dst.order)
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
            buf_size = ((src.prec > dst.u.f.msize ? src.prec : dst.u.f.msize) + 7) / 8;
            int_buf  = (uint8_t *)H5MM_calloc(buf_size);

            /* Allocate space for order-reversed source buffer */
            src_rev = (uint8_t *)H5MM_calloc(src_p->shared->size);

            /* The conversion loop */
            for (elmtno = 0; elmtno < nelmts; elmtno++) {
                /* Set these variables to default */
                except_ret = H5T_CONV_UNHANDLED;
                reverse    = true;

                /* Make sure these variables are reset to 0. */
                sign       = 0; /*source sign bit value         */
                is_max_neg = 0; /*source is maximal negative value*/
                do_round   = 0; /*whether there is roundup      */
                sfirst     = 0;

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

                /* Put the data in little endian order so our loops aren't so
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

                /* Zero-set all destination bits*/
                H5T__bit_set(d, dst.offset, dst.prec, false);

                /* Copy source into a temporary buffer */
                H5T__bit_copy(int_buf, (size_t)0, s, src.offset, src.prec);

                /* Find the sign bit value of the source */
                if (H5T_SGN_2 == src.u.i.sign)
                    sign = (size_t)H5T__bit_get_d(int_buf, src.prec - 1, (size_t)1);

                /* What is the bit position(starting from 0 as first one) for the most significant
                 * bit(MSB) of S which is set?
                 */
                if (H5T_SGN_2 == src.u.i.sign) {
                    sfirst = H5T__bit_find(int_buf, (size_t)0, src.prec - 1, H5T_BIT_MSB, true);
                    if (sign && sfirst < 0)
                        /* The case 0x80...00, which is negative with maximal value */
                        is_max_neg = 1;
                }
                else if (H5T_SGN_NONE == src.u.i.sign)
                    sfirst = H5T__bit_find(int_buf, (size_t)0, src.prec, H5T_BIT_MSB, true);

                /* Handle special cases here.  Integer is zero */
                if (!sign && sfirst < 0)
                    goto padding;

                /* Convert source integer if it's negative */
                if (H5T_SGN_2 == src.u.i.sign && sign) {
                    if (!is_max_neg) {
                        /* Equivalent to ~(i - 1) */
                        H5T__bit_dec(int_buf, (size_t)0, buf_size * 8);
                        H5T__bit_neg(int_buf, (size_t)0, buf_size * 8);
                        sfirst = H5T__bit_find(int_buf, (size_t)0, src.prec - 1, H5T_BIT_MSB, true);
                    }
                    else {
                        /* If it's maximal negative number 0x80...000, treat it as if it overflowed
                         * (create a carry) to help conversion.  i.e. a character type number 0x80
                         * is treated as 0x100.
                         */
                        sfirst     = (ssize_t)(src.prec - 1);
                        is_max_neg = 0;
                    }
                    if (sfirst < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "zero bit not found");

                    /* Sign bit has been negated if bit vector isn't 0x80...00.  Set all bits in front of
                     * sign bit to 0 in the temporary buffer because they're all negated from the previous
                     * step.
                     */
                    H5T__bit_set(int_buf, src.prec, (buf_size * 8) - src.prec, 0);

                    /* Set sign bit in destination */
                    H5T__bit_set_d(d, dst.u.f.sign, (size_t)1, (hsize_t)sign);
                } /* end if */

                first = (size_t)sfirst;

                /* Calculate the true destination exponent by adjusting according to
                 * the destination exponent bias.  Implied and non-implied normalization
                 * should be the same.
                 */
                if (H5T_NORM_NONE == dst.u.f.norm || H5T_NORM_IMPLIED == dst.u.f.norm) {
                    expo = first + dst.u.f.ebias;
                }
                else {
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                "normalization method not implemented yet");
                }

                /* Handle mantissa part here */
                if (H5T_NORM_IMPLIED == dst.u.f.norm) {
                    /* Imply first bit */
                    H5T__bit_set(int_buf, first, (size_t)1, 0);
                }
                else if (H5T_NORM_NONE == dst.u.f.norm) {
                    first++;
                }

                /* Roundup for mantissa */
                if (first > dst.u.f.msize) {
                    /* If the bit sequence is bigger than the mantissa part, there'll be some
                     * precision loss.  Let user's handler deal with the case if it's present
                     */
                    if (conv_ctx->u.conv.cb_struct.func) {
                        H5T__reverse_order(src_rev, s, src_p->shared->size,
                                           src_p->shared->u.atomic.order); /*reverse order first*/
                        except_ret = (conv_ctx->u.conv.cb_struct.func)(
                            H5T_CONV_EXCEPT_PRECISION, conv_ctx->u.conv.src_type_id,
                            conv_ctx->u.conv.dst_type_id, src_rev, d, conv_ctx->u.conv.cb_struct.user_data);
                    }

                    if (except_ret == H5T_CONV_HANDLED) {
                        reverse = false;
                        goto padding;
                    }
                    else if (except_ret == H5T_CONV_ABORT)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");

                    /* If user's exception handler does deal with it, we do it by dropping off the
                     * extra bits at the end and do rounding.  If we have .50...0(decimal) after radix
                     * point, we do roundup when the least significant digit before radix is odd, we do
                     * rounddown if it's even.
                     */

                    /* Check 1st dropoff bit, see if it's set. */
                    if (H5T__bit_get_d(int_buf, ((first - dst.u.f.msize) - 1), (size_t)1)) {
                        /* Check all bits after 1st dropoff bit, see if any of them is set. */
                        if (((first - dst.u.f.msize) - 1) > 0 &&
                            H5T__bit_get_d(int_buf, (size_t)0, ((first - dst.u.f.msize) - 1)))
                            do_round = 1;
                        else { /* The .50...0 case */
                            /* Check if the least significant bit is odd. */
                            if (H5T__bit_get_d(int_buf, (first - dst.u.f.msize), (size_t)1))
                                do_round = 1;
                        }
                    }

                    /* Right shift to drop off extra bits */
                    H5T__bit_shift(int_buf, (ssize_t)(dst.u.f.msize - first), (size_t)0, buf_size * 8);

                    if (do_round) {
                        H5T__bit_inc(int_buf, (size_t)0, buf_size * 8);
                        do_round = 0;

                        /* If integer is like 0x0ff...fff and we need to round up the
                         * last f, we get 0x100...000.  Treat this special case here.
                         */
                        if (H5T__bit_get_d(int_buf, dst.u.f.msize, (size_t)1)) {
                            if (H5T_NORM_IMPLIED == dst.u.f.norm) {
                                /* The bit at this 1's position was impled already, so this
                                 * number should be 0x200...000.  We need to increment the
                                 * exponent in this case.
                                 */
                                expo++;
                            }
                            else if (H5T_NORM_NONE == dst.u.f.norm) {
                                /* Right shift 1 bit to let the carried 1 fit in the mantissa,
                                 * and increment exponent by 1.
                                 */
                                H5T__bit_shift(int_buf, (ssize_t)-1, (size_t)0, buf_size * 8);
                                expo++;
                            }
                        }
                    }
                }
                else {
                    /* The bit sequence can fit mantissa part.  Left shift to fit in from high-order of
                     * bit position. */
                    H5T__bit_shift(int_buf, (ssize_t)(dst.u.f.msize - first), (size_t)0, dst.u.f.msize);
                }

                /* Check if the exponent is too big */
                expo_max = (hsize_t)(pow(2.0, (double)dst.u.f.esize) - 1);

                if (expo > expo_max) { /*overflows*/
                    if (conv_ctx->u.conv.cb_struct
                            .func) { /*user's exception handler.  Reverse back source order*/
                        H5T__reverse_order(src_rev, s, src_p->shared->size,
                                           src_p->shared->u.atomic.order); /*reverse order first*/
                        except_ret = (conv_ctx->u.conv.cb_struct.func)(
                            H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                            conv_ctx->u.conv.dst_type_id, src_rev, d, conv_ctx->u.conv.cb_struct.user_data);

                        if (except_ret == H5T_CONV_ABORT)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "can't handle conversion exception");
                        else if (except_ret == H5T_CONV_HANDLED) {
                            reverse = false;
                            goto padding;
                        }
                    }

                    if (!conv_ctx->u.conv.cb_struct.func || (except_ret == H5T_CONV_UNHANDLED)) {
                        /*make destination infinity by setting exponent to maximal number and
                         *mantissa to zero.*/
                        expo = expo_max;
                        memset(int_buf, 0, buf_size);
                    }
                }

                if (except_ret == H5T_CONV_UNHANDLED) {
                    /* Set exponent in destination */
                    H5T__bit_set_d(d, dst.u.f.epos, dst.u.f.esize, expo);

                    /* Copy mantissa into destination */
                    H5T__bit_copy(d, dst.u.f.mpos, int_buf, (size_t)0,
                                  (buf_size * 8) > dst.u.f.msize ? dst.u.f.msize : buf_size * 8);
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
} /* end H5T__conv_i_f() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_schar_uchar
 *
 * Purpose:     Converts `signed char' to `unsigned char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_schar_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_su(SCHAR, UCHAR, signed char, unsigned char, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_schar_short
 *
 * Purpose:     Converts `signed char' to `short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_schar_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sS(SCHAR, SHORT, signed char, short, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_schar_ushort
 *
 * Purpose:     Converts `signed char' to `unsigned short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_schar_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sU(SCHAR, USHORT, signed char, unsigned short, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_schar_int
 *
 * Purpose:     Converts `signed char' to `int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_schar_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                    size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                    void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sS(SCHAR, INT, signed char, int, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_schar_uint
 *
 * Purpose:     Converts `signed char' to `unsigned int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_schar_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sU(SCHAR, UINT, signed char, unsigned, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_schar_long
 *
 * Purpose:     Converts `signed char' to `long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_schar_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sS(SCHAR, LONG, signed char, long, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_schar_ulong
 *
 * Purpose:     Converts `signed char' to `unsigned long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_schar_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sU(SCHAR, ULONG, signed char, unsigned long, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_schar_llong
 *
 * Purpose:     Converts `signed char' to `long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_schar_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sS(SCHAR, LLONG, signed char, long long, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_schar_ullong
 *
 * Purpose:     Converts `signed char' to `unsigned long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_schar_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sU(SCHAR, ULLONG, signed char, unsigned long long, -, -);
}

#ifdef H5_HAVE__FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_schar__Float16
 *
 * Purpose:     Converts `signed char` to `_Float16`
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_schar__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(SCHAR, FLOAT16, signed char, H5__Float16, -, -);
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_schar_float
 *
 * Purpose:     Convert native signed char to native float using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_schar_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(SCHAR, FLOAT, signed char, float, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_schar_double
 *
 * Purpose:     Convert native signed char to native double using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_schar_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(SCHAR, DOUBLE, signed char, double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_schar_ldouble
 *
 * Purpose:     Convert native signed char to native long double using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_schar_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(SCHAR, LDOUBLE, signed char, long double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uchar_schar
 *
 * Purpose:     Converts `unsigned char' to `signed char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uchar_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_us(UCHAR, SCHAR, unsigned char, signed char, -, SCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uchar_short
 *
 * Purpose:     Converts `unsigned char' to `short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uchar_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uS(UCHAR, SHORT, unsigned char, short, -, SHRT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uchar_ushort
 *
 * Purpose:     Converts `unsigned char' to `unsigned short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uchar_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uU(UCHAR, USHORT, unsigned char, unsigned short, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uchar_int
 *
 * Purpose:     Converts `unsigned char' to `int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uchar_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                    size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                    void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uS(UCHAR, INT, unsigned char, int, -, INT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uchar_uint
 *
 * Purpose:     Converts `unsigned char' to `unsigned int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uchar_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uU(UCHAR, UINT, unsigned char, unsigned, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uchar_long
 *
 * Purpose:     Converts `unsigned char' to `long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uchar_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uS(UCHAR, LONG, unsigned char, long, -, LONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uchar_ulong
 *
 * Purpose:     Converts `unsigned char' to `unsigned long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uchar_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uU(UCHAR, ULONG, unsigned char, unsigned long, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uchar_llong
 *
 * Purpose:     Converts `unsigned char' to `long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uchar_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uS(UCHAR, LLONG, unsigned char, long long, -, LLONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uchar_ullong
 *
 * Purpose:     Converts `unsigned char' to `unsigned long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uchar_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uU(UCHAR, ULLONG, unsigned char, unsigned long long, -, -);
}

#ifdef H5_HAVE__FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uchar__Float16
 *
 * Purpose:     Converts `unsigned char` to `_Float16`
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uchar__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(UCHAR, FLOAT16, unsigned char, H5__Float16, -, -);
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uchar_float
 *
 * Purpose:     Convert native unsigned char to native float using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uchar_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(UCHAR, FLOAT, unsigned char, float, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uchar_double
 *
 * Purpose:     Convert native unsigned char to native double using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uchar_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(UCHAR, DOUBLE, unsigned char, double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uchar_ldouble
 *
 * Purpose:     Convert native unsigned char to native long double using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uchar_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(UCHAR, LDOUBLE, unsigned char, long double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_short_schar
 *
 * Purpose:     Converts `short' to `signed char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_short_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Ss(SHORT, SCHAR, short, signed char, SCHAR_MIN, SCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_short_uchar
 *
 * Purpose:     Converts `short' to `unsigned char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_short_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Su(SHORT, UCHAR, short, unsigned char, -, UCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_short_ushort
 *
 * Purpose:     Converts `short' to `unsigned short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_short_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_su(SHORT, USHORT, short, unsigned short, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_short_int
 *
 * Purpose:     Converts `short' to `int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_short_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                    size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                    void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sS(SHORT, INT, short, int, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_short_uint
 *
 * Purpose:     Converts `short' to `unsigned int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_short_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sU(SHORT, UINT, short, unsigned, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_short_long
 *
 * Purpose:     Converts `short' to `long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_short_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sS(SHORT, LONG, short, long, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_short_ulong
 *
 * Purpose:     Converts `short' to `unsigned long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_short_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sU(SHORT, ULONG, short, unsigned long, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_short_llong
 *
 * Purpose:     Converts `short' to `long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_short_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sS(SHORT, LLONG, short, long long, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_short_ullong
 *
 * Purpose:     Converts `short' to `unsigned long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_short_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sU(SHORT, ULLONG, short, unsigned long long, -, -);
}

#ifdef H5_HAVE__FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_short__Float16
 *
 * Purpose:     Converts `short' to `_Float16'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_short__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(SHORT, FLOAT16, short, H5__Float16, -, -);
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_short_float
 *
 * Purpose:     Convert native short to native float using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_short_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(SHORT, FLOAT, short, float, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_short_double
 *
 * Purpose:     Convert native short to native double using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_short_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(SHORT, DOUBLE, short, double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_short_ldouble
 *
 * Purpose:     Convert native short to native long double using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_short_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(SHORT, LDOUBLE, short, long double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ushort_schar
 *
 * Purpose:     Converts `unsigned short' to `signed char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ushort_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Us(USHORT, SCHAR, unsigned short, signed char, -, SCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ushort_uchar
 *
 * Purpose:     Converts `unsigned short' to `unsigned char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ushort_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Uu(USHORT, UCHAR, unsigned short, unsigned char, -, UCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ushort_short
 *
 * Purpose:     Converts `unsigned short' to `short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ushort_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_us(USHORT, SHORT, unsigned short, short, -, SHRT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ushort_int
 *
 * Purpose:     Converts `unsigned short' to `int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ushort_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uS(USHORT, INT, unsigned short, int, -, INT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ushort_uint
 *
 * Purpose:     Converts `unsigned short' to `unsigned int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ushort_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uU(USHORT, UINT, unsigned short, unsigned, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ushort_long
 *
 * Purpose:     Converts `unsigned short' to `long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ushort_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uS(USHORT, LONG, unsigned short, long, -, LONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ushort_ulong
 *
 * Purpose:     Converts `unsigned short' to `unsigned long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ushort_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uU(USHORT, ULONG, unsigned short, unsigned long, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ushort_llong
 *
 * Purpose:     Converts `unsigned short' to `long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ushort_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uS(USHORT, LLONG, unsigned short, long long, -, LLONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ushort_ullong
 *
 * Purpose:     Converts `unsigned short' to `unsigned long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ushort_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uU(USHORT, ULLONG, unsigned short, unsigned long long, -, -);
}

#ifdef H5_HAVE__FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ushort__Float16
 *
 * Purpose:     Converts `unsigned short' to `_Float16'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ushort__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    /* Suppress warning about non-standard floating-point literal suffix */
    H5_GCC_CLANG_DIAG_OFF("pedantic")
    H5T_CONV_Xf(USHORT, FLOAT16, unsigned short, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_GCC_CLANG_DIAG_ON("pedantic")
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ushort_float
 *
 * Purpose:     Convert native unsigned short to native float using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ushort_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(USHORT, FLOAT, unsigned short, float, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ushort_double
 *
 * Purpose:     Convert native unsigned short to native double using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ushort_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(USHORT, DOUBLE, unsigned short, double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ushort_ldouble
 *
 * Purpose:     Convert native unsigned short to native long double using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ushort_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(USHORT, LDOUBLE, unsigned short, long double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_int_schar
 *
 * Purpose:     Converts `int' to `signed char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_int_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                    size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                    void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Ss(INT, SCHAR, int, signed char, SCHAR_MIN, SCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_int_uchar
 *
 * Purpose:     Converts `int' to `unsigned char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_int_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                    size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                    void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Su(INT, UCHAR, int, unsigned char, -, UCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_int_short
 *
 * Purpose:     Converts `int' to `short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_int_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                    size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                    void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Ss(INT, SHORT, int, short, SHRT_MIN, SHRT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_int_ushort
 *
 * Purpose:     Converts `int' to `unsigned short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_int_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Su(INT, USHORT, int, unsigned short, -, USHRT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_int_uint
 *
 * Purpose:     Converts `int' to `unsigned int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_int_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                   size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                   void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_su(INT, UINT, int, unsigned, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_int_long
 *
 * Purpose:     Converts `int' to `long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_int_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                   size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                   void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sS(INT, LONG, int, long, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_int_ulong
 *
 * Purpose:     Converts `int' to `unsigned long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_int_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                    size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                    void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sU(INT, LONG, int, unsigned long, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_int_llong
 *
 * Purpose:     Converts `int' to `long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_int_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                    size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                    void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sS(INT, LLONG, int, long long, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_int_ullong
 *
 * Purpose:     Converts `int' to `unsigned long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_int_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sU(INT, ULLONG, int, unsigned long long, -, -);
}

#ifdef H5_HAVE__FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_int__Float16
 *
 * Purpose:     Converts `int' to `_Float16'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_int__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    /* Suppress warning about non-standard floating-point literal suffix */
    H5_GCC_CLANG_DIAG_OFF("pedantic")
    H5T_CONV_Xf(INT, FLOAT16, int, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_GCC_CLANG_DIAG_ON("pedantic")
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_int_float
 *
 * Purpose:     Convert native integer to native float using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_int_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                    size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                    void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(INT, FLOAT, int, float, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_int_double
 *
 * Purpose:     Convert native integer to native double using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_int_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(INT, DOUBLE, int, double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_int_ldouble
 *
 * Purpose:     Convert native integer to native long double using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_int_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(INT, LDOUBLE, int, long double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uint_schar
 *
 * Purpose:     Converts `unsigned int' to `signed char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uint_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Us(UINT, SCHAR, unsigned, signed char, -, SCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uint_uchar
 *
 * Purpose:     Converts `unsigned int' to `unsigned char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uint_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Uu(UINT, UCHAR, unsigned, unsigned char, -, UCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uint_short
 *
 * Purpose:     Converts `unsigned int' to `short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uint_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Us(UINT, SHORT, unsigned, short, -, SHRT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uint_ushort
 *
 * Purpose:     Converts `unsigned int' to `unsigned short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uint_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Uu(UINT, USHORT, unsigned, unsigned short, -, USHRT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uint_int
 *
 * Purpose:     Converts `unsigned int' to `int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uint_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                   size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                   void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_us(UINT, INT, unsigned, int, -, INT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uint_long
 *
 * Purpose:     Converts `unsigned int' to `long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uint_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                    size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                    void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uS(UINT, LONG, unsigned, long, -, LONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uint_ulong
 *
 * Purpose:     Converts `unsigned int' to `unsigned long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uint_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uU(UINT, ULONG, unsigned, unsigned long, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uint_llong
 *
 * Purpose:     Converts `unsigned int' to `long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uint_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uS(UINT, LLONG, unsigned, long long, -, LLONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uint_ullong
 *
 * Purpose:     Converts `unsigned int' to `unsigned long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uint_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uU(UINT, ULLONG, unsigned, unsigned long long, -, -);
}

#ifdef H5_HAVE__FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uint__Float16
 *
 * Purpose:     Converts `unsigned int' to `_Float16'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uint__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    /* Suppress warning about non-standard floating-point literal suffix */
    H5_GCC_CLANG_DIAG_OFF("pedantic")
    H5T_CONV_Xf(UINT, FLOAT16, unsigned int, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_GCC_CLANG_DIAG_ON("pedantic")
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uint_float
 *
 * Purpose:     Convert native unsigned integer to native float using
 *              hardware.  This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uint_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(UINT, FLOAT, unsigned int, float, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uint_double
 *
 * Purpose:     Convert native unsigned integer to native double using
 *              hardware.  This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uint_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(UINT, DOUBLE, unsigned int, double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_uint_ldouble
 *
 * Purpose:     Convert native unsigned integer to native long double using
 *              hardware.  This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_uint_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(UINT, LDOUBLE, unsigned int, long double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_long_schar
 *
 * Purpose:     Converts `long' to `signed char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_long_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Ss(LONG, SCHAR, long, signed char, SCHAR_MIN, SCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_long_uchar
 *
 * Purpose:     Converts `long' to `unsigned char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_long_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Su(LONG, UCHAR, long, unsigned char, -, UCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_long_short
 *
 * Purpose:     Converts `long' to `short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_long_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Ss(LONG, SHORT, long, short, SHRT_MIN, SHRT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_long_ushort
 *
 * Purpose:     Converts `long' to `unsigned short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_long_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Su(LONG, USHORT, long, unsigned short, -, USHRT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_long_int
 *
 * Purpose:     Converts `long' to `int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_long_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                   size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                   void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Ss(LONG, INT, long, int, INT_MIN, INT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_long_uint
 *
 * Purpose:     Converts `long' to `unsigned int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_long_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                    size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                    void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Su(LONG, UINT, long, unsigned, -, UINT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_long_ulong
 *
 * Purpose:     Converts `long' to `unsigned long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_long_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_su(LONG, ULONG, long, unsigned long, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_long_llong
 *
 * Purpose:     Converts `long' to `long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_long_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sS(LONG, LLONG, long, long long, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_long_ullong
 *
 * Purpose:     Converts `long' to `unsigned long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_long_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_sU(LONG, ULLONG, long, unsigned long long, -, -);
}

#ifdef H5_HAVE__FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_long__Float16
 *
 * Purpose:     Converts `long' to `_Float16'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_long__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    /* Suppress warning about non-standard floating-point literal suffix */
    H5_GCC_CLANG_DIAG_OFF("pedantic")
    H5T_CONV_Xf(LONG, FLOAT16, long, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_GCC_CLANG_DIAG_ON("pedantic")
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_long_float
 *
 * Purpose:     Convert native long to native float using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_long_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(LONG, FLOAT, long, float, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_long_double
 *
 * Purpose:     Convert native long to native double using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_long_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(LONG, DOUBLE, long, double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_long_ldouble
 *
 * Purpose:     Convert native long to native long double using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_long_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(LONG, LDOUBLE, long, long double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ulong_schar
 *
 * Purpose:     Converts `unsigned long' to `signed char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ulong_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Us(ULONG, SCHAR, unsigned long, signed char, -, SCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ulong_uchar
 *
 * Purpose:     Converts `unsigned long' to `unsigned char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ulong_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Uu(ULONG, UCHAR, unsigned long, unsigned char, -, UCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ulong_short
 *
 * Purpose:     Converts `unsigned long' to `short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ulong_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Us(ULONG, SHORT, unsigned long, short, -, SHRT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ulong_ushort
 *
 * Purpose:     Converts `unsigned long' to `unsigned short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ulong_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Uu(ULONG, USHORT, unsigned long, unsigned short, -, USHRT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ulong_int
 *
 * Purpose:     Converts `unsigned long' to `int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ulong_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                    size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                    void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Us(ULONG, INT, unsigned long, int, -, INT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ulong_uint
 *
 * Purpose:     Converts `unsigned long' to `unsigned int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ulong_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Uu(ULONG, UINT, unsigned long, unsigned, -, UINT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ulong_long
 *
 * Purpose:     Converts `unsigned long' to `long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ulong_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_us(ULONG, LONG, unsigned long, long, -, LONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ulong_llong
 *
 * Purpose:     Converts `unsigned long' to `long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ulong_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uS(ULONG, LLONG, unsigned long, long long, -, LLONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ulong_ullong
 *
 * Purpose:     Converts `unsigned long' to `unsigned long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ulong_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_uU(ULONG, ULLONG, unsigned long, unsigned long long, -, -);
}

#ifdef H5_HAVE__FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ulong__Float16
 *
 * Purpose:     Converts `unsigned long' to `_Float16'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ulong__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    /* Suppress warning about non-standard floating-point literal suffix */
    H5_GCC_CLANG_DIAG_OFF("pedantic")
    H5T_CONV_Xf(ULONG, FLOAT16, unsigned long, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_GCC_CLANG_DIAG_ON("pedantic")
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ulong_float
 *
 * Purpose:     Convert native unsigned long to native float using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ulong_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(ULONG, FLOAT, unsigned long, float, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ulong_double
 *
 * Purpose:     Convert native unsigned long to native double using
 *              hardware. This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ulong_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(ULONG, DOUBLE, unsigned long, double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ulong_ldouble
 *
 * Purpose:     Convert native unsigned long to native long double using
 *              hardware.  This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ulong_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(ULONG, LDOUBLE, unsigned long, long double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_llong_schar
 *
 * Purpose:     Converts `long long' to `signed char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_llong_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Ss(LLONG, SCHAR, long long, signed char, SCHAR_MIN, SCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_llong_uchar
 *
 * Purpose:     Converts `long long' to `unsigned char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_llong_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Su(LLONG, UCHAR, long long, unsigned char, -, UCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_llong_short
 *
 * Purpose:     Converts `long long' to `short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_llong_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Ss(LLONG, SHORT, long long, short, SHRT_MIN, SHRT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_llong_ushort
 *
 * Purpose:     Converts `long long' to `unsigned short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_llong_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Su(LLONG, USHORT, long long, unsigned short, -, USHRT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_llong_int
 *
 * Purpose:     Converts `long long' to `int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_llong_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                    size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                    void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Ss(LLONG, INT, long long, int, INT_MIN, INT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_llong_uint
 *
 * Purpose:     Converts `long long' to `unsigned int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_llong_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Su(LLONG, UINT, long long, unsigned, -, UINT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_llong_long
 *
 * Purpose:     Converts `long long' to `long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_llong_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Ss(LLONG, LONG, long long, long, LONG_MIN, LONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_llong_ulong
 *
 * Purpose:     Converts `long long' to `unsigned long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_llong_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Su(LLONG, ULONG, long long, unsigned long, -, ULONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_llong_ullong
 *
 * Purpose:     Converts `long long' to `unsigned long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_llong_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_su(LLONG, ULLONG, long long, unsigned long long, -, -);
}

#ifdef H5_HAVE__FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_llong__Float16
 *
 * Purpose:     Converts `long long' to `_Float16'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_llong__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    /* Suppress warning about non-standard floating-point literal suffix */
    H5_GCC_CLANG_DIAG_OFF("pedantic")
    H5T_CONV_Xf(LLONG, FLOAT16, long long, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_GCC_CLANG_DIAG_ON("pedantic")
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_llong_float
 *
 * Purpose:     Convert native long long to native float using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_llong_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(LLONG, FLOAT, long long, float, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_llong_double
 *
 * Purpose:     Convert native long long to native double using hardware.
 *              This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_llong_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(LLONG, DOUBLE, long long, double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_llong_ldouble
 *
 * Purpose:     Convert native long long to native long double using
 *              hardware.  This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5T_CONV_INTERNAL_LLONG_LDOUBLE
herr_t
H5T__conv_llong_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(LLONG, LDOUBLE, long long, long double, -, -);
}
#endif /* H5T_CONV_INTERNAL_LLONG_LDOUBLE */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ullong_schar
 *
 * Purpose:     Converts `unsigned long long' to `signed char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ullong_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Us(ULLONG, SCHAR, unsigned long long, signed char, -, SCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ullong_uchar
 *
 * Purpose:     Converts `unsigned long long' to `unsigned char'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ullong_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Uu(ULLONG, UCHAR, unsigned long long, unsigned char, -, UCHAR_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ullong_short
 *
 * Purpose:     Converts `unsigned long long' to `short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ullong_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Us(ULLONG, SHORT, unsigned long long, short, -, SHRT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ullong_ushort
 *
 * Purpose:     Converts `unsigned long long' to `unsigned short'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ullong_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Uu(ULLONG, USHORT, unsigned long long, unsigned short, -, USHRT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ullong_int
 *
 * Purpose:     Converts `unsigned long long' to `int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ullong_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                     void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Us(ULLONG, INT, unsigned long long, int, -, INT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ullong_uint
 *
 * Purpose:     Converts `unsigned long long' to `unsigned int'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ullong_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Uu(ULLONG, UINT, unsigned long long, unsigned, -, UINT_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ullong_long
 *
 * Purpose:     Converts `unsigned long long' to `long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ullong_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                      size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                      void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Us(ULLONG, LONG, unsigned long long, long, -, LONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ullong_ulong
 *
 * Purpose:     Converts `unsigned long long' to `unsigned long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ullong_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_Uu(ULLONG, ULONG, unsigned long long, unsigned long, -, ULONG_MAX);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ullong_llong
 *
 * Purpose:     Converts `unsigned long long' to `long long'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ullong_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_us(ULLONG, LLONG, unsigned long long, long long, -, LLONG_MAX);
}

#ifdef H5_HAVE__FLOAT16
/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ullong__Float16
 *
 * Purpose:     Converts `unsigned long long' to `_Float16'
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ullong__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                          size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    /* Suppress warning about non-standard floating-point literal suffix */
    H5_GCC_CLANG_DIAG_OFF("pedantic")
    H5T_CONV_Xf(ULLONG, FLOAT16, unsigned long long, H5__Float16, -FLT16_MAX, FLT16_MAX);
    H5_GCC_CLANG_DIAG_ON("pedantic")
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ullong_float
 *
 * Purpose:     Convert native unsigned long long to native float using
 *              hardware.  This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ullong_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                       size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(ULLONG, FLOAT, unsigned long long, float, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ullong_double
 *
 * Purpose:     Convert native unsigned long long to native double using
 *              hardware.  This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ullong_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                        size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                        void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(ULLONG, DOUBLE, unsigned long long, double, -, -);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ullong_ldouble
 *
 * Purpose:     Convert native unsigned long long to native long double using
 *              hardware.  This is a fast special case.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5T_CONV_INTERNAL_ULLONG_LDOUBLE
herr_t
H5T__conv_ullong_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                         size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *buf,
                         void H5_ATTR_UNUSED *bkg)
{
    H5T_CONV_xF(ULLONG, LDOUBLE, unsigned long long, long double, -, -);
}
#endif /*H5T_CONV_INTERNAL_ULLONG_LDOUBLE*/
