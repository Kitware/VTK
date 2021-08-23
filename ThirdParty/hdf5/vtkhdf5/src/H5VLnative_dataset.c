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
 * Purpose:     Dataset callbacks for the native VOL connector
 *
 */

#define H5D_FRIEND /* Suppress error about including H5Dpkg    */

#include "H5private.h"   /* Generic Functions                        */
#include "H5CXprivate.h" /* API Contexts                             */
#include "H5Dpkg.h"      /* Datasets                                 */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5Fprivate.h"  /* Files                                    */
#include "H5Gprivate.h"  /* Groups                                   */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5Pprivate.h"  /* Property lists                           */
#include "H5Sprivate.h"  /* Dataspaces                               */
#include "H5VLprivate.h" /* Virtual Object Layer                     */

#include "H5VLnative_private.h" /* Native VOL connector                     */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_dataset_create
 *
 * Purpose:     Handles the dataset create callback
 *
 * Return:      Success:    dataset pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL__native_dataset_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t lcpl_id,
                            hid_t type_id, hid_t space_id, hid_t dcpl_id, hid_t dapl_id,
                            hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    H5G_loc_t    loc;         /* Object location to insert dataset into */
    H5D_t *      dset = NULL; /* New dataset's info */
    const H5S_t *space;       /* Dataspace for dataset */
    void *       ret_value;

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file or file object")
    if (H5I_DATATYPE != H5I_get_type(type_id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a datatype ID")
    if (NULL == (space = (const H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a dataspace ID")

    /* H5Dcreate_anon */
    if (NULL == name) {
        /* build and open the new dataset */
        if (NULL == (dset = H5D__create(loc.oloc->file, type_id, space, dcpl_id, dapl_id)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to create dataset")
    } /* end if */
    /* H5Dcreate2 */
    else {
        /* Create the new dataset & get its ID */
        if (NULL == (dset = H5D__create_named(&loc, name, type_id, space, lcpl_id, dcpl_id, dapl_id)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, NULL, "unable to create dataset")
    } /* end else */

    ret_value = (void *)dset;

done:
    if (NULL == name) {
        /* Release the dataset's object header, if it was created */
        if (dset) {
            H5O_loc_t *oloc; /* Object location for dataset */

            /* Get the new dataset's object location */
            if (NULL == (oloc = H5D_oloc(dset)))
                HDONE_ERROR(H5E_DATASET, H5E_CANTGET, NULL, "unable to get object location of dataset")

            /* Decrement refcount on dataset's object header in memory */
            if (H5O_dec_rc_by_loc(oloc) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CANTDEC, NULL,
                            "unable to decrement refcount on newly created object")
        } /* end if */
    }     /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_dataset_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_dataset_open
 *
 * Purpose:     Handles the dataset open callback
 *
 * Return:      Success:    dataset pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL__native_dataset_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t dapl_id,
                          hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    H5D_t *   dset = NULL;
    H5G_loc_t loc; /* Object location of group */
    void *    ret_value = NULL;

    FUNC_ENTER_PACKAGE

    if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file or file object")

    /* Open the dataset */
    if (NULL == (dset = H5D__open_name(&loc, name, dapl_id)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTOPENOBJ, NULL, "unable to open dataset")

    ret_value = (void *)dset;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_dataset_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_dataset_read
 *
 * Purpose:     Handles the dataset read callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_dataset_read(void *obj, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id,
                          hid_t dxpl_id, void *buf, void H5_ATTR_UNUSED **req)
{
    H5D_t *      dset       = (H5D_t *)obj;
    const H5S_t *mem_space  = NULL;
    const H5S_t *file_space = NULL;
    herr_t       ret_value  = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    if (NULL == dset->oloc.file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dataset is not associated with a file")

    /* Get validated dataspace pointers */
    if (H5S_get_validated_dataspace(mem_space_id, &mem_space) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "could not get a validated dataspace from mem_space_id")
    if (H5S_get_validated_dataspace(file_space_id, &file_space) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "could not get a validated dataspace from file_space_id")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Read raw data */
    if (H5D__read(dset, mem_type_id, mem_space, file_space, buf /*out*/) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "can't read data")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_dataset_read() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_dataset_write
 *
 * Purpose:     Handles the dataset write callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_dataset_write(void *obj, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id,
                           hid_t dxpl_id, const void *buf, void H5_ATTR_UNUSED **req)
{
    H5D_t *      dset       = (H5D_t *)obj;
    const H5S_t *mem_space  = NULL;
    const H5S_t *file_space = NULL;
    herr_t       ret_value  = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* check arguments */
    if (NULL == dset->oloc.file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dataset is not associated with a file")

    /* Get validated dataspace pointers */
    if (H5S_get_validated_dataspace(mem_space_id, &mem_space) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "could not get a validated dataspace from mem_space_id")
    if (H5S_get_validated_dataspace(file_space_id, &file_space) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "could not get a validated dataspace from file_space_id")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Write the data */
    if (H5D__write(dset, mem_type_id, mem_space, file_space, buf) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "can't write data")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_dataset_write() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_dataset_get
 *
 * Purpose:     Handles the dataset get callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_dataset_get(void *obj, H5VL_dataset_get_t get_type, hid_t H5_ATTR_UNUSED dxpl_id,
                         void H5_ATTR_UNUSED **req, va_list arguments)
{
    H5D_t *dset      = (H5D_t *)obj;
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (get_type) {
        /* H5Dget_space */
        case H5VL_DATASET_GET_SPACE: {
            hid_t *ret_id = HDva_arg(arguments, hid_t *);

            if ((*ret_id = H5D__get_space(dset)) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_CANTGET, FAIL, "can't get space ID of dataset")

            break;
        }

        /* H5Dget_space_status */
        case H5VL_DATASET_GET_SPACE_STATUS: {
            H5D_space_status_t *allocation = HDva_arg(arguments, H5D_space_status_t *);

            /* Read data space address and return */
            if (H5D__get_space_status(dset, allocation) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to get space status")

            break;
        }

        /* H5Dget_type */
        case H5VL_DATASET_GET_TYPE: {
            hid_t *ret_id = HDva_arg(arguments, hid_t *);

            if ((*ret_id = H5D__get_type(dset)) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_CANTGET, FAIL, "can't get datatype ID of dataset")

            break;
        }

        /* H5Dget_create_plist */
        case H5VL_DATASET_GET_DCPL: {
            hid_t *ret_id = HDva_arg(arguments, hid_t *);

            if ((*ret_id = H5D_get_create_plist(dset)) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_CANTGET, FAIL, "can't get creation property list for dataset")

            break;
        }

        /* H5Dget_access_plist */
        case H5VL_DATASET_GET_DAPL: {
            hid_t *ret_id = HDva_arg(arguments, hid_t *);

            if ((*ret_id = H5D_get_access_plist(dset)) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_CANTGET, FAIL, "can't get access property list for dataset")

            break;
        }

        /* H5Dget_storage_size */
        case H5VL_DATASET_GET_STORAGE_SIZE: {
            hsize_t *ret = HDva_arg(arguments, hsize_t *);

            /* Set return value */
            if (H5D__get_storage_size(dset, ret) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get size of dataset's storage")
            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get this type of information from dataset")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_dataset_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_dataset_specific
 *
 * Purpose:     Handles the dataset specific callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_dataset_specific(void *obj, H5VL_dataset_specific_t specific_type, hid_t H5_ATTR_UNUSED dxpl_id,
                              void H5_ATTR_UNUSED **req, va_list arguments)
{
    H5D_t *dset      = (H5D_t *)obj;
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (specific_type) {
        /* H5Dspecific_space */
        case H5VL_DATASET_SET_EXTENT: { /* H5Dset_extent (H5Dextend - deprecated) */
            const hsize_t *size = HDva_arg(arguments, const hsize_t *);

            if (H5D__set_extent(dset, size) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to set extent of dataset")
            break;
        }

        case H5VL_DATASET_FLUSH: { /* H5Dflush */
            hid_t dset_id = HDva_arg(arguments, hid_t);

            /* Flush the dataset */
            if (H5D__flush(dset, dset_id) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTFLUSH, FAIL, "unable to flush dataset")

            break;
        }

        case H5VL_DATASET_REFRESH: { /* H5Drefresh */
            hid_t dset_id = HDva_arg(arguments, hid_t);

            /* Refresh the dataset */
            if ((H5D__refresh(dset_id, dset)) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTLOAD, FAIL, "unable to refresh dataset")

            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "invalid specific operation")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_dataset_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_dataset_optional
 *
 * Purpose:     Handles the dataset optional callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_dataset_optional(void *obj, H5VL_dataset_optional_t optional_type, hid_t dxpl_id,
                              void H5_ATTR_UNUSED **req, va_list arguments)
{
    H5D_t *dset      = (H5D_t *)obj; /* Dataset */
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity checks */
    HDassert(dset);

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    switch (optional_type) {
        case H5VL_NATIVE_DATASET_FORMAT_CONVERT: { /* H5Dformat_convert */
            switch (dset->shared->layout.type) {
                case H5D_CHUNKED:
                    /* Convert the chunk indexing type to version 1 B-tree if not */
                    if (dset->shared->layout.u.chunk.idx_type != H5D_CHUNK_IDX_BTREE)
                        if ((H5D__format_convert(dset)) < 0)
                            HGOTO_ERROR(H5E_DATASET, H5E_CANTLOAD, FAIL,
                                        "unable to downgrade chunk indexing type for dataset")
                    break;

                case H5D_CONTIGUOUS:
                case H5D_COMPACT:
                    /* Downgrade the layout version to 3 if greater than 3 */
                    if (dset->shared->layout.version > H5O_LAYOUT_VERSION_DEFAULT)
                        if ((H5D__format_convert(dset)) < 0)
                            HGOTO_ERROR(H5E_DATASET, H5E_CANTLOAD, FAIL,
                                        "unable to downgrade layout version for dataset")
                    break;

                case H5D_VIRTUAL:
                    /* Nothing to do even though layout is version 4 */
                    break;

                case H5D_LAYOUT_ERROR:
                case H5D_NLAYOUTS:
                    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataset layout type")

                default:
                    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "unknown dataset layout type")
            } /* end switch */

            break;
        }

        case H5VL_NATIVE_DATASET_GET_CHUNK_INDEX_TYPE: { /* H5Dget_chunk_index_type */
            H5D_chunk_index_t *idx_type = HDva_arg(arguments, H5D_chunk_index_t *);

            /* Make sure the dataset is chunked */
            if (H5D_CHUNKED != dset->shared->layout.type)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a chunked dataset")

            /* Get the chunk indexing type */
            *idx_type = dset->shared->layout.u.chunk.idx_type;

            break;
        }

        case H5VL_NATIVE_DATASET_GET_CHUNK_STORAGE_SIZE: { /* H5Dget_chunk_storage_size */
            hsize_t *offset       = HDva_arg(arguments, hsize_t *);
            hsize_t *chunk_nbytes = HDva_arg(arguments, hsize_t *);

            /* Make sure the dataset is chunked */
            if (H5D_CHUNKED != dset->shared->layout.type)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a chunked dataset")

            /* Call private function */
            if (H5D__get_chunk_storage_size(dset, offset, chunk_nbytes) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get storage size of chunk")

            break;
        }

        case H5VL_NATIVE_DATASET_GET_NUM_CHUNKS: { /* H5Dget_num_chunks */
            const H5S_t *space    = NULL;
            hid_t        space_id = HDva_arg(arguments, hid_t);
            hsize_t *    nchunks  = HDva_arg(arguments, hsize_t *);

            HDassert(dset->shared);
            HDassert(dset->shared->space);

            /* When default dataspace is given, use the dataset's dataspace */
            if (space_id == H5S_ALL)
                space = dset->shared->space;
            else /*  otherwise, use the given space ID */
                if (NULL == (space = (const H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a valid dataspace ID")

            /* Make sure the dataset is chunked */
            if (H5D_CHUNKED != dset->shared->layout.type)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a chunked dataset")

            /* Call private function */
            if (H5D__get_num_chunks(dset, space, nchunks) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get number of chunks")

            break;
        }

        case H5VL_NATIVE_DATASET_GET_CHUNK_INFO_BY_IDX: { /* H5Dget_chunk_info */
            const H5S_t *space       = NULL;
            hid_t        space_id    = HDva_arg(arguments, hid_t);
            hsize_t      chk_index   = HDva_arg(arguments, hsize_t);
            hsize_t *    offset      = HDva_arg(arguments, hsize_t *);
            unsigned *   filter_mask = HDva_arg(arguments, unsigned *);
            haddr_t *    addr        = HDva_arg(arguments, haddr_t *);
            hsize_t *    size        = HDva_arg(arguments, hsize_t *);

            HDassert(dset->shared);
            HDassert(dset->shared->space);

            /* When default dataspace is given, use the dataset's dataspace */
            if (space_id == H5S_ALL)
                space = dset->shared->space;
            else /*  otherwise, use the given space ID */
                if (NULL == (space = (const H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a valid dataspace ID")

            /* Make sure the dataset is chunked */
            if (H5D_CHUNKED != dset->shared->layout.type)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a chunked dataset")

            /* Call private function */
            if (H5D__get_chunk_info(dset, space, chk_index, offset, filter_mask, addr, size) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get chunk info by index")
            break;
        }

        case H5VL_NATIVE_DATASET_GET_CHUNK_INFO_BY_COORD: { /* H5Dget_chunk_info_by_coord */
            hsize_t * offset      = HDva_arg(arguments, hsize_t *);
            unsigned *filter_mask = HDva_arg(arguments, unsigned *);
            haddr_t * addr        = HDva_arg(arguments, haddr_t *);
            hsize_t * size        = HDva_arg(arguments, hsize_t *);

            HDassert(dset->shared);

            /* Make sure the dataset is chunked */
            if (H5D_CHUNKED != dset->shared->layout.type)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a chunked dataset")

            /* Call private function */
            if (H5D__get_chunk_info_by_coord(dset, offset, filter_mask, addr, size) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get chunk info by its logical coordinates")

            break;
        }

        case H5VL_NATIVE_DATASET_CHUNK_READ: { /* H5Dread_chunk */
            const hsize_t *offset  = HDva_arg(arguments, hsize_t *);
            uint32_t *     filters = HDva_arg(arguments, uint32_t *);
            void *         buf     = HDva_arg(arguments, void *);
            hsize_t        offset_copy[H5O_LAYOUT_NDIMS]; /* Internal copy of chunk offset */

            /* Check arguments */
            if (NULL == dset->oloc.file)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dataset is not associated with a file")
            if (H5D_CHUNKED != dset->shared->layout.type)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a chunked dataset")

            /* Copy the user's offset array so we can be sure it's terminated properly.
             * (we don't want to mess with the user's buffer).
             */
            if (H5D__get_offset_copy(dset, offset, offset_copy) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "failure to copy offset array")

            /* Read the raw chunk */
            if (H5D__chunk_direct_read(dset, offset_copy, filters, buf) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "can't read unprocessed chunk data")

            break;
        }

        case H5VL_NATIVE_DATASET_CHUNK_WRITE: { /* H5Dwrite_chunk */
            uint32_t       filters      = HDva_arg(arguments, uint32_t);
            const hsize_t *offset       = HDva_arg(arguments, const hsize_t *);
            uint32_t       data_size_32 = HDva_arg(arguments, uint32_t);
            const void *   buf          = HDva_arg(arguments, const void *);
            hsize_t        offset_copy[H5O_LAYOUT_NDIMS]; /* Internal copy of chunk offset */

            /* Check arguments */
            if (NULL == dset->oloc.file)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dataset is not associated with a file")
            if (H5D_CHUNKED != dset->shared->layout.type)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a chunked dataset")

            /* Copy the user's offset array so we can be sure it's terminated properly.
             * (we don't want to mess with the user's buffer).
             */
            if (H5D__get_offset_copy(dset, offset, offset_copy) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "failure to copy offset array")

            /* Write chunk */
            if (H5D__chunk_direct_write(dset, filters, offset_copy, data_size_32, buf) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "can't write unprocessed chunk data")

            break;
        }

        case H5VL_NATIVE_DATASET_GET_VLEN_BUF_SIZE: { /* H5Dvlen_get_buf_size */
            hid_t    type_id  = HDva_arg(arguments, hid_t);
            hid_t    space_id = HDva_arg(arguments, hid_t);
            hsize_t *size     = HDva_arg(arguments, hsize_t *);

            if (H5D__vlen_get_buf_size(dset, type_id, space_id, size) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get size of vlen buf needed")
            break;
        }

        /* H5Dget_offset */
        case H5VL_NATIVE_DATASET_GET_OFFSET: {
            haddr_t *ret = HDva_arg(arguments, haddr_t *);

            /* Set return value */
            *ret = H5D__get_offset(dset);
            if (!H5F_addr_defined(*ret))
                *ret = HADDR_UNDEF;
            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "invalid optional operation")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_dataset_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_dataset_close
 *
 * Purpose:     Handles the dataset close callback
 *
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL (dataset will not be closed)
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_dataset_close(void *dset, hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    if (H5D_close((H5D_t *)dset) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTDEC, FAIL, "can't close dataset")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_dataset_close() */
