/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:		H5Dfill.c
 *			Jun 19 2007
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		Fill value operations for datasets
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5D_PACKAGE		/*suppress error about including H5Dpkg	  */

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Dpkg.h"		/* Dataset functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5VMprivate.h"		/* Vector and array functions		*/
#include "H5WBprivate.h"        /* Wrapped Buffers                      */


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

static herr_t H5D__fill_release(H5D_fill_buf_info_t *fb_info);


/*********************/
/* Package Variables */
/*********************/

/* Declare extern the free list to manage blocks of type conversion data */
H5FL_BLK_EXTERN(type_conv);


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

/* Declare the free list to manage blocks of non-zero fill-value data */
H5FL_BLK_DEFINE_STATIC(non_zero_fill);

/* Declare the free list to manage blocks of zero fill-value data */
H5FL_BLK_DEFINE_STATIC(zero_fill);


/*--------------------------------------------------------------------------
 NAME
    H5Dfill
 PURPOSE
    Fill a selection in memory with a value
 USAGE
    herr_t H5Dfill(fill, fill_type, space, buf, buf_type)
        const void *fill;       IN: Pointer to fill value to use
        hid_t fill_type_id;     IN: Datatype of the fill value
        void *buf;              IN/OUT: Memory buffer to fill selection within
        hid_t buf_type_id;      IN: Datatype of the elements in buffer
        hid_t space_id;         IN: Dataspace describing memory buffer &
                                    containing selection to use.
 RETURNS
    Non-negative on success/Negative on failure.
 DESCRIPTION
    Use the selection in the dataspace to fill elements in a memory buffer.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    If "fill" parameter is NULL, use all zeros as fill value
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Dfill(const void *fill, hid_t fill_type_id, void *buf, hid_t buf_type_id, hid_t space_id)
{
    H5S_t *space;               /* Dataspace */
    H5T_t *fill_type;           /* Fill-value datatype */
    H5T_t *buf_type;            /* Buffer datatype */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "*xi*xii", fill, fill_type_id, buf, buf_type_id, space_id);

    /* Check args */
    if(buf == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid buffer")
    if(NULL == (space = (H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not a dataspace")
    if(NULL == (fill_type = (H5T_t *)H5I_object_verify(fill_type_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not a datatype")
    if(NULL == (buf_type = (H5T_t *)H5I_object_verify(buf_type_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not a datatype")

    /* Fill the selection in the memory buffer */
    if(H5D__fill(fill, fill_type, buf, buf_type, space, H5AC_dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTENCODE, FAIL, "filling selection failed")

done:
    FUNC_LEAVE_API(ret_value)
}   /* H5Dfill() */


/*--------------------------------------------------------------------------
 NAME
    H5D__fill
 PURPOSE
    Fill a selection in memory with a value (internal version)
 USAGE
    herr_t H5D__fill(fill, fill_type, buf, buf_type, space)
        const void *fill;       IN: Pointer to fill value to use
        H5T_t *fill_type;       IN: Datatype of the fill value
        void *buf;              IN/OUT: Memory buffer to fill selection within
        H5T_t *buf_type;        IN: Datatype of the elements in buffer
        H5S_t *space;           IN: Dataspace describing memory buffer &
                                    containing selection to use.
 RETURNS
    Non-negative on success/Negative on failure.
 DESCRIPTION
    Use the selection in the dataspace to fill elements in a memory buffer.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    If "fill" parameter is NULL, use all zeros as fill value.
 EXAMPLES
 REVISION LOG
    Raymond Lu - 20 March 2007
    If there's VL type of data, the address of the data is copied multiple
    times into the buffer, causing some trouble when the data is released.
    Instead, make multiple copies of fill value first, then do conversion
    on each element so that each of them has a copy of the VL data.
--------------------------------------------------------------------------*/
herr_t
H5D__fill(const void *fill, const H5T_t *fill_type, void *buf,
    const H5T_t *buf_type, const H5S_t *space, hid_t dxpl_id)
{
    H5WB_t  *elem_wb = NULL;    /* Wrapped buffer for element data */
    uint8_t elem_buf[H5T_ELEM_BUF_SIZE]; /* Buffer for element data */
    H5WB_t  *bkg_elem_wb = NULL;     /* Wrapped buffer for background data */
    uint8_t bkg_elem_buf[H5T_ELEM_BUF_SIZE]; /* Buffer for background data */
    uint8_t *bkg_buf = NULL;    /* Background conversion buffer */
    uint8_t *tmp_buf = NULL;    /* Temp conversion buffer */
    hid_t src_id = -1, dst_id = -1;     /* Temporary type IDs */
    size_t dst_type_size;       /* Size of destination type*/
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(fill_type);
    HDassert(buf);
    HDassert(buf_type);
    HDassert(space);

    /* Make sure the dataspace has an extent set (or is NULL) */
    if(!(H5S_has_extent(space)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "dataspace extent has not been set")

    /* Get the memory datatype size */
    dst_type_size = H5T_get_size(buf_type);

    /* If there's no fill value, just use zeros */
    if(fill == NULL) {
        void *elem_ptr;         /* Pointer to element to use for fill value */

        /* Wrap the local buffer for elements */
        if(NULL == (elem_wb = H5WB_wrap(elem_buf, sizeof(elem_buf))))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't wrap buffer")

        /* Get a pointer to a buffer that's large enough for element */
        if(NULL == (elem_ptr = H5WB_actual_clear(elem_wb, dst_type_size)))
            HGOTO_ERROR(H5E_DATASET, H5E_NOSPACE, FAIL, "can't get actual buffer")

        /* Fill the selection in the memory buffer */
        if(H5S_select_fill(elem_ptr, dst_type_size, space, buf) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTENCODE, FAIL, "filling selection failed")
    } /* end if */
    else {
        H5T_path_t *tpath;      /* Conversion path information */
        size_t src_type_size;   /* Size of source type	*/
        size_t buf_size;        /* Desired buffer size	*/

        /* Get the file datatype size */
        src_type_size = H5T_get_size(fill_type);

        /* Get the maximum buffer size needed and allocate it */
        buf_size = MAX(src_type_size, dst_type_size);

        /* Set up type conversion function */
        if(NULL == (tpath = H5T_path_find(fill_type, buf_type, NULL, NULL, dxpl_id, FALSE)))
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "unable to convert between src and dest datatype")

        /* Construct source & destination datatype IDs, if we will need them */
        if(!H5T_path_noop(tpath)) {
            if((src_id = H5I_register(H5I_DATATYPE, H5T_copy(fill_type, H5T_COPY_ALL), FALSE)) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTREGISTER, FAIL, "unable to register types for conversion")

            if((dst_id = H5I_register(H5I_DATATYPE, H5T_copy(buf_type, H5T_COPY_ALL), FALSE)) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTREGISTER, FAIL, "unable to register types for conversion")
        } /* end if */

        /* If there's VL type of data, make multiple copies of fill value first,
         * then do conversion on each element so that each of them has a copy
         * of the VL data.
         */
        if(TRUE == H5T_detect_class(fill_type, H5T_VLEN, FALSE)) {
            H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
            H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
            H5S_sel_iter_t mem_iter;            /* Memory selection iteration info */
            hssize_t nelmts;                    /* Number of data elements */

            /* Get the number of elements in the selection */
            nelmts = H5S_GET_SELECT_NPOINTS(space);
            HDassert(nelmts >= 0);
            H5_CHECK_OVERFLOW(nelmts, hssize_t, size_t);

            /* Allocate a temporary buffer */
            if(NULL == (tmp_buf = H5FL_BLK_MALLOC(type_conv, (size_t)nelmts * buf_size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

            /* Allocate a background buffer, if necessary */
            if(H5T_path_bkg(tpath) && NULL == (bkg_buf = H5FL_BLK_CALLOC(type_conv, (size_t)nelmts * buf_size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

            /* Replicate the file's fill value into the temporary buffer */
            H5VM_array_fill(tmp_buf, fill, src_type_size, (size_t)nelmts);

            /* Convert from file's fill value into memory form */
            if(H5T_convert(tpath, src_id, dst_id, (size_t)nelmts, (size_t)0, (size_t)0, tmp_buf, bkg_buf, dxpl_id) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "data type conversion failed")

            /* Fill the DXPL cache values for later use */
            if(H5D__get_dxpl_cache(dxpl_id, &dxpl_cache) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

            /* Create a selection iterator for scattering the elements to memory buffer */
            if(H5S_select_iter_init(&mem_iter, space, dst_type_size) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize memory selection information")

            /* Scatter the data into memory */
            if(H5D__scatter_mem(tmp_buf, space, &mem_iter, (size_t)nelmts, dxpl_cache, buf/*out*/) < 0) {
                H5S_SELECT_ITER_RELEASE(&mem_iter);
                HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "scatter failed")
            } /* end if */

            /* Release the selection iterator */
            if(H5S_SELECT_ITER_RELEASE(&mem_iter) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release selection iterator")
        } /* end if */
        else {
            const uint8_t *fill_buf;          /* Buffer to use for writing fill values */

            /* Convert disk buffer into memory buffer */
            if(!H5T_path_noop(tpath)) {
                void *elem_ptr;         /* Pointer to element to use for fill value */
                void *bkg_ptr = NULL;   /* Pointer to background element to use for fill value */

                /* Wrap the local buffer for elements */
                if(NULL == (elem_wb = H5WB_wrap(elem_buf, sizeof(elem_buf))))
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't wrap buffer")

                /* Get a pointer to a buffer that's large enough for element */
                if(NULL == (elem_ptr = H5WB_actual(elem_wb, buf_size)))
                    HGOTO_ERROR(H5E_DATASET, H5E_NOSPACE, FAIL, "can't get actual buffer")

                /* Copy the user's data into the buffer for conversion */
                HDmemcpy(elem_ptr, fill, src_type_size);

                /* If there's no VL type of data, do conversion first then fill the data into
                 * the memory buffer. */
                if(H5T_path_bkg(tpath)) {
                    /* Wrap the local buffer for background elements */
                    if(NULL == (bkg_elem_wb = H5WB_wrap(bkg_elem_buf, sizeof(bkg_elem_buf))))
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't wrap buffer")

                    /* Get a pointer to a buffer that's large enough for element */
                    if(NULL == (bkg_ptr = H5WB_actual_clear(bkg_elem_wb, buf_size)))
                        HGOTO_ERROR(H5E_DATASET, H5E_NOSPACE, FAIL, "can't get actual buffer")
                } /* end if */

                /* Perform datatype conversion */
                if(H5T_convert(tpath, src_id, dst_id, (size_t)1, (size_t)0, (size_t)0, elem_ptr, bkg_ptr, dxpl_id) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "data type conversion failed")

                /* Point at element buffer */
                fill_buf = (const uint8_t *)elem_ptr;
            } /* end if */
            else
                fill_buf = (const uint8_t *)fill;

            /* Fill the selection in the memory buffer */
            if(H5S_select_fill(fill_buf, dst_type_size, space, buf) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTENCODE, FAIL, "filling selection failed")
        } /* end else */
    } /* end else */

done:
    if(src_id != (-1) && H5I_dec_ref(src_id) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't decrement temporary datatype ID")
    if(dst_id != (-1) && H5I_dec_ref(dst_id) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't decrement temporary datatype ID")
    if(tmp_buf)
        tmp_buf = H5FL_BLK_FREE(type_conv, tmp_buf);
    if(elem_wb && H5WB_unwrap(elem_wb) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CLOSEERROR, FAIL, "can't close wrapped buffer")
    if(bkg_elem_wb && H5WB_unwrap(bkg_elem_wb) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CLOSEERROR, FAIL, "can't close wrapped buffer")
    if(bkg_buf)
        bkg_buf = H5FL_BLK_FREE(type_conv, bkg_buf);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__fill() */


/*-------------------------------------------------------------------------
 * Function:	H5D__fill_init
 *
 * Purpose:	Initialize buffer filling operation
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		June 21, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__fill_init(H5D_fill_buf_info_t *fb_info, void *caller_fill_buf,
    H5MM_allocate_t alloc_func, void *alloc_info,
    H5MM_free_t free_func, void *free_info,
    const H5O_fill_t *fill, const H5T_t *dset_type, hid_t dset_type_id,
    size_t total_nelmts, size_t max_buf_size, hid_t dxpl_id)
{
    herr_t	ret_value = SUCCEED;	/* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(fb_info);
    HDassert(fill);
    HDassert(dset_type);
    HDassert(dset_type_id > 0);

    /* Reset fill buffer information */
    HDmemset(fb_info, 0, sizeof(*fb_info));

    /* Cache constant information from the dataset */
    fb_info->fill = fill;
    fb_info->file_type = dset_type;
    fb_info->file_tid = dset_type_id;
    fb_info->fill_alloc_func = alloc_func;
    fb_info->fill_alloc_info = alloc_info;
    fb_info->fill_free_func = free_func;
    fb_info->fill_free_info = free_info;

    /* Fill the buffer with the user's fill value */
    if(fill->buf) {
        htri_t has_vlen_type;   /* Whether the datatype has a VL component */

        /* Detect whether the datatype has a VL component */
        if((has_vlen_type = H5T_detect_class(dset_type, H5T_VLEN, FALSE)) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, FAIL, "unable to detect vlen datatypes?")
        fb_info->has_vlen_fill_type = (hbool_t)has_vlen_type;

        /* If necessary, convert fill value datatypes (which copies VL components, etc.) */
        if(fb_info->has_vlen_fill_type) {
            /* Create temporary datatype for conversion operation */
            if(NULL == (fb_info->mem_type = H5T_copy(dset_type, H5T_COPY_REOPEN)))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTCOPY, FAIL, "unable to copy file datatype")
            if((fb_info->mem_tid = H5I_register(H5I_DATATYPE, fb_info->mem_type, FALSE)) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register memory datatype")

            /* Retrieve sizes of memory & file datatypes */
            fb_info->mem_elmt_size = H5T_get_size(fb_info->mem_type);
            HDassert(fb_info->mem_elmt_size > 0);
            fb_info->file_elmt_size = H5T_get_size(dset_type);
            HDassert(fb_info->file_elmt_size == (size_t)fill->size);

            /* If fill value is not library default, use it to set the element size */
            fb_info->max_elmt_size = MAX(fb_info->mem_elmt_size, fb_info->file_elmt_size);

            /* Compute the number of elements that fit within a buffer to write */
            if(total_nelmts > 0)
                fb_info->elmts_per_buf = MIN(total_nelmts, MAX(1, (max_buf_size / fb_info->max_elmt_size)));
            else
                fb_info->elmts_per_buf = max_buf_size / fb_info->max_elmt_size;
            HDassert(fb_info->elmts_per_buf > 0);

            /* Compute the buffer size to use */
            fb_info->fill_buf_size = MIN(max_buf_size, (fb_info->elmts_per_buf * fb_info->max_elmt_size));

            /* Allocate fill buffer */
            if(caller_fill_buf) {
                fb_info->fill_buf = caller_fill_buf;
                fb_info->use_caller_fill_buf = TRUE;
            } /* end if */
            else {
                if(alloc_func)
                    fb_info->fill_buf = alloc_func(fb_info->fill_buf_size, alloc_info);
                else
                    fb_info->fill_buf = H5FL_BLK_MALLOC(non_zero_fill, fb_info->fill_buf_size);
                if(NULL == fb_info->fill_buf)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for fill buffer")
            } /* end else */

            /* Get the datatype conversion path for this operation */
            if(NULL == (fb_info->fill_to_mem_tpath = H5T_path_find(dset_type, fb_info->mem_type, NULL, NULL, dxpl_id, FALSE)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to convert between src and dst datatypes")

            /* Get the inverse datatype conversion path for this operation */
            if(NULL == (fb_info->mem_to_dset_tpath = H5T_path_find(fb_info->mem_type, dset_type, NULL, NULL, dxpl_id, FALSE)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to convert between src and dst datatypes")

            /* Check if we need to allocate a background buffer */
            if(H5T_path_bkg(fb_info->fill_to_mem_tpath) || H5T_path_bkg(fb_info->mem_to_dset_tpath)) {
                /* Check for inverse datatype conversion needing a background buffer */
                /* (do this first, since it needs a larger buffer) */
                if(H5T_path_bkg(fb_info->mem_to_dset_tpath))
                    fb_info->bkg_buf_size = fb_info->elmts_per_buf * fb_info->max_elmt_size;
                else
                    fb_info->bkg_buf_size = fb_info->max_elmt_size;

                /* Allocate the background buffer */
                if(NULL == (fb_info->bkg_buf = H5FL_BLK_MALLOC(type_conv, fb_info->bkg_buf_size)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
            } /* end if */
        } /* end if */
        else {
            /* If fill value is not library default, use it to set the element size */
            HDassert(fill->size >= 0);
            fb_info->max_elmt_size = fb_info->file_elmt_size = fb_info->mem_elmt_size = (size_t)fill->size;

            /* Compute the number of elements that fit within a buffer to write */
            if(total_nelmts > 0)
                fb_info->elmts_per_buf = MIN(total_nelmts, MAX(1, (max_buf_size / fb_info->max_elmt_size)));
            else
                fb_info->elmts_per_buf = max_buf_size / fb_info->max_elmt_size;
            HDassert(fb_info->elmts_per_buf > 0);

            /* Compute the buffer size to use */
            fb_info->fill_buf_size = MIN(max_buf_size, fb_info->elmts_per_buf * fb_info->max_elmt_size);

            /* Allocate temporary buffer */
            if(caller_fill_buf) {
                fb_info->fill_buf = caller_fill_buf;
                fb_info->use_caller_fill_buf = TRUE;
            } /* end if */
            else {
                if(alloc_func)
                    fb_info->fill_buf = alloc_func(fb_info->fill_buf_size, alloc_info);
                else
                    fb_info->fill_buf = H5FL_BLK_MALLOC(non_zero_fill, fb_info->fill_buf_size);
                if(NULL == fb_info->fill_buf)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for fill buffer")
            } /* end else */

            /* Replicate the fill value into the cached buffer */
            H5VM_array_fill(fb_info->fill_buf, fill->buf, fb_info->max_elmt_size, fb_info->elmts_per_buf);
        } /* end else */
    } /* end if */
    else {      /* Fill the buffer with the default fill value */
        /* Retrieve size of elements */
        fb_info->max_elmt_size = fb_info->file_elmt_size = fb_info->mem_elmt_size = H5T_get_size(dset_type);
        HDassert(fb_info->max_elmt_size > 0);

        /* Compute the number of elements that fit within a buffer to write */
        if(total_nelmts > 0)
            fb_info->elmts_per_buf = MIN(total_nelmts, MAX(1, (max_buf_size / fb_info->max_elmt_size)));
        else
            fb_info->elmts_per_buf = max_buf_size / fb_info->max_elmt_size;
        HDassert(fb_info->elmts_per_buf > 0);

        /* Compute the buffer size to use */
        fb_info->fill_buf_size = MIN(max_buf_size, (fb_info->elmts_per_buf * fb_info->max_elmt_size));

        /* Use (and zero) caller's buffer, if provided */
        if(caller_fill_buf) {
            fb_info->fill_buf = caller_fill_buf;
            fb_info->use_caller_fill_buf = TRUE;

            HDmemset(fb_info->fill_buf, 0, fb_info->fill_buf_size);
        } /* end if */
        else {
            if(alloc_func) {
                fb_info->fill_buf = alloc_func(fb_info->fill_buf_size, alloc_info);

                HDmemset(fb_info->fill_buf, 0, fb_info->fill_buf_size);
            } /* end if */
            else {
                htri_t buf_avail = H5FL_BLK_AVAIL(zero_fill, fb_info->fill_buf_size);  /* Check if there is an already zeroed out buffer available */
                HDassert(buf_avail != FAIL);

                /* Allocate temporary buffer (zeroing it if no buffer is available) */
                if(!buf_avail)
                    fb_info->fill_buf = H5FL_BLK_CALLOC(zero_fill, fb_info->fill_buf_size);
                else
                    fb_info->fill_buf = H5FL_BLK_MALLOC(zero_fill, fb_info->fill_buf_size);
            } /* end else */
            if(fb_info->fill_buf == NULL)
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for fill buffer")
        } /* end else */
    } /* end else */

done:
    /* Cleanup on error */
    if(ret_value < 0)
        if(H5D__fill_term(fb_info) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "Can't release fill buffer info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__fill_init() */


/*-------------------------------------------------------------------------
 * Function:	H5D__fill_refill_vl
 *
 * Purpose:	Refill fill value buffer that contains VL-datatype fill values
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		June 21, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__fill_refill_vl(H5D_fill_buf_info_t *fb_info, size_t nelmts, hid_t dxpl_id)
{
    herr_t	ret_value = SUCCEED;	/* Return value */
    void * buf = NULL;              /* Temporary fill buffer */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(fb_info);
    HDassert(fb_info->has_vlen_fill_type);
    HDassert(fb_info->fill_buf);

    /* Make a copy of the (disk-based) fill value into the buffer */
    HDmemcpy(fb_info->fill_buf, fb_info->fill->buf, fb_info->file_elmt_size);

    /* Reset first element of background buffer, if necessary */
    if(H5T_path_bkg(fb_info->fill_to_mem_tpath))
        HDmemset(fb_info->bkg_buf, 0, fb_info->max_elmt_size);

    /* Type convert the dataset buffer, to copy any VL components */
    if(H5T_convert(fb_info->fill_to_mem_tpath, fb_info->file_tid, fb_info->mem_tid, (size_t)1, (size_t)0, (size_t)0, fb_info->fill_buf, fb_info->bkg_buf, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "data type conversion failed")

    /* Replicate the fill value into the cached buffer */
    if(nelmts > 1)
        H5VM_array_fill((void *)((unsigned char *)fb_info->fill_buf + fb_info->mem_elmt_size), fb_info->fill_buf, fb_info->mem_elmt_size, (nelmts - 1));

    /* Reset the entire background buffer, if necessary */
    if(H5T_path_bkg(fb_info->mem_to_dset_tpath))
        HDmemset(fb_info->bkg_buf, 0, fb_info->bkg_buf_size);

    /* Make a copy of the fill buffer so we can free dynamic elements after conversion */
    if(fb_info->fill_alloc_func)
        buf = fb_info->fill_alloc_func(fb_info->fill_buf_size, fb_info->fill_alloc_info);
    else
        buf = H5FL_BLK_MALLOC(non_zero_fill, fb_info->fill_buf_size);
    if(!buf)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "memory allocation failed for temporary fill buffer")

    HDmemcpy(buf, fb_info->fill_buf, fb_info->fill_buf_size);

    /* Type convert the dataset buffer, to copy any VL components */
    if(H5T_convert(fb_info->mem_to_dset_tpath, fb_info->mem_tid, fb_info->file_tid, nelmts, (size_t)0, (size_t)0, fb_info->fill_buf, fb_info->bkg_buf, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTCONVERT, FAIL, "data type conversion failed")

done:
    if(buf) {
        /* Free dynamically allocated VL elements in fill buffer */
        if(fb_info->fill->type) {
            if(H5T_vlen_reclaim_elmt(buf, fb_info->fill->type, dxpl_id) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "can't reclaim vlen element")
        } /* end if */
        else {
            if(H5T_vlen_reclaim_elmt(buf, fb_info->mem_type, dxpl_id) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "can't reclaim vlen element")
        } /* end else */

        /* Free temporary fill buffer */
        if(fb_info->fill_free_func)
            fb_info->fill_free_func(buf, fb_info->fill_free_info);
        else
            buf = H5FL_BLK_FREE(non_zero_fill, buf);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__fill_refill_vl() */


/*-------------------------------------------------------------------------
 * Function:	H5D__fill_release
 *
 * Purpose:	Release fill value buffer
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		June 22, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__fill_release(H5D_fill_buf_info_t *fb_info)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Check args */
    HDassert(fb_info);
    HDassert(fb_info->fill);

    /* Free the buffer for fill values */
    if(!fb_info->use_caller_fill_buf && fb_info->fill_buf) {
        if(fb_info->fill_free_func)
            fb_info->fill_free_func(fb_info->fill_buf, fb_info->fill_free_info);
        else {
            if(fb_info->fill->buf)
                fb_info->fill_buf = H5FL_BLK_FREE(non_zero_fill, fb_info->fill_buf);
            else
                fb_info->fill_buf = H5FL_BLK_FREE(zero_fill, fb_info->fill_buf);
        } /* end else */
        fb_info->fill_buf = NULL;
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D__fill_release() */


/*-------------------------------------------------------------------------
 * Function:	H5D__fill_term
 *
 * Purpose:	Release fill value buffer info
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		June 21, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__fill_term(H5D_fill_buf_info_t *fb_info)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Check args */
    HDassert(fb_info);

    /* Free the buffer for fill values */
    H5D__fill_release(fb_info);

    /* Free other resources for vlen fill values */
    if(fb_info->has_vlen_fill_type) {
        if(fb_info->mem_tid > 0)
            H5I_dec_ref(fb_info->mem_tid);
        else if(fb_info->mem_type)
            H5T_close(fb_info->mem_type);
        if(fb_info->bkg_buf)
            fb_info->bkg_buf = H5FL_BLK_FREE(type_conv, fb_info->bkg_buf);
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D__fill_term() */

