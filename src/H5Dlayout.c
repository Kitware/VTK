/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/****************/
/* Module Setup */
/****************/

#include "H5Dmodule.h" /* This source code file is part of the H5D module */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions                        */
#include "H5Dpkg.h"      /* Datasets                                 */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5HLprivate.h" /* Local heaps                              */

/****************/
/* Local Macros */
/****************/

/******************/
/* Local Typedefs */
/******************/

/********************/
/* Local Prototypes */
/********************/

/*********************/
/* Package Variables */
/*********************/

/* Format version bounds for layout */
const unsigned H5O_layout_ver_bounds[] = {
    H5O_LAYOUT_VERSION_1, /* H5F_LIBVER_EARLIEST */
    H5O_LAYOUT_VERSION_3,
    /* H5F_LIBVER_V18 */      /* H5O_LAYOUT_VERSION_DEFAULT */
    H5O_LAYOUT_VERSION_4,     /* H5F_LIBVER_V110 */
    H5O_LAYOUT_VERSION_4,     /* H5F_LIBVER_V112 */
    H5O_LAYOUT_VERSION_4,     /* H5F_LIBVER_V114 */
    H5O_LAYOUT_VERSION_5,     /* H5F_LIBVER_V200 */
    H5O_LAYOUT_VERSION_LATEST /* H5F_LIBVER_LATEST */
};

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/*-------------------------------------------------------------------------
 * Function:    H5D__layout_set_io_ops
 *
 * Purpose:     Set the I/O operation function pointers for a dataset,
 *              according to the dataset's layout
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__layout_set_io_ops(const H5D_t *dataset)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* check args */
    assert(dataset);

    /* Set the I/O functions for each layout type */
    switch (dataset->shared->layout.type) {
        case H5D_CONTIGUOUS:
            if (dataset->shared->dcpl_cache.efl.nused > 0)
                dataset->shared->layout.ops = H5D_LOPS_EFL;
            else
                dataset->shared->layout.ops = H5D_LOPS_CONTIG;
            break;

        case H5D_CHUNKED:
            dataset->shared->layout.ops = H5D_LOPS_CHUNK;

            /* Set the chunk operations */
            switch (dataset->shared->layout.u.chunk.idx_type) {
                case H5D_CHUNK_IDX_BTREE:
                    dataset->shared->layout.storage.u.chunk.ops = H5D_COPS_BTREE;
                    break;

                case H5D_CHUNK_IDX_NONE:
                    dataset->shared->layout.storage.u.chunk.ops = H5D_COPS_NONE;
                    break;

                case H5D_CHUNK_IDX_SINGLE:
                    dataset->shared->layout.storage.u.chunk.ops = H5D_COPS_SINGLE;
                    break;

                case H5D_CHUNK_IDX_FARRAY:
                    dataset->shared->layout.storage.u.chunk.ops = H5D_COPS_FARRAY;
                    break;

                case H5D_CHUNK_IDX_EARRAY:
                    dataset->shared->layout.storage.u.chunk.ops = H5D_COPS_EARRAY;
                    break;

                case H5D_CHUNK_IDX_BT2:
                    dataset->shared->layout.storage.u.chunk.ops = H5D_COPS_BT2;
                    break;

                case H5D_CHUNK_IDX_NTYPES:
                default:
                    assert(0 && "Unknown chunk index method!");
                    HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "unknown chunk index method");
            } /* end switch */
            break;

        case H5D_COMPACT:
            dataset->shared->layout.ops = H5D_LOPS_COMPACT;
            break;

        case H5D_VIRTUAL:
            dataset->shared->layout.ops = H5D_LOPS_VIRTUAL;
            break;

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        default:
            HGOTO_ERROR(H5E_DATASET, H5E_UNSUPPORTED, FAIL, "unknown storage method");
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__layout_set_io_ops() */

/*-------------------------------------------------------------------------
 * Function:    H5D__layout_meta_size
 *
 * Purpose:     Returns the size of the raw message in bytes except raw data
 *              part for compact dataset.  This function doesn't take into
 *              account message alignment.
 *
 * Return:      Success:        Message data size in bytes
 *              Failure:        0
 *
 *-------------------------------------------------------------------------
 */
size_t
H5D__layout_meta_size(const H5F_t *f, const H5O_layout_t *layout, bool include_compact_data)
{
    size_t ret_value = 0; /* Return value */

    FUNC_ENTER_PACKAGE

    /* check args */
    assert(f);
    assert(layout);

    ret_value = 1 + /* Version number                       */
                1;  /* layout class type                    */

    switch (layout->type) {
        case H5D_COMPACT:
            /* This information only present in older versions of message */
            /* Size of raw data */
            ret_value += 2;
            if (include_compact_data)
                ret_value += layout->storage.u.compact.size; /* data for compact dataset             */
            break;

        case H5D_CONTIGUOUS:
            /* This information only present in older versions of message */
            ret_value += H5F_SIZEOF_ADDR(f); /* Address of data */
            ret_value += H5F_SIZEOF_SIZE(f); /* Length of data */
            break;

        case H5D_CHUNKED:
            if (layout->version < H5O_LAYOUT_VERSION_4) {
                /* Number of dimensions (1 byte) */
                assert(layout->u.chunk.ndims > 0 && layout->u.chunk.ndims <= H5O_LAYOUT_NDIMS);
                ret_value++;

                /* B-tree address */
                ret_value += H5F_SIZEOF_ADDR(f); /* Address of data */

                /* Dimension sizes */
                ret_value += layout->u.chunk.ndims * 4;
            } /* end if */
            else {
                /* Chunked layout feature flags */
                ret_value++;

                /* Number of dimensions (1 byte) */
                assert(layout->u.chunk.ndims > 0 && layout->u.chunk.ndims <= H5O_LAYOUT_NDIMS);
                ret_value++;

                /* Encoded # of bytes for each chunk dimension */
                assert(layout->u.chunk.enc_bytes_per_dim > 0 && layout->u.chunk.enc_bytes_per_dim <= 8);
                ret_value++;

                /* Dimension sizes */
                ret_value += layout->u.chunk.ndims * (size_t)layout->u.chunk.enc_bytes_per_dim;

                /* Type of chunk index */
                ret_value++;

                switch (layout->u.chunk.idx_type) {
                    case H5D_CHUNK_IDX_BTREE:
                        HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, 0,
                                    "v1 B-tree index type found for layout message >v3");

                    case H5D_CHUNK_IDX_NONE:
                        /* nothing */
                        break;

                    case H5D_CHUNK_IDX_SINGLE:
                        /* Possible filter information */
                        if (layout->u.chunk.flags & H5O_LAYOUT_CHUNK_SINGLE_INDEX_WITH_FILTER) {
                            ret_value += H5F_SIZEOF_SIZE(f); /* Size of chunk (in file) */
                            ret_value += 4;                  /* Filter mask for chunk */
                        }                                    /* end if */
                        break;

                    case H5D_CHUNK_IDX_FARRAY:
                        /* Fixed array creation parameters */
                        ret_value += H5D_FARRAY_CREATE_PARAM_SIZE;
                        break;

                    case H5D_CHUNK_IDX_EARRAY:
                        /* Extensible array creation parameters */
                        ret_value += H5D_EARRAY_CREATE_PARAM_SIZE;
                        break;

                    case H5D_CHUNK_IDX_BT2:
                        /* v2 B-tree creation parameters */
                        ret_value += H5D_BT2_CREATE_PARAM_SIZE;
                        break;

                    case H5D_CHUNK_IDX_NTYPES:
                    default:
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTENCODE, 0, "Invalid chunk index type");
                } /* end switch */

                /* Chunk index address */
                ret_value += H5F_SIZEOF_ADDR(f);
            } /* end else */
            break;

        case H5D_VIRTUAL:
            ret_value += H5F_SIZEOF_ADDR(f); /* Address of global heap */
            ret_value += 4;                  /* Global heap index */
            break;

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        default:
            HGOTO_ERROR(H5E_OHDR, H5E_CANTENCODE, 0, "Invalid layout class");
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__layout_meta_size() */

/*-------------------------------------------------------------------------
 * Function:    H5D__layout_oh_create
 *
 * Purpose:     Create layout/pline/efl information for dataset
 *
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__layout_oh_create(H5F_t *file, H5O_t *oh, H5D_t *dset, hid_t dapl_id)
{
    H5O_layout_t     *layout;                /* Dataset's layout information */
    const H5O_fill_t *fill_prop;             /* Pointer to dataset's fill value information */
    unsigned          layout_mesg_flags;     /* Flags for inserting layout message */
    bool              layout_init = false;   /* Flag to indicate that chunk information was initialized */
    herr_t            ret_value   = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE_TAG(dset->oloc.addr)

    /* Sanity checking */
    assert(file);
    assert(oh);
    assert(dset);

    /* Set some local variables, for convenience */
    layout    = &dset->shared->layout;
    fill_prop = &dset->shared->dcpl_cache.fill;

    /* Update the filters message, if this is a chunked dataset */
    if (layout->type == H5D_CHUNKED) {
        H5O_pline_t *pline; /* Dataset's I/O pipeline information */

        pline = &dset->shared->dcpl_cache.pline;
        if (pline->nused > 0 &&
            H5O_msg_append_oh(file, oh, H5O_PLINE_ID, H5O_MSG_FLAG_CONSTANT, 0, pline) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update filter header message");
    } /* end if */

    /* Initialize the layout information for the new dataset */
    if (dset->shared->layout.ops->init && (dset->shared->layout.ops->init)(file, dset, dapl_id, false) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize layout information");

    /* Indicate that the layout information was initialized */
    layout_init = true;

    /*
     * Allocate storage if space allocate time is early; otherwise delay
     * allocation until later.
     */
    if (fill_prop->alloc_time == H5D_ALLOC_TIME_EARLY)
        if (H5D__alloc_storage(dset, H5D_ALLOC_CREATE, false, NULL) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize storage");

    /* Update external storage message, if it's used */
    if (dset->shared->dcpl_cache.efl.nused > 0) {
        H5O_efl_t *efl = &dset->shared->dcpl_cache.efl; /* Dataset's external file list */
        H5HL_t    *heap;                                /* Pointer to local heap for EFL file names */
        size_t     heap_size = H5HL_ALIGN(1);
        size_t     u;
        size_t     name_offset;

        /* Determine size of heap needed to stored the file names */
        for (u = 0; u < efl->nused; ++u)
            heap_size += H5HL_ALIGN(strlen(efl->slot[u].name) + 1);

        /* Create the heap for the EFL file names */
        if (H5HL_create(file, heap_size, &efl->heap_addr /*out*/) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create EFL file name heap");

        /* Pin the heap down in memory */
        if (NULL == (heap = H5HL_protect(file, efl->heap_addr, H5AC__NO_FLAGS_SET)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTPROTECT, FAIL, "unable to protect EFL file name heap");

        /* Insert "empty" name first */
        if (H5HL_insert(file, heap, (size_t)1, "", &name_offset) < 0) {
            H5HL_unprotect(heap);
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, FAIL, "unable to insert file name into heap");
        }

        for (u = 0; u < efl->nused; ++u) {
            /* Insert file name into heap */
            if (H5HL_insert(file, heap, strlen(efl->slot[u].name) + 1, efl->slot[u].name, &name_offset) < 0) {
                H5HL_unprotect(heap);
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, FAIL, "unable to insert file name into heap");
            }

            /* Store EFL file name offset */
            efl->slot[u].name_offset = name_offset;
        }

        /* Release the heap */
        if (H5HL_unprotect(heap) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTUNPROTECT, FAIL, "unable to unprotect EFL file name heap");
        heap = NULL;

        /* Insert EFL message into dataset object header */
        if (H5O_msg_append_oh(file, oh, H5O_EFL_ID, H5O_MSG_FLAG_CONSTANT, 0, efl) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update external file list message");
    } /* end if */

    /* Create layout message */
    /* (Don't make layout message constant unless allocation time is early and
     *  non-filtered and has >0 elements, since space may not be allocated -QAK) */
    /* (Note: this is relying on H5D__alloc_storage not calling H5O_msg_write during dataset creation) */
    if (fill_prop->alloc_time == H5D_ALLOC_TIME_EARLY && H5D_COMPACT != layout->type &&
        !dset->shared->dcpl_cache.pline.nused && (0 != H5S_GET_EXTENT_NPOINTS(dset->shared->space)))
        layout_mesg_flags = H5O_MSG_FLAG_CONSTANT;
    else
        layout_mesg_flags = 0;

    /* Store VDS info in global heap */
    if (H5D_VIRTUAL == layout->type)
        if (H5D__virtual_store_layout(file, layout) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to store VDS info");

    /* Create layout message */
    if (H5O_msg_append_oh(file, oh, H5O_LAYOUT_ID, layout_mesg_flags, 0, layout) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to update layout");

done:
    /* Error cleanup */
    if (ret_value < 0)
        if (layout_init)
            /* Destroy any cached layout information for the dataset */
            if (dset->shared->layout.ops->dest && (dset->shared->layout.ops->dest)(dset) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "unable to destroy layout info");

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5D__layout_oh_create() */

/*-------------------------------------------------------------------------
 * Function:    H5D__layout_oh_read
 *
 * Purpose:     Read layout/pline/efl information for dataset
 *
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__layout_oh_read(H5D_t *dataset, hid_t dapl_id, H5P_genplist_t *plist)
{
    htri_t msg_exists;                      /* Whether a particular type of message exists */
    bool   pline_copied          = false;   /* Flag to indicate that dcpl_cache.pline's message was copied */
    bool   layout_copied_to_dset = false;   /* Flag to indicate that layout message was copied */
    bool   efl_copied            = false;   /* Flag to indicate that the EFL message was copied */
    herr_t ret_value             = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity checking */
    assert(dataset);
    assert(plist);

    /* Get the optional filters message */
    if ((msg_exists = H5O_msg_exists(&(dataset->oloc), H5O_PLINE_ID)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't check if message exists");
    if (msg_exists) {
        /* Retrieve the I/O pipeline message */
        if (NULL == H5O_msg_read(&(dataset->oloc), H5O_PLINE_ID, &dataset->shared->dcpl_cache.pline))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't retrieve message");
        pline_copied = true;
        /* Set the I/O pipeline info in the property list */
        if (H5P_set(plist, H5O_CRT_PIPELINE_NAME, &dataset->shared->dcpl_cache.pline) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set pipeline");
    } /* end if */

    /*
     * Get the raw data layout info.  It's actually stored in two locations:
     * the storage message of the dataset (dataset->storage) and certain
     * values are copied to the dataset create plist so the user can query
     * them.
     */
    if (NULL == H5O_msg_read(&(dataset->oloc), H5O_LAYOUT_ID, &(dataset->shared->layout)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to read data layout message");
    layout_copied_to_dset = true;

    /* Check for external file list message (which might not exist) */
    if ((msg_exists = H5O_msg_exists(&(dataset->oloc), H5O_EFL_ID)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't check if message exists");
    if (msg_exists) {
        /* Retrieve the EFL  message */
        if (NULL == H5O_msg_read(&(dataset->oloc), H5O_EFL_ID, &dataset->shared->dcpl_cache.efl))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't retrieve message");
        efl_copied = true;

        /* Set the EFL info in the property list */
        if (H5P_set(plist, H5D_CRT_EXT_FILE_LIST_NAME, &dataset->shared->dcpl_cache.efl) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set external file list");

        /* Set the dataset's I/O operations */
        dataset->shared->layout.ops = H5D_LOPS_EFL;
    } /* end if */

    /* Sanity check that the layout operations are set up */
    assert(dataset->shared->layout.ops);

    /* Initialize the layout information for the dataset */
    if (dataset->shared->layout.ops->init &&
        (dataset->shared->layout.ops->init)(dataset->oloc.file, dataset, dapl_id, true) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize layout information");

#ifndef NDEBUG
    /* Set invalid layout to detect erroneous usage */
    H5O_layout_t error_layout;
    error_layout.type         = H5D_LAYOUT_ERROR;
    error_layout.version      = 0;
    error_layout.ops          = NULL;
    error_layout.storage.type = H5D_LAYOUT_ERROR;

    if (H5P_poke(plist, H5D_CRT_LAYOUT_NAME, &error_layout) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to setup placeholder layout");
#endif

done:
    if (ret_value < 0) {
        if (pline_copied)
            if (H5O_msg_reset(H5O_PLINE_ID, &dataset->shared->dcpl_cache.pline) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CANTRESET, FAIL, "unable to reset pipeline info");
        if (layout_copied_to_dset)
            if (H5O_msg_reset(H5O_LAYOUT_ID, &dataset->shared->layout) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CANTRESET, FAIL, "unable to reset layout info");
        if (efl_copied)
            if (H5O_msg_reset(H5O_EFL_ID, &dataset->shared->dcpl_cache.efl) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CANTRESET, FAIL, "unable to reset efl message");
    }
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__layout_oh_read() */

/*-------------------------------------------------------------------------
 * Function:    H5D__layout_oh_write
 *
 * Purpose:     Write layout information for dataset
 *
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__layout_oh_write(const H5D_t *dataset, H5O_t *oh, unsigned update_flags)
{
    htri_t msg_exists;          /* Whether the layout message exists */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity checking */
    assert(dataset);
    assert(oh);

    /* Check if the layout message has been added to the dataset's header */
    if ((msg_exists = H5O_msg_exists_oh(oh, H5O_LAYOUT_ID)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to check if layout message exists");
    if (msg_exists) {
        /* Write the layout message to the dataset's header */
        if (H5O_msg_write_oh(dataset->oloc.file, oh, H5O_LAYOUT_ID, 0, update_flags,
                             &dataset->shared->layout) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to update layout message");
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__layout_oh_write() */
