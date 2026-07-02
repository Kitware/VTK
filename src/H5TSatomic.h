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

/***********/
/* Headers */
/***********/

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
 * Function:    H5TS_atomic_load_int
 *
 * Purpose:     Retrieves the value of atomic 'int' variable object.
 *
 * Return:      Value of the atomic 'int'
 *
 *--------------------------------------------------------------------------
 */
static inline int
H5TS_atomic_load_int(H5TS_atomic_int_t *obj)
{
    int ret_value;

    /* Lock mutex that protects the "atomic" value */
    H5TS_mutex_lock(&obj->mutex);

    /* Get the value */
    ret_value = obj->value;

    /* Release the object's mutex */
    H5TS_mutex_unlock(&obj->mutex);

    return ret_value;
} /* end H5TS_atomic_load_int() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_atomic_store_int
 *
 * Purpose:     Atomically replaces the value of the atomic 'int' variable
 *
 * Return:      None
 *
 *--------------------------------------------------------------------------
 */
static inline void
H5TS_atomic_store_int(H5TS_atomic_int_t *obj, int desired)
{
    /* Lock mutex that protects the "atomic" value */
    H5TS_mutex_lock(&obj->mutex);

    /* Set the value */
    obj->value = desired;

    /* Release the object's mutex */
    H5TS_mutex_unlock(&obj->mutex);

    return;
} /* end H5TS_atomic_store_int() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_atomic_fetch_add_int
 *
 * Purpose:     Atomically replaces the value of an atomic 'int' variable with the
 *              result of addition of the 'arg' to the old value of the
 *              atomic variable.
 *
 * Return:      Returns the value of the atomic variable held previously
 *
 *--------------------------------------------------------------------------
 */
static inline int
H5TS_atomic_fetch_add_int(H5TS_atomic_int_t *obj, int arg)
{
    int ret_value;

    /* Lock mutex that protects the "atomic" value */
    H5TS_mutex_lock(&obj->mutex);

    /* Get the current value */
    ret_value = obj->value;

    /* Increment the value */
    obj->value += arg;

    /* Release the object's mutex */
    H5TS_mutex_unlock(&obj->mutex);

    return ret_value;
} /* end H5TS_atomic_fetch_add_int() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_atomic_fetch_sub_int
 *
 * Purpose:     Atomically replaces the value of an atomic 'int' variable with the
 *              result of subtracting the 'arg' from the old value of the
 *              atomic variable.
 *
 * Return:      Returns the value of the atomic variable held previously
 *
 *--------------------------------------------------------------------------
 */
static inline int
H5TS_atomic_fetch_sub_int(H5TS_atomic_int_t *obj, int arg)
{
    int ret_value;

    /* Lock mutex that protects the "atomic" value */
    H5TS_mutex_lock(&obj->mutex);

    /* Get the current value */
    ret_value = obj->value;

    /* Decrement the value */
    obj->value -= arg;

    /* Release the object's mutex */
    H5TS_mutex_unlock(&obj->mutex);

    return ret_value;
} /* end H5TS_atomic_fetch_sub_int() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_atomic_load_uint
 *
 * Purpose:     Retrieves the value of atomic 'unsigned' variable object.
 *
 * Return:      Value of the atomic 'unsigned'
 *
 *--------------------------------------------------------------------------
 */
static inline unsigned
H5TS_atomic_load_uint(H5TS_atomic_uint_t *obj)
{
    unsigned ret_value;

    /* Lock mutex that protects the "atomic" value */
    H5TS_mutex_lock(&obj->mutex);

    /* Get the value */
    ret_value = obj->value;

    /* Release the object's mutex */
    H5TS_mutex_unlock(&obj->mutex);

    return ret_value;
} /* end H5TS_atomic_load_uint() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_atomic_store_uint
 *
 * Purpose:     Atomically replaces the value of the atomic 'unsigned' variable
 *
 * Return:      None
 *
 *--------------------------------------------------------------------------
 */
static inline void
H5TS_atomic_store_uint(H5TS_atomic_uint_t *obj, unsigned desired)
{
    /* Lock mutex that protects the "atomic" value */
    H5TS_mutex_lock(&obj->mutex);

    /* Set the value */
    obj->value = desired;

    /* Release the object's mutex */
    H5TS_mutex_unlock(&obj->mutex);

    return;
} /* end H5TS_atomic_store_uint() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_atomic_fetch_add_uint
 *
 * Purpose:     Atomically replaces the value of an atomic 'unsigned' variable with the
 *              result of addition of the 'arg' to the old value of the
 *              atomic variable.
 *
 * Return:      Returns the value of the atomic variable held previously
 *
 *--------------------------------------------------------------------------
 */
static inline unsigned
H5TS_atomic_fetch_add_uint(H5TS_atomic_uint_t *obj, unsigned arg)
{
    unsigned ret_value;

    /* Lock mutex that protects the "atomic" value */
    H5TS_mutex_lock(&obj->mutex);

    /* Get the current value */
    ret_value = obj->value;

    /* Increment the value */
    obj->value += arg;

    /* Release the object's mutex */
    H5TS_mutex_unlock(&obj->mutex);

    return ret_value;
} /* end H5TS_atomic_fetch_add_uint() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_atomic_fetch_sub_uint
 *
 * Purpose:     Atomically replaces the value of an atomic 'unsigned' variable with the
 *              result of subtracting the 'arg' from the old value of the
 *              atomic variable.
 *
 * Return:      Returns the value of the atomic variable held previously
 *
 *--------------------------------------------------------------------------
 */
static inline unsigned
H5TS_atomic_fetch_sub_uint(H5TS_atomic_uint_t *obj, unsigned arg)
{
    unsigned ret_value;

    /* Lock mutex that protects the "atomic" value */
    H5TS_mutex_lock(&obj->mutex);

    /* Get the current value */
    ret_value = obj->value;

    /* Decrement the value */
    obj->value -= arg;

    /* Release the object's mutex */
    H5TS_mutex_unlock(&obj->mutex);

    return ret_value;
} /* end H5TS_atomic_fetch_sub_uint() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_atomic_exchange_voidp
 *
 * Purpose:     Atomically replaces the value of an atomic 'void *' variable
 *              and returns the value held previously.
 *
 * Return:      Returns the value of the atomic variable held previously
 *
 *--------------------------------------------------------------------------
 */
static inline void *
H5TS_atomic_exchange_voidp(H5TS_atomic_voidp_t *obj, void *desired)
{
    void *ret_value;

    /* Lock mutex that protects the "atomic" value */
    H5TS_mutex_lock(&obj->mutex);

    /* Get the current value */
    ret_value = obj->value;

    /* Set the value */
    obj->value = desired;

    /* Release the object's mutex */
    H5TS_mutex_unlock(&obj->mutex);

    return ret_value;
} /* end H5TS_atomic_exchange_voidp() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_atomic_compare_exchange_strong_voidp
 *
 * Purpose:     Atomically compares the contents of 'obj' with 'expected', and
 *              if those are bitwise equal, replaces the former with 'desired'
 *              (performs read-modify-write operation). Otherwise, loads the
 *              actual contents of 'obj' into '*expected' (performs load
 *              operation).
 *
 * Return:      The result of the comparison: true if 'obj' was equal to
 *              'expected', false otherwise.
 *
 *--------------------------------------------------------------------------
 */
static inline bool
H5TS_atomic_compare_exchange_strong_voidp(H5TS_atomic_voidp_t *obj, void **expected, void *desired)
{
    bool ret_value;

    /* Lock mutex that protects the "atomic" value */
    H5TS_mutex_lock(&obj->mutex);

    /* Compare 'obj' w/'expected' */
    if (obj->value == *expected) {
        obj->value = desired;
        ret_value  = true;
    }
    else {
        *expected = obj->value;
        ret_value = false;
    }
    /* Release the object's mutex */
    H5TS_mutex_unlock(&obj->mutex);

    return ret_value;
} /* end H5TS_atomic_compare_exchange_strong_voidp() */

#endif /* H5_HAVE_STDATOMIC_H */
