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
 * Purpose: This file contains support for thread-local key operations,
 *        equivalent to the pthread 'pthread_key_t' type and capabilities.
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
 * Function: H5TS_key_create
 *
 * Purpose:  Thread-local key creation
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_key_create(H5TS_key_t *key, H5TS_key_destructor_func_t dtor)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Sanity check */
    if (H5_UNLIKELY(NULL == key))
        HGOTO_DONE(FAIL);

    /* Create the key */
    if (H5_UNLIKELY(tss_create(key, dtor) != thrd_success))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_key_create() */

/*-------------------------------------------------------------------------
 * Function: H5TS_key_delete
 *
 * Purpose:  Thread-local key deletion
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_key_delete(H5TS_key_t key)
{
    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Delete the key */
    /* NOTE: tss_delete() can't fail */
    tss_delete(key);

    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(SUCCEED)
} /* end H5TS_key_delete() */

#else
#ifdef H5_HAVE_WIN_THREADS
/*-------------------------------------------------------------------------
 * Function: H5TS_key_create
 *
 * Purpose:  Thread-local key creation
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_key_create(H5TS_key_t *key, H5TS_key_destructor_func_t dtor)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Sanity check */
    if (H5_UNLIKELY(NULL == key))
        HGOTO_DONE(FAIL);

    /* Fail if the key has a destructor callback, this is not supported by Windows */
    if (NULL != dtor)
        HGOTO_DONE(FAIL);

    /* Create the key */
    if (H5_UNLIKELY(TLS_OUT_OF_INDEXES == (*key = TlsAlloc())))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_key_create() */

/*-------------------------------------------------------------------------
 * Function: H5TS_key_delete
 *
 * Purpose:  Thread-local key deletion
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_key_delete(H5TS_key_t key)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Delete the key */
    if (TLS_OUT_OF_INDEXES != key)
        if (H5_UNLIKELY(0 == TlsFree(key)))
            HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_key_delete() */

#else
/*-------------------------------------------------------------------------
 * Function: H5TS_key_create
 *
 * Purpose:  Thread-local key creation
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_key_create(H5TS_key_t *key, H5TS_key_destructor_func_t dtor)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Sanity check */
    if (H5_UNLIKELY(NULL == key))
        HGOTO_DONE(FAIL);

    /* Create the key */
    if (H5_UNLIKELY(pthread_key_create(key, dtor)))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_key_create() */

/*-------------------------------------------------------------------------
 * Function: H5TS_key_delete
 *
 * Purpose:  Thread-local key deletion
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_key_delete(H5TS_key_t key)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Delete the key */
    if (H5_UNLIKELY(pthread_key_delete(key)))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_key_delete() */

#endif
#endif

#endif /* H5_HAVE_THREADS */
