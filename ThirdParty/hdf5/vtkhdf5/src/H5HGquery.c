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
 * Programmer:  Quincey Koziol <koziol@hdfgroup.org>
 *              Wednesday, July 20, 2011
 *
 * Purpose:	Query routines for global heaps.
 *
 */

/****************/
/* Module Setup */
/****************/

#define H5HG_PACKAGE		/*suppress error about including H5HGpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5HGpkg.h"		/* Global heaps				*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


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



/*-------------------------------------------------------------------------
 * Function:	H5HG_get_addr
 *
 * Purpose:	Query the address of a global heap object.
 *
 * Return:	Address of heap on success/abort on failure (shouldn't fail)
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, July 20, 2011
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5HG_get_addr(const H5HG_heap_t *heap)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(heap);

    FUNC_LEAVE_NOAPI(heap->addr)
} /* H5HG_get_addr() */


/*-------------------------------------------------------------------------
 * Function:	H5HG_get_size
 *
 * Purpose:	Query the size of a global heap object.
 *
 * Return:	Size of heap on success/abort on failure (shouldn't fail)
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, July 20, 2011
 *
 *-------------------------------------------------------------------------
 */
size_t
H5HG_get_size(const H5HG_heap_t *heap)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(heap);

    FUNC_LEAVE_NOAPI(heap->size)
} /* H5HG_get_size() */


/*-------------------------------------------------------------------------
 * Function:	H5HG_get_free_size
 *
 * Purpose:	Query the free size of a global heap object.
 *
 * Return:	Free size of heap on success/abort on failure (shouldn't fail)
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, July 20, 2011
 *
 *-------------------------------------------------------------------------
 */
size_t
H5HG_get_free_size(const H5HG_heap_t *heap)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(heap);

    FUNC_LEAVE_NOAPI(heap->obj[0].size)
} /* H5HG_get_free_size() */

