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
 * Purpose: Datatype conversion functions for reference datatypes
 */

/****************/
/* Module Setup */
/****************/
#include "H5Tmodule.h" /* This source code file is part of the H5T module */
#define H5R_FRIEND     /* Suppress error about including H5Rpkg */

/***********/
/* Headers */
/***********/
#include "H5Eprivate.h"
#include "H5FLprivate.h"
#include "H5Rpkg.h"
#include "H5Tconv.h"
#include "H5Tconv_reference.h"

/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage pieces of reference data */
H5FL_BLK_DEFINE_STATIC(ref_seq);

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_ref
 *
 * Purpose:     Converts between reference datatypes in memory and on disk.
 *              This is a soft conversion function.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_ref(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
              const H5T_conv_ctx_t H5_ATTR_UNUSED *conv_ctx, size_t nelmts, size_t buf_stride,
              size_t bkg_stride, void *buf, void *bkg)
{
    uint8_t *s        = NULL;        /* source buffer                        */
    uint8_t *d        = NULL;        /* destination buffer                   */
    uint8_t *b        = NULL;        /* background buffer                    */
    ssize_t  s_stride = 0;           /* src stride                  */
    ssize_t  d_stride = 0;           /* dst stride                  */
    ssize_t  b_stride;               /* bkg stride                           */
    size_t   safe          = 0;      /* how many elements are safe to process in each pass */
    void    *conv_buf      = NULL;   /* temporary conversion buffer          */
    size_t   conv_buf_size = 0;      /* size of conversion buffer in bytes   */
    size_t   elmtno        = 0;      /* element number counter               */
    size_t   orig_d_stride = 0;      /* Original destination stride (used for error handling) */
    size_t   orig_nelmts   = nelmts; /* Original # of elements to convert (used for error handling) */
    bool     convert_forward =
        true; /* Current direction of conversion (forward or backward, used for error handling) */
    bool conversions_made =
        false; /* Flag to indicate conversions have been performed, used for error handling */
    herr_t ret_value = SUCCEED; /* return value                         */

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
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a datatype");
            if (H5T_REFERENCE != src->shared->type)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a H5T_REFERENCE datatype");
            if (H5T_REFERENCE != dst->shared->type)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a H5T_REFERENCE datatype");
            /* Only allow for source reference that is not an opaque type, destination must be opaque */
            if (!dst->shared->u.atomic.u.r.opaque)
                HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not an H5T_STD_REF datatype");

            /* Reference types don't need a background buffer */
            cdata->need_bkg = H5T_BKG_NO;
            break;

        case H5T_CONV_FREE:
            break;

        case H5T_CONV_CONV: {
            /*
             * Conversion.
             */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");

            assert(src->shared->u.atomic.u.r.cls);

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
                    size_t buf_size;
                    bool   dst_copy = false;
                    bool   is_nil; /* Whether reference is "nil" */

                    /* Check for "nil" source reference */
                    if ((*(src->shared->u.atomic.u.r.cls->isnull))(src->shared->u.atomic.u.r.file, s,
                                                                   &is_nil) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL,
                                    "can't check if reference data is 'nil'");

                    if (is_nil) {
                        /* Write "nil" reference to destination location */
                        if ((*(dst->shared->u.atomic.u.r.cls->setnull))(dst->shared->u.atomic.u.r.file, d,
                                                                        b) < 0)
                            HGOTO_ERROR(H5E_DATATYPE, H5E_WRITEERROR, FAIL,
                                        "can't set reference data to 'nil'");
                    } /* end else-if */
                    else {
                        /* Get size of references */
                        if (0 == (buf_size = src->shared->u.atomic.u.r.cls->getsize(
                                      src->shared->u.atomic.u.r.file, s, src->shared->size,
                                      dst->shared->u.atomic.u.r.file, &dst_copy)))
                            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "unable to obtain size of reference");

                        /* Check if conversion buffer is large enough, resize if necessary. */
                        if (conv_buf_size < buf_size) {
                            conv_buf_size = buf_size;
                            if (NULL == (conv_buf = H5FL_BLK_REALLOC(ref_seq, conv_buf, conv_buf_size)))
                                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                                            "memory allocation failed for type conversion");
                            memset(conv_buf, 0, conv_buf_size);
                        } /* end if */

                        if (dst_copy && (src->shared->u.atomic.u.r.loc == H5T_LOC_DISK))
                            H5MM_memcpy(conv_buf, s, buf_size);
                        else {
                            /* Read reference */
                            if (src->shared->u.atomic.u.r.cls->read(
                                    src->shared->u.atomic.u.r.file, s, src->shared->size,
                                    dst->shared->u.atomic.u.r.file, conv_buf, buf_size) < 0)
                                HGOTO_ERROR(H5E_DATATYPE, H5E_READERROR, FAIL, "can't read reference data");
                        } /* end else */

                        if (dst_copy && (dst->shared->u.atomic.u.r.loc == H5T_LOC_DISK))
                            H5MM_memcpy(d, conv_buf, buf_size);
                        else {
                            /* Write reference to destination location */
                            if (dst->shared->u.atomic.u.r.cls->write(
                                    src->shared->u.atomic.u.r.file, conv_buf, buf_size,
                                    src->shared->u.atomic.u.r.rtype, dst->shared->u.atomic.u.r.file, d,
                                    dst->shared->size, b) < 0)
                                HGOTO_ERROR(H5E_DATATYPE, H5E_WRITEERROR, FAIL, "can't write reference data");
                        } /* end else */
                    }     /* end else */

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
        }     /* end case */
        break;

        default: /* Some other command we don't know about yet.*/
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    } /* end switch */

done:
    /* Release converted elements on error */
    if (ret_value < 0 && conversions_made) {
        H5R_ref_priv_t ref_priv;
        size_t         dest_count;

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

            /* Destroy references that have already been converted */
            while (dest_count > 0) {
                memcpy(&ref_priv, d, sizeof(H5R_ref_priv_t));
                H5R__destroy(&ref_priv); /* Ignore errors at this point */
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
                memcpy(&ref_priv, d, sizeof(H5R_ref_priv_t));
                H5R__destroy(&ref_priv); /* Ignore errors at this point */
                d += orig_d_stride;
                dest_count--;
            }
        }
    }

    /* Release the conversion buffer (always allocated, except on errors) */
    if (conv_buf)
        conv_buf = H5FL_BLK_FREE(ref_seq, conv_buf);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_ref() */
