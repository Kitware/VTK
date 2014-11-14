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

/*
 * Reference counting buffer algorithms.
 *
 * These are used for various internal buffers which are shared.
 *
 */


#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free lists                           */
#include "H5RCprivate.h"        /* Reference-counted buffers            */

/* Private typedefs & structs */

/* Declare a free list to manage the H5RC_t struct */
H5FL_DEFINE_STATIC(H5RC_t);


/*--------------------------------------------------------------------------
 NAME
    H5RC_create
 PURPOSE
    Create a reference counted object
 USAGE
    H5RC_t *H5RC_create(o,free)
        const void *o;          IN: Object to initialize ref-counted object with
        H5RC_free_func_t free;  IN: Function to call when ref-count drop to zero

 RETURNS
    Returns a pointer to a new ref-counted object on success, NULL on failure.
 DESCRIPTION
    Create a reference counted object.  The object is not duplicated, it is
    assumed to be owned by the reference counted object now and will be freed
    with the 'free' function when the reference count drops to zero.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5RC_t *
H5RC_create(void *o, H5RC_free_func_t free_func)
{
    H5RC_t *ret_value;   /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Sanity check */
    HDassert(o);
    HDassert(free_func);

    /* Allocate ref-counted string structure */
    if(NULL == (ret_value = H5FL_MALLOC(H5RC_t)))
        HGOTO_ERROR(H5E_RS,H5E_NOSPACE,NULL,"memory allocation failed")

    /* Set the internal fields */
    ret_value->o = o;
    ret_value->n = 1;
    ret_value->free_func = free_func;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5RC_create() */


/*--------------------------------------------------------------------------
 NAME
    H5RC_decr
 PURPOSE
    Decrement the reference count for a ref-counted object
 USAGE
    herr_t H5RC_decr(rc)
        H5RC_t *rc;             IN: Ref-counted object to decrement count for

 RETURNS
    SUCCEED/FAIL
 DESCRIPTION
    Decrements the reference count for a ref-counted object, calling the
    object's free function if ref-count drops to zero.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5RC_decr(H5RC_t *rc)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(rc);
    HDassert(rc->o);
    HDassert(rc->n > 0);
    HDassert(rc->free_func);

    /* Decrement reference count */
    rc->n--;

    /* Check if we should delete this object now */
    if(rc->n == 0) {
        if((rc->free_func)(rc->o) < 0) {
            rc = H5FL_FREE(H5RC_t, rc);
            HGOTO_ERROR(H5E_RS, H5E_CANTFREE, FAIL, "memory release failed")
        } /* end if */
        rc = H5FL_FREE(H5RC_t, rc);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5RC_decr() */

