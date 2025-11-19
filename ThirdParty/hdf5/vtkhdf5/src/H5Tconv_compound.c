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
 * Purpose: Datatype conversion functions for compound datatypes
 */

/****************/
/* Module Setup */
/****************/
#include "H5Tmodule.h" /* This source code file is part of the H5T module */

/***********/
/* Headers */
/***********/
#include "H5Eprivate.h"
#include "H5Iprivate.h"
#include "H5Tconv.h"
#include "H5Tconv_compound.h"

/******************/
/* Local Typedefs */
/******************/

/* Private conversion data for compound datatypes */
typedef struct H5T_conv_struct_t {
    int              *src2dst;     /* mapping from src to dst member num */
    H5T_t           **src_memb;    /* source member datatypes            */
    H5T_t           **dst_memb;    /* destination member datatypes       */
    hid_t            *src_memb_id; /* source member type ID's            */
    hid_t            *dst_memb_id; /* destination member type ID's       */
    H5T_path_t      **memb_path;   /* conversion path for each member    */
    H5T_subset_info_t subset_info; /* info related to compound subsets   */
    unsigned          src_nmembs;  /* needed by free function            */
} H5T_conv_struct_t;

/********************/
/* Local Prototypes */
/********************/

static herr_t H5T__conv_struct_init(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx);
static herr_t H5T__conv_struct_free(H5T_conv_struct_t *priv);

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_struct_subset
 *
 * Purpose:     A quick way to return a field in a struct private in this
 *              file. The `subset` enum field of the `H5T_subset_info_t`
 *              structure indicates whether the source members are a subset
 *              of the destination or the destination members are a subset
 *              of the source, and the order is the same, and no conversion
 *              is needed. For example:
 *
 *                  struct source {            struct destination {
 *                      TYPE1 A;      -->          TYPE1 A;
 *                      TYPE2 B;      -->          TYPE2 B;
 *                      TYPE3 C;      -->          TYPE3 C;
 *                  };                             TYPE4 D;
 *                                                 TYPE5 E;
 *                                             };
 *
 * Return:      A pointer to the subset info struct in `cdata`. Points
 *              directly into the structure.
 *
 *-------------------------------------------------------------------------
 */
H5T_subset_info_t *
H5T__conv_struct_subset(const H5T_cdata_t *cdata)
{
    H5T_conv_struct_t *priv = NULL;

    FUNC_ENTER_PACKAGE_NOERR

    assert(cdata);
    assert(cdata->priv);

    priv = (H5T_conv_struct_t *)(cdata->priv);

    FUNC_LEAVE_NOAPI((H5T_subset_info_t *)&priv->subset_info)
} /* end H5T__conv_struct_subset() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_struct_init
 *
 * Purpose:     Initialize the `priv' field of `cdata' with conversion
 *              information that is relatively constant. If `priv' is
 *              already initialized then the member conversion functions
 *              are recalculated.
 *
 *              Priv fields are indexed by source member number or
 *              destination member number depending on whether the field
 *              contains information about the source datatype or the
 *              destination datatype (fields that contains the same
 *              information for both source and destination are indexed by
 *              source member number).  The src2dst[] priv array maps
 *              source member numbers to destination member numbers, but
 *              if the source member doesn't have a corresponding
 *              destination member then the src2dst[i] == -1.
 *
 *              Special optimization case when the source and destination
 *              members are a subset of each other, and the order is the
 *              same, and no conversion is needed. For example:
 *
 *                  struct source {            struct destination {
 *                      TYPE1 A;      -->          TYPE1 A;
 *                      TYPE2 B;      -->          TYPE2 B;
 *                      TYPE3 C;      -->          TYPE3 C;
 *                  };                             TYPE4 D;
 *                                                 TYPE5 E;
 *                                             };
 *
 *              or
 *
 *                  struct destination {       struct source {
 *                      TYPE1 A;      <--          TYPE1 A;
 *                      TYPE2 B;      <--          TYPE2 B;
 *                      TYPE3 C;      <--          TYPE3 C;
 *                  };                             TYPE4 D;
 *                                                 TYPE5 E;
 *                                             };
 *
 *              The optimization is simply moving data to the appropriate
 *              places in the buffer.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T__conv_struct_init(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx)
{
    H5T_conv_struct_t *priv    = (H5T_conv_struct_t *)(cdata->priv);
    int               *src2dst = NULL;
    unsigned           src_nmembs, dst_nmembs;
    unsigned           i, j;
    herr_t             ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    src_nmembs = src->shared->u.compnd.nmembs;
    dst_nmembs = dst->shared->u.compnd.nmembs;

    if (!priv) {
        /*
         * Allocate private data structure and arrays.
         */
        if (NULL == (priv = (H5T_conv_struct_t *)(cdata->priv = H5MM_calloc(sizeof(H5T_conv_struct_t)))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "couldn't allocate private conversion data");
        if (NULL == (priv->src2dst = (int *)H5MM_malloc(src_nmembs * sizeof(int))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL,
                        "couldn't allocate source to destination member mapping array");
        if (NULL == (priv->src_memb = (H5T_t **)H5MM_malloc(src_nmembs * sizeof(H5T_t *))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL,
                        "couldn't allocate source compound member datatype array");
        if (NULL == (priv->dst_memb = (H5T_t **)H5MM_malloc(dst_nmembs * sizeof(H5T_t *))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL,
                        "couldn't allocate destination compound member datatype array");

        /* Allocate and initialize arrays for datatype IDs */
        if (NULL == (priv->src_memb_id = (hid_t *)H5MM_malloc(src_nmembs * sizeof(hid_t))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL,
                        "couldn't allocate source compound member datatype ID array");
        for (i = 0; i < src_nmembs; i++)
            priv->src_memb_id[i] = H5I_INVALID_HID;

        if (NULL == (priv->dst_memb_id = (hid_t *)H5MM_malloc(dst_nmembs * sizeof(hid_t))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL,
                        "couldn't allocate destination compound member datatype ID array");
        for (i = 0; i < dst_nmembs; i++)
            priv->dst_memb_id[i] = H5I_INVALID_HID;

        src2dst          = priv->src2dst;
        priv->src_nmembs = src_nmembs;

        /* The flag of special optimization to indicate if source members and destination
         * members are a subset of each other.  Initialize it to false */
        priv->subset_info.subset    = H5T_SUBSET_FALSE;
        priv->subset_info.copy_size = 0;

        /*
         * Ensure that members are sorted.
         */
        H5T__sort_value(src, NULL);
        H5T__sort_value(dst, NULL);

        /*
         * Build a mapping from source member number to destination member
         * number. If some source member is not a destination member then that
         * mapping element will be negative.  Also create atoms for each
         * source and destination member datatype if necessary.
         */
        for (i = 0; i < src_nmembs; i++) {
            src2dst[i] = -1;
            for (j = 0; j < dst_nmembs; j++) {
                if (!strcmp(src->shared->u.compnd.memb[i].name, dst->shared->u.compnd.memb[j].name)) {
                    H5_CHECKED_ASSIGN(src2dst[i], int, j, unsigned);
                    break;
                } /* end if */
            }     /* end for */
            if (src2dst[i] >= 0) {
                H5T_t *type;

                if (NULL == (type = H5T_copy(src->shared->u.compnd.memb[i].type, H5T_COPY_ALL)))
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, FAIL,
                                "can't copy source compound member datatype");
                priv->src_memb[i] = type;

                if (NULL == (type = H5T_copy(dst->shared->u.compnd.memb[src2dst[i]].type, H5T_COPY_ALL)))
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, FAIL,
                                "can't copy destination compound member datatype");
                priv->dst_memb[src2dst[i]] = type;
            } /* end if */
        }     /* end for */
    }         /* end if */
    else {
        /* Restore sorted conditions for the datatypes */
        /* (Required for the src2dst array to be valid) */
        H5T__sort_value(src, NULL);
        H5T__sort_value(dst, NULL);
    } /* end else */

    /*
     * (Re)build the cache of member conversion functions and pointers to
     * their cdata entries.
     */
    src2dst = priv->src2dst;
    H5MM_xfree(priv->memb_path);
    if (NULL ==
        (priv->memb_path = (H5T_path_t **)H5MM_malloc(src->shared->u.compnd.nmembs * sizeof(H5T_path_t *))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");

    for (i = 0; i < src_nmembs; i++) {
        if (src2dst[i] >= 0) {
            H5T_path_t *tpath;
            bool        need_ids;

            tpath = H5T_path_find(src->shared->u.compnd.memb[i].type,
                                  dst->shared->u.compnd.memb[src2dst[i]].type);

            if (NULL == (priv->memb_path[i] = tpath)) {
                H5T__conv_struct_free(priv);
                cdata->priv = NULL;
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unable to convert member datatype");
            } /* end if */

            /* Create IDs for the compound member datatypes if the conversion path uses
             * an application conversion function or if a conversion exception function
             * was provided.
             */
            need_ids = tpath->conv.is_app ||
                       (cdata->command == H5T_CONV_INIT && conv_ctx->u.init.cb_struct.func) ||
                       (cdata->command == H5T_CONV_CONV && conv_ctx->u.conv.cb_struct.func);

            if (need_ids) {
                hid_t tid;

                /* Only register new IDs for the source and destination member datatypes
                 * if IDs weren't already registered for them. If the cached conversion
                 * information has to be recalculated (in the case where the library's
                 * table of conversion functions is modified), the same IDs can be reused
                 * since the only information that needs to be recalculated is the conversion
                 * paths used.
                 */
                if (priv->src_memb_id[i] == H5I_INVALID_HID) {
                    if ((tid = H5I_register(H5I_DATATYPE, priv->src_memb[i], false)) < 0) {
                        H5T__conv_struct_free(priv);
                        cdata->priv = NULL;
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL,
                                    "can't register ID for source compound member datatype");
                    }
                    priv->src_memb_id[i] = tid;
                }

                if (priv->dst_memb_id[src2dst[i]] == H5I_INVALID_HID) {
                    if ((tid = H5I_register(H5I_DATATYPE, priv->dst_memb[src2dst[i]], false)) < 0) {
                        H5T__conv_struct_free(priv);
                        cdata->priv = NULL;
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL,
                                    "can't register ID for destination compound member datatype");
                    }
                    priv->dst_memb_id[src2dst[i]] = tid;
                }
            }
        } /* end if */
    }     /* end for */

    /* The compound conversion functions need a background buffer */
    cdata->need_bkg = H5T_BKG_YES;

    if (src_nmembs < dst_nmembs) {
        priv->subset_info.subset = H5T_SUBSET_SRC;
        for (i = 0; i < src_nmembs; i++) {
            /* If any of source members doesn't have counterpart in the same
             * order or there's conversion between members, don't do the
             * optimization.
             */
            if (src2dst[i] != (int)i ||
                (src->shared->u.compnd.memb[i].offset != dst->shared->u.compnd.memb[i].offset) ||
                (priv->memb_path[i])->is_noop == false) {
                priv->subset_info.subset = H5T_SUBSET_FALSE;
                break;
            } /* end if */
        }     /* end for */
        /* Compute the size of the data to be copied for each element.  It
         * may be smaller than either src or dst if there is extra space at
         * the end of src.
         */
        if (priv->subset_info.subset == H5T_SUBSET_SRC)
            priv->subset_info.copy_size = src->shared->u.compnd.memb[src_nmembs - 1].offset +
                                          src->shared->u.compnd.memb[src_nmembs - 1].size;
    }
    else if (dst_nmembs < src_nmembs) {
        priv->subset_info.subset = H5T_SUBSET_DST;
        for (i = 0; i < dst_nmembs; i++) {
            /* If any of source members doesn't have counterpart in the same order or
             * there's conversion between members, don't do the optimization. */
            if (src2dst[i] != (int)i ||
                (src->shared->u.compnd.memb[i].offset != dst->shared->u.compnd.memb[i].offset) ||
                (priv->memb_path[i])->is_noop == false) {
                priv->subset_info.subset = H5T_SUBSET_FALSE;
                break;
            }
        } /* end for */
        /* Compute the size of the data to be copied for each element.  It
         * may be smaller than either src or dst if there is extra space at
         * the end of dst.
         */
        if (priv->subset_info.subset == H5T_SUBSET_DST)
            priv->subset_info.copy_size = dst->shared->u.compnd.memb[dst_nmembs - 1].offset +
                                          dst->shared->u.compnd.memb[dst_nmembs - 1].size;
    }
    else /* If the numbers of source and dest members are equal and no conversion is needed,
          * the case should have been handled as noop earlier in H5Dio.c. */
    {
    }

    cdata->recalc = false;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_struct_init() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_struct_free
 *
 * Purpose:     Free the private data structure used by the compound
 *              conversion functions.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T__conv_struct_free(H5T_conv_struct_t *priv)
{
    int    *src2dst     = priv->src2dst;
    H5T_t **src_memb    = priv->src_memb;
    H5T_t **dst_memb    = priv->dst_memb;
    hid_t  *src_memb_id = priv->src_memb_id;
    hid_t  *dst_memb_id = priv->dst_memb_id;
    herr_t  ret_value   = SUCCEED;

    FUNC_ENTER_PACKAGE_NOERR

    for (unsigned i = 0; i < priv->src_nmembs; i++)
        if (src2dst[i] >= 0) {
            if (src_memb_id[i] >= 0) {
                if (H5I_dec_ref(src_memb_id[i]) < 0)
                    ret_value = FAIL; /* set return value, but keep going */
                src_memb_id[i] = H5I_INVALID_HID;
                src_memb[i]    = NULL;
            }
            else {
                if (H5T_close(src_memb[i]) < 0)
                    ret_value = FAIL; /* set return value, but keep going */
                src_memb[i] = NULL;
            }
            if (dst_memb_id[src2dst[i]] >= 0) {
                if (H5I_dec_ref(dst_memb_id[src2dst[i]]) < 0)
                    ret_value = FAIL; /* set return value, but keep going */
                dst_memb_id[src2dst[i]] = H5I_INVALID_HID;
                dst_memb[src2dst[i]]    = NULL;
            }
            else {
                if (H5T_close(dst_memb[src2dst[i]]) < 0)
                    ret_value = FAIL; /* set return value, but keep going */
                dst_memb[src2dst[i]] = NULL;
            }
        } /* end if */

    H5MM_xfree(src2dst);
    H5MM_xfree(src_memb);
    H5MM_xfree(dst_memb);
    H5MM_xfree(src_memb_id);
    H5MM_xfree(dst_memb_id);

    H5MM_xfree(priv->memb_path);
    H5MM_xfree(priv);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_struct_free() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_struct
 *
 * Purpose:     Converts between compound datatypes. This is a soft
 *              conversion function. The algorithm is basically:
 *
 *              For each element do
 *                For I=1..NELMTS do
 *                  If sizeof destination type <= sizeof source type then
 *                    Convert member to destination type;
 *                  Move member as far left as possible;
 *
 *              For I=NELMTS..1 do
 *                If not destination type then
 *                  Convert member to destination type;
 *                Move member to correct position in BKG
 *
 *              Copy BKG to BUF
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_struct(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                 size_t nelmts, size_t buf_stride, size_t bkg_stride, void *_buf, void *_bkg)
{
    uint8_t           *buf  = (uint8_t *)_buf;  /*cast for pointer arithmetic    */
    uint8_t           *bkg  = (uint8_t *)_bkg;  /*background pointer arithmetic    */
    uint8_t           *xbuf = buf, *xbkg = bkg; /*temp pointers into buf and bkg*/
    int               *src2dst  = NULL;         /*maps src member to dst member    */
    H5T_cmemb_t       *src_memb = NULL;         /*source struct member descript.*/
    H5T_cmemb_t       *dst_memb = NULL;         /*destination struct memb desc.    */
    size_t             offset;                  /*byte offset wrt struct    */
    ssize_t            src_delta;               /*source stride    */
    ssize_t            bkg_delta;               /*background stride    */
    size_t             elmtno;
    unsigned           u; /*counters */
    H5T_conv_struct_t *priv         = (H5T_conv_struct_t *)(cdata->priv);
    H5T_conv_ctx_t     tmp_conv_ctx = {0};
    herr_t             ret_value    = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT:
            /*
             * First, determine if this conversion function applies to the
             * conversion path SRC-->DST.  If not, return failure;
             * otherwise initialize the `priv' field of `cdata' with information
             * that remains (almost) constant for this conversion path.
             */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a datatype");
            if (H5T_COMPOUND != src->shared->type)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a H5T_COMPOUND datatype");
            if (H5T_COMPOUND != dst->shared->type)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a H5T_COMPOUND datatype");

            if (H5T__conv_struct_init(src, dst, cdata, conv_ctx) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to initialize conversion data");
            break;

        case H5T_CONV_FREE: {
            /*
             * Free the private conversion data.
             */
            herr_t status = H5T__conv_struct_free(priv);
            cdata->priv   = NULL;
            if (status < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "unable to free private conversion data");

            break;
        }

        case H5T_CONV_CONV:
            /*
             * Conversion.
             */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a datatype");
            if (NULL == conv_ctx)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADVALUE, FAIL, "invalid datatype conversion context pointer");
            assert(priv);
            assert(bkg && cdata->need_bkg);

            /* Initialize temporary conversion context */
            tmp_conv_ctx = *conv_ctx;

            if (cdata->recalc && H5T__conv_struct_init(src, dst, cdata, conv_ctx) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to initialize conversion data");

            /*
             * Insure that members are sorted.
             */
            H5T__sort_value(src, NULL);
            H5T__sort_value(dst, NULL);
            src2dst = priv->src2dst;

            /*
             * Direction of conversion and striding through background.
             */
            if (buf_stride) {
                H5_CHECKED_ASSIGN(src_delta, ssize_t, buf_stride, size_t);
                if (!bkg_stride) {
                    H5_CHECKED_ASSIGN(bkg_delta, ssize_t, dst->shared->size, size_t);
                } /* end if */
                else
                    H5_CHECKED_ASSIGN(bkg_delta, ssize_t, bkg_stride, size_t);
            } /* end if */
            else if (dst->shared->size <= src->shared->size) {
                H5_CHECKED_ASSIGN(src_delta, ssize_t, src->shared->size, size_t);
                H5_CHECKED_ASSIGN(bkg_delta, ssize_t, dst->shared->size, size_t);
            } /* end else-if */
            else {
                H5_CHECK_OVERFLOW(src->shared->size, size_t, ssize_t);
                src_delta = -(ssize_t)src->shared->size;
                H5_CHECK_OVERFLOW(dst->shared->size, size_t, ssize_t);
                bkg_delta = -(ssize_t)dst->shared->size;
                xbuf += (nelmts - 1) * src->shared->size;
                xbkg += (nelmts - 1) * dst->shared->size;
            } /* end else */

            /* Conversion loop... */
            for (elmtno = 0; elmtno < nelmts; elmtno++) {
                /*
                 * For each source member which will be present in the
                 * destination, convert the member to the destination type unless
                 * it is larger than the source type.  Then move the member to the
                 * left-most unoccupied position in the buffer.  This makes the
                 * data point as small as possible with all the free space on the
                 * right side.
                 */
                tmp_conv_ctx.u.conv.recursive = true;
                for (u = 0, offset = 0; u < src->shared->u.compnd.nmembs; u++) {
                    if (src2dst[u] < 0)
                        continue; /*subsetting*/
                    src_memb = src->shared->u.compnd.memb + u;
                    dst_memb = dst->shared->u.compnd.memb + src2dst[u];

                    if (dst_memb->size <= src_memb->size) {
                        /* Update IDs in conversion context */
                        tmp_conv_ctx.u.conv.src_type_id = priv->src_memb_id[u];
                        tmp_conv_ctx.u.conv.dst_type_id = priv->dst_memb_id[src2dst[u]];

                        if (H5T_convert_with_ctx(priv->memb_path[u], priv->src_memb[u],
                                                 priv->dst_memb[src2dst[u]], &tmp_conv_ctx, (size_t)1,
                                                 (size_t)0, (size_t)0, /*no striding (packed array)*/
                                                 xbuf + src_memb->offset, xbkg + dst_memb->offset) < 0)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "unable to convert compound datatype member");

                        memmove(xbuf + offset, xbuf + src_memb->offset, dst_memb->size);
                        offset += dst_memb->size;
                    } /* end if */
                    else {
                        memmove(xbuf + offset, xbuf + src_memb->offset, src_memb->size);
                        offset += src_memb->size;
                    } /* end else */
                }     /* end for */
                tmp_conv_ctx.u.conv.recursive = false;

                /*
                 * For each source member which will be present in the
                 * destination, convert the member to the destination type if it
                 * is larger than the source type (that is, has not been converted
                 * yet).  Then copy the member to the destination offset in the
                 * background buffer.
                 */
                tmp_conv_ctx.u.conv.recursive = true;
                H5_CHECK_OVERFLOW(src->shared->u.compnd.nmembs, size_t, int);
                for (int i = (int)src->shared->u.compnd.nmembs - 1; i >= 0; --i) {
                    if (src2dst[i] < 0)
                        continue; /*subsetting*/
                    src_memb = src->shared->u.compnd.memb + i;
                    dst_memb = dst->shared->u.compnd.memb + src2dst[i];

                    if (dst_memb->size > src_memb->size) {
                        /* Update IDs in conversion context */
                        tmp_conv_ctx.u.conv.src_type_id = priv->src_memb_id[i];
                        tmp_conv_ctx.u.conv.dst_type_id = priv->dst_memb_id[src2dst[i]];

                        offset -= src_memb->size;
                        if (H5T_convert_with_ctx(priv->memb_path[i], priv->src_memb[i],
                                                 priv->dst_memb[src2dst[i]], &tmp_conv_ctx, (size_t)1,
                                                 (size_t)0, (size_t)0, /*no striding (packed array)*/
                                                 xbuf + offset, xbkg + dst_memb->offset) < 0)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "unable to convert compound datatype member");
                    } /* end if */
                    else
                        offset -= dst_memb->size;
                    memcpy(xbkg + dst_memb->offset, xbuf + offset, dst_memb->size);
                } /* end for */
                tmp_conv_ctx.u.conv.recursive = false;

                assert(0 == offset);

                /*
                 * Update pointers
                 */
                xbuf += src_delta;
                xbkg += bkg_delta;
            } /* end for */

            /* If the bkg_delta was set to -(dst->shared->size), make it positive now */
            if (buf_stride == 0 && dst->shared->size > src->shared->size)
                H5_CHECKED_ASSIGN(bkg_delta, ssize_t, dst->shared->size, size_t);

            /*
             * Copy the background buffer back into the in-place conversion
             * buffer.
             */
            for (xbuf = buf, xbkg = bkg, elmtno = 0; elmtno < nelmts; elmtno++) {
                memcpy(xbuf, xbkg, dst->shared->size);
                xbuf += buf_stride ? buf_stride : dst->shared->size;
                xbkg += bkg_delta;
            } /* end for */
            break;

        default:
            /* Some other command we don't know about yet.*/
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_struct() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_struct_opt
 *
 * Purpose:     Converts between compound datatypes in a manner more
 *              efficient than the general-purpose H5T__conv_struct()
 *              function. This function isn't applicable if the destination
 *              is larger than the source type. This is a soft conversion
 *              function. The algorithm is basically:
 *
 *              For each member of the struct
 *                If sizeof destination type <= sizeof source type then
 *                  Convert member to destination type for all elements
 *                  Move memb to BKG buffer for all elements
 *                Else
 *                  Move member as far left as possible for all elements
 *
 *              For each member of the struct (in reverse order)
 *                If not destination type then
 *                  Convert member to destination type for all elements
 *                  Move member to correct position in BKG for all elements
 *
 *              Copy BKG to BUF for all elements
 *
 *              Special case when the source and destination members
 *              are a subset of each other, and the order is the same, and
 *              no conversion is needed. For example:
 *
 *                  struct source {            struct destination {
 *                      TYPE1 A;      -->          TYPE1 A;
 *                      TYPE2 B;      -->          TYPE2 B;
 *                      TYPE3 C;      -->          TYPE3 C;
 *                  };                             TYPE4 D;
 *                                                 TYPE5 E;
 *                                             };
 *
 *              The optimization is simply moving data to the appropriate
 *              places in the buffer.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_struct_opt(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
                     size_t nelmts, size_t buf_stride, size_t bkg_stride, void *_buf, void *_bkg)
{
    uint8_t           *buf      = (uint8_t *)_buf; /*cast for pointer arithmetic    */
    uint8_t           *bkg      = (uint8_t *)_bkg; /*background pointer arithmetic    */
    uint8_t           *xbuf     = NULL;            /*temporary pointer into `buf'    */
    uint8_t           *xbkg     = NULL;            /*temporary pointer into `bkg'    */
    int               *src2dst  = NULL;            /*maps src member to dst member    */
    H5T_cmemb_t       *src_memb = NULL;            /*source struct member descript.*/
    H5T_cmemb_t       *dst_memb = NULL;            /*destination struct memb desc.    */
    size_t             offset;                     /*byte offset wrt struct    */
    size_t             elmtno;                     /*element counter        */
    size_t             copy_size;                  /*size of element for copying   */
    H5T_conv_struct_t *priv         = NULL;        /*private data            */
    H5T_conv_ctx_t     tmp_conv_ctx = {0};         /*temporary conversion context */
    bool               no_stride    = false;       /*flag to indicate no stride    */
    unsigned           u;                          /*counters */
    herr_t             ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT:
            /*
             * First, determine if this conversion function applies to the
             * conversion path SRC-->DST.  If not, return failure;
             * otherwise initialize the `priv' field of `cdata' with information
             * that remains (almost) constant for this conversion path.
             */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (H5T_COMPOUND != src->shared->type)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a H5T_COMPOUND datatype");
            if (H5T_COMPOUND != dst->shared->type)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a H5T_COMPOUND datatype");

            /* Initialize data which is relatively constant */
            if (H5T__conv_struct_init(src, dst, cdata, conv_ctx) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to initialize conversion data");
            priv    = (H5T_conv_struct_t *)(cdata->priv);
            src2dst = priv->src2dst;

            /*
             * If the destination type is not larger than the source type then
             * this conversion function is guaranteed to work (provided all
             * members can be converted also). Otherwise the determination is
             * quite a bit more complicated. Essentially we have to make sure
             * that there is always room in the source buffer to do the
             * conversion of a member in place. This is basically the same pair
             * of loops as in the actual conversion except it checks that there
             * is room for each conversion instead of actually doing anything.
             */
            if (dst->shared->size > src->shared->size) {
                for (u = 0, offset = 0; u < src->shared->u.compnd.nmembs; u++) {
                    if (src2dst[u] < 0)
                        continue;
                    src_memb = src->shared->u.compnd.memb + u;
                    dst_memb = dst->shared->u.compnd.memb + src2dst[u];
                    if (dst_memb->size > src_memb->size)
                        offset += src_memb->size;
                } /* end for */
                H5_CHECK_OVERFLOW(src->shared->u.compnd.nmembs, size_t, int);
                for (int i = (int)src->shared->u.compnd.nmembs - 1; i >= 0; --i) {
                    if (src2dst[i] < 0)
                        continue;
                    src_memb = src->shared->u.compnd.memb + i;
                    dst_memb = dst->shared->u.compnd.memb + src2dst[i];
                    if (dst_memb->size > src_memb->size) {
                        offset -= src_memb->size;
                        if (dst_memb->size > src->shared->size - offset) {
                            H5T__conv_struct_free(priv);
                            cdata->priv = NULL;
                            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                        "conversion is unsupported by this function");
                        } /* end if */
                    }     /* end if */
                }         /* end for */
            }             /* end if */
            break;

        case H5T_CONV_FREE: {
            /*
             * Free the private conversion data.
             */
            herr_t status = H5T__conv_struct_free((H5T_conv_struct_t *)(cdata->priv));
            cdata->priv   = NULL;
            if (status < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "unable to free private conversion data");

            break;
        }

        case H5T_CONV_CONV:
            /*
             * Conversion.
             */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (NULL == conv_ctx)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADVALUE, FAIL, "invalid datatype conversion context pointer");
            if (!bkg)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADVALUE, FAIL, "invalid background buffer pointer");

            /* Initialize temporary conversion context */
            tmp_conv_ctx = *conv_ctx;

            /* Update cached data if necessary */
            if (cdata->recalc && H5T__conv_struct_init(src, dst, cdata, conv_ctx) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to initialize conversion data");
            priv = (H5T_conv_struct_t *)(cdata->priv);
            assert(priv);
            src2dst = priv->src2dst;
            assert(cdata->need_bkg);

            /*
             * Insure that members are sorted.
             */
            H5T__sort_value(src, NULL);
            H5T__sort_value(dst, NULL);

            /*
             * Calculate strides. If BUF_STRIDE is non-zero then convert one
             * data element at every BUF_STRIDE bytes through the main buffer
             * (BUF), leaving the result of each conversion at the same
             * location; otherwise assume the source and destination data are
             * packed tightly based on src->shared->size and dst->shared->size.  Also, if
             * BUF_STRIDE and BKG_STRIDE are both non-zero then place
             * background data into the BKG buffer at multiples of BKG_STRIDE;
             * otherwise assume BKG buffer is the packed destination datatype.
             */
            if (!buf_stride || !bkg_stride)
                bkg_stride = dst->shared->size;
            if (!buf_stride) {
                no_stride  = true;
                buf_stride = src->shared->size;
            } /* end if */

            if (priv->subset_info.subset == H5T_SUBSET_SRC || priv->subset_info.subset == H5T_SUBSET_DST) {
                /* If the optimization flag is set to indicate source members are a subset and
                 * in the top of the destination, simply copy the source members to background buffer.
                 */
                xbuf      = buf;
                xbkg      = bkg;
                copy_size = priv->subset_info.copy_size;

                for (elmtno = 0; elmtno < nelmts; elmtno++) {
                    memcpy(xbkg, xbuf, copy_size);

                    /* Update pointers */
                    xbuf += buf_stride;
                    xbkg += bkg_stride;
                } /* end for */
            }     /* end if */
            else {
                /*
                 * For each member where the destination is not larger than the
                 * source, stride through all the elements converting only that member
                 * in each element and then copying the element to its final
                 * destination in the bkg buffer. Otherwise move the element as far
                 * left as possible in the buffer.
                 */
                tmp_conv_ctx.u.conv.recursive = true;
                for (u = 0, offset = 0; u < src->shared->u.compnd.nmembs; u++) {
                    if (src2dst[u] < 0)
                        continue; /*subsetting*/
                    src_memb = src->shared->u.compnd.memb + u;
                    dst_memb = dst->shared->u.compnd.memb + src2dst[u];

                    if (dst_memb->size <= src_memb->size) {
                        /* Update IDs in conversion context */
                        tmp_conv_ctx.u.conv.src_type_id = priv->src_memb_id[u];
                        tmp_conv_ctx.u.conv.dst_type_id = priv->dst_memb_id[src2dst[u]];

                        xbuf = buf + src_memb->offset;
                        xbkg = bkg + dst_memb->offset;
                        if (H5T_convert_with_ctx(priv->memb_path[u], priv->src_memb[u],
                                                 priv->dst_memb[src2dst[u]], &tmp_conv_ctx, nelmts,
                                                 buf_stride, bkg_stride, xbuf, xbkg) < 0)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "unable to convert compound datatype member");

                        for (elmtno = 0; elmtno < nelmts; elmtno++) {
                            memcpy(xbkg, xbuf, dst_memb->size);
                            xbuf += buf_stride;
                            xbkg += bkg_stride;
                        } /* end for */
                    }     /* end if */
                    else {
                        for (xbuf = buf, elmtno = 0; elmtno < nelmts; elmtno++) {
                            memmove(xbuf + offset, xbuf + src_memb->offset, src_memb->size);
                            xbuf += buf_stride;
                        } /* end for */
                        offset += src_memb->size;
                    } /* end else */
                }     /* end else */
                tmp_conv_ctx.u.conv.recursive = false;

                /*
                 * Work from right to left, converting those members that weren't
                 * converted in the previous loop (those members where the destination
                 * is larger than the source) and them to their final position in the
                 * bkg buffer.
                 */
                tmp_conv_ctx.u.conv.recursive = true;
                H5_CHECK_OVERFLOW(src->shared->u.compnd.nmembs, size_t, int);
                for (int i = (int)src->shared->u.compnd.nmembs - 1; i >= 0; --i) {
                    if (src2dst[i] < 0)
                        continue;
                    src_memb = src->shared->u.compnd.memb + i;
                    dst_memb = dst->shared->u.compnd.memb + src2dst[i];

                    if (dst_memb->size > src_memb->size) {
                        /* Update IDs in conversion context */
                        tmp_conv_ctx.u.conv.src_type_id = priv->src_memb_id[i];
                        tmp_conv_ctx.u.conv.dst_type_id = priv->dst_memb_id[src2dst[i]];

                        offset -= src_memb->size;
                        xbuf = buf + offset;
                        xbkg = bkg + dst_memb->offset;
                        if (H5T_convert_with_ctx(priv->memb_path[i], priv->src_memb[i],
                                                 priv->dst_memb[src2dst[i]], &tmp_conv_ctx, nelmts,
                                                 buf_stride, bkg_stride, xbuf, xbkg) < 0)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                        "unable to convert compound datatype member");
                        for (elmtno = 0; elmtno < nelmts; elmtno++) {
                            memcpy(xbkg, xbuf, dst_memb->size);
                            xbuf += buf_stride;
                            xbkg += bkg_stride;
                        } /* end for */
                    }     /* end if */
                }         /* end for */
                tmp_conv_ctx.u.conv.recursive = false;
            } /* end else */

            if (no_stride)
                buf_stride = dst->shared->size;

            /* Move background buffer into result buffer */
            for (xbuf = buf, xbkg = bkg, elmtno = 0; elmtno < nelmts; elmtno++) {
                memcpy(xbuf, xbkg, dst->shared->size);
                xbuf += buf_stride;
                xbkg += bkg_stride;
            } /* end for */
            break;

        default:
            /* Some other command we don't know about yet.*/
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_struct_opt() */
