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

/* Internal I/O routines */
static herr_t H5D_read(H5D_t *dataset, hid_t mem_type_id,
    const H5S_t *mem_space, const H5S_t *file_space, hid_t dset_xfer_plist,
    void *buf/*out*/);
static herr_t H5D_write(H5D_t *dataset, hid_t mem_type_id,
    const H5S_t *mem_space, const H5S_t *file_space, hid_t dset_xfer_plist,
    const void *buf);

/* Setup/teardown routines */
static herr_t H5D_ioinfo_init(H5D_t *dset, const H5D_dxpl_cache_t *dxpl_cache,
    hid_t dxpl_id, const H5D_type_info_t *type_info, H5D_storage_t *store,
    H5D_io_info_t *io_info);
static herr_t H5D_typeinfo_init(const H5D_t *dset, const H5D_dxpl_cache_t *dxpl_cache,
    hid_t dxpl_id, hid_t mem_type_id, hbool_t do_write,
    H5D_type_info_t *type_info);
#ifdef H5_HAVE_PARALLEL
static herr_t H5D_ioinfo_adjust(H5D_io_info_t *io_info, const H5D_t *dset,
    hid_t dxpl_id, const H5S_t *file_space, const H5S_t *mem_space,
    const H5D_type_info_t *type_info, const H5D_chunk_map_t *fm);
static herr_t H5D_ioinfo_term(H5D_io_info_t *io_info);
#endif /* H5_HAVE_PARALLEL */
static herr_t H5D_typeinfo_term(const H5D_type_info_t *type_info);


/*********************/
/* Package Variables */
/*********************/


/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage blocks of type conversion data */
H5FL_BLK_DEFINE(type_conv);



/*-------------------------------------------------------------------------
 * Function:	H5Dread
 *
 * Purpose:	Reads (part of) a DSET from the file into application
 *		memory BUF. The part of the dataset to read is defined with
 *		MEM_SPACE_ID and FILE_SPACE_ID.	 The data points are
 *		converted from their file type to the MEM_TYPE_ID specified.
 *		Additional miscellaneous data transfer properties can be
 *		passed to this function with the PLIST_ID argument.
 *
 *		The FILE_SPACE_ID can be the constant H5S_ALL which indicates
 *		that the entire file data space is to be referenced.
 *
 *		The MEM_SPACE_ID can be the constant H5S_ALL in which case
 *		the memory data space is the same as the file data space
 *		defined when the dataset was created.
 *
 *		The number of elements in the memory data space must match
 *		the number of elements in the file data space.
 *
 *		The PLIST_ID can be the constant H5P_DEFAULT in which
 *		case the default data transfer properties are used.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Thursday, December  4, 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dread(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id,
	hid_t file_space_id, hid_t plist_id, void *buf/*out*/)
{
    H5D_t		   *dset = NULL;
    const H5S_t		   *mem_space = NULL;
    const H5S_t		   *file_space = NULL;
    char                    fake_char;
    herr_t                  ret_value = SUCCEED;  /* Return value */

    FUNC_ENTER_API(H5Dread, FAIL)
    H5TRACE6("e", "iiiiix", dset_id, mem_type_id, mem_space_id, file_space_id,
             plist_id, buf);

    /* check arguments */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
    if(NULL == dset->oloc.file)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
    if(H5S_ALL != mem_space_id) {
	if(NULL == (mem_space = (const H5S_t *)H5I_object_verify(mem_space_id, H5I_DATASPACE)))
	    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space")

	/* Check for valid selection */
	if(H5S_SELECT_VALID(mem_space) != TRUE)
	    HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "selection+offset not within extent")
    } /* end if */
    if(H5S_ALL != file_space_id) {
	if(NULL == (file_space = (const H5S_t *)H5I_object_verify(file_space_id, H5I_DATASPACE)))
	    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space")

	/* Check for valid selection */
	if(H5S_SELECT_VALID(file_space) != TRUE)
	    HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "selection+offset not within extent")
    } /* end if */

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == plist_id)
        plist_id= H5P_DATASET_XFER_DEFAULT;
    else
        if(TRUE != H5P_isa_class(plist_id, H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")
    if(!buf && (NULL == file_space || H5S_GET_SELECT_NPOINTS(file_space) != 0))
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no output buffer")

    /* If the buffer is nil, and 0 element is selected, make a fake buffer.
     * This is for some MPI package like ChaMPIon on NCSA's tungsten which
     * doesn't support this feature.
     */
    if(!buf)
        buf = &fake_char;

    /* read raw data */
    if(H5D_read(dset, mem_type_id, mem_space, file_space, plist_id, buf/*out*/) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "can't read data")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dread() */


/*-------------------------------------------------------------------------
 * Function:	H5Dwrite
 *
 * Purpose:	Writes (part of) a DSET from application memory BUF to the
 *		file.  The part of the dataset to write is defined with the
 *		MEM_SPACE_ID and FILE_SPACE_ID arguments. The data points
 *		are converted from their current type (MEM_TYPE_ID) to their
 *		file datatype.	 Additional miscellaneous data transfer
 *		properties can be passed to this function with the
 *		PLIST_ID argument.
 *
 *		The FILE_SPACE_ID can be the constant H5S_ALL which indicates
 *		that the entire file data space is to be referenced.
 *
 *		The MEM_SPACE_ID can be the constant H5S_ALL in which case
 *		the memory data space is the same as the file data space
 *		defined when the dataset was created.
 *
 *		The number of elements in the memory data space must match
 *		the number of elements in the file data space.
 *
 *		The PLIST_ID can be the constant H5P_DEFAULT in which
 *		case the default data transfer properties are used.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Thursday, December  4, 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dwrite(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id,
	 hid_t file_space_id, hid_t dxpl_id, const void *buf)
{
    H5D_t		   *dset = NULL;
    const H5S_t		   *mem_space = NULL;
    const H5S_t		   *file_space = NULL;
    char                    fake_char;
    herr_t                  ret_value = SUCCEED;  /* Return value */

    FUNC_ENTER_API(H5Dwrite, FAIL)
    H5TRACE6("e", "iiiii*x", dset_id, mem_type_id, mem_space_id, file_space_id,
             dxpl_id, buf);

    /* check arguments */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
    if(NULL == dset->oloc.file)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")
    if(H5S_ALL != mem_space_id) {
	if(NULL == (mem_space = (const H5S_t *)H5I_object_verify(mem_space_id, H5I_DATASPACE)))
	    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space")

	/* Check for valid selection */
	if(H5S_SELECT_VALID(mem_space) != TRUE)
	    HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "memory selection+offset not within extent")
    } /* end if */
    if(H5S_ALL != file_space_id) {
	if(NULL == (file_space = (const H5S_t *)H5I_object_verify(file_space_id, H5I_DATASPACE)))
	    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space")

	/* Check for valid selection */
	if(H5S_SELECT_VALID(file_space) != TRUE)
	    HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "file selection+offset not within extent")
    } /* end if */

    /* Get the default dataset transfer property list if the user didn't provide one */
    if(H5P_DEFAULT == dxpl_id)
        dxpl_id= H5P_DATASET_XFER_DEFAULT;
    else
        if(TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")
    if(!buf && (NULL == file_space || H5S_GET_SELECT_NPOINTS(file_space) != 0))
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no output buffer")

    /* If the buffer is nil, and 0 element is selected, make a fake buffer.
     * This is for some MPI package like ChaMPIon on NCSA's tungsten which
     * doesn't support this feature.
     */
    if(!buf)
        buf = &fake_char;

    /* write raw data */
    if(H5D_write(dset, mem_type_id, mem_space, file_space, dxpl_id, buf) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "can't write data")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dwrite() */


/*-------------------------------------------------------------------------
 * Function:	H5D_read
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
static herr_t
H5D_read(H5D_t *dataset, hid_t mem_type_id, const H5S_t *mem_space,
	 const H5S_t *file_space, hid_t dxpl_id, void *buf/*out*/)
{
    H5D_chunk_map_t fm;                 /* Chunk file<->memory mapping */
    H5D_io_info_t io_info;              /* Dataset I/O info     */
    H5D_type_info_t type_info;          /* Datatype info for operation */
    hbool_t type_info_init = FALSE;     /* Whether the datatype info has been initialized */
    H5D_storage_t store;                /*union of EFL and chunk pointer in file space */
    hssize_t	snelmts;                /*total number of elmts	(signed) */
    hsize_t	nelmts;                 /*total number of elmts	*/
#ifdef H5_HAVE_PARALLEL
    hbool_t     io_info_init = FALSE;   /* Whether the I/O info has been initialized */
#endif /*H5_HAVE_PARALLEL*/
    hbool_t     io_op_init = FALSE;     /* Whether the I/O op has been initialized */
    H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
    H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
    herr_t	ret_value = SUCCEED;	/* Return value	*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_read)

    /* check args */
    HDassert(dataset && dataset->oloc.file);

    if(!file_space)
        file_space = dataset->shared->space;
    if(!mem_space)
        mem_space = file_space;
    if((snelmts = H5S_GET_SELECT_NPOINTS(mem_space)) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "dst dataspace has invalid selection")
    H5_ASSIGN_OVERFLOW(nelmts,snelmts,hssize_t,hsize_t);

    /* Fill the DXPL cache values for later use */
    if(H5D_get_dxpl_cache(dxpl_id, &dxpl_cache) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

    /* Set up datatype info for operation */
    if(H5D_typeinfo_init(dataset, dxpl_cache, dxpl_id, mem_type_id, FALSE, &type_info) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to set up type info")
    type_info_init = TRUE;

#ifdef H5_HAVE_PARALLEL
    /* Collective access is not permissible without a MPI based VFD */
    if(dxpl_cache->xfer_mode == H5FD_MPIO_COLLECTIVE && !IS_H5FD_MPI(dataset->oloc.file))
        HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "collective access for MPI-based drivers only")
#endif /*H5_HAVE_PARALLEL*/

    /* Make certain that the number of elements in each selection is the same */
    if(nelmts != (hsize_t)H5S_GET_SELECT_NPOINTS(file_space))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "src and dest data spaces have different sizes")

    /* Make sure that both selections have their extents set */
    if(!(H5S_has_extent(file_space)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file dataspace does not have extent set")
    if(!(H5S_has_extent(mem_space)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "memory dataspace does not have extent set")

    /* Retrieve dataset properties */
    /* <none needed in the general case> */

    /* If space hasn't been allocated and not using external storage,
     * return fill value to buffer if fill time is upon allocation, or
     * do nothing if fill time is never.  If the dataset is compact and
     * fill time is NEVER, there is no way to tell whether part of data
     * has been overwritten.  So just proceed in reading.
     */
    if(nelmts > 0 && dataset->shared->dcpl_cache.efl.nused == 0 &&
            !(*dataset->shared->layout.ops->is_space_alloc)(&dataset->shared->layout.storage)) {
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
        if(H5D_fill(dataset->shared->dcpl_cache.fill.buf, dataset->shared->type, buf, type_info.mem_type, mem_space, dxpl_id) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "filling buf failed")
        else
            HGOTO_DONE(SUCCEED)
    } /* end if */

    /* Set up I/O operation */
    io_info.op_type = H5D_IO_OP_READ;
    io_info.u.rbuf = buf;
    if(H5D_ioinfo_init(dataset, dxpl_cache, dxpl_id, &type_info, &store, &io_info) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "unable to set up I/O operation")
#ifdef H5_HAVE_PARALLEL
    io_info_init = TRUE;
#endif /*H5_HAVE_PARALLEL*/

    /* Sanity check that space is allocated, if there are elements */
    if(nelmts > 0)
        HDassert((*dataset->shared->layout.ops->is_space_alloc)(&dataset->shared->layout.storage)
                || dataset->shared->dcpl_cache.efl.nused > 0
                || dataset->shared->layout.type == H5D_COMPACT);

    /* Call storage method's I/O initialization routine */
    if(io_info.layout_ops.io_init && (*io_info.layout_ops.io_init)(&io_info, &type_info, nelmts, file_space, mem_space, &fm) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize I/O info")
    io_op_init = TRUE;

#ifdef H5_HAVE_PARALLEL
    /* Adjust I/O info for any parallel I/O */
    if(H5D_ioinfo_adjust(&io_info, dataset, dxpl_id, file_space, mem_space, &type_info, &fm) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to adjust I/O info for parallel I/O")
#endif /*H5_HAVE_PARALLEL*/

    /* Invoke correct "high level" I/O routine */
    if((*io_info.io_ops.multi_read)(&io_info, &type_info, nelmts, file_space, mem_space, &fm) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "can't read data")

done:
    /* Shut down the I/O op information */
    if(io_op_init && io_info.layout_ops.io_term && (*io_info.layout_ops.io_term)(&fm) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTCLOSEOBJ, FAIL, "unable to shut down I/O op info")
#ifdef H5_HAVE_PARALLEL
    /* Shut down io_info struct */
    if(io_info_init)
        if(H5D_ioinfo_term(&io_info) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CANTCLOSEOBJ, FAIL, "can't shut down io_info")
#endif /*H5_HAVE_PARALLEL*/
    /* Shut down datatype info for operation */
    if(type_info_init && H5D_typeinfo_term(&type_info) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTCLOSEOBJ, FAIL, "unable to shut down type info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_read() */


/*-------------------------------------------------------------------------
 * Function:	H5D_write
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
static herr_t
H5D_write(H5D_t *dataset, hid_t mem_type_id, const H5S_t *mem_space,
	  const H5S_t *file_space, hid_t dxpl_id, const void *buf)
{
    H5D_chunk_map_t fm;                 /* Chunk file<->memory mapping */
    H5D_io_info_t io_info;              /* Dataset I/O info     */
    H5D_type_info_t type_info;          /* Datatype info for operation */
    hbool_t type_info_init = FALSE;     /* Whether the datatype info has been initialized */
    H5D_storage_t store;                /*union of EFL and chunk pointer in file space */
    hssize_t	snelmts;                /*total number of elmts	(signed) */
    hsize_t	nelmts;                 /*total number of elmts	*/
#ifdef H5_HAVE_PARALLEL
    hbool_t     io_info_init = FALSE;   /* Whether the I/O info has been initialized */
#endif /*H5_HAVE_PARALLEL*/
    hbool_t     io_op_init = FALSE;     /* Whether the I/O op has been initialized */
    H5D_dxpl_cache_t _dxpl_cache;       /* Data transfer property cache buffer */
    H5D_dxpl_cache_t *dxpl_cache = &_dxpl_cache;   /* Data transfer property cache */
    herr_t	ret_value = SUCCEED;	/* Return value	*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_write)

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

    /* Fill the DXPL cache values for later use */
    if(H5D_get_dxpl_cache(dxpl_id, &dxpl_cache) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't fill dxpl cache")

    /* Set up datatype info for operation */
    if(H5D_typeinfo_init(dataset, dxpl_cache, dxpl_id, mem_type_id, TRUE, &type_info) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to set up type info")
    type_info_init = TRUE;

    /* Various MPI based checks */
#ifdef H5_HAVE_PARALLEL
    if(IS_H5FD_MPI(dataset->oloc.file)) {
        /* If MPI based VFD is used, no VL datatype support yet. */
        /* This is because they use the global heap in the file and we don't */
        /* support parallel access of that yet */
        if(H5T_detect_class(type_info.mem_type, H5T_VLEN, FALSE) > 0)
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "Parallel IO does not support writing VL datatypes yet")

        /* If MPI based VFD is used, no VL datatype support yet. */
        /* This is because they use the global heap in the file and we don't */
        /* support parallel access of that yet */
        /* We should really use H5T_detect_class() here, but it will be difficult
         * to detect the type of the reference if it is nested... -QAK
         */
        if(H5T_get_class(type_info.mem_type, TRUE) == H5T_REFERENCE &&
                H5T_get_ref_type(type_info.mem_type) == H5R_DATASET_REGION)
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "Parallel IO does not support writing region reference datatypes yet")

        /* Can't write to chunked datasets with filters, in parallel */
        if(dataset->shared->layout.type == H5D_CHUNKED &&
                dataset->shared->dcpl_cache.pline.nused > 0)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "cannot write to chunked storage with filters in parallel")
    } /* end if */
    else {
        /* Collective access is not permissible without a MPI based VFD */
        if(dxpl_cache->xfer_mode == H5FD_MPIO_COLLECTIVE)
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
    H5_ASSIGN_OVERFLOW(nelmts, snelmts, hssize_t, hsize_t);

    /* Make certain that the number of elements in each selection is the same */
    if(nelmts != (hsize_t)H5S_GET_SELECT_NPOINTS(file_space))
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "src and dest data spaces have different sizes")

    /* Make sure that both selections have their extents set */
    if(!(H5S_has_extent(file_space)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file dataspace does not have extent set")
    if(!(H5S_has_extent(mem_space)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "memory dataspace does not have extent set")

    /* Retrieve dataset properties */
    /* <none needed currently> */

    /* Allocate data space and initialize it if it hasn't been. */
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
        if(H5D_alloc_storage(dataset, dxpl_id, H5D_ALLOC_WRITE, full_overwrite, NULL) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize storage")
    } /* end if */

    /* Set up I/O operation */
    io_info.op_type = H5D_IO_OP_WRITE;
    io_info.u.wbuf = buf;
    if(H5D_ioinfo_init(dataset, dxpl_cache, dxpl_id, &type_info, &store, &io_info) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to set up I/O operation")
#ifdef H5_HAVE_PARALLEL
    io_info_init = TRUE;
#endif /*H5_HAVE_PARALLEL*/

    /* Call storage method's I/O initialization routine */
    if(io_info.layout_ops.io_init && (*io_info.layout_ops.io_init)(&io_info, &type_info, nelmts, file_space, mem_space, &fm) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't initialize I/O info")
    io_op_init = TRUE;

#ifdef H5_HAVE_PARALLEL
    /* Adjust I/O info for any parallel I/O */
    if(H5D_ioinfo_adjust(&io_info, dataset, dxpl_id, file_space, mem_space, &type_info, &fm) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to adjust I/O info for parallel I/O")
#endif /*H5_HAVE_PARALLEL*/

    /* Invoke correct "high level" I/O routine */
    if((*io_info.io_ops.multi_write)(&io_info, &type_info, nelmts, file_space, mem_space, &fm) < 0)
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
    if(H5O_touch(&(dataset->oloc), FALSE, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update modification time")
#endif /* OLD_WAY */

done:
    /* Shut down the I/O op information */
    if(io_op_init && io_info.layout_ops.io_term && (*io_info.layout_ops.io_term)(&fm) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTCLOSEOBJ, FAIL, "unable to shut down I/O op info")
#ifdef H5_HAVE_PARALLEL
    /* Shut down io_info struct */
    if(io_info_init && H5D_ioinfo_term(&io_info) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTCLOSEOBJ, FAIL, "can't shut down io_info")
#endif /*H5_HAVE_PARALLEL*/
    /* Shut down datatype info for operation */
    if(type_info_init && H5D_typeinfo_term(&type_info) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTCLOSEOBJ, FAIL, "unable to shut down type info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_write() */


/*-------------------------------------------------------------------------
 * Function:	H5D_ioinfo_init
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
H5D_ioinfo_init(H5D_t *dset, const H5D_dxpl_cache_t *dxpl_cache, hid_t dxpl_id,
    const H5D_type_info_t *type_info, H5D_storage_t *store, H5D_io_info_t *io_info)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_ioinfo_init)

    /* check args */
    HDassert(dset);
    HDassert(dset->oloc.file);
    HDassert(type_info);
    HDassert(type_info->tpath);
    HDassert(io_info);

    /* Set up "normal" I/O fields */
    io_info->dset = dset;
    io_info->dxpl_cache = dxpl_cache;
    io_info->dxpl_id = dxpl_id;
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
        io_info->io_ops.single_read = H5D_select_read;
        io_info->io_ops.single_write = H5D_select_write;
    } /* end if */
    else {
        /*
         * This is the general case (type conversion, usually).
         */
        io_info->io_ops.single_read = H5D_scatgath_read;
        io_info->io_ops.single_write = H5D_scatgath_write;
    } /* end else */

#ifdef H5_HAVE_PARALLEL
    /* Determine if the file was opened with an MPI VFD */
    io_info->using_mpi_vfd = IS_H5FD_MPI(dset->oloc.file);
#endif /* H5_HAVE_PARALLEL */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D_ioinfo_init() */


/*-------------------------------------------------------------------------
 * Function:	H5D_typeinfo_init
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
H5D_typeinfo_init(const H5D_t *dset, const H5D_dxpl_cache_t *dxpl_cache,
    hid_t dxpl_id, hid_t mem_type_id, hbool_t do_write,
    H5D_type_info_t *type_info)
{
    const H5T_t	*src_type;              /* Source datatype */
    const H5T_t	*dst_type;              /* Destination datatype */
    herr_t ret_value = SUCCEED;	        /* Return value	*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_typeinfo_init)

    /* check args */
    HDassert(type_info);
    HDassert(dset);

    /* Initialize type info safely */
    HDmemset(type_info, 0, sizeof(*type_info));

    /* Get the memory & dataset datatypes */
    if(NULL == (type_info->mem_type = (const H5T_t *)H5I_object_verify(mem_type_id, H5I_DATATYPE)))
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
     * Locate the type conversion function and data space conversion
     * functions, and set up the element numbering information. If a data
     * type conversion is necessary then register datatype atoms. Data type
     * conversion is necessary if the user has set the `need_bkg' to a high
     * enough value in xfer_parms since turning off datatype conversion also
     * turns off background preservation.
     */
    if(NULL == (type_info->tpath = H5T_path_find(src_type, dst_type, NULL, NULL, dxpl_id, FALSE)))
	HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "unable to convert between src and dest datatype")

    /* Precompute some useful information */
    type_info->src_type_size = H5T_get_size(src_type);
    type_info->dst_type_size = H5T_get_size(dst_type);
    type_info->max_type_size = MAX(type_info->src_type_size, type_info->dst_type_size);
    type_info->is_conv_noop = H5T_path_noop(type_info->tpath);
    type_info->is_xform_noop = H5Z_xform_noop(dxpl_cache->data_xform_prop);
    if(type_info->is_xform_noop && type_info->is_conv_noop) {
        type_info->cmpd_subset = NULL;
        type_info->need_bkg = H5T_BKG_NO;
    } /* end if */
    else {
        size_t	target_size;		/* Desired buffer size	*/

        /* Check if the datatypes are compound subsets of one another */
        type_info->cmpd_subset = H5T_path_compound_subset(type_info->tpath);

        /* Check if we need a background buffer */
        if(do_write && H5T_detect_class(dset->shared->type, H5T_VLEN, FALSE))
            type_info->need_bkg = H5T_BKG_YES;
        else {
            H5T_bkg_t path_bkg;     /* Type conversion's background info */

            if((path_bkg = H5T_path_bkg(type_info->tpath))) {
                /* Retrieve the bkgr buffer property */
                type_info->need_bkg = dxpl_cache->bkgr_buf_type;
                type_info->need_bkg = MAX(path_bkg, type_info->need_bkg);
            } /* end if */
            else
                type_info->need_bkg = H5T_BKG_NO; /*never needed even if app says yes*/
        } /* end else */


        /* Set up datatype conversion/background buffers */

        /* Get buffer size from DXPL */
        target_size = dxpl_cache->max_temp_buf;

        /* If the buffer is too small to hold even one element, try to make it bigger */
        if(target_size < type_info->max_type_size) {
            hbool_t default_buffer_info;    /* Whether the buffer information are the defaults */

            /* Detect if we have all default settings for buffers */
            default_buffer_info = (hbool_t)((H5D_TEMP_BUF_SIZE == dxpl_cache->max_temp_buf)
                    && (NULL == dxpl_cache->tconv_buf) && (NULL == dxpl_cache->bkgr_buf));

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

        /*
         * Get a temporary buffer for type conversion unless the app has already
         * supplied one through the xfer properties. Instead of allocating a
         * buffer which is the exact size, we allocate the target size.  The
         * malloc() is usually less resource-intensive if we allocate/free the
         * same size over and over.
         */
        if(NULL == (type_info->tconv_buf = (uint8_t *)dxpl_cache->tconv_buf)) {
            /* Allocate temporary buffer */
            if(NULL == (type_info->tconv_buf = H5FL_BLK_MALLOC(type_conv, target_size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for type conversion")
            type_info->tconv_buf_allocated = TRUE;
        } /* end if */
        if(type_info->need_bkg && NULL == (type_info->bkg_buf = (uint8_t *)dxpl_cache->bkgr_buf)) {
            size_t	bkg_size;		/* Desired background buffer size	*/

            /* Compute the background buffer size */
            /* (don't try to use buffers smaller than the default size) */
            bkg_size = type_info->request_nelmts * type_info->dst_type_size;
            if(bkg_size < dxpl_cache->max_temp_buf)
                bkg_size = dxpl_cache->max_temp_buf;

            /* Allocate background buffer */
            /* (Need calloc()-like call since memory needs to be initialized) */
            if(NULL == (type_info->bkg_buf = H5FL_BLK_CALLOC(type_conv, bkg_size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for background conversion")
            type_info->bkg_buf_allocated = TRUE;
        } /* end if */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_typeinfo_init() */

#ifdef H5_HAVE_PARALLEL

/*-------------------------------------------------------------------------
 * Function:	H5D_ioinfo_adjust
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
H5D_ioinfo_adjust(H5D_io_info_t *io_info, const H5D_t *dset, hid_t dxpl_id,
    const H5S_t *file_space, const H5S_t *mem_space,
    const H5D_type_info_t *type_info, const H5D_chunk_map_t *fm)
{
    herr_t	ret_value = SUCCEED;	/* Return value	*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_ioinfo_adjust)

    /* check args */
    HDassert(dset);
    HDassert(dset->oloc.file);
    HDassert(mem_space);
    HDassert(file_space);
    HDassert(type_info);
    HDassert(type_info->tpath);
    HDassert(io_info);

    /* Make any parallel I/O adjustments */
    if(io_info->using_mpi_vfd) {
        htri_t opt;         /* Flag whether a selection is optimizable */

        /* Record the original state of parallel I/O transfer options */
        io_info->orig.xfer_mode = io_info->dxpl_cache->xfer_mode;
        io_info->orig.coll_opt_mode = io_info->dxpl_cache->coll_opt_mode;
        io_info->orig.io_ops.single_read = io_info->io_ops.single_read;
        io_info->orig.io_ops.single_write = io_info->io_ops.single_write;

        /* Get MPI communicator */
        if(MPI_COMM_NULL == (io_info->comm = H5F_mpi_get_comm(dset->oloc.file)))
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't retrieve MPI communicator")

        /* Check if we can set direct MPI-IO read/write functions */
        if((opt = H5D_mpio_opt_possible(io_info, file_space, mem_space, type_info, fm)) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "invalid check for direct IO dataspace ")

        /* Check if we can use the optimized parallel I/O routines */
        if(opt == TRUE) {
            /* Override the I/O op pointers to the MPI-specific routines */
            io_info->io_ops.multi_read = dset->shared->layout.ops->par_read;
            io_info->io_ops.multi_write = dset->shared->layout.ops->par_write;
            io_info->io_ops.single_read = H5D_mpio_select_read;
            io_info->io_ops.single_write = H5D_mpio_select_write;
        } /* end if */
        else {
            /* If we won't be doing collective I/O, but the user asked for
             * collective I/O, change the request to use independent I/O, but
             * mark it so that we remember to revert the change.
             */
            if(io_info->dxpl_cache->xfer_mode == H5FD_MPIO_COLLECTIVE) {
                H5P_genplist_t *dx_plist;           /* Data transer property list */

                /* Get the dataset transfer property list */
                if(NULL == (dx_plist = (H5P_genplist_t *)H5I_object(dxpl_id)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset creation property list")

                /* Change the xfer_mode to independent for handling the I/O */
                io_info->dxpl_cache->xfer_mode = H5FD_MPIO_INDEPENDENT;
                if(H5P_set(dx_plist, H5D_XFER_IO_XFER_MODE_NAME, &io_info->dxpl_cache->xfer_mode) < 0)
                    HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set transfer mode")
            } /* end if */
        } /* end else */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_ioinfo_adjust() */


/*-------------------------------------------------------------------------
 * Function:	H5D_ioinfo_term
 *
 * Purpose:	Common logic for terminating an I/O info object
 *              (Only used for restoring MPI transfer mode currently)
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Friday, February  6, 2004
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_ioinfo_term(H5D_io_info_t *io_info)
{
    herr_t	ret_value = SUCCEED;	/*return value		*/

    FUNC_ENTER_NOAPI_NOINIT(H5D_ioinfo_term)

    /* Check if we used the MPI VFD for the I/O */
    if(io_info->using_mpi_vfd) {
        /* Check if we need to revert the change to the xfer mode */
        if(io_info->orig.xfer_mode != io_info->dxpl_cache->xfer_mode) {
            H5P_genplist_t *dx_plist;           /* Data transer property list */

            /* Get the dataset transfer property list */
            if(NULL == (dx_plist = (H5P_genplist_t *)H5I_object(io_info->dxpl_id)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset transfer property list")

            /* Restore the original parallel I/O mode */
            if(H5P_set(dx_plist, H5D_XFER_IO_XFER_MODE_NAME, &io_info->orig.xfer_mode) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set transfer mode")
        } /* end if */

        /* Check if we need to revert the change to the collective opt mode */
        if(io_info->orig.coll_opt_mode != io_info->dxpl_cache->coll_opt_mode) {
            H5P_genplist_t *dx_plist;           /* Data transer property list */

            /* Get the dataset transfer property list */
            if(NULL == (dx_plist = (H5P_genplist_t *)H5I_object(io_info->dxpl_id)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset transfer property list")

            /* Restore the original parallel I/O mode */
            if(H5P_set(dx_plist, H5D_XFER_MPIO_COLLECTIVE_OPT_NAME, &io_info->orig.coll_opt_mode) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set collective option mode")
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_ioinfo_term() */

#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 * Function:	H5D_typeinfo_term
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
H5D_typeinfo_term(const H5D_type_info_t *type_info)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_typeinfo_term)

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
} /* end H5D_typeinfo_term() */

