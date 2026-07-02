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

#ifdef H5_HAVE_C11_THREADS
#define H5TS_key_set_value(key, value) (H5_UNLIKELY(tss_set((key), (value)) != thrd_success) ? FAIL : SUCCEED)
#define H5TS_key_get_value(key, value) (*(value) = tss_get(key), SUCCEED)

#else
#ifdef H5_HAVE_WIN_THREADS
/*-------------------------------------------------------------------------
 * Function: H5TS_key_set_value
 *
 * Purpose:  Set a thread-specific value for a thread-local key
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static inline herr_t
H5TS_key_set_value(H5TS_key_t key, void *value)
{
    /* Set the value for this thread */
    if (H5_UNLIKELY(0 == TlsSetValue(key, (LPVOID)value)))
        return FAIL;

    return SUCCEED;
} /* end H5TS_key_set_value() */

/*-------------------------------------------------------------------------
 * Function: H5TS_key_get_value
 *
 * Purpose:  Get a thread-specific value for a thread-local key
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static inline herr_t
H5TS_key_get_value(H5TS_key_t key, void **value)
{
    /* Get the value for this thread */
    if (H5_UNLIKELY(NULL == (*value = TlsGetValue(key))))
        /* Check for possible error, when NULL value is returned */
        if (H5_UNLIKELY(ERROR_SUCCESS != GetLastError()))
            return FAIL;

    return SUCCEED;
} /* end H5TS_key_get_value() */

#else

#define H5TS_key_set_value(key, value) (H5_UNLIKELY(pthread_setspecific((key), (value))) ? FAIL : SUCCEED)
#define H5TS_key_get_value(key, value) (*(value) = pthread_getspecific(key), SUCCEED)

#endif
#endif
