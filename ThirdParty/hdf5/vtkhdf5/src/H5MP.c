/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:		H5MP.c
 *			May  2 2005
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Implements memory pools.  (Similar to Apache's APR
 *                      memory pools)
 *
 *                      Please see the documentation in:
 *                      doc/html/TechNotes/MemoryPools.html for a full description
 *                      of how they work, etc.
 *
 *-------------------------------------------------------------------------
 */

#include "H5MPmodule.h"         /* This source code file is part of the H5MP module */

/* Private headers */
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5MPpkg.h"		/* Memory Pools				*/

/****************/
/* Local Macros */
/****************/

/* Minimum sized block */
#define H5MP_MIN_BLOCK  (H5MP_BLOCK_ALIGN(sizeof(H5MP_page_blk_t)) + H5MP_BLOCK_ALIGNMENT)

/* First block in page */
#define H5MP_PAGE_FIRST_BLOCK(p) \
    (H5MP_page_blk_t *)((unsigned char *)(p) + H5MP_BLOCK_ALIGN(sizeof(H5MP_page_t)))


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Local Prototypes */
/********************/


/********************************/
/* Package Variable Definitions */
/********************************/

/* Package initialization variable */
hbool_t H5_PKG_INIT_VAR = FALSE;


/********************/
/* Static Variables */
/********************/

/* Declare a free list to manage the H5MP_pool_t struct */
H5FL_DEFINE(H5MP_pool_t);



/*-------------------------------------------------------------------------
 * Function:	H5MP_create
 *
 * Purpose:	Create a new memory pool
 *
 * Return:	Pointer to the memory pool "header" on success/NULL on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May  2 2005
 *
 *-------------------------------------------------------------------------
 */
H5MP_pool_t *
H5MP_create(size_t page_size, unsigned flags)
{
    H5MP_pool_t *mp = NULL;             /* New memory pool header */
    H5MP_pool_t *ret_value = NULL;      /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Allocate space for the pool header */
    if(NULL == (mp = H5FL_MALLOC(H5MP_pool_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for memory pool header")

    /* Assign information */
    mp->page_size = H5MP_BLOCK_ALIGN(page_size);
    mp->flags = flags;

    /* Initialize information */
    mp->free_size = 0;
    mp->first = NULL;
    mp->max_size = mp->page_size - H5MP_BLOCK_ALIGN(sizeof(H5MP_page_t));

    /* Create factory for pool pages */
    if(NULL == (mp->page_fac = H5FL_fac_init(page_size)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, NULL, "can't create page factory")

    /* Set return value */
    ret_value = mp;

done:
    if(NULL == ret_value && mp)
        if(H5MP_close(mp) < 0)
            HDONE_ERROR(H5E_RESOURCE, H5E_CANTFREE, NULL, "unable to free memory pool header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MP_create() */


/*-------------------------------------------------------------------------
 * Function:	H5MP_new_page
 *
 * Purpose:	Allocate new page for a memory pool
 *
 * Return:	Pointer to the page allocated on success/NULL on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May  4 2005
 *
 *-------------------------------------------------------------------------
 */
static H5MP_page_t *
H5MP_new_page(H5MP_pool_t *mp, size_t page_size)
{
    H5MP_page_t *new_page;              /* New page created */
    H5MP_page_blk_t *first_blk;         /* Pointer to first block in page */
    H5MP_page_t *ret_value = NULL;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(mp);
    HDassert(page_size >= mp->page_size);

    /* Allocate page */
    if(page_size > mp->page_size) {
        if(NULL == (new_page = (H5MP_page_t *)H5MM_malloc(page_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for page")
        new_page->free_size = page_size - H5MP_BLOCK_ALIGN(sizeof(H5MP_page_t));
        new_page->fac_alloc = FALSE;
    } /* end if */
    else {
        if(NULL == (new_page = (H5MP_page_t *)H5FL_FAC_MALLOC(mp->page_fac)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for page")
        new_page->free_size = mp->max_size;
        new_page->fac_alloc = TRUE;
    } /* end else */
#ifdef QAK
HDfprintf(stderr,"%s: Allocating new page = %p\n", FUNC, new_page);
#endif /* QAK */

    /* Initialize page information */
    first_blk = H5MP_PAGE_FIRST_BLOCK(new_page);
    first_blk->size = new_page->free_size;
    first_blk->page = new_page;
    first_blk->is_free = TRUE;
    first_blk->prev = NULL;
    first_blk->next = NULL;

    /* Insert into page list */
    new_page->prev = NULL;
    new_page->next = mp->first;
    if(mp->first)
        mp->first->prev = new_page;
    mp->first = new_page;

    /* Account for new free space */
    new_page->free_blk = first_blk;
    mp->free_size += new_page->free_size;

    /* Assign return value */
    ret_value = new_page;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MP_new_page() */


/*-------------------------------------------------------------------------
 * Function:	H5MP_malloc
 *
 * Purpose:	Allocate space in a memory pool
 *
 * Return:	Pointer to the space allocated on success/NULL on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May  2 2005
 *
 *-------------------------------------------------------------------------
 */
void *
H5MP_malloc (H5MP_pool_t *mp, size_t request)
{
    H5MP_page_t *alloc_page = NULL; /* Page to allocate space from */
    H5MP_page_blk_t *alloc_free;    /* Pointer to free space in page */
    size_t needed;                  /* Size requested, plus block header and alignment */
    void *ret_value = NULL;         /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Sanity check */
    HDassert(mp);
    HDassert(request > 0);

    /* Compute actual size needed */
    needed = H5MP_BLOCK_ALIGN(request) + H5MP_BLOCK_ALIGN(sizeof(H5MP_page_blk_t));
#ifdef QAK
HDfprintf(stderr,"%s: sizeof(H5MP_page_blk_t) = %Zu\n", FUNC, sizeof(H5MP_page_blk_t));
HDfprintf(stderr,"%s: request = %Zu, needed = %Zu\n", FUNC, request, needed);
#endif /* QAK */

    /* See if the request can be handled by existing free space */
    if(needed <= mp->free_size) {
        size_t pool_free_avail;      /* Amount of free space possibly available in pool */

        /* Locate page with enough free space */
        alloc_page = mp->first;
        pool_free_avail = mp->free_size;
        while(alloc_page && pool_free_avail >= needed) {
            /* If we found a page with enough free space, search for large
             * enough free block on that page */
            if(alloc_page->free_size >= needed) {
                size_t page_free_avail;      /* Amount of free space possibly available */

                /* Locate large enough block */
                alloc_free = alloc_page->free_blk;
                page_free_avail = alloc_page->free_size;
                while(alloc_free && page_free_avail >= needed) {
                    if(alloc_free->is_free) {
                        /* If we found a large enough block, leave now */
                        if(alloc_free->size >= needed)
                            goto found;     /* Needed to escape double "while" loop */

                        /* Decrement amount of potential space left */
                        page_free_avail -= alloc_free->size;
                    } /* end if */

                    /* Go to next block */
                    alloc_free = alloc_free->next;
                } /* end while */
            } /* end if */

            /* Decrement amount of potential space left */
            pool_free_avail -= alloc_page->free_size;

            /* Go to next page */
            alloc_page = alloc_page->next;
        } /* end while */
    } /* end if */

    {
        size_t page_size;       /* Size of page needed */

        /* Check if the request is too large for a standard page */
        page_size = (needed > mp->max_size) ?
            (needed + H5MP_BLOCK_ALIGN(sizeof(H5MP_page_t))) : mp->page_size;

        /* Allocate new page */
        if(NULL == (alloc_page = H5MP_new_page(mp, page_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for page")

        /* Set the block to allocate from */
        alloc_free = alloc_page->free_blk;
    } /* end block */

    /* Allocate space in page */
found:

    /* Sanity check */
    HDassert(alloc_page);
    HDassert(alloc_free);

    /* Check if we can subdivide the free space */
    if(alloc_free->size > (needed + H5MP_MIN_BLOCK)) {
        H5MP_page_blk_t *new_free;          /* New free block created */

        /* Carve out new free block after block to allocate */
        new_free = (H5MP_page_blk_t *)(((unsigned char *)alloc_free) + needed);

        /* Link into existing lists */
        new_free->next = alloc_free->next;
        if(alloc_free->next)
            alloc_free->next->prev = new_free;
        new_free->prev = alloc_free;
        alloc_free->next = new_free;

        /* Set blocks' information */
        new_free->size = alloc_free->size - needed;
        new_free->is_free = TRUE;
        new_free->page = alloc_free->page;
        alloc_free->size = needed;
        alloc_free->is_free = FALSE;
    } /* end if */
    else {
        /* Use whole free space block for new block */
        alloc_free->is_free = FALSE;
    } /* end else */

    /* Update page & pool's free size information */
    alloc_page->free_size -= alloc_free->size;
    if(alloc_page->free_blk == alloc_free)
        alloc_page->free_blk = alloc_free->next;
    mp->free_size -= alloc_free->size;

    /* Set new space pointer for the return value */
    ret_value = ((unsigned char *)alloc_free) + H5MP_BLOCK_ALIGN(sizeof(H5MP_page_blk_t));
#ifdef QAK
HDfprintf(stderr,"%s: Allocating space from page, ret_value = %p\n", FUNC, ret_value);
#endif /* QAK */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MP_malloc() */


/*-------------------------------------------------------------------------
 * Function:	H5MP_free
 *
 * Purpose:	Release space in a memory pool
 *
 * Return:	NULL on success/NULL on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May  3 2005
 *
 * Note:        Should we release pages that have no used blocks?
 *
 *-------------------------------------------------------------------------
 */
void *
H5MP_free(H5MP_pool_t *mp, void *spc)
{
    H5MP_page_blk_t *spc_blk;           /* Block for space to free */
    H5MP_page_t *spc_page;              /* Page containing block to free */
    void *ret_value = NULL;             /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(mp);
    HDassert(spc);

    /* Get block header for space to free */
    spc_blk = (H5MP_page_blk_t *)(((unsigned char *)spc) - H5MP_BLOCK_ALIGN(sizeof(H5MP_page_blk_t)));

    /* Mark block as free */
    HDassert(spc_blk->is_free == FALSE);
    spc_blk->is_free = TRUE;

    /* Add it's space to the amount of free space in the page & pool */
    spc_page = spc_blk->page;
#ifdef QAK
HDfprintf(stderr,"%s: Freeing from page = %p\n", "H5MP_free", spc_page);
#endif /* QAK */
    spc_page->free_size += spc_blk->size;
    mp->free_size += spc_blk->size;

    /* Move page with newly freed space to front of list of pages in pool */
    if(spc_page != mp->first) {
        /* Remove page from list */
        spc_page->prev->next = spc_page->next;
        if(spc_page->next)
            spc_page->next->prev = spc_page->prev;

        /* Insert page at beginning of list */
        spc_page->prev = NULL;
        spc_page->next = mp->first;
        mp->first->prev = spc_page;
        mp->first = spc_page;
    } /* end if */

    /* Check if block can be merged with free space after it on page */
    if(spc_blk->next != NULL) {
        H5MP_page_blk_t *next_blk;          /* Block following space to free */

        next_blk = spc_blk->next;
        HDassert(next_blk->prev == spc_blk);
        if(next_blk->is_free) {
            spc_blk->size += next_blk->size;
            spc_blk->next = next_blk->next;
        } /* end if */
    } /* end if */

    /* Check if block can be merged with free space before it on page */
    if(spc_blk->prev != NULL) {
        H5MP_page_blk_t *prev_blk;          /* Block before space to free */

        prev_blk = spc_blk->prev;
        HDassert(prev_blk->next == spc_blk);
        if(prev_blk->is_free) {
            prev_blk->size += spc_blk->size;
            prev_blk->next = spc_blk->next;
        } /* end if */
    } /* end if */

    /* Check if the block freed becomes the first free block on the page */
    if(spc_page->free_blk == NULL || spc_blk < spc_page->free_blk)
        spc_page->free_blk = spc_blk;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MP_free() */


/*-------------------------------------------------------------------------
 * Function:	H5MP_close
 *
 * Purpose:	Release all memory for a pool and destroy pool
 *
 * Return:	Non-negative on success/negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May  3 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MP_close(H5MP_pool_t *mp)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Release memory for pool pages */
    if(mp->first != NULL) {
        H5MP_page_t *page, *next_page;  /* Pointer to pages in pool */

        /* Iterate through pages, releasing them */
        page = mp->first;
        while(page) {
            next_page = page->next;

            /* Free the page appropriately */
            if(page->fac_alloc)
                page = (H5MP_page_t *)H5FL_FAC_FREE(mp->page_fac, page);
            else
                page = (H5MP_page_t *)H5MM_xfree(page);

            page = next_page;
        } /* end while */
    } /* end if */

    /* Release page factory */
    if(mp->page_fac)
        if(H5FL_fac_term(mp->page_fac) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't destroy page factory")

done:
    /* Free the memory pool itself */
    mp = H5FL_FREE(H5MP_pool_t, mp);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MP_close() */

