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
 * Purpose: Datatype conversion functions for enum datatypes
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
#include "H5Tconv_enum.h"

/******************/
/* Local Typedefs */
/******************/

/* Private conversion data for enum datatypes */
typedef struct H5T_conv_enum_t {
    H5T_t   *src_copy; /* cached copy of source datatype      */
    H5T_t   *dst_copy; /* cached copy of destination datatype */
    int      base;     /* lowest `in' value                   */
    unsigned length;   /* num elements in arrays              */
    int     *src2dst;  /* map from src to dst index           */
} H5T_conv_enum_t;

/********************/
/* Local Prototypes */
/********************/

static herr_t H5T__conv_enum_init(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx);
static herr_t H5T__conv_enum_free(H5T_conv_enum_t *priv);

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_enum_init
 *
 * Purpose:     Initialize private data for enum datatype conversions.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T__conv_enum_init(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx)
{
    H5T_conv_enum_t *priv          = NULL; /* Private conversion data */
    int             *map           = NULL; /* Map from src value to dst idx */
    bool             rebuild_cache = false;
    herr_t           ret_value     = SUCCEED;

    FUNC_ENTER_PACKAGE

    cdata->need_bkg = H5T_BKG_NO;

    priv = (H5T_conv_enum_t *)(cdata->priv);
    if (!priv) {
        if (NULL == (priv = (H5T_conv_enum_t *)(cdata->priv = calloc(1, sizeof(*priv)))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");
        rebuild_cache = true;
    }
    else {
        /* Check if we need to rebuild our cache. For now, treat
         * enums as different even if one is just a subset of the
         * other
         */
        if (cdata->command == H5T_CONV_CONV && conv_ctx->u.conv.recursive)
            /* Recursive conversion; we can reuse the cache */
            rebuild_cache = false;
        else {
            if (0 != H5T_cmp(src, priv->src_copy, false) || 0 != H5T_cmp(dst, priv->dst_copy, false))
                rebuild_cache = true;
        }
    }

    if (rebuild_cache) {
        H5T_shared_t *src_sh;
        H5T_shared_t *dst_sh;
        size_t        src_nmembs;
        size_t        dst_nmembs;
        void         *tmp_realloc;

        /* Allocate everything we need to cache */
        if (priv->src_copy && H5T_close(priv->src_copy) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCLOSEOBJ, FAIL, "unable to close copied source datatype");
        if (priv->dst_copy && H5T_close(priv->dst_copy) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCLOSEOBJ, FAIL, "unable to close copied destination datatype");

        if (NULL == (priv->src_copy = H5T_copy(src, H5T_COPY_ALL)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, FAIL, "unable to copy source datatype");
        if (NULL == (priv->dst_copy = H5T_copy(dst, H5T_COPY_ALL)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, FAIL, "unable to copy destination datatype");

        /* Nothing more to do if enum has no members */
        if (0 == src->shared->u.enumer.nmembs)
            HGOTO_DONE(SUCCEED);

        src_sh     = priv->src_copy->shared;
        dst_sh     = priv->src_copy->shared;
        src_nmembs = src_sh->u.enumer.nmembs;
        dst_nmembs = dst_sh->u.enumer.nmembs;

        if (NULL == (tmp_realloc = realloc(priv->src2dst, src_nmembs * sizeof(int)))) {
            free(priv->src2dst);
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL,
                        "unable to allocate space for source to destination enum mapping");
        }
        priv->src2dst = tmp_realloc;

        /*
         * Check that the source symbol names are a subset of the destination
         * symbol names and build a map from source member index to destination
         * member index.
         */
        H5T__sort_name(priv->src_copy, NULL);
        H5T__sort_name(priv->dst_copy, NULL);
        for (size_t i = 0, j = 0; i < src_nmembs && j < dst_nmembs; i++, j++) {
            char *src_name = src_sh->u.enumer.name[i];
            char *dst_name = dst_sh->u.enumer.name[j];

            while (j < dst_nmembs && strcmp(src_name, dst_name) != 0)
                j++;

            if (j >= dst_nmembs)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                            "source enum type is not a subset of destination enum type");

            H5_CHECKED_ASSIGN(priv->src2dst[i], int, j, size_t);
        }

        /*
         * The conversion function will use an O(log N) lookup method for each
         * value converted. However, if all of the following constraints are met
         * then we can build a perfect hash table and use an O(1) lookup method.
         *
         *      A: The source datatype size matches one of our native datatype
         *         sizes.
         *
         *      B: After casting the source value bit pattern to a native type
         *         the size of the range of values is less than 20% larger than
         *         the number of values.
         *
         * If this special case is met then we use the source bit pattern cast as
         * a native integer type as an index into the `val2dst'. The values of
         * that array are the index numbers in the destination type or negative
         * if the entry is unused.
         *
         * (This optimized algorithm doesn't work when the byte orders are different.
         * The code such as "n = *((int *)((void *)((uint8_t *)src_sh->u.enumer.value + (i *
         * src_sh->size))));" can change the value significantly. i.g. if the source value is big-endian
         * 0x0000000f, executing the casting on little-endian machine will get a big number 0x0f000000. Then
         * it can't meet the condition "if (src_nmembs < 2 || ((double)length / (double)src_nmembs <
         * (double)(1.2F)))" Because this is the optimized code, we won't fix it. It should still work in some
         * situations. SLU - 2011/5/24)
         */
        if (1 == src_sh->size || sizeof(short) == src_sh->size || sizeof(int) == src_sh->size) {
            unsigned length;
            int      domain[2] = {0, 0}; /* Min and max source values */

            for (size_t i = 0; i < src_nmembs; i++) {
                int n;

                if (1 == src_sh->size)
                    n = *((signed char *)((uint8_t *)src_sh->u.enumer.value + i));
                else if (sizeof(short) == src_sh->size)
                    n = *((short *)((void *)((uint8_t *)src_sh->u.enumer.value + (i * src_sh->size))));
                else
                    n = *((int *)((void *)((uint8_t *)src_sh->u.enumer.value + (i * src_sh->size))));
                if (0 == i) {
                    domain[0] = domain[1] = n;
                }
                else {
                    domain[0] = MIN(domain[0], n);
                    domain[1] = MAX(domain[1], n);
                }
            }
            assert(domain[1] >= domain[0]);

            length = (unsigned)(domain[1] - domain[0]) + 1;
            if (src_nmembs < 2 || ((double)length / (double)src_nmembs < (double)(1.2F))) {
                priv->base   = domain[0];
                priv->length = length;

                if (NULL == (map = malloc(length * sizeof(int))))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "memory allocation failed");

                for (size_t i = 0; i < length; i++)
                    map[i] = -1; /*entry unused*/

                for (size_t i = 0; i < src_nmembs; i++) {
                    int n;

                    if (1 == src_sh->size)
                        n = *((signed char *)((uint8_t *)src_sh->u.enumer.value + i));
                    else if (sizeof(short) == src_sh->size)
                        n = *((short *)((void *)((uint8_t *)src_sh->u.enumer.value + (i * src_sh->size))));
                    else
                        n = *((int *)((void *)((uint8_t *)src_sh->u.enumer.value + (i * src_sh->size))));
                    n -= priv->base;
                    assert(n >= 0 && (unsigned)n < priv->length);
                    assert(map[n] < 0);
                    map[n] = priv->src2dst[i];
                }

                /*
                 * Replace original src2dst array with our new one. The original
                 * was indexed by source member number while the new one is
                 * indexed by source values.
                 */
                free(priv->src2dst);
                priv->src2dst = map;

                HGOTO_DONE(SUCCEED);
            }
        }

        /* Sort source type by value and adjust src2dst[] appropriately */
        H5T__sort_value(priv->src_copy, priv->src2dst);
    }

#ifdef H5T_DEBUG
    if (H5DEBUG(T)) {
        fprintf(H5DEBUG(T), "      Using %s mapping function%s\n", priv->length ? "O(1)" : "O(log N)",
                priv->length ? "" : ", where N is the number of enum members");
    }
#endif

done:
    if (ret_value < 0 && priv) {
        if (map) {
            free(map);
            priv->src2dst = NULL;
        }

        if (H5T__conv_enum_free(priv) < 0)
            HDONE_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "can't free enum conversion data");

        cdata->priv = NULL;
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_enum_init() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_enum_free
 *
 * Purpose:     Free the private data structure used by the enum conversion
 *              functions.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T__conv_enum_free(H5T_conv_enum_t *priv)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (priv) {
        free(priv->src2dst);

        if (priv->dst_copy && H5T_close(priv->dst_copy) < 0)
            HDONE_ERROR(H5E_DATATYPE, H5E_CANTCLOSEOBJ, FAIL, "unable to close copied source datatype");
        if (priv->src_copy && H5T_close(priv->src_copy) < 0)
            HDONE_ERROR(H5E_DATATYPE, H5E_CANTCLOSEOBJ, FAIL, "unable to close copied destination datatype");

        free(priv);
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_enum_free() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_enum
 *
 * Purpose:     Converts one type of enumerated data to another.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_enum(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
               size_t nelmts, size_t buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *_buf,
               void H5_ATTR_UNUSED *bkg)
{
    H5T_conv_enum_t *priv   = (H5T_conv_enum_t *)(cdata->priv);
    H5T_shared_t    *src_sh = NULL;
    H5T_shared_t    *dst_sh = NULL;
    uint8_t         *buf    = (uint8_t *)_buf; /*cast for pointer arithmetic    */
    uint8_t         *s = NULL, *d = NULL;      /*src and dst BUF pointers    */
    ssize_t          src_delta, dst_delta;     /*conversion strides        */
    int              n;                        /*src value cast as native int    */
    H5T_conv_ret_t   except_ret;               /*return of callback function   */
    size_t           i;                        /*counters            */
    herr_t           ret_value = SUCCEED;      /* Return value                 */

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT:
            /*
             * Determine if this conversion function applies to the conversion
             * path SRC->DST.  If not return failure; otherwise initialize
             * the `priv' field of `cdata' with information about the underlying
             * integer conversion.
             */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a datatype");
            if (H5T_ENUM != src->shared->type)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a H5T_ENUM datatype");
            if (H5T_ENUM != dst->shared->type)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a H5T_ENUM datatype");

            if (H5T__conv_enum_init(src, dst, cdata, conv_ctx) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to initialize private data");
            break;

        case H5T_CONV_FREE: {
            herr_t status = H5T__conv_enum_free(priv);
            cdata->priv   = NULL;
            if (status < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "unable to free private conversion data");

            break;
        }

        case H5T_CONV_CONV:
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (NULL == conv_ctx)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid datatype conversion context pointer");
            if (H5T_ENUM != src->shared->type)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a H5T_ENUM datatype");
            if (H5T_ENUM != dst->shared->type)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a H5T_ENUM datatype");

            /* Reuse cache if possible, rebuild otherwise */
            if (H5T__conv_enum_init(src, dst, cdata, conv_ctx) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to initialize private data");

            src_sh = priv->src_copy->shared;
            dst_sh = priv->dst_copy->shared;

            /*
             * Direction of conversion.
             */
            if (buf_stride) {
                H5_CHECK_OVERFLOW(buf_stride, size_t, ssize_t);
                src_delta = dst_delta = (ssize_t)buf_stride;
                s = d = buf;
            }
            else if (dst_sh->size <= src_sh->size) {
                H5_CHECKED_ASSIGN(src_delta, ssize_t, src_sh->size, size_t);
                H5_CHECKED_ASSIGN(dst_delta, ssize_t, dst_sh->size, size_t);
                s = d = buf;
            }
            else {
                H5_CHECK_OVERFLOW(src_sh->size, size_t, ssize_t);
                H5_CHECK_OVERFLOW(dst_sh->size, size_t, ssize_t);
                src_delta = -(ssize_t)src_sh->size;
                dst_delta = -(ssize_t)dst_sh->size;
                s         = buf + (nelmts - 1) * src_sh->size;
                d         = buf + (nelmts - 1) * dst_sh->size;
            }

            if (priv->length) {
                for (i = 0; i < nelmts; i++, s += src_delta, d += dst_delta) {
                    /* Use O(1) lookup */
                    /* (The casting won't work when the byte orders are different. i.g. if the source value
                     * is big-endian 0x0000000f, the direct casting "n = *((int *)((void *)s));" will make
                     * it a big number 0x0f000000 on little-endian machine. But we won't fix it because it's
                     * an optimization code. Please also see the comment in the H5T__conv_enum_init()
                     * function. SLU - 2011/5/24)
                     */
                    if (1 == src_sh->size)
                        n = *((signed char *)s);
                    else if (sizeof(short) == src_sh->size)
                        n = *((short *)((void *)s));
                    else
                        n = *((int *)((void *)s));
                    n -= priv->base;
                    if (n < 0 || (unsigned)n >= priv->length || priv->src2dst[n] < 0) {
                        /*overflow*/
                        except_ret = H5T_CONV_UNHANDLED;
                        /*If user's exception handler is present, use it*/
                        if (conv_ctx->u.conv.cb_struct.func)
                            except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                                conv_ctx->u.conv.dst_type_id, s, d, conv_ctx->u.conv.cb_struct.user_data);

                        if (except_ret == H5T_CONV_UNHANDLED)
                            memset(d, 0xff, dst_sh->size);
                        else if (except_ret == H5T_CONV_ABORT)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "can't handle conversion exception");
                    }
                    else
                        H5MM_memcpy(d,
                                    (uint8_t *)dst_sh->u.enumer.value +
                                        ((unsigned)priv->src2dst[n] * dst_sh->size),
                                    dst_sh->size);
                }
            }
            else {
                for (i = 0; i < nelmts; i++, s += src_delta, d += dst_delta) {
                    /* Use O(log N) lookup */
                    unsigned lt = 0;
                    unsigned rt = src_sh->u.enumer.nmembs;
                    unsigned md = 0;
                    int      cmp;

                    while (lt < rt) {
                        md = (lt + rt) / 2;
                        cmp =
                            memcmp(s, (uint8_t *)src_sh->u.enumer.value + (md * src_sh->size), src_sh->size);
                        if (cmp < 0)
                            rt = md;
                        else if (cmp > 0)
                            lt = md + 1;
                        else
                            break;
                    } /* end while */
                    if (lt >= rt) {
                        except_ret = H5T_CONV_UNHANDLED;
                        /*If user's exception handler is present, use it*/
                        if (conv_ctx->u.conv.cb_struct.func)
                            except_ret = (conv_ctx->u.conv.cb_struct.func)(
                                H5T_CONV_EXCEPT_RANGE_HI, conv_ctx->u.conv.src_type_id,
                                conv_ctx->u.conv.dst_type_id, s, d, conv_ctx->u.conv.cb_struct.user_data);

                        if (except_ret == H5T_CONV_UNHANDLED)
                            memset(d, 0xff, dst_sh->size);
                        else if (except_ret == H5T_CONV_ABORT)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "can't handle conversion exception");
                    } /* end if */
                    else {
                        assert(priv->src2dst[md] >= 0);
                        H5MM_memcpy(d,
                                    (uint8_t *)dst_sh->u.enumer.value +
                                        ((unsigned)priv->src2dst[md] * dst_sh->size),
                                    dst_sh->size);
                    } /* end else */
                }
            }

            break;

        default:
            /* Some other command we don't know about yet.*/
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_enum() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_enum_numeric
 *
 * Purpose:     Converts enumerated data to a numeric type (integer or
 *              floating-point number). This function is registered into
 *              the conversion table twice in H5T_init_interface in H5T.c.
 *              Once for enum-integer conversion. Once for enum-float
 *              conversion.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_enum_numeric(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                       const H5T_conv_ctx_t H5_ATTR_UNUSED *conv_ctx, size_t nelmts,
                       size_t H5_ATTR_UNUSED buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void *_buf,
                       void H5_ATTR_UNUSED *bkg)
{
    H5T_t      *src_parent;          /*parent type for src           */
    H5T_path_t *tpath;               /* Conversion information       */
    herr_t      ret_value = SUCCEED; /* Return value                 */

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT:
            /*
             * Determine if this conversion function applies to the conversion
             * path SRC->DST.  If not, return failure.
             */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a datatype");
            if (H5T_ENUM != src->shared->type)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "source type is not a H5T_ENUM datatype");
            if (H5T_INTEGER != dst->shared->type && H5T_FLOAT != dst->shared->type)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "destination is not an integer type");

            cdata->need_bkg = H5T_BKG_NO;
            break;

        case H5T_CONV_FREE:
            break;

        case H5T_CONV_CONV:
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");

            src_parent = src->shared->parent;

            if (NULL == (tpath = H5T_path_find(src_parent, dst))) {
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                            "unable to convert between src and dest datatype");
            }
            else if (!H5T_path_noop(tpath)) {
                /* Convert the data */
                if (H5T_convert(tpath, src_parent, dst, nelmts, buf_stride, bkg_stride, _buf, bkg) < 0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "datatype conversion failed");
            }
            break;

        default:
            /* Some other command we don't know about yet.*/
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_enum_numeric() */
