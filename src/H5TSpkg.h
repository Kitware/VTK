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

/*
 * Purpose: This file contains declarations which are visible only within
 *          the H5TS package.  Source files outside the H5TS package should
 *          include H5TSprivate.h instead.
 */
#if !(defined H5TS_FRIEND || defined H5TS_MODULE)
#error "Do not include this file outside the H5TS package!"
#endif

#ifndef H5TSpkg_H
#define H5TSpkg_H

#ifdef H5_HAVE_THREADS
/* Get package's private header */
#include "H5TSprivate.h"

/* Other private headers needed by this file */

/**************************/
/* Package Private Macros */
/**************************/

/* Enable statistics for recursive R/W lock when H5TS debugging is enabled */
#ifdef H5TS_DEBUG
#define H5TS_ENABLE_REC_RWLOCK_STATS 1
#else
#define H5TS_ENABLE_REC_RWLOCK_STATS 0
#endif

/****************************/
/* Package Private Typedefs */
/****************************/

#ifdef H5_HAVE_THREADSAFE_API
/* Info for the global API lock */
typedef struct H5TS_api_info_t {
#ifdef H5_HAVE_THREADSAFE
    /* API lock */
    H5TS_mutex_t api_mutex;

    /* Count of recursive API calls by the same thread */
    unsigned lock_count;
#else /* H5_HAVE_CONCURRENCY */
    /* API lock */
    H5TS_rwlock_t api_lock;
#endif

    /* Count of # of attempts to acquire API lock */
    H5TS_atomic_uint_t attempt_lock_count;
} H5TS_api_info_t;
#endif /* H5_HAVE_THREADSAFE_API */

#if H5TS_ENABLE_REC_RWLOCK_STATS
/******************************************************************************
 *
 * Structure H5TS_rec_rwlock_stats_t
 *
 * Statistics for the recursive R/W lock.
 *
 * Since a mutex must be held to read a consistent set of statistics from a
 * recursive R/W lock, it simplifies matters to bundle them into a single
 * structure.
 *
 * Individual fields are:
 *
 * Read lock stats:
 *      read_locks_granted: The total number of read locks granted, including
 *              recursive lock requests.
 *
 *      read_locks_released: The total number of read locks released, including
 *              recursive lock requests.
 *
 *      real_read_locks_granted: The total number of read locks granted, less
 *              any recursive lock requests.
 *
 *      real_read_locks_released:  The total number of read locks released,
 *              less any recursive lock releases.
 *
 *      max_read_locks: The maximum number of read locks active at any point
 *              in time.
 *
 *      max_read_lock_recursion_depth: The maximum recursion depth observed
 *              for any read lock.
 *
 *      read_locks_delayed: The number of read locks not granted immediately.
 *
 *
 * Write lock stats:
 *      write_locks_granted: The total number of write locks granted,
 *              including recursive lock requests.
 *
 *      write_locks_released: The total number of write locks released,
 *              including recursive lock requests.
 *
 *      real_write_locks_granted: The total number of write locks granted,
 *              less any recursive lock requests.
 *
 *      real_write_locks_released: The total number of write locks released,
 *              less any recursive lock requests.
 *
 *      max_write_locks: The maximum number of write locks active at any point
 *              in time.  Must be either zero or one.
 *
 *      max_write_lock_recursion_depth: The maximum recursion depth observed
 *              for any write lock.
 *
 *      write_locks_delayed: The number of write locks not granted immediately.
 *
 *      max_write_locks_pending: The maximum number of pending write locks at
 *              any point in time.
 *
 ******************************************************************************/

typedef struct H5TS_rec_rwlock_stats_t {
    int64_t read_locks_granted;
    int64_t read_locks_released;
    int64_t real_read_locks_granted;
    int64_t real_read_locks_released;
    int64_t max_read_locks;
    int64_t max_read_lock_recursion_depth;
    int64_t read_locks_delayed;
    int64_t write_locks_granted;
    int64_t write_locks_released;
    int64_t real_write_locks_granted;
    int64_t real_write_locks_released;
    int64_t max_write_locks;
    int64_t max_write_lock_recursion_depth;
    int64_t write_locks_delayed;
    int64_t max_write_locks_pending;
} H5TS_rec_rwlock_stats_t;
#endif

/******************************************************************************
 *
 * Structure H5TS_rec_rwlock_t
 *
 * A recursive readers / writer (R/W) lock.
 *
 * This structure holds the fields needed to implement a recursive R/W lock that
 * allows recursive write locks, and the associated statistics collection fields.
 *
 * Note that we can't use the pthreads or Win32 R/W locks: they permit
 * recursive read locks, but disallow recursive write locks.
 *
 * Individual fields are:
 *
 * mutex:       Mutex used to maintain mutual exclusion on the fields of this
 *              structure.
 *
 * lock_type:   Whether the lock is unused, a reader, or a writer.
 *
 * writers_cv:  Condition variable used for waiting writers.
 *
 * write_thread: The thread that owns a write lock, which is recursive for
 *              that thread.
 *
 * rec_write_lock_count: The # of recursive write locks outstanding for the
 *              thread that owns the write lock.
 *
 * waiting_writers_count: The count of waiting writers.
 *
 * readers_cv:  Condition variable used for waiting readers.
 *
 * reader_thread_count: The # of threads holding a read lock.
 *
 * rec_read_lock_count_key: Instance of thread-local key used to maintain a
 *              recursive lock count for each thread holding a read lock.
 *
 * is_key_registered: Flag to track if the read_lock_count_key has been
 *              registered yet for a lock.
 *
 * stats:       Instance of H5TS_rec_rwlock_stats_t used to track statistics
 *              on the lock.
 *
 ******************************************************************************/

typedef enum {
    H5TS_REC_RWLOCK_UNUSED = 0, /* Lock is currently unused */
    H5TS_REC_RWLOCK_WRITE,      /* Lock is a recursive write lock */
    H5TS_REC_RWLOCK_READ        /* Lock is a recursive read lock */
} H5TS_rec_rwlock_type_t;

typedef struct H5TS_rec_rwlock_t {
    /* General fields */
    H5TS_mutex_t           mutex;
    H5TS_rec_rwlock_type_t lock_type;

    /* Writer fields */
    H5TS_cond_t   writers_cv;
    H5TS_thread_t write_thread;
    int32_t       rec_write_lock_count;
    int32_t       waiting_writers_count;

    /* Reader fields */
    H5TS_cond_t readers_cv;
    int32_t     reader_thread_count;
    H5TS_key_t  rec_read_lock_count_key;
    bool        is_key_registered;

#if H5TS_ENABLE_REC_RWLOCK_STATS
    /* Stats */
    H5TS_rec_rwlock_stats_t stats;
#endif
} H5TS_rec_rwlock_t;

/*****************************/
/* Package Private Variables */
/*****************************/

#ifdef H5_HAVE_THREADSAFE_API
/* API threadsafety info */
extern H5TS_api_info_t H5TS_api_info_p;

/* Per-thread info */
extern H5TS_key_t H5TS_thrd_info_key_g;
#endif /* H5_HAVE_THREADSAFE_API */

/******************************/
/* Package Private Prototypes */
/******************************/
#ifdef H5_HAVE_THREADSAFE_API
H5_DLL herr_t H5TS__init_package(void);
H5_DLL herr_t H5TS__api_mutex_acquire(unsigned lock_count, bool *acquired);
H5_DLL herr_t H5TS__api_mutex_release(unsigned *lock_count);
H5_DLL herr_t H5TS__tinfo_init(void);
H5_DLL void   H5TS__tinfo_destroy(void *tinfo_node);
H5_DLL herr_t H5TS__tinfo_term(void);
#endif /* H5_HAVE_THREADSAFE_API */

/* Recursive R/W lock related function declarations */
H5_DLL herr_t H5TS__rec_rwlock_init(H5TS_rec_rwlock_t *lock);
H5_DLL herr_t H5TS__rec_rwlock_rdlock(H5TS_rec_rwlock_t *lock);
H5_DLL herr_t H5TS__rec_rwlock_wrlock(H5TS_rec_rwlock_t *lock);
H5_DLL herr_t H5TS__rec_rwlock_rdunlock(H5TS_rec_rwlock_t *lock);
H5_DLL herr_t H5TS__rec_rwlock_wrunlock(H5TS_rec_rwlock_t *lock);
H5_DLL herr_t H5TS__rec_rwlock_destroy(H5TS_rec_rwlock_t *lock);

/* 'once' callbacks */
#ifdef H5_HAVE_THREADSAFE_API
#ifdef H5_HAVE_C11_THREADS
H5_DLL void H5TS__c11_first_thread_init(void);
#else
#ifdef H5_HAVE_WIN_THREADS
H5_DLL BOOL CALLBACK H5TS__win32_process_enter(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *lpContex);
#else
H5_DLL void H5TS__pthread_first_thread_init(void);
#endif /* H5_HAVE_WIN_THREADS */
#endif
#endif /* H5_HAVE_THREADSAFE_API */

#if H5TS_ENABLE_REC_RWLOCK_STATS
H5_DLL herr_t H5TS__rec_rwlock_get_stats(H5TS_rec_rwlock_t *lock, H5TS_rec_rwlock_stats_t *stats);
H5_DLL herr_t H5TS__rec_rwlock_reset_stats(H5TS_rec_rwlock_t *lock);
H5_DLL herr_t H5TS__rec_rwlock_print_stats(const char *header_str, H5TS_rec_rwlock_stats_t *stats);
#endif

#endif /* H5_HAVE_THREADS */

#endif /* H5TSpkg_H */
