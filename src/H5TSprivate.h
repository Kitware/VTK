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

/*-------------------------------------------------------------------------
 *
 * Created:     H5TSprivate.h
 *
 * Purpose:     Thread-safety abstractions used by the library
 *
 *-------------------------------------------------------------------------
 */
#ifndef H5TSprivate_H_
#define H5TSprivate_H_

#ifdef H5_HAVE_THREADSAFE

/* Public headers needed by this file */
#ifdef LATER
#include "H5TSpublic.h" /* Public API prototypes */
#endif                  /* LATER */

/**************************/
/* Library Private Macros */
/**************************/

/* Defines */

#ifdef H5_HAVE_WIN_THREADS

/* Scope Definitions (Pthreads only) */
#define H5TS_SCOPE_SYSTEM  0
#define H5TS_SCOPE_PROCESS 0

/* Calling convention (Windows only) */
#define H5TS_CALL_CONV WINAPI

/* Portability function aliases */
#define H5TS_get_thread_local_value(key)        TlsGetValue(key)
#define H5TS_set_thread_local_value(key, value) TlsSetValue(key, value)
#define H5TS_attr_init(attr)                    0
#define H5TS_attr_setscope(attr, scope)         0
#define H5TS_attr_destroy(attr)                 0
#define H5TS_wait_for_thread(thread)            WaitForSingleObject(thread, INFINITE)
#define H5TS_mutex_init(mutex)                  InitializeCriticalSection(mutex)
#define H5TS_mutex_lock_simple(mutex)           EnterCriticalSection(mutex)
#define H5TS_mutex_unlock_simple(mutex)         LeaveCriticalSection(mutex)

/* No Pthreads equivalent - we use a custom H5TS call with that thread library */
#define H5TS_thread_id() ((uint64_t)GetCurrentThreadId())

#else

/* Scope Definitions (Pthreads only) */
#define H5TS_SCOPE_SYSTEM                       PTHREAD_SCOPE_SYSTEM
#define H5TS_SCOPE_PROCESS                      PTHREAD_SCOPE_PROCESS

/* Calling convention (Windows only) */
#define H5TS_CALL_CONV                          /* N/A */

/* Portability function aliases */
#define H5TS_get_thread_local_value(key)        pthread_getspecific(key)
#define H5TS_set_thread_local_value(key, value) pthread_setspecific(key, value)
#define H5TS_attr_init(attr)                    pthread_attr_init((attr))
#define H5TS_attr_setscope(attr, scope)         pthread_attr_setscope(attr, scope)
#define H5TS_attr_destroy(attr)                 pthread_attr_destroy(attr)
#define H5TS_wait_for_thread(thread)            pthread_join(thread, NULL)
#define H5TS_mutex_init(mutex)                  pthread_mutex_init(mutex, NULL)
#define H5TS_mutex_lock_simple(mutex)           pthread_mutex_lock(mutex)
#define H5TS_mutex_unlock_simple(mutex)         pthread_mutex_unlock(mutex)

/* No Win32 thread equivalent - only needed for RW locks which are not supported
 * under Windows threads.
 */
#define H5TS_mutex_destroy(mutex)               pthread_mutex_destroy(mutex)
#define H5TS_cond_init(cond)                    pthread_cond_init(cond, NULL)
#define H5TS_cond_destroy(cond)                 pthread_cond_destroy(cond)
#define H5TS_cond_wait(cond, mutex)             pthread_cond_wait(cond, mutex)
#define H5TS_cond_signal(cond)                  pthread_cond_signal(cond)
#define H5TS_cond_broadcast(cond)               pthread_cond_broadcast(cond)

#endif /* H5_HAVE_WIN_THREADS */

/******************************************************************************
 * Macros to maintain statistics on the Pthreads recursive R/W lock.
 ******************************************************************************/

#ifdef H5_USE_RECURSIVE_WRITER_LOCKS

/* Magic values for struct sanity checking */

/* RW lock */
#define H5TS_RW_LOCK_MAGIC 0XABCD

/* RW lock entry counts */
#define H5TS_RW_ENTRY_COUNT_MAGIC 0XABBA

/* Flag for favoring writers */
/* THIS SHOULD BE AN ENUM */
#define H5TS_RW_LOCK_POLICY_FAVOR_WRITERS 0

#endif /* H5_USE_RECURSIVE_WRITER_LOCKS */

/****************************/
/* Library Private Typedefs */
/****************************/

/* Mutexes, Threads, and Attributes */

#ifdef H5_HAVE_WIN_THREADS

typedef struct H5TS_mutex_struct {
    CRITICAL_SECTION CriticalSection;
} H5TS_mutex_t;

#else

typedef struct H5TS_mutex_struct {
    pthread_t       owner_thread; /* current lock owner */
    pthread_mutex_t atomic_lock;  /* lock for atomicity of new mechanism */
    pthread_cond_t  cond_var;     /* condition variable */
    unsigned int    lock_count;
} H5TS_mutex_t;

#endif /* H5_HAVE_WIN_THREADS */

/* Portability wrappers */

#ifdef H5_HAVE_WIN_THREADS

typedef HANDLE             H5TS_thread_t;
typedef HANDLE             H5TS_attr_t;
typedef CRITICAL_SECTION   H5TS_mutex_simple_t;
typedef DWORD              H5TS_key_t;
typedef INIT_ONCE          H5TS_once_t;
typedef CONDITION_VARIABLE H5TS_cond_t;

#else

typedef pthread_t       H5TS_thread_t;
typedef pthread_attr_t  H5TS_attr_t;
typedef pthread_mutex_t H5TS_mutex_simple_t;
typedef pthread_key_t   H5TS_key_t;
typedef pthread_once_t  H5TS_once_t;
typedef pthread_cond_t  H5TS_cond_t;

#endif /* H5_HAVE_WIN_THREADS */

#ifdef H5_USE_RECURSIVE_WRITER_LOCKS

/******************************************************************************
 *
 * Structure H5TS_rw_lock_stats_t
 *
 * Catchall structure for statistics on the recursive p-threads based
 * recursive R/W lock (see declaration of H5TS_rw_lock_t below).
 *
 * Since the mutex must be held when reading a consistent set of statistics
 * from the recursibe R/W lock, it simplifies matters to bundle them into
 * a single structure.  This structure exists for that purpose.
 *
 * If you modify this structure, be sure to make equivalent changes to
 * the reset_stats initializer in H5TS_rw_lock_reset_stats().
 *
 * Individual fields are discussed below.
 *
 *                                           JRM -- 8/28/20
 *
 * Read lock stats:
 *
 * read_locks_granted: 64 bit integer used to count the total number of read
 *              locks granted.  Note that this includes recursive lock
 *              requests.
 *
 * read_locks_released: 64 bit integer used to count the total number of read
 *              locks released.  Note that this includes recursive lock
 *              release requests.
 *
 * real_read_locks_granted: 64 bit integer used to count the total number of
 *              read locks granted, less any recursive lock requests.
 *
 * real_read_locks_released:  64 bit integer used to count the total number of
 *              read locks released, less any recursive lock releases.
 *
 * max_read_locks; 64 bit integer used to track the maximum number of read
 *              locks active at any point in time.
 *
 * max_read_lock_recursion_depth; 64 bit integer used to track the maximum
 *              recursion depth observed for any read lock.
 *
 * read_locks_delayed: 64 bit integer used to track the number of read locks
 *              that were not granted immediately.
 *
 * max_read_locks_delayed; 64 bit integer used to track the maximum number of
 *              pending read locks at any point in time.
 *
 *
 * Write lock stats:
 *
 * write_locks_granted: 64 bit integer used to count the total number of write
 *              locks granted.  Note that this includes recursive lock
 *              requests.
 *
 * write_locks_released: 64 bit integer used to count the total number of write
 *              locks released.  Note that this includes recursive lock
 *              release requests.
 *
 * real_write_locks_granted: 64 bit integer used to count the total number of
 *              write locks granted, less any recursive lock requests.
 *
 * real_write_locks_released:  64 bit integer used to count the total number of
 *              write locks released, less any recursive lock releases.
 *
 * max_write_locks; 64 bit integer used to track the maximum number of write
 *              locks active at any point in time.  Must be either zero or one.
 *
 * max_write_lock_recursion_depth; 64 bit integer used to track the maximum
 *              recursion depth observed for any write lock.
 *
 * write_locks_delayed: 64 bit integer used to track the number of write locks
 *              that were not granted immediately.
 *
 * max_write_locks_delayed; 64 bit integer used to track the maximum number of
 *              pending write locks at any point in time.
 *
 ******************************************************************************/

typedef struct H5TS_rw_lock_stats_t {

    int64_t read_locks_granted;
    int64_t read_locks_released;
    int64_t real_read_locks_granted;
    int64_t real_read_locks_released;
    int64_t max_read_locks;
    int64_t max_read_lock_recursion_depth;
    int64_t read_locks_delayed;
    int64_t max_read_locks_pending;
    int64_t write_locks_granted;
    int64_t write_locks_released;
    int64_t real_write_locks_granted;
    int64_t real_write_locks_released;
    int64_t max_write_locks;
    int64_t max_write_lock_recursion_depth;
    int64_t write_locks_delayed;
    int64_t max_write_locks_pending;

} H5TS_rw_lock_stats_t;

/******************************************************************************
 *
 * Structure H5TS_rw_lock_t
 *
 * A read / write lock, is a lock that allows either an arbitrary number
 * of readers, or a single writer into a critical region.  A recurssive
 * lock is one that allows a thread that already has a lock (be it read or
 * write) to successfully request the lock again, only dropping the lock
 * when the number of un-lock calls equals the number of lock calls.
 *
 * Note that we can't use the Pthreads or Win32 R/W locks, as while they
 * permit recursive read locks, they disallow recursive write locks.
 *
 * This structure is a catchall for the fields needed to implement a
 * recursive R/W lock that allows recursive write locks, and for the
 * associate statistics collection fields.
 *
 * This recursive R/W lock implementation is an extension of the R/W lock
 * implementation given in "UNIX network programming" Volume 2, Chapter 8
 * by w. Richard Stevens, 2nd edition.
 *
 * Individual fields are discussed below.
 *
 *                                           JRM  -- 8/28/20
 *
 * magic:       Unsigned 32 bit integer field used for sanity checking.  This
 *              fields must always be set to H5TS_RW_LOCK_MAGIC.
 *              If this structure is allocated dynamically, remember to set
 *              it to some invalid value before discarding the structure.
 *
 * policy       Integer containing a code indicating the precidence policy
 *              used by the R/W lock.  The supported policies are listed
 *              below:
 *
 *              H5TS__RW_LOCK_POLICY__FAVOR_WRITERS:
 *
 *              If selected, the R/W lock will grant access to a pending
 *              writer if there are both pending readers and writers.
 *
 *
 *              --- Define other policies here ---
 *
 *
 * mutex:       Mutex used to maintain mutual exclusion on the fields of
 *              of this structure.
 *
 * readers_cv:  Condition variable used for waiting readers.
 *
 * writers_cv:  Condition variable used for waiting writers.
 *
 * waiting_readers_count: 32 bit integer used to maintain a count of
 *              waiting readers.  This value should always be non-negative.
 *
 * waiting_writers_count: 32 bit integer used to maintain a count of
 *              waiting writers.  This value should always be non-negative.
 *
 * The following two fields could be combined into a single field, with
 * the count of active readers being represented by a positive value, and
 * the number of writers by a negative value.  Two fields are used to
 * facilitate sanity checking.
 *
 * active_readers: 32 bit integer used to maintain a count of
 *              readers that currently hold a read lock.  This value
 *              must be zero if active_writers is positive. It should
 *              never be negative.
 *
 * active_writers: 32 bit integer used to maintain a count of
 *              writers that currently hold a write lock.  This value
 *              must always be either 0 or 1, and must be zero if
 *              active_readers is positive.  It should never be negative.
 *
 * rec_entry_count_key: Instance of thread-local key used to maintain
 *              a thread specific lock type and recursive entry count
 *              for all threads holding a lock.
 *
 * stats:       Instance of H5TS_rw_lock_stats_t used to track
 *              statistics on the recursive R/W lock.  See the declaration
 *              of the structure for discussion of its fields.
 *
 *              Note that the stats are gathered into a structure because
 *              we must obtain the mutex when reading the statistics to
 *              avoid changes while the statistics are being read.  Collecting
 *              them into a structure facilitates this.
 *
 ******************************************************************************/

typedef struct H5TS_rw_lock_t {

    uint32_t                    magic;
    int32_t                     policy;
    H5TS_mutex_simple_t         mutex;
    H5TS_cond_t                 readers_cv;
    H5TS_cond_t                 writers_cv;
    int32_t                     waiting_readers_count;
    int32_t                     waiting_writers_count;
    int32_t                     active_readers;
    int32_t                     active_writers;
    H5TS_key_t                  rec_entry_count_key;
    int32_t                     writer_rec_entry_count;
    struct H5TS_rw_lock_stats_t stats;

} H5TS_rw_lock_t;

/******************************************************************************
 *
 * Structure H5TS_rec_entry_count
 *
 * Strucure associated with the reader_rec_entry_count_key defined in
 * H5TS_rw_lock_t.
 *
 * The primary purpose of this structure is to maintain a count of recursive
 * locks so that the lock can be dropped when the count drops to zero.
 *
 * Aditional fields are included for purposes of sanity checking.
 *
 * Individual fields are discussed below.
 *
 *                                           JRM  -- 8/28/20
 *
 * magic:       Unsigned 32 bit integer field used for sanity checking.  This
 *              field must always be set to H5TS_RW_ENTRY_COUNT_MAGIC, and
 *              should be set to some invalid value just before the structure
 *              is freed.
 *
 * write_lock:  Boolean field that is set to TRUE if the count is for a write
 *              lock, and to FALSE if it is for a read lock.
 *
 * rec_lock_count: Count of the number of recursive lock calls, less
 *              the number of recursive unlock calls.  The lock in question
 *              is dropped when the count drops to zero.
 *
 ******************************************************************************/

typedef struct H5TS_rec_entry_count {

    uint32_t magic;
    hbool_t  write_lock;
    int64_t  rec_lock_count;

} H5TS_rec_entry_count;

#endif /* H5_USE_RECURSIVE_WRITER_LOCKS */

/*****************************/
/* Library-private Variables */
/*****************************/

/* Library-scope global variables */

/* Library initialization */
extern H5TS_once_t H5TS_first_init_g;

/* Error stacks */
extern H5TS_key_t H5TS_errstk_key_g;

/* Function stacks */
#ifdef H5_HAVE_CODESTACK
extern H5TS_key_t H5TS_funcstk_key_g;
#endif

/* API contexts */
extern H5TS_key_t H5TS_apictx_key_g;

/***********************************/
/* Private static inline functions */
/***********************************/

#ifdef H5_USE_RECURSIVE_WRITER_LOCKS

static inline void
H5TS_update_stats_rd_lock(H5TS_rw_lock_t *rw_lock, H5TS_rec_entry_count *count)
{
    HDassert(rw_lock);
    HDassert(rw_lock->magic == H5TS_RW_LOCK_MAGIC);
    HDassert(count);
    HDassert(count->magic == H5TS_RW_ENTRY_COUNT_MAGIC);
    HDassert(count->rec_lock_count >= 1);
    HDassert(!count->write_lock);

    rw_lock->stats.read_locks_granted++;

    if (count->rec_lock_count == 1) {

        rw_lock->stats.real_read_locks_granted++;

        if (rw_lock->active_readers > rw_lock->stats.max_read_locks)
            rw_lock->stats.max_read_locks = rw_lock->active_readers;
    }

    if (count->rec_lock_count > rw_lock->stats.max_read_lock_recursion_depth)
        rw_lock->stats.max_read_lock_recursion_depth = count->rec_lock_count;

} /* end H5TS_update_stats_rd_lock() */

static inline void
H5TS_update_stats_rd_lock_delay(H5TS_rw_lock_t *rw_lock, int waiting_count)
{
    HDassert(rw_lock);
    HDassert(rw_lock->magic == H5TS_RW_LOCK_MAGIC);
    HDassert((waiting_count) > 0);

    rw_lock->stats.read_locks_delayed++;

    if (rw_lock->stats.max_read_locks_pending < waiting_count)
        rw_lock->stats.max_read_locks_pending = (waiting_count);

} /* end H5TS_update_stats_rd_lock_delay() */

static inline void
H5TS_update_stats_rd_unlock(H5TS_rw_lock_t *rw_lock, H5TS_rec_entry_count *count)
{
    HDassert(rw_lock);
    HDassert(rw_lock->magic == H5TS_RW_LOCK_MAGIC);
    HDassert(count);
    HDassert(count->magic == H5TS_RW_ENTRY_COUNT_MAGIC);
    HDassert(count->rec_lock_count >= 0);
    HDassert(!count->write_lock);

    rw_lock->stats.read_locks_released++;

    if (count->rec_lock_count == 0)
        rw_lock->stats.real_read_locks_released++;

} /* end H5TS_update_stats_rd_unlock() */

static inline void
H5TS_update_stats_wr_lock(H5TS_rw_lock_t *rw_lock, H5TS_rec_entry_count *count)
{
    HDassert(rw_lock);
    HDassert(rw_lock->magic == H5TS_RW_LOCK_MAGIC);
    HDassert(count);
    HDassert(count->magic == H5TS_RW_ENTRY_COUNT_MAGIC);
    HDassert(count->rec_lock_count >= 1);
    HDassert(count->write_lock);

    rw_lock->stats.write_locks_granted++;

    if (count->rec_lock_count == 1) {

        rw_lock->stats.real_write_locks_granted++;

        if (rw_lock->active_writers > rw_lock->stats.max_write_locks)
            rw_lock->stats.max_write_locks = rw_lock->active_writers;
    }

    if (count->rec_lock_count > rw_lock->stats.max_write_lock_recursion_depth)
        rw_lock->stats.max_write_lock_recursion_depth = count->rec_lock_count;

} /* end H5TS_update_stats_wr_lock() */

static inline void
H5TS_update_stats_wr_lock_delay(H5TS_rw_lock_t *rw_lock, int waiting_count)
{
    HDassert(rw_lock);
    HDassert(rw_lock->magic == H5TS_RW_LOCK_MAGIC);
    HDassert(waiting_count > 0);

    rw_lock->stats.write_locks_delayed++;

    if (rw_lock->stats.max_write_locks_pending < waiting_count)
        rw_lock->stats.max_write_locks_pending = waiting_count;

} /* end H5TS_update_stats_wr_lock_delay() */

static inline void
H5TS_update_stats_wr_unlock(H5TS_rw_lock_t *rw_lock, H5TS_rec_entry_count *count)
{
    HDassert(rw_lock);
    HDassert(rw_lock->magic == H5TS_RW_LOCK_MAGIC);
    HDassert(count);
    HDassert(count->magic == H5TS_RW_ENTRY_COUNT_MAGIC);
    HDassert(count->rec_lock_count >= 0);
    HDassert(count->write_lock);

    rw_lock->stats.write_locks_released++;

    if (count->rec_lock_count == 0)
        rw_lock->stats.real_write_locks_released++;

} /* end H5TS_update_stats_wr_unlock() */
#endif

/***************************************/
/* Library-private Function Prototypes */
/***************************************/

/* Platform-specific functions */
#ifdef H5_HAVE_WIN_THREADS

/* Functions called from DllMain */
H5_DLL BOOL CALLBACK H5TS_win32_process_enter(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *lpContex);
H5_DLL void          H5TS_win32_process_exit(void);
H5_DLL herr_t        H5TS_win32_thread_enter(void);
H5_DLL herr_t        H5TS_win32_thread_exit(void);

#else

H5_DLL uint64_t H5TS_thread_id(void);
H5_DLL void     H5TS_pthread_first_thread_init(void);

#endif /* H5_HAVE_WIN_THREADS */

/* Library-scope routines */
/* (Only used within H5private.h macros) */
H5_DLL herr_t H5TS_mutex_lock(H5TS_mutex_t *mutex);
H5_DLL herr_t H5TS_mutex_unlock(H5TS_mutex_t *mutex);
H5_DLL herr_t H5TS_cancel_count_inc(void);
H5_DLL herr_t H5TS_cancel_count_dec(void);

/* Fully recursive R/W lock related function declarations */
#ifdef H5_USE_RECURSIVE_WRITER_LOCKS
H5_DLL H5TS_rec_entry_count *H5TS_alloc_rec_entry_count(hbool_t write_lock);
H5_DLL void                  H5TS_free_rec_entry_count(void *target);
H5_DLL herr_t                H5TS_rw_lock_init(H5TS_rw_lock_t *rw_lock, int policy);
H5_DLL herr_t                H5TS_rw_lock_destroy(H5TS_rw_lock_t *rw_lock);
H5_DLL herr_t                H5TS_rw_rdlock(H5TS_rw_lock_t *rw_lock);
H5_DLL herr_t                H5TS_rw_wrlock(H5TS_rw_lock_t *rw_lock);
H5_DLL herr_t                H5TS_rw_unlock(H5TS_rw_lock_t *rw_lock);
H5_DLL herr_t                H5TS_rw_lock_get_stats(H5TS_rw_lock_t *rw_lock, H5TS_rw_lock_stats_t *stats);
H5_DLL herr_t                H5TS_rw_lock_reset_stats(H5TS_rw_lock_t *rw_lock);
H5_DLL herr_t                H5TS_rw_lock_print_stats(const char *header_str, H5TS_rw_lock_stats_t *stats);
#endif

/* Testing routines */
H5_DLL H5TS_thread_t H5TS_create_thread(void *(*func)(void *), H5TS_attr_t *attr, void *udata);

#else /* H5_HAVE_THREADSAFE */

/* Non-threadsafe code needs this */
#define H5TS_thread_id() ((uint64_t)0)

#endif /* H5_HAVE_THREADSAFE */

#endif /* H5TSprivate_H_ */
