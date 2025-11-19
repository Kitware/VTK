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
 * Purpose: Datatype conversion functions for array datatypes
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
#include "H5Tconv_array.h"

/******************/
/* Local Typedefs */
/******************/

/* Private conversion data for array datatypes */
typedef struct H5T_conv_array_t {
    H5T_path_t *tpath; /* Conversion path for parent types */
} H5T_conv_array_t;

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_array
 *
 * Purpose:     Converts between array datatypes in memory and on disk.
 *              This is a soft conversion function.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_array(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                const H5T_conv_ctx_t H5_ATTR_UNUSED *conv_ctx, size_t nelmts, size_t buf_stride,
                size_t bkg_stride, void *_buf, void *_bkg)
{
    H5T_conv_array_t *priv         = NULL;             /* Private conversion data */
    H5T_conv_ctx_t    tmp_conv_ctx = {0};              /* Temporary conversion context */
    H5T_t            *tsrc_cpy     = NULL;             /* Temporary copy of source base datatype */
    H5T_t            *tdst_cpy     = NULL;             /* Temporary copy of destination base datatype */
    hid_t             tsrc_id      = H5I_INVALID_HID;  /* Temporary type atom */
    hid_t             tdst_id      = H5I_INVALID_HID;  /* Temporary type atom */
    uint8_t          *sp, *dp, *bp;                    /* Source, dest, and bkg traversal ptrs */
    ssize_t           src_delta, dst_delta, bkg_delta; /* Source, dest, and bkg strides */
    int               direction;                       /* Direction of traversal */
    herr_t            ret_value = SUCCEED;             /* Return value */

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT:
            /*
             * First, determine if this conversion function applies to the
             * conversion path SRC-->DST.  If not, return failure;
             * otherwise initialize the `priv' field of `cdata' with
             * information that remains (almost) constant for this
             * conversion path.
             */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            assert(H5T_ARRAY == src->shared->type);
            assert(H5T_ARRAY == dst->shared->type);

            /* Check the number and sizes of the dimensions */
            if (src->shared->u.array.ndims != dst->shared->u.array.ndims)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                            "array datatypes do not have the same number of dimensions");
            for (unsigned u = 0; u < src->shared->u.array.ndims; u++)
                if (src->shared->u.array.dim[u] != dst->shared->u.array.dim[u])
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                "array datatypes do not have the same sizes of dimensions");

            /* Initialize parent type conversion if necessary. We need to do this here because we need to
             * report whether we need a background buffer or not. */
            if (!cdata->priv) {
                /* Allocate private data */
                if (NULL == (priv = (H5T_conv_array_t *)(cdata->priv = calloc(1, sizeof(*priv)))))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");

                /* Find conversion path between parent types */
                if (NULL == (priv->tpath = H5T_path_find(src->shared->parent, dst->shared->parent))) {
                    free(priv);
                    cdata->priv = NULL;
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                                "unable to convert between src and dest datatype");
                }

                /* Array datatypes don't need a background buffer by themselves, but the parent type might.
                 * Pass the need_bkg field through to the upper layer. */
                cdata->need_bkg = priv->tpath->cdata.need_bkg;
            }

            break;

        case H5T_CONV_FREE:
            /*
             * Free private data
             */
            free(cdata->priv);
            cdata->priv = NULL;

            break;

        case H5T_CONV_CONV:
            /*
             * Conversion.
             */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (NULL == conv_ctx)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADVALUE, FAIL, "invalid datatype conversion context pointer");
            priv = (H5T_conv_array_t *)cdata->priv;

            /* Initialize temporary conversion context */
            tmp_conv_ctx = *conv_ctx;

            /*
             * Do we process the values from beginning to end or vice
             * versa? Also, how many of the elements have the source and
             * destination areas overlapping?
             */
            if (src->shared->size >= dst->shared->size || buf_stride > 0) {
                sp = dp   = (uint8_t *)_buf;
                bp        = _bkg;
                direction = 1;
            }
            else {
                sp = (uint8_t *)_buf + (nelmts - 1) * (buf_stride ? buf_stride : src->shared->size);
                dp = (uint8_t *)_buf + (nelmts - 1) * (buf_stride ? buf_stride : dst->shared->size);
                bp = _bkg ? (uint8_t *)_bkg + (nelmts - 1) * (bkg_stride ? bkg_stride : dst->shared->size)
                          : NULL;
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
            bkg_delta = (ssize_t)direction * (ssize_t)(bkg_stride ? bkg_stride : dst->shared->size);

            /* Set up conversion path for base elements */
            if (!H5T_path_noop(priv->tpath)) {
                if (NULL == (tsrc_cpy = H5T_copy(src->shared->parent, H5T_COPY_ALL)))
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, FAIL,
                                "unable to copy src base type for conversion");

                if (NULL == (tdst_cpy = H5T_copy(dst->shared->parent, H5T_COPY_ALL)))
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, FAIL,
                                "unable to copy dst base type for conversion");

                /* Create IDs for the array base datatypes if the conversion path uses an
                 * application conversion function or if a conversion exception function
                 * was provided.
                 */
                if (priv->tpath->conv.is_app || conv_ctx->u.conv.cb_struct.func) {
                    if ((tsrc_id = H5I_register(H5I_DATATYPE, tsrc_cpy, false)) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL,
                                    "unable to register ID for source base datatype");
                    if ((tdst_id = H5I_register(H5I_DATATYPE, tdst_cpy, false)) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL,
                                    "unable to register ID for destination base datatype");
                }

                /* Update IDs in conversion context */
                tmp_conv_ctx.u.conv.src_type_id = tsrc_id;
                tmp_conv_ctx.u.conv.dst_type_id = tdst_id;
            }

            /* Perform the actual conversion */
            tmp_conv_ctx.u.conv.recursive = true;
            for (size_t elmtno = 0; elmtno < nelmts; elmtno++) {
                /* Copy the source array into the correct location for the destination */
                memmove(dp, sp, src->shared->size);

                /* Convert array */
                if (H5T_convert_with_ctx(priv->tpath, tsrc_cpy, tdst_cpy, &tmp_conv_ctx,
                                         src->shared->u.array.nelem, (size_t)0, (size_t)0, dp, bp) < 0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL, "datatype conversion failed");

                /* Advance the source, destination, and background pointers */
                sp += src_delta;
                dp += dst_delta;
                if (bp)
                    bp += bkg_delta;
            } /* end for */
            tmp_conv_ctx.u.conv.recursive = false;

            break;

        default: /* Some other command we don't know about yet.*/
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    } /* end switch */

done:
    if (tsrc_id >= 0) {
        if (H5I_dec_ref(tsrc_id) < 0)
            HDONE_ERROR(H5E_DATATYPE, H5E_CANTDEC, FAIL, "can't decrement reference on temporary ID");
    }
    else if (tsrc_cpy) {
        if (H5T_close(tsrc_cpy) < 0)
            HDONE_ERROR(H5E_DATATYPE, H5E_CANTCLOSEOBJ, FAIL, "can't close temporary datatype");
    }
    if (tdst_id >= 0) {
        if (H5I_dec_ref(tdst_id) < 0)
            HDONE_ERROR(H5E_DATATYPE, H5E_CANTDEC, FAIL, "can't decrement reference on temporary ID");
    }
    else if (tdst_cpy) {
        if (H5T_close(tdst_cpy) < 0)
            HDONE_ERROR(H5E_DATATYPE, H5E_CANTCLOSEOBJ, FAIL, "can't close temporary datatype");
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_array() */
