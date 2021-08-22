/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
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
 * Programmer:  rky 980813
 * KY 2005 revised the code and made the change to support and optimize
 * collective IO support.
 * Purpose:    Functions to read/write directly between app buffer and file.
 *
 *         Beware of the ifdef'ed print statements.
 *         I didn't make them portable.
 */

/****************/
/* Module Setup */
/****************/

#include "H5Dmodule.h" /* This source code file is part of the H5D module */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions */
#include "H5CXprivate.h" /* API Contexts      */
#include "H5Dpkg.h"      /* Datasets          */
#include "H5Eprivate.h"  /* Error handling    */
#include "H5Fprivate.h"  /* File access       */
#include "H5FDprivate.h" /* File drivers      */
#include "H5Iprivate.h"  /* IDs               */
#include "H5MMprivate.h" /* Memory management */
#include "H5Oprivate.h"  /* Object headers    */
#include "H5Pprivate.h"  /* Property lists    */
#include "H5Sprivate.h"  /* Dataspaces        */
#include "H5VMprivate.h" /* Vector            */

#ifdef H5_HAVE_PARALLEL

/****************/
/* Local Macros */
/****************/

/* Macros to represent different IO options */
#define H5D_ONE_LINK_CHUNK_IO          0
#define H5D_MULTI_CHUNK_IO             1
#define H5D_ONE_LINK_CHUNK_IO_MORE_OPT 2
#define H5D_MULTI_CHUNK_IO_MORE_OPT    3

/***** Macros for One linked collective IO case. *****/
/* The default value to do one linked collective IO for all chunks.
   If the average number of chunks per process is greater than this value,
      the library will create an MPI derived datatype to link all chunks to do collective IO.
      The user can set this value through an API. */

/* Macros to represent options on how to obtain chunk address for one linked-chunk IO case */
#define H5D_OBTAIN_ONE_CHUNK_ADDR_IND 0
#define H5D_OBTAIN_ALL_CHUNK_ADDR_COL 2

/* Macros to define the default ratio of obtaining all chunk addresses for one linked-chunk IO case */
#define H5D_ALL_CHUNK_ADDR_THRES_COL     30
#define H5D_ALL_CHUNK_ADDR_THRES_COL_NUM 10000

/***** Macros for multi-chunk collective IO case. *****/
/* The default value of the threshold to do collective IO for this chunk.
   If the average number of processes per chunk is greater than the default value,
   collective IO is done for this chunk.
*/

/* Macros to represent different IO modes(NONE, Independent or collective)for multiple chunk IO case */
#define H5D_CHUNK_IO_MODE_COL 1

/* Macros to represent the regularity of the selection for multiple chunk IO case. */
#define H5D_CHUNK_SELECT_REG 1

/******************/
/* Local Typedefs */
/******************/
/* Combine chunk address and chunk info into a struct for better performance. */
typedef struct H5D_chunk_addr_info_t {
    haddr_t          chunk_addr;
    H5D_chunk_info_t chunk_info;
} H5D_chunk_addr_info_t;

/* Rank 0 Bcast values */
typedef enum H5D_mpio_no_rank0_bcast_cause_t {
    H5D_MPIO_RANK0_BCAST            = 0x00,
    H5D_MPIO_RANK0_NOT_H5S_ALL      = 0x01,
    H5D_MPIO_RANK0_NOT_CONTIGUOUS   = 0x02,
    H5D_MPIO_RANK0_NOT_FIXED_SIZE   = 0x04,
    H5D_MPIO_RANK0_GREATER_THAN_2GB = 0x08
} H5D_mpio_no_rank0_bcast_cause_t;

/*
 * Information about a single chunk when performing collective filtered I/O. All
 * of the fields of one of these structs are initialized at the start of collective
 * filtered I/O in the function H5D__construct_filtered_io_info_list().
 *
 * This struct's fields are as follows:
 *
 *   index - The "Index" of the chunk in the dataset. The index of a chunk is used during
 *           the collective re-insertion of chunks into the chunk index after the collective
 *           I/O has been performed.
 *
 *   scaled - The scaled coordinates of the chunk in the dataset's file dataspace. The
 *            coordinates are used in both the collective re-allocation of space in the file
 *            and the collective re-insertion of chunks into the chunk index after the collective
 *            I/O has been performed.
 *
 *   full_overwrite - A flag which determines whether or not a chunk needs to be read from the
 *                    file when being updated. If a chunk is being fully overwritten (the entire
 *                    extent is selected in its file dataspace), then it is not necessary to
 *                    read the chunk from the file. However, if the chunk is not being fully
 *                    overwritten, it has to be read from the file in order to update the chunk
 *                    without trashing the parts of the chunk that are not selected.
 *
 *   num_writers - The total number of processors writing to this chunk. This field is used
 *                 when the new owner of a chunk is receiving messages, which contain selections in
 *                 the chunk and data to update the chunk with, from other processors which have this
 *                 chunk selected in the I/O operation. The new owner must know how many processors it
 *                 should expect messages from so that it can post an equal number of receive calls.
 *
 *   io_size - The total size of I/O to this chunk. This field is an accumulation of the size of
 *             I/O to the chunk from each processor which has the chunk selected and is used to
 *             determine the value for the previous full_overwrite flag.
 *
 *   buf - A pointer which serves the dual purpose of holding either the chunk data which is to be
 *         written to the file or the chunk data which has been read from the file.
 *
 *   chunk_states - In the case of dataset writes only, this struct is used to track a chunk's size and
 *                  address in the file before and after the filtering operation has occurred.
 *
 *                  Its fields are as follows:
 *
 *                  chunk_current - The address in the file and size of this chunk before the filtering
 *                                  operation. When reading a chunk from the file, this field is used to
 *                                  read the correct amount of bytes. It is also used when redistributing
 *                                  shared chunks among processors and as a parameter to the chunk file
 *                                  space reallocation function.
 *
 *                  new_chunk - The address in the file and size of this chunk after the filtering
 *                              operation. This field is relevant when collectively re-allocating space
 *                              in the file for all of the chunks written to in the I/O operation, as
 *                              their sizes may have changed after their data has been filtered.
 *
 *   owners - In the case of dataset writes only, this struct is used to manage which single processor
 *            will ultimately write data out to the chunk. It allows the other processors to act according
 *            to the decision and send their selection in the chunk, as well as the data they wish
 *            to update the chunk with, to the processor which is writing to the chunk.
 *
 *            Its fields are as follows:
 *
 *            original_owner - The processor which originally had this chunk selected at the beginning of
 *                             the collective filtered I/O operation. This field is currently used when
 *                             redistributing shared chunks among processors.
 *
 *            new_owner - The processor which has been selected to perform the write to this chunk.
 *
 *   async_info - In the case of dataset writes only, this struct is used by the owning processor of the
 *                chunk in order to manage the MPI send and receive calls made between it and all of
 *                the other processors which have this chunk selected in the I/O operation.
 *
 *                Its fields are as follows:
 *
 *                receive_requests_array - An array containing one MPI_Request for each of the
 *                                         asynchronous MPI receive calls the owning processor of this
 *                                         chunk makes to another processor in order to receive that
 *                                         processor's chunk modification data and selection in the chunk.
 *
 *                receive_buffer_array - An array of buffers into which the owning processor of this chunk
 *                                       will store chunk modification data and the selection in the chunk
 *                                       received from another processor.
 *
 *                num_receive_requests - The number of entries in the receive_request_array and
 *                                       receive_buffer_array fields.
 */
typedef struct H5D_filtered_collective_io_info_t {
    hsize_t index;
    hsize_t scaled[H5O_LAYOUT_NDIMS];
    hbool_t full_overwrite;
    size_t  num_writers;
    size_t  io_size;
    void *  buf;

    struct {
        H5F_block_t chunk_current;
        H5F_block_t new_chunk;
    } chunk_states;

    struct {
        int original_owner;
        int new_owner;
    } owners;

    struct {
        MPI_Request *   receive_requests_array;
        unsigned char **receive_buffer_array;
        int             num_receive_requests;
    } async_info;
} H5D_filtered_collective_io_info_t;

/********************/
/* Local Prototypes */
/********************/
static herr_t H5D__chunk_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
                                       H5D_chunk_map_t *fm);
static herr_t H5D__multi_chunk_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
                                             H5D_chunk_map_t *fm);
static herr_t H5D__multi_chunk_filtered_collective_io(H5D_io_info_t *        io_info,
                                                      const H5D_type_info_t *type_info, H5D_chunk_map_t *fm);
static herr_t H5D__link_chunk_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
                                            H5D_chunk_map_t *fm, int sum_chunk);
static herr_t H5D__link_chunk_filtered_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
                                                     H5D_chunk_map_t *fm);
static herr_t H5D__inter_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
                                       const H5S_t *file_space, const H5S_t *mem_space);
static herr_t H5D__final_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
                                       hsize_t nelmts, MPI_Datatype mpi_file_type, MPI_Datatype mpi_buf_type);
static herr_t H5D__sort_chunk(H5D_io_info_t *io_info, const H5D_chunk_map_t *fm,
                              H5D_chunk_addr_info_t chunk_addr_info_array[], int many_chunk_opt);
static herr_t H5D__obtain_mpio_mode(H5D_io_info_t *io_info, H5D_chunk_map_t *fm, uint8_t assign_io_mode[],
                                    haddr_t chunk_addr[]);
static herr_t H5D__mpio_get_sum_chunk(const H5D_io_info_t *io_info, const H5D_chunk_map_t *fm,
                                      int *sum_chunkf);
static herr_t H5D__construct_filtered_io_info_list(const H5D_io_info_t *               io_info,
                                                   const H5D_type_info_t *             type_info,
                                                   const H5D_chunk_map_t *             fm,
                                                   H5D_filtered_collective_io_info_t **chunk_list,
                                                   size_t *                            num_entries);
#if MPI_VERSION >= 3
static herr_t H5D__chunk_redistribute_shared_chunks(const H5D_io_info_t *              io_info,
                                                    const H5D_type_info_t *            type_info,
                                                    const H5D_chunk_map_t *            fm,
                                                    H5D_filtered_collective_io_info_t *local_chunk_array,
                                                    size_t *local_chunk_array_num_entries);
#endif
static herr_t H5D__mpio_array_gatherv(void *local_array, size_t local_array_num_entries,
                                      size_t array_entry_size, void **gathered_array,
                                      size_t *gathered_array_num_entries, hbool_t allgather, int root,
                                      MPI_Comm comm, int (*sort_func)(const void *, const void *));
static herr_t H5D__mpio_filtered_collective_write_type(H5D_filtered_collective_io_info_t *chunk_list,
                                                       size_t num_entries, MPI_Datatype *new_mem_type,
                                                       hbool_t *mem_type_derived, MPI_Datatype *new_file_type,
                                                       hbool_t *file_type_derived);
static herr_t H5D__filtered_collective_chunk_entry_io(H5D_filtered_collective_io_info_t *chunk_entry,
                                                      const H5D_io_info_t *              io_info,
                                                      const H5D_type_info_t *            type_info,
                                                      const H5D_chunk_map_t *            fm);
static int    H5D__cmp_chunk_addr(const void *chunk_addr_info1, const void *chunk_addr_info2);
static int    H5D__cmp_filtered_collective_io_info_entry(const void *filtered_collective_io_info_entry1,
                                                         const void *filtered_collective_io_info_entry2);
#if MPI_VERSION >= 3
static int H5D__cmp_filtered_collective_io_info_entry_owner(const void *filtered_collective_io_info_entry1,
                                                            const void *filtered_collective_io_info_entry2);
#endif

/*********************/
/* Package Variables */
/*********************/

/*******************/
/* Local Variables */
/*******************/

/*-------------------------------------------------------------------------
 * Function:    H5D__mpio_opt_possible
 *
 * Purpose:     Checks if an direct I/O transfer is possible between memory and
 *                  the file.
 *
 * Return:      Success:   Non-negative: TRUE or FALSE
 *              Failure:    Negative
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, April 3, 2002
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5D__mpio_opt_possible(const H5D_io_info_t *io_info, const H5S_t *file_space, const H5S_t *mem_space,
                       const H5D_type_info_t *type_info)
{
    H5FD_mpio_xfer_t io_xfer_mode;            /* MPI I/O transfer mode */
    unsigned         local_cause[2] = {0, 0}; /* [0] Local reason(s) for breaking collective mode */
                                              /* [1] Flag if dataset is both: H5S_ALL and small */
    unsigned global_cause[2] = {0, 0};        /* Global reason(s) for breaking collective mode */
    htri_t   is_vl_storage;       /* Whether the dataset's datatype is stored in a variable-length form */
    htri_t   ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(io_info);
    HDassert(mem_space);
    HDassert(file_space);
    HDassert(type_info);

    /* For independent I/O, get out quickly and don't try to form consensus */
    if (H5CX_get_io_xfer_mode(&io_xfer_mode) < 0)
        /* Set error flag, but keep going */
        local_cause[0] |= H5D_MPIO_ERROR_WHILE_CHECKING_COLLECTIVE_POSSIBLE;
    if (io_xfer_mode == H5FD_MPIO_INDEPENDENT)
        local_cause[0] |= H5D_MPIO_SET_INDEPENDENT;

    /* Optimized MPI types flag must be set */
    /* (based on 'HDF5_MPI_OPT_TYPES' environment variable) */
    if (!H5FD_mpi_opt_types_g)
        local_cause[0] |= H5D_MPIO_MPI_OPT_TYPES_ENV_VAR_DISABLED;

    /* Don't allow collective operations if datatype conversions need to happen */
    if (!type_info->is_conv_noop)
        local_cause[0] |= H5D_MPIO_DATATYPE_CONVERSION;

    /* Don't allow collective operations if data transform operations should occur */
    if (!type_info->is_xform_noop)
        local_cause[0] |= H5D_MPIO_DATA_TRANSFORMS;

    /* Check whether these are both simple or scalar dataspaces */
    if (!((H5S_SIMPLE == H5S_GET_EXTENT_TYPE(mem_space) || H5S_SCALAR == H5S_GET_EXTENT_TYPE(mem_space)) &&
          (H5S_SIMPLE == H5S_GET_EXTENT_TYPE(file_space) || H5S_SCALAR == H5S_GET_EXTENT_TYPE(file_space))))
        local_cause[0] |= H5D_MPIO_NOT_SIMPLE_OR_SCALAR_DATASPACES;

    /* Dataset storage must be contiguous or chunked */
    if (!(io_info->dset->shared->layout.type == H5D_CONTIGUOUS ||
          io_info->dset->shared->layout.type == H5D_CHUNKED))
        local_cause[0] |= H5D_MPIO_NOT_CONTIGUOUS_OR_CHUNKED_DATASET;

    /* check if external-file storage is used */
    if (io_info->dset->shared->dcpl_cache.efl.nused > 0)
        local_cause[0] |= H5D_MPIO_NOT_CONTIGUOUS_OR_CHUNKED_DATASET;

        /* The handling of memory space is different for chunking and contiguous
         *  storage.  For contiguous storage, mem_space and file_space won't change
         *  when it it is doing disk IO.  For chunking storage, mem_space will
         *  change for different chunks. So for chunking storage, whether we can
         *  use collective IO will defer until each chunk IO is reached.
         */

#if MPI_VERSION < 3
    /*
     * Don't allow parallel writes to filtered datasets if the MPI version
     * is less than 3. The functions needed (MPI_Mprobe and MPI_Imrecv) will
     * not be available.
     */
    if (io_info->op_type == H5D_IO_OP_WRITE && io_info->dset->shared->layout.type == H5D_CHUNKED &&
        io_info->dset->shared->dcpl_cache.pline.nused > 0)
        local_cause[0] |= H5D_MPIO_PARALLEL_FILTERED_WRITES_DISABLED;
#endif

    /* Check if we are able to do a MPI_Bcast of the data from one rank
     * instead of having all the processes involved in the collective I/O call.
     */

    /* Check to see if the process is reading the entire dataset */
    if (H5S_GET_SELECT_TYPE(file_space) != H5S_SEL_ALL)
        local_cause[1] |= H5D_MPIO_RANK0_NOT_H5S_ALL;
    /* Only perform this optimization for contigous datasets, currently */
    else if (H5D_CONTIGUOUS != io_info->dset->shared->layout.type)
        /* Flag to do a MPI_Bcast of the data from one proc instead of
         * having all the processes involved in the collective I/O.
         */
        local_cause[1] |= H5D_MPIO_RANK0_NOT_CONTIGUOUS;
    else if ((is_vl_storage = H5T_is_vl_storage(type_info->dset_type)) < 0)
        local_cause[0] |= H5D_MPIO_ERROR_WHILE_CHECKING_COLLECTIVE_POSSIBLE;
    else if (is_vl_storage)
        local_cause[1] |= H5D_MPIO_RANK0_NOT_FIXED_SIZE;
    else {
        size_t type_size; /* Size of dataset's datatype */

        /* Retrieve the size of the dataset's datatype */
        if (0 == (type_size = H5T_GET_SIZE(type_info->dset_type)))
            local_cause[0] |= H5D_MPIO_ERROR_WHILE_CHECKING_COLLECTIVE_POSSIBLE;
        else {
            hssize_t snelmts; /* [Signed] # of elements in dataset's dataspace */

            /* Retrieve the size of the dataset's datatype */
            if ((snelmts = H5S_GET_EXTENT_NPOINTS(file_space)) < 0)
                local_cause[0] |= H5D_MPIO_ERROR_WHILE_CHECKING_COLLECTIVE_POSSIBLE;
            else {
                hsize_t dset_size;

                /* Determine dataset size */
                dset_size = ((hsize_t)snelmts) * type_size;

                /* If the size of the dataset is less than 2GB then do an MPI_Bcast
                 * of the data from one process instead of having all the processes
                 * involved in the collective I/O.
                 */
                if (dset_size > ((hsize_t)(2.0F * H5_GB) - 1))
                    local_cause[1] |= H5D_MPIO_RANK0_GREATER_THAN_2GB;
            } /* end else */
        }     /* end else */
    }         /* end else */

    /* Check for independent I/O */
    if (local_cause[0] & H5D_MPIO_SET_INDEPENDENT)
        global_cause[0] = local_cause[0];
    else {
        int mpi_code; /* MPI error code */

        /* Form consensus opinion among all processes about whether to perform
         * collective I/O
         */
        if (MPI_SUCCESS !=
            (mpi_code = MPI_Allreduce(local_cause, global_cause, 2, MPI_UNSIGNED, MPI_BOR, io_info->comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Allreduce failed", mpi_code)
    } /* end else */

    /* Set the local & global values of no-collective-cause in the API context */
    H5CX_set_mpio_local_no_coll_cause(local_cause[0]);
    H5CX_set_mpio_global_no_coll_cause(global_cause[0]);

    /* Set read-with-rank0-and-bcast flag if possible */
    if (global_cause[0] == 0 && global_cause[1] == 0) {
        H5CX_set_mpio_rank0_bcast(TRUE);
#ifdef H5_HAVE_INSTRUMENTED_LIBRARY
        H5CX_test_set_mpio_coll_rank0_bcast(TRUE);
#endif /* H5_HAVE_INSTRUMENTED_LIBRARY */
    }  /* end if */

    /* Set the return value, based on the global cause */
    ret_value = global_cause[0] > 0 ? FALSE : TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D__mpio_opt_possible() */

/*-------------------------------------------------------------------------
 * Function:    H5D__mpio_select_read
 *
 * Purpose:     MPI-IO function to read directly from app buffer to file.
 *
 * Return:      non-negative on success, negative on failure.
 *
 * Programmer:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__mpio_select_read(const H5D_io_info_t *io_info, const H5D_type_info_t H5_ATTR_UNUSED *type_info,
                      hsize_t mpi_buf_count, const H5S_t H5_ATTR_UNUSED *file_space,
                      const H5S_t H5_ATTR_UNUSED *mem_space)
{
    const H5D_contig_storage_t *store_contig =
        &(io_info->store->contig); /* Contiguous storage info for this I/O operation */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    H5_CHECK_OVERFLOW(mpi_buf_count, hsize_t, size_t);
    if (H5F_shared_block_read(io_info->f_sh, H5FD_MEM_DRAW, store_contig->dset_addr, (size_t)mpi_buf_count,
                              io_info->u.rbuf) < 0)
        HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "can't finish collective parallel read")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__mpio_select_read() */

/*-------------------------------------------------------------------------
 * Function:    H5D__mpio_select_write
 *
 * Purpose:     MPI-IO function to write directly from app buffer to file.
 *
 * Return:      non-negative on success, negative on failure.
 *
 * Programmer:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__mpio_select_write(const H5D_io_info_t *io_info, const H5D_type_info_t H5_ATTR_UNUSED *type_info,
                       hsize_t mpi_buf_count, const H5S_t H5_ATTR_UNUSED *file_space,
                       const H5S_t H5_ATTR_UNUSED *mem_space)
{
    const H5D_contig_storage_t *store_contig =
        &(io_info->store->contig); /* Contiguous storage info for this I/O operation */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    /*OKAY: CAST DISCARDS CONST QUALIFIER*/
    H5_CHECK_OVERFLOW(mpi_buf_count, hsize_t, size_t);
    if (H5F_shared_block_write(io_info->f_sh, H5FD_MEM_DRAW, store_contig->dset_addr, (size_t)mpi_buf_count,
                               io_info->u.wbuf) < 0)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "can't finish collective parallel write")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__mpio_select_write() */

/*-------------------------------------------------------------------------
 * Function:    H5D__mpio_array_gatherv
 *
 * Purpose:     Given an array, specified in local_array, by each processor
 *              calling this function, collects each array into a single
 *              array which is then either gathered to the processor
 *              specified by root, when allgather is false, or is
 *              distributed back to all processors when allgather is true.
 *
 *              The number of entries in the array contributed by an
 *              individual processor and the size of each entry should be
 *              specified in local_array_num_entries and array_entry_size,
 *              respectively.
 *
 *              The MPI communicator to use should be specified for comm.
 *
 *              If the sort_func argument is supplied, the array is sorted
 *              before the function returns.
 *
 *              Note: if allgather is specified as true, root is ignored.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Jordan Henderson
 *              Sunday, April 9th, 2017
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__mpio_array_gatherv(void *local_array, size_t local_array_num_entries, size_t array_entry_size,
                        void **_gathered_array, size_t *_gathered_array_num_entries, hbool_t allgather,
                        int root, MPI_Comm comm, int (*sort_func)(const void *, const void *))
{
    size_t gathered_array_num_entries = 0;    /* The size of the newly-constructed array */
    void * gathered_array             = NULL; /* The newly-constructed array returned to the caller */
    int *receive_counts_array = NULL; /* Array containing number of entries each processor is contributing */
    int *displacements_array =
        NULL; /* Array of displacements where each processor places its data in the final array */
    int    mpi_code, mpi_rank, mpi_size;
    int    sendcount;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    HDassert(_gathered_array);
    HDassert(_gathered_array_num_entries);

    MPI_Comm_size(comm, &mpi_size);
    MPI_Comm_rank(comm, &mpi_rank);

    /* Determine the size of the end result array by collecting the number
     * of entries contributed by each processor into a single total.
     */
    if (MPI_SUCCESS != (mpi_code = MPI_Allreduce(&local_array_num_entries, &gathered_array_num_entries, 1,
                                                 MPI_INT, MPI_SUM, comm)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Allreduce failed", mpi_code)

    /* If 0 entries resulted from the collective operation, no processor is contributing anything and there is
     * nothing to do */
    if (gathered_array_num_entries > 0) {
        /*
         * If gathering to all processors, all processors need to allocate space for the resulting array, as
         * well as the receive counts and displacements arrays for the collective MPI_Allgatherv call.
         * Otherwise, only the root processor needs to allocate the space for an MPI_Gatherv call.
         */
        if (allgather || (mpi_rank == root)) {
            if (NULL == (gathered_array = H5MM_malloc(gathered_array_num_entries * array_entry_size)))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate gathered array")

            if (NULL == (receive_counts_array = (int *)H5MM_malloc((size_t)mpi_size * sizeof(int))))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate receive counts array")

            if (NULL == (displacements_array = (int *)H5MM_malloc((size_t)mpi_size * sizeof(int))))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate receive displacements array")
        } /* end if */

        /*
         * If gathering to all processors, inform each processor of how many entries each other processor is
         * contributing to the resulting array by collecting the counts into each processor's "receive counts"
         * array. Otherwise, inform only the root processor of how many entries each other processor is
         * contributing.
         */
        if (allgather) {
            if (MPI_SUCCESS != (mpi_code = MPI_Allgather(&local_array_num_entries, 1, MPI_INT,
                                                         receive_counts_array, 1, MPI_INT, comm)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Allgather failed", mpi_code)
        } /* end if */
        else {
            if (MPI_SUCCESS != (mpi_code = MPI_Gather(&local_array_num_entries, 1, MPI_INT,
                                                      receive_counts_array, 1, MPI_INT, root, comm)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Gather failed", mpi_code)
        } /* end else */

        if (allgather || (mpi_rank == root)) {
            size_t i;

            /* Multiply each receive count by the size of the array entry, since the data is sent as bytes. */
            for (i = 0; i < (size_t)mpi_size; i++)
                H5_CHECKED_ASSIGN(receive_counts_array[i], int,
                                  (size_t)receive_counts_array[i] * array_entry_size, size_t);

            /* Set receive buffer offsets for the collective MPI_Allgatherv/MPI_Gatherv call. */
            displacements_array[0] = 0;
            for (i = 1; i < (size_t)mpi_size; i++)
                displacements_array[i] = displacements_array[i - 1] + receive_counts_array[i - 1];
        } /* end if */

        /* As the data is sent as bytes, calculate the true sendcount for the data. */
        H5_CHECKED_ASSIGN(sendcount, int, local_array_num_entries *array_entry_size, size_t);

        if (allgather) {
            if (MPI_SUCCESS !=
                (mpi_code = MPI_Allgatherv(local_array, sendcount, MPI_BYTE, gathered_array,
                                           receive_counts_array, displacements_array, MPI_BYTE, comm)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Allgatherv failed", mpi_code)
        } /* end if */
        else {
            if (MPI_SUCCESS !=
                (mpi_code = MPI_Gatherv(local_array, sendcount, MPI_BYTE, gathered_array,
                                        receive_counts_array, displacements_array, MPI_BYTE, root, comm)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Gatherv failed", mpi_code)
        } /* end else */

        if (sort_func && (allgather || (mpi_rank == root)))
            HDqsort(gathered_array, gathered_array_num_entries, array_entry_size, sort_func);
    } /* end if */

    *_gathered_array             = gathered_array;
    *_gathered_array_num_entries = gathered_array_num_entries;

done:
    if (receive_counts_array)
        H5MM_free(receive_counts_array);
    if (displacements_array)
        H5MM_free(displacements_array);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__mpio_array_gatherv() */

/*-------------------------------------------------------------------------
 * Function:    H5D__mpio_get_sum_chunk
 *
 * Purpose:     Routine for obtaining total number of chunks to cover
 *              hyperslab selection selected by all processors.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__mpio_get_sum_chunk(const H5D_io_info_t *io_info, const H5D_chunk_map_t *fm, int *sum_chunkf)
{
    int    num_chunkf; /* Number of chunks to iterate over */
    size_t ori_num_chunkf;
    int    mpi_code; /* MPI return code */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /* Get the number of chunks to perform I/O on */
    num_chunkf     = 0;
    ori_num_chunkf = H5SL_count(fm->sel_chunks);
    H5_CHECKED_ASSIGN(num_chunkf, int, ori_num_chunkf, size_t);

    /* Determine the summation of number of chunks for all processes */
    if (MPI_SUCCESS !=
        (mpi_code = MPI_Allreduce(&num_chunkf, sum_chunkf, 1, MPI_INT, MPI_SUM, io_info->comm)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Allreduce failed", mpi_code)

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__mpio_get_sum_chunk() */

/*-------------------------------------------------------------------------
 * Function:    H5D__contig_collective_read
 *
 * Purpose:     Reads directly from contiguous data in file into application
 *              memory using collective I/O.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, March  4, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__contig_collective_read(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
                            hsize_t H5_ATTR_UNUSED nelmts, const H5S_t *file_space, const H5S_t *mem_space,
                            H5D_chunk_map_t H5_ATTR_UNUSED *fm)
{
    H5D_mpio_actual_io_mode_t actual_io_mode = H5D_MPIO_CONTIGUOUS_COLLECTIVE;
    herr_t                    ret_value      = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(H5FD_MPIO == H5F_DRIVER_ID(io_info->dset->oloc.file));

    /* Call generic internal collective I/O routine */
    if (H5D__inter_collective_io(io_info, type_info, file_space, mem_space) < 0)
        HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "couldn't finish shared collective MPI-IO")

    /* Set the actual I/O mode property. internal_collective_io will not break to
     * independent I/O, so we set it here.
     */
    H5CX_set_mpio_actual_io_mode(actual_io_mode);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__contig_collective_read() */

/*-------------------------------------------------------------------------
 * Function:    H5D__contig_collective_write
 *
 * Purpose:     Write directly to contiguous data in file from application
 *              memory using collective I/O.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, March  4, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__contig_collective_write(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
                             hsize_t H5_ATTR_UNUSED nelmts, const H5S_t *file_space, const H5S_t *mem_space,
                             H5D_chunk_map_t H5_ATTR_UNUSED *fm)
{
    H5D_mpio_actual_io_mode_t actual_io_mode = H5D_MPIO_CONTIGUOUS_COLLECTIVE;
    herr_t                    ret_value      = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(H5FD_MPIO == H5F_DRIVER_ID(io_info->dset->oloc.file));

    /* Call generic internal collective I/O routine */
    if (H5D__inter_collective_io(io_info, type_info, file_space, mem_space) < 0)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "couldn't finish shared collective MPI-IO")

    /* Set the actual I/O mode property. internal_collective_io will not break to
     * independent I/O, so we set it here.
     */
    H5CX_set_mpio_actual_io_mode(actual_io_mode);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__contig_collective_write() */

/*-------------------------------------------------------------------------
 * Function:    H5D__chunk_collective_io
 *
 * Purpose:     Routine for
 *              1) choose an IO option:
 *                    a) One collective IO defined by one MPI derived datatype to link through all chunks
 *              or    b) multiple chunk IOs,to do MPI-IO for each chunk, the IO mode may be adjusted
 *                       due to the selection pattern for each chunk.
 *              For option a)
 *                      1. Sort the chunk address, obtain chunk info according to the sorted chunk address
 *                      2. Build up MPI derived datatype for each chunk
 *                      3. Build up the final MPI derived datatype
 *                      4. Set up collective IO property list
 *                      5. Do IO
 *              For option b)
 *                      1. Use MPI_gather and MPI_Bcast to obtain information of *collective/independent/none*
 *                         IO mode for each chunk of the selection
 *                      2. Depending on whether the IO mode is collective or independent or none,
 *                         Create either MPI derived datatype for each chunk to do collective IO or
 *                         just do independent IO or independent IO with file set view
 *                      3. Set up collective IO property list for collective mode
 *                      4. DO IO
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 * Modification:
 *  - Refctore to remove multi-chunk-without-opimization feature and update for
 *    multi-chunk-io accordingly
 * Programmer: Jonathan Kim
 * Date: 2012-10-10
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__chunk_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info, H5D_chunk_map_t *fm)
{
    H5FD_mpio_chunk_opt_t chunk_opt_mode;
    int                   io_option = H5D_MULTI_CHUNK_IO_MORE_OPT;
    int                   sum_chunk = -1;
#ifdef H5_HAVE_INSTRUMENTED_LIBRARY
    htri_t temp_not_link_io = FALSE;
#endif
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(io_info);
    HDassert(io_info->using_mpi_vfd);
    HDassert(type_info);
    HDassert(fm);

    /* Disable collective metadata reads for chunked dataset I/O operations
     * in order to prevent potential hangs */
    H5CX_set_coll_metadata_read(FALSE);

    /* Check the optional property list for the collective chunk IO optimization option */
    if (H5CX_get_mpio_chunk_opt_mode(&chunk_opt_mode) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "couldn't get chunk optimization option")

    if (H5FD_MPIO_CHUNK_ONE_IO == chunk_opt_mode)
        io_option = H5D_ONE_LINK_CHUNK_IO; /*no opt*/
    /* direct request to multi-chunk-io */
    else if (H5FD_MPIO_CHUNK_MULTI_IO == chunk_opt_mode)
        io_option = H5D_MULTI_CHUNK_IO;
    /* via default path. branch by num threshold */
    else {
        unsigned one_link_chunk_io_threshold; /* Threshold to use single collective I/O for all chunks */
        int      mpi_size;                    /* Number of processes in MPI job */

        if (H5D__mpio_get_sum_chunk(io_info, fm, &sum_chunk) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSWAP, FAIL,
                        "unable to obtain the total chunk number of all processes");
        if ((mpi_size = H5F_mpi_get_size(io_info->dset->oloc.file)) < 0)
            HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi size")

        /* Get the chunk optimization option threshold */
        if (H5CX_get_mpio_chunk_opt_num(&one_link_chunk_io_threshold) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL,
                        "couldn't get chunk optimization option threshold value")

        /* step 1: choose an IO option */
        /* If the average number of chunk per process is greater than a threshold, we will do one link chunked
         * IO. */
        if ((unsigned)sum_chunk / (unsigned)mpi_size >= one_link_chunk_io_threshold)
            io_option = H5D_ONE_LINK_CHUNK_IO_MORE_OPT;
#ifdef H5_HAVE_INSTRUMENTED_LIBRARY
        else
            temp_not_link_io = TRUE;
#endif /* H5_HAVE_INSTRUMENTED_LIBRARY */
    }  /* end else */

#ifdef H5_HAVE_INSTRUMENTED_LIBRARY
    {
        /*** Set collective chunk user-input optimization APIs. ***/
        if (H5D_ONE_LINK_CHUNK_IO == io_option) {
            if (H5CX_test_set_mpio_coll_chunk_link_hard(0) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to set property value")
        } /* end if */
        else if (H5D_MULTI_CHUNK_IO == io_option) {
            if (H5CX_test_set_mpio_coll_chunk_multi_hard(0) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to set property value")
        } /* end else-if */
        else if (H5D_ONE_LINK_CHUNK_IO_MORE_OPT == io_option) {
            if (H5CX_test_set_mpio_coll_chunk_link_num_true(0) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to set property value")
        } /* end if */
        else if (temp_not_link_io) {
            if (H5CX_test_set_mpio_coll_chunk_link_num_false(0) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to set property value")
        } /* end if */
    }
#endif /* H5_HAVE_INSTRUMENTED_LIBRARY */

    /* step 2:  Go ahead to do IO.*/
    switch (io_option) {
        case H5D_ONE_LINK_CHUNK_IO:
        case H5D_ONE_LINK_CHUNK_IO_MORE_OPT:
            /* Check if there are any filters in the pipeline */
            if (io_info->dset->shared->dcpl_cache.pline.nused > 0) {
                /* For now, Multi-chunk IO must be forced for parallel filtered read,
                 * so that data can be unfiltered as it is received. There is significant
                 * complexity in unfiltering the data when it is read all at once into a
                 * single buffer.
                 */
                if (io_info->op_type == H5D_IO_OP_READ) {
                    if (H5D__multi_chunk_filtered_collective_io(io_info, type_info, fm) < 0)
                        HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL,
                                    "couldn't finish optimized multiple filtered chunk MPI-IO")
                } /* end if */
                else if (H5D__link_chunk_filtered_collective_io(io_info, type_info, fm) < 0)
                    HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish filtered linked chunk MPI-IO")
            } /* end if */
            else
                /* Perform unfiltered link chunk collective IO */
                if (H5D__link_chunk_collective_io(io_info, type_info, fm, sum_chunk) < 0)
                HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish linked chunk MPI-IO")
            break;

        case H5D_MULTI_CHUNK_IO: /* direct request to do multi-chunk IO */
        default:                 /* multiple chunk IO via threshold */
            /* Check if there are any filters in the pipeline */
            if (io_info->dset->shared->dcpl_cache.pline.nused > 0) {
                if (H5D__multi_chunk_filtered_collective_io(io_info, type_info, fm) < 0)
                    HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL,
                                "couldn't finish optimized multiple filtered chunk MPI-IO")
            } /* end if */
            else
                /* Perform unfiltered multi chunk collective IO */
                if (H5D__multi_chunk_collective_io(io_info, type_info, fm) < 0)
                HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish optimized multiple chunk MPI-IO")
            break;
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_collective_io */

/*-------------------------------------------------------------------------
 * Function:    H5D__chunk_collective_read
 *
 * Purpose:     Reads directly from chunks in file into application memory
 *              using collective I/O.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, March  4, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__chunk_collective_read(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
                           hsize_t H5_ATTR_UNUSED nelmts, const H5S_t H5_ATTR_UNUSED *file_space,
                           const H5S_t H5_ATTR_UNUSED *mem_space, H5D_chunk_map_t *fm)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Call generic selection operation */
    if (H5D__chunk_collective_io(io_info, type_info, fm) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_READERROR, FAIL, "read error")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_collective_read() */

/*-------------------------------------------------------------------------
 * Function:    H5D__chunk_collective_write
 *
 * Purpose:     Write directly to chunks in file from application memory
 *              using collective I/O.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, March  4, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D__chunk_collective_write(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
                            hsize_t H5_ATTR_UNUSED nelmts, const H5S_t H5_ATTR_UNUSED *file_space,
                            const H5S_t H5_ATTR_UNUSED *mem_space, H5D_chunk_map_t *fm)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Call generic selection operation */
    if (H5D__chunk_collective_io(io_info, type_info, fm) < 0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_WRITEERROR, FAIL, "write error")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_collective_write() */

/*-------------------------------------------------------------------------
 * Function:    H5D__link_chunk_collective_io
 *
 * Purpose:     Routine for one collective IO with one MPI derived datatype to link with all chunks
 *
 *                      1. Sort the chunk address and chunk info
 *                      2. Build up MPI derived datatype for each chunk
 *                      3. Build up the final MPI derived datatype
 *                      4. Use common collective IO routine to do MPI-IO
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__link_chunk_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info, H5D_chunk_map_t *fm,
                              int sum_chunk)
{
    H5D_chunk_addr_info_t *chunk_addr_info_array = NULL;
    MPI_Datatype           chunk_final_mtype; /* Final memory MPI datatype for all chunks with seletion */
    hbool_t                chunk_final_mtype_is_derived = FALSE;
    MPI_Datatype           chunk_final_ftype; /* Final file MPI datatype for all chunks with seletion */
    hbool_t                chunk_final_ftype_is_derived = FALSE;
    H5D_storage_t          ctg_store; /* Storage info for "fake" contiguous dataset */
    size_t                 total_chunks;
    MPI_Datatype *         chunk_mtype          = NULL;
    MPI_Datatype *         chunk_ftype          = NULL;
    MPI_Aint *             chunk_disp_array     = NULL;
    MPI_Aint *             chunk_mem_disp_array = NULL;
    hbool_t *              chunk_mft_is_derived_array =
        NULL; /* Flags to indicate each chunk's MPI file datatype is derived */
    hbool_t *chunk_mbt_is_derived_array =
        NULL;                            /* Flags to indicate each chunk's MPI memory datatype is derived */
    int *  chunk_mpi_file_counts = NULL; /* Count of MPI file datatype for each chunk */
    int *  chunk_mpi_mem_counts  = NULL; /* Count of MPI memory datatype for each chunk */
    int    mpi_code;                     /* MPI return code */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /* Set the actual-chunk-opt-mode property. */
    H5CX_set_mpio_actual_chunk_opt(H5D_MPIO_LINK_CHUNK);

    /* Set the actual-io-mode property.
     * Link chunk I/O does not break to independent, so can set right away */
    H5CX_set_mpio_actual_io_mode(H5D_MPIO_CHUNK_COLLECTIVE);

    /* Get the sum # of chunks, if not already available */
    if (sum_chunk < 0) {
        if (H5D__mpio_get_sum_chunk(io_info, fm, &sum_chunk) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSWAP, FAIL,
                        "unable to obtain the total chunk number of all processes");
    } /* end if */

    /* Retrieve total # of chunks in dataset */
    H5_CHECKED_ASSIGN(total_chunks, size_t, fm->layout->u.chunk.nchunks, hsize_t);

    /* Handle special case when dataspace dimensions only allow one chunk in
     *  the dataset.  [This sometimes is used by developers who want the
     *  equivalent of compressed contiguous datasets - QAK]
     */
    if (total_chunks == 1) {
        H5SL_node_t *chunk_node; /* Pointer to chunk node for selection */
        H5S_t *      fspace;     /* Dataspace describing chunk & selection in it */
        H5S_t *      mspace;     /* Dataspace describing selection in memory corresponding to this chunk */

        /* Check for this process having selection in this chunk */
        chunk_node = H5SL_first(fm->sel_chunks);

        if (chunk_node == NULL) {
            /* Set the dataspace info for I/O to NULL, this process doesn't have any I/O to perform */
            fspace = mspace = NULL;

            /* Initialize chunk address */
            ctg_store.contig.dset_addr = 0;
        } /* end if */
        else {
            H5D_chunk_ud_t    udata;      /* User data for querying chunk info */
            H5D_chunk_info_t *chunk_info; /* Info for chunk in skiplist */

            /* Get the chunk info, for the selection in the chunk */
            if (NULL == (chunk_info = (H5D_chunk_info_t *)H5SL_item(chunk_node)))
                HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL, "couldn't get chunk info from skip list")

            /* Set the dataspace info for I/O */
            fspace = chunk_info->fspace;
            mspace = chunk_info->mspace;

            /* Look up address of chunk */
            if (H5D__chunk_lookup(io_info->dset, chunk_info->scaled, &udata) < 0)
                HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL, "couldn't get chunk address")
            ctg_store.contig.dset_addr = udata.chunk_block.offset;
        } /* end else */

        /* Set up the base storage address for this chunk */
        io_info->store = &ctg_store;

#ifdef H5D_DEBUG
        if (H5DEBUG(D))
            HDfprintf(H5DEBUG(D), "before inter_collective_io for total chunk = 1 \n");
#endif

        /* Perform I/O */
        if (H5D__inter_collective_io(io_info, type_info, fspace, mspace) < 0)
            HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL, "couldn't finish shared collective MPI-IO")
    } /* end if */
    else {
        hsize_t mpi_buf_count; /* Number of MPI types */
        size_t  num_chunk;     /* Number of chunks for this process */
        size_t  u;             /* Local index variable */

        /* Get the number of chunks with a selection */
        num_chunk = H5SL_count(fm->sel_chunks);
        H5_CHECK_OVERFLOW(num_chunk, size_t, int);

#ifdef H5D_DEBUG
        if (H5DEBUG(D))
            HDfprintf(H5DEBUG(D), "total_chunks = %zu, num_chunk = %zu\n", total_chunks, num_chunk);
#endif

        /* Set up MPI datatype for chunks selected */
        if (num_chunk) {
            /* Allocate chunking information */
            if (NULL == (chunk_addr_info_array =
                             (H5D_chunk_addr_info_t *)H5MM_malloc(num_chunk * sizeof(H5D_chunk_addr_info_t))))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate chunk array buffer")
            if (NULL == (chunk_mtype = (MPI_Datatype *)H5MM_malloc(num_chunk * sizeof(MPI_Datatype))))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL,
                            "couldn't allocate chunk memory datatype buffer")
            if (NULL == (chunk_ftype = (MPI_Datatype *)H5MM_malloc(num_chunk * sizeof(MPI_Datatype))))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate chunk file datatype buffer")
            if (NULL == (chunk_disp_array = (MPI_Aint *)H5MM_malloc(num_chunk * sizeof(MPI_Aint))))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL,
                            "couldn't allocate chunk file displacement buffer")
            if (NULL == (chunk_mem_disp_array = (MPI_Aint *)H5MM_calloc(num_chunk * sizeof(MPI_Aint))))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL,
                            "couldn't allocate chunk memory displacement buffer")
            if (NULL == (chunk_mpi_mem_counts = (int *)H5MM_calloc(num_chunk * sizeof(int))))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate chunk memory counts buffer")
            if (NULL == (chunk_mpi_file_counts = (int *)H5MM_calloc(num_chunk * sizeof(int))))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate chunk file counts buffer")
            if (NULL == (chunk_mbt_is_derived_array = (hbool_t *)H5MM_calloc(num_chunk * sizeof(hbool_t))))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL,
                            "couldn't allocate chunk memory is derived datatype flags buffer")
            if (NULL == (chunk_mft_is_derived_array = (hbool_t *)H5MM_calloc(num_chunk * sizeof(hbool_t))))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL,
                            "couldn't allocate chunk file is derived datatype flags buffer")

#ifdef H5D_DEBUG
            if (H5DEBUG(D))
                HDfprintf(H5DEBUG(D), "before sorting the chunk address \n");
#endif
            /* Sort the chunk address */
            if (H5D__sort_chunk(io_info, fm, chunk_addr_info_array, sum_chunk) < 0)
                HGOTO_ERROR(H5E_DATASPACE, H5E_CANTSWAP, FAIL, "unable to sort chunk address")
            ctg_store.contig.dset_addr = chunk_addr_info_array[0].chunk_addr;

#ifdef H5D_DEBUG
            if (H5DEBUG(D))
                HDfprintf(H5DEBUG(D), "after sorting the chunk address \n");
#endif

            /* Obtain MPI derived datatype from all individual chunks */
            for (u = 0; u < num_chunk; u++) {
                hsize_t *permute_map = NULL; /* array that holds the mapping from the old,
                                                out-of-order displacements to the in-order
                                                displacements of the MPI datatypes of the
                                                point selection of the file space */
                hbool_t is_permuted = FALSE;

                /* Obtain disk and memory MPI derived datatype */
                /* NOTE: The permute_map array can be allocated within H5S_mpio_space_type
                 *              and will be fed into the next call to H5S_mpio_space_type
                 *              where it will be freed.
                 */
                if (H5S_mpio_space_type(chunk_addr_info_array[u].chunk_info.fspace, type_info->src_type_size,
                                        &chunk_ftype[u],                  /* OUT: datatype created */
                                        &chunk_mpi_file_counts[u],        /* OUT */
                                        &(chunk_mft_is_derived_array[u]), /* OUT */
                                        TRUE,                             /* this is a file space,
                                                                             so permute the
                                                                             datatype if the point
                                                                             selections are out of
                                                                             order */
                                        &permute_map,                     /* OUT: a map to indicate the
                                                                             permutation of points
                                                                             selected in case they
                                                                             are out of order */
                                        &is_permuted /* OUT */) < 0)
                    HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "couldn't create MPI file type")
                /* Sanity check */
                if (is_permuted)
                    HDassert(permute_map);
                if (H5S_mpio_space_type(chunk_addr_info_array[u].chunk_info.mspace, type_info->dst_type_size,
                                        &chunk_mtype[u], &chunk_mpi_mem_counts[u],
                                        &(chunk_mbt_is_derived_array[u]), FALSE, /* this is a memory
                                                                                    space, so if the file
                                                                                    space is not
                                                                                    permuted, there is no
                                                                                    need to permute the
                                                                                    datatype if the point
                                                                                    selections are out of
                                                                                    order*/
                                        &permute_map,                            /* IN: the permutation map
                                                                                    generated by the
                                                                                    file_space selection
                                                                                    and applied to the
                                                                                    memory selection */
                                        &is_permuted /* IN */) < 0)
                    HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "couldn't create MPI buf type")
                /* Sanity check */
                if (is_permuted)
                    HDassert(!permute_map);

                /* Chunk address relative to the first chunk */
                chunk_addr_info_array[u].chunk_addr -= ctg_store.contig.dset_addr;

                /* Assign chunk address to MPI displacement */
                /* (assume MPI_Aint big enough to hold it) */
                chunk_disp_array[u] = (MPI_Aint)chunk_addr_info_array[u].chunk_addr;
            } /* end for */

            /* Create final MPI derived datatype for the file */
            if (MPI_SUCCESS !=
                (mpi_code = MPI_Type_create_struct((int)num_chunk, chunk_mpi_file_counts, chunk_disp_array,
                                                   chunk_ftype, &chunk_final_ftype)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_create_struct failed", mpi_code)
            if (MPI_SUCCESS != (mpi_code = MPI_Type_commit(&chunk_final_ftype)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)
            chunk_final_ftype_is_derived = TRUE;

            /* Create final MPI derived datatype for memory */
            if (MPI_SUCCESS !=
                (mpi_code = MPI_Type_create_struct((int)num_chunk, chunk_mpi_mem_counts, chunk_mem_disp_array,
                                                   chunk_mtype, &chunk_final_mtype)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_create_struct failed", mpi_code)
            if (MPI_SUCCESS != (mpi_code = MPI_Type_commit(&chunk_final_mtype)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)
            chunk_final_mtype_is_derived = TRUE;

            /* Free the file & memory MPI datatypes for each chunk */
            for (u = 0; u < num_chunk; u++) {
                if (chunk_mbt_is_derived_array[u])
                    if (MPI_SUCCESS != (mpi_code = MPI_Type_free(chunk_mtype + u)))
                        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)

                if (chunk_mft_is_derived_array[u])
                    if (MPI_SUCCESS != (mpi_code = MPI_Type_free(chunk_ftype + u)))
                        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)
            } /* end for */

            /* We have a single, complicated MPI datatype for both memory & file */
            mpi_buf_count = (hsize_t)1;
        }      /* end if */
        else { /* no selection at all for this process */
            ctg_store.contig.dset_addr = 0;

            /* Set the MPI datatype */
            chunk_final_ftype = MPI_BYTE;
            chunk_final_mtype = MPI_BYTE;

            /* No chunks selected for this process */
            mpi_buf_count = (hsize_t)0;
        } /* end else */
#ifdef H5D_DEBUG
        if (H5DEBUG(D))
            HDfprintf(H5DEBUG(D), "before coming to final collective IO\n");
#endif

        /* Set up the base storage address for this chunk */
        io_info->store = &ctg_store;

        /* Perform final collective I/O operation */
        if (H5D__final_collective_io(io_info, type_info, mpi_buf_count, chunk_final_ftype,
                                     chunk_final_mtype) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish MPI-IO")
    } /* end else */

done:
#ifdef H5D_DEBUG
    if (H5DEBUG(D))
        HDfprintf(H5DEBUG(D), "before freeing memory inside H5D_link_collective_io ret_value = %d\n",
                  ret_value);
#endif
    /* Release resources */
    if (chunk_addr_info_array)
        H5MM_xfree(chunk_addr_info_array);
    if (chunk_mtype)
        H5MM_xfree(chunk_mtype);
    if (chunk_ftype)
        H5MM_xfree(chunk_ftype);
    if (chunk_disp_array)
        H5MM_xfree(chunk_disp_array);
    if (chunk_mem_disp_array)
        H5MM_xfree(chunk_mem_disp_array);
    if (chunk_mpi_mem_counts)
        H5MM_xfree(chunk_mpi_mem_counts);
    if (chunk_mpi_file_counts)
        H5MM_xfree(chunk_mpi_file_counts);
    if (chunk_mbt_is_derived_array)
        H5MM_xfree(chunk_mbt_is_derived_array);
    if (chunk_mft_is_derived_array)
        H5MM_xfree(chunk_mft_is_derived_array);

    /* Free the MPI buf and file types, if they were derived */
    if (chunk_final_mtype_is_derived && MPI_SUCCESS != (mpi_code = MPI_Type_free(&chunk_final_mtype)))
        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)
    if (chunk_final_ftype_is_derived && MPI_SUCCESS != (mpi_code = MPI_Type_free(&chunk_final_ftype)))
        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__link_chunk_collective_io */

/*-------------------------------------------------------------------------
 * Function:    H5D__link_chunk_filtered_collective_io
 *
 * Purpose:     Routine for one collective IO with one MPI derived datatype
 *              to link with all filtered chunks
 *
 *              1. Construct a list of selected chunks in the collective IO
 *                 operation
 *                 A. If any chunk is being written to by more than 1
 *                    process, the process writing to the chunk which
 *                    currently has the least amount of chunks assigned
 *                    to it becomes the new owner (in the case of ties,
 *                    the lowest MPI rank becomes the new owner)
 *              2. If the operation is a write operation
 *                 A. Loop through each chunk in the operation
 *                    I. If this is not a full overwrite of the chunk
 *                       a) Read the chunk from file and pass the chunk
 *                          through the filter pipeline in reverse order
 *                          (Unfilter the chunk)
 *                    II. Update the chunk data with the modifications from
 *                        the owning process
 *                    III. Receive any modification data from other
 *                         processes and update the chunk data with these
 *                         modifications
 *                    IV. Filter the chunk
 *                 B. Contribute the modified chunks to an array gathered
 *                    by all processes which contains the new sizes of
 *                    every chunk modified in the collective IO operation
 *                 C. All processes collectively re-allocate each chunk
 *                    from the gathered array with their new sizes after
 *                    the filter operation
 *                 D. If this process has any chunks selected in the IO
 *                    operation, create an MPI derived type for memory and
 *                    file to write out the process' selected chunks to the
 *                    file
 *                 E. Perform the collective write
 *                 F. All processes collectively re-insert each modified
 *                    chunk from the gathered array into the chunk index
 *
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Jordan Henderson
 *              Friday, Nov. 4th, 2016
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__link_chunk_filtered_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
                                       H5D_chunk_map_t *fm)
{
    H5D_filtered_collective_io_info_t *chunk_list = NULL; /* The list of chunks being read/written */
    H5D_filtered_collective_io_info_t *collective_chunk_list =
        NULL;                /* The list of chunks used during collective operations */
    H5D_storage_t ctg_store; /* Chunk storage information as contiguous dataset */
    MPI_Datatype  mem_type             = MPI_BYTE;
    MPI_Datatype  file_type            = MPI_BYTE;
    hbool_t       mem_type_is_derived  = FALSE;
    hbool_t       file_type_is_derived = FALSE;
    size_t        chunk_list_num_entries;
    size_t        collective_chunk_list_num_entries;
    size_t *      num_chunks_selected_array = NULL; /* Array of number of chunks selected on each process */
    size_t        i;                                /* Local index variable */
    int           mpi_rank, mpi_size, mpi_code;
    herr_t        ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    HDassert(io_info);
    HDassert(type_info);
    HDassert(fm);

    /* Obtain the current rank of the process and the number of processes */
    if ((mpi_rank = H5F_mpi_get_rank(io_info->dset->oloc.file)) < 0)
        HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi rank")
    if ((mpi_size = H5F_mpi_get_size(io_info->dset->oloc.file)) < 0)
        HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi size")

    /* Set the actual-chunk-opt-mode property. */
    H5CX_set_mpio_actual_chunk_opt(H5D_MPIO_LINK_CHUNK);

    /* Set the actual-io-mode property.
     * Link chunk filtered I/O does not break to independent, so can set right away
     */
    H5CX_set_mpio_actual_io_mode(H5D_MPIO_CHUNK_COLLECTIVE);

    /* Build a list of selected chunks in the collective io operation */
    if (H5D__construct_filtered_io_info_list(io_info, type_info, fm, &chunk_list, &chunk_list_num_entries) <
        0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "couldn't construct filtered I/O info list")

    if (io_info->op_type == H5D_IO_OP_WRITE) { /* Filtered collective write */
        H5D_chk_idx_info_t index_info;
        H5D_chunk_ud_t     udata;
        hsize_t            mpi_buf_count;

        /* Construct chunked index info */
        index_info.f       = io_info->dset->oloc.file;
        index_info.pline   = &(io_info->dset->shared->dcpl_cache.pline);
        index_info.layout  = &(io_info->dset->shared->layout.u.chunk);
        index_info.storage = &(io_info->dset->shared->layout.storage.u.chunk);

        /* Set up chunk information for insertion to chunk index */
        udata.common.layout  = index_info.layout;
        udata.common.storage = index_info.storage;
        udata.filter_mask    = 0;

        /* Iterate through all the chunks in the collective write operation,
         * updating each chunk with the data modifications from other processes,
         * then re-filtering the chunk.
         */
        for (i = 0; i < chunk_list_num_entries; i++)
            if (mpi_rank == chunk_list[i].owners.new_owner)
                if (H5D__filtered_collective_chunk_entry_io(&chunk_list[i], io_info, type_info, fm) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "couldn't process chunk entry")

        /* Gather the new chunk sizes to all processes for a collective reallocation
         * of the chunks in the file.
         */
        if (H5D__mpio_array_gatherv(chunk_list, chunk_list_num_entries,
                                    sizeof(H5D_filtered_collective_io_info_t),
                                    (void **)&collective_chunk_list, &collective_chunk_list_num_entries, true,
                                    0, io_info->comm, NULL) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGATHER, FAIL, "couldn't gather new chunk sizes")

        /* Collectively re-allocate the modified chunks (from each process) in the file */
        for (i = 0; i < collective_chunk_list_num_entries; i++) {
            hbool_t insert;

            if (H5D__chunk_file_alloc(&index_info, &collective_chunk_list[i].chunk_states.chunk_current,
                                      &collective_chunk_list[i].chunk_states.new_chunk, &insert,
                                      collective_chunk_list[i].scaled) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "unable to allocate chunk")
        } /* end for */

        if (NULL == (num_chunks_selected_array = (size_t *)H5MM_malloc((size_t)mpi_size * sizeof(size_t))))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate num chunks selected array")

        if (MPI_SUCCESS !=
            (mpi_code = MPI_Allgather(&chunk_list_num_entries, 1, MPI_UNSIGNED_LONG_LONG,
                                      num_chunks_selected_array, 1, MPI_UNSIGNED_LONG_LONG, io_info->comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Allgather failed", mpi_code)

        /* If this process has any chunks selected, create a MPI type for collectively
         * writing out the chunks to file. Otherwise, the process contributes to the
         * collective write with a none type.
         */
        if (chunk_list_num_entries) {
            size_t offset;

            /* During the collective re-allocation of chunks in the file, the record for each
             * chunk is only updated in the collective array, not in the local copy of chunks on each
             * process. However, each process needs the updated chunk records so that they can create
             * a MPI type for the collective write that will write to the chunk's possible new locations
             * in the file instead of the old ones. This ugly hack seems to be the best solution to
             * copy the information back to the local array and avoid having to modify the collective
             * write type function in an ugly way so that it will accept the collective array instead
             * of the local array. This works correctly because the array gather function guarantees
             * that the chunk data in the collective array is ordered in blocks by rank.
             */
            for (i = 0, offset = 0; i < (size_t)mpi_rank; i++)
                offset += num_chunks_selected_array[i];

            H5MM_memcpy(chunk_list, &collective_chunk_list[offset],
                        num_chunks_selected_array[mpi_rank] * sizeof(H5D_filtered_collective_io_info_t));

            /* Create single MPI type encompassing each selection in the dataspace */
            if (H5D__mpio_filtered_collective_write_type(chunk_list, chunk_list_num_entries, &mem_type,
                                                         &mem_type_is_derived, &file_type,
                                                         &file_type_is_derived) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_BADTYPE, FAIL, "couldn't create MPI link chunk I/O type")

            /* Override the write buffer to point to the address of the first
             * chunk data buffer
             */
            io_info->u.wbuf = chunk_list[0].buf;
        } /* end if */

        /* We have a single, complicated MPI datatype for both memory & file */
        mpi_buf_count = (mem_type_is_derived && file_type_is_derived) ? (hsize_t)1 : (hsize_t)0;

        /* Set up the base storage address for this operation */
        ctg_store.contig.dset_addr = 0; /* Write address must be set to address 0 */
        io_info->store             = &ctg_store;

        /* Perform I/O */
        if (H5D__final_collective_io(io_info, type_info, mpi_buf_count, file_type, mem_type) < 0)
            HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish MPI-IO")

        /* Participate in the collective re-insertion of all chunks modified
         * in this iteration into the chunk index
         */
        for (i = 0; i < collective_chunk_list_num_entries; i++) {
            udata.chunk_block   = collective_chunk_list[i].chunk_states.new_chunk;
            udata.common.scaled = collective_chunk_list[i].scaled;
            udata.chunk_idx     = collective_chunk_list[i].index;

            if ((index_info.storage->ops->insert)(&index_info, &udata, io_info->dset) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, FAIL, "unable to insert chunk address into index")
        } /* end for */
    }     /* end if */

done:
    /* Free resources used by a process which had some selection */
    if (chunk_list) {
        for (i = 0; i < chunk_list_num_entries; i++)
            if (chunk_list[i].buf)
                H5MM_free(chunk_list[i].buf);

        H5MM_free(chunk_list);
    } /* end if */

    if (num_chunks_selected_array)
        H5MM_free(num_chunks_selected_array);
    if (collective_chunk_list)
        H5MM_free(collective_chunk_list);

    /* Free the MPI buf and file types, if they were derived */
    if (mem_type_is_derived && MPI_SUCCESS != (mpi_code = MPI_Type_free(&mem_type)))
        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)
    if (file_type_is_derived && MPI_SUCCESS != (mpi_code = MPI_Type_free(&file_type)))
        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__link_chunk_filtered_collective_io() */

/*-------------------------------------------------------------------------
 * Function:    H5D__multi_chunk_collective_io
 *
 * Purpose:     To do IO per chunk according to IO mode(collective/independent/none)
 *
 *              1. Use MPI_gather and MPI_Bcast to obtain IO mode in each chunk(collective/independent/none)
 *              2. Depending on whether the IO mode is collective or independent or none,
 *                 Create either MPI derived datatype for each chunk or just do independent IO
 *              3. Use common collective IO routine to do MPI-IO
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__multi_chunk_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info, H5D_chunk_map_t *fm)
{
    H5D_io_info_t              ctg_io_info; /* Contiguous I/O info object */
    H5D_storage_t              ctg_store;   /* Chunk storage information as contiguous dataset */
    H5D_io_info_t              cpt_io_info; /* Compact I/O info object */
    H5D_storage_t              cpt_store;   /* Chunk storage information as compact dataset */
    hbool_t                    cpt_dirty;   /* Temporary placeholder for compact storage "dirty" flag */
    uint8_t *                  chunk_io_option = NULL;
    haddr_t *                  chunk_addr      = NULL;
    H5D_storage_t              store; /* union of EFL and chunk pointer in file space */
    H5FD_mpio_collective_opt_t last_coll_opt_mode =
        H5FD_MPIO_COLLECTIVE_IO; /* Last parallel transfer with independent IO or collective IO with this mode
                                  */
    size_t total_chunk;          /* Total # of chunks in dataset */
#ifdef H5Dmpio_DEBUG
    int mpi_rank;
#endif
    size_t                    u; /* Local index variable */
    H5D_mpio_actual_io_mode_t actual_io_mode =
        H5D_MPIO_NO_COLLECTIVE; /* Local variable for tracking the I/O mode used. */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /* Set the actual chunk opt mode property */
    H5CX_set_mpio_actual_chunk_opt(H5D_MPIO_MULTI_CHUNK);

#ifdef H5Dmpio_DEBUG
    mpi_rank = H5F_mpi_get_rank(io_info->dset->oloc.file);
#endif

    /* Retrieve total # of chunks in dataset */
    H5_CHECKED_ASSIGN(total_chunk, size_t, fm->layout->u.chunk.nchunks, hsize_t);
    HDassert(total_chunk != 0);

    /* Allocate memories */
    chunk_io_option = (uint8_t *)H5MM_calloc(total_chunk);
    chunk_addr      = (haddr_t *)H5MM_calloc(total_chunk * sizeof(haddr_t));
#ifdef H5D_DEBUG
    if (H5DEBUG(D))
        HDfprintf(H5DEBUG(D), "total_chunk %zu\n", total_chunk);
#endif

    /* Obtain IO option for each chunk */
    if (H5D__obtain_mpio_mode(io_info, fm, chunk_io_option, chunk_addr) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTRECV, FAIL, "unable to obtain MPIO mode")

    /* Set up contiguous I/O info object */
    H5MM_memcpy(&ctg_io_info, io_info, sizeof(ctg_io_info));
    ctg_io_info.store      = &ctg_store;
    ctg_io_info.layout_ops = *H5D_LOPS_CONTIG;

    /* Initialize temporary contiguous storage info */
    ctg_store.contig.dset_size = (hsize_t)io_info->dset->shared->layout.u.chunk.size;

    /* Set up compact I/O info object */
    H5MM_memcpy(&cpt_io_info, io_info, sizeof(cpt_io_info));
    cpt_io_info.store      = &cpt_store;
    cpt_io_info.layout_ops = *H5D_LOPS_COMPACT;

    /* Initialize temporary compact storage info */
    cpt_store.compact.dirty = &cpt_dirty;

    /* Set dataset storage for I/O info */
    io_info->store = &store;

    /* Loop over _all_ the chunks */
    for (u = 0; u < total_chunk; u++) {
        H5D_chunk_info_t *chunk_info; /* Chunk info for current chunk */
        H5S_t *           fspace;     /* Dataspace describing chunk & selection in it */
        H5S_t *           mspace; /* Dataspace describing selection in memory corresponding to this chunk */

#ifdef H5D_DEBUG
        if (H5DEBUG(D))
            HDfprintf(H5DEBUG(D), "mpi_rank = %d, chunk index = %zu\n", mpi_rank, u);
#endif
        /* Get the chunk info for this chunk, if there are elements selected */
        chunk_info = fm->select_chunk[u];

        /* Set the storage information for chunks with selections */
        if (chunk_info) {
            HDassert(chunk_info->index == u);

            /* Pass in chunk's coordinates in a union. */
            store.chunk.scaled = chunk_info->scaled;
        } /* end if */

        /* Collective IO for this chunk,
         * Note: even there is no selection for this process, the process still
         *      needs to contribute MPI NONE TYPE.
         */
        if (chunk_io_option[u] == H5D_CHUNK_IO_MODE_COL) {
#ifdef H5D_DEBUG
            if (H5DEBUG(D))
                HDfprintf(H5DEBUG(D), "inside collective chunk IO mpi_rank = %d, chunk index = %zu\n",
                          mpi_rank, u);
#endif

            /* Set the file & memory dataspaces */
            if (chunk_info) {
                fspace = chunk_info->fspace;
                mspace = chunk_info->mspace;

                /* Update the local variable tracking the actual io mode property.
                 *
                 * Note: H5D_MPIO_COLLECTIVE_MULTI | H5D_MPIO_INDEPENDENT = H5D_MPIO_MIXED
                 *      to ease switching between to mixed I/O without checking the current
                 *      value of the property. You can see the definition in H5Ppublic.h
                 */
                actual_io_mode = (H5D_mpio_actual_io_mode_t)(actual_io_mode | H5D_MPIO_CHUNK_COLLECTIVE);

            } /* end if */
            else {
                fspace = mspace = NULL;
            } /* end else */

            /* Switch back to collective I/O */
            if (last_coll_opt_mode != H5FD_MPIO_COLLECTIVE_IO) {
                if (H5CX_set_mpio_coll_opt(H5FD_MPIO_COLLECTIVE_IO) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't switch to collective I/O")
                last_coll_opt_mode = H5FD_MPIO_COLLECTIVE_IO;
            } /* end if */

            /* Initialize temporary contiguous storage address */
            ctg_store.contig.dset_addr = chunk_addr[u];

            /* Perform the I/O */
            if (H5D__inter_collective_io(&ctg_io_info, type_info, fspace, mspace) < 0)
                HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish shared collective MPI-IO")
        }      /* end if */
        else { /* possible independent IO for this chunk */
#ifdef H5D_DEBUG
            if (H5DEBUG(D))
                HDfprintf(H5DEBUG(D), "inside independent IO mpi_rank = %d, chunk index = %zu\n", mpi_rank,
                          u);
#endif

            HDassert(chunk_io_option[u] == 0);

            /* Set the file & memory dataspaces */
            if (chunk_info) {
                fspace = chunk_info->fspace;
                mspace = chunk_info->mspace;

                /* Update the local variable tracking the actual io mode. */
                actual_io_mode = (H5D_mpio_actual_io_mode_t)(actual_io_mode | H5D_MPIO_CHUNK_INDEPENDENT);
            } /* end if */
            else {
                fspace = mspace = NULL;
            } /* end else */

            /* Using independent I/O with file setview.*/
            if (last_coll_opt_mode != H5FD_MPIO_INDIVIDUAL_IO) {
                if (H5CX_set_mpio_coll_opt(H5FD_MPIO_INDIVIDUAL_IO) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't switch to individual I/O")
                last_coll_opt_mode = H5FD_MPIO_INDIVIDUAL_IO;
            } /* end if */

            /* Initialize temporary contiguous storage address */
            ctg_store.contig.dset_addr = chunk_addr[u];

            /* Perform the I/O */
            if (H5D__inter_collective_io(&ctg_io_info, type_info, fspace, mspace) < 0)
                HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish shared collective MPI-IO")
#ifdef H5D_DEBUG
            if (H5DEBUG(D))
                HDfprintf(H5DEBUG(D), "after inter collective IO\n");
#endif
        } /* end else */
    }     /* end for */

    /* Write the local value of actual io mode to the API context. */
    H5CX_set_mpio_actual_io_mode(actual_io_mode);

done:
    if (chunk_io_option)
        H5MM_xfree(chunk_io_option);
    if (chunk_addr)
        H5MM_xfree(chunk_addr);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__multi_chunk_collective_io */

/*-------------------------------------------------------------------------
 * Function:    H5D__multi_chunk_filtered_collective_io
 *
 * Purpose:     To do filtered collective IO iteratively to save on memory.
 *              While link_chunk_filtered_collective_io will construct and
 *              work on a list of all of the chunks selected in the IO
 *              operation at once, this function works iteratively on a set
 *              of chunks at a time; at most one chunk per rank per
 *              iteration.
 *
 *              1. Construct a list of selected chunks in the collective IO
 *                 operation
 *                 A. If any chunk is being written to by more than 1
 *                    process, the process writing to the chunk which
 *                    currently has the least amount of chunks assigned
 *                    to it becomes the new owner (in the case of ties,
 *                    the lowest MPI rank becomes the new owner)
 *              2. If the operation is a read operation
 *                 A. Loop through each chunk in the operation
 *                    I. Read the chunk from the file
 *                    II. Unfilter the chunk
 *                    III. Scatter the read chunk data to the user's buffer
 *              3. If the operation is a write operation
 *                 A. Loop through each chunk in the operation
 *                    I. If this is not a full overwrite of the chunk
 *                       a) Read the chunk from file and pass the chunk
 *                          through the filter pipeline in reverse order
 *                          (Unfilter the chunk)
 *                    II. Update the chunk data with the modifications from
 *                        the owning process
 *                    III. Receive any modification data from other
 *                         processes and update the chunk data with these
 *                         modifications
 *                    IV. Filter the chunk
 *                    V. Contribute the chunk to an array gathered by
 *                        all processes which contains every chunk
 *                        modified in this iteration (up to one chunk
 *                        per process, some processes may not have a
 *                        selection/may have less chunks to work on than
 *                        other processes)
 *                    VI. All processes collectively re-allocate each
 *                        chunk from the gathered array with their new
 *                        sizes after the filter operation
 *                    VII. Proceed with the collective write operation
 *                        for the chunks modified on this iteration
 *                    VIII. All processes collectively re-insert each
 *                       chunk from the gathered array into the chunk
 *                       index
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Jordan Henderson
 *              Friday, Dec. 2nd, 2016
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__multi_chunk_filtered_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
                                        H5D_chunk_map_t *fm)
{
    H5D_filtered_collective_io_info_t *chunk_list = NULL; /* The list of chunks being read/written */
    H5D_filtered_collective_io_info_t *collective_chunk_list =
        NULL;                  /* The list of chunks used during collective operations */
    H5D_storage_t store;       /* union of EFL and chunk pointer in file space */
    H5D_io_info_t ctg_io_info; /* Contiguous I/O info object */
    H5D_storage_t ctg_store;   /* Chunk storage information as contiguous dataset */
    MPI_Datatype *file_type_array            = NULL;
    MPI_Datatype *mem_type_array             = NULL;
    hbool_t *     file_type_is_derived_array = NULL;
    hbool_t *     mem_type_is_derived_array  = NULL;
    hbool_t *     has_chunk_selected_array =
        NULL; /* Array of whether or not each process is contributing a chunk to each iteration */
    size_t chunk_list_num_entries;
    size_t collective_chunk_list_num_entries;
    size_t i, j; /* Local index variable */
    int    mpi_rank, mpi_size, mpi_code;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    HDassert(io_info);
    HDassert(type_info);
    HDassert(fm);

    /* Obtain the current rank of the process and the number of processes */
    if ((mpi_rank = H5F_mpi_get_rank(io_info->dset->oloc.file)) < 0)
        HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi rank")
    if ((mpi_size = H5F_mpi_get_size(io_info->dset->oloc.file)) < 0)
        HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi size")

    /* Set the actual chunk opt mode property */
    H5CX_set_mpio_actual_chunk_opt(H5D_MPIO_MULTI_CHUNK);

    /* Set the actual_io_mode property.
     * Multi chunk I/O does not break to independent, so can set right away
     */
    H5CX_set_mpio_actual_io_mode(H5D_MPIO_CHUNK_COLLECTIVE);

    /* Build a list of selected chunks in the collective IO operation */
    if (H5D__construct_filtered_io_info_list(io_info, type_info, fm, &chunk_list, &chunk_list_num_entries) <
        0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "couldn't construct filtered I/O info list")

    /* Set up contiguous I/O info object */
    H5MM_memcpy(&ctg_io_info, io_info, sizeof(ctg_io_info));
    ctg_io_info.store      = &ctg_store;
    ctg_io_info.layout_ops = *H5D_LOPS_CONTIG;

    /* Initialize temporary contiguous storage info */
    ctg_store.contig.dset_size = (hsize_t)io_info->dset->shared->layout.u.chunk.size;
    ctg_store.contig.dset_addr = 0;

    /* Set dataset storage for I/O info */
    io_info->store = &store;

    if (io_info->op_type == H5D_IO_OP_READ) { /* Filtered collective read */
        for (i = 0; i < chunk_list_num_entries; i++)
            if (H5D__filtered_collective_chunk_entry_io(&chunk_list[i], io_info, type_info, fm) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "couldn't process chunk entry")
    }      /* end if */
    else { /* Filtered collective write */
        H5D_chk_idx_info_t index_info;
        H5D_chunk_ud_t     udata;
        size_t             max_num_chunks;
        hsize_t            mpi_buf_count;

        /* Construct chunked index info */
        index_info.f       = io_info->dset->oloc.file;
        index_info.pline   = &(io_info->dset->shared->dcpl_cache.pline);
        index_info.layout  = &(io_info->dset->shared->layout.u.chunk);
        index_info.storage = &(io_info->dset->shared->layout.storage.u.chunk);

        /* Set up chunk information for insertion to chunk index */
        udata.common.layout  = index_info.layout;
        udata.common.storage = index_info.storage;
        udata.filter_mask    = 0;

        /* Retrieve the maximum number of chunks being written among all processes */
        if (MPI_SUCCESS != (mpi_code = MPI_Allreduce(&chunk_list_num_entries, &max_num_chunks, 1,
                                                     MPI_UNSIGNED_LONG_LONG, MPI_MAX, io_info->comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Allreduce failed", mpi_code)

        /* If no one is writing anything at all, end the operation */
        if (!(max_num_chunks > 0))
            HGOTO_DONE(SUCCEED);

        /* Allocate arrays for storing MPI file and mem types and whether or not the
         * types were derived.
         */
        if (NULL == (file_type_array = (MPI_Datatype *)H5MM_malloc(max_num_chunks * sizeof(MPI_Datatype))))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate file type array")

        if (NULL == (file_type_is_derived_array = (hbool_t *)H5MM_calloc(max_num_chunks * sizeof(hbool_t))))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate file type is derived array")

        if (NULL == (mem_type_array = (MPI_Datatype *)H5MM_malloc(max_num_chunks * sizeof(MPI_Datatype))))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate mem type array")

        if (NULL == (mem_type_is_derived_array = (hbool_t *)H5MM_calloc(max_num_chunks * sizeof(hbool_t))))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate mem type is derived array")

        /* Iterate over the max number of chunks among all processes, as this process could
         * have no chunks left to work on, but it still needs to participate in the collective
         * re-allocation and re-insertion of chunks modified by other processes.
         */
        for (i = 0; i < max_num_chunks; i++) {
            /* Check if this process has a chunk to work on for this iteration */
            hbool_t have_chunk_to_process =
                (i < chunk_list_num_entries) && (mpi_rank == chunk_list[i].owners.new_owner);

            if (have_chunk_to_process)
                if (H5D__filtered_collective_chunk_entry_io(&chunk_list[i], io_info, type_info, fm) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "couldn't process chunk entry")

            /* Gather the new chunk sizes to all processes for a collective re-allocation
             * of the chunks in the file
             */
            if (H5D__mpio_array_gatherv(&chunk_list[i], have_chunk_to_process ? 1 : 0,
                                        sizeof(H5D_filtered_collective_io_info_t),
                                        (void **)&collective_chunk_list, &collective_chunk_list_num_entries,
                                        true, 0, io_info->comm, NULL) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGATHER, FAIL, "couldn't gather new chunk sizes")

            /* Participate in the collective re-allocation of all chunks modified
             * in this iteration.
             */
            for (j = 0; j < collective_chunk_list_num_entries; j++) {
                hbool_t insert = FALSE;

                if (H5D__chunk_file_alloc(&index_info, &collective_chunk_list[j].chunk_states.chunk_current,
                                          &collective_chunk_list[j].chunk_states.new_chunk, &insert,
                                          chunk_list[j].scaled) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "unable to allocate chunk")
            } /* end for */

            if (NULL ==
                (has_chunk_selected_array = (hbool_t *)H5MM_malloc((size_t)mpi_size * sizeof(hbool_t))))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate num chunks selected array")

            if (MPI_SUCCESS !=
                (mpi_code = MPI_Allgather(&have_chunk_to_process, 1, MPI_C_BOOL, has_chunk_selected_array, 1,
                                          MPI_C_BOOL, io_info->comm)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Allgather failed", mpi_code)

            /* If this process has a chunk to work on, create a MPI type for the
             * memory and file for writing out the chunk
             */
            if (have_chunk_to_process) {
                size_t offset;
                int    mpi_type_count;

                for (j = 0, offset = 0; j < (size_t)mpi_rank; j++)
                    offset += has_chunk_selected_array[j];

                /* Collect the new chunk info back to the local copy, since only the record in the
                 * collective array gets updated by the chunk re-allocation */
                H5MM_memcpy(&chunk_list[i].chunk_states.new_chunk,
                            &collective_chunk_list[offset].chunk_states.new_chunk,
                            sizeof(chunk_list[i].chunk_states.new_chunk));

                H5_CHECKED_ASSIGN(mpi_type_count, int, chunk_list[i].chunk_states.new_chunk.length, hsize_t);

                /* Create MPI memory type for writing to chunk */
                if (MPI_SUCCESS !=
                    (mpi_code = MPI_Type_contiguous(mpi_type_count, MPI_BYTE, &mem_type_array[i])))
                    HMPI_GOTO_ERROR(FAIL, "MPI_Type_contiguous failed", mpi_code)
                if (MPI_SUCCESS != (mpi_code = MPI_Type_commit(&mem_type_array[i])))
                    HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)
                mem_type_is_derived_array[i] = TRUE;

                /* Create MPI file type for writing to chunk */
                if (MPI_SUCCESS !=
                    (mpi_code = MPI_Type_contiguous(mpi_type_count, MPI_BYTE, &file_type_array[i])))
                    HMPI_GOTO_ERROR(FAIL, "MPI_Type_contiguous failed", mpi_code)
                if (MPI_SUCCESS != (mpi_code = MPI_Type_commit(&file_type_array[i])))
                    HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)
                file_type_is_derived_array[i] = TRUE;

                mpi_buf_count = 1;

                /* Set up the base storage address for this operation */
                ctg_store.contig.dset_addr = chunk_list[i].chunk_states.new_chunk.offset;

                /* Override the write buffer to point to the address of the
                 * chunk data buffer
                 */
                ctg_io_info.u.wbuf = chunk_list[i].buf;
            } /* end if */
            else {
                mem_type_array[i] = file_type_array[i] = MPI_BYTE;
                mpi_buf_count                          = 0;
            } /* end else */

            /* Perform the I/O */
            if (H5D__final_collective_io(&ctg_io_info, type_info, mpi_buf_count, file_type_array[i],
                                         mem_type_array[i]) < 0)
                HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish MPI-IO")

            /* Participate in the collective re-insertion of all chunks modified
             * in this iteration into the chunk index
             */
            for (j = 0; j < collective_chunk_list_num_entries; j++) {
                udata.chunk_block   = collective_chunk_list[j].chunk_states.new_chunk;
                udata.common.scaled = collective_chunk_list[j].scaled;
                udata.chunk_idx     = collective_chunk_list[j].index;

                if ((index_info.storage->ops->insert)(&index_info, &udata, io_info->dset) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINSERT, FAIL,
                                "unable to insert chunk address into index")
            } /* end for */

            if (collective_chunk_list) {
                H5MM_free(collective_chunk_list);
                collective_chunk_list = NULL;
            } /* end if */
            if (has_chunk_selected_array) {
                H5MM_free(has_chunk_selected_array);
                has_chunk_selected_array = NULL;
            } /* end if */
        }     /* end for */

        /* Free the MPI file and memory types, if they were derived */
        for (i = 0; i < max_num_chunks; i++) {
            if (file_type_is_derived_array[i])
                if (MPI_SUCCESS != (mpi_code = MPI_Type_free(&file_type_array[i])))
                    HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)

            if (mem_type_is_derived_array[i])
                if (MPI_SUCCESS != (mpi_code = MPI_Type_free(&mem_type_array[i])))
                    HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)
        } /* end for */
    }     /* end else */

done:
    if (chunk_list) {
        for (i = 0; i < chunk_list_num_entries; i++)
            if (chunk_list[i].buf)
                H5MM_free(chunk_list[i].buf);

        H5MM_free(chunk_list);
    } /* end if */

    if (collective_chunk_list)
        H5MM_free(collective_chunk_list);
    if (file_type_array)
        H5MM_free(file_type_array);
    if (mem_type_array)
        H5MM_free(mem_type_array);
    if (file_type_is_derived_array)
        H5MM_free(file_type_is_derived_array);
    if (mem_type_is_derived_array)
        H5MM_free(mem_type_is_derived_array);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__multi_chunk_filtered_collective_io() */

/*-------------------------------------------------------------------------
 * Function:    H5D__inter_collective_io
 *
 * Purpose:     Routine for the shared part of collective IO between multiple chunk
 *              collective IO and contiguous collective IO
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__inter_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info, const H5S_t *file_space,
                         const H5S_t *mem_space)
{
    int          mpi_buf_count; /* # of MPI types */
    hbool_t      mbt_is_derived = FALSE;
    hbool_t      mft_is_derived = FALSE;
    MPI_Datatype mpi_file_type, mpi_buf_type;
    int          mpi_code;            /* MPI return code */
    herr_t       ret_value = SUCCEED; /* return value */

    FUNC_ENTER_STATIC

    if ((file_space != NULL) && (mem_space != NULL)) {
        int      mpi_file_count;     /* Number of file "objects" to transfer */
        hsize_t *permute_map = NULL; /* array that holds the mapping from the old,
                                        out-of-order displacements to the in-order
                                        displacements of the MPI datatypes of the
                                        point selection of the file space */
        hbool_t is_permuted = FALSE;

        /* Obtain disk and memory MPI derived datatype */
        /* NOTE: The permute_map array can be allocated within H5S_mpio_space_type
         *              and will be fed into the next call to H5S_mpio_space_type
         *              where it will be freed.
         */
        if (H5S_mpio_space_type(file_space, type_info->src_type_size, &mpi_file_type, &mpi_file_count,
                                &mft_is_derived, /* OUT: datatype created */
                                TRUE,            /* this is a file space, so
                                                    permute the datatype if the
                                                    point selection is out of
                                                    order */
                                &permute_map,    /* OUT: a map to indicate
                                                    the permutation of
                                                    points selected in
                                                    case they are out of
                                                    order */
                                &is_permuted /* OUT */) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "couldn't create MPI file type")
        /* Sanity check */
        if (is_permuted)
            HDassert(permute_map);
        if (H5S_mpio_space_type(mem_space, type_info->src_type_size, &mpi_buf_type, &mpi_buf_count,
                                &mbt_is_derived, /* OUT: datatype created */
                                FALSE,           /* this is a memory space, so if
                                                    the file space is not
                                                    permuted, there is no need to
                                                    permute the datatype if the
                                                    point selections are out of
                                                    order*/
                                &permute_map     /* IN: the permutation map
                                                    generated by the
                                                    file_space selection
                                                    and applied to the
                                                    memory selection */
                                ,
                                &is_permuted /* IN */) < 0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL, "couldn't create MPI buffer type")
        /* Sanity check */
        if (is_permuted)
            HDassert(!permute_map);
    } /* end if */
    else {
        /* For non-selection, participate with a none MPI derived datatype, the count is 0.  */
        mpi_buf_type   = MPI_BYTE;
        mpi_file_type  = MPI_BYTE;
        mpi_buf_count  = 0;
        mbt_is_derived = FALSE;
        mft_is_derived = FALSE;
    } /* end else */

#ifdef H5D_DEBUG
    if (H5DEBUG(D))
        HDfprintf(H5DEBUG(D), "before final collective IO \n");
#endif

    /* Perform final collective I/O operation */
    if (H5D__final_collective_io(io_info, type_info, (hsize_t)mpi_buf_count, mpi_file_type, mpi_buf_type) < 0)
        HGOTO_ERROR(H5E_IO, H5E_CANTGET, FAIL, "couldn't finish collective MPI-IO")

done:
    /* Free the MPI buf and file types, if they were derived */
    if (mbt_is_derived && MPI_SUCCESS != (mpi_code = MPI_Type_free(&mpi_buf_type)))
        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)
    if (mft_is_derived && MPI_SUCCESS != (mpi_code = MPI_Type_free(&mpi_file_type)))
        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)

#ifdef H5D_DEBUG
    if (H5DEBUG(D))
        HDfprintf(H5DEBUG(D), "before leaving inter_collective_io ret_value = %d\n", ret_value);
#endif

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__inter_collective_io() */

/*-------------------------------------------------------------------------
 * Function:    H5D__final_collective_io
 *
 * Purpose:     Routine for the common part of collective IO with different storages.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__final_collective_io(H5D_io_info_t *io_info, const H5D_type_info_t *type_info, hsize_t mpi_buf_count,
                         MPI_Datatype mpi_file_type, MPI_Datatype mpi_buf_type)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /* Pass buf type, file type to the file driver.  */
    if (H5CX_set_mpi_coll_datatypes(mpi_buf_type, mpi_file_type) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set MPI-I/O collective I/O datatypes")

    if (io_info->op_type == H5D_IO_OP_WRITE) {
        if ((io_info->io_ops.single_write)(io_info, type_info, mpi_buf_count, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "optimized write failed")
    } /* end if */
    else {
        if ((io_info->io_ops.single_read)(io_info, type_info, mpi_buf_count, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "optimized read failed")
    } /* end else */

done:
#ifdef H5D_DEBUG
    if (H5DEBUG(D))
        HDfprintf(H5DEBUG(D), "ret_value before leaving final_collective_io=%d\n", ret_value);
#endif
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__final_collective_io */

/*-------------------------------------------------------------------------
 * Function:    H5D__cmp_chunk_addr
 *
 * Purpose:     Routine to compare chunk addresses
 *
 * Description: Callback for qsort() to compare chunk addresses
 *
 * Return:      -1, 0, 1
 *
 * Programmer:  Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static int
H5D__cmp_chunk_addr(const void *chunk_addr_info1, const void *chunk_addr_info2)
{
    haddr_t addr1 = HADDR_UNDEF, addr2 = HADDR_UNDEF;

    FUNC_ENTER_STATIC_NOERR

    addr1 = ((const H5D_chunk_addr_info_t *)chunk_addr_info1)->chunk_addr;
    addr2 = ((const H5D_chunk_addr_info_t *)chunk_addr_info2)->chunk_addr;

    FUNC_LEAVE_NOAPI(H5F_addr_cmp(addr1, addr2))
} /* end H5D__cmp_chunk_addr() */

/*-------------------------------------------------------------------------
 * Function:    H5D__cmp_filtered_collective_io_info_entry
 *
 * Purpose:     Routine to compare filtered collective chunk io info
 *              entries
 *
 * Description: Callback for qsort() to compare filtered collective chunk
 *              io info entries
 *
 * Return:      -1, 0, 1
 *
 * Programmer:  Jordan Henderson
 *              Wednesday, Nov. 30th, 2016
 *
 *-------------------------------------------------------------------------
 */
static int
H5D__cmp_filtered_collective_io_info_entry(const void *filtered_collective_io_info_entry1,
                                           const void *filtered_collective_io_info_entry2)
{
    haddr_t addr1 = HADDR_UNDEF, addr2 = HADDR_UNDEF;

    FUNC_ENTER_STATIC_NOERR

    addr1 = ((const H5D_filtered_collective_io_info_t *)filtered_collective_io_info_entry1)
                ->chunk_states.new_chunk.offset;
    addr2 = ((const H5D_filtered_collective_io_info_t *)filtered_collective_io_info_entry2)
                ->chunk_states.new_chunk.offset;

    FUNC_LEAVE_NOAPI(H5F_addr_cmp(addr1, addr2))
} /* end H5D__cmp_filtered_collective_io_info_entry() */

#if MPI_VERSION >= 3

/*-------------------------------------------------------------------------
 * Function:    H5D__cmp_filtered_collective_io_info_entry_owner
 *
 * Purpose:     Routine to compare filtered collective chunk io info
 *              entries's original owner fields
 *
 * Description: Callback for qsort() to compare filtered collective chunk
 *              io info entries's original owner fields
 *
 * Return:      The difference between the two
 *              H5D_filtered_collective_io_info_t's original owner fields
 *
 * Programmer:  Jordan Henderson
 *              Monday, Apr. 10th, 2017
 *
 *-------------------------------------------------------------------------
 */
static int
H5D__cmp_filtered_collective_io_info_entry_owner(const void *filtered_collective_io_info_entry1,
                                                 const void *filtered_collective_io_info_entry2)
{
    int owner1 = -1, owner2 = -1;

    FUNC_ENTER_STATIC_NOERR

    owner1 = ((const H5D_filtered_collective_io_info_t *)filtered_collective_io_info_entry1)
                 ->owners.original_owner;
    owner2 = ((const H5D_filtered_collective_io_info_t *)filtered_collective_io_info_entry2)
                 ->owners.original_owner;

    FUNC_LEAVE_NOAPI(owner1 - owner2)
} /* end H5D__cmp_filtered_collective_io_info_entry_owner() */
#endif

/*-------------------------------------------------------------------------
 * Function:    H5D__sort_chunk
 *
 * Purpose:     Routine to sort chunks in increasing order of chunk address
 *              Each chunk address is also obtained.
 *
 * Description:
 *              For most cases, the chunk address has already been sorted in increasing order.
 *              The special sorting flag is used to optimize this common case.
 *              quick sort is used for necessary sorting.
 *
 * Parameters:
 *              Input: H5D_io_info_t* io_info,
 *                      H5D_chunk_map_t *fm(global chunk map struct)
 *              Input/Output:  H5D_chunk_addr_info_t chunk_addr_info_array[]   : array to store chunk address
 *and information many_chunk_opt                         : flag to optimize the way to obtain chunk addresses
 *                                                              for many chunks
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__sort_chunk(H5D_io_info_t *io_info, const H5D_chunk_map_t *fm,
                H5D_chunk_addr_info_t chunk_addr_info_array[], int sum_chunk)
{
    H5SL_node_t *     chunk_node;            /* Current node in chunk skip list */
    H5D_chunk_info_t *chunk_info;            /* Current chunking info. of this node. */
    haddr_t           chunk_addr;            /* Current chunking address of this node */
    haddr_t *total_chunk_addr_array = NULL;  /* The array of chunk address for the total number of chunk */
    hbool_t  do_sort                = FALSE; /* Whether the addresses need to be sorted */
    int      bsearch_coll_chunk_threshold;
    int      many_chunk_opt = H5D_OBTAIN_ONE_CHUNK_ADDR_IND;
    int      mpi_size;            /* Number of MPI processes */
    int      mpi_code;            /* MPI return code */
    int      i;                   /* Local index variable */
    herr_t   ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Retrieve # of MPI processes */
    if ((mpi_size = H5F_mpi_get_size(io_info->dset->oloc.file)) < 0)
        HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi size")

    /* Calculate the actual threshold to obtain all chunk addresses collectively
     *  The bigger this number is, the more possible the use of obtaining chunk
     * address collectively.
     */
    /* For non-optimization one-link IO, actual bsearch threshold is always
     *   0, we would always want to obtain the chunk addresses individually
     *   for each process.
     */
    bsearch_coll_chunk_threshold = (sum_chunk * 100) / ((int)fm->layout->u.chunk.nchunks * mpi_size);
    if ((bsearch_coll_chunk_threshold > H5D_ALL_CHUNK_ADDR_THRES_COL) &&
        ((sum_chunk / mpi_size) >= H5D_ALL_CHUNK_ADDR_THRES_COL_NUM))
        many_chunk_opt = H5D_OBTAIN_ALL_CHUNK_ADDR_COL;

#ifdef H5D_DEBUG
    if (H5DEBUG(D))
        HDfprintf(H5DEBUG(D), "many_chunk_opt= %d\n", many_chunk_opt);
#endif

    /* If we need to optimize the way to obtain the chunk address */
    if (many_chunk_opt != H5D_OBTAIN_ONE_CHUNK_ADDR_IND) {
        int mpi_rank;

#ifdef H5D_DEBUG
        if (H5DEBUG(D))
            HDfprintf(H5DEBUG(D), "Coming inside H5D_OBTAIN_ALL_CHUNK_ADDR_COL\n");
#endif
        /* Allocate array for chunk addresses */
        if (NULL == (total_chunk_addr_array =
                         (haddr_t *)H5MM_malloc(sizeof(haddr_t) * (size_t)fm->layout->u.chunk.nchunks)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate memory chunk address array")

        /* Retrieve all the chunk addresses with process 0 */
        if ((mpi_rank = H5F_mpi_get_rank(io_info->dset->oloc.file)) < 0)
            HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi rank")

        if (mpi_rank == 0) {
            if (H5D__chunk_addrmap(io_info, total_chunk_addr_array) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get chunk address")
        } /* end if */

        /* Broadcasting the MPI_IO option info. and chunk address info. */
        if (MPI_SUCCESS != (mpi_code = MPI_Bcast(total_chunk_addr_array,
                                                 (int)(sizeof(haddr_t) * fm->layout->u.chunk.nchunks),
                                                 MPI_BYTE, (int)0, io_info->comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_BCast failed", mpi_code)
    } /* end if */

    /* Start at first node in chunk skip list */
    i = 0;
    if (NULL == (chunk_node = H5SL_first(fm->sel_chunks)))
        HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL, "couldn't get chunk node from skipped list")

    /* Iterate over all chunks for this process */
    while (chunk_node) {
        if (NULL == (chunk_info = (H5D_chunk_info_t *)H5SL_item(chunk_node)))
            HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL, "couldn't get chunk info from skipped list")

        if (many_chunk_opt == H5D_OBTAIN_ONE_CHUNK_ADDR_IND) {
            H5D_chunk_ud_t udata; /* User data for querying chunk info */

            /* Get address of chunk */
            if (H5D__chunk_lookup(io_info->dset, chunk_info->scaled, &udata) < 0)
                HGOTO_ERROR(H5E_STORAGE, H5E_CANTGET, FAIL, "couldn't get chunk info from skipped list")
            chunk_addr = udata.chunk_block.offset;
        } /* end if */
        else
            chunk_addr = total_chunk_addr_array[chunk_info->index];

        /* Check if chunk addresses are not in increasing order in the file */
        if (i > 0 && chunk_addr < chunk_addr_info_array[i - 1].chunk_addr)
            do_sort = TRUE;

        /* Set the address & info for this chunk */
        chunk_addr_info_array[i].chunk_addr = chunk_addr;
        chunk_addr_info_array[i].chunk_info = *chunk_info;

        /* Advance to next chunk in list */
        i++;
        chunk_node = H5SL_next(chunk_node);
    } /* end while */

#ifdef H5D_DEBUG
    if (H5DEBUG(D))
        HDfprintf(H5DEBUG(D), "before Qsort\n");
#endif
    if (do_sort) {
        size_t num_chunks = H5SL_count(fm->sel_chunks);

        HDqsort(chunk_addr_info_array, num_chunks, sizeof(chunk_addr_info_array[0]), H5D__cmp_chunk_addr);
    } /* end if */

done:
    if (total_chunk_addr_array)
        H5MM_xfree(total_chunk_addr_array);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__sort_chunk() */

/*-------------------------------------------------------------------------
 * Function:    H5D__obtain_mpio_mode
 *
 * Purpose:     Routine to obtain each io mode(collective,independent or none) for each chunk;
 *              Each chunk address is also obtained.
 *
 * Description:
 *
 *              1) Each process provides two piece of information for all chunks having selection
 *                 a) chunk index
 *                 b) wheather this chunk is regular(for MPI derived datatype not working case)
 *
 *              2) Gather all the information to the root process
 *
 *              3) Root process will do the following:
 *                 a) Obtain chunk addresses for all chunks in this dataspace
 *                 b) With the consideration of the user option, calculate IO mode for each chunk
 *                 c) Build MPI derived datatype to combine "chunk address" and "assign_io" information
 *                      in order to do MPI Bcast only once
 *                 d) MPI Bcast the IO mode and chunk address information for each chunk.
 *              4) Each process then retrieves IO mode and chunk address information to assign_io_mode and
 *chunk_addr.
 *
 * Parameters:
 *
 *              Input: H5D_io_info_t* io_info,
 *                      H5D_chunk_map_t *fm,(global chunk map struct)
 *              Output: uint8_t assign_io_mode[], : IO mode, collective, independent or none
 *                      haddr_t chunk_addr[],     : chunk address array for each chunk
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Muqun Yang
 *              Monday, Feb. 13th, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__obtain_mpio_mode(H5D_io_info_t *io_info, H5D_chunk_map_t *fm, uint8_t assign_io_mode[],
                      haddr_t chunk_addr[])
{
    size_t            total_chunks;
    unsigned          percent_nproc_per_chunk, threshold_nproc_per_chunk;
    uint8_t *         io_mode_info      = NULL;
    uint8_t *         recv_io_mode_info = NULL;
    uint8_t *         mergebuf          = NULL;
    uint8_t *         tempbuf;
    H5SL_node_t *     chunk_node;
    H5D_chunk_info_t *chunk_info;
    int               mpi_size, mpi_rank;
    MPI_Comm          comm;
    int               root;
    size_t            ic;
    int               mpi_code;
    herr_t            ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /* Assign the rank 0 to the root */
    root = 0;
    comm = io_info->comm;

    /* Obtain the number of process and the current rank of the process */
    if ((mpi_rank = H5F_mpi_get_rank(io_info->dset->oloc.file)) < 0)
        HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi rank")
    if ((mpi_size = H5F_mpi_get_size(io_info->dset->oloc.file)) < 0)
        HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi size")

    /* Setup parameters */
    H5_CHECKED_ASSIGN(total_chunks, size_t, fm->layout->u.chunk.nchunks, hsize_t);
    if (H5CX_get_mpio_chunk_opt_ratio(&percent_nproc_per_chunk) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "couldn't get percent nproc per chunk")
    /* if ratio is 0, perform collective io */
    if (0 == percent_nproc_per_chunk) {
        if (H5D__chunk_addrmap(io_info, chunk_addr) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get chunk address");
        for (ic = 0; ic < total_chunks; ic++)
            assign_io_mode[ic] = H5D_CHUNK_IO_MODE_COL;

        HGOTO_DONE(SUCCEED)
    } /* end if */

    threshold_nproc_per_chunk = (unsigned)mpi_size * percent_nproc_per_chunk / 100;

    /* Allocate memory */
    if (NULL == (io_mode_info = (uint8_t *)H5MM_calloc(total_chunks)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate I/O mode info buffer")
    if (NULL == (mergebuf = (uint8_t *)H5MM_malloc((sizeof(haddr_t) + 1) * total_chunks)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate mergebuf buffer")
    tempbuf = mergebuf + total_chunks;
    if (mpi_rank == root)
        if (NULL == (recv_io_mode_info = (uint8_t *)H5MM_malloc(total_chunks * (size_t)mpi_size)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate recv I/O mode info buffer")

    /* Obtain the regularity and selection information for all chunks in this process. */
    chunk_node = H5SL_first(fm->sel_chunks);
    while (chunk_node) {
        chunk_info = (H5D_chunk_info_t *)H5SL_item(chunk_node);

        io_mode_info[chunk_info->index] = H5D_CHUNK_SELECT_REG; /* this chunk is selected and is "regular" */
        chunk_node                      = H5SL_next(chunk_node);
    } /* end while */

    /* Gather all the information */
    H5_CHECK_OVERFLOW(total_chunks, size_t, int)
    if (MPI_SUCCESS != (mpi_code = MPI_Gather(io_mode_info, (int)total_chunks, MPI_BYTE, recv_io_mode_info,
                                              (int)total_chunks, MPI_BYTE, root, comm)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Gather failed", mpi_code)

    /* Calculate the mode for IO(collective, independent or none) at root process */
    if (mpi_rank == root) {
        size_t    nproc;
        unsigned *nproc_per_chunk;

        /* pre-computing: calculate number of processes and
            regularity of the selection occupied in each chunk */
        if (NULL == (nproc_per_chunk = (unsigned *)H5MM_calloc(total_chunks * sizeof(unsigned))))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate nproc_per_chunk buffer")

        /* calculating the chunk address */
        if (H5D__chunk_addrmap(io_info, chunk_addr) < 0) {
            H5MM_free(nproc_per_chunk);
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get chunk address")
        } /* end if */

        /* checking for number of process per chunk and regularity of the selection*/
        for (nproc = 0; nproc < (size_t)mpi_size; nproc++) {
            uint8_t *tmp_recv_io_mode_info = recv_io_mode_info + (nproc * total_chunks);

            /* Calculate the number of process per chunk and adding irregular selection option */
            for (ic = 0; ic < total_chunks; ic++, tmp_recv_io_mode_info++) {
                if (*tmp_recv_io_mode_info != 0) {
                    nproc_per_chunk[ic]++;
                } /* end if */
            }     /* end for */
        }         /* end for */

        /* Calculating MPIO mode for each chunk (collective, independent, none) */
        for (ic = 0; ic < total_chunks; ic++) {
            if (nproc_per_chunk[ic] > MAX(1, threshold_nproc_per_chunk)) {
                assign_io_mode[ic] = H5D_CHUNK_IO_MODE_COL;
            } /* end if */
        }     /* end for */

        /* merge buffer io_mode info and chunk addr into one */
        H5MM_memcpy(mergebuf, assign_io_mode, total_chunks);
        H5MM_memcpy(tempbuf, chunk_addr, sizeof(haddr_t) * total_chunks);

        H5MM_free(nproc_per_chunk);
    } /* end if */

    /* Broadcasting the MPI_IO option info. and chunk address info. */
    if ((sizeof(haddr_t) + 1) * total_chunks > INT_MAX)
        HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, FAIL, "result overflow")
    if (MPI_SUCCESS !=
        (mpi_code = MPI_Bcast(mergebuf, (int)((sizeof(haddr_t) + 1) * total_chunks), MPI_BYTE, root, comm)))
        HMPI_GOTO_ERROR(FAIL, "MPI_BCast failed", mpi_code)

    H5MM_memcpy(assign_io_mode, mergebuf, total_chunks);
    H5MM_memcpy(chunk_addr, tempbuf, sizeof(haddr_t) * total_chunks);

#ifdef H5_HAVE_INSTRUMENTED_LIBRARY
    {
        hbool_t coll_op = FALSE;

        for (ic = 0; ic < total_chunks; ic++)
            if (assign_io_mode[ic] == H5D_CHUNK_IO_MODE_COL) {
                if (H5CX_test_set_mpio_coll_chunk_multi_ratio_coll(0) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to set property value")
                coll_op = TRUE;
                break;
            } /* end if */

        if (!coll_op)
            if (H5CX_test_set_mpio_coll_chunk_multi_ratio_ind(0) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to set property value")
    }
#endif

done:
    if (io_mode_info)
        H5MM_free(io_mode_info);
    if (mergebuf)
        H5MM_free(mergebuf);
    if (recv_io_mode_info) {
        HDassert(mpi_rank == root);
        H5MM_free(recv_io_mode_info);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__obtain_mpio_mode() */

/*-------------------------------------------------------------------------
 * Function:    H5D__construct_filtered_io_info_list
 *
 * Purpose:     Constructs a list of entries which contain the necessary
 *              information for inter-process communication when performing
 *              collective io on filtered chunks. This list is used by
 *              each process when performing I/O on locally selected chunks
 *              and also in operations that must be collectively done
 *              on every chunk, such as chunk re-allocation, insertion of
 *              chunks into the chunk index, etc.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Jordan Henderson
 *              Tuesday, January 10th, 2017
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__construct_filtered_io_info_list(const H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
                                     const H5D_chunk_map_t *             fm,
                                     H5D_filtered_collective_io_info_t **chunk_list, size_t *num_entries)
{
    H5D_filtered_collective_io_info_t *local_info_array =
        NULL; /* The list of initially selected chunks for this process */
    size_t num_chunks_selected;
    size_t i;
    int    mpi_rank;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    HDassert(io_info);
    HDassert(type_info);
    HDassert(fm);
    HDassert(chunk_list);
    HDassert(num_entries);

    if ((mpi_rank = H5F_mpi_get_rank(io_info->dset->oloc.file)) < 0)
        HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi rank")

    /* Each process builds a local list of the chunks they have selected */
    if ((num_chunks_selected = H5SL_count(fm->sel_chunks))) {
        H5D_chunk_info_t *chunk_info;
        H5D_chunk_ud_t    udata;
        H5SL_node_t *     chunk_node;
        hsize_t           select_npoints;
        hssize_t          chunk_npoints;

        if (NULL == (local_info_array = (H5D_filtered_collective_io_info_t *)H5MM_malloc(
                         num_chunks_selected * sizeof(H5D_filtered_collective_io_info_t))))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate local io info array buffer")

        chunk_node = H5SL_first(fm->sel_chunks);
        for (i = 0; chunk_node; i++) {
            chunk_info = (H5D_chunk_info_t *)H5SL_item(chunk_node);

            /* Obtain this chunk's address */
            if (H5D__chunk_lookup(io_info->dset, chunk_info->scaled, &udata) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "error looking up chunk address")

            local_info_array[i].index                      = chunk_info->index;
            local_info_array[i].chunk_states.chunk_current = local_info_array[i].chunk_states.new_chunk =
                udata.chunk_block;
            local_info_array[i].num_writers           = 0;
            local_info_array[i].owners.original_owner = local_info_array[i].owners.new_owner = mpi_rank;
            local_info_array[i].buf                                                          = NULL;

            local_info_array[i].async_info.num_receive_requests   = 0;
            local_info_array[i].async_info.receive_buffer_array   = NULL;
            local_info_array[i].async_info.receive_requests_array = NULL;

            H5MM_memcpy(local_info_array[i].scaled, chunk_info->scaled, sizeof(chunk_info->scaled));

            select_npoints              = H5S_GET_SELECT_NPOINTS(chunk_info->mspace);
            local_info_array[i].io_size = (size_t)select_npoints * type_info->src_type_size;

            /* Currently the full overwrite status of a chunk is only obtained on a per-process
             * basis. This means that if the total selection in the chunk, as determined by the combination
             * of selections of all of the processes interested in the chunk, covers the entire chunk,
             * the performance optimization of not reading the chunk from the file is still valid, but
             * is not applied in the current implementation. Something like an appropriately placed
             * MPI_Allreduce or a running total of the number of chunk points selected during chunk
             * redistribution should suffice for implementing this case - JTH.
             */
            if ((chunk_npoints = H5S_GET_EXTENT_NPOINTS(chunk_info->fspace)) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTCOUNT, FAIL, "dataspace is invalid")
            local_info_array[i].full_overwrite =
                (local_info_array[i].io_size >= (hsize_t)chunk_npoints * type_info->dst_type_size) ? TRUE
                                                                                                   : FALSE;

            chunk_node = H5SL_next(chunk_node);
        } /* end for */
    }     /* end if */

    /* Redistribute shared chunks to new owners as necessary */
    if (io_info->op_type == H5D_IO_OP_WRITE)
#if MPI_VERSION >= 3
        if (H5D__chunk_redistribute_shared_chunks(io_info, type_info, fm, local_info_array,
                                                  &num_chunks_selected) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "unable to redistribute shared chunks")
#else
        HGOTO_ERROR(
            H5E_DATASET, H5E_WRITEERROR, FAIL,
            "unable to redistribute shared chunks - MPI version < 3 (MPI_Mprobe and MPI_Imrecv missing)")
#endif

    *chunk_list  = local_info_array;
    *num_entries = num_chunks_selected;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__construct_filtered_io_info_list() */

#if MPI_VERSION >= 3

/*-------------------------------------------------------------------------
 * Function:    H5D__chunk_redistribute_shared_chunks
 *
 * Purpose:     When performing a collective write on a Dataset with
 *              filters applied, this function is used to redistribute any
 *              chunks which are selected by more than one process, so as
 *              to preserve file integrity after the write by ensuring
 *              that any shared chunks are only modified by one process.
 *
 *              The current implementation follows this 3-phase process:
 *
 *              - Collect everyone's list of chunks into one large list,
 *                sort the list in increasing order of chunk offset in the
 *                file and hand the list off to rank 0
 *
 *              - Rank 0 scans the list looking for matching runs of chunk
 *                offset in the file (corresponding to a shared chunk which
 *                has been selected by more than one rank in the I/O
 *                operation) and for each shared chunk, it redistributes
 *                the chunk to the process writing to the chunk which
 *                currently has the least amount of chunks assigned to it
 *                by modifying the "new_owner" field in each of the list
 *                entries corresponding to that chunk
 *
 *              - After the chunks have been redistributed, rank 0 re-sorts
 *                the list in order of previous owner so that each rank
 *                will get back exactly the array that they contributed to
 *                the redistribution operation, with the "new_owner" field
 *                of each chunk they are modifying having possibly been
 *                modified. Rank 0 then scatters each segment of the list
 *                back to its corresponding rank
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Jordan Henderson
 *              Monday, May 1, 2017
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__chunk_redistribute_shared_chunks(const H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
                                      const H5D_chunk_map_t *            fm,
                                      H5D_filtered_collective_io_info_t *local_chunk_array,
                                      size_t *                           local_chunk_array_num_entries)
{
    H5D_filtered_collective_io_info_t *shared_chunks_info_array =
        NULL;                        /* The list of all chunks selected in the operation by all processes */
    H5S_sel_iter_t *mem_iter = NULL; /* Memory iterator for H5D__gather_mem */
    unsigned char **mod_data =
        NULL; /* Array of chunk modification data buffers sent by a process to new chunk owners */
    MPI_Request *send_requests = NULL; /* Array of MPI_Isend chunk modification data send requests */
    MPI_Status * send_statuses = NULL; /* Array of MPI_Isend chunk modification send statuses */
    hbool_t      mem_iter_init = FALSE;
    size_t       shared_chunks_info_array_num_entries = 0;
    size_t       num_send_requests                    = 0;
    size_t *     num_assigned_chunks_array            = NULL;
    size_t       i, last_assigned_idx;
    int *        send_counts        = NULL;
    int *        send_displacements = NULL;
    int          scatter_recvcount_int;
    int          mpi_rank, mpi_size, mpi_code;
    herr_t       ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    HDassert(io_info);
    HDassert(type_info);
    HDassert(fm);
    HDassert(local_chunk_array_num_entries);

    if ((mpi_rank = H5F_mpi_get_rank(io_info->dset->oloc.file)) < 0)
        HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi rank")
    if ((mpi_size = H5F_mpi_get_size(io_info->dset->oloc.file)) < 0)
        HGOTO_ERROR(H5E_IO, H5E_MPI, FAIL, "unable to obtain mpi size")

    /* Set to latest format for encoding dataspace */
    H5CX_set_libver_bounds(NULL);

    if (*local_chunk_array_num_entries)
        if (NULL == (send_requests =
                         (MPI_Request *)H5MM_malloc(*local_chunk_array_num_entries * sizeof(MPI_Request))))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate send requests buffer")

    if (NULL == (mem_iter = (H5S_sel_iter_t *)H5MM_malloc(sizeof(H5S_sel_iter_t))))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate memory iterator")

    /* Gather every rank's list of chunks to rank 0 to allow it to perform the redistribution operation. After
     * this call, the gathered list will initially be sorted in increasing order of chunk offset in the file.
     */
    if (H5D__mpio_array_gatherv(local_chunk_array, *local_chunk_array_num_entries,
                                sizeof(H5D_filtered_collective_io_info_t), (void **)&shared_chunks_info_array,
                                &shared_chunks_info_array_num_entries, false, 0, io_info->comm,
                                H5D__cmp_filtered_collective_io_info_entry) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGATHER, FAIL, "couldn't gather array")

    /* Rank 0 redistributes any shared chunks to new owners as necessary */
    if (mpi_rank == 0) {
        if (NULL == (send_counts = (int *)H5MM_calloc((size_t)mpi_size * sizeof(int))))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "unable to allocate send counts buffer")

        if (NULL == (send_displacements = (int *)H5MM_malloc((size_t)mpi_size * sizeof(int))))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "unable to allocate send displacements buffer")

        if (NULL == (num_assigned_chunks_array = (size_t *)H5MM_calloc((size_t)mpi_size * sizeof(size_t))))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL,
                        "unable to allocate number of assigned chunks array")

        for (i = 0; i < shared_chunks_info_array_num_entries;) {
            H5D_filtered_collective_io_info_t chunk_entry;
            haddr_t last_seen_addr  = shared_chunks_info_array[i].chunk_states.chunk_current.offset;
            size_t  set_begin_index = i;
            size_t  num_writers     = 0;
            int     new_chunk_owner = shared_chunks_info_array[i].owners.original_owner;

            /* Process each set of duplicate entries caused by another process writing to the same chunk */
            do {
                chunk_entry = shared_chunks_info_array[i];

                send_counts[chunk_entry.owners.original_owner] += (int)sizeof(chunk_entry);

                /* The new owner of the chunk is determined by the process
                 * writing to the chunk which currently has the least amount
                 * of chunks assigned to it
                 */
                if (num_assigned_chunks_array[chunk_entry.owners.original_owner] <
                    num_assigned_chunks_array[new_chunk_owner])
                    new_chunk_owner = chunk_entry.owners.original_owner;

                num_writers++;
            } while (++i < shared_chunks_info_array_num_entries &&
                     shared_chunks_info_array[i].chunk_states.chunk_current.offset == last_seen_addr);

            /* Set all of the chunk entries' "new_owner" fields */
            for (; set_begin_index < i; set_begin_index++) {
                shared_chunks_info_array[set_begin_index].owners.new_owner = new_chunk_owner;
                shared_chunks_info_array[set_begin_index].num_writers      = num_writers;
            } /* end for */

            num_assigned_chunks_array[new_chunk_owner]++;
        } /* end for */

        /* Sort the new list in order of previous owner so that each original owner of a chunk
         * entry gets that entry back, with the possibly newly-modified "new_owner" field
         */
        if (shared_chunks_info_array_num_entries > 1)
            HDqsort(shared_chunks_info_array, shared_chunks_info_array_num_entries,
                    sizeof(H5D_filtered_collective_io_info_t),
                    H5D__cmp_filtered_collective_io_info_entry_owner);

        send_displacements[0] = 0;
        for (i = 1; i < (size_t)mpi_size; i++)
            send_displacements[i] = send_displacements[i - 1] + send_counts[i - 1];
    } /* end if */

    /* Scatter the segments of the list back to each process */
    H5_CHECKED_ASSIGN(scatter_recvcount_int, int,
                      *local_chunk_array_num_entries * sizeof(H5D_filtered_collective_io_info_t), size_t);
    if (MPI_SUCCESS !=
        (mpi_code = MPI_Scatterv(shared_chunks_info_array, send_counts, send_displacements, MPI_BYTE,
                                 local_chunk_array, scatter_recvcount_int, MPI_BYTE, 0, io_info->comm)))
        HMPI_GOTO_ERROR(FAIL, "unable to scatter shared chunks info buffer", mpi_code)

    if (shared_chunks_info_array) {
        H5MM_free(shared_chunks_info_array);
        shared_chunks_info_array = NULL;
    } /* end if */

    /* Now that the chunks have been redistributed, each process must send its modification data
     * to the new owners of any of the chunks it previously possessed. Accordingly, each process
     * must also issue asynchronous receives for any messages it may receive for each of the
     * chunks it is assigned, in order to avoid potential deadlocking issues.
     */
    if (*local_chunk_array_num_entries)
        if (NULL == (mod_data = (unsigned char **)H5MM_malloc(*local_chunk_array_num_entries *
                                                              sizeof(unsigned char *))))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "unable to allocate modification data buffer array")

    for (i = 0, last_assigned_idx = 0; i < *local_chunk_array_num_entries; i++) {
        H5D_filtered_collective_io_info_t *chunk_entry = &local_chunk_array[i];

        if (mpi_rank != chunk_entry->owners.new_owner) {
            H5D_chunk_info_t *chunk_info = NULL;
            unsigned char *   mod_data_p = NULL;
            hsize_t           iter_nelmts;
            size_t            mod_data_size;

            /* Look up the chunk and get its file and memory dataspaces */
            if (NULL == (chunk_info = (H5D_chunk_info_t *)H5SL_search(fm->sel_chunks, &chunk_entry->index)))
                HGOTO_ERROR(H5E_DATASPACE, H5E_NOTFOUND, FAIL, "can't locate chunk in skip list")

            /* Determine size of serialized chunk file dataspace, plus the size of
             * the data being written
             */
            if (H5S_encode(chunk_info->fspace, &mod_data_p, &mod_data_size) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTENCODE, FAIL, "unable to get encoded dataspace size")

            iter_nelmts = H5S_GET_SELECT_NPOINTS(chunk_info->mspace);

            H5_CHECK_OVERFLOW(iter_nelmts, hsize_t, size_t);
            mod_data_size += (size_t)iter_nelmts * type_info->src_type_size;

            if (NULL == (mod_data[num_send_requests] = (unsigned char *)H5MM_malloc(mod_data_size)))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL,
                            "couldn't allocate chunk modification send buffer")

            /* Serialize the chunk's file dataspace into the buffer */
            mod_data_p = mod_data[num_send_requests];
            if (H5S_encode(chunk_info->fspace, &mod_data_p, &mod_data_size) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTENCODE, FAIL, "unable to encode dataspace")

            /* Initialize iterator for memory selection */
            if (H5S_select_iter_init(mem_iter, chunk_info->mspace, type_info->src_type_size, 0) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL,
                            "unable to initialize memory selection information")
            mem_iter_init = TRUE;

            /* Collect the modification data into the buffer */
            if (0 == H5D__gather_mem(io_info->u.wbuf, mem_iter, (size_t)iter_nelmts, mod_data_p))
                HGOTO_ERROR(H5E_IO, H5E_CANTGATHER, FAIL, "couldn't gather from write buffer")

            /* Send modification data to new owner */
            H5_CHECK_OVERFLOW(mod_data_size, size_t, int)
            H5_CHECK_OVERFLOW(chunk_entry->index, hsize_t, int)
            if (MPI_SUCCESS !=
                (mpi_code = MPI_Isend(mod_data[num_send_requests], (int)mod_data_size, MPI_BYTE,
                                      chunk_entry->owners.new_owner, (int)chunk_entry->index, io_info->comm,
                                      &send_requests[num_send_requests])))
                HMPI_GOTO_ERROR(FAIL, "MPI_Isend failed", mpi_code)

            if (mem_iter_init && H5S_SELECT_ITER_RELEASE(mem_iter) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "couldn't release memory selection iterator")
            mem_iter_init = FALSE;

            num_send_requests++;
        } /* end if */
        else {
            /* Allocate all necessary buffers for an asynchronous receive operation */
            if (chunk_entry->num_writers > 1) {
                MPI_Message message;
                MPI_Status  status;
                size_t      j;

                chunk_entry->async_info.num_receive_requests = (int)chunk_entry->num_writers - 1;
                if (NULL == (chunk_entry->async_info.receive_requests_array = (MPI_Request *)H5MM_malloc(
                                 (size_t)chunk_entry->async_info.num_receive_requests * sizeof(MPI_Request))))
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "unable to allocate async requests array")

                if (NULL ==
                    (chunk_entry->async_info.receive_buffer_array = (unsigned char **)H5MM_malloc(
                         (size_t)chunk_entry->async_info.num_receive_requests * sizeof(unsigned char *))))
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "unable to allocate async receive buffers")

                for (j = 0; j < chunk_entry->num_writers - 1; j++) {
                    int count = 0;

                    /* Probe for a particular message from any process, removing that message
                     * from the receive queue in the process and allocating that much memory
                     * for the asynchronous receive
                     */
                    if (MPI_SUCCESS != (mpi_code = MPI_Mprobe(MPI_ANY_SOURCE, (int)chunk_entry->index,
                                                              io_info->comm, &message, &status)))
                        HMPI_GOTO_ERROR(FAIL, "MPI_Mprobe failed", mpi_code)

                    if (MPI_SUCCESS != (mpi_code = MPI_Get_count(&status, MPI_BYTE, &count)))
                        HMPI_GOTO_ERROR(FAIL, "MPI_Get_count failed", mpi_code)

                    HDassert(count >= 0);
                    if (NULL == (chunk_entry->async_info.receive_buffer_array[j] =
                                     (unsigned char *)H5MM_malloc((size_t)count * sizeof(char *))))
                        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL,
                                    "unable to allocate modification data receive buffer")

                    if (MPI_SUCCESS != (mpi_code = MPI_Imrecv(
                                            chunk_entry->async_info.receive_buffer_array[j], count, MPI_BYTE,
                                            &message, &chunk_entry->async_info.receive_requests_array[j])))
                        HMPI_GOTO_ERROR(FAIL, "MPI_Imrecv failed", mpi_code)
                } /* end for */
            }     /* end if */

            local_chunk_array[last_assigned_idx++] = local_chunk_array[i];
        } /* end else */
    }     /* end for */

    *local_chunk_array_num_entries = last_assigned_idx;

    /* Wait for all async send requests to complete before returning */
    if (num_send_requests) {
        if (NULL == (send_statuses = (MPI_Status *)H5MM_malloc(num_send_requests * sizeof(MPI_Status))))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate send statuses buffer")

        H5_CHECK_OVERFLOW(num_send_requests, size_t, int);
        if (MPI_SUCCESS != (mpi_code = MPI_Waitall((int)num_send_requests, send_requests, send_statuses)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Waitall failed", mpi_code)
    } /* end if */

done:
    /* Now that all async send requests have completed, free up the send
     * buffers used in the async operations
     */
    for (i = 0; i < num_send_requests; i++) {
        if (mod_data[i])
            H5MM_free(mod_data[i]);
    } /* end for */

    if (send_requests)
        H5MM_free(send_requests);
    if (send_statuses)
        H5MM_free(send_statuses);
    if (send_counts)
        H5MM_free(send_counts);
    if (send_displacements)
        H5MM_free(send_displacements);
    if (mod_data)
        H5MM_free(mod_data);
    if (mem_iter_init && H5S_SELECT_ITER_RELEASE(mem_iter) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "couldn't release selection iterator")
    if (mem_iter)
        H5MM_free(mem_iter);
    if (num_assigned_chunks_array)
        H5MM_free(num_assigned_chunks_array);
    if (shared_chunks_info_array)
        H5MM_free(shared_chunks_info_array);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__chunk_redistribute_shared_chunks() */
#endif

/*-------------------------------------------------------------------------
 * Function:    H5D__mpio_filtered_collective_write_type
 *
 * Purpose:     Constructs a MPI derived datatype for both the memory and
 *              the file for a collective write of filtered chunks. The
 *              datatype contains the offsets in the file and the locations
 *              of the filtered chunk data buffers.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Jordan Henderson
 *              Tuesday, November 22, 2016
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__mpio_filtered_collective_write_type(H5D_filtered_collective_io_info_t *chunk_list, size_t num_entries,
                                         MPI_Datatype *new_mem_type, hbool_t *mem_type_derived,
                                         MPI_Datatype *new_file_type, hbool_t *file_type_derived)
{
    MPI_Aint *write_buf_array   = NULL; /* Relative displacements of filtered chunk data buffers */
    MPI_Aint *file_offset_array = NULL; /* Chunk offsets in the file */
    int *     length_array      = NULL; /* Filtered Chunk lengths */
    herr_t    ret_value         = SUCCEED;

    FUNC_ENTER_STATIC

    HDassert(chunk_list);
    HDassert(new_mem_type);
    HDassert(mem_type_derived);
    HDassert(new_file_type);
    HDassert(file_type_derived);

    if (num_entries > 0) {
        size_t i;
        int    mpi_code;
        void * base_buf;

        H5_CHECK_OVERFLOW(num_entries, size_t, int);

        /* Allocate arrays */
        if (NULL == (length_array = (int *)H5MM_malloc((size_t)num_entries * sizeof(int))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL,
                        "memory allocation failed for filtered collective write length array")
        if (NULL == (write_buf_array = (MPI_Aint *)H5MM_malloc((size_t)num_entries * sizeof(MPI_Aint))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL,
                        "memory allocation failed for filtered collective write buf length array")
        if (NULL == (file_offset_array = (MPI_Aint *)H5MM_malloc((size_t)num_entries * sizeof(MPI_Aint))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL,
                        "memory allocation failed for collective write offset array")

        /* Ensure the list is sorted in ascending order of offset in the file */
        HDqsort(chunk_list, num_entries, sizeof(H5D_filtered_collective_io_info_t),
                H5D__cmp_filtered_collective_io_info_entry);

        base_buf = chunk_list[0].buf;
        for (i = 0; i < num_entries; i++) {
            /* Set up the offset in the file, the length of the chunk data, and the relative
             * displacement of the chunk data write buffer
             */
            file_offset_array[i] = (MPI_Aint)chunk_list[i].chunk_states.new_chunk.offset;
            length_array[i]      = (int)chunk_list[i].chunk_states.new_chunk.length;
            write_buf_array[i]   = (MPI_Aint)chunk_list[i].buf - (MPI_Aint)base_buf;
        } /* end for */

        /* Create memory MPI type */
        if (MPI_SUCCESS != (mpi_code = MPI_Type_create_hindexed((int)num_entries, length_array,
                                                                write_buf_array, MPI_BYTE, new_mem_type)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_create_hindexed failed", mpi_code)
        *mem_type_derived = TRUE;
        if (MPI_SUCCESS != (mpi_code = MPI_Type_commit(new_mem_type)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)

        /* Create file MPI type */
        if (MPI_SUCCESS != (mpi_code = MPI_Type_create_hindexed((int)num_entries, length_array,
                                                                file_offset_array, MPI_BYTE, new_file_type)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_create_hindexed failed", mpi_code)
        *file_type_derived = TRUE;
        if (MPI_SUCCESS != (mpi_code = MPI_Type_commit(new_file_type)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)
    } /* end if */

done:
    if (write_buf_array)
        H5MM_free(write_buf_array);
    if (file_offset_array)
        H5MM_free(file_offset_array);
    if (length_array)
        H5MM_free(length_array);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__mpio_filtered_collective_write_type() */

/*-------------------------------------------------------------------------
 * Function:    H5D__filtered_collective_chunk_entry_io
 *
 * Purpose:     Given an entry for a filtered chunk, performs the necessary
 *              steps for updating the chunk data during a collective
 *              write, or for reading the chunk from file during a
 *              collective read.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Jordan Henderson
 *              Wednesday, January 18, 2017
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D__filtered_collective_chunk_entry_io(H5D_filtered_collective_io_info_t *chunk_entry,
                                        const H5D_io_info_t *io_info, const H5D_type_info_t *type_info,
                                        const H5D_chunk_map_t *fm)
{
    H5D_chunk_info_t *chunk_info = NULL;
    H5S_sel_iter_t *  mem_iter   = NULL; /* Memory iterator for H5D__scatter_mem/H5D__gather_mem */
    H5S_sel_iter_t *  file_iter  = NULL;
    H5Z_EDC_t         err_detect; /* Error detection info */
    H5Z_cb_t          filter_cb;  /* I/O filter callback function */
    unsigned          filter_mask = 0;
    hsize_t           iter_nelmts; /* Number of points to iterate over for the chunk IO operation */
    hssize_t          extent_npoints;
    hsize_t           true_chunk_size;
    hbool_t           mem_iter_init  = FALSE;
    hbool_t           file_iter_init = FALSE;
    size_t            buf_size;
    size_t            i;
    H5S_t *           dataspace    = NULL; /* Other process' dataspace for the chunk */
    void *            tmp_gath_buf = NULL; /* Temporary gather buffer to gather into from application buffer
                                              before scattering out to the chunk data buffer (when writing data),
                                              or vice versa (when reading data) */
    int    mpi_code;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    HDassert(chunk_entry);
    HDassert(io_info);
    HDassert(type_info);
    HDassert(fm);

    /* Retrieve filter settings from API context */
    if (H5CX_get_err_detect(&err_detect) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get error detection info")
    if (H5CX_get_filter_cb(&filter_cb) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get I/O filter callback function")

    /* Look up the chunk and get its file and memory dataspaces */
    if (NULL == (chunk_info = (H5D_chunk_info_t *)H5SL_search(fm->sel_chunks, &chunk_entry->index)))
        HGOTO_ERROR(H5E_DATASPACE, H5E_NOTFOUND, FAIL, "can't locate chunk in skip list")

    if ((extent_npoints = H5S_GET_EXTENT_NPOINTS(chunk_info->fspace)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTCOUNT, FAIL, "dataspace is invalid")
    true_chunk_size = (hsize_t)extent_npoints * type_info->src_type_size;

    /* If the size of the filtered chunk is larger than the number of points in the
     * chunk file space extent times the datatype size, allocate enough space to hold the
     * whole filtered chunk. Otherwise, allocate a buffer equal to the size of the
     * chunk so that the unfiltering operation doesn't have to grow the buffer.
     */
    buf_size = MAX(chunk_entry->chunk_states.chunk_current.length, true_chunk_size);

    if (NULL == (chunk_entry->buf = H5MM_malloc(buf_size)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate chunk data buffer")

    /* If this is not a full chunk overwrite or this is a read operation, the chunk must be
     * read from the file and unfiltered.
     */
    if (!chunk_entry->full_overwrite || io_info->op_type == H5D_IO_OP_READ) {
        H5FD_mpio_xfer_t xfer_mode; /* Parallel transfer for this request */

        chunk_entry->chunk_states.new_chunk.length = chunk_entry->chunk_states.chunk_current.length;

        /* Currently, these chunk reads are done independently and will likely
         * cause issues with collective metadata reads enabled. In the future,
         * this should be refactored to use collective chunk reads - JTH */

        /* Get the original state of parallel I/O transfer mode */
        if (H5CX_get_io_xfer_mode(&xfer_mode) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get MPI-I/O transfer mode")

        /* Change the xfer_mode to independent for handling the I/O */
        if (H5CX_set_io_xfer_mode(H5FD_MPIO_INDEPENDENT) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set MPI-I/O transfer mode")

        if (H5F_shared_block_read(io_info->f_sh, H5FD_MEM_DRAW,
                                  chunk_entry->chunk_states.chunk_current.offset,
                                  chunk_entry->chunk_states.new_chunk.length, chunk_entry->buf) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "unable to read raw data chunk")

        /* Return to the original I/O transfer mode setting */
        if (H5CX_set_io_xfer_mode(xfer_mode) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set MPI-I/O transfer mode")

        if (H5Z_pipeline(&io_info->dset->shared->dcpl_cache.pline, H5Z_FLAG_REVERSE, &filter_mask, err_detect,
                         filter_cb, (size_t *)&chunk_entry->chunk_states.new_chunk.length, &buf_size,
                         &chunk_entry->buf) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTFILTER, FAIL, "couldn't unfilter chunk for modifying")
    } /* end if */
    else {
        chunk_entry->chunk_states.new_chunk.length = true_chunk_size;
    } /* end else */

    /* Initialize iterator for memory selection */
    if (NULL == (mem_iter = (H5S_sel_iter_t *)H5MM_malloc(sizeof(H5S_sel_iter_t))))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate memory iterator")

    if (H5S_select_iter_init(mem_iter, chunk_info->mspace, type_info->src_type_size, 0) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to initialize memory selection information")
    mem_iter_init = TRUE;

    /* If this is a read operation, scatter the read chunk data to the user's buffer.
     *
     * If this is a write operation, update the chunk data buffer with the modifications
     * from the current process, then apply any modifications from other processes. Finally,
     * filter the newly-updated chunk.
     */
    switch (io_info->op_type) {
        case H5D_IO_OP_READ:
            if (NULL == (file_iter = (H5S_sel_iter_t *)H5MM_malloc(sizeof(H5S_sel_iter_t))))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate file iterator")

            if (H5S_select_iter_init(file_iter, chunk_info->fspace, type_info->src_type_size, 0) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL,
                            "unable to initialize memory selection information")
            file_iter_init = TRUE;

            iter_nelmts = H5S_GET_SELECT_NPOINTS(chunk_info->fspace);

            if (NULL == (tmp_gath_buf = H5MM_malloc(iter_nelmts * type_info->src_type_size)))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate temporary gather buffer")

            if (!H5D__gather_mem(chunk_entry->buf, file_iter, (size_t)iter_nelmts, tmp_gath_buf))
                HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "couldn't gather from chunk buffer")

            iter_nelmts = H5S_GET_SELECT_NPOINTS(chunk_info->mspace);

            if (H5D__scatter_mem(tmp_gath_buf, mem_iter, (size_t)iter_nelmts, io_info->u.rbuf) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "couldn't scatter to read buffer")

            break;

        case H5D_IO_OP_WRITE:
            iter_nelmts = H5S_GET_SELECT_NPOINTS(chunk_info->mspace);

            if (NULL == (tmp_gath_buf = H5MM_malloc(iter_nelmts * type_info->src_type_size)))
                HGOTO_ERROR(H5E_DATASET, H5E_CANTALLOC, FAIL, "couldn't allocate temporary gather buffer")

            /* Gather modification data from the application write buffer into a temporary buffer */
            if (0 == H5D__gather_mem(io_info->u.wbuf, mem_iter, (size_t)iter_nelmts, tmp_gath_buf))
                HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "couldn't gather from write buffer")

            if (H5S_SELECT_ITER_RELEASE(mem_iter) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "couldn't release selection iterator")
            mem_iter_init = FALSE;

            /* Initialize iterator for file selection */
            if (H5S_select_iter_init(mem_iter, chunk_info->fspace, type_info->dst_type_size, 0) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL,
                            "unable to initialize file selection information")
            mem_iter_init = TRUE;

            iter_nelmts = H5S_GET_SELECT_NPOINTS(chunk_info->fspace);

            /* Scatter the owner's modification data into the chunk data buffer according to
             * the file space.
             */
            if (H5D__scatter_mem(tmp_gath_buf, mem_iter, (size_t)iter_nelmts, chunk_entry->buf) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "couldn't scatter to chunk data buffer")

            if (H5S_SELECT_ITER_RELEASE(mem_iter) < 0)
                HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "couldn't release selection iterator")
            mem_iter_init = FALSE;

            if (MPI_SUCCESS !=
                (mpi_code = MPI_Waitall(chunk_entry->async_info.num_receive_requests,
                                        chunk_entry->async_info.receive_requests_array, MPI_STATUSES_IGNORE)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Waitall failed", mpi_code)

            /* For each asynchronous receive call previously posted, receive the chunk modification
             * buffer from another rank and update the chunk data
             */
            for (i = 0; i < (size_t)chunk_entry->async_info.num_receive_requests; i++) {
                const unsigned char *mod_data_p;

                /* Decode the process' chunk file dataspace */
                mod_data_p = chunk_entry->async_info.receive_buffer_array[i];
                if (NULL == (dataspace = H5S_decode(&mod_data_p)))
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTDECODE, FAIL, "unable to decode dataspace")

                if (H5S_select_iter_init(mem_iter, dataspace, type_info->dst_type_size, 0) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL,
                                "unable to initialize memory selection information")
                mem_iter_init = TRUE;

                iter_nelmts = H5S_GET_SELECT_NPOINTS(dataspace);

                /* Update the chunk data with the received modification data */
                if (H5D__scatter_mem(mod_data_p, mem_iter, (size_t)iter_nelmts, chunk_entry->buf) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "couldn't scatter to write buffer")

                if (H5S_SELECT_ITER_RELEASE(mem_iter) < 0)
                    HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "couldn't release selection iterator")
                mem_iter_init = FALSE;
                if (dataspace) {
                    if (H5S_close(dataspace) < 0)
                        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTFREE, FAIL, "can't close dataspace")
                    dataspace = NULL;
                }
                H5MM_free(chunk_entry->async_info.receive_buffer_array[i]);
            } /* end for */

            /* Filter the chunk */
            if (H5Z_pipeline(&io_info->dset->shared->dcpl_cache.pline, 0, &filter_mask, err_detect, filter_cb,
                             (size_t *)&chunk_entry->chunk_states.new_chunk.length, &buf_size,
                             &chunk_entry->buf) < 0)
                HGOTO_ERROR(H5E_PLINE, H5E_CANTFILTER, FAIL, "output pipeline failed")

#if H5_SIZEOF_SIZE_T > 4
            /* Check for the chunk expanding too much to encode in a 32-bit value */
            if (chunk_entry->chunk_states.new_chunk.length > ((size_t)0xffffffff))
                HGOTO_ERROR(H5E_DATASET, H5E_BADRANGE, FAIL, "chunk too large for 32-bit length")
#endif
            break;

        default:
            HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, FAIL, "invalid I/O operation")
    } /* end switch */

done:
    if (chunk_entry->async_info.receive_buffer_array)
        H5MM_free(chunk_entry->async_info.receive_buffer_array);
    if (chunk_entry->async_info.receive_requests_array)
        H5MM_free(chunk_entry->async_info.receive_requests_array);
    if (tmp_gath_buf)
        H5MM_free(tmp_gath_buf);
    if (file_iter_init && H5S_SELECT_ITER_RELEASE(file_iter) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "couldn't release selection iterator")
    if (file_iter)
        H5MM_free(file_iter);
    if (mem_iter_init && H5S_SELECT_ITER_RELEASE(mem_iter) < 0)
        HDONE_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "couldn't release selection iterator")
    if (mem_iter)
        H5MM_free(mem_iter);
    if (dataspace)
        if (H5S_close(dataspace) < 0)
            HDONE_ERROR(H5E_DATASPACE, H5E_CANTFREE, FAIL, "can't close dataspace")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D__filtered_collective_chunk_entry_io() */
#endif /* H5_HAVE_PARALLEL */
