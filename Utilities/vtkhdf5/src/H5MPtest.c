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

/* Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Tuesday, May  3, 2005
 *
 * Purpose:	Memory pool testing functions.
 */

#define H5MP_PACKAGE		/*suppress error about including H5MPpkg  */
#define H5MP_TESTING		/*include H5MP testing funcs*/

/* Private headers */
#include "H5private.h"		/* Generic Functions			*/
#include "H5MPpkg.h"		/* Memory Pools				*/
#include "H5Eprivate.h"		/* Error handling		  	*/

/* Static Prototypes */

/* Package variables */


/*-------------------------------------------------------------------------
 * Function:	H5MP_get_pool_free_size
 *
 * Purpose:	Retrieve the total amount of free space in entire pool
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, May  3, 2005
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MP_get_pool_free_size(const H5MP_pool_t *mp, size_t *free_size)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5MP_get_pool_free_size)

    /* Check arguments. */
    HDassert(mp);
    HDassert(free_size);

    /* Get memory pool's free space */
    *free_size = mp->free_size;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5MP_get_pool_free_size() */


/*-------------------------------------------------------------------------
 * Function:	H5MP_get_pool_first_page
 *
 * Purpose:	Retrieve the first page in a memory pool
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, May  3, 2005
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MP_get_pool_first_page(const H5MP_pool_t *mp, H5MP_page_t **page)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5MP_get_pool_first_page)

    /* Check arguments. */
    HDassert(mp);
    HDassert(page);

    /* Get memory pool's first page */
    *page = mp->first;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5MP_get_pool_first_page() */


/*-------------------------------------------------------------------------
 * Function:	H5MP_pool_is_free_size_correct
 *
 * Purpose:	Check that the free space reported in each page corresponds
 *              to the free size in each page and that the free space in the
 *              free blocks for a page corresponds with the free space for
 *              the page.
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, May  3, 2005
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5MP_pool_is_free_size_correct(const H5MP_pool_t *mp)
{
    H5MP_page_t *page;          /* Pointer to current page */
    size_t pool_free;           /* Size of pages' free space */
    htri_t ret_value = TRUE;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5MP_pool_is_free_size_correct)

    /* Check arguments. */
    HDassert(mp);

    /* Iterate through pages, checking the free size & accumulating the
     * free space for all the pages */
    page = mp->first;
    pool_free = 0;
    while(page != NULL) {
        H5MP_page_blk_t *blk;           /* Pointer to current free block */
        size_t page_free;               /* Size of blocks on free list */

        /* Iterate through the blocks in page, accumulating free space */
        blk = (H5MP_page_blk_t *)((unsigned char *)page + H5MP_BLOCK_ALIGN(sizeof(H5MP_page_t)));
        page_free = 0;
        while(blk != NULL) {
            if(blk->is_free)
                page_free += blk->size;
            blk = blk->next;
        } /* end while */

        /* Check that the free space from the blocks on the free list
         * corresponds to space in page */
#ifdef QAK
HDfprintf(stderr,"%s: page_free = %Zu, page->free_size = %Zu\n", "H5MP_pool_is_free_size_correct", page_free, page->free_size);
#endif /* QAK */
        if(page_free != page->free_size)
            HGOTO_DONE (FALSE)

        /* Increment the amount of free space in pool */
        pool_free += page->free_size;

        /* Advance to next page */
        page = page->next;
    } /* end while */

    /* Check that the free space from the pages
     * corresponds to free space in pool */
#ifdef QAK
HDfprintf(stderr,"%s: pool_free = %Zu, mp->free_size = %Zu\n", "H5MP_pool_is_free_size_correct", pool_free, mp->free_size);
#endif /* QAK */
    if(pool_free != mp->free_size)
        HGOTO_DONE (FALSE)

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5MP_pool_is_free_size_correct() */


/*-------------------------------------------------------------------------
 * Function:	H5MP_get_page_free_size
 *
 * Purpose:	Retrieve the amount of free space in given page
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, May  3, 2005
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MP_get_page_free_size(const H5MP_page_t *page, size_t *free_size)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5MP_get_page_free_size)

    /* Check arguments. */
    HDassert(page);
    HDassert(free_size);

    /* Get memory page's free space */
    *free_size = page->free_size;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5MP_get_page_free_size() */


/*-------------------------------------------------------------------------
 * Function:	H5MP_get_page_next_page
 *
 * Purpose:	Retrieve the next page in the pool
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, May  3, 2005
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MP_get_page_next_page(const H5MP_page_t *page, H5MP_page_t **next_page)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5MP_get_page_next_page)

    /* Check arguments. */
    HDassert(page);
    HDassert(next_page);

    /* Get next memory page */
    *next_page = page->next;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5MP_get_page_next_page() */

