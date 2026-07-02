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
 * Purpose: This file contains support for thread-local 'once' operations,
 *        equivalent to the pthread 'pthread_once_t' type and capabilities.
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
/*-------------------------------------------------------------------------
 * Function: H5TS_once
 *
 * Purpose:  Dynamic package initialization
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_once(H5TS_once_t *once, H5TS_once_init_func_t func)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Sanity check */
    if (H5_UNLIKELY(NULL == once || NULL == func))
        HGOTO_DONE(FAIL);

    /* Invoke the function, only once per process */
    call_once(once, func);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_once() */
#else
#ifdef H5_HAVE_WIN_THREADS
/*-------------------------------------------------------------------------
 * Function: H5TS_once
 *
 * Purpose:  Dynamic package initialization
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_once(H5TS_once_t *once, H5TS_once_init_func_t func)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Sanity check */
    if (H5_UNLIKELY(NULL == once || NULL == func))
        HGOTO_DONE(FAIL);

    /* Invoke the function, only once per process */
    if (H5_UNLIKELY(0 == InitOnceExecuteOnce(once, func, NULL, NULL)))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_once() */
#else
/*-------------------------------------------------------------------------
 * Function: H5TS_once
 *
 * Purpose:  Dynamic package initialization
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_once(H5TS_once_t *once, H5TS_once_init_func_t func)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Sanity check */
    if (H5_UNLIKELY(NULL == once || NULL == func))
        HGOTO_DONE(FAIL);

    /* Invoke the function, only once per process */
    if (H5_UNLIKELY(pthread_once(once, func)))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_once() */
#endif
#endif

#endif /* H5_HAVE_THREADS */
