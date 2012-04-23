/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* private headers */
#include "H5private.h"		/*library                 		*/
#include "H5Eprivate.h"		/*error handling          		*/
#include "H5MMprivate.h"	/*memory management functions		*/

#ifdef H5_HAVE_THREADSAFE

/* Module specific data structures */

/* cancelability structure */
typedef struct H5TS_cancel_struct {
    int previous_state;
    unsigned int cancel_count;
} H5TS_cancel_t;

/* Global variable definitions */
pthread_once_t H5TS_first_init_g = PTHREAD_ONCE_INIT;
pthread_key_t H5TS_errstk_key_g;
pthread_key_t H5TS_funcstk_key_g;
pthread_key_t H5TS_cancel_key_g;
hbool_t H5TS_allow_concurrent_g = FALSE; /* concurrent APIs override this */

/* Local function definitions */
#ifdef NOT_USED
static void H5TS_mutex_init(H5TS_mutex_t *mutex);
#endif /* NOT_USED */


/*--------------------------------------------------------------------------
 * NAME
 *    H5TS_key_destructor
 *
 * USAGE
 *    H5TS_key_destructor()
 *
 * RETURNS
 *
 * DESCRIPTION
 *   Frees the memory for a key.  Called by each thread as it exits.
 *   Currently all the thread-specific information for all keys are simple
 *   structures allocated with malloc, so we can free them all uniformly.
 *
 * PROGRAMMER: Quincey Koziol
 *             February 7, 2003
 *
 * MODIFICATIONS:
 *
 *--------------------------------------------------------------------------
 */
static void
H5TS_key_destructor(void *key_val)
{
    /* Use HDfree here instead of H5MM_xfree(), to avoid calling the H5CS routines */
    if(key_val!=NULL)
        HDfree(key_val);
}

/*--------------------------------------------------------------------------
 * NAME
 *    H5TS_first_thread_init
 *
 * USAGE
 *    H5TS_first_thread_init()
 *
 * RETURNS
 *
 * DESCRIPTION
 *   Initialization of global API lock, keys for per-thread error stacks and
 *   cancallability information. Called by the first thread that enters the
 *   library.
 *
 * PROGRAMMER: Chee Wai LEE
 *             May 2, 2000
 *
 * MODIFICATIONS:
 *
 *--------------------------------------------------------------------------
 */
void
H5TS_first_thread_init(void)
{
    H5_g.H5_libinit_g = FALSE;

    /* initialize global API mutex lock */
    pthread_mutex_init(&H5_g.init_lock.atomic_lock, NULL);
    pthread_cond_init(&H5_g.init_lock.cond_var, NULL);
    H5_g.init_lock.lock_count = 0;

    /* initialize key for thread-specific error stacks */
    pthread_key_create(&H5TS_errstk_key_g, H5TS_key_destructor);

    /* initialize key for thread-specific function stacks */
    pthread_key_create(&H5TS_funcstk_key_g, H5TS_key_destructor);

    /* initialize key for thread cancellability mechanism */
    pthread_key_create(&H5TS_cancel_key_g, H5TS_key_destructor);
}

/*--------------------------------------------------------------------------
 * NAME
 *    H5TS_mutex_lock
 *
 * USAGE
 *    H5TS_mutex_lock(&mutex_var)
 *
 * RETURNS
 *    0 on success and non-zero on error.
 *
 * DESCRIPTION
 *    Recursive lock semantics for HDF5 (locking) -
 *    Multiple acquisition of a lock by a thread is permitted with a
 *    corresponding unlock operation required.
 *
 * PROGRAMMER: Chee Wai LEE
 *             May 2, 2000
 *
 * MODIFICATIONS:
 *
 *    19 May 2000, Bill Wendling
 *    Changed (*foo). form of accessing structure members to the -> form.
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_mutex_lock(H5TS_mutex_t *mutex)
{
    herr_t ret_value;

    ret_value = pthread_mutex_lock(&mutex->atomic_lock);

    if (ret_value)
        return ret_value;

    if(mutex->lock_count && pthread_equal(HDpthread_self(), mutex->owner_thread)) {
        /* already owned by self - increment count */
        mutex->lock_count++;
    } else {
        /* if owned by other thread, wait for condition signal */
        while(mutex->lock_count)
            pthread_cond_wait(&mutex->cond_var, &mutex->atomic_lock);

        /* After we've received the signal, take ownership of the mutex */
        mutex->owner_thread = HDpthread_self();
        mutex->lock_count = 1;
    }

    return pthread_mutex_unlock(&mutex->atomic_lock);
}

/*--------------------------------------------------------------------------
 * NAME
 *    H5TS_mutex_unlock
 *
 * USAGE
 *    H5TS_mutex_unlock(&mutex_var)
 *
 * RETURNS
 *    0 on success and non-zero on error.
 *
 * DESCRIPTION
 *    Recursive lock semantics for HDF5 (unlocking) -
 *    Multiple acquisition of a lock by a thread is permitted with a
 *    corresponding unlock operation required.
 *
 * PROGRAMMER: Chee Wai LEE
 *             May 2, 2000
 *
 * MODIFICATIONS:
 *
 *    19 May 2000, Bill Wendling
 *    Changed (*foo). form of accessing structure members to the -> form.
 *    Also gave the function a return value.
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_mutex_unlock(H5TS_mutex_t *mutex)
{
    herr_t ret_value;           /* Return value */

    ret_value = pthread_mutex_lock(&mutex->atomic_lock);

    if(ret_value)
        return ret_value;

    mutex->lock_count--;

    ret_value = pthread_mutex_unlock(&mutex->atomic_lock);

    if(mutex->lock_count == 0) {
        int err;

        err = pthread_cond_signal(&mutex->cond_var);
        if(err != 0)
            ret_value = err;
    } /* end if */

    return ret_value;
}

/*--------------------------------------------------------------------------
 * NAME
 *    H5TS_cancel_count_inc
 *
 * USAGE
 *    H5TS_cancel_count_inc()
 *
 * RETURNS
 *    0 on success non-zero error code on error.
 *
 * DESCRIPTION
 *    Creates a cancelation counter for a thread if it is the first time
 *    the thread is entering the library.
 *
 *    if counter value is zero, then set cancelability type of the thread
 *    to PTHREAD_CANCEL_DISABLE as thread is entering the library and store
 *    the previous cancelability type into cancelation counter.
 *    Increase the counter value by 1.
 *
 * PROGRAMMER: Chee Wai LEE
 *            May 2, 2000
 *
 * MODIFICATIONS:
 *
 *    19 May 2000, Bill Wendling
 *    Changed function to return a value. Also changed the malloc() call to
 *    the H5MM_malloc() call and checked the returned pointer.
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_cancel_count_inc(void)
{
    H5TS_cancel_t *cancel_counter;
    herr_t ret_value = 0;

    cancel_counter = pthread_getspecific(H5TS_cancel_key_g);

    if (!cancel_counter) {
        /*
	 * First time thread calls library - create new counter and associate
         * with key
         */
	cancel_counter = H5MM_calloc(sizeof(H5TS_cancel_t));

	if (!cancel_counter) {
	    H5E_push_stack(NULL, "H5TS_cancel_count_inc",
		     __FILE__, __LINE__, H5E_ERR_CLS_g, H5E_RESOURCE, H5E_NOSPACE, "memory allocation failed");
	    return FAIL;
	}

        ret_value = pthread_setspecific(H5TS_cancel_key_g,
					(void *)cancel_counter);
    }

    if (cancel_counter->cancel_count == 0)
        /* thread entering library */
        ret_value = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
					   &cancel_counter->previous_state);

    ++cancel_counter->cancel_count;
    return ret_value;
}

/*--------------------------------------------------------------------------
 * NAME
 *    H5TS_cancel_count_dec
 *
 * USAGE
 *    H5TS_cancel_count_dec()
 *
 * RETURNS
 *    0 on success and a non-zero error code on error.
 *
 * DESCRIPTION
 *    If counter value is one, then set cancelability type of the thread
 *    to the previous cancelability type stored in the cancelation counter.
 *    (the thread is leaving the library).
 *
 *    Decrement the counter value by 1.
 *
 * PROGRAMMER: Chee Wai LEE
 *             May 2, 2000
 *
 * MODIFICATIONS:
 *
 *    19 May 2000, Bill Wendling
 *    Changed so that function returns a value. May be of limited usefulness.
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_cancel_count_dec(void)
{
    herr_t ret_value = 0;
    register H5TS_cancel_t *cancel_counter;

    cancel_counter = pthread_getspecific(H5TS_cancel_key_g);

    if (cancel_counter->cancel_count == 1)
        ret_value = pthread_setcancelstate(cancel_counter->previous_state, NULL);

    --cancel_counter->cancel_count;
    return ret_value;
}

#endif	/* H5_HAVE_THREADSAFE */
