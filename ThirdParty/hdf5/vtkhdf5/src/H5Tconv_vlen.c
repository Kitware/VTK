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
 * Purpose: Datatype conversion functions for variable-length datatypes
 */

/****************/
/* Module Setup */
/****************/
#include "H5Tmodule.h" /* This source code file is part of the H5T module */

/***********/
/* Headers */
/***********/
#include "H5CXprivate.h"
#include "H5Eprivate.h"
#include "H5Iprivate.h"
#include "H5Tconv.h"
#include "H5Tconv_vlen.h"

/****************/
/* Local Macros */
/****************/

/* Minimum size of variable-length conversion buffer */
#define H5T_VLEN_MIN_CONF_BUF_SIZE 4096

/******************/
/* Local Typedefs */
/******************/

/********************/
/* Local Prototypes */
/********************/

static herr_t H5T__conv_vlen_nested_free(uint8_t *buf, H5T_t *dt);

/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage pieces of vlen data */
H5FL_BLK_DEFINE_STATIC(vlen_seq);

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_vlen_nested_free
 *
 * Purpose:     Recursively locates and frees any nested VLEN components of
 *              complex data types (including COMPOUND).
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T__conv_vlen_nested_free(uint8_t *buf, H5T_t *dt)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (dt->shared->type) {
        case H5T_VLEN:
            /* Pointer buf refers to VLEN data; free it (always reset tmp) */
            if ((*(dt->shared->u.vlen.cls->del))(dt->shared->u.vlen.file, buf) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "can't free nested vlen");
            break;

        case H5T_COMPOUND:
            /* Pointer buf refers to COMPOUND data; recurse for each member. */
            for (unsigned i = 0; i < dt->shared->u.compnd.nmembs; ++i)
                if (H5T__conv_vlen_nested_free(buf + dt->shared->u.compnd.memb[i].offset,
                                               dt->shared->u.compnd.memb[i].type) < 0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "can't free compound member");
            break;

        case H5T_ARRAY:
            /* Pointer buf refers to ARRAY data; recurse for each element. */
            for (unsigned i = 0; i < dt->shared->u.array.nelem; ++i)
                if (H5T__conv_vlen_nested_free(buf + i * dt->shared->parent->shared->size,
                                               dt->shared->parent) < 0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "can't free array data");
            break;

        case H5T_INTEGER:
        case H5T_FLOAT:
        case H5T_TIME:
        case H5T_STRING:
        case H5T_BITFIELD:
        case H5T_OPAQUE:
        case H5T_REFERENCE:
        case H5T_ENUM:
            /* These types cannot contain vl data */
            break;

        case H5T_NO_CLASS:
        case H5T_NCLASSES:
        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "invalid datatype class");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5T__conv_vlen_nested_free() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_vlen
 *
 * Purpose:     Converts between VL datatypes in memory and on disk.
 *              This is a soft conversion function. The algorithm is
 *              basically:
 *
 *              For every VL struct in the main buffer:
 *              1. Allocate space for temporary dst VL data (reuse buffer
 *                 if possible)
 *                    2. Copy VL data from src buffer into dst buffer
 *                    3. Convert VL data into dst representation
 *                    4. Allocate buffer in dst heap
 *              5. Free heap objects storing old data
 *                    6. Write dst VL data into dst heap
 *                    7. Store (heap ID or pointer) and length in main dst buffer
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_vlen(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata, const H5T_conv_ctx_t *conv_ctx,
               size_t nelmts, size_t buf_stride, size_t bkg_stride, void *buf, void *bkg)
{
    H5T_vlen_alloc_info_t vl_alloc_info;         /* VL allocation info */
    H5T_conv_ctx_t        tmp_conv_ctx  = {0};   /* Temporary conversion context */
    H5T_path_t           *tpath         = NULL;  /* Type conversion path             */
    bool                  noop_conv     = false; /* Flag to indicate a noop conversion */
    bool                  write_to_file = false; /* Flag to indicate writing to file */
    htri_t                parent_is_vlen;        /* Flag to indicate parent is vlen datatype */
    size_t                bg_seq_len = 0;        /* The number of elements in the background sequence */
    H5T_t                *tsrc_cpy   = NULL;     /* Temporary copy of source base datatype */
    H5T_t                *tdst_cpy   = NULL;     /* Temporary copy of destination base datatype */
    hid_t                 tsrc_id    = H5I_INVALID_HID; /* Temporary type atom */
    hid_t                 tdst_id    = H5I_INVALID_HID; /* Temporary type atom */
    uint8_t              *s          = NULL;            /* Source buffer            */
    uint8_t              *d          = NULL;            /* Destination buffer        */
    uint8_t              *b          = NULL;            /* Background buffer        */
    ssize_t               s_stride   = 0;               /* Src stride */
    ssize_t               d_stride   = 0;               /* Dst stride */
    ssize_t               b_stride;                     /* Bkg stride            */
    size_t                safe = 0;              /* How many elements are safe to process in each pass */
    size_t                src_base_size;         /* Source base size*/
    size_t                dst_base_size;         /* Destination base size*/
    void                 *conv_buf      = NULL;  /* Temporary conversion buffer          */
    size_t                conv_buf_size = 0;     /* Size of conversion buffer in bytes */
    void                 *tmp_buf       = NULL;  /* Temporary background buffer          */
    size_t                tmp_buf_size  = 0;     /* Size of temporary bkg buffer         */
    bool                  nested        = false; /* Flag of nested VL case             */
    size_t                elmtno        = 0;     /* Element number counter               */
    size_t                orig_d_stride = 0;     /* Original destination stride (used for error handling) */
    size_t orig_nelmts = nelmts; /* Original # of elements to convert (used for error handling) */
    bool   convert_forward =
        true; /* Current direction of conversion (forward or backward, used for error handling) */
    bool conversions_made =
        false; /* Flag to indicate conversions have been performed, used for error handling */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT:
            /*
             * First, determine if this conversion function applies to the
             * conversion path SRC_ID-->DST_ID.  If not, return failure;
             * otherwise initialize the `priv' field of `cdata' with
             * information that remains (almost) constant for this
             * conversion path.
             */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a datatype");
            if (H5T_VLEN != src->shared->type)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a H5T_VLEN datatype");
            if (H5T_VLEN != dst->shared->type)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a H5T_VLEN datatype");
            if (H5T_VLEN_STRING == src->shared->u.vlen.type && H5T_VLEN_STRING == dst->shared->u.vlen.type) {
                if ((H5T_CSET_ASCII == src->shared->u.vlen.cset &&
                     H5T_CSET_UTF8 == dst->shared->u.vlen.cset) ||
                    (H5T_CSET_ASCII == dst->shared->u.vlen.cset && H5T_CSET_UTF8 == src->shared->u.vlen.cset))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                                "The library doesn't convert between strings of ASCII and UTF");
            } /* end if */

            /* Variable-length types don't need a background buffer */
            cdata->need_bkg = H5T_BKG_NO;

            break;

        case H5T_CONV_FREE:
            /* QAK - Nothing to do currently */
            break;

        case H5T_CONV_CONV:
            /*
             * Conversion.
             */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (NULL == conv_ctx)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADVALUE, FAIL, "invalid datatype conversion context pointer");

            /* Initialize temporary conversion context */
            tmp_conv_ctx = *conv_ctx;

            /* Initialize source & destination strides */
            if (buf_stride) {
                assert(buf_stride >= src->shared->size);
                assert(buf_stride >= dst->shared->size);
                H5_CHECK_OVERFLOW(buf_stride, size_t, ssize_t);
                s_stride = d_stride = (ssize_t)buf_stride;
            } /* end if */
            else {
                H5_CHECK_OVERFLOW(src->shared->size, size_t, ssize_t);
                H5_CHECK_OVERFLOW(dst->shared->size, size_t, ssize_t);
                s_stride = (ssize_t)src->shared->size;
                d_stride = (ssize_t)dst->shared->size;
            } /* end else */
            if (bkg) {
                if (bkg_stride)
                    b_stride = (ssize_t)bkg_stride;
                else
                    b_stride = d_stride;
            } /* end if */
            else
                b_stride = 0;

            /* Get the size of the base types in src & dst */
            src_base_size = H5T_get_size(src->shared->parent);
            dst_base_size = H5T_get_size(dst->shared->parent);

            /* Set up conversion path for base elements */
            if (NULL == (tpath = H5T_path_find(src->shared->parent, dst->shared->parent)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL,
                            "unable to convert between src and dest datatypes");
            else if (!H5T_path_noop(tpath)) {
                if (NULL == (tsrc_cpy = H5T_copy(src->shared->parent, H5T_COPY_ALL)))
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, FAIL,
                                "unable to copy src base type for conversion");
                /* References need to know about the src file */
                if (tsrc_cpy->shared->type == H5T_REFERENCE)
                    if (H5T_set_loc(tsrc_cpy, src->shared->u.vlen.file, src->shared->u.vlen.loc) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTSET, FAIL, "can't set datatype location");

                if (NULL == (tdst_cpy = H5T_copy(dst->shared->parent, H5T_COPY_ALL)))
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, FAIL,
                                "unable to copy dst base type for conversion");
                /* References need to know about the dst file */
                if (tdst_cpy->shared->type == H5T_REFERENCE)
                    if (H5T_set_loc(tdst_cpy, dst->shared->u.vlen.file, dst->shared->u.vlen.loc) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTSET, FAIL, "can't set datatype location");

                /* Create IDs for the variable-length base datatypes if the conversion path
                 * uses an application conversion function or if a conversion exception function
                 * was provided.
                 */
                if (tpath->conv.is_app || conv_ctx->u.conv.cb_struct.func) {
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
            } /* end else-if */
            else
                noop_conv = true;

            /* Check if we need a temporary buffer for this conversion */
            if ((parent_is_vlen = H5T_detect_class(dst->shared->parent, H5T_VLEN, false)) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_SYSTEM, FAIL,
                            "internal error when detecting variable-length class");
            if (tpath->cdata.need_bkg || parent_is_vlen) {
                /* Set up initial background buffer */
                tmp_buf_size = MAX(src_base_size, dst_base_size);
                if (NULL == (tmp_buf = H5FL_BLK_CALLOC(vlen_seq, tmp_buf_size)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL,
                                "memory allocation failed for type conversion");
            } /* end if */

            /* Get the allocation info */
            if (H5CX_get_vlen_alloc_info(&vl_alloc_info) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "unable to retrieve VL allocation info");

            /* Set flags to indicate we are writing to or reading from the file */
            if (dst->shared->u.vlen.file != NULL)
                write_to_file = true;

            /* Set the flag for nested VL case */
            if (write_to_file && parent_is_vlen && bkg != NULL)
                nested = true;

            /* Save info for unraveling on errors */
            orig_d_stride   = (size_t)d_stride;
            convert_forward = !(d_stride > s_stride);

            /* The outer loop of the type conversion macro, controlling which */
            /* direction the buffer is walked */
            while (nelmts > 0) {
                /* Check if we need to go backwards through the buffer */
                if (d_stride > s_stride) {
                    /* Sanity check */
                    assert(s_stride > 0);
                    assert(d_stride > 0);
                    assert(b_stride >= 0);

                    /* Compute the number of "safe" destination elements at */
                    /* the end of the buffer (Those which don't overlap with */
                    /* any source elements at the beginning of the buffer) */
                    safe =
                        nelmts - (((nelmts * (size_t)s_stride) + ((size_t)d_stride - 1)) / (size_t)d_stride);

                    /* If we're down to the last few elements, just wrap up */
                    /* with a "real" reverse copy */
                    if (safe < 2) {
                        s = (uint8_t *)buf + (nelmts - 1) * (size_t)s_stride;
                        d = (uint8_t *)buf + (nelmts - 1) * (size_t)d_stride;
                        if (bkg)
                            b = (uint8_t *)bkg + (nelmts - 1) * (size_t)b_stride;
                        s_stride = -s_stride;
                        d_stride = -d_stride;
                        b_stride = -b_stride;

                        safe = nelmts;
                    } /* end if */
                    else {
                        s = (uint8_t *)buf + (nelmts - safe) * (size_t)s_stride;
                        d = (uint8_t *)buf + (nelmts - safe) * (size_t)d_stride;
                        if (bkg)
                            b = (uint8_t *)bkg + (nelmts - safe) * (size_t)b_stride;
                    } /* end else */
                }     /* end if */
                else {
                    /* Single forward pass over all data */
                    s = d = (uint8_t *)buf;
                    b     = (uint8_t *)bkg;
                    safe  = nelmts;
                } /* end else */

                for (elmtno = 0; elmtno < safe; elmtno++) {
                    bool is_nil; /* Whether sequence is "nil" */

                    /* Check for "nil" source sequence */
                    if ((*(src->shared->u.vlen.cls->isnull))(src->shared->u.vlen.file, s, &is_nil) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't check if VL data is 'nil'");
                    else if (is_nil) {
                        /* Write "nil" sequence to destination location */
                        if ((*(dst->shared->u.vlen.cls->setnull))(dst->shared->u.vlen.file, d, b) < 0)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_WRITEERROR, FAIL, "can't set VL data to 'nil'");
                    } /* end else-if */
                    else {
                        size_t seq_len; /* The number of elements in the current sequence */

                        /* Get length of element sequences */
                        if ((*(src->shared->u.vlen.cls->getlen))(src->shared->u.vlen.file, s, &seq_len) < 0)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "bad sequence length");

                        /* If we are reading from memory and there is no conversion, just get the pointer to
                         * sequence */
                        if (write_to_file && noop_conv) {
                            /* Get direct pointer to sequence */
                            if (NULL == (conv_buf = (*(src->shared->u.vlen.cls->getptr))(s)))
                                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid source pointer");
                        } /* end if */
                        else {
                            size_t src_size, dst_size; /*source & destination total size in bytes*/

                            src_size = seq_len * src_base_size;
                            dst_size = seq_len * dst_base_size;

                            /* Check if conversion buffer is large enough, resize if
                             * necessary.  If the SEQ_LEN is 0, allocate a minimal size buffer.
                             */
                            if (!seq_len && !conv_buf) {
                                conv_buf_size = H5T_VLEN_MIN_CONF_BUF_SIZE;
                                if (NULL == (conv_buf = H5FL_BLK_CALLOC(vlen_seq, conv_buf_size)))
                                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                                                "memory allocation failed for type conversion");
                            } /* end if */
                            else if (conv_buf_size < MAX(src_size, dst_size)) {
                                /* Only allocate conversion buffer in H5T_VLEN_MIN_CONF_BUF_SIZE increments */
                                conv_buf_size = ((MAX(src_size, dst_size) / H5T_VLEN_MIN_CONF_BUF_SIZE) + 1) *
                                                H5T_VLEN_MIN_CONF_BUF_SIZE;
                                if (NULL == (conv_buf = H5FL_BLK_REALLOC(vlen_seq, conv_buf, conv_buf_size)))
                                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                                                "memory allocation failed for type conversion");
                                memset(conv_buf, 0, conv_buf_size);
                            } /* end else-if */

                            /* Read in VL sequence */
                            if ((*(src->shared->u.vlen.cls->read))(src->shared->u.vlen.file, s, conv_buf,
                                                                   src_size) < 0)
                                HGOTO_ERROR(H5E_DATATYPE, H5E_READERROR, FAIL, "can't read VL data");
                        } /* end else */

                        if (!noop_conv) {
                            /* Check if temporary buffer is large enough, resize if necessary */
                            /* (Chain off the conversion buffer size) */
                            if (tmp_buf && tmp_buf_size < conv_buf_size) {
                                /* Set up initial background buffer */
                                tmp_buf_size = conv_buf_size;
                                if (NULL == (tmp_buf = H5FL_BLK_REALLOC(vlen_seq, tmp_buf, tmp_buf_size)))
                                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                                                "memory allocation failed for type conversion");
                                memset(tmp_buf, 0, tmp_buf_size);
                            } /* end if */

                            /* If we are writing and there is a nested VL type, read
                             * the sequence into the background buffer */
                            if (nested) {
                                /* Sanity check */
                                assert(write_to_file);

                                /* Get length of background element sequence */
                                if ((*(dst->shared->u.vlen.cls->getlen))(dst->shared->u.vlen.file, b,
                                                                         &bg_seq_len) < 0)
                                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "bad sequence length");

                                /* Read sequence if length > 0 */
                                if (bg_seq_len > 0) {
                                    if (tmp_buf_size < (bg_seq_len * MAX(src_base_size, dst_base_size))) {
                                        tmp_buf_size = (bg_seq_len * MAX(src_base_size, dst_base_size));
                                        if (NULL ==
                                            (tmp_buf = H5FL_BLK_REALLOC(vlen_seq, tmp_buf, tmp_buf_size)))
                                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                                                        "memory allocation failed for type conversion");
                                        memset(tmp_buf, 0, tmp_buf_size);
                                    } /* end if */

                                    /* Read in background VL sequence */
                                    if ((*(dst->shared->u.vlen.cls->read))(dst->shared->u.vlen.file, b,
                                                                           tmp_buf,
                                                                           bg_seq_len * dst_base_size) < 0)
                                        HGOTO_ERROR(H5E_DATATYPE, H5E_READERROR, FAIL, "can't read VL data");
                                } /* end if */

                                /* If the sequence gets shorter, pad out the original sequence with zeros */
                                if (bg_seq_len < seq_len)
                                    memset((uint8_t *)tmp_buf + dst_base_size * bg_seq_len, 0,
                                           (seq_len - bg_seq_len) * dst_base_size);
                            } /* end if */

                            /* Convert VL sequence */
                            tmp_conv_ctx.u.conv.recursive = true;
                            if (H5T_convert_with_ctx(tpath, tsrc_cpy, tdst_cpy, &tmp_conv_ctx, seq_len,
                                                     (size_t)0, (size_t)0, conv_buf, tmp_buf) < 0)
                                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCONVERT, FAIL,
                                            "datatype conversion failed");
                            tmp_conv_ctx.u.conv.recursive = false;
                        } /* end if */

                        /* Write sequence to destination location */
                        if ((*(dst->shared->u.vlen.cls->write))(dst->shared->u.vlen.file, &vl_alloc_info, d,
                                                                conv_buf, b, seq_len, dst_base_size) < 0)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_WRITEERROR, FAIL, "can't write VL data");

                        if (!noop_conv) {
                            /* For nested VL case, free leftover heap objects from the deeper level if the
                             * length of new data elements is shorter than the old data elements.*/
                            if (nested && seq_len < bg_seq_len) {
                                uint8_t *tmp;
                                size_t   u;

                                /* Sanity check */
                                assert(write_to_file);

                                tmp = (uint8_t *)tmp_buf + seq_len * dst_base_size;
                                for (u = seq_len; u < bg_seq_len; u++, tmp += dst_base_size) {
                                    /* Recursively free destination data */
                                    if (H5T__conv_vlen_nested_free(tmp, dst->shared->parent) < 0)
                                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREMOVE, FAIL,
                                                    "unable to remove heap object");
                                } /* end for */
                            }     /* end if */
                        }         /* end if */
                    }             /* end else */

                    /* Indicate that elements have been converted, in case of error */
                    conversions_made = true;

                    /* Advance pointers */
                    s += s_stride;
                    d += d_stride;

                    if (b)
                        b += b_stride;
                } /* end for */

                /* Decrement number of elements left to convert */
                nelmts -= safe;
            } /* end while */

            break;

        default: /* Some other command we don't know about yet.*/
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    } /* end switch */

done:
    /* Release converted elements on error */
    if (ret_value < 0 && conversions_made) {
        size_t dest_count;

        /* Set up for first pass to destroy references */
        if (nelmts < orig_nelmts || (convert_forward && elmtno < safe)) {
            dest_count = orig_nelmts - nelmts;

            /* Set pointer to correct location, based on direction chosen */
            if (convert_forward) {
                d = (uint8_t *)buf;
                dest_count += elmtno; /* Include partial iteration in first pass, for forward conversions */
            }
            else
                d = (uint8_t *)buf + (nelmts * orig_d_stride);

            /* Destroy vlen elements that have already been converted */
            while (dest_count > 0) {
                H5T_vlen_reclaim_elmt(d, dst); /* Ignore errors at this point */
                d += orig_d_stride;
                dest_count--;
            }
        }

        /* Do any remaining partial iteration, if converting backwards */
        if (!convert_forward && elmtno < safe) {
            dest_count = elmtno;

            /* Set pointer to correct location */
            if (d_stride > 0)
                d = (uint8_t *)buf + ((nelmts - safe) * orig_d_stride);
            else
                d = (uint8_t *)buf + ((nelmts - elmtno) * orig_d_stride);

            /* Destroy references that have already been converted */
            while (dest_count > 0) {
                H5T_vlen_reclaim_elmt(d, dst); /* Ignore errors at this point */
                d += orig_d_stride;
                dest_count--;
            }
        }
    }

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

    /* If the conversion buffer doesn't need to be freed, reset its pointer */
    if (write_to_file && noop_conv)
        conv_buf = NULL;
    /* Release the conversion buffer (always allocated, except on errors) */
    if (conv_buf)
        conv_buf = H5FL_BLK_FREE(vlen_seq, conv_buf);
    /* Release the background buffer, if we have one */
    if (tmp_buf)
        tmp_buf = H5FL_BLK_FREE(vlen_seq, tmp_buf);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_vlen() */
