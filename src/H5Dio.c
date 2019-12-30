/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/****************/
/* Module Setup */
/****************/

#include "H5Dmodule.h"          /* This source code file is part of the H5D module */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5CXprivate.h"        /* API Contexts                         */
#include "H5Dpkg.h"		/* Dataset functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"        /* Memory management                    */
#include "H5Sprivate.h"		/* Dataspace			  	*/

#ifdef H5_HAVE_PARALLEL
/* Remove this if H5R_DATASET_REGION is no longer used in this file */
#include "H5Rpublic.h"
#endif /*H5_HAVE_PARALLEL*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Local Prototypes */
/********************/

static herr_t H5D__get_offset_copy(const H5D_t *dset, const hsize_t *offset,
        hsize_t *offset_copy/*out*/);
static herr_t H5D__ioinfo_init(H5D_t *dset, const H5D_type_info_t *type_info,
    H5D_storage_t *store, H5D_io_info_t *io_info);
static herr_t H5D__typeinfo_init(const H5D_t *dset, hid_t mem_type_id,
    hbool_t do_write, H5D_type_info_t *type_info);
#ifdef H5_HAVE_PARALLEL
static herr_t H5D__ioinfo_adjust(H5D_io_info_t *io_info, const H5D_t *dset,
    const H5S_t *file_space, const H5S_t *mem_space, const H5D_type_info_t *type_info);
#endif /* H5_HAVE_PARALLEL */
static herr_t H5D__typeinfo_term(const H5D_type_info_t *type_info);


/*********************/
/* Package Variables */
/*********************/


/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage blocks of type conversion data */
H5FL_BLK_DEFINE(type_conv);

/* Declare a free list to manage the H5D_chunk_map_t struct */
H5FL_DEFINE(H5D_chunk_map_t);



/*-------------------------------------------------------------------------
 * Function:    H5D__get_offset_copy
 *
 * Purpose:     Gets a copy of the user's offset array that is guaraneteed
 *              to be suitable for use by the library.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__get_offset_copy(const H5D_t *dset, const hsize_t *offset, hsize_t *offset_copy)
{
    unsigned u;
    herr_t ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(dset);
    HDassert(offset);
    HDassert(offset_copy);


    /* The library's chunking code requires the offset to terminate with a zero.
     * So transfer the offset array to an internal offset array that we
     * can properly terminate (handled via the calloc call).
     */

    HDmemset(offset_copy, 0, H5O_LAYOUT_NDIMS * sizeof(hsize_t));

    for (u = 0; u < dset->shared->ndims; u++) {
        /* Make sure the offset doesn't exceed the dataset's dimensions */
        if (offset[u] > dset->shared->curr_dims[u])
            HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "offset exceeds dimensions of dataset")

        /* Make sure the offset fall right on a chunk's boundary */
        if (offset[u] % dset->shared->layout.u.chunk.dim[u])
            HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "offset doesn't fall on chunks's boundary")

        offset_copy[u] = offset[u];
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* end H5D__get_offset_copy() */



/*-------------------------------------------------------------------------
 * Function:	H5Dread
 *
 * Purpose:     Reads (part of) a DSET from the file into application
 *              memory BUF. The part of the dataset to read is defined with
 *              MEM_SPACE_ID and FILE_SPACE_ID.	 The data points are
 *              converted from their file type to the MEM_TYPE_ID specified.
 *              Additional miscellaneous data transfer properties can be
 *              passed to this function with the PLIST_ID argument.
 *
 *              The FILE_SPACE_ID can be the constant H5S_ALL which indicates
 *              that the entire file dataspace is to be referenced.
 *
 *              The MEM_SPACE_ID can be the constant H5S_ALL in which case
 *              the memory dataspace is the same as the file dataspace
 *              defined when the dataset was created.
 *
 *              The number of elements in the memory dataspace must match
 *              the number of elements in the file dataspace.
 *
 *              The PLIST_ID can be the constant H5P_DEFAULT in which
 *              case the default data transfer properties are used.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, December 4, 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dread(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id,
    hid_t file_space_id, hid_t dxpl_id, void *buf/*out*/)
{
    H5D_t	       *dset        = NULL;
    const H5S_t	   *mem_space   = NULL;
    const H5S_t	   *file_space  = NULL;
    herr_t          ret_value   = SUCCEED;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "iiiiix", dset_id, mem_type_id, mem_space_id, file_space_id,
             dxpl_id, buf);

    /* Get dataset pointer */
    if (NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dset_id is not a dataset ID")
    if (NULL == dset->oloc.file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dataset is not associated with a file")

    /* Get validated dataspace pointers */
    if (H5S_get_validated_dataspace(mem_space_id, &mem_space) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "could not get a validated dataspace from mem_space_id")
    if (H5S_get_validated_dataspace(file_space_id, &file_space) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "could not get a validated dataspace from file_space_id")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else
        if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Read raw data */
    if (H5D__read(dset, mem_type_id, mem_space, file_space, buf/*out*/) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "can't read data")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dread() */


/*-------------------------------------------------------------------------
 * Function:    H5Dread_chunk
 *
 * Purpose:     Reads an entire chunk from the file directly.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Matthew Strong (GE Healthcare)
 *              14 February 2016
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5Dread_chunk(hid_t dset_id, hid_t dxpl_id, const hsize_t *offset, uint32_t *filters,
         void *buf)
{
    H5D_t      *dset = NULL;
    hsize_t     offset_copy[H5O_LAYOUT_NDIMS];  /* Internal copy of chunk offset */
    herr_t      ret_value = SUCCEED;            /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "ii*h*Iu*x", dset_id, dxpl_id, offset, filters, buf);

    /* Check arguments */
    if (NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dset_id is not a dataset ID")
    if (NULL == dset->oloc.file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dataset is not associated with a file")
    if (H5D_CHUNKED != dset->shared->layout.type)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a chunked dataset")
    if (!buf)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "buf cannot be NULL")
    if (!offset)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "offset cannot be NULL")
    if (!filters)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "filters cannot be NULL")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else
        if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dxpl_id is not a dataset transfer property list ID")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Copy the user's offset array so we can be sure it's terminated properly.
     * (we don't want to mess with the user's buffer).
     */
    if (H5D__get_offset_copy(dset, offset, offset_copy) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "failure to copy offset array")

    /* Read the raw chunk */
    if (H5D__chunk_direct_read(dset, offset_copy, filters, buf) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "can't read unprocessed chunk data")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dread_chunk() */


/*-------------------------------------------------------------------------
 * Function:    H5Dwrite
 *
 * Purpose:     Writes (part of) a DSET from application memory BUF to the
 *              file. The part of the dataset to write is defined with the
 *              MEM_SPACE_ID and FILE_SPACE_ID arguments. The data points
 *              are converted from their current type (MEM_TYPE_ID) to their
 *              file datatype. Additional miscellaneous data transfer
 *              properties can be passed to this function with the
 *              PLIST_ID argument.
 *
 *              The FILE_SPACE_ID can be the constant H5S_ALL which indicates
 *              that the entire file dataspace is to be referenced.
 *
 *              The MEM_SPACE_ID can be the constant H5S_ALL in which case
 *              the memory dataspace is the same as the file dataspace
 *              defined when the dataset was created.
 *
 *              The number of elements in the memory dataspace must match
 *              the number of elements in the file dataspace.
 *
 *              The PLIST_ID can be the constant H5P_DEFAULT in which
 *              case the default data transfer properties are used.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, December 4, 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dwrite(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id,
    hid_t file_space_id, hid_t dxpl_id, const void *buf)
{
    H5D_t                  *dset = NULL;
    const H5S_t            *mem_space = NULL;
    const H5S_t            *file_space = NULL;
    herr_t                  ret_value = SUCCEED;  /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "iiiii*x", dset_id, mem_type_id, mem_space_id, file_space_id,
             dxpl_id, buf);

    /* Get dataset pointer and ensure it's associated with a file */
    if (NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dset_id is not a dataset ID")
    if (NULL == dset->oloc.file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dataset is not associated with a file")

    /* Get validated dataspace pointers */
    if (H5S_get_validated_dataspace(mem_space_id, &mem_space) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "could not get a validated dataspace from mem_space_id")
    if (H5S_get_validated_dataspace(file_space_id, &file_space) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "could not get a validated dataspace from file_space_id")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else
        if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Write the data */
    if (H5D__write(dset, mem_type_id, mem_space, file_space, buf) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "can't write data")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dwrite() */


/*-------------------------------------------------------------------------
 * Function:    H5Dwrite_chunk
 *
 * Purpose:     Writes an entire chunk to the file directly.	
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *		        30 July 2012
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dwrite_chunk(hid_t dset_id, hid_t dxpl_id, uint32_t filters, const hsize_t *offset, 
         size_t data_size, const void *buf)
{
    H5D_t      *dset = NULL;
    hsize_t     offset_copy[H5O_LAYOUT_NDIMS];  /* Internal copy of chunk offset */
    uint32_t    data_size_32;                   /* Chunk data size (limited to 32-bits currently) */
    herr_t      ret_value = SUCCEED;            /* Return value */
    
    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "iiIu*hz*x", dset_id, dxpl_id, filters, offset, data_size, buf);

    /* Check arguments */
    if (NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataset ID")
    if (NULL == dset->oloc.file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dataset is not associated with a file")
    if (H5D_CHUNKED != dset->shared->layout.type)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a chunked dataset")
    if (!buf)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "buf cannot be NULL")
    if (!offset)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "offset cannot be NULL")
    if (0 == data_size)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "data_size cannot be zero")

    /* Make sure data size is less than 4 GiB */
    data_size_32 = (uint32_t)data_size;
    if (data_size != (size_t)data_size_32)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid data_size - chunks cannot be > 4 GiB")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else
        if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dxpl_id is not a dataset transfer property list ID")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Copy the user's offset array so we can be sure it's terminated properly.
     * (we don't want to mess with the user's buffer).
     */
    if (H5D__get_offset_copy(dset, offset, offset_copy) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "failure to copy offset array")

    /* Write chunk */
    if (H5D__chunk_direct_write(dset, filters, offset_copy, data_size_32, buf) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "can't write unprocessed chunk data")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dwrite_chunk() */


/*-------------------------------------------------------------------------
 * Function:	H5D__read
 *
 * Purpose:	Reads (part of) a DATASET into application memory BUF. See
 *		H5Dread() for complete details.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Thursday, December  4, 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__read(H5D_t *dataset, hid_t mem_type_id, const H5S_t *mem_space,
    const H5S_t *file_space, void *buf/*out*/)
{
    H5D_chunk_map_t *fm = NULL;         /* Chunk file<->memory mapping */
    H5D_io_info_t io_info;              /* Dataset I/O info     */
    H5D_type_info_t type_info;          /* Datatype info for operation */
    hbool_t type_info_init = FALSE;     /* Whether the datatype info has been initialized */
    H5S_t * projected_mem_space = NULL; /* If not NULL, ptr to dataspace containing a     */
                                        /* projection of the supplied mem_space to a new  */
                                        /* dataspace with rank equal to that of           */
                                        /* file_space.                                    */
                                        /*                                                */
                                        /* This field is only used if                     */
                                        /* H5S_select_shape_same() returns TRUE when      */
                                        /* comparing the mem_space and the data_space,    */
                                        /* and the mem_space have different rank.         */
                                        /*                                                */
                                        /* Note that if this variable is used, the        */
                                        /* projected mem space must be discarded at the   */
                                        /* end of the function to avoid a memory leak.    */
    H5D_storage_t store;                /*union of EFL and chunk pointer in file space */
    hssize_t	snelmts;                /*total number of elmts	(signed) */
    hsize_t	nelmts;                 /*total number of elmts	*/
    hbool_t     io_op_init = FALSE;     /* Whether the I/O op has been initialized */
    char        fake_char;              /* Temporary variable for NULL buffer pointers */
    herr_t	ret_value = SUCCEED;	/* Return value	*/

    FUNC_ENTER_PACKAGE_TAG(dataset->oloc.addr)

    /* check args */
    HDassert(dataset && dataset->oloc.file);

    if(!file_space)
        file_space = dataset->shared->space;
    if(!mem_space)
        mem_space = file_space;
    if((snelmts = H5S_GET_SELECT_NPOINTS(mem_space)) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "dst dataspace has invalid selection")
    H5_CHECKED_ASSIGN(nelmts, hsize_t, snelmts, hssize_t);

    /* Set up datatype info for operation */
    if(H5D__typeinfo_init(dataset, mem_type_id, FALSE, &type_info) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to set up type info")
    type_info_init = TRUE;

#ifdef H5_HAVE_PARALLEL
    /* Check for non-MPI-based VFD */
    if(!(H5F_HAS_FEATURE(dataset->oloc.file, H5FD_FEAT_HAS_MPI))) {
        H5FD_mpio_xfer_t io_xfer_mode;      /* MPI I/O transfer mode */

        /* Get I/O transfer mode */
        if(H5CX_get_io_xfer_mode(&io_xfer_mode) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get MPI-I/O transfer mode")

        /* Collective access is not permissible without a MPI based VFD */
        if(io_xfer_mode == H5FD_MPIO_COLLECTIVE)
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "collective access for MPI-based drivers only")
    } /* end if */
#endif /*H5_HAVE_PARALLEL*/

    /* Make certain that the number of elements in each selection is the same */
    if(nelmts != (hsize_t)H5S_GET_SELECT_NPOINTS(file_space))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "src and dest dataspaces have different number of elements selected")

    /* Check for a NULL buffer, after the H5S_ALL dataspace selection has been handled */
    if(NULL == buf) {
        /* Check for any elements selected (which is invalid) */
        if(nelmts > 0)
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no output buffer")

	/* If the buffer is nil, and 0 element is selected, make a fake buffer.
	 * This is for some MPI package like ChaMPIon on NCSA's tungsten which
	 * doesn't support this feature.
	 */
        buf = &fake_char;
    } /* end if */

    /* Make sure that both selections have their extents set */
    if(!(H5S_has_extent(file_space)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file dataspace does not have extent set")
    if(!(H5S_has_extent(mem_space)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "memory dataspace does not have extent set")

    /* H5S_select_shape_same() has been modified to accept topologically identical
     * selections with different rank as having the same shape (if the most 
     * rapidly changing coordinates match up), but the I/O code still has 
     * difficulties with the notion.
     *
     * To solve this, we check to see if H5S_select_shape_same() returns true, 
     * and if the ranks of the mem and file spaces are different.  If the are, 
     * construct a new mem space that is equivalent to the old mem space, and 
     * use that instead.
     *
     * Note that in general, this requires us to touch up the memory buffer as 
     * well.
     */
    if(TRUE == H5S_select_shape_same(mem_space, file_space) &&
            H5S_GET_EXTENT_NDIMS(mem_space) != H5S_GET_EXTENT_NDIMS(file_space)) {
        void *adj_buf = NULL;   /* Pointer to the location in buf corresponding  */
                                /* to the beginning of the projected mem space.  */

        /* Attempt to construct projected dataspace for memory dataspace */
        if(H5S_select_construct_projection(mem_space, &projected_mem_space,
                (unsigned)H5S_GET_EXTENT_NDIMS(file_space), buf, (const void **)&adj_buf, type_info.dst_type_size) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to construct projected memory dataspace")
        HDassert(projected_mem_space);
        HDassert(adj_buf);

        /* Switch to using projected memory dataspace & adjusted buffer */
        mem_space = projected_mem_space;
        buf = adj_buf;
    } /* end if */


    /* Retrieve dataset properties */
    /* <none needed in the general case> */

    /* If space hasn't been allocated and not using external storage,
     * return fill value to buffer if fill time is upon allocation, or
     * do nothing if fill time is never.  If the dataset is compact and
     * fill time is NEVER, there is no way to tell whether part of data
     * has been overwritten.  So just proceed in reading.
     */
    if(nelmts > 0 && dataset->shared->dcpl_cache.efl.nused == 0 &&
            !(*dataset->shared->layout.ops->is_space_alloc)(&dataset->shared->layout.storage) &&
            !(dataset->shared->layout.ops->is_data_cached && (*dataset->shared->layout.ops->is_data_cached)(dataset->shared))) {
        H5D_fill_value_t fill_status;   /* Whether/How the fill value is defined */

        /* Retrieve dataset's fill-value properties */
        if(H5P_is_fill_value_defined(&dataset->shared->dcpl_cache.fill, &fill_status) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't tell if fill value defined")

        /* Should be impossible, but check anyway... */
        if(fill_status == H5D_FILL_VALUE_UNDEFINED &&
                (dataset->shared->dcpl_cache.fill.fill_time == H5D_FILL_TIME_ALLOC || dataset->shared->dcpl_cache.fill.fill_time == H5D_FILL_TIME_IFSET))
            HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "read failed: dataset doesn't exist, no data can be read")

        /* If we're never going to fill this dataset, just leave the junk in the user's buffer */
        if(dataset->shared->dcpl_cache.fill.fill_time == H5D_FILL_TIME_NEVER)
            HGOTO_DONE(SUCCEED)

        /* Go fill the user's selection with the dataset's fill value */
        if(H5D__fill(dataset->shared->dcpl_cache.fill.buf, dataset->shared->type, buf, type_info.mem_type, mem_space) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "filling buf failed")
        else
            HGOTO_DONE(SUCCEED)
    } /* end if */

    /* Set up I/O operation */
    io_info.op_type = H5D_IO_OP_READ;
    io_info.u.rbuf = buf;
    if(H5D__ioinfo_init(dataset, &type_info, &store, &io_info) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "unable to set up I/O operation")

    /* Sanity check that space is allocated, if there are elements */
    if(nelmts > 0)
        HDassert((*dataset->shared->layout.ops->is_space_alloc)(&dataset->shared->layout.storage)
                || (dataset->shared->layout.ops->is_data_cached && (*dataset->shared->layout.ops->is_data_cached)(dataset->shared))
                || dataset->shared->dcpl_cache.efl.nused > 0
                || dataset->shared->layout.type == H5D_COMPACT);

    /* Allocate the chunk map */
    if(NULL == (fm = H5FL_CALLOC(H5D_chunk_map_t)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "can't allocate chunk map")

    /* Call storage method's I/O initialization routine */
    if(io_info.layout_ops.io_init && (*io_info.layout_ops.io_init)(&io_info, &type_info, nelmts, file_space, mem_space, fm) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize I/O info")
    io_op_init = TRUE;

#ifdef H5_HAVE_PARALLEL
    /* Adjust I/O info for any parallel I/O */
    if(H5D__ioinfo_adjust(&io_info, dataset, file_space, mem_space, &type_info) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to adjust I/O info for parallel I/O")
#endif /*H5_HAVE_PARALLEL*/

    /* Invoke correct "high level" I/O routine */
    if((*io_info.io_ops.multi_read)(&io_info, &type_info, nelmts, file_space, mem_space, fm) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "can't read data")

done:
    /* Shut down the I/O op information */
    if(io_op_init && io_info.layout_ops.io_term && (*io_info.layout_ops.io_term)(fm) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTCLOSEOBJ, FAIL, "unable to shut down I/O op info")
    if(fm)
        fm = H5FL_FREE(H5D_chunk_map_t, fm);

    /* Shut down datatype info for operation */
    if(type_info_init && H5D__typeinfo_term(&type_info) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTCLOSEOBJ, FAIL, "unable to shut down type info")

    /* discard projected mem space if it was created */
    if(NULL != projected_mem_space)
        if(H5S_close(projected_mem_space) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTCLOSEOBJ, FAIL, "unable to shut down projected memory dataspace")

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5D__read() */


/*-------------------------------------------------------------------------
 * Function:	H5D__write
 *
 * Purpose:	Writes (part of) a DATASET to a file from application memory
 *		BUF. See H5Dwrite() for complete details.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Thursday, December  4, 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__write(H5D_t *dataset, hid_t mem_type_id, const H5S_t *mem_space,
    const H5S_t *file_space, const void *buf)
{
    H5D_chunk_map_t *fm = NULL;         /* Chunk file<->memory mapping */
    H5D_io_info_t io_info;              /* Dataset I/O info     */
    H5D_type_info_t type_info;          /* Datatype info for operation */
    hbool_t type_info_init = FALSE;     /* Whether the datatype info has been initialized */
    H5S_t * projected_mem_space = NULL; /* If not NULL, ptr to dataspace containing a     */
                                        /* projection of the supplied mem_space to a new  */
                                        /* dataspace with rank equal to that of           */
                                        /* file_space.                                    */
                                        /*                                                */
                                        /* This field is only used if                     */
                                        /* H5S_select_shape_same() returns TRUE when      */
                                        /* comparing the mem_space and the data_space,    */
                                        /* and the mem_space have different rank.         */
                                        /*                                                */
                                        /* Note that if this variable is used, the        */
                                        /* projected mem space must be discarded at the   */
                                        /* end of the function to avoid a memory leak.    */
    H5D_storage_t store;                /*union of EFL and chunk pointer in file space */
    hssize_t	snelmts;                /*total number of elmts	(signed) */
    hsize_t	nelmts;                 /*total number of elmts	*/
    hbool_t     io_op_init = FALSE;     /* Whether the I/O op has been initialized */
    char        fake_char;              /* Temporary variable for NULL buffer pointers */
    herr_t	ret_value = SUCCEED;	/* Return value	*/

    FUNC_ENTER_PACKAGE_TAG(dataset->oloc.addr)

    /* check args */
    HDassert(dataset && dataset->oloc.file);

    /* All filters in the DCPL must have encoding enabled. */
    if(!dataset->shared->checked_filters) {
        if(H5Z_can_apply(dataset->shared->dcpl_id, dataset->shared->type_id) < 0)
            HGOTO_ERROR(H5E_PLINE, H5E_CANAPPLY, FAIL, "can't apply filters")

        dataset->shared->checked_filters = TRUE;
    } /* end if */

    /* Check if we are allowed to write to this file */
    if(0 == (H5F_INTENT(dataset->oloc.file) & H5F_ACC_RDWR))
        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "no write intent on file")

    /* Set up datatype info for operation */
    if(H5D__typeinfo_init(dataset, mem_type_id, TRUE, &type_info) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to set up type info")
    type_info_init = TRUE;

    /* Various MPI based checks */
#ifdef H5_HAVE_PARALLEL
    if(H5F_HAS_FEATURE(dataset->oloc.file, H5FD_FEAT_HAS_MPI)) {
        /* If MPI based VFD is used, no VL or region reference datatype support yet. */
        /* This is because they use the global heap in the file and we don't */
        /* support parallel access of that yet */
        if(H5T_is_vl_storage(type_info.mem_type) > 0)
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "Parallel IO does not support writing VL or region reference datatypes yet")
    } /* end if */
    else {
        H5FD_mpio_xfer_t io_xfer_mode;      /* MPI I/O transfer mode */

        /* Get I/O transfer mode */
        if(H5CX_get_io_xfer_mode(&io_xfer_mode) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get MPI-I/O transfer mode")

        /* Collective access is not permissible without a MPI based VFD */
        if(io_xfer_mode == H5FD_MPIO_COLLECTIVE)
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "collective access for MPI-based driver only")
    } /* end else */
#endif /*H5_HAVE_PARALLEL*/

    /* Initialize dataspace information */
    if(!file_space)
        file_space = dataset->shared->space;
    if(!mem_space)
        mem_space = file_space;

    if((snelmts = H5S_GET_SELECT_NPOINTS(mem_space)) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "src dataspace has invalid selection")
    H5_CHECKED_ASSIGN(nelmts, hsize_t, snelmts, hssize_t);

    /* Make certain that the number of elements in each selection is the same */
    if(nelmts != (hsize_t)H5S_GET_SELECT_NPOINTS(file_space))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "src and dest dataspaces have different number of elements selected")

    /* Check for a NULL buffer, after the H5S_ALL dataspace selection has been handled */
    if(NULL == buf) {
        /* Check for any elements selected (which is invalid) */
        if(nelmts > 0)
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no output buffer")

	/* If the buffer is nil, and 0 element is selected, make a fake buffer.
	 * This is for some MPI package like ChaMPIon on NCSA's tungsten which
	 * doesn't support this feature.
	 */
        buf = &fake_char;
    } /* end if */

    /* Make sure that both selections have their extents set */
    if(!(H5S_has_extent(file_space)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file dataspace does not have extent set")
    if(!(H5S_has_extent(mem_space)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "memory dataspace does not have extent set")

    /* H5S_select_shape_same() has been modified to accept topologically 
     * identical selections with different rank as having the same shape 
     * (if the most rapidly changing coordinates match up), but the I/O 
     * code still has difficulties with the notion.
     *
     * To solve this, we check to see if H5S_select_shape_same() returns 
     * true, and if the ranks of the mem and file spaces are different.  
     * If the are, construct a new mem space that is equivalent to the 
     * old mem space, and use that instead.
     *
     * Note that in general, this requires us to touch up the memory buffer 
     * as well.
     */
    if(TRUE == H5S_select_shape_same(mem_space, file_space) &&
            H5S_GET_EXTENT_NDIMS(mem_space) != H5S_GET_EXTENT_NDIMS(file_space)) {
        void *adj_buf = NULL;   /* Pointer to the location in buf corresponding  */
                                /* to the beginning of the projected mem space.  */

        /* Attempt to construct projected dataspace for memory dataspace */
        if(H5S_select_construct_projection(mem_space, &projected_mem_space,
                (unsigned)H5S_GET_EXTENT_NDIMS(file_space), buf, (const void **)&adj_buf, type_info.src_type_size) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to construct projected memory dataspace")
        HDassert(projected_mem_space);
        HDassert(adj_buf);

        /* Switch to using projected memory dataspace & adjusted buffer */
        mem_space = projected_mem_space;
        buf = adj_buf;
    } /* end if */

    /* Retrieve dataset properties */
    /* <none needed currently> */

    /* Set up I/O operation */
    io_info.op_type = H5D_IO_OP_WRITE;
    io_info.u.wbuf = buf;
    if(H5D__ioinfo_init(dataset, &type_info, &store, &io_info) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to set up I/O operation")

    /* Allocate dataspace and initialize it if it hasn't been. */
    if(nelmts > 0 && dataset->shared->dcpl_cache.efl.nused == 0 &&
            !(*dataset->shared->layout.ops->is_space_alloc)(&dataset->shared->layout.storage)) {
        hssize_t file_nelmts;   /* Number of elements in file dataset's dataspace */
        hbool_t full_overwrite; /* Whether we are over-writing all the elements */

        /* Get the number of elements in file dataset's dataspace */
        if((file_nelmts = H5S_GET_EXTENT_NPOINTS(file_space)) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, FAIL, "can't retrieve number of elements in file dataset")

        /* Always allow fill values to be written if the dataset has a VL datatype */
        if(H5T_detect_class(dataset->shared->type, H5T_VLEN, FALSE))
            full_overwrite = FALSE;
        else
            full_overwrite = (hbool_t)((hsize_t)file_nelmts == nelmts ? TRUE : FALSE);

        /* Allocate storage */
        if(H5D__alloc_storage(&io_info, H5D_ALLOC_WRITE, full_overwrite, NULL) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize storage")
    } /* end if */

    /* Allocate the chunk map */
    if(NULL == (fm = H5FL_CALLOC(H5D_chunk_map_t)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "can't allocate chunk map")

    /* Call storage method's I/O initialization routine */
    if(io_info.layout_ops.io_init && (*io_info.layout_ops.io_init)(&io_info, &type_info, nelmts, file_space, mem_space, fm) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize I/O info")
    io_op_init = TRUE;

#ifdef H5_HAVE_PARALLEL
    /* Adjust I/O info for any parallel I/O */
    if(H5D__ioinfo_adjust(&io_info, dataset, file_space, mem_space, &type_info) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to adjust I/O info for parallel I/O")
#endif /*H5_HAVE_PARALLEL*/

    /* Invoke correct "high level" I/O routine */
    if((*io_info.io_ops.multi_write)(&io_info, &type_info, nelmts, file_space, mem_space, fm) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "can't write data")

#ifdef OLD_WAY
/*
 * This was taken out because it can be called in a parallel program with
 * independent access, causing the metadata cache to get corrupted. Its been
 * disabled for all types of access (serial as well as parallel) to make the
 * modification time consistent for all programs. -QAK
 *
 * We should set a value in the dataset's shared information instead and flush
 * it to the file when the dataset is being closed. -QAK
 */
    /*
     * Update modification time.  We have to do this explicitly because
     * writing to a dataset doesn't necessarily change the object header.
     */
    if(H5O_touch(&(dataset->oloc), FALSE) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update modification time")
#endif /* OLD_WAY */

done:
    /* Shut down the I/O op information */
    if(io_op_init && io_info.layout_ops.io_term && (*io_info.layout_ops.io_term)(fm) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTCLOSEOBJ, FAIL, "unable to shut down I/O op info")
    if(fm)
        fm = H5FL_FREE(H5D_chunk_map_t, fm);

    /* Shut down datatype info for operation */
    if(type_info_init && H5D__typeinfo_term(&type_info) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTCLOSEOBJ, FAIL, "unable to shut down type info")

    /* discard projected mem space if it was created */
    if(NULL != projected_mem_space)
        if(H5S_close(projected_mem_space) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTCLOSEOBJ, FAIL, "unable to shut down projected memory dataspace")

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5D__write() */


/*-------------------------------------------------------------------------
 * Function:	H5D__ioinfo_init
 *
 * Purpose:	Routine for determining correct I/O operations for
 *              each I/O action.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Thursday, September 30, 2004
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__ioinfo_init(H5D_t *dset, const H5D_type_info_t *type_info,
    H5D_storage_t *store, H5D_io_info_t *io_info)
{
    FUNC_ENTER_STATIC_NOERR

    /* check args */
    HDassert(dset);
    HDassert(dset->oloc.file);
    HDassert(type_info);
    HDassert(type_info->tpath);
    HDassert(io_info);

    /* Set up "normal" I/O fields */
    io_info->dset = dset;
    io_info->store = store;

    /* Set I/O operations to initial values */
    io_info->layout_ops = *dset->shared->layout.ops;

    /* Set the "high-level" I/O operations for the dataset */
    io_info->io_ops.multi_read = dset->shared->layout.ops->ser_read;
    io_info->io_ops.multi_write = dset->shared->layout.ops->ser_write;

    /* Set the I/O operations for reading/writing single blocks on disk */
    if(type_info->is_xform_noop && type_info->is_conv_noop) {
        /*
         * If there is no data transform or type conversion then read directly into
         *  the application's buffer.  This saves at least one mem-to-mem copy.
         */
        io_info->io_ops.single_read = H5D__select_read;
        io_info->io_ops.single_write = H5D__select_write;
    } /* end if */
    else {
        /*
         * This is the general case (type conversion, usually).
         */
        io_info->io_ops.single_read = H5D__scatgath_read;
        io_info->io_ops.single_write = H5D__scatgath_write;
    } /* end else */

#ifdef H5_HAVE_PARALLEL
    /* Determine if the file was opened with an MPI VFD */
    io_info->using_mpi_vfd = H5F_HAS_FEATURE(dset->oloc.file, H5FD_FEAT_HAS_MPI);
#endif /* H5_HAVE_PARALLEL */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D__ioinfo_init() */


/*-------------------------------------------------------------------------
 * Function:	H5D__typeinfo_init
 *
 * Purpose:	Routine for determining correct datatype information for
 *              each I/O action.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, March  4, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__typeinfo_init(const H5D_t *dset, hid_t mem_type_id, hbool_t do_write,
    H5D_type_info_t *type_info)
{
    H5T_t	*src_type;              /* Source datatype */
    H5T_t	*dst_type;              /* Destination datatype */
    H5Z_data_xform_t *data_transform;   /* Data transform info */
    herr_t ret_value = SUCCEED;	        /* Return value	*/

    FUNC_ENTER_STATIC

    /* check args */
    HDassert(type_info);
    HDassert(dset);

    /* Patch the top level file pointer for dt->shared->u.vlen.f if needed */
    if(H5T_patch_vlen_file(dset->shared->type, dset->oloc.file) < 0 )
        HGOTO_ERROR(H5E_DATASET, H5E_CANTOPENOBJ, FAIL, "can't patch VL datatype file pointer")

    /* Initialize type info safely */
    HDmemset(type_info, 0, sizeof(*type_info));

    /* Get the memory & dataset datatypes */
    if(NULL == (type_info->mem_type = (H5T_t *)H5I_object_verify(mem_type_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    type_info->dset_type = dset->shared->type;

    if(do_write) {
        src_type = type_info->mem_type;
        dst_type = dset->shared->type;
        type_info->src_type_id = mem_type_id;
        type_info->dst_type_id = dset->shared->type_id;
    } /* end if */
    else {
        src_type = dset->shared->type;
        dst_type = type_info->mem_type;
        type_info->src_type_id = dset->shared->type_id;
        type_info->dst_type_id = mem_type_id;
    } /* end else */

    /*
     * Locate the type conversion function and dataspace conversion
     * functions, and set up the element numbering information. If a data
     * type conversion is necessary then register datatype atoms. Data type
     * conversion is necessary if the user has set the `need_bkg' to a high
     * enough value in xfer_parms since turning off datatype conversion also
     * turns off background preservation.
     */
    if(NULL == (type_info->tpath = H5T_path_find(src_type, dst_type)))
        HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "unable to convert between src and dest datatype")

    /* Retrieve info from API context */
    if(H5CX_get_data_transform(&data_transform) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get data transform info")

    /* Precompute some useful information */
    type_info->src_type_size = H5T_get_size(src_type);
    type_info->dst_type_size = H5T_get_size(dst_type);
    type_info->max_type_size = MAX(type_info->src_type_size, type_info->dst_type_size);
    type_info->is_conv_noop = H5T_path_noop(type_info->tpath);
    type_info->is_xform_noop = H5Z_xform_noop(data_transform);
    if(type_info->is_xform_noop && type_info->is_conv_noop) {
        type_info->cmpd_subset = NULL;
        type_info->need_bkg = H5T_BKG_NO;
    } /* end if */
    else {
        void	*tconv_buf;		/* Temporary conversion buffer pointer */
        void	*bkgr_buf;		/* Background conversion buffer pointer */
        size_t	max_temp_buf;		/* Maximum temporary buffer size */
        H5T_bkg_t bkgr_buf_type;        /* Background buffer type */
        size_t	target_size;		/* Desired buffer size	*/

        /* Get info from API context */
        if(H5CX_get_max_temp_buf(&max_temp_buf) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't retrieve max. temp. buf size")
        if(H5CX_get_tconv_buf(&tconv_buf) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't retrieve temp. conversion buffer pointer")
        if(H5CX_get_bkgr_buf(&bkgr_buf) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't retrieve background conversion buffer pointer")
        if(H5CX_get_bkgr_buf_type(&bkgr_buf_type) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't retrieve background buffer type")

        /* Check if the datatypes are compound subsets of one another */
        type_info->cmpd_subset = H5T_path_compound_subset(type_info->tpath);

        /* Check if we need a background buffer */
        if(do_write && H5T_detect_class(dset->shared->type, H5T_VLEN, FALSE))
            type_info->need_bkg = H5T_BKG_YES;
        else {
            H5T_bkg_t path_bkg;     /* Type conversion's background info */

            if((path_bkg = H5T_path_bkg(type_info->tpath))) {
                /* Retrieve the bkgr buffer property */
                type_info->need_bkg = bkgr_buf_type;
                type_info->need_bkg = MAX(path_bkg, type_info->need_bkg);
            } /* end if */
            else
                type_info->need_bkg = H5T_BKG_NO; /*never needed even if app says yes*/
        } /* end else */


        /* Set up datatype conversion/background buffers */

        target_size = max_temp_buf;

        /* If the buffer is too small to hold even one element, try to make it bigger */
        if(target_size < type_info->max_type_size) {
            hbool_t default_buffer_info;    /* Whether the buffer information are the defaults */

            /* Detect if we have all default settings for buffers */
            default_buffer_info = (hbool_t)((H5D_TEMP_BUF_SIZE == max_temp_buf)
                    && (NULL == tconv_buf) && (NULL == bkgr_buf));

            /* Check if we are using the default buffer info */
            if(default_buffer_info)
                /* OK to get bigger for library default settings */
                target_size = type_info->max_type_size;
            else
                /* Don't get bigger than the application has requested */
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "temporary buffer max size is too small")
        } /* end if */

        /* Compute the number of elements that will fit into buffer */
        type_info->request_nelmts = target_size / type_info->max_type_size;

        /* Sanity check elements in temporary buffer */
        if(type_info->request_nelmts == 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "temporary buffer max size is too small")

        /* Get a temporary buffer for type conversion unless the app has already
         * supplied one through the xfer properties. Instead of allocating a
         * buffer which is the exact size, we allocate the target size.
         */
        if(NULL == (type_info->tconv_buf = (uint8_t *)tconv_buf)) {
            /* Allocate temporary buffer */
            if(NULL == (type_info->tconv_buf = H5FL_BLK_CALLOC(type_conv, target_size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for type conversion")
            type_info->tconv_buf_allocated = TRUE;
        } /* end if */
        if(type_info->need_bkg && NULL == (type_info->bkg_buf = (uint8_t *)bkgr_buf)) {
            size_t	bkg_size;		/* Desired background buffer size	*/

            /* Compute the background buffer size */
            /* (don't try to use buffers smaller than the default size) */
            bkg_size = type_info->request_nelmts * type_info->dst_type_size;
            if(bkg_size < max_temp_buf)
                bkg_size = max_temp_buf;

            /* Allocate background buffer */
            /* (Need calloc()-like call since memory needs to be initialized) */
            if(NULL == (type_info->bkg_buf = H5FL_BLK_CALLOC(type_conv, bkg_size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for background conversion")
            type_info->bkg_buf_allocated = TRUE;
        } /* end if */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__typeinfo_init() */

#ifdef H5_HAVE_PARALLEL

/*-------------------------------------------------------------------------
 * Function:	H5D__ioinfo_adjust
 *
 * Purpose:	Adjust operation's I/O info for any parallel I/O
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Thursday, March 27, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__ioinfo_adjust(H5D_io_info_t *io_info, const H5D_t *dset,
    const H5S_t *file_space, const H5S_t *mem_space,
    const H5D_type_info_t *type_info)
{
    herr_t	ret_value = SUCCEED;	/* Return value	*/

    FUNC_ENTER_STATIC

    /* check args */
    HDassert(dset);
    HDassert(dset->oloc.file);
    HDassert(mem_space);
    HDassert(file_space);
    HDassert(type_info);
    HDassert(type_info->tpath);
    HDassert(io_info);

    /* Reset the actual io mode properties to the default values in case
     * the DXPL (if it's non-default) was previously used in a collective
     * I/O operation.
     */
    if(!H5CX_is_def_dxpl()) {
        H5CX_set_mpio_actual_chunk_opt(H5D_MPIO_NO_CHUNK_OPTIMIZATION);
        H5CX_set_mpio_actual_io_mode(H5D_MPIO_NO_COLLECTIVE);
    } /* end if */

    /* Make any parallel I/O adjustments */
    if(io_info->using_mpi_vfd) {
        H5FD_mpio_xfer_t xfer_mode; /* Parallel transfer for this request */
        htri_t opt;         /* Flag whether a selection is optimizable */

        /* Get the original state of parallel I/O transfer mode */
        if(H5CX_get_io_xfer_mode(&xfer_mode) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get MPI-I/O transfer mode")

        /* Get MPI communicator */
        if(MPI_COMM_NULL == (io_info->comm = H5F_mpi_get_comm(dset->oloc.file)))
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't retrieve MPI communicator")

        /* Check if we can set direct MPI-IO read/write functions */
        if((opt = H5D__mpio_opt_possible(io_info, file_space, mem_space, type_info)) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "invalid check for direct IO dataspace ")

        /* Check if we can use the optimized parallel I/O routines */
        if(opt == TRUE) {
            /* Override the I/O op pointers to the MPI-specific routines */
            io_info->io_ops.multi_read = dset->shared->layout.ops->par_read;
            io_info->io_ops.multi_write = dset->shared->layout.ops->par_write;
            io_info->io_ops.single_read = H5D__mpio_select_read;
            io_info->io_ops.single_write = H5D__mpio_select_write;
        } /* end if */
        else {
            /* Check if there are any filters in the pipeline. If there are,
             * we cannot break to independent I/O if this is a write operation;
             * otherwise there will be metadata inconsistencies in the file.
             */
            if (io_info->op_type == H5D_IO_OP_WRITE && io_info->dset->shared->dcpl_cache.pline.nused > 0) {
                H5D_mpio_no_collective_cause_t  cause;
                uint32_t                        local_no_collective_cause;
                uint32_t                        global_no_collective_cause;
                hbool_t                         local_error_message_previously_written = FALSE;
                hbool_t                         global_error_message_previously_written = FALSE;
                size_t                          index;
                size_t                          cause_strings_len;
                char                            local_no_collective_cause_string[512] = "";
                char                            global_no_collective_cause_string[512] = "";
                const char                     *cause_strings[] = { "independent I/O was requested",
                                                                    "datatype conversions were required",
                                                                    "data transforms needed to be applied",
                                                                    "optimized MPI types flag wasn't set",
                                                                    "one of the dataspaces was neither simple nor scalar",
                                                                    "dataset was not contiguous or chunked",
                                                                    "parallel writes to filtered datasets are disabled",
                                                                    "an error occurred while checking if collective I/O was possible" };

                cause_strings_len = sizeof(cause_strings) / sizeof(cause_strings[0]);

                if(H5CX_get_mpio_local_no_coll_cause(&local_no_collective_cause) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to get local no collective cause value")
                if(H5CX_get_mpio_global_no_coll_cause(&global_no_collective_cause) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to get global no collective cause value")

                /* Append each of the "reason for breaking collective I/O" error messages to the
                 * local and global no collective cause strings */
                for (cause = 1, index = 0; (cause < H5D_MPIO_NO_COLLECTIVE_MAX_CAUSE) && (index < cause_strings_len); cause <<= 1, index++) {
                    size_t cause_strlen = HDstrlen(cause_strings[index]);

                    if (cause & local_no_collective_cause) {
                        /* Check if there were any previous error messages included. If so, prepend a semicolon
                         * to separate the messages.
                         */
                        if(local_error_message_previously_written)
                            HDstrncat(local_no_collective_cause_string, "; ", 2);

                        HDstrncat(local_no_collective_cause_string, cause_strings[index], cause_strlen);

                        local_error_message_previously_written = TRUE;
                    } /* end if */

                    if(cause & global_no_collective_cause) {
                        /* Check if there were any previous error messages included. If so, prepend a semicolon
                         * to separate the messages.
                         */
                        if(global_error_message_previously_written)
                            HDstrncat(global_no_collective_cause_string, "; ", 2);

                        HDstrncat(global_no_collective_cause_string, cause_strings[index], cause_strlen);

                        global_error_message_previously_written = TRUE;
                    } /* end if */
                } /* end for */

                HGOTO_ERROR(H5E_IO, H5E_NO_INDEPENDENT, FAIL, "Can't perform independent write with filters in pipeline.\n"
                                                              "    The following caused a break from collective I/O:\n"
                                                              "        Local causes: %s\n"
                                                              "        Global causes: %s",
                                                              local_no_collective_cause_string,
                                                              global_no_collective_cause_string);
            } /* end if */

            /* If we won't be doing collective I/O, but the user asked for
             * collective I/O, change the request to use independent I/O
             */
            if(xfer_mode == H5FD_MPIO_COLLECTIVE) {
                /* Change the xfer_mode to independent for handling the I/O */
                if(H5CX_set_io_xfer_mode(H5FD_MPIO_INDEPENDENT) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set MPI-I/O transfer mode")
            } /* end if */
        } /* end else */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__ioinfo_adjust() */
#endif /*H5_HAVE_PARALLEL*/


/*-------------------------------------------------------------------------
 * Function:	H5D__typeinfo_term
 *
 * Purpose:	Common logic for terminating a type info object
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Thursday, March  6, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__typeinfo_term(const H5D_type_info_t *type_info)
{
    FUNC_ENTER_STATIC_NOERR

    /* Check for releasing datatype conversion & background buffers */
    if(type_info->tconv_buf_allocated) {
        HDassert(type_info->tconv_buf);
        (void)H5FL_BLK_FREE(type_conv, type_info->tconv_buf);
    } /* end if */
    if(type_info->bkg_buf_allocated) {
        HDassert(type_info->bkg_buf);
        (void)H5FL_BLK_FREE(type_conv, type_info->bkg_buf);
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D__typeinfo_term() */

