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
 * Function:    H5TS_barrier_init
 *
 * Purpose:     Initialize a thread barrier
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_barrier_init(H5TS_barrier_t *barrier, unsigned count)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    if (H5_UNLIKELY(NULL == barrier || 0 == count))
        HGOTO_DONE(FAIL);

#ifdef H5_HAVE_PTHREAD_BARRIER
    /* Initialize the barrier */
    if (H5_UNLIKELY(pthread_barrier_init(barrier, NULL, count)))
        HGOTO_DONE(FAIL);
#else
    /* Initialize fields */
    barrier->count = count;
    H5TS_atomic_init_uint(&barrier->openings, count);
    H5TS_atomic_init_uint(&barrier->generation, 0);
#endif

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_barrier_init() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_barrier_destroy
 *
 * Purpose:     Destroy an H5TS_barrier_t.  All internal components are
 *              destroyed, but the instance of H5TS_barrier_t is not freed.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_barrier_destroy(H5TS_barrier_t *barrier)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    if (H5_UNLIKELY(NULL == barrier))
        HGOTO_DONE(FAIL);

#ifdef H5_HAVE_PTHREAD_BARRIER
    if (H5_UNLIKELY(pthread_barrier_destroy(barrier)))
        HGOTO_DONE(FAIL);
#else
    /* Destroy the (emulated) atomic variables */
    H5TS_atomic_destroy_uint(&barrier->openings);
    H5TS_atomic_destroy_uint(&barrier->generation);
#endif

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_barrier_destroy() */

#endif /* H5_HAVE_THREADS */
