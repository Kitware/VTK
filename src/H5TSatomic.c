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
 * Purpose: C11 atomic emulation outines
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

#ifndef H5_HAVE_STDATOMIC_H

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
 * Function:    H5TS_atomic_init_int
 *
 * Purpose:     Initializes an atomic 'int' variable object with a value.
 *
 * Note:        Per the C11 standard, this function is not atomic and
 *              concurrent execution from multiple threads is a data race.
 *
 * Return:      None
 *
 *--------------------------------------------------------------------------
 */
void
H5TS_atomic_init_int(H5TS_atomic_int_t *obj, int desired)
{
    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Initialize mutex that protects the "atomic" value */
    H5TS_mutex_init(&obj->mutex, H5TS_MUTEX_TYPE_PLAIN);

    /* Set the value */
    obj->value = desired;

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* end H5TS_atomic_init_int() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_atomic_destroy_int
 *
 * Purpose:     Destroys / releases resources for an atomic 'int' variable
 *
 * Note:        No equivalent in the C11 atomics, but needed here, to destroy
 *              the mutex used to protect the atomic value.
 *
 * Return:      None
 *
 *--------------------------------------------------------------------------
 */
void
H5TS_atomic_destroy_int(H5TS_atomic_int_t *obj)
{
    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Destroy mutex that protects the "atomic" value */
    H5TS_mutex_destroy(&obj->mutex);

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* end H5TS_atomic_destroy_int() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_atomic_init_uint
 *
 * Purpose:     Initializes an atomic 'unsigned' variable object with a value.
 *
 * Note:        Per the C11 standard, this function is not atomic and
 *              concurrent execution from multiple threads is a data race.
 *
 * Return:      None
 *
 *--------------------------------------------------------------------------
 */
void
H5TS_atomic_init_uint(H5TS_atomic_uint_t *obj, unsigned desired)
{
    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Initialize mutex that protects the "atomic" value */
    H5TS_mutex_init(&obj->mutex, H5TS_MUTEX_TYPE_PLAIN);

    /* Set the value */
    obj->value = desired;

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* end H5TS_atomic_init_uint() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_atomic_destroy_uint
 *
 * Purpose:     Destroys / releases resources for an atomic 'unsigned' variable
 *
 * Note:        No equivalent in the C11 atomics, but needed here, to destroy
 *              the mutex used to protect the atomic value.
 *
 * Return:      None
 *
 *--------------------------------------------------------------------------
 */
void
H5TS_atomic_destroy_uint(H5TS_atomic_uint_t *obj)
{
    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Destroy mutex that protects the "atomic" value */
    H5TS_mutex_destroy(&obj->mutex);

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* end H5TS_atomic_destroy_uint() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_atomic_init_voidp
 *
 * Purpose:     Initializes an atomic 'void *' variable object with a value.
 *
 * Note:        Per the C11 standard, this function is not atomic and
 *              concurrent execution from multiple threads is a data race.
 *
 * Return:      None
 *
 *--------------------------------------------------------------------------
 */
void
H5TS_atomic_init_voidp(H5TS_atomic_voidp_t *obj, void *desired)
{
    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Initialize mutex that protects the "atomic" value */
    H5TS_mutex_init(&obj->mutex, H5TS_MUTEX_TYPE_PLAIN);

    /* Set the value */
    obj->value = desired;

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* end H5TS_atomic_init_voidp() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_atomic_destroy_voidp
 *
 * Purpose:     Destroys / releases resources for an atomic 'void *' variable
 *
 * Note:        No equivalent in the C11 atomics, but needed here, to destroy
 *              the mutex used to protect the atomic value.
 *
 * Return:      None
 *
 *--------------------------------------------------------------------------
 */
void
H5TS_atomic_destroy_voidp(H5TS_atomic_voidp_t *obj)
{
    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Destroy mutex that protects the "atomic" value */
    H5TS_mutex_destroy(&obj->mutex);

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* end H5TS_atomic_destroy_voidp() */

#endif /* H5_HAVE_STDATOMIC_H */

#endif /* H5_HAVE_THREADS */
