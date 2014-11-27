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

#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5F_init_fake_interface


/* Packages needed by this file... */
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"             /* File access				*/

/* PRIVATE PROTOTYPES */


/*--------------------------------------------------------------------------
NAME
   H5F_init_fake_interface -- Initialize interface-specific information
USAGE
    herr_t H5F_init_fake_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5F_init() currently).

--------------------------------------------------------------------------*/
static herr_t
H5F_init_fake_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    FUNC_LEAVE_NOAPI(H5F_init())
} /* H5F_init_fake_interface() */


/*-------------------------------------------------------------------------
 * Function:    H5F_fake_alloc
 *
 * Purpose:     Allocate a "fake" file structure, for various routines to
 *              use for encoding/decoding data structures using internal API
 *              routines that need a file structure, but don't ultimately
 *              depend on having a "real" file.
 *
 * Return:      Success:        Pointer to 'faked up' file structure
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              koziol@hdfgroup.org
 *              Oct  2, 2006
 *
 *-------------------------------------------------------------------------
 */
H5F_t *
H5F_fake_alloc(uint8_t sizeof_size)
{
    H5F_t *f = NULL;            /* Pointer to fake file struct */
    H5F_t *ret_value;           /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Allocate faked file struct */
    if(NULL == (f = H5FL_CALLOC(H5F_t)))
	HGOTO_ERROR(H5E_FILE, H5E_NOSPACE, NULL, "can't allocate top file structure")
    if(NULL == (f->shared = H5FL_CALLOC(H5F_file_t)))
	HGOTO_ERROR(H5E_FILE, H5E_NOSPACE, NULL, "can't allocate shared file structure")

    /* Only set fields necessary for clients */
    if(sizeof_size == 0)
        f->shared->sizeof_size = H5F_OBJ_SIZE_SIZE;
    else
        f->shared->sizeof_size = sizeof_size;

    /* Set return value */
    ret_value = f;

done:
    if(!ret_value)
        H5F_fake_free(f);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_fake_alloc() */


/*-------------------------------------------------------------------------
 * Function:    H5F_fake_free
 *
 * Purpose:     Free a "fake" file structure.
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:  Quincey Koziol
 *              koziol@hdfgroup.org
 *              Oct  2, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_fake_free(H5F_t *f)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Free faked file struct */
    if(f) {
        /* Destroy shared file struct */
        if(f->shared)
            f->shared = H5FL_FREE(H5F_file_t, f->shared);
        f = H5FL_FREE(H5F_t, f);
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5F_fake_free() */

