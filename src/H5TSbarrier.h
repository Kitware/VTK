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
 * Purpose: This file contains support for thread barrier operations, equivalent
 *        to the pthread 'pthread_barrier_t' type and capabilities.
 *
 * Note:  Because this threadsafety framework operates outside the library,
 *        it does not use the error stack (although it does use error macros
 *        that don't push errors on a stack) and only uses the "namecheck only"
 *        FUNC_ENTER_* / FUNC_LEAVE_* macros.
 */

/****************/
/* Module Setup */
/****************/

/***********/
/* Headers */
/***********/

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

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/*--------------------------------------------------------------------------
 * Function:    H5TS_barrier_wait
 *
 * Purpose:     Wait at a barrier.
 *
 * Note:     	Similar to pthread_barrier_wait, a barrier may be reused
 *		multiple times without intervening calls to H5TS_barrier_init.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
static inline herr_t
H5TS_barrier_wait(H5TS_barrier_t *barrier)
{
    if (H5_UNLIKELY(NULL == barrier))
        return FAIL;

#ifdef H5_HAVE_PTHREAD_BARRIER
    {
        int ret = pthread_barrier_wait(barrier);
        if (H5_UNLIKELY(ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD))
            return FAIL;
    }
#else
    {
        const unsigned my_generation = H5TS_atomic_load_uint(&barrier->generation);

        /* When the last thread enters, reset the openings & bump the generation */
        if (1 == H5TS_atomic_fetch_sub_uint(&barrier->openings, 1)) {
            H5TS_atomic_store_uint(&barrier->openings, barrier->count);
            H5TS_atomic_fetch_add_uint(&barrier->generation, 1);
        }
        else {
            /* Not the last thread, wait for the generation to change */
            while (H5TS_atomic_load_uint(&barrier->generation) == my_generation)
                H5TS_thread_yield();
        }
    }
#endif

    return SUCCEED;
} /* end H5TS_barrier_wait() */
