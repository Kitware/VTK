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
 * Purpose: Datatype conversion functions for string datatypes
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
#include "H5Tconv_string.h"

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_s_s
 *
 * Purpose:     Convert one fixed-length string type to another.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_s_s(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
              const H5T_conv_ctx_t H5_ATTR_UNUSED *conv_ctx, size_t nelmts, size_t buf_stride,
              size_t H5_ATTR_UNUSED bkg_stride, void *buf, void H5_ATTR_UNUSED *bkg)
{
    ssize_t  src_delta, dst_delta; /*source & destination stride    */
    int      direction;            /*direction of traversal    */
    size_t   elmtno;               /*element number        */
    size_t   olap;                 /*num overlapping elements    */
    size_t   nchars = 0;           /*number of characters copied    */
    uint8_t *s, *sp, *d, *dp;      /*src and dst traversal pointers*/
    uint8_t *dbuf      = NULL;     /*temp buf for overlap converts.    */
    herr_t   ret_value = SUCCEED;  /* Return value */

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT:
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (8 * src->shared->size != src->shared->u.atomic.prec ||
                8 * dst->shared->size != dst->shared->u.atomic.prec)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "bad precision");
            if (0 != src->shared->u.atomic.offset || 0 != dst->shared->u.atomic.offset)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "bad offset");
            if (H5T_CSET_ASCII != src->shared->u.atomic.u.s.cset &&
                H5T_CSET_UTF8 != src->shared->u.atomic.u.s.cset)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "bad source character set");
            if (H5T_CSET_ASCII != dst->shared->u.atomic.u.s.cset &&
                H5T_CSET_UTF8 != dst->shared->u.atomic.u.s.cset)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "bad destination character set");
            if ((H5T_CSET_ASCII == src->shared->u.atomic.u.s.cset &&
                 H5T_CSET_UTF8 == dst->shared->u.atomic.u.s.cset) ||
                (H5T_CSET_ASCII == dst->shared->u.atomic.u.s.cset &&
                 H5T_CSET_UTF8 == src->shared->u.atomic.u.s.cset))
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                            "The library doesn't convert between strings of ASCII and UTF");
            if (src->shared->u.atomic.u.s.pad < 0 || src->shared->u.atomic.u.s.pad >= H5T_NSTR ||
                dst->shared->u.atomic.u.s.pad < 0 || dst->shared->u.atomic.u.s.pad >= H5T_NSTR)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "bad character padding");
            cdata->need_bkg = H5T_BKG_NO;
            break;

        case H5T_CONV_FREE:
            break;

        case H5T_CONV_CONV:
            /* Get the datatypes */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");

            /*
             * Do we process the values from beginning to end or vice versa? Also,
             * how many of the elements have the source and destination areas
             * overlapping?
             */
            if (src->shared->size == dst->shared->size || buf_stride) {
                /*
                 * When the source and destination are the same size we can do
                 * all the conversions in place.
                 */
                sp = dp   = (uint8_t *)buf;
                direction = 1;
                olap      = 0;
            }
            else if (src->shared->size >= dst->shared->size) {
                double olapd =
                    ceil((double)(dst->shared->size) / (double)(src->shared->size - dst->shared->size));
                olap = (size_t)olapd;
                sp = dp   = (uint8_t *)buf;
                direction = 1;
            }
            else {
                double olapd =
                    ceil((double)(src->shared->size) / (double)(dst->shared->size - src->shared->size));
                olap      = (size_t)olapd;
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

            /* Allocate the overlap buffer */
            if (NULL == (dbuf = (uint8_t *)H5MM_calloc(dst->shared->size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                            "memory allocation failed for string conversion");

            /* The conversion loop. */
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
                if (src->shared->size == dst->shared->size || buf_stride) {
                    assert(s == d);
                }
                else if (d == dbuf) {
                    assert((dp >= sp && dp < sp + src->shared->size) ||
                           (sp >= dp && sp < dp + dst->shared->size));
                }
                else {
                    assert((dp < sp && dp + dst->shared->size <= sp) ||
                           (sp < dp && sp + src->shared->size <= dp));
                }
#endif

                /* Copy characters from source to destination */
                switch (src->shared->u.atomic.u.s.pad) {
                    case H5T_STR_NULLTERM:
                        for (nchars = 0;
                             nchars < dst->shared->size && nchars < src->shared->size && s[nchars];
                             nchars++) {
                            d[nchars] = s[nchars];
                        }
                        break;

                    case H5T_STR_NULLPAD:
                        for (nchars = 0;
                             nchars < dst->shared->size && nchars < src->shared->size && s[nchars];
                             nchars++) {
                            d[nchars] = s[nchars];
                        }
                        break;

                    case H5T_STR_SPACEPAD:
                        nchars = src->shared->size;
                        while (nchars > 0 && ' ' == s[nchars - 1])
                            --nchars;
                        nchars = MIN(dst->shared->size, nchars);
                        if (d != s)
                            H5MM_memcpy(d, s, nchars);
                        break;

                    case H5T_STR_RESERVED_3:
                    case H5T_STR_RESERVED_4:
                    case H5T_STR_RESERVED_5:
                    case H5T_STR_RESERVED_6:
                    case H5T_STR_RESERVED_7:
                    case H5T_STR_RESERVED_8:
                    case H5T_STR_RESERVED_9:
                    case H5T_STR_RESERVED_10:
                    case H5T_STR_RESERVED_11:
                    case H5T_STR_RESERVED_12:
                    case H5T_STR_RESERVED_13:
                    case H5T_STR_RESERVED_14:
                    case H5T_STR_RESERVED_15:
                    case H5T_STR_ERROR:
                    default:
                        HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                    "source string padding method not supported");
                } /* end switch */

                /* Terminate or pad the destination */
                switch (dst->shared->u.atomic.u.s.pad) {
                    case H5T_STR_NULLTERM:
                        while (nchars < dst->shared->size)
                            d[nchars++] = '\0';
                        d[dst->shared->size - 1] = '\0';
                        break;

                    case H5T_STR_NULLPAD:
                        while (nchars < dst->shared->size)
                            d[nchars++] = '\0';
                        break;

                    case H5T_STR_SPACEPAD:
                        while (nchars < dst->shared->size)
                            d[nchars++] = ' ';
                        break;

                    case H5T_STR_RESERVED_3:
                    case H5T_STR_RESERVED_4:
                    case H5T_STR_RESERVED_5:
                    case H5T_STR_RESERVED_6:
                    case H5T_STR_RESERVED_7:
                    case H5T_STR_RESERVED_8:
                    case H5T_STR_RESERVED_9:
                    case H5T_STR_RESERVED_10:
                    case H5T_STR_RESERVED_11:
                    case H5T_STR_RESERVED_12:
                    case H5T_STR_RESERVED_13:
                    case H5T_STR_RESERVED_14:
                    case H5T_STR_RESERVED_15:
                    case H5T_STR_ERROR:
                    default:
                        HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                    "destination string padding method not supported");
                } /* end switch */

                /*
                 * If we used a temporary buffer for the destination then we
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
    H5MM_xfree(dbuf);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_s_s() */
