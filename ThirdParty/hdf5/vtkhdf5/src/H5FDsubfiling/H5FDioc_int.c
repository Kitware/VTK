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
 * Purpose: This is part of an I/O concentrator driver.
 */

#include "H5FDioc_priv.h"

/*
 * Given a file offset, the stripe size, the number of IOCs and the number of
 * subfiles, calculate the target IOC for I/O, the index of the target subfile
 * out of the subfiles that the IOC controls and the file offset into that subfile
 */
static inline void
H5FD__ioc_calculate_target_ioc(int64_t file_offset, int64_t stripe_size, int num_io_concentrators,
                               int num_subfiles, int64_t *target_ioc, int64_t *ioc_file_offset,
                               int64_t *ioc_subfile_idx)
{
    int64_t stripe_idx;
    int64_t subfile_row;
    int64_t subfile_idx;

    FUNC_ENTER_PACKAGE_NOERR

    assert(stripe_size > 0);
    assert(num_io_concentrators > 0);
    assert(num_subfiles > 0);
    assert(target_ioc);
    assert(ioc_file_offset);
    assert(ioc_subfile_idx);

    stripe_idx  = file_offset / stripe_size;
    subfile_row = stripe_idx / num_subfiles;
    subfile_idx = (stripe_idx % num_subfiles) / num_io_concentrators;

    *target_ioc      = (stripe_idx % num_subfiles) % num_io_concentrators;
    *ioc_file_offset = (subfile_row * stripe_size) + (file_offset % stripe_size);
    *ioc_subfile_idx = subfile_idx;

    FUNC_LEAVE_NOAPI_VOID
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__ioc_write_independent_async
 *
 * Purpose:     The IO operations can be striped across a selection of
 *              IO concentrators.  The read and write independent calls
 *              compute the group of 1 or more IOCs and further create
 *              derived MPI datatypes when required by the size of the
 *              contiguous read or write requests.
 *
 *              IOC(0) contains the logical data storage for file offset
 *              zero and all offsets that reside within modulo range of
 *              the subfiling stripe_size.
 *
 *              We cycle through all 'n_io_conentrators' and send a descriptor
 *              to each IOC that has a non-zero sized IO request to fulfill.
 *
 *              Sending descriptors to an IOC usually gets an ACK or NACK in
 *              response.  For the write operations, we post asynch READs to
 *              receive ACKs from IOC ranks that have allocated memory receive
 *              the data to write to the subfile.  Upon receiving an ACK, we
 *              send the actual user data to the IOC.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__ioc_write_independent_async(int64_t context_id, int64_t offset, int64_t elements, const void *data,
                                  io_req_t **io_req)
{
    subfiling_context_t *sf_context    = NULL;
    MPI_Request          ack_request   = MPI_REQUEST_NULL;
    io_req_t            *sf_io_request = NULL;
    int64_t              ioc_start;
    int64_t              ioc_offset;
    int64_t              ioc_subfile_idx;
    int64_t              msg[3]           = {0};
    int                 *io_concentrators = NULL;
    int                  num_io_concentrators;
    int                  num_subfiles;
    int                  data_tag = 0;
    int                  mpi_code;
    herr_t               ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(io_req);

    if (NULL == (sf_context = H5FD__subfiling_get_object(context_id)))
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "can't get subfiling context from ID");
    assert(sf_context->topology);
    assert(sf_context->topology->io_concentrators);

    io_concentrators     = sf_context->topology->io_concentrators;
    num_io_concentrators = sf_context->topology->n_io_concentrators;
    num_subfiles         = sf_context->sf_num_subfiles;

    /*
     * Calculate the IOC that we'll send the I/O request to and the offset within
     * that IOC's subfile
     */
    H5FD__ioc_calculate_target_ioc(offset, sf_context->sf_stripe_size, num_io_concentrators, num_subfiles,
                                   &ioc_start, &ioc_offset, &ioc_subfile_idx);

    /*
     * Wait for memory to be allocated on the target IOC before beginning send of
     * user data. Once that memory has been allocated, we will receive an ACK (or
     * NACK) message from the IOC to allow us to proceed.
     *
     * On ACK, the IOC will send the tag to be used for sending data. This allows
     * us to distinguish between multiple concurrent writes from a single rank.
     *
     * Post an early non-blocking receive for the MPI tag here.
     */
    if (MPI_SUCCESS != (mpi_code = MPI_Irecv(&data_tag, 1, MPI_INT, io_concentrators[ioc_start],
                                             WRITE_INDEP_ACK, sf_context->sf_data_comm, &ack_request)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Irecv failed", mpi_code);

    /* Prepare and send an I/O request to the IOC identified by the file offset */
    msg[0] = elements;
    msg[1] = ioc_offset;
    msg[2] = ioc_subfile_idx;
    if (MPI_SUCCESS != (mpi_code = MPI_Send(msg, 1, H5_subfiling_rpc_msg_type, io_concentrators[ioc_start],
                                            WRITE_INDEP, sf_context->sf_msg_comm)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Send failed", mpi_code);

    /* Wait to receive the data tag from the IOC */
    if (MPI_SUCCESS != (mpi_code = MPI_Wait(&ack_request, MPI_STATUS_IGNORE)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Wait failed", mpi_code);

    if (data_tag == 0)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "received NACK from IOC");

    /* Allocate the I/O request object that will be returned to the caller */
    if (NULL == (sf_io_request = H5MM_malloc(sizeof(io_req_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_WRITEERROR, FAIL, "couldn't allocate I/O request");

    H5_CHECK_OVERFLOW(ioc_start, int64_t, int);
    sf_io_request->ioc             = (int)ioc_start;
    sf_io_request->context_id      = context_id;
    sf_io_request->offset          = offset;
    sf_io_request->elements        = elements;
    sf_io_request->data            = H5FD__subfiling_cast_to_void(data);
    sf_io_request->io_transfer_req = MPI_REQUEST_NULL;
    sf_io_request->io_comp_req     = MPI_REQUEST_NULL;
    sf_io_request->io_comp_tag     = -1;

    /*
     * Start a non-blocking receive from the IOC that signifies
     * when the actual write is complete
     */
    if (MPI_SUCCESS !=
        (mpi_code = MPI_Irecv(&sf_io_request->io_comp_tag, 1, MPI_INT, io_concentrators[ioc_start],
                              WRITE_DATA_DONE, sf_context->sf_data_comm, &sf_io_request->io_comp_req)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Irecv failed", mpi_code);

    /*
     * Start the actual data transfer using the ack received
     * from the IOC as the tag for the send
     */
    H5_CHECK_OVERFLOW(elements, int64_t, int);
    if (MPI_SUCCESS !=
        (mpi_code = MPI_Isend(data, (int)elements, MPI_BYTE, io_concentrators[ioc_start], data_tag,
                              sf_context->sf_data_comm, &sf_io_request->io_transfer_req)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Isend failed", mpi_code);

    /*
     * NOTE: When we actually have the async I/O support, the request should be
     * queued before we return to the caller. Having queued the I/O operation, we
     * might want to get additional work started before allowing the queued I/O
     * requests to make further progress and/or to complete, so we just return
     * to the caller.
     */
    *io_req = sf_io_request;

done:
    if (ret_value < 0) {
        if (ack_request != MPI_REQUEST_NULL)
            if (MPI_SUCCESS != (mpi_code = MPI_Wait(&ack_request, MPI_STATUS_IGNORE)))
                HMPI_DONE_ERROR(FAIL, "MPI_Wait failed", mpi_code);
        if (sf_io_request) {
            if (sf_io_request->io_transfer_req != MPI_REQUEST_NULL)
                if (MPI_SUCCESS != (mpi_code = MPI_Wait(&sf_io_request->io_transfer_req, MPI_STATUS_IGNORE)))
                    HMPI_DONE_ERROR(FAIL, "MPI_Wait failed", mpi_code);
            if (sf_io_request->io_comp_req != MPI_REQUEST_NULL)
                if (MPI_SUCCESS != (mpi_code = MPI_Wait(&sf_io_request->io_comp_req, MPI_STATUS_IGNORE)))
                    HMPI_DONE_ERROR(FAIL, "MPI_Wait failed", mpi_code);
        }

        H5MM_free(sf_io_request);
        *io_req = NULL;
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__ioc_write_independent_async() */

/*-------------------------------------------------------------------------
 * Function:    Internal H5FD__ioc_read_independent_async
 *
 * Purpose:     The IO operations can be striped across a selection of
 *              IO concentrators.  The read and write independent calls
 *              compute the group of 1 or more IOCs and further create
 *              derived MPI datatypes when required by the size of the
 *              contiguous read or write requests.
 *
 *              IOC(0) contains the logical data storage for file offset
 *              zero and all offsets that reside within modulo range of
 *              the subfiling stripe_size.
 *
 *              We cycle through all 'n_io_conentrators' and send a
 *              descriptor to each IOC that has a non-zero sized IO
 *              request to fulfill.
 *
 *              Sending descriptors to an IOC usually gets an ACK or
 *              NACK in response.  For the read operations, we post
 *              asynch READs to receive the file data and wait until
 *              all pending operations have completed.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__ioc_read_independent_async(int64_t context_id, int64_t offset, int64_t elements, void *data,
                                 io_req_t **io_req)
{
    subfiling_context_t *sf_context    = NULL;
    MPI_Request          ack_request   = MPI_REQUEST_NULL;
    io_req_t            *sf_io_request = NULL;
    bool                 need_data_tag = false;
    int64_t              ioc_start;
    int64_t              ioc_offset;
    int64_t              ioc_subfile_idx;
    int64_t              msg[3]           = {0};
    int                 *io_concentrators = NULL;
    int                  num_io_concentrators;
    int                  num_subfiles;
    int                  data_tag = 0;
    int                  mpi_code;
    herr_t               ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(io_req);

    H5_CHECK_OVERFLOW(elements, int64_t, int);

    if (NULL == (sf_context = H5FD__subfiling_get_object(context_id)))
        HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "can't get subfiling context from ID");
    assert(sf_context->topology);
    assert(sf_context->topology->io_concentrators);

    io_concentrators     = sf_context->topology->io_concentrators;
    num_io_concentrators = sf_context->topology->n_io_concentrators;
    num_subfiles         = sf_context->sf_num_subfiles;

    /*
     * If we are using 1 subfile per IOC, we can optimize reads a little since
     * each read will go to a separate IOC and we won't be in danger of data
     * being received in an unpredictable order. However, if some IOCs own more
     * than 1 subfile, we need to associate each read with a unique message tag
     * to make sure the data is received in the correct order. We also need a
     * unique message tag in the case where only 1 subfile is used in total. In
     * this case, vector I/O calls are passed directly down to this VFD without
     * being split up into multiple I/O requests, so we need the tag to
     * distinguish each I/O request.
     */
    need_data_tag = (num_subfiles == 1) || (num_subfiles != num_io_concentrators);
    if (!need_data_tag)
        data_tag = READ_INDEP_DATA;

    /*
     * Calculate the IOC that we'll send the I/O request to and the offset within
     * that IOC's subfile
     */
    H5FD__ioc_calculate_target_ioc(offset, sf_context->sf_stripe_size, num_io_concentrators, num_subfiles,
                                   &ioc_start, &ioc_offset, &ioc_subfile_idx);

    /* Allocate the I/O request object that will be returned to the caller */
    if (NULL == (sf_io_request = H5MM_malloc(sizeof(io_req_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_READERROR, FAIL, "couldn't allocate I/O request");

    H5_CHECK_OVERFLOW(ioc_start, int64_t, int);
    sf_io_request->ioc             = (int)ioc_start;
    sf_io_request->context_id      = context_id;
    sf_io_request->offset          = offset;
    sf_io_request->elements        = elements;
    sf_io_request->data            = data;
    sf_io_request->io_transfer_req = MPI_REQUEST_NULL;
    sf_io_request->io_comp_req     = MPI_REQUEST_NULL;
    sf_io_request->io_comp_tag     = -1;

    if (need_data_tag) {
        /*
         * Post an early non-blocking receive for IOC to send an ACK (or NACK)
         * message with a data tag that we will use for receiving data
         */
        if (MPI_SUCCESS != (mpi_code = MPI_Irecv(&data_tag, 1, MPI_INT, io_concentrators[ioc_start],
                                                 READ_INDEP_ACK, sf_context->sf_data_comm, &ack_request)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Irecv failed", mpi_code);

        /* Prepare and send an I/O request to the IOC identified by the file offset */
        msg[0] = elements;
        msg[1] = ioc_offset;
        msg[2] = ioc_subfile_idx;
        if (MPI_SUCCESS !=
            (mpi_code = MPI_Send(msg, 1, H5_subfiling_rpc_msg_type, io_concentrators[ioc_start], READ_INDEP,
                                 sf_context->sf_msg_comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Send failed", mpi_code);

        /* Wait to receive the data tag from the IOC */
        if (MPI_SUCCESS != (mpi_code = MPI_Wait(&ack_request, MPI_STATUS_IGNORE)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Wait failed", mpi_code);

        if (data_tag == 0)
            HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "received NACK from IOC");
    }

    /*
     * Post a non-blocking receive for the data from the IOC using the selected
     * data tag (either the one received from the IOC or the static
     * READ_INDEP_DATA tag)
     */
    if (MPI_SUCCESS !=
        (mpi_code = MPI_Irecv(data, (int)elements, MPI_BYTE, io_concentrators[ioc_start], data_tag,
                              sf_context->sf_data_comm, &sf_io_request->io_transfer_req)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Irecv failed", mpi_code);

    if (!need_data_tag) {
        /* Prepare and send an I/O request to the IOC identified by the file offset */
        msg[0] = elements;
        msg[1] = ioc_offset;
        msg[2] = ioc_subfile_idx;
        if (MPI_SUCCESS !=
            (mpi_code = MPI_Send(msg, 1, H5_subfiling_rpc_msg_type, io_concentrators[ioc_start], READ_INDEP,
                                 sf_context->sf_msg_comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Send failed", mpi_code);
    }

    *io_req = sf_io_request;

done:
    if (ret_value < 0) {
        if (ack_request != MPI_REQUEST_NULL)
            if (MPI_SUCCESS != (mpi_code = MPI_Wait(&ack_request, MPI_STATUS_IGNORE)))
                HMPI_DONE_ERROR(FAIL, "MPI_Wait failed", mpi_code);
        if (sf_io_request)
            if (sf_io_request->io_transfer_req != MPI_REQUEST_NULL)
                if (MPI_SUCCESS != (mpi_code = MPI_Wait(&sf_io_request->io_transfer_req, MPI_STATUS_IGNORE)))
                    HMPI_DONE_ERROR(FAIL, "MPI_Wait failed", mpi_code);

        H5MM_free(sf_io_request);
        *io_req = NULL;
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__ioc_read_independent_async() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__ioc_async_completion
 *
 * Purpose:     IOC function to complete outstanding I/O requests.
 *              Currently just a wrapper around MPI_Waitall on the given
 *              MPI_Request array.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__ioc_async_completion(MPI_Request *mpi_reqs, size_t num_reqs)
{
    herr_t ret_value = SUCCEED;
    int    mpi_code;

    FUNC_ENTER_PACKAGE

    assert(mpi_reqs);

    H5_CHECK_OVERFLOW(num_reqs, size_t, int);

    /* Have to suppress gcc warnings regarding MPI_STATUSES_IGNORE
     * with MPICH (https://github.com/pmodels/mpich/issues/5687)
     */
    H5_GCC_DIAG_OFF("stringop-overflow")
    if (MPI_SUCCESS != (mpi_code = MPI_Waitall((int)num_reqs, mpi_reqs, MPI_STATUSES_IGNORE)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Waitall failed", mpi_code);
    H5_GCC_DIAG_ON("stringop-overflow")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}
