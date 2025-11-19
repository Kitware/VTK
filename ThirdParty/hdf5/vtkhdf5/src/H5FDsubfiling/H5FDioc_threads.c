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

#include "H5FDioc_priv.h"

#include "H5FDsubfiling.h"

#define MIN_READ_RETRIES 10

/*
 * The amount of time (in nanoseconds) for the IOC main
 * thread to sleep when there are no incoming I/O requests
 * to process
 */
#define IOC_MAIN_SLEEP_DELAY (20000)

/*
 * IOC data for a file that is stored in that
 * file's subfiling context object
 */
typedef struct ioc_data_t {
    ioc_io_queue_t    io_queue;
    hg_thread_t       ioc_main_thread;
    hg_thread_pool_t *io_thread_pool;
    int64_t           sf_context_id;

    atomic_int sf_ioc_ready;
    atomic_int sf_shutdown_flag;
    /* sf_io_ops_pending tracks the number of I/O operations pending so that we can wait
     * until all I/O operations have been serviced before shutting down the worker thread pool.
     * The value of this variable must always be non-negative.
     *
     * Note that this is a convenience variable -- we could use io_queue.q_len instead.
     * However, accessing this field requires locking io_queue.q_mutex.
     */
    atomic_int sf_io_ops_pending;
    atomic_int sf_work_pending;
} ioc_data_t;

/*
 * NOTES:
 * Rather than re-create the code for creating and managing a thread pool,
 * I'm utilizing a reasonably well tested implementation from the mercury
 * project.  At some point, we should revisit this decision or possibly
 * directly link against the mercury library.  This would make sense if
 * we move away from using MPI as the messaging infrastructure and instead
 * use mercury for that purpose...
 */

static hg_thread_mutex_t ioc_thread_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef H5FD_IOC_COLLECT_STATS
static int    sf_write_ops        = 0;
static int    sf_read_ops         = 0;
static double sf_pwrite_time      = 0.0;
static double sf_pread_time       = 0.0;
static double sf_write_wait_time  = 0.0;
static double sf_queue_delay_time = 0.0;
#endif

/* Prototypes */
static HG_THREAD_RETURN_TYPE H5FD__ioc_thread_main(void *arg);

static int H5FD__ioc_file_queue_write_indep(sf_work_request_t *msg, int ioc_idx, int source, MPI_Comm comm,
                                            uint32_t counter);
static int H5FD__ioc_file_queue_read_indep(sf_work_request_t *msg, int ioc_idx, int source, MPI_Comm comm,
                                           uint32_t counter);

static int H5FD__ioc_file_write_data(int fd, int64_t file_offset, void *data_buffer, int64_t data_size,
                                     int ioc_idx);
static int H5FD__ioc_file_read_data(int fd, int64_t file_offset, void *data_buffer, int64_t data_size,
                                    int ioc_idx);
static int H5FD__ioc_file_truncate(sf_work_request_t *msg);
static int H5FD__ioc_file_report_eof(sf_work_request_t *msg, MPI_Comm comm);

static ioc_io_queue_entry_t *H5FD__ioc_io_queue_alloc_entry(void);
static void H5FD__ioc_io_queue_complete_entry(ioc_data_t *ioc_data, ioc_io_queue_entry_t *entry_ptr);
static void H5FD__ioc_io_queue_dispatch_eligible_entries(ioc_data_t *ioc_data, bool try_lock);
static void H5FD__ioc_io_queue_add_entry(ioc_data_t *ioc_data, sf_work_request_t *wk_req_ptr);

/*-------------------------------------------------------------------------
 * Function:    H5FD__ioc_init_threads
 *
 * Purpose:     The principal entry point to initialize the execution
 *              context for an I/O Concentrator (IOC). The main thread
 *              is responsible for receiving I/O requests from each
 *              HDF5 "client" and distributing those to helper threads
 *              for actual processing. We initialize a fixed number
 *              of helper threads by creating a thread pool.
 *
 * Return:      SUCCESS (0) or FAIL (-1) if any errors are detected
 *              for the multi-threaded initialization.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__ioc_init_threads(void *_sf_context)
{
    subfiling_context_t *sf_context       = _sf_context;
    ioc_data_t          *ioc_data         = NULL;
    unsigned             thread_pool_size = H5FD_IOC_DEFAULT_THREAD_POOL_SIZE;
    char                *env_value;
    herr_t               ret_value = SUCCEED;
#ifdef H5FD_IOC_COLLECT_STATS
    double t_start = 0.0, t_end = 0.0;
#endif

    FUNC_ENTER_PACKAGE

    assert(sf_context);

    /* Allocate and initialize IOC data that will be passed to the IOC main thread */
    if (NULL == (ioc_data = H5MM_malloc(sizeof(*ioc_data))))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "can't allocate IOC data for IOC main thread");
    ioc_data->sf_context_id  = sf_context->sf_context_id;
    ioc_data->io_thread_pool = NULL;
    ioc_data->io_queue       = (ioc_io_queue_t){/* magic               = */ H5FD_IOC__IO_Q_MAGIC,
                                          /* q_head              = */ NULL,
                                          /* q_tail              = */ NULL,
                                          /* num_pending         = */ 0,
                                          /* num_in_progress     = */ 0,
                                          /* num_failed          = */ 0,
                                          /* q_len               = */ 0,
                                          /* req_counter         = */ 0,
                                          /* q_mutex             = */
                                          PTHREAD_MUTEX_INITIALIZER,
#ifdef H5FD_IOC_COLLECT_STATS
                                          /* max_q_len           = */ 0,
                                          /* max_num_pending     = */ 0,
                                          /* max_num_in_progress = */ 0,
                                          /* ind_read_requests   = */ 0,
                                          /* ind_write_requests  = */ 0,
                                          /* truncate_requests   = */ 0,
                                          /* get_eof_requests    = */ 0,
                                          /* requests_queued     = */ 0,
                                          /* requests_dispatched = */ 0,
                                          /* requests_completed  = */ 0
#endif
    };

    sf_context->ioc_data = ioc_data;

    /* Initialize atomic vars */
    atomic_init(&ioc_data->sf_ioc_ready, 0);
    atomic_init(&ioc_data->sf_shutdown_flag, 0);
    atomic_init(&ioc_data->sf_io_ops_pending, 0);
    atomic_init(&ioc_data->sf_work_pending, 0);

#ifdef H5FD_IOC_COLLECT_STATS
    t_start = MPI_Wtime();
#endif

    if (hg_thread_mutex_init(&ioc_data->io_queue.q_mutex) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "can't initialize IOC thread queue mutex");

    /* Allow experimentation with the number of helper threads */
    if ((env_value = getenv(H5FD_IOC_THREAD_POOL_SIZE)) != NULL) {
        int value_check = atoi(env_value);
        if (value_check > 0)
            thread_pool_size = (unsigned int)value_check;
    }

    /* Initialize a thread pool for the I/O concentrator's worker threads */
    if (hg_thread_pool_init(thread_pool_size, &ioc_data->io_thread_pool) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "can't initialize IOC worker thread pool");

    /* Create the main IOC thread that will receive and dispatch I/O requests */
    if (hg_thread_create(&ioc_data->ioc_main_thread, H5FD__ioc_thread_main, ioc_data) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "can't create IOC main thread");

    /* Wait until H5FD__ioc_thread_main() reports that it is ready */
    while (atomic_load(&ioc_data->sf_ioc_ready) != 1)
        usleep(20);

#ifdef H5FD_IOC_COLLECT_STATS
    t_end = MPI_Wtime();

#ifdef H5FD_IOC_DEBUG
    if (sf_context->topology->ioc_idx == 0) {
        printf("%s: time = %lf seconds\n", __func__, (t_end - t_start));
        fflush(stdout);
    }
#endif

#endif

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

herr_t
H5FD__ioc_finalize_threads(void *_sf_context)
{
    subfiling_context_t *sf_context = _sf_context;
    ioc_data_t          *ioc_data   = NULL;
    herr_t               ret_value  = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(sf_context);
    assert(sf_context->topology->rank_is_ioc);

    ioc_data = sf_context->ioc_data;
    if (ioc_data) {
        assert(0 == atomic_load(&ioc_data->sf_shutdown_flag));

        /* Shutdown the main IOC thread */
        atomic_store(&ioc_data->sf_shutdown_flag, 1);

        /* Allow H5FD__ioc_thread_main to exit.*/
        do {
            usleep(20);
        } while (0 != atomic_load(&ioc_data->sf_shutdown_flag));

        /* Tear down IOC worker thread pool */
        assert(0 == atomic_load(&ioc_data->sf_io_ops_pending));
        hg_thread_pool_destroy(ioc_data->io_thread_pool);

        hg_thread_mutex_destroy(&ioc_data->io_queue.q_mutex);

        /* Wait for IOC main thread to exit */
        hg_thread_join(ioc_data->ioc_main_thread);
    }

    if (ioc_data->io_queue.num_failed > 0)
        HDONE_ERROR(H5E_VFL, H5E_CLOSEERROR, FAIL, "%" PRId32 " I/O requests failed",
                    ioc_data->io_queue.num_failed);

    H5MM_free(ioc_data);
    sf_context->ioc_data = NULL;

    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__ioc_thread_main
 *
 * Purpose:     This is the principal function run by the I/O Concentrator
 *              main thread.  It remains within a loop until allowed to
 *              exit by means of setting the 'sf_shutdown_flag'. This is
 *              usually accomplished as part of the file close operation.
 *
 *              The function implements an asynchronous polling approach
 *              for incoming messages. These messages can be thought of
 *              as a primitive RPC which utilizes MPI tags to code and
 *              implement the desired subfiling functionality.
 *
 *              As each incoming message is received, it gets added to
 *              a queue for processing by a thread_pool thread. The
 *              message handlers are dispatched via the
 *              "H5FD__ioc_handle_work_request" routine.
 *
 *              Subfiling is effectively a software RAID-0 implementation
 *              where having multiple I/O Concentrators and independent
 *              subfiles is equated to the multiple disks and a true
 *              hardware base RAID implementation.
 *
 *              I/O Concentrators are ordered according to their MPI rank.
 *              In the simplest interpretation, IOC(0) will always contain
 *              the initial bytes of the logical disk image.  Byte 0 of
 *              IOC(1) will contain the byte written to the logical disk
 *              offset "stripe_size" X IOC(number).
 *
 *              Example: If the stripe size is defined to be 256K, then
 *              byte 0 of subfile(1) is at logical offset 262144 of the
 *              file.   Similarly, byte 0 of subfile(2) represents the
 *              logical file offset = 524288.   For logical files larger
 *              than 'N' X stripe_size, we simply "wrap around" back to
 *              subfile(0).  The following shows the mapping of 30
 *              logical blocks of data over 3 subfiles:
 *              +--------+--------+--------+--------+--------+--------+
 *              | blk(0 )| blk(1) | blk(2 )| blk(3 )| blk(4 )| blk(5 )|
 *              | IOC(0) | IOC(1) | IOC(2) | IOC(0) | IOC(1) | IOC(2) |
 *              +--------+--------+--------+--------+--------+--------+
 *              | blk(6 )| blk(7) | blk(8 )| blk(9 )| blk(10)| blk(11)|
 *              | IOC(0) | IOC(1) | IOC(2) | IOC(0) | IOC(1) | IOC(2) |
 *              +--------+--------+--------+--------+--------+--------+
 *              | blk(12)| blk(13)| blk(14)| blk(15)| blk(16)| blk(17)|
 *              | IOC(0) | IOC(1) | IOC(2) | IOC(0) | IOC(1) | IOC(2) |
 *              +--------+--------+--------+--------+--------+--------+
 *              | blk(18)| blk(19)| blk(20)| blk(21)| blk(22)| blk(23)|
 *              | IOC(0) | IOC(1) | IOC(2) | IOC(0) | IOC(1) | IOC(2) |
 *              +--------+--------+--------+--------+--------+--------+
 *              | blk(24)| blk(25)| blk(26)| blk(27)| blk(28)| blk(29)|
 *              | IOC(0) | IOC(1) | IOC(2) | IOC(0) | IOC(1) | IOC(2) |
 *              +--------+--------+--------+--------+--------+--------+
 *
 * Return:      None
 * Errors:      None
 *
 *-------------------------------------------------------------------------
 */
static HG_THREAD_RETURN_TYPE
H5FD__ioc_thread_main(void *arg)
{
    ioc_data_t          *ioc_data = (ioc_data_t *)arg;
    subfiling_context_t *context  = NULL;
    sf_work_request_t    wk_req;
    int                  shutdown_requested;
    hg_thread_ret_t      ret_value = (hg_thread_ret_t)SUCCEED;

    assert(ioc_data);

    context = H5FD__subfiling_get_object(ioc_data->sf_context_id);
    assert(context);

    /* We can't have opened any files at this point..
     * The file open approach has changed so that the normal
     * application rank (hosting this thread) does the file open.
     * We can simply utilize the file descriptor (which should now
     * represent an open file).
     */

    /* tell H5FD__ioc_init_threads() that ioc_main() is ready to enter its main loop */
    atomic_store(&ioc_data->sf_ioc_ready, 1);

    shutdown_requested = 0;

    while ((!shutdown_requested) || (0 < atomic_load(&ioc_data->sf_io_ops_pending)) ||
           (0 < atomic_load(&ioc_data->sf_work_pending))) {
        MPI_Status status;
        int        flag = 0;
        int        mpi_code;

        /* Probe for incoming work requests */
        if (MPI_SUCCESS !=
            (mpi_code = (MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, context->sf_msg_comm, &flag, &status))))
            HGOTO_DONE((hg_thread_ret_t)FAIL);

        if (flag) {
            int count;
            int source = status.MPI_SOURCE;
            int tag    = status.MPI_TAG;

            if (tag != READ_INDEP && tag != WRITE_INDEP && tag != TRUNC_OP && tag != GET_EOF_OP)
                HGOTO_DONE((hg_thread_ret_t)FAIL);

            if (MPI_SUCCESS != (mpi_code = MPI_Get_count(&status, MPI_BYTE, &count)))
                HGOTO_DONE((hg_thread_ret_t)FAIL);

            if (count < 0)
                HGOTO_DONE((hg_thread_ret_t)FAIL);
            if ((size_t)count > sizeof(sf_work_request_t))
                HGOTO_DONE((hg_thread_ret_t)FAIL);

            /*
             * Zero out work request, since the received message should
             * be smaller than sizeof(sf_work_request_t)
             */
            memset(&wk_req, 0, sizeof(sf_work_request_t));
            if (MPI_SUCCESS != (mpi_code = MPI_Recv(&wk_req, count, MPI_BYTE, source, tag,
                                                    context->sf_msg_comm, MPI_STATUS_IGNORE)))
                HGOTO_DONE((hg_thread_ret_t)FAIL);

            /* Dispatch work request to worker threads in thread pool */

            wk_req.tag        = tag;
            wk_req.source     = source;
            wk_req.ioc_idx    = context->topology->ioc_idx;
            wk_req.context_id = ioc_data->sf_context_id;
#ifdef H5FD_IOC_COLLECT_STATS
            wk_req.start_time = MPI_Wtime();
#endif

            H5FD__ioc_io_queue_add_entry(ioc_data, &wk_req);

            assert(atomic_load(&ioc_data->sf_io_ops_pending) >= 0);
        }
        else {
            struct timespec sleep_spec = {0, IOC_MAIN_SLEEP_DELAY};

            HDnanosleep(&sleep_spec, NULL);
        }

        H5FD__ioc_io_queue_dispatch_eligible_entries(ioc_data, flag ? 0 : 1);

        shutdown_requested = atomic_load(&ioc_data->sf_shutdown_flag);
    }

    /* Reset the shutdown flag */
    atomic_store(&ioc_data->sf_shutdown_flag, 0);

done:
    return ret_value;
} /* H5FD__ioc_thread_main() */

#ifdef H5_SUBFILING_DEBUG
static const char *
translate_opcode(io_op_t op)
{
    switch (op) {
        case READ_OP:
            return "READ_OP";
            break;
        case WRITE_OP:
            return "WRITE_OP";
            break;
        case OPEN_OP:
            return "OPEN_OP";
            break;
        case CLOSE_OP:
            return "CLOSE_OP";
            break;
        case TRUNC_OP:
            return "TRUNC_OP";
            break;
        case GET_EOF_OP:
            return "GET_EOF_OP";
            break;
        case FINI_OP:
            return "FINI_OP";
            break;
        case LOGGING_OP:
            return "LOGGING_OP";
            break;
        default:
            return "unknown";
    }
}
#endif

/*-------------------------------------------------------------------------
 * Function:    H5FD__ioc_handle_work_request
 *
 * Purpose:     Handle a work request from the thread pool work queue.
 *              We dispatch the specific function as indicated by the
 *              TAG that has been added to the work request by the
 *              IOC main thread (which is just a copy of the MPI tag
 *              associated with the RPC message) and provide the subfiling
 *              context associated with the HDF5 file.
 *
 *              Any status associated with the function processing is
 *              returned directly to the client via ACK or NACK messages.
 *
 * Return:      (none) Doesn't fail.
 *
 *-------------------------------------------------------------------------
 */
static HG_THREAD_RETURN_TYPE
H5FD__ioc_handle_work_request(void *arg)
{
    ioc_io_queue_entry_t *q_entry_ptr     = (ioc_io_queue_entry_t *)arg;
    subfiling_context_t  *sf_context      = NULL;
    sf_work_request_t    *msg             = &(q_entry_ptr->wk_req);
    ioc_data_t           *ioc_data        = NULL;
    int64_t               file_context_id = msg->context_id;
    int                   op_ret;
    hg_thread_ret_t       ret_value = 0;

    assert(q_entry_ptr);
    assert(q_entry_ptr->magic == H5FD_IOC__IO_Q_ENTRY_MAGIC);
    assert(q_entry_ptr->in_progress);

    sf_context = H5FD__subfiling_get_object(file_context_id);
    assert(sf_context);

    ioc_data = sf_context->ioc_data;
    assert(ioc_data);

    atomic_fetch_add(&ioc_data->sf_work_pending, 1);

    switch (msg->tag) {
        case WRITE_INDEP:
            op_ret = H5FD__ioc_file_queue_write_indep(msg, msg->ioc_idx, msg->source,
                                                      sf_context->sf_data_comm, q_entry_ptr->counter);
            break;

        case READ_INDEP:
            op_ret = H5FD__ioc_file_queue_read_indep(msg, msg->ioc_idx, msg->source, sf_context->sf_data_comm,
                                                     q_entry_ptr->counter);
            break;

        case TRUNC_OP:
            op_ret = H5FD__ioc_file_truncate(msg);
            break;

        case GET_EOF_OP:
            op_ret = H5FD__ioc_file_report_eof(msg, sf_context->sf_eof_comm);
            break;

        default:
#ifdef H5_SUBFILING_DEBUG
            H5FD__subfiling_log(file_context_id,
                                "%s: IOC %d received unknown message with tag %x from rank %d", __func__,
                                msg->ioc_idx, msg->tag, msg->source);
#endif

            op_ret = -1;
            break;
    }

    atomic_fetch_sub(&ioc_data->sf_work_pending, 1);

    if (op_ret < 0) {
#ifdef H5_SUBFILING_DEBUG
        H5FD__subfiling_log(file_context_id,
                            "%s: IOC %d request(%s) from rank(%d), (%" PRId64 ", %" PRId64 ", %" PRId64
                            ") FAILED with ret %d",
                            __func__, msg->ioc_idx, translate_opcode((io_op_t)msg->tag), msg->source,
                            msg->header[0], msg->header[1], msg->header[2], op_ret);
#endif

        q_entry_ptr->wk_ret = op_ret;
    }

#ifdef H5FD_IOC_DEBUG
    {
        int curr_io_ops_pending = atomic_load(&ioc_data->sf_io_ops_pending);
        assert(curr_io_ops_pending > 0);
    }
#endif

    /* complete the I/O request */
    H5FD__ioc_io_queue_complete_entry(ioc_data, q_entry_ptr);

    assert(atomic_load(&ioc_data->sf_io_ops_pending) >= 0);

    /* Check the I/O Queue to see if there are any dispatchable entries */
    H5FD__ioc_io_queue_dispatch_eligible_entries(ioc_data, 1);

    return ret_value;
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_ioc_begin_thread_exclusive
 *
 * Purpose:     Mutex lock to restrict access to code or variables.
 *
 * Return:      integer result of mutex_lock request.
 *
 *-------------------------------------------------------------------------
 */
void
H5FD_ioc_begin_thread_exclusive(void)
{
    hg_thread_mutex_lock(&ioc_thread_mutex);
}

/*-------------------------------------------------------------------------
 * Function:    H5FD_ioc_end_thread_exclusive
 *
 * Purpose:     Mutex unlock.  Should only be called by the current holder
 *              of the locked mutex.
 *
 * Return:      result of mutex_unlock operation.
 *
 *-------------------------------------------------------------------------
 */
void
H5FD_ioc_end_thread_exclusive(void)
{
    hg_thread_mutex_unlock(&ioc_thread_mutex);
}

static herr_t
H5FD__ioc_send_ack_to_client(int ack_val, int dest_rank, int msg_tag, MPI_Comm comm)
{
    assert(ack_val > 0);

    if (MPI_SUCCESS != MPI_Send(&ack_val, 1, MPI_INT, dest_rank, msg_tag, comm))
        return FAIL;

    return SUCCEED;
}

static herr_t
H5FD__ioc_send_nack_to_client(int dest_rank, int msg_tag, MPI_Comm comm)
{
    int nack = 0;

    if (MPI_SUCCESS != MPI_Send(&nack, 1, MPI_INT, dest_rank, msg_tag, comm))
        return FAIL;

    return SUCCEED;
}

/*
=========================================
queue_xxx functions that should be run
from the thread pool threads...
=========================================
*/

/*-------------------------------------------------------------------------
 * Function:    H5FD__ioc_file_queue_write_indep
 *
 * Purpose:     Implement the IOC independent write function.  The
 *              function is invoked as a result of the IOC receiving the
 *              "header"/RPC.  What remains is to allocate memory for the
 *              data sent by the client and then write the data to our
 *              subfile.  We utilize pwrite for the actual file writing.
 *              File flushing is done at file close.
 *
 * Return:      The integer status returned by the internal read_independent
 *              function.  Successful operations will return 0.
 * Errors:      An MPI related error value.
 *
 *-------------------------------------------------------------------------
 */
static int
H5FD__ioc_file_queue_write_indep(sf_work_request_t *msg, int ioc_idx, int source, MPI_Comm comm,
                                 uint32_t counter)
{
    subfiling_context_t *sf_context = NULL;
    MPI_Status           msg_status;
    bool                 send_nack = false;
    int64_t              file_context_id;
    int64_t              data_size;
    int64_t              file_offset;
    int64_t              subfile_idx;
    int64_t              stripe_id;
    haddr_t              sf_eof;
#ifdef H5FD_IOC_COLLECT_STATS
    double t_start;
    double t_end;
    double t_write;
    double t_wait;
    double t_queue_delay;
#endif
    char *recv_buf = NULL;
    int   rcv_tag;
    int   sf_fid;
    int   data_bytes_received;
    int   mpi_code;
    int   ret_value = 0;

    assert(msg);

    file_context_id = msg->context_id;

    /* Retrieve the fields of the RPC message for the write operation */
    data_size   = msg->header[0];
    file_offset = msg->header[1];
    subfile_idx = msg->header[2];

    if (data_size < 0) {
        send_nack = true;
        HGOTO_DONE(FAIL);
    }

    sf_context = H5FD__subfiling_get_object(file_context_id);
    assert(sf_context);

    stripe_id = file_offset + data_size;
    sf_eof    = (haddr_t)(stripe_id % sf_context->sf_stripe_size);

    stripe_id /= sf_context->sf_stripe_size;
    sf_eof += (haddr_t)((stripe_id * sf_context->sf_blocksize_per_stripe) + sf_context->sf_base_addr);

    /* Flag that we've attempted to write data to the file */
    sf_context->sf_write_count++;

#ifdef H5FD_IOC_COLLECT_STATS
    /* For debugging performance */
    sf_write_ops++;

    t_start       = MPI_Wtime();
    t_queue_delay = t_start - msg->start_time;

#ifdef H5_SUBFILING_DEBUG
    H5FD__subfiling_log(file_context_id,
                        "[ioc(%d) %s]: msg from %d: datasize=%ld\toffset=%ld, queue_delay = %lf seconds\n",
                        ioc_idx, __func__, source, data_size, file_offset, t_queue_delay);
#endif

#endif

    /* Allocate space to receive data sent from the client */
    if (NULL == (recv_buf = H5MM_malloc((size_t)data_size))) {
        send_nack = true;
        HGOTO_DONE(FAIL);
    }

    /*
     * Calculate message tag for the client to use for sending
     * data, then send an ACK message to the client with the
     * calculated message tag. This calculated message tag
     * allows us to distinguish between multiple concurrent
     * writes from a single rank.
     */
    assert(H5FD_IOC_tag_ub_val_ptr && (*H5FD_IOC_tag_ub_val_ptr >= IO_TAG_BASE));
    rcv_tag = (int)(counter % (INT_MAX - IO_TAG_BASE));
    rcv_tag %= (*H5FD_IOC_tag_ub_val_ptr - IO_TAG_BASE);
    rcv_tag += IO_TAG_BASE;

    if (H5FD__ioc_send_ack_to_client(rcv_tag, source, WRITE_INDEP_ACK, comm) < 0)
        HGOTO_DONE(FAIL);

    /* Receive data from client */
    H5_CHECK_OVERFLOW(data_size, int64_t, int);
    if (MPI_SUCCESS !=
        (mpi_code = MPI_Recv(recv_buf, (int)data_size, MPI_BYTE, source, rcv_tag, comm, &msg_status)))
        HGOTO_DONE(FAIL);

    if (MPI_SUCCESS != (mpi_code = MPI_Get_count(&msg_status, MPI_BYTE, &data_bytes_received)))
        HGOTO_DONE(FAIL);

    if (data_bytes_received != data_size)
        HGOTO_DONE(FAIL);

#ifdef H5FD_IOC_COLLECT_STATS
    t_end  = MPI_Wtime();
    t_wait = t_end - t_start;
    sf_write_wait_time += t_wait;

    t_start = t_end;

#ifdef H5_SUBFILING_DEBUG
    H5FD__subfiling_log(file_context_id, "[ioc(%d) %s] MPI_Recv(%ld bytes, from = %d) status = %d\n", ioc_idx,
                        __func__, data_size, source, mpi_code);
#endif

#endif

    assert(subfile_idx < sf_context->sf_num_fids);
    sf_fid = sf_context->sf_fids[subfile_idx];

#ifdef H5_SUBFILING_DEBUG
    if (sf_fid < 0)
        H5FD__subfiling_log(file_context_id, "%s: WARNING: attempt to write data to closed subfile FID %d",
                            __func__, sf_fid);
#endif

    if (sf_fid >= 0) {
        /* Actually write data received from client into subfile */
        if (H5FD__ioc_file_write_data(sf_fid, file_offset, recv_buf, data_size, ioc_idx) < 0)
            HGOTO_DONE(FAIL);

#ifdef H5FD_IOC_COLLECT_STATS
        t_end   = MPI_Wtime();
        t_write = t_end - t_start;
        sf_pwrite_time += t_write;
#endif
    }

#ifdef H5FD_IOC_COLLECT_STATS
    sf_queue_delay_time += t_queue_delay;
#endif

    H5FD_ioc_begin_thread_exclusive();

    /* Adjust EOF if necessary */
    if (sf_eof > sf_context->sf_eof)
        sf_context->sf_eof = sf_eof;

    H5FD_ioc_end_thread_exclusive();

    /*
     * Send a message back to the client that the I/O call has
     * completed and it is safe to return from the write call
     */
    if (MPI_SUCCESS != (mpi_code = MPI_Send(&rcv_tag, 1, MPI_INT, source, WRITE_DATA_DONE, comm)))
        HGOTO_DONE(FAIL);

done:
    if (send_nack)
        /* Send NACK back to client so client can handle failure gracefully */
        if (H5FD__ioc_send_nack_to_client(source, WRITE_INDEP_ACK, comm) < 0)
            ret_value = FAIL;

    H5MM_free(recv_buf);

    return ret_value;
} /* H5FD__ioc_file_queue_write_indep() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__ioc_file_queue_read_indep
 *
 * Purpose:     Implement the IOC independent read function.  The
 *              function is invoked as a result of the IOC receiving the
 *              "header"/RPC.  What remains is to allocate memory for
 *              reading the data and then to send this to the client.
 *              We utilize pread for the actual file reading.
 *
 * Return:      The integer status returned by the Internal read_independent
 *              function.  Successful operations will return 0.
 * Errors:      An MPI related error value.
 *
 *-------------------------------------------------------------------------
 */
static int
H5FD__ioc_file_queue_read_indep(sf_work_request_t *msg, int ioc_idx, int source, MPI_Comm comm,
                                uint32_t counter)
{
    subfiling_context_t *sf_context     = NULL;
    bool                 send_empty_buf = true;
    bool                 send_nack      = false;
    bool                 need_data_tag  = false;
    int64_t              file_context_id;
    int64_t              data_size;
    int64_t              file_offset;
    int64_t              subfile_idx;
#ifdef H5FD_IOC_COLLECT_STATS
    double t_start;
    double t_end;
    double t_read;
    double t_queue_delay;
#endif
    char *send_buf = NULL;
    int   send_tag;
    int   sf_fid;
    int   ret_value = 0;

    assert(msg);

    file_context_id = msg->context_id;

    sf_context = H5FD__subfiling_get_object(file_context_id);
    assert(sf_context);

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
    need_data_tag = (sf_context->sf_num_subfiles == 1) ||
                    (sf_context->sf_num_subfiles != sf_context->topology->n_io_concentrators);
    if (!need_data_tag)
        send_tag = READ_INDEP_DATA;

    /* Retrieve the fields of the RPC message for the read operation */
    data_size   = msg->header[0];
    file_offset = msg->header[1];
    subfile_idx = msg->header[2];

    if (data_size < 0) {
        if (need_data_tag) {
            send_nack      = true;
            send_empty_buf = false;
        }
        HGOTO_DONE(FAIL);
    }

    /* Flag that we've attempted to read data from the file */
    sf_context->sf_read_count++;

#ifdef H5FD_IOC_COLLECT_STATS
    /* For debugging performance */
    sf_read_ops++;

    t_start       = MPI_Wtime();
    t_queue_delay = t_start - msg->start_time;

#ifdef H5_SUBFILING_DEBUG
    H5FD__subfiling_log(file_context_id,
                        "[ioc(%d) %s] msg from %d: datasize=%ld\toffset=%ld queue_delay=%lf seconds\n",
                        ioc_idx, __func__, source, data_size, file_offset, t_queue_delay);
#endif

#endif

    /* Allocate space to send data read from file to client */
    if (NULL == (send_buf = H5MM_malloc((size_t)data_size))) {
        if (need_data_tag) {
            send_nack      = true;
            send_empty_buf = false;
        }
        HGOTO_DONE(FAIL);
    }

    if (need_data_tag) {
        /*
         * Calculate message tag for the client to use for receiving
         * data, then send an ACK message to the client with the
         * calculated message tag. This calculated message tag
         * allows us to distinguish between multiple concurrent
         * reads from a single rank, which can happen when a rank
         * owns multiple subfiles.
         */
        assert(H5FD_IOC_tag_ub_val_ptr && (*H5FD_IOC_tag_ub_val_ptr >= IO_TAG_BASE));
        send_tag = (int)(counter % (INT_MAX - IO_TAG_BASE));
        send_tag %= (*H5FD_IOC_tag_ub_val_ptr - IO_TAG_BASE);
        send_tag += IO_TAG_BASE;

        if (H5FD__ioc_send_ack_to_client(send_tag, source, READ_INDEP_ACK, comm) < 0) {
            send_empty_buf = false;
            HGOTO_DONE(FAIL);
        }
    }

    /* Read data from the subfile */
    assert(subfile_idx < sf_context->sf_num_fids);
    sf_fid = sf_context->sf_fids[subfile_idx];
    if (sf_fid < 0)
        HGOTO_DONE(FAIL);

    if (H5FD__ioc_file_read_data(sf_fid, file_offset, send_buf, data_size, ioc_idx) < 0)
        HGOTO_DONE(FAIL);

    send_empty_buf = false;

    /* Send read data to the client */
    H5_CHECK_OVERFLOW(data_size, int64_t, int);
    if (MPI_SUCCESS != MPI_Send(send_buf, (int)data_size, MPI_BYTE, source, send_tag, comm))
        HGOTO_DONE(FAIL);

#ifdef H5FD_IOC_COLLECT_STATS
    t_end  = MPI_Wtime();
    t_read = t_end - t_start;
    sf_pread_time += t_read;
    sf_queue_delay_time += t_queue_delay;

#ifdef H5_SUBFILING_DEBUG
    H5FD__subfiling_log(sf_context->sf_context_id, "[ioc(%d)] MPI_Send to source(%d) completed\n", ioc_idx,
                        source);
#endif

#endif

done:
    if (need_data_tag && send_nack)
        /* Send NACK back to client so client can handle failure gracefully */
        if (H5FD__ioc_send_nack_to_client(source, READ_INDEP_ACK, comm) < 0)
            ret_value = FAIL;
    if (send_empty_buf)
        /*
         * Send an empty message back to client on failure. The client will
         * likely get a message truncation error, but at least shouldn't hang.
         */
        if (MPI_SUCCESS != MPI_Send(NULL, 0, MPI_BYTE, source, send_tag, comm))
            ret_value = FAIL;

    H5MM_free(send_buf);

    return ret_value;
} /* end H5FD__ioc_file_queue_read_indep() */

/*
======================================================
File functions

The pread and pwrite posix functions are described as
being thread safe.
======================================================
*/

static int
H5FD__ioc_file_write_data(int fd, int64_t file_offset, void *data_buffer, int64_t data_size, int ioc_idx)
{
    ssize_t bytes_remaining = (ssize_t)data_size;
    ssize_t bytes_written   = 0;
    char   *this_data       = (char *)data_buffer;
    int     ret_value       = 0;

#ifndef H5FD_IOC_DEBUG
    (void)ioc_idx;
#endif

    HDcompile_assert(H5_SIZEOF_OFF_T == sizeof(file_offset));

    while (bytes_remaining) {
        errno = 0;

        bytes_written = HDpwrite(fd, this_data, (size_t)bytes_remaining, file_offset);

        if (bytes_written >= 0) {
            bytes_remaining -= bytes_written;

#ifdef H5FD_IOC_DEBUG
            printf("[ioc(%d) %s]: wrote %ld bytes, remaining=%ld, file_offset=%" PRId64 "\n", ioc_idx,
                   __func__, bytes_written, bytes_remaining, file_offset);
#endif

            this_data += bytes_written;
            file_offset += bytes_written;
        }
        else
            HGOTO_DONE(FAIL);
    }

    /* We don't usually use this for each file write.  We usually do the file
     * flush as part of file close operation.
     */
#ifdef H5FD_IOC_REQUIRE_FLUSH
    fdatasync(fd);
#endif

done:
    return ret_value;
} /* end H5FD__ioc_file_write_data() */

static int
H5FD__ioc_file_read_data(int fd, int64_t file_offset, void *data_buffer, int64_t data_size, int ioc_idx)
{
    useconds_t delay           = 100;
    ssize_t    bytes_remaining = (ssize_t)data_size;
    ssize_t    bytes_read      = 0;
    char      *this_buffer     = (char *)data_buffer;
    int        retries         = MIN_READ_RETRIES;
    int        ret_value       = 0;

#ifndef H5FD_IOC_DEBUG
    (void)ioc_idx;
#endif

    HDcompile_assert(H5_SIZEOF_OFF_T == sizeof(file_offset));

    while (bytes_remaining) {
        errno = 0;

        bytes_read = HDpread(fd, this_buffer, (size_t)bytes_remaining, file_offset);

        if (bytes_read > 0) {
            /* Reset retry params */
            retries = MIN_READ_RETRIES;
            delay   = 100;

            bytes_remaining -= bytes_read;

#ifdef H5FD_IOC_DEBUG
            printf("[ioc(%d) %s]: read %ld bytes, remaining=%ld, file_offset=%" PRId64 "\n", ioc_idx,
                   __func__, bytes_read, bytes_remaining, file_offset);
#endif

            this_buffer += bytes_read;
            file_offset += bytes_read;
        }
        else if (bytes_read == 0) {
            assert(bytes_remaining > 0);

            /* end of file but not end of format address space */
            memset(this_buffer, 0, (size_t)bytes_remaining);
            break;
        }
        else {
            if (retries == 0) {
#ifdef H5FD_IOC_DEBUG
                printf("[ioc(%d) %s]: TIMEOUT: file_offset=%" PRId64 ", data_size=%" PRId64 "\n", ioc_idx,
                       __func__, file_offset, data_size);
#endif

                HGOTO_DONE(FAIL);
            }

            retries--;
            usleep(delay);
            delay *= 2;
        }
    }

done:
    return ret_value;
} /* end H5FD__ioc_file_read_data() */

static int
H5FD__ioc_file_truncate(sf_work_request_t *msg)
{
    subfiling_context_t *sf_context = NULL;
    int64_t              file_context_id;
    int64_t              length;
    int64_t              subfile_idx;
    int                  fd;
    int                  ioc_idx;
    int                  ret_value = 0;

    assert(msg);

    file_context_id = msg->context_id;
    ioc_idx         = msg->ioc_idx;

    length      = msg->header[0];
    subfile_idx = msg->header[1];

#ifndef H5FD_IOC_DEBUG
    (void)ioc_idx;
#endif

    if (NULL == (sf_context = H5FD__subfiling_get_object(file_context_id)))
        HGOTO_DONE(FAIL);

    assert(subfile_idx < sf_context->sf_num_fids);

    fd = sf_context->sf_fids[subfile_idx];

    if (HDftruncate(fd, (HDoff_t)length) != 0)
        HGOTO_DONE(FAIL);

    /*
     * Send a completion message back to the source that
     * requested the truncation operation
     */
    if (MPI_SUCCESS != MPI_Send(msg->header, 1, H5_subfiling_rpc_msg_type, msg->source, TRUNC_COMPLETED,
                                sf_context->sf_eof_comm))
        HGOTO_DONE(FAIL);

#ifdef H5FD_IOC_DEBUG
    printf("[ioc(%d) %s]: truncated subfile to %lld bytes. ret = %d\n", ioc_idx, __func__, (long long)length,
           errno);
    fflush(stdout);
#endif

done:
    return ret_value;
} /* end H5FD__ioc_file_truncate() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__ioc_file_report_eof
 *
 * Purpose:     Determine the target subfile's eof and report this value
 *              to the requesting rank.
 *
 *              Notes: This function will have to be reworked once we solve
 *                     the IOC error reporting problem.
 *
 *                     This function mixes functionality that should be
 *                     in two different VFDs.
 *
 * Return:      0 if successful, 1 or an MPI error code on failure.
 *
 *-------------------------------------------------------------------------
 */
static int
H5FD__ioc_file_report_eof(sf_work_request_t *msg, MPI_Comm comm)
{
    subfiling_context_t *sf_context = NULL;
    h5_stat_t            sb;
    int64_t              eof_req_reply[3];
    int64_t              file_context_id;
    int64_t              subfile_idx;
    int                  fd;
    int                  source;
    int                  ioc_idx;
    int                  ret_value = 0;

    assert(msg);

    file_context_id = msg->context_id;
    source          = msg->source;
    ioc_idx         = msg->ioc_idx;

    subfile_idx = msg->header[0];

    if (NULL == (sf_context = H5FD__subfiling_get_object(file_context_id)))
        HGOTO_DONE(FAIL);

    assert(subfile_idx < sf_context->sf_num_fids);

    fd = sf_context->sf_fids[subfile_idx];

    memset(&sb, 0, sizeof(h5_stat_t));
    if (HDfstat(fd, &sb) < 0)
        HGOTO_DONE(FAIL);

    eof_req_reply[0] = (int64_t)ioc_idx;
    eof_req_reply[1] = (int64_t)(sb.st_size);
    eof_req_reply[2] = subfile_idx;

#ifdef H5_SUBFILING_DEBUG
    H5FD__subfiling_log(file_context_id, "%s: reporting file EOF as %" PRId64 ".", __func__,
                        eof_req_reply[1]);
#endif

    /* return the subfile EOF to the querying rank */
    if (MPI_SUCCESS != MPI_Send(eof_req_reply, 1, H5_subfiling_rpc_msg_type, source, GET_EOF_COMPLETED, comm))
        HGOTO_DONE(FAIL);

done:
    return ret_value;
} /* H5FD__ioc_file_report_eof() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__ioc_io_queue_alloc_entry
 *
 * Purpose:     Allocate and initialize an instance of
 *              ioc_io_queue_entry_t.  Return pointer to the new
 *              instance on success, and NULL on failure.
 *
 * Return:      Pointer to new instance of ioc_io_queue_entry_t
 *              on success, and NULL on failure.
 *
 *-------------------------------------------------------------------------
 */
static ioc_io_queue_entry_t *
H5FD__ioc_io_queue_alloc_entry(void)
{
    ioc_io_queue_entry_t *q_entry_ptr = NULL;

    q_entry_ptr = (ioc_io_queue_entry_t *)H5MM_calloc(sizeof(ioc_io_queue_entry_t));
    if (q_entry_ptr)
        q_entry_ptr->magic = H5FD_IOC__IO_Q_ENTRY_MAGIC;

    return q_entry_ptr;
} /* H5FD__ioc_io_queue_alloc_entry() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__ioc_io_queue_add_entry
 *
 * Purpose:     Add an I/O request to the tail of the IOC I/O Queue.
 *
 *              To do this, we must:
 *
 *              1) allocate a new instance of ioc_io_queue_entry_t
 *
 *              2) Initialize the new instance and copy the supplied
 *                 instance of sf_work_request_t into it.
 *
 *              3) Append it to the IOC I/O queue.
 *
 *              Note that this does not dispatch the request even if it
 *              is eligible for immediate dispatch.  This is done with
 *              a call to H5FD__ioc_io_queue_dispatch_eligible_entries().
 *
 * Return:      void.
 *
 *-------------------------------------------------------------------------
 */
static void
H5FD__ioc_io_queue_add_entry(ioc_data_t *ioc_data, sf_work_request_t *wk_req_ptr)
{
    ioc_io_queue_entry_t *entry_ptr = NULL;

    assert(ioc_data);
    assert(ioc_data->io_queue.magic == H5FD_IOC__IO_Q_MAGIC);
    assert(wk_req_ptr);

    entry_ptr = H5FD__ioc_io_queue_alloc_entry();

    assert(entry_ptr);
    assert(entry_ptr->magic == H5FD_IOC__IO_Q_ENTRY_MAGIC);

    H5MM_memcpy((void *)(&(entry_ptr->wk_req)), (const void *)wk_req_ptr, sizeof(sf_work_request_t));

    /* must obtain io_queue mutex before appending */
    hg_thread_mutex_lock(&ioc_data->io_queue.q_mutex);

    assert(ioc_data->io_queue.q_len == atomic_load(&ioc_data->sf_io_ops_pending));

    entry_ptr->counter = ioc_data->io_queue.req_counter++;

    ioc_data->io_queue.num_pending++;

    H5FD_IOC__Q_APPEND(&ioc_data->io_queue, entry_ptr);

    atomic_fetch_add(&ioc_data->sf_io_ops_pending, 1);

#ifdef H5_SUBFILING_DEBUG
    H5FD__subfiling_log(
        wk_req_ptr->context_id,
        "%s: request %d queued. op = %d, req = (%lld, %lld, %lld), q-ed/disp/ops_pend = %d/%d/%d.", __func__,
        entry_ptr->counter, (entry_ptr->wk_req.tag), (long long)(entry_ptr->wk_req.header[0]),
        (long long)(entry_ptr->wk_req.header[1]), (long long)(entry_ptr->wk_req.header[2]),
        ioc_data->io_queue.num_pending, ioc_data->io_queue.num_in_progress,
        atomic_load(&ioc_data->sf_io_ops_pending));
#endif

    assert(ioc_data->io_queue.num_pending + ioc_data->io_queue.num_in_progress == ioc_data->io_queue.q_len);

#ifdef H5FD_IOC_COLLECT_STATS
    entry_ptr->q_time = H5_now_usec();

    if (ioc_data->io_queue.q_len > ioc_data->io_queue.max_q_len)
        ioc_data->io_queue.max_q_len = ioc_data->io_queue.q_len;

    if (ioc_data->io_queue.num_pending > ioc_data->io_queue.max_num_pending)
        ioc_data->io_queue.max_num_pending = ioc_data->io_queue.num_pending;

    if (entry_ptr->wk_req.tag == READ_INDEP)
        ioc_data->io_queue.ind_read_requests++;
    else if (entry_ptr->wk_req.tag == WRITE_INDEP)
        ioc_data->io_queue.ind_write_requests++;
    else if (entry_ptr->wk_req.tag == TRUNC_OP)
        ioc_data->io_queue.truncate_requests++;
    else if (entry_ptr->wk_req.tag == GET_EOF_OP)
        ioc_data->io_queue.get_eof_requests++;

    ioc_data->io_queue.requests_queued++;
#endif

#ifdef H5_SUBFILING_DEBUG
    if (ioc_data->io_queue.q_len != atomic_load(&ioc_data->sf_io_ops_pending)) {
        H5FD__subfiling_log(
            wk_req_ptr->context_id,
            "%s: ioc_data->io_queue->q_len = %d != %d = atomic_load(&ioc_data->sf_io_ops_pending).", __func__,
            ioc_data->io_queue.q_len, atomic_load(&ioc_data->sf_io_ops_pending));
    }
#endif

    assert(ioc_data->io_queue.q_len == atomic_load(&ioc_data->sf_io_ops_pending));

    hg_thread_mutex_unlock(&ioc_data->io_queue.q_mutex);

    return;
} /* H5FD__ioc_io_queue_add_entry() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__ioc_io_queue_dispatch_eligible_entries
 *
 * Purpose:     Scan the IOC I/O Queue for dispatchable entries, and
 *              dispatch any such entries found.
 *
 *              Do this by scanning the I/O queue from head to tail for
 *              entries that:
 *
 *              1) Have not already been dispatched
 *
 *              2) Either:
 *
 *                 a) do not intersect with any prior entries on the
 *                    I/O queue, or
 *
 *                 b) Are read requests, and all intersections are with
 *                    prior read requests.
 *
 *              Dispatch any such entries found.
 *
 *              Do this to maintain the POSIX semantics required by
 *              HDF5.
 *
 *              Note that TRUNC_OPs and GET_EOF_OPs are a special case.
 *              Specifically, no I/O queue entry can be dispatched if
 *              there is a truncate or get EOF operation between it and
 *              the head of the queue.  Further, a truncate or get EOF
 *              request cannot be executed unless it is at the head of
 *              the queue.
 *
 * Return:      void.
 *
 *-------------------------------------------------------------------------
 */
/* TODO: Keep an eye on statistics and optimize this algorithm if necessary.  While it is O(N)
 *       where N is the number of elements in the I/O Queue if there are are no-overlaps, it
 *       can become O(N**2) in the worst case.
 */
static void
H5FD__ioc_io_queue_dispatch_eligible_entries(ioc_data_t *ioc_data, bool try_lock)
{
    bool                  conflict_detected;
    int64_t               entry_offset;
    int64_t               entry_len;
    int64_t               scan_offset;
    int64_t               scan_len;
    ioc_io_queue_entry_t *entry_ptr = NULL;
    ioc_io_queue_entry_t *scan_ptr  = NULL;

    assert(ioc_data);
    assert(ioc_data->io_queue.magic == H5FD_IOC__IO_Q_MAGIC);

    if (try_lock) {
        if (hg_thread_mutex_try_lock(&ioc_data->io_queue.q_mutex) < 0)
            return;
    }
    else
        hg_thread_mutex_lock(&ioc_data->io_queue.q_mutex);

    entry_ptr = ioc_data->io_queue.q_head;

    /* sanity check on first element in the I/O queue */
    assert((entry_ptr == NULL) || (entry_ptr->prev == NULL));
    while (entry_ptr && ioc_data->io_queue.num_pending > 0) {
        assert(entry_ptr->magic == H5FD_IOC__IO_Q_ENTRY_MAGIC);

        /* Check for a get EOF or truncate operation at head of queue */
        if (ioc_data->io_queue.q_head->in_progress)
            /* We have a truncate or get eof operation in progress -- thus no other operations
             * can be dispatched until the truncate or get eof operation completes.  Just break
             * out of the loop.
             */
            if (ioc_data->io_queue.q_head->wk_req.tag == TRUNC_OP ||
                ioc_data->io_queue.q_head->wk_req.tag == GET_EOF_OP)
                break;

        if (!entry_ptr->in_progress) {

            entry_offset = entry_ptr->wk_req.header[1];
            entry_len    = entry_ptr->wk_req.header[0];

            conflict_detected = false;

            scan_ptr = entry_ptr->prev;

            assert(scan_ptr == NULL || scan_ptr->magic == H5FD_IOC__IO_Q_ENTRY_MAGIC);
            if (entry_ptr->wk_req.tag == TRUNC_OP || entry_ptr->wk_req.tag == GET_EOF_OP)
                /* the TRUNC_OP or GET_EOF_OP is not at the head of the queue, and thus cannot
                 * be dispatched.  Further, no operation can be dispatched if a truncate request
                 * appears before it in the queue.  Thus we have done all we can and will break
                 * out of the loop.
                 */
                if (scan_ptr != NULL)
                    break;

            while (scan_ptr && !conflict_detected) {
                /* check for overlaps */
                scan_offset = scan_ptr->wk_req.header[1];
                scan_len    = scan_ptr->wk_req.header[0];

                if ((scan_offset + scan_len) > entry_offset && (entry_offset + entry_len) > scan_offset)
                    /* the two request overlap -- unless they are both reads, we have detected a conflict */
                    if (entry_ptr->wk_req.tag != READ_INDEP || scan_ptr->wk_req.tag != READ_INDEP)
                        conflict_detected = true;

                scan_ptr = scan_ptr->prev;
            }

            if (!conflict_detected) { /* dispatch I/O request */
                assert(scan_ptr == NULL);
                assert(!entry_ptr->in_progress);

                entry_ptr->in_progress = true;

                assert(ioc_data->io_queue.num_pending > 0);

                ioc_data->io_queue.num_pending--;
                ioc_data->io_queue.num_in_progress++;

                assert(ioc_data->io_queue.num_pending + ioc_data->io_queue.num_in_progress ==
                       ioc_data->io_queue.q_len);

                entry_ptr->thread_wk.func = H5FD__ioc_handle_work_request;
                entry_ptr->thread_wk.args = entry_ptr;

#ifdef H5_SUBFILING_DEBUG
                H5FD__subfiling_log(
                    entry_ptr->wk_req.context_id,
                    "%s: request %d dispatched. op = %d, req = (%lld, %lld, %lld), "
                    "q-ed/disp/ops_pend = %d/%d/%d.",
                    __func__, entry_ptr->counter, (entry_ptr->wk_req.tag),
                    (long long)(entry_ptr->wk_req.header[0]), (long long)(entry_ptr->wk_req.header[1]),
                    (long long)(entry_ptr->wk_req.header[2]), ioc_data->io_queue.num_pending,
                    ioc_data->io_queue.num_in_progress, atomic_load(&ioc_data->sf_io_ops_pending));
#endif

#ifdef H5FD_IOC_COLLECT_STATS
                if (ioc_data->io_queue.num_in_progress > ioc_data->io_queue.max_num_in_progress)
                    ioc_data->io_queue.max_num_in_progress = ioc_data->io_queue.num_in_progress;

                ioc_data->io_queue.requests_dispatched++;

                entry_ptr->dispatch_time = H5_now_usec();
#endif

                hg_thread_pool_post(ioc_data->io_thread_pool, &(entry_ptr->thread_wk));
            }
        }

        entry_ptr = entry_ptr->next;
    }

    assert(ioc_data->io_queue.q_len == atomic_load(&ioc_data->sf_io_ops_pending));

    hg_thread_mutex_unlock(&ioc_data->io_queue.q_mutex);
} /* H5FD__ioc_io_queue_dispatch_eligible_entries() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__ioc_io_queue_complete_entry
 *
 * Purpose:     Update the IOC I/O Queue for the completion of an I/O
 *              request.
 *
 *              To do this:
 *
 *              1) Remove the entry from the I/O Queue
 *
 *              2) If so configured, update statistics
 *
 *              3) Discard the instance of ioc_io_queue_entry_t.
 *
 * Return:      void.
 *
 *-------------------------------------------------------------------------
 */
static void
H5FD__ioc_io_queue_complete_entry(ioc_data_t *ioc_data, ioc_io_queue_entry_t *entry_ptr)
{
    assert(ioc_data);
    assert(ioc_data->io_queue.magic == H5FD_IOC__IO_Q_MAGIC);
    assert(entry_ptr);
    assert(entry_ptr->magic == H5FD_IOC__IO_Q_ENTRY_MAGIC);

    /* must obtain io_queue mutex before deleting and updating stats */
    hg_thread_mutex_lock(&ioc_data->io_queue.q_mutex);

    assert(ioc_data->io_queue.num_pending + ioc_data->io_queue.num_in_progress == ioc_data->io_queue.q_len);
    assert(ioc_data->io_queue.num_in_progress > 0);

    if (entry_ptr->wk_ret < 0)
        ioc_data->io_queue.num_failed++;

    H5FD_IOC__Q_REMOVE(&ioc_data->io_queue, entry_ptr);

    ioc_data->io_queue.num_in_progress--;

    assert(ioc_data->io_queue.num_pending + ioc_data->io_queue.num_in_progress == ioc_data->io_queue.q_len);

    atomic_fetch_sub(&ioc_data->sf_io_ops_pending, 1);

#ifdef H5_SUBFILING_DEBUG
    H5FD__subfiling_log(entry_ptr->wk_req.context_id,
                        "%s: request %d completed with ret %d. op = %d, req = (%lld, %lld, %lld), "
                        "q-ed/disp/ops_pend = %d/%d/%d.",
                        __func__, entry_ptr->counter, entry_ptr->wk_ret, (entry_ptr->wk_req.tag),
                        (long long)(entry_ptr->wk_req.header[0]), (long long)(entry_ptr->wk_req.header[1]),
                        (long long)(entry_ptr->wk_req.header[2]), ioc_data->io_queue.num_pending,
                        ioc_data->io_queue.num_in_progress, atomic_load(&ioc_data->sf_io_ops_pending));

    /*
     * If this I/O request is a truncate or "get eof" op, make sure
     * there aren't other operations in progress
     */
    if ((entry_ptr->wk_req.tag == GET_EOF_OP) || (entry_ptr->wk_req.tag == TRUNC_OP))
        assert(ioc_data->io_queue.num_in_progress == 0);
#endif

    assert(ioc_data->io_queue.q_len == atomic_load(&ioc_data->sf_io_ops_pending));

#ifdef H5FD_IOC_COLLECT_STATS
    ioc_data->io_queue.requests_completed++;

    entry_ptr->q_time = H5_now_usec();
#endif

    hg_thread_mutex_unlock(&ioc_data->io_queue.q_mutex);

    H5MM_free(entry_ptr);

    return;
} /* H5FD__ioc_io_queue_complete_entry() */
