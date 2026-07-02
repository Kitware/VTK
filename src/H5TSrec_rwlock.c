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
 * Purpose: This file contains support for recursive R/W locks, equivalent to
 *        the pthread 'pthread_rwlock_t' type and capabilities, except that
 *        threads that hold write access for the lock are allowed to acquire
 *        write access again (and must match each lock with an unlock operation).
 *
 * Note:  Because this threadsafety framework operates outside the library,
 *        it does not use the error stack (although it does use error macros
 *        that don't push errors on a stack) and only uses the "namecheck only"
 *        FUNC_ENTER_* / FUNC_LEAVE_* macros.
 */

/****************/
/* Module Setup */
/****************/

#include "H5TSmodule.h" /* This source code file is part of the H5TS module */

/***********/
/* Headers */
/***********/
#include "H5private.h"  /* Generic Functions                   */
#include "H5Eprivate.h" /* Error handling                      */
#include "H5TSpkg.h"    /* Threadsafety                        */

#ifdef H5_HAVE_THREADS

/****************/
/* Local Macros */
/****************/

/******************/
/* Local Typedefs */
/******************/

/*
 * Count of the number of active [recursive] read lock calls for a given thread.
 * The # of readers for the lock in question is decremented when the recursive
 * read lock count drops to zero.
 */
typedef int64_t H5TS_rec_entry_count_t;

/********************/
/* Local Prototypes */
/********************/

/*********************/
/* Package Variables */
/*********************/

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

#if H5TS_ENABLE_REC_RWLOCK_STATS
/*--------------------------------------------------------------------------
 * Function:    H5TS__update_stats_rdlock
 *
 * Purpose:     Update stats for acquiring a read lock
 *
 * Return:      none
 *
 *--------------------------------------------------------------------------
 */
static void
H5TS__update_stats_rdlock(H5TS_rec_rwlock_t *lock, const H5TS_rec_entry_count_t *count)
{
    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    assert(lock);
    assert(H5TS_REC_RWLOCK_READ == lock->lock_type);
    assert(count);
    assert(*count >= 1);

    lock->stats.read_locks_granted++;

    if (*count == 1) {
        lock->stats.real_read_locks_granted++;
        if (lock->reader_thread_count > lock->stats.max_read_locks)
            lock->stats.max_read_locks = lock->reader_thread_count;
    }

    if (*count > lock->stats.max_read_lock_recursion_depth)
        lock->stats.max_read_lock_recursion_depth = *count;

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* end H5TS__update_stats_rdlock() */

/*--------------------------------------------------------------------------
 * Function:    H5TS__update_stats_rd_lock_delay
 *
 * Purpose:     Update stats for delay in acquiring a read lock
 *
 * Return:      none
 *
 *--------------------------------------------------------------------------
 */
static void
H5TS__update_stats_rd_lock_delay(H5TS_rec_rwlock_t *lock)
{
    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    assert(lock);

    lock->stats.read_locks_delayed++;

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* end H5TS__update_stats_rd_lock_delay() */

/*--------------------------------------------------------------------------
 * Function:    H5TS__update_stats_rd_unlock
 *
 * Purpose:     Update stats for releasing a read lock
 *
 * Return:      none
 *
 *--------------------------------------------------------------------------
 */
static void
H5TS__update_stats_rd_unlock(H5TS_rec_rwlock_t *lock, const H5TS_rec_entry_count_t *count)
{
    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    assert(lock);
    assert(H5TS_REC_RWLOCK_READ == lock->lock_type);
    assert(count);
    assert(*count >= 0);

    lock->stats.read_locks_released++;

    if (*count == 0)
        lock->stats.real_read_locks_released++;

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* end H5TS__update_stats_rd_unlock() */

/*--------------------------------------------------------------------------
 * Function:    H5TS__update_stats_wr_lock
 *
 * Purpose:     Update stats for acquiring a write lock
 *
 * Return:      none
 *
 *--------------------------------------------------------------------------
 */
static void
H5TS__update_stats_wr_lock(H5TS_rec_rwlock_t *lock)
{
    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    assert(lock);
    assert(H5TS_REC_RWLOCK_WRITE == lock->lock_type);
    assert(lock->rec_write_lock_count >= 1);

    lock->stats.write_locks_granted++;

    if (lock->rec_write_lock_count == 1) {
        lock->stats.real_write_locks_granted++;
        if (lock->rec_write_lock_count > lock->stats.max_write_locks)
            lock->stats.max_write_locks = lock->rec_write_lock_count;
    }

    if (lock->rec_write_lock_count > lock->stats.max_write_lock_recursion_depth)
        lock->stats.max_write_lock_recursion_depth = lock->rec_write_lock_count;

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* end H5TS__update_stats_wr_lock() */

/*--------------------------------------------------------------------------
 * Function:    H5TS__update_stats_wr_lock_delay
 *
 * Purpose:     Update stats for delay in acquiring a write lock
 *
 * Return:      none
 *
 *--------------------------------------------------------------------------
 */
static void
H5TS__update_stats_wr_lock_delay(H5TS_rec_rwlock_t *lock)
{
    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    assert(lock);

    lock->stats.write_locks_delayed++;

    if (lock->stats.max_write_locks_pending <= lock->waiting_writers_count)
        lock->stats.max_write_locks_pending = lock->waiting_writers_count + 1;

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* end H5TS__update_stats_wr_lock_delay() */

/*--------------------------------------------------------------------------
 * Function:    H5TS__update_stats_wr_unlock
 *
 * Purpose:     Update stats for releasing a write lock
 *
 * Return:      none
 *
 *--------------------------------------------------------------------------
 */
static void
H5TS__update_stats_wr_unlock(H5TS_rec_rwlock_t *lock)
{
    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    assert(lock);
    assert(H5TS_REC_RWLOCK_WRITE == lock->lock_type);
    assert(lock->rec_write_lock_count >= 0);

    lock->stats.write_locks_released++;

    if (lock->rec_write_lock_count == 0)
        lock->stats.real_write_locks_released++;

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* end H5TS__update_stats_wr_unlock() */

/*--------------------------------------------------------------------------
 * Function:    H5TS__rec_rwlock_get_stats
 *
 * Purpose:     Obtain a copy of the current statistics for a recursive
 *              read / write lock.
 *
 * Note:        To obtain a consistent set of statistics, the function must
 *              obtain the lock mutex.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS__rec_rwlock_get_stats(H5TS_rec_rwlock_t *lock, H5TS_rec_rwlock_stats_t *stats)
{
    bool   have_mutex = false;
    herr_t ret_value  = SUCCEED;

    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    if (H5_UNLIKELY(NULL == lock || NULL == stats))
        HGOTO_DONE(FAIL);

    /* Acquire the mutex */
    if (H5_UNLIKELY(H5TS_mutex_lock(&lock->mutex)))
        HGOTO_DONE(FAIL);
    have_mutex = true;

    /* Copy R/W lock stats */
    *stats = lock->stats;

done:
    if (H5_LIKELY(have_mutex))
        if (H5_UNLIKELY(H5TS_mutex_unlock(&lock->mutex) < 0))
            ret_value = FAIL;

    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS__rec_rwlock_get_stats() */

/*--------------------------------------------------------------------------
 * Function:    H5TS__rec_rwlock_reset_stats
 *
 * Purpose:     Reset the statistics for the supplied recursive read / write
 *              lock.
 *
 * Note:        To obtain a consistent set of statistics, the function must
 *              obtain the lock mutex.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS__rec_rwlock_reset_stats(H5TS_rec_rwlock_t *lock)
{
    bool   have_mutex = false;
    herr_t ret_value  = SUCCEED;

    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    if (H5_UNLIKELY(NULL == lock))
        HGOTO_DONE(FAIL);

    /* Acquire the mutex */
    if (H5_UNLIKELY(H5TS_mutex_lock(&lock->mutex) < 0))
        HGOTO_DONE(FAIL);
    have_mutex = true;

    /* Reset stats */
    memset(&lock->stats, 0, sizeof(lock->stats));

done:
    if (H5_LIKELY(have_mutex))
        if (H5_UNLIKELY(H5TS_mutex_unlock(&lock->mutex) < 0))
            ret_value = FAIL;

    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS__rec_rwlock_reset_stats() */

/*--------------------------------------------------------------------------
 * Function:    H5TS__rec_rwlock_print_stats
 *
 * Purpose:     Print statistics for the supplied recursive R/W lock.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS__rec_rwlock_print_stats(const char *header_str, H5TS_rec_rwlock_stats_t *stats)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    if (H5_UNLIKELY(NULL == header_str || NULL == stats))
        HGOTO_DONE(FAIL);

    fprintf(stdout, "\n\n%s\n\n", header_str);
    fprintf(stdout, "  read_locks_granted             = %" PRId64 "\n", stats->read_locks_granted);
    fprintf(stdout, "  read_locks_released            = %" PRId64 "\n", stats->read_locks_released);
    fprintf(stdout, "  real_read_locks_granted        = %" PRId64 "\n", stats->real_read_locks_granted);
    fprintf(stdout, "  real_read_locks_released       = %" PRId64 "\n", stats->real_read_locks_released);
    fprintf(stdout, "  max_read_locks                 = %" PRId64 "\n", stats->max_read_locks);
    fprintf(stdout, "  max_read_lock_recursion_depth  = %" PRId64 "\n", stats->max_read_lock_recursion_depth);
    fprintf(stdout, "  read_locks_delayed             = %" PRId64 "\n", stats->read_locks_delayed);
    fprintf(stdout, "  write_locks_granted            = %" PRId64 "\n", stats->write_locks_granted);
    fprintf(stdout, "  write_locks_released           = %" PRId64 "\n", stats->write_locks_released);
    fprintf(stdout, "  real_write_locks_granted       = %" PRId64 "\n", stats->real_write_locks_granted);
    fprintf(stdout, "  real_write_locks_released      = %" PRId64 "\n", stats->real_write_locks_released);
    fprintf(stdout, "  max_write_locks                = %" PRId64 "\n", stats->max_write_locks);
    fprintf(stdout, "  max_write_lock_recursion_depth = %" PRId64 "\n",
            stats->max_write_lock_recursion_depth);
    fprintf(stdout, "  write_locks_delayed            = %" PRId64 "\n", stats->write_locks_delayed);
    fprintf(stdout, "  max_write_locks_pending        = %" PRId64 "\n\n", stats->max_write_locks_pending);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS__rec_rwlock_print_stats() */
#endif /* H5TS_ENABLE_REC_RWLOCK_STATS */

/*--------------------------------------------------------------------------
 * Function:    H5TS__rec_rwlock_init
 *
 * Purpose:     Initialize the supplied instance of H5TS_rec_rwlock_t.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS__rec_rwlock_init(H5TS_rec_rwlock_t *lock)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    if (H5_UNLIKELY(NULL == lock))
        HGOTO_DONE(FAIL);

#ifdef H5_HAVE_WIN_THREADS
    /* The current H5TS_rec_rwlock_t implementation uses H5TS_key_create() with a
     * key destructor callback, which is not [currently] supported by Windows.
     */
    HGOTO_DONE(FAIL);
#else
    /* Initialize the lock */
    memset(lock, 0, sizeof(*lock));
    HDcompile_assert(H5TS_REC_RWLOCK_UNUSED == 0);
    if (H5_UNLIKELY(H5TS_mutex_init(&lock->mutex, H5TS_MUTEX_TYPE_PLAIN) < 0))
        HGOTO_DONE(FAIL);
    if (H5_UNLIKELY(H5TS_cond_init(&lock->writers_cv) < 0))
        HGOTO_DONE(FAIL);
    if (H5_UNLIKELY(H5TS_cond_init(&lock->readers_cv) < 0))
        HGOTO_DONE(FAIL);
#endif

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS__rec_rwlock_init() */

/*--------------------------------------------------------------------------
 * Function:    H5TS__rec_rwlock_destroy
 *
 * Purpose:     Take down an instance of H5TS_rec_rwlock_t.  All mutex, condition
 *              variables, and keys are destroyed.  However, the instance of
 *              H5TS_rec_rwlock_t is not freed.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS__rec_rwlock_destroy(H5TS_rec_rwlock_t *lock)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    if (H5_UNLIKELY(NULL == lock))
        HGOTO_DONE(FAIL);

    /* Call the appropriate destroy routines.  We are committed
     * to the destroy at this point, so call them all, even if one fails
     * along the way.
     */
    if (H5_UNLIKELY(H5TS_mutex_destroy(&lock->mutex) < 0))
        ret_value = FAIL;
    if (H5_UNLIKELY(H5TS_cond_destroy(&lock->readers_cv) < 0))
        ret_value = FAIL;
    if (H5_UNLIKELY(H5TS_cond_destroy(&lock->writers_cv) < 0))
        ret_value = FAIL;
    if (lock->is_key_registered)
        if (H5_UNLIKELY(H5TS_key_delete(lock->rec_read_lock_count_key) < 0))
            ret_value = FAIL;

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS__rec_rwlock_destroy() */

/*--------------------------------------------------------------------------
 * Function:    H5TS__rec_rwlock_rdlock
 *
 * Purpose:     Attempt to obtain a read lock on the associated recursive
 *              read / write lock.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS__rec_rwlock_rdlock(H5TS_rec_rwlock_t *lock)
{
    H5TS_rec_entry_count_t *count;
    H5TS_thread_t           my_thread  = H5TS_thread_self();
    bool                    have_mutex = false;
    herr_t                  ret_value  = SUCCEED;

    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    if (H5_UNLIKELY(NULL == lock))
        HGOTO_DONE(FAIL);

    /* Acquire the mutex */
    if (H5_UNLIKELY(H5TS_mutex_lock(&lock->mutex) < 0))
        HGOTO_DONE(FAIL);
    have_mutex = true;

    /* Fail if attempting to acquire a read lock on a thread that holds
     * a write lock
     */
    if (H5_UNLIKELY(H5TS_REC_RWLOCK_WRITE == lock->lock_type &&
                    H5TS_thread_equal(my_thread, lock->write_thread)))
        HGOTO_DONE(FAIL);

    /* If there is no thread-specific data for this thread, set it up */
    if (!lock->is_key_registered) {
        if (H5_UNLIKELY(H5TS_key_create(&lock->rec_read_lock_count_key, free) < 0))
            HGOTO_DONE(FAIL);
        lock->is_key_registered = true;
        count                   = NULL;
    }
    else if (H5_UNLIKELY(H5TS_key_get_value(lock->rec_read_lock_count_key, (void **)&count) < 0))
        HGOTO_DONE(FAIL);
    if (NULL == count) {
        if (H5_UNLIKELY(NULL == (count = calloc(1, sizeof(*count)))))
            HGOTO_DONE(FAIL);
        if (H5_UNLIKELY(H5TS_key_set_value(lock->rec_read_lock_count_key, (void *)count) < 0))
            HGOTO_DONE(FAIL);
    }

    if (*count > 0) { /* This is a recursive lock */
        assert(H5TS_REC_RWLOCK_READ == lock->lock_type);
        assert(lock->reader_thread_count > 0 && lock->rec_write_lock_count == 0);
    }
    else { /* This is an initial read lock request, on this thread */
        /* Readers defer to current or pending writers */
        if (H5TS_REC_RWLOCK_WRITE == lock->lock_type) {
#if H5TS_ENABLE_REC_RWLOCK_STATS
            H5TS__update_stats_rd_lock_delay(lock);
#endif

            do {
                if (H5_UNLIKELY(H5TS_cond_wait(&lock->readers_cv, &lock->mutex) < 0))
                    HGOTO_DONE(FAIL);
            } while (H5TS_REC_RWLOCK_WRITE == lock->lock_type);
        }

        /* Set counter's lock type (which might already be set) & increment
         * number of reader threads
         */
        lock->lock_type = H5TS_REC_RWLOCK_READ;
        lock->reader_thread_count++;
    }

    /* Increment read lock count for this thread */
    (*count)++;
#if H5TS_ENABLE_REC_RWLOCK_STATS
    H5TS__update_stats_rdlock(lock, count);
#endif

done:
    if (H5_LIKELY(have_mutex))
        if (H5_UNLIKELY(H5TS_mutex_unlock(&lock->mutex) < 0))
            ret_value = FAIL;

    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS__rec_rwlock_rdlock() */

/*--------------------------------------------------------------------------
 * Function:    H5TS__rec_rwlock_wrlock
 *
 * Purpose:     Attempt to obtain a write lock on the associated recursive
 *              read / write lock.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS__rec_rwlock_wrlock(H5TS_rec_rwlock_t *lock)
{
    H5TS_thread_t my_thread  = H5TS_thread_self();
    bool          have_mutex = false;
    herr_t        ret_value  = SUCCEED;

    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    if (H5_UNLIKELY(NULL == lock))
        HGOTO_DONE(FAIL);

    /* Acquire the mutex */
    if (H5_UNLIKELY(H5TS_mutex_lock(&lock->mutex) < 0))
        HGOTO_DONE(FAIL);
    have_mutex = true;

    /* Check for initial write lock request on this thread */
    if (H5TS_REC_RWLOCK_WRITE != lock->lock_type || !H5TS_thread_equal(my_thread, lock->write_thread)) {
        /* Fail if attempting to acquire a write lock on a thread that holds
         * a read lock
         */
        if (H5TS_REC_RWLOCK_READ == lock->lock_type) {
            H5TS_rec_entry_count_t *count;

            /* Sanity check */
            assert(lock->is_key_registered);

            /* Fail if read lock count for this thread is > 0 */
            if (H5_UNLIKELY(H5TS_key_get_value(lock->rec_read_lock_count_key, (void **)&count) < 0))
                HGOTO_DONE(FAIL);
            if (H5_UNLIKELY(NULL != count && *count > 0))
                HGOTO_DONE(FAIL);
        }

        /* If lock is already held, wait to acquire it */
        if (H5TS_REC_RWLOCK_UNUSED != lock->lock_type) {
#if H5TS_ENABLE_REC_RWLOCK_STATS
            H5TS__update_stats_wr_lock_delay(lock);
#endif

            do {
                int result;

                lock->waiting_writers_count++;
                result = H5TS_cond_wait(&lock->writers_cv, &lock->mutex);
                lock->waiting_writers_count--;
                if (H5_UNLIKELY(result != 0))
                    HGOTO_DONE(FAIL);
            } while (H5TS_REC_RWLOCK_UNUSED != lock->lock_type);
        }

        /* Set lock type & owner thread */
        lock->lock_type    = H5TS_REC_RWLOCK_WRITE;
        lock->write_thread = my_thread;
    }

    /* Increment write lock count for this thread */
    lock->rec_write_lock_count++;
#if H5TS_ENABLE_REC_RWLOCK_STATS
    H5TS__update_stats_wr_lock(lock);
#endif

done:
    if (H5_LIKELY(have_mutex))
        if (H5_UNLIKELY(H5TS_mutex_unlock(&lock->mutex) < 0))
            ret_value = FAIL;

    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS__rec_rwlock_wrlock() */

/*--------------------------------------------------------------------------
 * Function:    H5TS__rec_rwlock_rdunlock
 *
 * Purpose:     Attempt to unlock a read lock.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS__rec_rwlock_rdunlock(H5TS_rec_rwlock_t *lock)
{
    H5TS_rec_entry_count_t *count;
    bool                    have_mutex = false;
    herr_t                  ret_value  = SUCCEED;

    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    if (H5_UNLIKELY(NULL == lock))
        HGOTO_DONE(FAIL);

    /* Acquire the mutex */
    if (H5_UNLIKELY(H5TS_mutex_lock(&lock->mutex) < 0))
        HGOTO_DONE(FAIL);
    have_mutex = true;

    /* Error check */
    if (H5_UNLIKELY(H5TS_REC_RWLOCK_READ != lock->lock_type))
        HGOTO_DONE(FAIL);

    /* Sanity and error checks */
    assert(lock->is_key_registered);
    assert(lock->reader_thread_count > 0);
    assert(0 == lock->rec_write_lock_count);
    if (H5_UNLIKELY(H5TS_key_get_value(lock->rec_read_lock_count_key, (void **)&count) < 0))
        HGOTO_DONE(FAIL);
    if (H5_UNLIKELY(NULL == count))
        HGOTO_DONE(FAIL);
    assert(*count > 0);

    /* Decrement recursive lock count for this thread */
    (*count)--;
#if H5TS_ENABLE_REC_RWLOCK_STATS
    H5TS__update_stats_rd_unlock(lock, count);
#endif

    /* Check if this thread is releasing its last read lock */
    if (0 == *count) {
        /* Decrement the # of threads with a read lock */
        lock->reader_thread_count--;

        /* Check if lock is unused now */
        if (0 == lock->reader_thread_count) {
            lock->lock_type = H5TS_REC_RWLOCK_UNUSED;

            /* Indicate that lock is unused now */
            /* Prioritize pending writers if there are any */
            if (lock->waiting_writers_count > 0) {
                if (H5_UNLIKELY(H5TS_cond_signal(&lock->writers_cv) < 0))
                    HGOTO_DONE(FAIL);
            }
            else {
                if (H5_UNLIKELY(H5TS_cond_broadcast(&lock->readers_cv) < 0))
                    HGOTO_DONE(FAIL);
            }
        }
    }

done:
    if (H5_LIKELY(have_mutex))
        if (H5_UNLIKELY(H5TS_mutex_unlock(&lock->mutex) < 0))
            ret_value = FAIL;

    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS__rec_rwlock_rdunlock() */

/*--------------------------------------------------------------------------
 * Function:    H5TS__rec_rwlock_wrunlock
 *
 * Purpose:     Attempt to unlock a write lock
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS__rec_rwlock_wrunlock(H5TS_rec_rwlock_t *lock)
{
    bool   have_mutex = false;
    herr_t ret_value  = SUCCEED;

    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    if (H5_UNLIKELY(NULL == lock))
        HGOTO_DONE(FAIL);

    /* Acquire the mutex */
    if (H5_UNLIKELY(H5TS_mutex_lock(&lock->mutex) < 0))
        HGOTO_DONE(FAIL);
    have_mutex = true;

    /* Error check */
    if (H5_UNLIKELY(H5TS_REC_RWLOCK_WRITE != lock->lock_type))
        HGOTO_DONE(FAIL);

    /* Sanity checks */
    assert(0 == lock->reader_thread_count);
    assert(lock->rec_write_lock_count > 0);

    /* Decrement recursive lock count */
    lock->rec_write_lock_count--;
#if H5TS_ENABLE_REC_RWLOCK_STATS
    H5TS__update_stats_wr_unlock(lock);
#endif

    /* Check if lock is unused now */
    if (0 == lock->rec_write_lock_count) {
        lock->lock_type = H5TS_REC_RWLOCK_UNUSED;

        /* Indicate that lock is unused now */
        /* Prioritize pending writers if there are any */
        if (lock->waiting_writers_count > 0) {
            if (H5_UNLIKELY(H5TS_cond_signal(&lock->writers_cv) < 0))
                HGOTO_DONE(FAIL);
        }
        else {
            if (H5_UNLIKELY(H5TS_cond_broadcast(&lock->readers_cv) < 0))
                HGOTO_DONE(FAIL);
        }
    }

done:
    if (H5_LIKELY(have_mutex))
        if (H5_UNLIKELY(H5TS_mutex_unlock(&lock->mutex) < 0))
            ret_value = FAIL;

    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS__rec_rwlock_wrunlock() */

#endif /* H5_HAVE_THREADS */
