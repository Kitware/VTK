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
 * Purpose: This file contains support for thread operations, equivalent to
 *        the pthread 'pthread_t' type and capabilities.
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

#ifdef H5_HAVE_C11_THREADS
/*--------------------------------------------------------------------------
 * Function: H5TS_thread_create
 *
 * Purpose:  Spawn off a new thread calling function 'func' with input 'udata'.
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_thread_create(H5TS_thread_t *thread, H5TS_thread_start_func_t func, void *udata)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    if (H5_UNLIKELY(thrd_create(thread, func, udata) != thrd_success))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* H5TS_thread_create() */

/*--------------------------------------------------------------------------
 * Function: H5TS_thread_join
 *
 * Purpose:  Wait for thread termination
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_thread_join(H5TS_thread_t thread, H5TS_thread_ret_t *ret_val)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    if (H5_UNLIKELY(thrd_join(thread, ret_val) != thrd_success))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* H5TS_thread_join() */

/*--------------------------------------------------------------------------
 * Function: H5TS_thread_detach
 *
 * Purpose:  Detach a thread
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_thread_detach(H5TS_thread_t thread)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    if (H5_UNLIKELY(thrd_detach(thread) != thrd_success))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* H5TS_thread_detach() */

/*--------------------------------------------------------------------------
 * Function: H5TS_thread_yield
 *
 * Purpose:  Yield thread execution
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
void
H5TS_thread_yield(void)
{
    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    thrd_yield();

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* H5TS_thread_yield() */
#else
#ifdef H5_HAVE_WIN_THREADS
/*--------------------------------------------------------------------------
 * Function: H5TS_thread_create
 *
 * Purpose:  Spawn off a new thread calling function 'func' with input 'udata'.
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_thread_create(H5TS_thread_t *thread, H5TS_thread_start_func_t func, void *udata)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* When calling C runtime functions, you should use _beginthread or
     * _beginthreadex instead of CreateThread.  Threads created with
     * CreateThread risk being killed in low-memory situations, but
     * we'll use the easier-to-deal-with CreateThread for now.
     *
     * NOTE: _beginthread() auto-recycles its handle when execution completes
     *       so you can't wait on it, making it unsuitable for the existing
     *       test code.
     */
    if (H5_UNLIKELY(NULL == (*thread = CreateThread(NULL, 0, func, udata, 0, NULL))))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* H5TS_thread_create() */

/*--------------------------------------------------------------------------
 * Function: H5TS_thread_join
 *
 * Purpose:  Wait for thread termination
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_thread_join(H5TS_thread_t thread, H5TS_thread_ret_t *ret_val)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    if (H5_UNLIKELY(WAIT_OBJECT_0 != WaitForSingleObject(thread, INFINITE)))
        HGOTO_DONE(FAIL);
    if (ret_val)
        if (H5_UNLIKELY(0 == GetExitCodeThread(thread, ret_val)))
            HGOTO_DONE(FAIL);
    if (H5_UNLIKELY(0 == CloseHandle(thread)))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* H5TS_thread_join() */

/*--------------------------------------------------------------------------
 * Function: H5TS_thread_detach
 *
 * Purpose:  Detach a thread
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_thread_detach(H5TS_thread_t thread)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    if (H5_UNLIKELY(0 == CloseHandle(thread)))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* H5TS_thread_detach() */

/*--------------------------------------------------------------------------
 * Function: H5TS_thread_yield
 *
 * Purpose:  Yield thread execution
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
void
H5TS_thread_yield(void)
{
    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    SwitchToThread();

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* H5TS_thread_yield() */
#else
/*--------------------------------------------------------------------------
 * Function: H5TS_thread_create
 *
 * Purpose:  Spawn off a new thread calling function 'func' with input 'udata'.
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_thread_create(H5TS_thread_t *thread, H5TS_thread_start_func_t func, void *udata)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    if (H5_UNLIKELY(pthread_create(thread, NULL, func, udata)))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* H5TS_thread_create() */

/*--------------------------------------------------------------------------
 * Function: H5TS_thread_join
 *
 * Purpose:  Wait for thread termination
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_thread_join(H5TS_thread_t thread, H5TS_thread_ret_t *ret_val)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    if (H5_UNLIKELY(pthread_join(thread, ret_val)))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* H5TS_thread_join() */

/*--------------------------------------------------------------------------
 * Function: H5TS_thread_detach
 *
 * Purpose:  Detach a thread
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_thread_detach(H5TS_thread_t thread)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    if (H5_UNLIKELY(pthread_detach(thread)))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* H5TS_thread_detach() */

/*--------------------------------------------------------------------------
 * Function: H5TS_thread_yield
 *
 * Purpose:  Yield thread execution
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
void
H5TS_thread_yield(void)
{
    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    sched_yield();

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* H5TS_thread_yield() */
#endif
#endif
#endif /* H5_HAVE_THREADS */
