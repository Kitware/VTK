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
 * Private definitions for HDF5 IOC VFD
 */

#ifndef H5FDioc_priv_H
#define H5FDioc_priv_H

/********************/
/* Standard Headers */
/********************/

#include <stdatomic.h>

/**************/
/* H5 Headers */
/**************/

#include "H5private.h"   /* Generic Functions                        */
#include "H5CXprivate.h" /* API Contexts                             */
#include "H5Dprivate.h"  /* Datasets                                 */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5FDioc.h"     /* IOC VFD                                  */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5MMprivate.h" /* Memory management                        */
#include "H5Pprivate.h"  /* Property lists                           */

#include "H5subfiling_common.h"

#include "mercury_thread.h"
#include "mercury_thread_mutex.h"
#include "mercury_thread_pool.h"

/*
 * Some definitions for debugging the IOC VFD
 */

/* #define H5FD_IOC_DEBUG */
/* #define H5FD_IOC_REQUIRE_FLUSH */
/* #define H5FD_IOC_COLLECT_STATS */

/****************************************************************************
 *
 * IOC I/O Queue management macros:
 *
 * The following macros perform the necessary operations on the IOC I/O
 * Queue, which is implemented as a doubly linked list of instances of
 * ioc_io_queue_entry_t.
 *
 * WARNING: q_ptr->q_mutex must be held when these macros are executed..
 *
 * At present, the necessary operations are append (insert an entry at the
 * end of the queue), and delete (remove an entry from the queue).
 *
 * At least initially, all sanity checking is done with asserts, as the
 * the existing I/O concentrator code is not well integrated into the HDF5
 * error reporting system.  This will have to be revisited for a production
 * version, but it should be sufficient for now.
 *
 *                                                 JRM -- 11/2/21
 *
 ****************************************************************************/

#define H5FD_IOC__IO_Q_ENTRY_MAGIC 0x1357

/* clang-format off */

#define H5FD_IOC__Q_APPEND(q_ptr, entry_ptr)                                                      \
do {                                                                                              \
    assert(q_ptr);                                                                              \
    assert((q_ptr)->magic == H5FD_IOC__IO_Q_MAGIC);                                             \
    assert((((q_ptr)->q_len == 0) && ((q_ptr)->q_head == NULL) && ((q_ptr)->q_tail == NULL)) || \
             (((q_ptr)->q_len > 0) && ((q_ptr)->q_head != NULL) && ((q_ptr)->q_tail != NULL)));   \
    assert(entry_ptr);                                                                          \
    assert((entry_ptr)->magic == H5FD_IOC__IO_Q_ENTRY_MAGIC);                                   \
    assert((entry_ptr)->next == NULL);                                                          \
    assert((entry_ptr)->prev == NULL);                                                          \
    assert((entry_ptr)->in_progress == false);                                                  \
                                                                                                  \
    if ( ((q_ptr)->q_head) == NULL )                                                              \
    {                                                                                             \
       ((q_ptr)->q_head) = (entry_ptr);                                                           \
       ((q_ptr)->q_tail) = (entry_ptr);                                                           \
    }                                                                                             \
    else                                                                                          \
    {                                                                                             \
       ((q_ptr)->q_tail)->next = (entry_ptr);                                                     \
       (entry_ptr)->prev = ((q_ptr)->q_tail);                                                     \
       ((q_ptr)->q_tail) = (entry_ptr);                                                           \
    }                                                                                             \
    ((q_ptr)->q_len)++;                                                                           \
} while ( false ) /* H5FD_IOC__Q_APPEND() */

#define H5FD_IOC__Q_REMOVE(q_ptr, entry_ptr)                                                                         \
do {                                                                                                                 \
    assert(q_ptr);                                                                                                 \
    assert((q_ptr)->magic == H5FD_IOC__IO_Q_MAGIC);                                                                \
    assert((((q_ptr)->q_len == 1) && ((q_ptr)->q_head ==((q_ptr)->q_tail)) && ((q_ptr)->q_head == (entry_ptr))) || \
             (((q_ptr)->q_len > 0) && ((q_ptr)->q_head != NULL) && ((q_ptr)->q_tail != NULL)));                      \
    assert(entry_ptr);                                                                                             \
    assert((entry_ptr)->magic == H5FD_IOC__IO_Q_ENTRY_MAGIC);                                                      \
    assert((((q_ptr)->q_len == 1) && ((entry_ptr)->next == NULL) && ((entry_ptr)->prev == NULL)) ||                \
             (((q_ptr)->q_len > 1) && (((entry_ptr)->next != NULL) || ((entry_ptr)->prev != NULL))));                \
    assert((entry_ptr)->in_progress == true);                                                                      \
                                                                                                                     \
    {                                                                                                                \
       if ( (((q_ptr)->q_head)) == (entry_ptr) )                                                                     \
       {                                                                                                             \
          (((q_ptr)->q_head)) = (entry_ptr)->next;                                                                   \
          if ( (((q_ptr)->q_head)) != NULL )                                                                         \
             (((q_ptr)->q_head))->prev = NULL;                                                                       \
       }                                                                                                             \
       else                                                                                                          \
       {                                                                                                             \
          (entry_ptr)->prev->next = (entry_ptr)->next;                                                               \
       }                                                                                                             \
       if (((q_ptr)->q_tail) == (entry_ptr) )                                                                        \
       {                                                                                                             \
          ((q_ptr)->q_tail) = (entry_ptr)->prev;                                                                     \
          if ( ((q_ptr)->q_tail) != NULL )                                                                           \
             ((q_ptr)->q_tail)->next = NULL;                                                                         \
       }                                                                                                             \
       else                                                                                                          \
       {                                                                                                             \
          (entry_ptr)->next->prev = (entry_ptr)->prev;                                                               \
       }                                                                                                             \
       (entry_ptr)->next = NULL;                                                                                     \
       (entry_ptr)->prev = NULL;                                                                                     \
       ((q_ptr)->q_len)--;                                                                                           \
    }                                                                                                                \
} while ( false ) /* H5FD_IOC__Q_REMOVE() */

/* clang-format on */

/****************************************************************************
 *
 * structure ioc_io_queue_entry
 *
 * magic:  Unsigned 32 bit integer always set to H5FD_IOC__IO_Q_ENTRY_MAGIC.
 *         This field is used to validate pointers to instances of
 *         ioc_io_queue_entry_t.
 *
 * next:   Next pointer in the doubly linked list used to implement
 *         the IOC I/O Queue.  This field points to the next entry
 *         in the queue, or NULL if there is no next entry.
 *
 * prev:   Prev pointer in the doubly linked list used to implement
 *         the IOC I/O Queue.  This field points to the previous entry
 *         in the queue, or NULL if there is no previous entry.
 *
 * in_progress: Boolean flag that must be false when the entry is inserted
 *         into the IOC I/O Queue, and set to true when the entry is dispatched
 *         to the worker thread pool for execution.
 *
 *         When in_progress is false, the entry is said to be pending.
 *
 * counter: uint32_t containing a serial number assigned to this IOC
 *         I/O Queue entry.  Note that this will roll over on long
 *         computations, and thus is not in general unique.
 *
 *         The counter fields is used to construct a tag to distinguish
 *         multiple concurrent I/O requests from a give rank, and thus
 *         this should not be a problem as long as there is sufficient
 *         time between roll overs.  As only the lower bits of the counter
 *         are used in tag construction, this is more frequent than the
 *         size of the counter field would suggest -- albeit hopefully
 *         still infrequent enough.
 *
 * wk_req: Instance of sf_work_request_t.  Replace with individual
 *         fields when convenient.
 *
 *
 * Statistics:
 *
 * The following fields are only defined if H5FD_IOC_COLLECT_STATS is true.
 * They are intended to allow collection of basic statistics on the
 * behaviour of the IOC I/O Queue for purposes of debugging and performance
 * optimization.
 *
 * q_time:      uint64_t containing the time the entry was place on the
 *              IOC I/O Queue in usec after the UNIX epoch.
 *
 *              This value is used to compute the queue wait time, and the
 *              total processing time for the entry.
 *
 * dispatch_time:  uint64_t containing the time the entry is dispatched in
 *              usec after the UNIX epoch.  This field is undefined if the
 *              entry is pending.
 *
 *              This value is used to compute the execution time for the
 *              entry.
 *
 ****************************************************************************/

typedef struct ioc_io_queue_entry {

    uint32_t                   magic;
    struct ioc_io_queue_entry *next;
    struct ioc_io_queue_entry *prev;
    bool                       in_progress;
    uint32_t                   counter;

    sf_work_request_t     wk_req;
    struct hg_thread_work thread_wk;
    int                   wk_ret;

    /* statistics */
#ifdef H5FD_IOC_COLLECT_STATS

    uint64_t q_time;
    uint64_t dispatch_time;

#endif

} ioc_io_queue_entry_t;

/****************************************************************************
 *
 * structure ioc_io_queue
 *
 * This is a temporary structure -- its fields should be moved to an I/O
 * concentrator Catchall structure eventually.
 *
 * The fields of this structure support the io queue used to receive and
 * sequence I/O requests for execution by the worker threads.  The rules
 * for sequencing are as follows:
 *
 * 1) Non-overlaping I/O requests must be fed to the worker threads in
 *    the order received, and may execute concurrently
 *
 * 2) Overlapping read requests must be fed to the worker threads in
 *    the order received, but may execute concurrently.
 *
 * 3) If any pair of I/O requests overlap, and at least one is a write
 *    request, they must be executed in strict arrival order, and the
 *    first must complete before the second starts.
 *
 * Due to the strict ordering requirement in rule 3, entries must be
 * inserted at the tail of the queue in receipt order, and retained on
 * the queue until completed.  Entries in the queue are marked pending
 * when inserted on the queue, in progress when handed to a worker
 * thread, and deleted from the queue when completed.
 *
 * The dispatch algorithm is as follows:
 *
 * 1) Set X equal to the element at the head of the queue.
 *
 * 2) If X is pending, and there exists no prior element (i.e. between X
 *    and the head of the queue) that intersects with X, goto 5).
 *
 * 3) If X is pending, X is a read, and all prior intersecting elements
 *    are reads, goto 5).
 *
 * 4) If X is in progress, or if any prior intersecting element is a
 *    write, or if X is a write, set X equal to its successor in the
 *    queue (i.e. the next element further down the queue from the head)
 *    and goto 2)  If there is no next element, exit without dispatching
 *    any I/O request.
 *
 * 5) If we get to 5, X must be pending.  Mark it in progress, and
 *    dispatch it.  If the number of in progress entries is less than
 *    the number of worker threads, and X has a successor in the queue,
 *    set X equal to its predecessor, and goto 2).  Otherwise exit without
 *    dispatching further I/O requests.
 *
 * Note that the above dispatch algorithm doesn't address collective
 * I/O requests -- this should be OK for now, but it will have to
 * addressed prior to production release.
 *
 * On I/O request completion, worker threads must delete their assigned
 * I/O requests from the queue, check to see if there are any pending
 * requests, and trigger the dispatch algorithm if there are.
 *
 * The fields in the structure are discussed individually below.
 *
 * magic:  Unsigned 32 bit integer always set to H5FD_IOC__IO_Q_MAGIC.
 *         This field is used to validate pointers to instances of
 *         H5C_t.
 *
 * q_head: Pointer to the head of the doubly linked list of entries in
 *         the I/O queue.
 *
 *         This field is NULL if the I/O queue is empty.
 *
 * q_tail: Pointer to the tail of the doubly linked list of entries in
 *         the I/O queue.
 *
 *         This field is NULL if the I/O queue is empty.
 *
 * num_pending:  Number of I/O request pending on the I/O queue.
 *
 * num_in_progress: Number of I/O requests in progress on the I/O queue.
 *
 * q_len:  Number of I/O requests on the I/O queue.  Observe that q_len
 *         must equal (num_pending + num_in_progress).
 *
 * req_counter: unsigned 16 bit integer used to provide a "unique" tag for
 *         each I/O request.  This value is incremented by 1, and then
 *         passed to the worker thread where its lower bits are incorporated
 *         into the tag used to disambiguate multiple, concurrent I/O
 *         requests from a single rank.  The value is 32 bits, as MPI tags
 *         are limited to 32 bits.  The value is unsigned as it is expected
 *         to wrap around once its maximum value is reached.
 *
 * q_mutex: Mutex used to ensure that only one thread accesses the IOC I/O
 *         Queue at once.  This mutex must be held to access of modify
 *         all fields of the
 *
 *
 * Statistics:
 *
 * The following fields are only defined if H5FD_IOC_COLLECT_STATS is true.
 * They are intended to allow collection of basic statistics on the
 * behaviour of the IOC I/O Queue for purposes of debugging and performance
 * optimization.
 *
 * max_q_len: Maximum number of requests residing on the IOC I/O Queue at
 *         any point in time in the current run.
 *
 * max_num_pending: Maximum number of pending requests residing on the IOC
 *         I/O Queue at any point in time in the current run.
 *
 * max_num_in_progress: Maximum number of in progress requests residing on
 *         the IOC I/O Queue at any point in time in the current run.
 *
 * ind_read_requests:  Number of independent read requests received by the
 *          IOC to date.
 *
 * ind_write_requests Number of independent write requests received by the
 *          IOC to date.
 *
 * truncate_requests:  Number of truncate requests received by the IOC to
 *           date.
 *
 * get_eof_requests: Number fo get EOF request received by the IO to date.
 *
 * requests_queued: Number of I/O requests received and placed on the IOC
 *          I/O queue.
 *
 * requests_dispatched: Number of I/O requests dispatched for execution by
 *          the worker threads.
 *
 * requests_completed: Number of I/O requests completed by the worker threads.
 *          Observe that on file close, requests_queued, requests_dispatched,
 *          and requests_completed should be equal.
 *
 ****************************************************************************/

#define H5FD_IOC__IO_Q_MAGIC 0x2468

typedef struct ioc_io_queue {

    uint32_t              magic;
    ioc_io_queue_entry_t *q_head;
    ioc_io_queue_entry_t *q_tail;
    int32_t               num_pending;
    int32_t               num_in_progress;
    int32_t               num_failed;
    int32_t               q_len;
    uint32_t              req_counter;
    hg_thread_mutex_t     q_mutex;

    /* statistics */
#ifdef H5FD_IOC_COLLECT_STATS
    int32_t max_q_len;
    int32_t max_num_pending;
    int32_t max_num_in_progress;
    int64_t ind_read_requests;
    int64_t ind_write_requests;
    int64_t truncate_requests;
    int64_t get_eof_requests;
    int64_t requests_queued;
    int64_t requests_dispatched;
    int64_t requests_completed;
#endif

} ioc_io_queue_t;

/*
 * Structure definitions to enable async io completions
 * We first define a structure which contains the basic
 * input arguments for the functions which were originally
 * invoked.  See below.
 */
typedef struct _io_req {
    int         ioc;             /* ID of the IO Concentrator handling this IO.   */
    int64_t     context_id;      /* The context id provided for the read or write */
    int64_t     offset;          /* The file offset for the IO operation          */
    int64_t     elements;        /* How many bytes                                */
    void       *data;            /* A pointer to the (contiguous) data segment    */
    MPI_Request io_transfer_req; /* MPI request for Isend/Irecv of I/O data */
    MPI_Request io_comp_req;     /* MPI request signifying when actual I/O is finished */
    int         io_comp_tag;     /* MPI tag value used for completed I/O request */
} io_req_t;

extern int *H5FD_IOC_tag_ub_val_ptr;

#ifdef __cplusplus
extern "C" {
#endif

H5_DLL herr_t H5FD__ioc_init_threads(void *_sf_context);
H5_DLL herr_t H5FD__ioc_finalize_threads(void *_sf_context);
H5_DLL herr_t H5FD__ioc_write_independent_async(int64_t context_id, int64_t offset, int64_t elements,
                                                const void *data, io_req_t **io_req);
H5_DLL herr_t H5FD__ioc_read_independent_async(int64_t context_id, int64_t offset, int64_t elements,
                                               void *data, io_req_t **io_req);
H5_DLL herr_t H5FD__ioc_async_completion(MPI_Request *mpi_reqs, size_t num_reqs);

#ifdef __cplusplus
}
#endif

#endif /* H5FDioc_priv_H */
