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

/*-------------------------------------------------------------------------
 *
 * Created:		H5HL.c
 *			Jul 16 1997
 *			Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:		Heap functions for the local heaps used by symbol
 *			tables to store names (among other things).
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5HL_PACKAGE		/* Suppress error about including H5HLpkg */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"         /* File access				*/
#include "H5HLpkg.h"		/* Local Heaps				*/
#include "H5MFprivate.h"	/* File memory management		*/


/****************/
/* Local Macros */
/****************/

#define H5HL_MIN_HEAP   128             /* Minimum size to reduce heap buffer to */


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

static H5HL_free_t *H5HL_remove_free(H5HL_t *heap, H5HL_free_t *fl);
static herr_t H5HL_minimize_heap_space(H5F_t *f, hid_t dxpl_id, H5HL_t *heap);


/*********************/
/* Package Variables */
/*********************/

/* Declare a free list to manage the H5HL_free_t struct */
H5FL_DEFINE(H5HL_free_t);

/* Declare a PQ free list to manage the heap chunk information */
H5FL_BLK_DEFINE(lheap_chunk);


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5HL_create
 *
 * Purpose:	Creates a new heap data structure on disk and caches it
 *		in memory.  SIZE_HINT is a hint for the initial size of the
 *		data area of the heap.	If size hint is invalid then a
 *		reasonable (but probably not optimal) size will be chosen.
 *		If the heap ever has to grow, then REALLOC_HINT is the
 *		minimum amount by which the heap will grow.
 *
 * Return:	Success:	Non-negative. The file address of new heap is
 *				returned through the ADDR argument.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jul 16 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HL_create(H5F_t *f, hid_t dxpl_id, size_t size_hint, haddr_t *addr_p/*out*/)
{
    H5HL_t	*heap = NULL;           /* Heap created */
    H5HL_prfx_t *prfx = NULL;           /* Heap prefix */
    hsize_t	total_size;		/* Total heap size on disk	*/
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check arguments */
    HDassert(f);
    HDassert(addr_p);

    /* Adjust size hint as necessary */
    if(size_hint && size_hint < H5HL_SIZEOF_FREE(f))
	size_hint = H5HL_SIZEOF_FREE(f);
    size_hint = H5HL_ALIGN(size_hint);

    /* Allocate new heap structure */
    if(NULL == (heap = H5HL_new(H5F_SIZEOF_SIZE(f), H5F_SIZEOF_ADDR(f), H5HL_SIZEOF_HDR(f))))
	HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, FAIL, "can't allocate new heap struct")

    /* Allocate file space */
    total_size = heap->prfx_size + size_hint;
    if(HADDR_UNDEF == (heap->prfx_addr = H5MF_alloc(f, H5FD_MEM_LHEAP, dxpl_id, total_size)))
	HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, FAIL, "unable to allocate file memory")

    /* Initialize info */
    heap->single_cache_obj = TRUE;
    heap->dblk_addr = heap->prfx_addr + (hsize_t)heap->prfx_size;
    heap->dblk_size = size_hint;
    if(size_hint)
        if(NULL == (heap->dblk_image = H5FL_BLK_CALLOC(lheap_chunk, size_hint)))
            HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, FAIL, "memory allocation failed")

    /* free list */
    if(size_hint) {
	if(NULL == (heap->freelist = H5FL_MALLOC(H5HL_free_t)))
	    HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, FAIL, "memory allocation failed")
	heap->freelist->offset = 0;
	heap->freelist->size = size_hint;
	heap->freelist->prev = heap->freelist->next = NULL;
        heap->free_block = 0;
    } /* end if */
    else {
	heap->freelist = NULL;
        heap->free_block = H5HL_FREE_NULL;
    } /* end else */

    /* Allocate the heap prefix */
    if(NULL == (prfx = H5HL_prfx_new(heap)))
	HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, FAIL, "memory allocation failed")

    /* Add to cache */
    if(H5AC_insert_entry(f, dxpl_id, H5AC_LHEAP_PRFX, heap->prfx_addr, prfx, H5AC__NO_FLAGS_SET) < 0)
	HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, FAIL, "unable to cache local heap prefix")

    /* Set address to return */
    *addr_p = heap->prfx_addr;

done:
    if(ret_value < 0) {
        if(prfx) {
            if(H5HL_prfx_dest(prfx) < 0)
                HDONE_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "unable to destroy local heap prefix")
        } /* end if */
        else {
            if(heap) {
                if(H5F_addr_defined(heap->prfx_addr))
                    if(H5MF_xfree(f, H5FD_MEM_LHEAP, dxpl_id, heap->prfx_addr, total_size) < 0)
                        HDONE_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "can't release heap data?")
                if(H5HL_dest(heap) < 0)
                    HDONE_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "unable to destroy local heap")
            } /* end if */
        } /* end else */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL_create() */


/*-------------------------------------------------------------------------
 * Function:	H5HL_dblk_realloc
 *
 * Purpose:	Reallocate data block for heap
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct 12 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HL_dblk_realloc(H5F_t *f, hid_t dxpl_id, H5HL_t *heap, size_t new_heap_size)
{
    H5HL_dblk_t *dblk;                  /* Local heap data block */
    haddr_t old_addr;                   /* Old location of heap data block */
    haddr_t new_addr;                   /* New location of heap data block */
    size_t old_heap_size;               /* Old size of heap data block */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check arguments */
    HDassert(heap);
    HDassert(new_heap_size > 0);

    /* Release old space on disk */
    old_addr = heap->dblk_addr;
    old_heap_size = heap->dblk_size;
    H5_CHECK_OVERFLOW(old_heap_size, size_t, hsize_t);
    if(H5MF_xfree(f, H5FD_MEM_LHEAP, dxpl_id, old_addr, (hsize_t)old_heap_size) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "can't release old heap data?")

    /* Allocate new space on disk */
    H5_CHECK_OVERFLOW(new_heap_size, size_t, hsize_t);
    if(HADDR_UNDEF == (new_addr = H5MF_alloc(f, H5FD_MEM_LHEAP, dxpl_id, (hsize_t)new_heap_size)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, FAIL, "unable to allocate file space for heap")

    /* Update heap info*/
    heap->dblk_addr = new_addr;
    heap->dblk_size = new_heap_size;

    /* Check if heap data block actually moved in the file */
    if(H5F_addr_eq(old_addr, new_addr)) {
        /* Check if heap data block is contiguous w/prefix */
        if(heap->single_cache_obj) {
            /* Sanity check */
            HDassert(H5F_addr_eq(heap->prfx_addr + heap->prfx_size, old_addr));
            HDassert(heap->prfx);

            /* Resize the heap prefix in the cache */
            if(H5AC_resize_entry(heap->prfx, (size_t)(heap->prfx_size + new_heap_size)) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTRESIZE, FAIL, "unable to resize heap in cache")
        } /* end if */
        else {
            /* Sanity check */
            HDassert(H5F_addr_ne(heap->prfx_addr + heap->prfx_size, old_addr));
            HDassert(heap->dblk);

            /* Resize the heap data block in the cache */
            if(H5AC_resize_entry(heap->dblk, (size_t)new_heap_size) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTRESIZE, FAIL, "unable to resize heap in cache")
        } /* end else */
    } /* end if */
    else {
        /* Check if heap data block was contiguous w/prefix previously */
        if(heap->single_cache_obj) {
            /* Create new heap data block */
            if(NULL == (dblk = H5HL_dblk_new(heap)))
                HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, FAIL, "unable to allocate local heap data block")

            /* Resize current heap prefix */
            heap->prfx_size = H5HL_SIZEOF_HDR(f);
            if(H5AC_resize_entry(heap->prfx, (size_t)heap->prfx_size) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTRESIZE, FAIL, "unable to resize heap prefix in cache")

            /* Insert data block into cache (pinned) */
            if(H5AC_insert_entry(f, dxpl_id, H5AC_LHEAP_DBLK, new_addr, dblk, H5AC__PIN_ENTRY_FLAG) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, FAIL, "unable to cache local heap data block")
            dblk = NULL;

            /* Reset 'single cache object' flag */
            heap->single_cache_obj = FALSE;
        } /* end if */
        else {
            /* Resize the heap data block in the cache */
            /* (ignore [unlikely] case where heap data block ends up
             *      contiguous w/heap prefix again.
             */
            if(H5AC_resize_entry(heap->dblk, (size_t)new_heap_size) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTRESIZE, FAIL, "unable to resize heap data block in cache")

            /* Relocate the heap data block in the cache */
            if(H5AC_move_entry(f, H5AC_LHEAP_DBLK, old_addr, new_addr) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTMOVE, FAIL, "unable to move heap data block in cache")
        } /* end else */
    } /* end else */

done:
    if(ret_value < 0) {
        /* Restore old heap address & size */
        heap->dblk_addr = old_addr;
        heap->dblk_size = old_heap_size;
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL_dblk_realloc() */


/*-------------------------------------------------------------------------
 * Function:    H5HL_minimize_heap_space
 *
 * Purpose:     Go through the heap's freelist and determine if we can
 *              eliminate the free blocks at the tail of the buffer.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  Bill Wendling
 *              wendling@ncsa.uiuc.edu
 *              Sept. 16, 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HL_minimize_heap_space(H5F_t *f, hid_t dxpl_id, H5HL_t *heap)
{
    size_t new_heap_size = heap->dblk_size;     /* New size of heap */
    herr_t ret_value = SUCCEED;                 /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(f);
    HDassert(heap);

    /*
     * Check to see if we can reduce the size of the heap in memory by
     * eliminating free blocks at the tail of the buffer before flushing the
     * buffer out.
     */
    if(heap->freelist) {
        H5HL_free_t    *tmp_fl;
        H5HL_free_t    *last_fl = NULL;

        /* Search for a free block at the end of the buffer */
        for(tmp_fl = heap->freelist; tmp_fl; tmp_fl = tmp_fl->next)
            /* Check if the end of this free block is at the end of the buffer */
            if(tmp_fl->offset + tmp_fl->size == heap->dblk_size) {
                last_fl = tmp_fl;
                break;
            } /* end if */

        /*
         * Found free block at the end of the buffer, decide what to do
         * about it
         */
        if(last_fl) {
            /*
             * If the last free block's size is more than half the memory
             * buffer size (and the memory buffer is larger than the
             * minimum size), reduce or eliminate it.
             */
            if(last_fl->size >= (heap->dblk_size / 2) && heap->dblk_size > H5HL_MIN_HEAP) {
                /*
                 * Reduce size of buffer until it's too small or would
                 * eliminate the free block
                 */
                while(new_heap_size > H5HL_MIN_HEAP &&
                        new_heap_size >= (last_fl->offset + H5HL_SIZEOF_FREE(f)))
                    new_heap_size /= 2;

                /*
                 * Check if reducing the memory buffer size would
                 * eliminate the free block
                 */
                if(new_heap_size < (last_fl->offset + H5HL_SIZEOF_FREE(f))) {
                    /* Check if this is the only block on the free list */
                    if(last_fl->prev == NULL && last_fl->next == NULL) {
                        /* Double the new memory size */
                        new_heap_size *= 2;

                        /* Truncate the free block */
                        last_fl->size = H5HL_ALIGN(new_heap_size - last_fl->offset);
                        new_heap_size = last_fl->offset + last_fl->size;
                        HDassert(last_fl->size >= H5HL_SIZEOF_FREE(f));
                    } /* end if */
                    else {
                        /*
                         * Set the size of the memory buffer to the start
                         * of the free list
                         */
                        new_heap_size = last_fl->offset;

                        /* Eliminate the free block from the list */
                        last_fl = H5HL_remove_free(heap, last_fl);
                    } /* end else */
                } /* end if */
                else {
                    /* Truncate the free block */
                    last_fl->size = H5HL_ALIGN(new_heap_size - last_fl->offset);
                    new_heap_size = last_fl->offset + last_fl->size;
                    HDassert(last_fl->size >= H5HL_SIZEOF_FREE(f));
                    HDassert(last_fl->size == H5HL_ALIGN(last_fl->size));
                } /* end else */
            } /* end if */
        } /* end if */
    } /* end if */

    /*
     * If the heap grew smaller than disk storage then move the
     * data segment of the heap to another contiguous block of disk
     * storage.
     */
    if(new_heap_size != heap->dblk_size) {
	HDassert(new_heap_size < heap->dblk_size);

        /* Resize the memory buffer */
        if(NULL == (heap->dblk_image = H5FL_BLK_REALLOC(lheap_chunk, heap->dblk_image, new_heap_size)))
            HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, FAIL, "memory allocation failed")

        /* Reallocate data block in file */
        if(H5HL_dblk_realloc(f, dxpl_id, heap, new_heap_size) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTRESIZE, FAIL, "reallocating data block failed")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5HL_minimize_heap_space() */


/*-------------------------------------------------------------------------
 * Function:    H5HL_protect
 *
 * Purpose:     This function is a wrapper for the H5AC_protect call.
 *
 * Return:      Success:    Non-NULL pointer to the local heap prefix.
 *              Failure:    NULL
 *
 * Programmer:  Bill Wendling
 *              wendling@ncsa.uiuc.edu
 *              Sept. 17, 2003
 *
 *-------------------------------------------------------------------------
 */
H5HL_t *
H5HL_protect(H5F_t *f, hid_t dxpl_id, haddr_t addr, H5AC_protect_t rw)
{
    H5HL_cache_prfx_ud_t prfx_udata; /* User data for protecting local heap prefix */
    H5HL_prfx_t *prfx = NULL;   /* Local heap prefix */
    H5HL_dblk_t *dblk = NULL;   /* Local heap data block */
    H5HL_t *heap;               /* Heap data structure */
    unsigned prfx_cache_flags = H5AC__NO_FLAGS_SET;         /* Cache flags for unprotecting prefix entry */
    unsigned dblk_cache_flags = H5AC__NO_FLAGS_SET;         /* Cache flags for unprotecting data block entry */
    H5HL_t *ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));

    /* Construct the user data for protect callback */
    prfx_udata.sizeof_size = H5F_SIZEOF_SIZE(f);
    prfx_udata.sizeof_addr = H5F_SIZEOF_ADDR(f);
    prfx_udata.prfx_addr = addr;
    prfx_udata.sizeof_prfx = H5HL_SIZEOF_HDR(f);

    /* Protect the local heap prefix */
    if(NULL == (prfx = (H5HL_prfx_t *)H5AC_protect(f, dxpl_id, H5AC_LHEAP_PRFX, addr, &prfx_udata, rw)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, NULL, "unable to load heap prefix")

    /* Get the pointer to the heap */
    heap = prfx->heap;

    /* Check if the heap is already pinned in memory */
    /* (for re-entrant situation) */
    if(heap->prots == 0) {
        /* Check if heap has separate data block */
        if(heap->single_cache_obj) {
            /* Set the flag for pinning the prefix when unprotecting it */
            prfx_cache_flags |= H5AC__PIN_ENTRY_FLAG;
        } /* end if */
        else {
            H5HL_cache_dblk_ud_t dblk_udata; /* User data for protecting local heap data block */

            /* Construct the user data for protect callback */
            dblk_udata.heap = heap;
            dblk_udata.loaded = FALSE;

            /* Protect the local heap data block */
            if(NULL == (dblk = (H5HL_dblk_t *)H5AC_protect(f, dxpl_id, H5AC_LHEAP_DBLK, heap->dblk_addr, &dblk_udata, rw)))
                HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, NULL, "unable to load heap data block")

            /* Pin the prefix, if the data block was loaded from file */
            if(dblk_udata.loaded)
                prfx_cache_flags |= H5AC__PIN_ENTRY_FLAG;

            /* Set the flag for pinning the data block when unprotecting it */
            dblk_cache_flags |= H5AC__PIN_ENTRY_FLAG;
        } /* end if */
    } /* end if */

    /* Increment # of times heap is protected */
    heap->prots++;

    /* Set return value */
    ret_value = heap;

done:
    /* Release the prefix from the cache, now pinned */
    if(prfx && H5AC_unprotect(f, dxpl_id, H5AC_LHEAP_PRFX, heap->prfx_addr, prfx, prfx_cache_flags) < 0)
        HDONE_ERROR(H5E_HEAP, H5E_CANTUNPROTECT, NULL, "unable to release local heap prefix")

    /* Release the data block from the cache, now pinned */
    if(dblk && H5AC_unprotect(f, dxpl_id, H5AC_LHEAP_DBLK, heap->dblk_addr, dblk, dblk_cache_flags) < 0)
        HDONE_ERROR(H5E_HEAP, H5E_CANTUNPROTECT, NULL, "unable to release local heap data block")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL_protect() */


/*-------------------------------------------------------------------------
 * Function:    H5HL_offset_into
 *
 * Purpose:     Called directly after the call to H5HL_protect so that
 *              a pointer to the object in the heap can be got.
 *
 * Return:      Success:    Valid pointer.
 *              Failure:    NULL
 *
 * Programmer:  Bill Wendling
 *              wendling@ncsa.uiuc.edu
 *              Sept. 17, 2003
 *
 *-------------------------------------------------------------------------
 */
void *
H5HL_offset_into(const H5HL_t *heap, size_t offset)
{
    /*
     * We need to have called some other function before this to get a
     * valid heap pointer. So, this can remain "FUNC_ENTER_NOAPI_NOINIT"
     */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(heap);
    HDassert(offset < heap->dblk_size);

    FUNC_LEAVE_NOAPI(heap->dblk_image + offset)
} /* end H5HL_offset_into() */


/*-------------------------------------------------------------------------
 * Function:    H5HL_unprotect
 *
 * Purpose:     Unprotect the data retrieved by the H5HL_protect call.
 *
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL
 *
 * Programmer:  Bill Wendling
 *              wendling@ncsa.uiuc.edu
 *              Sept. 17, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HL_unprotect(H5HL_t *heap)
{
    herr_t  ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* check arguments */
    HDassert(heap);

    /* Decrement # of times heap is protected */
    heap->prots--;

    /* Check for last unprotection of heap */
    if(heap->prots == 0) {
        /* Check for separate heap data block */
        if(heap->single_cache_obj) {
            /* Mark local heap prefix as evictable again */
            if(H5AC_unpin_entry(heap->prfx) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTUNPIN, FAIL, "unable to unpin local heap data block")
        } /* end if */
        else {
            /* Sanity check */
            HDassert(heap->dblk);

            /* Mark local heap data block as evictable again */
            /* (data block still pins prefix) */
            if(H5AC_unpin_entry(heap->dblk) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTUNPIN, FAIL, "unable to unpin local heap data block")
        } /* end else */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL_unprotect() */


/*-------------------------------------------------------------------------
 * Function:	H5HL_remove_free
 *
 * Purpose:	Removes free list element FL from the specified heap and
 *		frees it.
 *
 * Return:	NULL
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jul 17 1997
 *
 *-------------------------------------------------------------------------
 */
static H5HL_free_t *
H5HL_remove_free(H5HL_t *heap, H5HL_free_t *fl)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(fl->prev)
        fl->prev->next = fl->next;
    if(fl->next)
        fl->next->prev = fl->prev;

    if(!fl->prev)
        heap->freelist = fl->next;

    FUNC_LEAVE_NOAPI((H5HL_free_t *)H5FL_FREE(H5HL_free_t, fl))
} /* end H5HL_remove_free() */


/*-------------------------------------------------------------------------
 * Function:	H5HL_dirty
 *
 * Purpose:	Mark heap as dirty
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct 12 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HL_dirty(H5HL_t *heap)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check arguments */
    HDassert(heap);
    HDassert(heap->prfx);

    /* Mark heap data block as dirty, if there is one */
    if(!heap->single_cache_obj) {
        /* Sanity check */
        HDassert(heap->dblk);

        if(H5AC_mark_entry_dirty(heap->dblk) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTMARKDIRTY, FAIL, "unable to mark heap data block as dirty")
    } /* end if */

    /* Mark heap prefix as dirty */
    if(H5AC_mark_entry_dirty(heap->prfx) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTMARKDIRTY, FAIL, "unable to mark heap prefix as dirty")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL_dirty() */


/*-------------------------------------------------------------------------
 * Function:	H5HL_insert
 *
 * Purpose:	Inserts a new item into the heap.
 *
 * Return:	Success:	Offset of new item within heap.
 *		Failure:	UFAIL
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jul 17 1997
 *
 *-------------------------------------------------------------------------
 */
size_t
H5HL_insert(H5F_t *f, hid_t dxpl_id, H5HL_t *heap, size_t buf_size, const void *buf)
{
    H5HL_free_t	*fl = NULL, *last_fl = NULL;
    size_t	offset = 0;
    size_t	need_size;
    hbool_t	found;
    size_t	ret_value;      /* Return value */

    FUNC_ENTER_NOAPI(UFAIL)

    /* check arguments */
    HDassert(f);
    HDassert(heap);
    HDassert(buf_size > 0);
    HDassert(buf);

    /* Mark heap as dirty in cache */
    /* (A bit early in the process, but it's difficult to determine in the
     *  code below where to mark the heap as dirty, especially in error cases,
     *  so we just accept that an extra flush of the heap info could occur
     *  if an error occurs -QAK)
     */
    if(H5HL_dirty(heap) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTMARKDIRTY, UFAIL, "unable to mark heap as dirty")

    /*
     * In order to keep the free list descriptors aligned on word boundaries,
     * whatever that might mean, we round the size up to the next multiple of
     * a word.
     */
    need_size = H5HL_ALIGN(buf_size);

    /*
     * Look for a free slot large enough for this object and which would
     * leave zero or at least H5G_SIZEOF_FREE bytes left over.
     */
    for(fl = heap->freelist, found = FALSE; fl; fl = fl->next) {
	if(fl->size > need_size &&
                fl->size - need_size >= H5HL_SIZEOF_FREE(f)) {
	    /* a big enough free block was found */
	    offset = fl->offset;
	    fl->offset += need_size;
	    fl->size -= need_size;
	    HDassert(fl->offset == H5HL_ALIGN(fl->offset));
	    HDassert(fl->size == H5HL_ALIGN(fl->size));
	    found = TRUE;
	    break;
	} else if(fl->size == need_size) {
	    /* free block of exact size found */
	    offset = fl->offset;
	    fl = H5HL_remove_free(heap, fl);
	    found = TRUE;
	    break;
	} else if(!last_fl || last_fl->offset < fl->offset) {
	    /* track free space that's closest to end of heap */
	    last_fl = fl;
	}
    } /* end for */

    /*
     * If no free chunk was large enough, then allocate more space and
     * add it to the free list.	 If the heap ends with a free chunk, we
     * can extend that free chunk.  Otherwise we'll have to make another
     * free chunk.  If the heap must expand, we double its size.
     */
    if(found == FALSE) {
        size_t	need_more;      /* How much more space we need */
        size_t	new_dblk_size;  /* Final size of space allocated for heap data block */
        size_t	old_dblk_size;  /* Previous size of space allocated for heap data block */
        htri_t	extended;       /* Whether the local heap's data segment on disk was extended */

        /* At least double the heap's size, making certain there's enough room
         * for the new object */
	need_more = MAX(need_size, heap->dblk_size);

        /* If there is no last free block or it's not at the end of the heap,
         * and the amount of space to allocate is not big enough to include at
         * least the new object and a free-list info, trim down the amount of
         * space requested to just the amount of space needed.  (Generally
         * speaking, this only occurs when the heap is small -QAK)
         */
	if(!(last_fl && last_fl->offset + last_fl->size == heap->dblk_size)
                && (need_more < (need_size + H5HL_SIZEOF_FREE(f))))
            need_more = need_size;

	new_dblk_size = heap->dblk_size + need_more;
	HDassert(heap->dblk_size < new_dblk_size);
        old_dblk_size = heap->dblk_size;
	H5_CHECK_OVERFLOW(heap->dblk_size, size_t, hsize_t);
	H5_CHECK_OVERFLOW(new_dblk_size, size_t, hsize_t);

        /* Extend current heap if possible */
	extended = H5MF_try_extend(f, dxpl_id, H5FD_MEM_LHEAP, heap->dblk_addr, (hsize_t)(heap->dblk_size), (hsize_t)need_more);
        if(extended < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTEXTEND, UFAIL, "error trying to extend heap")

	/* Check if we extended the heap data block in file */
	if(extended == TRUE) {
            /* Check for prefix & data block contiguous */
            if(heap->single_cache_obj) {
                /* Resize prefix+data block */
                if(H5AC_resize_entry(heap->prfx, (size_t)(heap->prfx_size + new_dblk_size)) < 0)
                    HGOTO_ERROR(H5E_HEAP, H5E_CANTRESIZE, UFAIL, "unable to resize heap prefix in cache")
            } /* end if */
            else {
                /* Resize 'standalone' data block */
                if(H5AC_resize_entry(heap->dblk, (size_t)new_dblk_size) < 0)
                    HGOTO_ERROR(H5E_HEAP, H5E_CANTRESIZE, UFAIL, "unable to resize heap data block in cache")
            } /* end else */

            /* Note new size */
            heap->dblk_size = new_dblk_size;
	} /* end if */
        else { /* ...if we can't, allocate a new chunk & release the old */
            /* Reallocate data block in file */
            if(H5HL_dblk_realloc(f, dxpl_id, heap, new_dblk_size) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTRESIZE, UFAIL, "reallocating data block failed")
	} /* end if */

        /* If the last free list in the heap is at the end of the heap, extend it */
	if(last_fl && last_fl->offset + last_fl->size == old_dblk_size) {
	    /*
	     * Increase the size of the last free block.
	     */
	    offset = last_fl->offset;
	    last_fl->offset += need_size;
	    last_fl->size += need_more - need_size;
	    HDassert(last_fl->offset == H5HL_ALIGN(last_fl->offset));
	    HDassert(last_fl->size == H5HL_ALIGN(last_fl->size));

	    if (last_fl->size < H5HL_SIZEOF_FREE(f)) {
#ifdef H5HL_DEBUG
		if (H5DEBUG(HL) && last_fl->size) {
		    fprintf(H5DEBUG(HL), "H5HL: lost %lu bytes at line %d\n",
			    (unsigned long)(last_fl->size), __LINE__);
		}
#endif
		last_fl = H5HL_remove_free(heap, last_fl);
	    }
	} /* end if */
        else {
	    /*
	     * Create a new free list element large enough that we can
	     * take some space out of it right away.
	     */
	    offset = old_dblk_size;
	    if(need_more - need_size >= H5HL_SIZEOF_FREE(f)) {
		if(NULL == (fl = H5FL_MALLOC(H5HL_free_t)))
		    HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, UFAIL, "memory allocation failed")
		fl->offset = old_dblk_size + need_size;
		fl->size = need_more - need_size;
		HDassert(fl->offset == H5HL_ALIGN(fl->offset));
		HDassert(fl->size == H5HL_ALIGN(fl->size));
		fl->prev = NULL;
		fl->next = heap->freelist;
		if(heap->freelist)
                    heap->freelist->prev = fl;
		heap->freelist = fl;
#ifdef H5HL_DEBUG
	    } else if (H5DEBUG(HL) && need_more > need_size) {
		fprintf(H5DEBUG(HL),
			"H5HL_insert: lost %lu bytes at line %d\n",
			(unsigned long)(need_more - need_size), __LINE__);
#endif
	    }
	} /* end else */

#ifdef H5HL_DEBUG
	if (H5DEBUG(HL)) {
	    fprintf(H5DEBUG(HL),
		    "H5HL: resize mem buf from %lu to %lu bytes\n",
		    (unsigned long)(old_dblk_size),
		    (unsigned long)(old_dblk_size + need_more));
	}
#endif
	if(NULL == (heap->dblk_image = H5FL_BLK_REALLOC(lheap_chunk, heap->dblk_image, heap->dblk_size)))
	    HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, UFAIL, "memory allocation failed")

	/* Clear new section so junk doesn't appear in the file */
        /* (Avoid clearing section which will be overwritten with newly inserted data) */
	HDmemset(heap->dblk_image + offset + buf_size, 0, (new_dblk_size - (offset + buf_size)));
    } /* end if */

    /* Copy the data into the heap */
    HDmemcpy(heap->dblk_image + offset, buf, buf_size);

    /* Set return value */
    ret_value = offset;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5HL_insert() */


/*-------------------------------------------------------------------------
 * Function:	H5HL_remove
 *
 * Purpose:	Removes an object or part of an object from the heap at
 *		address ADDR of file F.	 The object (or part) to remove
 *		begins at byte OFFSET from the beginning of the heap and
 *		continues for SIZE bytes.
 *
 *		Once part of an object is removed, one must not attempt
 *		to access that part.  Removing the beginning of an object
 *		results in the object OFFSET increasing by the amount
 *		truncated.  Removing the end of an object results in
 *		object truncation.  Removing the middle of an object results
 *		in two separate objects, one at the original offset and
 *		one at the first offset past the removed portion.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jul 16 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HL_remove(H5F_t *f, hid_t dxpl_id, H5HL_t *heap, size_t offset, size_t size)
{
    H5HL_free_t		*fl = NULL;
    herr_t      	ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check arguments */
    HDassert(f);
    HDassert(heap);
    HDassert(size > 0);
    HDassert(offset == H5HL_ALIGN(offset));

    size = H5HL_ALIGN(size);

    HDassert(offset < heap->dblk_size);
    HDassert(offset + size <= heap->dblk_size);

    /* Mark heap as dirty in cache */
    /* (A bit early in the process, but it's difficult to determine in the
     *  code below where to mark the heap as dirty, especially in error cases,
     *  so we just accept that an extra flush of the heap info could occur
     *  if an error occurs -QAK)
     */
    if(H5HL_dirty(heap) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTMARKDIRTY, FAIL, "unable to mark heap as dirty")

    /*
     * Check if this chunk can be prepended or appended to an already
     * free chunk.  It might also fall between two chunks in such a way
     * that all three chunks can be combined into one.
     */
    fl = heap->freelist;
    while(fl) {
        H5HL_free_t *fl2 = NULL;

	if((offset + size) == fl->offset) {
	    fl->offset = offset;
	    fl->size += size;
	    HDassert(fl->offset == H5HL_ALIGN(fl->offset));
	    HDassert(fl->size == H5HL_ALIGN(fl->size));
	    fl2 = fl->next;
	    while(fl2) {
		if((fl2->offset + fl2->size) == fl->offset) {
		    fl->offset = fl2->offset;
		    fl->size += fl2->size;
		    HDassert(fl->offset == H5HL_ALIGN(fl->offset));
		    HDassert(fl->size == H5HL_ALIGN(fl->size));
		    fl2 = H5HL_remove_free(heap, fl2);
	            if(((fl->offset + fl->size) == heap->dblk_size) &&
                             ((2 * fl->size) > heap->dblk_size)) {
                        if(H5HL_minimize_heap_space(f, dxpl_id, heap) < 0)
	                    HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "heap size minimization failed")
                    }
		    HGOTO_DONE(SUCCEED);
		}
		fl2 = fl2->next;
	    }
	    if(((fl->offset + fl->size) == heap->dblk_size) &&
                     ((2 * fl->size) > heap->dblk_size)) {
                if(H5HL_minimize_heap_space(f, dxpl_id, heap) < 0)
	            HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "heap size minimization failed")
            }
	    HGOTO_DONE(SUCCEED);
	} else if(fl->offset + fl->size == offset) {
	    fl->size += size;
	    fl2 = fl->next;
	    HDassert(fl->size == H5HL_ALIGN(fl->size));
	    while(fl2) {
		if(fl->offset + fl->size == fl2->offset) {
		    fl->size += fl2->size;
		    HDassert(fl->size == H5HL_ALIGN(fl->size));
		    fl2 = H5HL_remove_free(heap, fl2);
	            if(((fl->offset + fl->size) == heap->dblk_size) &&
                            ((2 * fl->size) > heap->dblk_size)) {
                        if(H5HL_minimize_heap_space(f, dxpl_id, heap) < 0)
	                    HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "heap size minimization failed")
                    } /* end if */
		    HGOTO_DONE(SUCCEED);
		} /* end if */
		fl2 = fl2->next;
	    } /* end while */
	    if(((fl->offset + fl->size) == heap->dblk_size) &&
                    ((2 * fl->size) > heap->dblk_size)) {
                if(H5HL_minimize_heap_space(f, dxpl_id, heap) < 0)
	            HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "heap size minimization failed")
            } /* end if */
	    HGOTO_DONE(SUCCEED);
	} /* end if */
	fl = fl->next;
    } /* end while */

    /*
     * The amount which is being removed must be large enough to
     * hold the free list data.	 If not, the freed chunk is forever
     * lost.
     */
    if(size < H5HL_SIZEOF_FREE(f)) {
#ifdef H5HL_DEBUG
	if(H5DEBUG(HL)) {
	    fprintf(H5DEBUG(HL), "H5HL: lost %lu bytes\n",
		    (unsigned long) size);
	}
#endif
	HGOTO_DONE(SUCCEED);
    } /* end if */

    /*
     * Add an entry to the free list.
     */
    if(NULL == (fl = H5FL_MALLOC(H5HL_free_t)))
	HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, FAIL, "memory allocation failed")
    fl->offset = offset;
    fl->size = size;
    HDassert(fl->offset == H5HL_ALIGN(fl->offset));
    HDassert(fl->size == H5HL_ALIGN(fl->size));
    fl->prev = NULL;
    fl->next = heap->freelist;
    if(heap->freelist)
        heap->freelist->prev = fl;
    heap->freelist = fl;

    if(((fl->offset + fl->size) == heap->dblk_size) &&
            ((2 * fl->size) > heap->dblk_size)) {
        if(H5HL_minimize_heap_space(f, dxpl_id, heap) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "heap size minimization failed")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL_remove() */


/*-------------------------------------------------------------------------
 * Function:	H5HL_delete
 *
 * Purpose:	Deletes a local heap from disk, freeing disk space used.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar 22 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HL_delete(H5F_t *f, hid_t dxpl_id, haddr_t addr)
{
    H5HL_t	*heap;                  /* Local heap to delete */
    H5HL_cache_prfx_ud_t prfx_udata;    /* User data for protecting local heap prefix */
    H5HL_prfx_t *prfx = NULL;           /* Local heap prefix */
    H5HL_dblk_t *dblk = NULL;           /* Local heap data block */
    unsigned    cache_flags = H5AC__NO_FLAGS_SET;       /* Flags for unprotecting heap */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));

    /* Construct the user data for protect callback */
    prfx_udata.sizeof_size = H5F_SIZEOF_SIZE(f);
    prfx_udata.sizeof_addr = H5F_SIZEOF_ADDR(f);
    prfx_udata.prfx_addr = addr;
    prfx_udata.sizeof_prfx = H5HL_SIZEOF_HDR(f);

    /* Protect the local heap prefix */
    if(NULL == (prfx = (H5HL_prfx_t *)H5AC_protect(f, dxpl_id, H5AC_LHEAP_PRFX, addr, &prfx_udata, H5AC_WRITE)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, FAIL, "unable to load heap prefix")

    /* Get the pointer to the heap */
    heap = prfx->heap;

    /* Check if heap has separate data block */
    if(!heap->single_cache_obj) {
        H5HL_cache_dblk_ud_t dblk_udata; /* User data for protecting local heap data block */

        /* Construct the user data for protect callback */
        dblk_udata.heap = heap;
        dblk_udata.loaded = FALSE;

        /* Protect the local heap data block */
        if(NULL == (dblk = (H5HL_dblk_t *)H5AC_protect(f, dxpl_id, H5AC_LHEAP_DBLK, heap->dblk_addr, &dblk_udata, H5AC_WRITE)))
            HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, FAIL, "unable to load heap data block")

        /* Pin the prefix, if the data block was loaded from file */
        if(dblk_udata.loaded) {
            if(H5AC_pin_protected_entry(prfx) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTPIN, FAIL, "unable to pin local heap prefix")
        } /* end if */
    } /* end if */

    /* Set the flags for releasing the prefix and data block */
    cache_flags |= H5AC__DIRTIED_FLAG | H5AC__DELETED_FLAG | H5AC__FREE_FILE_SPACE_FLAG;

done:
    /* Release the data block from the cache, now deleted */
    if(dblk && H5AC_unprotect(f, dxpl_id, H5AC_LHEAP_DBLK, heap->dblk_addr, dblk, cache_flags) < 0)
        HDONE_ERROR(H5E_HEAP, H5E_CANTUNPROTECT, FAIL, "unable to release local heap data block")

    /* Release the prefix from the cache, now deleted */
    if(prfx && H5AC_unprotect(f, dxpl_id, H5AC_LHEAP_PRFX, heap->prfx_addr, prfx, cache_flags) < 0)
        HDONE_ERROR(H5E_HEAP, H5E_CANTUNPROTECT, FAIL, "unable to release local heap prefix")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL_delete() */


/*-------------------------------------------------------------------------
 * Function:	H5HL_get_size
 *
 * Purpose:	Retrieves the current size of a heap
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Nov  7 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HL_get_size(H5F_t *f, hid_t dxpl_id, haddr_t addr, size_t *size)
{
    H5HL_cache_prfx_ud_t prfx_udata;      /* User data for protecting local heap prefix */
    H5HL_prfx_t *prfx = NULL;   /* Local heap prefix */
    H5HL_t *heap;               /* Heap data structure */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(size);

    /* Construct the user data for protect callback */
    prfx_udata.sizeof_size = H5F_SIZEOF_SIZE(f);
    prfx_udata.sizeof_addr = H5F_SIZEOF_ADDR(f);
    prfx_udata.prfx_addr = addr;
    prfx_udata.sizeof_prfx = H5HL_SIZEOF_HDR(f);

    /* Protect the local heap prefix */
    if(NULL == (prfx = (H5HL_prfx_t *)H5AC_protect(f, dxpl_id, H5AC_LHEAP_PRFX, addr, &prfx_udata, H5AC_READ)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, FAIL, "unable to load heap prefix")

    /* Get the pointer to the heap */
    heap = prfx->heap;

    /* Set the size to return */
    *size = heap->dblk_size;

done:
    if(prfx && H5AC_unprotect(f, dxpl_id, H5AC_LHEAP_PRFX, heap->prfx_addr, prfx, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_HEAP, H5E_CANTUNPROTECT, FAIL, "unable to release local heap prefix")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL_get_size() */


/*-------------------------------------------------------------------------
 * Function:    H5HL_heapsize
 *
 * Purpose:     Compute the size in bytes of the specified instance of
 *              H5HL_t via H5HL_size()
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Vailin Choi
 *              June 19 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HL_heapsize(H5F_t *f, hid_t dxpl_id, haddr_t addr, hsize_t *heap_size)
{
    H5HL_cache_prfx_ud_t prfx_udata;      /* User data for protecting local heap prefix */
    H5HL_prfx_t *prfx = NULL;   /* Local heap prefix */
    H5HL_t *heap;               /* Heap data structure */
    herr_t ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(heap_size);

    /* Construct the user data for protect callback */
    prfx_udata.sizeof_size = H5F_SIZEOF_SIZE(f);
    prfx_udata.sizeof_addr = H5F_SIZEOF_ADDR(f);
    prfx_udata.prfx_addr = addr;
    prfx_udata.sizeof_prfx = H5HL_SIZEOF_HDR(f);

    /* Protect the local heap prefix */
    if(NULL == (prfx = (H5HL_prfx_t *)H5AC_protect(f, dxpl_id, H5AC_LHEAP_PRFX, addr, &prfx_udata, H5AC_READ)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, FAIL, "unable to load heap prefix")

    /* Get the pointer to the heap */
    heap = prfx->heap;

    /* Accumulate the size of the local heap */
    *heap_size += (hsize_t)(heap->prfx_size + heap->dblk_size);

done:
    if(prfx && H5AC_unprotect(f, dxpl_id, H5AC_LHEAP_PRFX, heap->prfx_addr, prfx, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_HEAP, H5E_CANTUNPROTECT, FAIL, "unable to release local heap prefix")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HL_heapsize() */

