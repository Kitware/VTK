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
 * Purpose: This file contains support for condition variables, equivalent to
 *        the pthread 'pthread_cond_t' type and capabilities.
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
 * Function: H5TS_cond_init
 *
 * Purpose:  Initialize a H5TS_cond_t (does not allocate it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_cond_init(H5TS_cond_t *cond)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    if (H5_UNLIKELY(cnd_init(cond) != thrd_success))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_cond_init() */

/*-------------------------------------------------------------------------
 * Function: H5TS_cond_destroy
 *
 * Purpose:  Destroy a H5TS_cond_t (does not free it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_cond_destroy(H5TS_cond_t *cond)
{
    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* NOTE: cnd_destroy() can't fail */
    cnd_destroy(cond);

    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(SUCCEED)
} /* end H5TS_cond_destroy() */

#else
#ifdef H5_HAVE_WIN_THREADS
/*-------------------------------------------------------------------------
 * Function: H5TS_cond_init
 *
 * Purpose:  Initialize a H5TS_cond_t (does not allocate it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_cond_init(H5TS_cond_t *cond)
{
    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    InitializeConditionVariable(cond);

    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(SUCCEED)
} /* end H5TS_cond_init() */

/*-------------------------------------------------------------------------
 * Function: H5TS_cond_destroy
 *
 * Purpose:  Destroy a H5TS_cond_t (does not free it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_cond_destroy(H5TS_cond_t *cond)
{
    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Condition variables in Windows are not destroyed */

    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(SUCCEED)
} /* end H5TS_cond_destroy() */
#else
/*-------------------------------------------------------------------------
 * Function: H5TS_cond_init
 *
 * Purpose:  Initialize a H5TS_cond_t (does not allocate it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_cond_init(H5TS_cond_t *cond)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    if (H5_UNLIKELY(pthread_cond_init(cond, NULL)))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_cond_init() */

/*-------------------------------------------------------------------------
 * Function: H5TS_cond_destroy
 *
 * Purpose:  Destroy a H5TS_cond_t (does not free it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_cond_destroy(H5TS_cond_t *cond)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    if (H5_UNLIKELY(pthread_cond_destroy(cond)))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_cond_destroy() */

#endif
#endif

#endif /* H5_HAVE_THREADS */
