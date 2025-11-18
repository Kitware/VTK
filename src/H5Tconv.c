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
 * Module Info: General datatype conversion and conversion-related functions
 *              for the H5T interface. Conversion functions for specific
 *              datatype classes are in separate files.
 */

/****************/
/* Module Setup */
/****************/

#include "H5Tmodule.h" /* This source code file is part of the H5T module */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions                        */
#include "H5CXprivate.h" /* API Contexts                             */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5Tpkg.h"      /* Datatypes                                */

/****************/
/* Local Macros */
/****************/

/* Swap two elements (I & J) of an array using a temporary variable */
#define H5_SWAP_BYTES(ARRAY, I, J)                                                                           \
    do {                                                                                                     \
        uint8_t _tmp;                                                                                        \
        _tmp     = ARRAY[I];                                                                                 \
        ARRAY[I] = ARRAY[J];                                                                                 \
        ARRAY[J] = _tmp;                                                                                     \
    } while (0)

/******************/
/* Local Typedefs */
/******************/

/********************/
/* Package Typedefs */
/********************/

/********************/
/* Local Prototypes */
/********************/

/*********************/
/* Public Variables */
/*********************/

/*********************/
/* Package Variables */
/*********************/

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/*-------------------------------------------------------------------------
 * Function:    H5T_reclaim
 *
 * Purpose:     Frees the buffers allocated for storing variable-length
 *              data in memory. Only frees the VL data in the selection
 *              defined in the
 *              dataspace.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_reclaim(const H5T_t *type, H5S_t *space, void *buf)
{
    H5S_sel_iter_op_t     dset_op;          /* Operator for iteration */
    H5T_vlen_alloc_info_t vl_alloc_info;    /* VL allocation info */
    herr_t                ret_value = FAIL; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    assert(type);
    assert(space);
    assert(buf);

    /* Get the allocation info */
    if (H5CX_get_vlen_alloc_info(&vl_alloc_info) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "unable to retrieve VL allocation info");

    /* Call H5S_select_iterate with args, etc. */
    dset_op.op_type  = H5S_SEL_ITER_OP_LIB;
    dset_op.u.lib_op = H5T_reclaim_cb;

    ret_value = H5S_select_iterate(buf, type, space, &dset_op, &vl_alloc_info);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_reclaim() */

/*-------------------------------------------------------------------------
 * Function:    H5T_reclaim_cb
 *
 * Purpose:     Iteration callback to reclaim conversion allocated memory
 *              for a buffer element.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_reclaim_cb(void *elem, const H5T_t *dt, unsigned H5_ATTR_UNUSED ndim, const hsize_t H5_ATTR_UNUSED *point,
               void *op_data)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    assert(elem);
    assert(dt);

    if (dt->shared->type == H5T_REFERENCE) {
        if (H5T__ref_reclaim(elem, dt) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "can't reclaim ref elements");
    }
    else {
        assert(op_data);

        /* Allow vlen reclaim to recurse into that routine */
        if (H5T__vlen_reclaim(elem, dt, (H5T_vlen_alloc_info_t *)op_data) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFREE, FAIL, "can't reclaim vlen elements");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_reclaim_cb() */

/*-------------------------------------------------------------------------
 * Function:  H5T_get_force_conv
 *
 * Purpose:   Determines if the type has forced conversion. This will be
 *            true if and only if the type keeps a pointer to a file VOL
 *            object internally.
 *
 * Return:    true/false (never fails)
 *
 *-------------------------------------------------------------------------
 */
bool
H5T_get_force_conv(const H5T_t *dt)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    assert(dt);
    assert(dt->shared);

    FUNC_LEAVE_NOAPI(dt->shared->force_conv)
} /* end H5T_get_force_conv() */

/*-------------------------------------------------------------------------
 * Function:    H5T__reverse_order
 *
 * Purpose:     Utility function to reverse the order of a sequence of
 *              bytes when it's big endian or VAX order. The byte sequence
 *              simulates the endian order.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__reverse_order(uint8_t *rev, uint8_t *s, size_t size, H5T_order_t order)
{
    size_t i;

    FUNC_ENTER_PACKAGE_NOERR

    assert(s);
    assert(size);

    if (H5T_ORDER_VAX == order) {
        for (i = 0; i < size; i += 2) {
            rev[i]     = s[(size - 2) - i];
            rev[i + 1] = s[(size - 1) - i];
        }
    }
    else if (H5T_ORDER_BE == order) {
        for (i = 0; i < size; i++)
            rev[size - (i + 1)] = s[i];
    }
    else {
        for (i = 0; i < size; i++)
            rev[i] = s[i];
    }

    FUNC_LEAVE_NOAPI(SUCCEED)
}

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_noop
 *
 * Purpose:    The no-op conversion.  The library knows about this
 *        conversion without it being registered.
 *
 * Return:     Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_noop(const H5T_t H5_ATTR_UNUSED *src, const H5T_t H5_ATTR_UNUSED *dst, H5T_cdata_t *cdata,
               const H5T_conv_ctx_t H5_ATTR_UNUSED *conv_ctx, size_t H5_ATTR_UNUSED nelmts,
               size_t H5_ATTR_UNUSED buf_stride, size_t H5_ATTR_UNUSED bkg_stride, void H5_ATTR_UNUSED *buf,
               void H5_ATTR_UNUSED *background)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT:
            cdata->need_bkg = H5T_BKG_NO;
            break;

        case H5T_CONV_CONV:
            /* Nothing to convert */
            break;

        case H5T_CONV_FREE:
            break;

        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_noop() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_order
 *
 * Purpose:    Convert one type to another when byte order is the only
 *        difference.
 *
 * Note:    This is a soft conversion function.
 *
 * Return:    Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_order(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                const H5T_conv_ctx_t H5_ATTR_UNUSED *conv_ctx, size_t nelmts, size_t buf_stride,
                size_t H5_ATTR_UNUSED bkg_stride, void *_buf, void H5_ATTR_UNUSED *background)
{
    uint8_t *buf = (uint8_t *)_buf;
    size_t   i;
    size_t   j, md;
    herr_t   ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT:
            /* Capability query */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (src->shared->size != dst->shared->size || 0 != src->shared->u.atomic.offset ||
                0 != dst->shared->u.atomic.offset ||
                !((H5T_ORDER_BE == src->shared->u.atomic.order &&
                   H5T_ORDER_LE == dst->shared->u.atomic.order) ||
                  (H5T_ORDER_LE == src->shared->u.atomic.order &&
                   H5T_ORDER_BE == dst->shared->u.atomic.order)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "conversion not supported");
            switch (src->shared->type) {
                case H5T_INTEGER:
                case H5T_BITFIELD:
                    /* nothing to check */
                    break;

                case H5T_FLOAT:
                    if (src->shared->u.atomic.u.f.sign != dst->shared->u.atomic.u.f.sign ||
                        src->shared->u.atomic.u.f.epos != dst->shared->u.atomic.u.f.epos ||
                        src->shared->u.atomic.u.f.esize != dst->shared->u.atomic.u.f.esize ||
                        src->shared->u.atomic.u.f.ebias != dst->shared->u.atomic.u.f.ebias ||
                        src->shared->u.atomic.u.f.mpos != dst->shared->u.atomic.u.f.mpos ||
                        src->shared->u.atomic.u.f.msize != dst->shared->u.atomic.u.f.msize ||
                        src->shared->u.atomic.u.f.norm != dst->shared->u.atomic.u.f.norm ||
                        src->shared->u.atomic.u.f.pad != dst->shared->u.atomic.u.f.pad) {
                        HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "conversion not supported");
                    } /* end if */
                    break;

                case H5T_NO_CLASS:
                case H5T_TIME:
                case H5T_STRING:
                case H5T_OPAQUE:
                case H5T_COMPOUND:
                case H5T_REFERENCE:
                case H5T_ENUM:
                case H5T_VLEN:
                case H5T_ARRAY:
                case H5T_NCLASSES:
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "conversion not supported");
            } /* end switch */
            cdata->need_bkg = H5T_BKG_NO;
            break;

        case H5T_CONV_CONV:
            /* The conversion */
            if (NULL == src)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");

            buf_stride = buf_stride ? buf_stride : src->shared->size;
            md         = src->shared->size / 2;
            for (i = 0; i < nelmts; i++, buf += buf_stride)
                for (j = 0; j < md; j++)
                    H5_SWAP_BYTES(buf, j, src->shared->size - (j + 1));
            break;

        case H5T_CONV_FREE:
            /* Free private data */
            break;

        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_order() */

/*-------------------------------------------------------------------------
 * Function:    H5T__conv_order_opt
 *
 * Purpose:    Convert one type to another when byte order is the only
 *        difference. This is the optimized version of H5T__conv_order()
 *              for a handful of different sizes.
 *
 * Note:    This is a soft conversion function.
 *
 * Return:    Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T__conv_order_opt(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                    const H5T_conv_ctx_t H5_ATTR_UNUSED *conv_ctx, size_t nelmts, size_t buf_stride,
                    size_t H5_ATTR_UNUSED bkg_stride, void *_buf, void H5_ATTR_UNUSED *background)
{
    uint8_t *buf = (uint8_t *)_buf;
    size_t   i;
    herr_t   ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (cdata->command) {
        case H5T_CONV_INIT:
            /* Capability query */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");
            if (src->shared->size != dst->shared->size || 0 != src->shared->u.atomic.offset ||
                0 != dst->shared->u.atomic.offset)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "conversion not supported");
            if ((src->shared->type == H5T_REFERENCE && dst->shared->type != H5T_REFERENCE) ||
                (dst->shared->type == H5T_REFERENCE && src->shared->type != H5T_REFERENCE))
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "conversion not supported");
            if (src->shared->type != H5T_REFERENCE && !((H5T_ORDER_BE == src->shared->u.atomic.order &&
                                                         H5T_ORDER_LE == dst->shared->u.atomic.order) ||
                                                        (H5T_ORDER_LE == src->shared->u.atomic.order &&
                                                         H5T_ORDER_BE == dst->shared->u.atomic.order)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "conversion not supported");
            if (src->shared->size != 1 && src->shared->size != 2 && src->shared->size != 4 &&
                src->shared->size != 8 && src->shared->size != 16)
                HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "conversion not supported");
            switch (src->shared->type) {
                case H5T_INTEGER:
                case H5T_BITFIELD:
                case H5T_REFERENCE:
                    /* nothing to check */
                    break;

                case H5T_FLOAT:
                    if (src->shared->u.atomic.u.f.sign != dst->shared->u.atomic.u.f.sign ||
                        src->shared->u.atomic.u.f.epos != dst->shared->u.atomic.u.f.epos ||
                        src->shared->u.atomic.u.f.esize != dst->shared->u.atomic.u.f.esize ||
                        src->shared->u.atomic.u.f.ebias != dst->shared->u.atomic.u.f.ebias ||
                        src->shared->u.atomic.u.f.mpos != dst->shared->u.atomic.u.f.mpos ||
                        src->shared->u.atomic.u.f.msize != dst->shared->u.atomic.u.f.msize ||
                        src->shared->u.atomic.u.f.norm != dst->shared->u.atomic.u.f.norm ||
                        src->shared->u.atomic.u.f.pad != dst->shared->u.atomic.u.f.pad)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "conversion not supported");
                    break;

                case H5T_NO_CLASS:
                case H5T_TIME:
                case H5T_STRING:
                case H5T_OPAQUE:
                case H5T_COMPOUND:
                case H5T_ENUM:
                case H5T_VLEN:
                case H5T_ARRAY:
                case H5T_NCLASSES:
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "conversion not supported");
            }
            cdata->need_bkg = H5T_BKG_NO;
            break;

        case H5T_CONV_CONV:
            /* The conversion */
            if (NULL == src || NULL == dst)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype");

            /* Check for "no op" reference conversion */
            if (src->shared->type == H5T_REFERENCE) {
                /* Sanity check */
                if (dst->shared->type != H5T_REFERENCE)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a H5T_REFERENCE datatype");

                /* Check if we are on a little-endian machine (the order that
                 * the addresses in the file must be) and just get out now, there
                 * is no need to convert the object reference.  Yes, this is
                 * icky and non-portable, but I can't think of a better way to
                 * support allowing the objno in the H5O_info_t struct and the
                 * hobj_ref_t type to be compared directly without introducing a
                 * "native" hobj_ref_t datatype and I think that would break a
                 * lot of existing programs.  -QAK
                 */
                if (H5T_native_order_g == H5T_ORDER_LE)
                    break;
            } /* end if */

            buf_stride = buf_stride ? buf_stride : src->shared->size;
            switch (src->shared->size) {
                case 1:
                    /*no-op*/
                    break;

                case 2:
                    for (/*void*/; nelmts >= 20; nelmts -= 20) {
                        H5_SWAP_BYTES(buf, 0, 1); /*  0 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /*  1 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /*  2 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /*  3 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /*  4 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /*  5 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /*  6 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /*  7 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /*  8 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /*  9 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /* 10 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /* 11 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /* 12 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /* 13 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /* 14 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /* 15 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /* 16 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /* 17 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /* 18 */
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 1); /* 19 */
                        buf += buf_stride;
                    } /* end for */
                    for (i = 0; i < nelmts; i++, buf += buf_stride)
                        H5_SWAP_BYTES(buf, 0, 1);
                    break;

                case 4:
                    for (/*void*/; nelmts >= 20; nelmts -= 20) {
                        H5_SWAP_BYTES(buf, 0, 3); /*  0 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /*  1 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /*  2 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /*  3 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /*  4 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /*  5 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /*  6 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /*  7 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /*  8 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /*  9 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /* 10 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /* 11 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /* 12 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /* 13 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /* 14 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /* 15 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /* 16 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /* 17 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /* 18 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 3); /* 19 */
                        H5_SWAP_BYTES(buf, 1, 2);
                        buf += buf_stride;
                    } /* end for */
                    for (i = 0; i < nelmts; i++, buf += buf_stride) {
                        H5_SWAP_BYTES(buf, 0, 3);
                        H5_SWAP_BYTES(buf, 1, 2);
                    } /* end for */
                    break;

                case 8:
                    for (/*void*/; nelmts >= 10; nelmts -= 10) {
                        H5_SWAP_BYTES(buf, 0, 7); /*  0 */
                        H5_SWAP_BYTES(buf, 1, 6);
                        H5_SWAP_BYTES(buf, 2, 5);
                        H5_SWAP_BYTES(buf, 3, 4);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 7); /*  1 */
                        H5_SWAP_BYTES(buf, 1, 6);
                        H5_SWAP_BYTES(buf, 2, 5);
                        H5_SWAP_BYTES(buf, 3, 4);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 7); /*  2 */
                        H5_SWAP_BYTES(buf, 1, 6);
                        H5_SWAP_BYTES(buf, 2, 5);
                        H5_SWAP_BYTES(buf, 3, 4);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 7); /*  3 */
                        H5_SWAP_BYTES(buf, 1, 6);
                        H5_SWAP_BYTES(buf, 2, 5);
                        H5_SWAP_BYTES(buf, 3, 4);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 7); /*  4 */
                        H5_SWAP_BYTES(buf, 1, 6);
                        H5_SWAP_BYTES(buf, 2, 5);
                        H5_SWAP_BYTES(buf, 3, 4);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 7); /*  5 */
                        H5_SWAP_BYTES(buf, 1, 6);
                        H5_SWAP_BYTES(buf, 2, 5);
                        H5_SWAP_BYTES(buf, 3, 4);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 7); /*  6 */
                        H5_SWAP_BYTES(buf, 1, 6);
                        H5_SWAP_BYTES(buf, 2, 5);
                        H5_SWAP_BYTES(buf, 3, 4);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 7); /*  7 */
                        H5_SWAP_BYTES(buf, 1, 6);
                        H5_SWAP_BYTES(buf, 2, 5);
                        H5_SWAP_BYTES(buf, 3, 4);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 7); /*  8 */
                        H5_SWAP_BYTES(buf, 1, 6);
                        H5_SWAP_BYTES(buf, 2, 5);
                        H5_SWAP_BYTES(buf, 3, 4);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 7); /*  9 */
                        H5_SWAP_BYTES(buf, 1, 6);
                        H5_SWAP_BYTES(buf, 2, 5);
                        H5_SWAP_BYTES(buf, 3, 4);
                        buf += buf_stride;
                    } /* end for */
                    for (i = 0; i < nelmts; i++, buf += buf_stride) {
                        H5_SWAP_BYTES(buf, 0, 7);
                        H5_SWAP_BYTES(buf, 1, 6);
                        H5_SWAP_BYTES(buf, 2, 5);
                        H5_SWAP_BYTES(buf, 3, 4);
                    } /* end for */
                    break;

                case 16:
                    for (/*void*/; nelmts >= 10; nelmts -= 10) {
                        H5_SWAP_BYTES(buf, 0, 15); /*  0 */
                        H5_SWAP_BYTES(buf, 1, 14);
                        H5_SWAP_BYTES(buf, 2, 13);
                        H5_SWAP_BYTES(buf, 3, 12);
                        H5_SWAP_BYTES(buf, 4, 11);
                        H5_SWAP_BYTES(buf, 5, 10);
                        H5_SWAP_BYTES(buf, 6, 9);
                        H5_SWAP_BYTES(buf, 7, 8);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 15); /*  1 */
                        H5_SWAP_BYTES(buf, 1, 14);
                        H5_SWAP_BYTES(buf, 2, 13);
                        H5_SWAP_BYTES(buf, 3, 12);
                        H5_SWAP_BYTES(buf, 4, 11);
                        H5_SWAP_BYTES(buf, 5, 10);
                        H5_SWAP_BYTES(buf, 6, 9);
                        H5_SWAP_BYTES(buf, 7, 8);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 15); /*  2 */
                        H5_SWAP_BYTES(buf, 1, 14);
                        H5_SWAP_BYTES(buf, 2, 13);
                        H5_SWAP_BYTES(buf, 3, 12);
                        H5_SWAP_BYTES(buf, 4, 11);
                        H5_SWAP_BYTES(buf, 5, 10);
                        H5_SWAP_BYTES(buf, 6, 9);
                        H5_SWAP_BYTES(buf, 7, 8);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 15); /*  3 */
                        H5_SWAP_BYTES(buf, 1, 14);
                        H5_SWAP_BYTES(buf, 2, 13);
                        H5_SWAP_BYTES(buf, 3, 12);
                        H5_SWAP_BYTES(buf, 4, 11);
                        H5_SWAP_BYTES(buf, 5, 10);
                        H5_SWAP_BYTES(buf, 6, 9);
                        H5_SWAP_BYTES(buf, 7, 8);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 15); /*  4 */
                        H5_SWAP_BYTES(buf, 1, 14);
                        H5_SWAP_BYTES(buf, 2, 13);
                        H5_SWAP_BYTES(buf, 3, 12);
                        H5_SWAP_BYTES(buf, 4, 11);
                        H5_SWAP_BYTES(buf, 5, 10);
                        H5_SWAP_BYTES(buf, 6, 9);
                        H5_SWAP_BYTES(buf, 7, 8);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 15); /*  5 */
                        H5_SWAP_BYTES(buf, 1, 14);
                        H5_SWAP_BYTES(buf, 2, 13);
                        H5_SWAP_BYTES(buf, 3, 12);
                        H5_SWAP_BYTES(buf, 4, 11);
                        H5_SWAP_BYTES(buf, 5, 10);
                        H5_SWAP_BYTES(buf, 6, 9);
                        H5_SWAP_BYTES(buf, 7, 8);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 15); /*  6 */
                        H5_SWAP_BYTES(buf, 1, 14);
                        H5_SWAP_BYTES(buf, 2, 13);
                        H5_SWAP_BYTES(buf, 3, 12);
                        H5_SWAP_BYTES(buf, 4, 11);
                        H5_SWAP_BYTES(buf, 5, 10);
                        H5_SWAP_BYTES(buf, 6, 9);
                        H5_SWAP_BYTES(buf, 7, 8);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 15); /*  7 */
                        H5_SWAP_BYTES(buf, 1, 14);
                        H5_SWAP_BYTES(buf, 2, 13);
                        H5_SWAP_BYTES(buf, 3, 12);
                        H5_SWAP_BYTES(buf, 4, 11);
                        H5_SWAP_BYTES(buf, 5, 10);
                        H5_SWAP_BYTES(buf, 6, 9);
                        H5_SWAP_BYTES(buf, 7, 8);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 15); /*  8 */
                        H5_SWAP_BYTES(buf, 1, 14);
                        H5_SWAP_BYTES(buf, 2, 13);
                        H5_SWAP_BYTES(buf, 3, 12);
                        H5_SWAP_BYTES(buf, 4, 11);
                        H5_SWAP_BYTES(buf, 5, 10);
                        H5_SWAP_BYTES(buf, 6, 9);
                        H5_SWAP_BYTES(buf, 7, 8);
                        buf += buf_stride;
                        H5_SWAP_BYTES(buf, 0, 15); /*  9 */
                        H5_SWAP_BYTES(buf, 1, 14);
                        H5_SWAP_BYTES(buf, 2, 13);
                        H5_SWAP_BYTES(buf, 3, 12);
                        H5_SWAP_BYTES(buf, 4, 11);
                        H5_SWAP_BYTES(buf, 5, 10);
                        H5_SWAP_BYTES(buf, 6, 9);
                        H5_SWAP_BYTES(buf, 7, 8);
                        buf += buf_stride;
                    } /* end for */
                    for (i = 0; i < nelmts; i++, buf += buf_stride) {
                        H5_SWAP_BYTES(buf, 0, 15);
                        H5_SWAP_BYTES(buf, 1, 14);
                        H5_SWAP_BYTES(buf, 2, 13);
                        H5_SWAP_BYTES(buf, 3, 12);
                        H5_SWAP_BYTES(buf, 4, 11);
                        H5_SWAP_BYTES(buf, 5, 10);
                        H5_SWAP_BYTES(buf, 6, 9);
                        H5_SWAP_BYTES(buf, 7, 8);
                    } /* end for */
                    break;

                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "invalid conversion size");
            } /* end switch */
            break;

        case H5T_CONV_FREE:
            /* Free private data */
            break;

        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown conversion command");
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T__conv_order_opt() */
