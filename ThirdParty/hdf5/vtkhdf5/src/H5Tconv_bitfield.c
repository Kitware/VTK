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
 * Purpose: Datatype conversion functions for bitfield datatypes
 */

/****************/
/* Module Setup */
/****************/
#include "H5Tmodule.h" /* This source code file is part of the H5T module */

/***********/
/* Headers */
/***********/
#include "H5Eprivate.h"
#include "H5Tconv.h"
#include "H5Tconv_bitfield.h"

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_b_b
 *
 * Purpose:     Convert from one bitfield to any other bitfield.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_b_b(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
              size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *_buf,
              void H5_ATTR_UNUSED *background)
{
    uint8_t       *buf = (uint8_t *)_buf;
    ssize_t        direction;       /*direction of traversal    */
    size_t         elmtno;          /*element number        */
    size_t         olap;            /*num overlapping elements    */
    size_t         half_size;       /*1/2 of total size for swapping*/
    uint8_t       *s, *sp, *d, *dp; /*source and dest traversal ptrs*/
    uint8_t        dbuf[256] = {0}; /*temp destination buffer    */
    size_t         msb_pad_offset;  /*offset for dest MSB padding    */
    size_t         i;
    uint8_t       *src_rev = NULL;      /*order-reversed source buffer  */
    H5T_conv_ret_t except_ret;          /*return of callback function   */
    bool           reverse;             /*if reverse the order of destination        */
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT:
            /* Capability query */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (H5T_ORDER_LE != src->shared->u.atomic.order && H5T_ORDER_BE != src->shared->u.atomic.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unsupported byte order");
            if (H5T_ORDER_LE != dst->shared->u.atomic.order && H5T_ORDER_BE != dst->shared->u.atomic.order)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unsupported byte order");
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

            /* Allocate space for order-reversed source buffer */
            src_rev = (uint8_t *)H5MM_calloc(src->shared->size);

            /* The conversion loop */
            H5_CHECK_OVERFLOW(buf_stride, size_t, ssize_t);
            H5_CHECK_OVERFLOW(src->shared->size, size_t, ssize_t);
            H5_CHECK_OVERFLOW(dst->shared->size, size_t, ssize_t);
            for (elmtno = 0; elmtno < nelmts; elmtno++) {

                /*
                 * If the source and destination buffers overlap then use a
                 * temporary buffer for the destination.
                 */
                if (direction > 0) {
                    s = sp;
                    d = elmtno < olap ? dbuf : dp;
                } /* end if */
                else {
                    s = sp;
                    d = (elmtno + olap) >= nelmts ? dbuf : dp;
                } /* end else */
#ifndef NDEBUG
                /* I don't quite trust the overlap calculations yet  */
                if (d == dbuf)
                    assert((dp >= sp && dp < sp + src->shared->size) ||
                           (sp >= dp && sp < dp + dst->shared->size));
                else
                    assert((dp < sp && dp + dst->shared->size <= sp) ||
                           (sp < dp && sp + src->shared->size <= dp));
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
                    } /* end for */
                }     /* end if */

                /* Initiate these variables */
                except_ret = H5T_CONV_UNHANDLED;
                reverse    = true;

                /*
                 * Copy the significant part of the value. If the source is larger
                 * than the destination then invoke the overflow function or copy
                 * as many bits as possible. Zero extra bits in the destination.
                 */
                if (src->shared->u.atomic.prec > dst->shared->u.atomic.prec) {
                    /*overflow*/
                    if (conv_ctx->u.conv.cb_struct.func) { /*If user's exception handler is present, use it*/
                        H5T__reverse_order(src_rev, s, src->shared->size,
                                           src->shared->u.atomic.order); /*reverse order first*/
                        except_ret = (conv_ctx->u.conv.cb_struct.func)(
                            H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                            conv_ctx->u.conv.dst_type_id, src_rev, d, conv_ctx->u.conv.cb_struct.user_data);
                    } /* end if */

                    if (except_ret == H5T_CONV_UNHANDLED) {
                        H5T__bit_copy(d, dst->shared->u.atomic.offset, s, src->shared->u.atomic.offset,
                                      dst->shared->u.atomic.prec);
                    }
                    else if (except_ret == H5T_CONV_ABORT)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "can't handle conversion exception");
                    else if (except_ret == H5T_CONV_HANDLED)
                        /*Don't reverse because user handles it*/
                        reverse = false;
                }
                else {
                    H5T__bit_copy(d, dst->shared->u.atomic.offset, s, src->shared->u.atomic.offset,
                                  src->shared->u.atomic.prec);
                    H5T__bit_set(d, dst->shared->u.atomic.offset + src->shared->u.atomic.prec,
                                 dst->shared->u.atomic.prec - src->shared->u.atomic.prec, false);
                }

                /*
                 * Fill the destination padding areas.
                 */
                switch (dst->shared->u.atomic.lsb_pad) {
                    case H5T_PAD_ZERO:
                        H5T__bit_set(d, (size_t)0, dst->shared->u.atomic.offset, false);
                        break;

                    case H5T_PAD_ONE:
                        H5T__bit_set(d, (size_t)0, dst->shared->u.atomic.offset, true);
                        break;

                    case H5T_PAD_ERROR:
                    case H5T_PAD_BACKGROUND:
                    case H5T_NPAD:
                    default:
                        HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unsupported LSB padding");
                } /* end switch */
                msb_pad_offset = dst->shared->u.atomic.offset + dst->shared->u.atomic.prec;
                switch (dst->shared->u.atomic.msb_pad) {
                    case H5T_PAD_ZERO:
                        H5T__bit_set(d, msb_pad_offset, 8 * dst->shared->size - msb_pad_offset, false);
                        break;

                    case H5T_PAD_ONE:
                        H5T__bit_set(d, msb_pad_offset, 8 * dst->shared->size - msb_pad_offset, true);
                        break;

                    case H5T_PAD_ERROR:
                    case H5T_PAD_BACKGROUND:
                    case H5T_NPAD:
                    default:
                        HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unsupported MSB padding");
                } /* end switch */

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
                    } /* end for */
                }     /* end if */

                /*
                 * If we had used a temporary buffer for the destination then we
                 * should copy the value to the true destination buffer.
                 */
                if (d == dbuf)
                    H5MM_memcpy(dp, d, dst->shared->size);
                if (buf_stride) {
                    sp += direction *
                          (ssize_t)buf_stride; /* Note that cast is checked with H5_CHECK_OVERFLOW, above */
                    dp += direction *
                          (ssize_t)buf_stride; /* Note that cast is checked with H5_CHECK_OVERFLOW, above */
                }                              /* end if */
                else {
                    sp += direction *
                          (ssize_t)
                              src->shared->size; /* Note that cast is checked with H5_CHECK_OVERFLOW, above */
                    dp += direction *
                          (ssize_t)
                              dst->shared->size; /* Note that cast is checked with H5_CHECK_OVERFLOW, above */
                }                                /* end else */
            }                                    /* end for */

            break;

        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    } /* end switch */

done:
    if (src_rev)
        H5MM_free(src_rev);
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_b_b() */
