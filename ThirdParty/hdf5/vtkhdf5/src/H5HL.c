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
 * Created:         H5HL.c
 *                  Jul 16 1997
 *                  Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:         Heap functions for the local heaps used by symbol
 *                  tables to store names (among other things).
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5HLmodule.h"         /* This source code file is part of the H5HL module */


/***********/
/* Headers */
/***********/
#include "H5private.h"      /* Generic Functions        */
#include "H5Eprivate.h"     /* Error handling           */
#include "H5Fprivate.h"     /* File access              */
#include "H5HLpkg.h"        /* Local Heaps              */
#include "H5MFprivate.h"    /* File memory management   */


/****************/
/* Local Macros */
/****************/

#define H5HL_MIN_HEAP   128     /* Minimum size to reduce heap buffer to */


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

static H5HL_free_t *H5HL__remove_free(H5HL_t *heap, H5HL_free_t *fl);
static herr_t H5HL__minimize_heap_space(H5F_t *f, H5HL_t *heap);
static herr_t H5HL__dirty(H5HL_t *heap);


/*********************/
/* Package Variables */
/*********************/

/* Package initialization variable */
hbool_t H5_PKG_INIT_VAR = FALSE;

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
 * Function:    H5HL_create
 *
 * Purpose:     Creates a new heap data structure on disk and caches it
 *              in memory.  SIZE_HINT is a hint for the initial size of the
 *              data area of the heap.  If size hint is invalid then a
 *              reasonable (but probably not optimal) size will be chosen.
 *
 * Return:      Success:    SUCCEED. The file address of new heap is
 *                          returned through the ADDR argument.
 *              Failure:    FAIL.  addr_p will be HADDR_UNDEF.
 *
 * Programmer:  Robb Matzke
 *              Jul 16 1997
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PRIV, ERR,
herr_t, SUCCEED, FAIL,
H5HL_create(H5F_t *f, size_t size_hint, haddr_t *addr_p/*out*/))

    H5HL_t      *heap = NULL;           /* Heap created                 */
    H5HL_prfx_t *prfx = NULL;           /* Heap prefix                  */
    hsize_t     total_size = 0;         /* Total heap size on disk      */

    /* check arguments */
    HDassert(f);
    HDassert(addr_p);

    /* Adjust size hint as necessary */
    if(size_hint && size_hint < H5HL_SIZEOF_FREE(f))
        size_hint = H5HL_SIZEOF_FREE(f);
    size_hint = H5HL_ALIGN(size_hint);

    /* Allocate new heap structure */
    if(NULL == (heap = H5HL__new(H5F_SIZEOF_SIZE(f), H5F_SIZEOF_ADDR(f), H5HL_SIZEOF_HDR(f))))
        H5E_THROW(H5E_CANTALLOC, "can't allocate new heap struct");

    /* Allocate file space */
    total_size = heap->prfx_size + size_hint;
    if(HADDR_UNDEF == (heap->prfx_addr = H5MF_alloc(f, H5FD_MEM_LHEAP, total_size)))
        H5E_THROW(H5E_CANTALLOC, "unable to allocate file memory");

    /* Initialize info */
    heap->single_cache_obj = TRUE;
    heap->dblk_addr = heap->prfx_addr + (hsize_t)heap->prfx_size;
    heap->dblk_size = size_hint;
    if(size_hint)
        if(NULL == (heap->dblk_image = H5FL_BLK_CALLOC(lheap_chunk, size_hint)))
            H5E_THROW(H5E_CANTALLOC, "memory allocation failed");

    /* free list */
    if(size_hint) {
        if(NULL == (heap->freelist = H5FL_MALLOC(H5HL_free_t)))
            H5E_THROW(H5E_CANTALLOC, "memory allocation failed");
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
    if(NULL == (prfx = H5HL__prfx_new(heap)))
        H5E_THROW(H5E_CANTALLOC, "memory allocation failed");

    /* Add to cache */
    if(FAIL == H5AC_insert_entry(f, H5AC_LHEAP_PRFX, heap->prfx_addr, prfx, H5AC__NO_FLAGS_SET))
        H5E_THROW(H5E_CANTINIT, "unable to cache local heap prefix");

    /* Set address to return */
    *addr_p = heap->prfx_addr;

CATCH
    if(ret_value < 0) {
        *addr_p = HADDR_UNDEF;
        if(prfx) {
            if(FAIL == H5HL__prfx_dest(prfx))
                H5E_THROW(H5E_CANTFREE, "unable to destroy local heap prefix");
        } /* end if */
        else {
            if(heap) {
                if(H5F_addr_defined(heap->prfx_addr))
                    if(FAIL == H5MF_xfree(f, H5FD_MEM_LHEAP, heap->prfx_addr, total_size))
                        H5E_THROW(H5E_CANTFREE, "can't release heap data?");
                if(FAIL == H5HL__dest(heap))
                    H5E_THROW(H5E_CANTFREE, "unable to destroy local heap");
            } /* end if */
        } /* end else */
    } /* end if */

END_FUNC(PRIV) /* end H5HL_create() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__minimize_heap_space
 *
 * Purpose:     Go through the heap's freelist and determine if we can
 *              eliminate the free blocks at the tail of the buffer.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Bill Wendling
 *              Sept. 16, 2003
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(STATIC, ERR,
herr_t, SUCCEED, FAIL,
H5HL__minimize_heap_space(H5F_t *f, H5HL_t *heap))

    size_t new_heap_size = heap->dblk_size;     /* New size of heap */

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
                        last_fl = H5HL__remove_free(heap, last_fl);
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
            H5E_THROW(H5E_CANTALLOC, "memory allocation failed");

        /* Reallocate data block in file */
        if(FAIL == H5HL__dblk_realloc(f, heap, new_heap_size))
            H5E_THROW(H5E_CANTRESIZE, "reallocating data block failed");
    } /* end if */

CATCH
    /* No special processing on errors */

END_FUNC(STATIC) /* H5HL__minimize_heap_space() */


/*-------------------------------------------------------------------------
 * Function:    H5HL_protect
 *
 * Purpose:     This function is a wrapper for the H5AC_protect call.
 *
 * Return:      Success:    Non-NULL pointer to the local heap prefix.
 *              Failure:    NULL
 *
 * Programmer:  Bill Wendling
 *              Sept. 17, 2003
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PRIV, ERR,
H5HL_t *, NULL, NULL,
H5HL_protect(H5F_t *f, haddr_t addr, unsigned flags))

    H5HL_cache_prfx_ud_t prfx_udata;                    /* User data for protecting local heap prefix       */
    H5HL_prfx_t *prfx = NULL;                           /* Local heap prefix                                */
    H5HL_dblk_t *dblk = NULL;                           /* Local heap data block                            */
    H5HL_t *heap = NULL;                                /* Heap data structure                              */
    unsigned prfx_cache_flags = H5AC__NO_FLAGS_SET;     /* Cache flags for unprotecting prefix entry        */
    unsigned dblk_cache_flags = H5AC__NO_FLAGS_SET;     /* Cache flags for unprotecting data block entry    */

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));

    /* only the H5AC__READ_ONLY_FLAG may appear in flags */
    HDassert((flags & (unsigned)(~H5AC__READ_ONLY_FLAG)) == 0);

    /* Construct the user data for protect callback */
    prfx_udata.sizeof_size = H5F_SIZEOF_SIZE(f);
    prfx_udata.sizeof_addr = H5F_SIZEOF_ADDR(f);
    prfx_udata.prfx_addr = addr;
    prfx_udata.sizeof_prfx = H5HL_SIZEOF_HDR(f);

    /* Protect the local heap prefix */
    if(NULL == (prfx = (H5HL_prfx_t *)H5AC_protect(f, H5AC_LHEAP_PRFX, addr, &prfx_udata, flags)))
        H5E_THROW(H5E_CANTPROTECT, "unable to load heap prefix");

    /* Get the pointer to the heap */
    heap = prfx->heap;

    /* Check if the heap is already pinned in memory */
    /* (for re-entrant situation) */
    if(heap->prots == 0) {
        /* Check if heap has separate data block */
        if(heap->single_cache_obj)
            /* Set the flag for pinning the prefix when unprotecting it */
            prfx_cache_flags |= H5AC__PIN_ENTRY_FLAG;
        else {
            /* Protect the local heap data block */
            if(NULL == (dblk = (H5HL_dblk_t *)H5AC_protect(f, H5AC_LHEAP_DBLK, heap->dblk_addr, heap, flags)))
                H5E_THROW(H5E_CANTPROTECT, "unable to load heap data block");

            /* Set the flag for pinning the data block when unprotecting it */
            dblk_cache_flags |= H5AC__PIN_ENTRY_FLAG;
        } /* end if */
    } /* end if */

    /* Increment # of times heap is protected */
    heap->prots++;

    /* Set return value */
    ret_value = heap;

CATCH
    /* Release the prefix from the cache, now pinned */
    if(prfx && heap && H5AC_unprotect(f, H5AC_LHEAP_PRFX, heap->prfx_addr, prfx, prfx_cache_flags) < 0)
        H5E_THROW(H5E_CANTUNPROTECT, "unable to release local heap prefix");

    /* Release the data block from the cache, now pinned */
    if(dblk && heap && H5AC_unprotect(f, H5AC_LHEAP_DBLK, heap->dblk_addr, dblk, dblk_cache_flags) < 0)
        H5E_THROW(H5E_CANTUNPROTECT, "unable to release local heap data block");

END_FUNC(PRIV) /* end H5HL_protect() */


/*-------------------------------------------------------------------------
 * Function:    H5HL_offset_into
 *
 * Purpose:     Called directly after the call to H5HL_protect so that
 *              a pointer to the object in the heap can be obtained.
 *
 * Return:      Success:    Valid pointer.
 *              Failure:    Can't fail
 *
 * Programmer:  Bill Wendling
 *              Sept. 17, 2003
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PRIV, ERR,
void *, NULL, NULL,
H5HL_offset_into(const H5HL_t *heap, size_t offset))

    /* Sanity check */
    HDassert(heap);
    if(offset >= heap->dblk_size)
       H5E_THROW(H5E_CANTGET, "unable to offset into local heap data block");

    ret_value = heap->dblk_image + offset;

CATCH
    /* No special processing on errors */
END_FUNC(PRIV) /* end H5HL_offset_into() */

/*-------------------------------------------------------------------------
 * Function:    H5HL_unprotect
 *
 * Purpose:     Unprotect the data retrieved by the H5HL_protect call.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Bill Wendling
 *              Sept. 17, 2003
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PRIV, ERR,
herr_t, SUCCEED, FAIL,
H5HL_unprotect(H5HL_t *heap))

    /* check arguments */
    HDassert(heap);

    /* Decrement # of times heap is protected */
    heap->prots--;

    /* Check for last unprotection of heap */
    if(heap->prots == 0) {
        /* Check for separate heap data block */
        if(heap->single_cache_obj) {
            /* Mark local heap prefix as evictable again */
            if(FAIL == H5AC_unpin_entry(heap->prfx))
                H5E_THROW(H5E_CANTUNPIN, "unable to unpin local heap data block");
        } /* end if */
        else {
            /* Sanity check */
            HDassert(heap->dblk);

            /* Mark local heap data block as evictable again */
            /* (data block still pins prefix) */
            if(FAIL == H5AC_unpin_entry(heap->dblk))
                H5E_THROW(H5E_CANTUNPIN, "unable to unpin local heap data block");
        } /* end else */
    } /* end if */

CATCH
    /* No special processing on errors */

END_FUNC(PRIV) /* end H5HL_unprotect() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__remove_free
 *
 * Purpose:     Removes free list element FL from the specified heap and
 *              frees it.
 *
 * Return:      NULL
 *
 * Programmer:  Robb Matzke
 *              Jul 17 1997
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(STATIC, NOERR,
H5HL_free_t *, NULL, -,
H5HL__remove_free(H5HL_t *heap, H5HL_free_t *fl))

    if(fl->prev)
        fl->prev->next = fl->next;
    if(fl->next)
        fl->next->prev = fl->prev;

    if(!fl->prev)
        heap->freelist = fl->next;

    /* H5FL_FREE always returns NULL so we can't check for errors */
    ret_value = (H5HL_free_t *)H5FL_FREE(H5HL_free_t, fl);

END_FUNC(STATIC) /* end H5HL__remove_free() */


/*-------------------------------------------------------------------------
 * Function:    H5HL__dirty
 *
 * Purpose:     Mark heap as dirty
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              Oct 12 2008
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(STATIC, ERR,
herr_t, SUCCEED, FAIL,
H5HL__dirty(H5HL_t *heap))

    /* check arguments */
    HDassert(heap);
    HDassert(heap->prfx);

    /* Mark heap data block as dirty, if there is one */
    if(!heap->single_cache_obj) {
        /* Sanity check */
        HDassert(heap->dblk);

        if(FAIL == H5AC_mark_entry_dirty(heap->dblk))
            H5E_THROW(H5E_CANTMARKDIRTY, "unable to mark heap data block as dirty");
    } /* end if */

    /* Mark heap prefix as dirty */
    if(FAIL == H5AC_mark_entry_dirty(heap->prfx))
        H5E_THROW(H5E_CANTMARKDIRTY, "unable to mark heap prefix as dirty");

CATCH
    /* No special processing on errors */

END_FUNC(STATIC) /* end H5HL__dirty() */


/*-------------------------------------------------------------------------
 * Function:    H5HL_insert
 *
 * Purpose:     Inserts a new item into the heap.
 *
 * Return:      Success:    Offset of new item within heap.
 *              Failure:    UFAIL
 *
 * Programmer:  Robb Matzke
 *              Jul 17 1997
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PRIV, ERR,
size_t, UFAIL, UFAIL,
H5HL_insert(H5F_t *f, H5HL_t *heap, size_t buf_size, const void *buf))

    H5HL_free_t     *fl = NULL, *last_fl = NULL;
    size_t          offset = 0;
    size_t          need_size;
    hbool_t         found;

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
    if(FAIL == H5HL__dirty(heap))
        H5E_THROW(H5E_CANTMARKDIRTY, "unable to mark heap as dirty");

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
        if(fl->size > need_size && fl->size - need_size >= H5HL_SIZEOF_FREE(f)) {
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
            fl = H5HL__remove_free(heap, fl);
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
        htri_t  was_extended;   /* Whether the local heap's data segment on disk was extended */

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
        was_extended = H5MF_try_extend(f, H5FD_MEM_LHEAP, heap->dblk_addr, (hsize_t)(heap->dblk_size), (hsize_t)need_more);
        if(FAIL == was_extended)
            H5E_THROW(H5E_CANTEXTEND, "error trying to extend heap");

        /* Check if we extended the heap data block in file */
        if(was_extended == TRUE) {
            /* Check for prefix & data block contiguous */
            if(heap->single_cache_obj) {
                /* Resize prefix+data block */
                if(FAIL == H5AC_resize_entry(heap->prfx, (size_t)(heap->prfx_size + new_dblk_size)))
                    H5E_THROW(H5E_CANTRESIZE, "unable to resize heap prefix in cache");
            } /* end if */
            else {
                /* Resize 'standalone' data block */
                if(FAIL == H5AC_resize_entry(heap->dblk, (size_t)new_dblk_size))
                    H5E_THROW(H5E_CANTRESIZE, "unable to resize heap data block in cache");
            } /* end else */

            /* Note new size */
            heap->dblk_size = new_dblk_size;
        } /* end if */
        else { /* ...if we can't, allocate a new chunk & release the old */
            /* Reallocate data block in file */
            if(FAIL == H5HL__dblk_realloc(f, heap, new_dblk_size))
                H5E_THROW(H5E_CANTRESIZE, "reallocating data block failed");
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
                    HDfprintf(H5DEBUG(HL), "H5HL: lost %lu bytes at line %d\n",
                        (unsigned long)(last_fl->size), __LINE__);
                }
#endif
                last_fl = H5HL__remove_free(heap, last_fl);
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
                    H5E_THROW(H5E_CANTALLOC, "memory allocation failed");
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
                HDfprintf(H5DEBUG(HL), "H5HL_insert: lost %lu bytes at line %d\n",
                    (unsigned long)(need_more - need_size), __LINE__);
#endif
            }
        } /* end else */

#ifdef H5HL_DEBUG
        if (H5DEBUG(HL)) {
            HDfprintf(H5DEBUG(HL), "H5HL: resize mem buf from %lu to %lu bytes\n",
                (unsigned long)(old_dblk_size),
                (unsigned long)(old_dblk_size + need_more));
        }
#endif
        if(NULL == (heap->dblk_image = H5FL_BLK_REALLOC(lheap_chunk, heap->dblk_image, heap->dblk_size)))
            H5E_THROW(H5E_CANTALLOC, "memory allocation failed");

        /* Clear new section so junk doesn't appear in the file */
        /* (Avoid clearing section which will be overwritten with newly inserted data) */
        HDmemset(heap->dblk_image + offset + buf_size, 0, (new_dblk_size - (offset + buf_size)));
    } /* end if */

    /* Copy the data into the heap */
    HDmemcpy(heap->dblk_image + offset, buf, buf_size);

    /* Set return value */
    ret_value = offset;

CATCH
    /* No special processing on errors */

END_FUNC(PRIV) /* H5HL_insert() */


/*-------------------------------------------------------------------------
 * Function:    H5HL_remove
 *
 * Purpose:     Removes an object or part of an object from the heap at
 *              address ADDR of file F.	 The object (or part) to remove
 *              begins at byte OFFSET from the beginning of the heap and
 *              continues for SIZE bytes.
 *
 *              Once part of an object is removed, one must not attempt
 *              to access that part.  Removing the beginning of an object
 *              results in the object OFFSET increasing by the amount
 *              truncated.  Removing the end of an object results in
 *              object truncation.  Removing the middle of an object results
 *              in two separate objects, one at the original offset and
 *              one at the first offset past the removed portion.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Robb Matzke
 *              Jul 16 1997
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PRIV, ERR,
herr_t, SUCCEED, FAIL,
H5HL_remove(H5F_t *f, H5HL_t *heap, size_t offset, size_t size))

    H5HL_free_t *fl = NULL;

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
    if(FAIL == H5HL__dirty(heap))
        H5E_THROW(H5E_CANTMARKDIRTY, "unable to mark heap as dirty");

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
                    fl2 = H5HL__remove_free(heap, fl2);
                    if(((fl->offset + fl->size) == heap->dblk_size) &&
                             ((2 * fl->size) > heap->dblk_size)) {
                        if(FAIL == H5HL__minimize_heap_space(f, heap))
                            H5E_THROW(H5E_CANTFREE, "heap size minimization failed");
                    }
                    H5_LEAVE(SUCCEED);
                }
                fl2 = fl2->next;
            }
            if(((fl->offset + fl->size) == heap->dblk_size) &&
                     ((2 * fl->size) > heap->dblk_size)) {
                if(FAIL == H5HL__minimize_heap_space(f, heap))
                    H5E_THROW(H5E_CANTFREE, "heap size minimization failed");
            }
            H5_LEAVE(SUCCEED);
        } else if(fl->offset + fl->size == offset) {
            fl->size += size;
            fl2 = fl->next;
            HDassert(fl->size == H5HL_ALIGN(fl->size));
            while(fl2) {
                if(fl->offset + fl->size == fl2->offset) {
                    fl->size += fl2->size;
                    HDassert(fl->size == H5HL_ALIGN(fl->size));
                    fl2 = H5HL__remove_free(heap, fl2);
                    if(((fl->offset + fl->size) == heap->dblk_size) &&
                        ((2 * fl->size) > heap->dblk_size)) {
                        if(FAIL == H5HL__minimize_heap_space(f, heap))
                            H5E_THROW(H5E_CANTFREE, "heap size minimization failed");
                    } /* end if */
                    H5_LEAVE(SUCCEED);
                } /* end if */
                fl2 = fl2->next;
            } /* end while */
            if(((fl->offset + fl->size) == heap->dblk_size) &&
                    ((2 * fl->size) > heap->dblk_size)) {
                if(FAIL == H5HL__minimize_heap_space(f, heap))
                    H5E_THROW(H5E_CANTFREE, "heap size minimization failed");
            } /* end if */
            H5_LEAVE(SUCCEED);
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
            HDfprintf(H5DEBUG(HL), "H5HL: lost %lu bytes\n", (unsigned long) size);
        }
#endif
        H5_LEAVE(SUCCEED);
    } /* end if */

    /*
     * Add an entry to the free list.
     */
    if(NULL == (fl = H5FL_MALLOC(H5HL_free_t)))
        H5E_THROW(H5E_CANTALLOC, "memory allocation failed");
    fl->offset = offset;
    fl->size = size;
    HDassert(fl->offset == H5HL_ALIGN(fl->offset));
    HDassert(fl->size == H5HL_ALIGN(fl->size));
    fl->prev = NULL;
    fl->next = heap->freelist;
    if(heap->freelist)
        heap->freelist->prev = fl;
    heap->freelist = fl;

    if(((fl->offset + fl->size) == heap->dblk_size) && ((2 * fl->size) > heap->dblk_size))
        if(FAIL == H5HL__minimize_heap_space(f, heap))
            H5E_THROW(H5E_CANTFREE, "heap size minimization failed");

CATCH
    /* No special processing on exit */

END_FUNC(PRIV) /* end H5HL_remove() */


/*-------------------------------------------------------------------------
 * Function:    H5HL_delete
 *
 * Purpose:     Deletes a local heap from disk, freeing disk space used.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              Mar 22 2003
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PRIV, ERR,
herr_t, SUCCEED, FAIL,
H5HL_delete(H5F_t *f, haddr_t addr))

    H5HL_t      *heap = NULL;           /* Local heap to delete */
    H5HL_cache_prfx_ud_t prfx_udata;    /* User data for protecting local heap prefix */
    H5HL_prfx_t *prfx = NULL;           /* Local heap prefix */
    H5HL_dblk_t *dblk = NULL;           /* Local heap data block */
    unsigned    cache_flags = H5AC__NO_FLAGS_SET;       /* Flags for unprotecting heap */

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));

    /* Construct the user data for protect callback */
    prfx_udata.sizeof_size = H5F_SIZEOF_SIZE(f);
    prfx_udata.sizeof_addr = H5F_SIZEOF_ADDR(f);
    prfx_udata.prfx_addr = addr;
    prfx_udata.sizeof_prfx = H5HL_SIZEOF_HDR(f);

    /* Protect the local heap prefix */
    if(NULL == (prfx = (H5HL_prfx_t *)H5AC_protect(f, H5AC_LHEAP_PRFX, addr, &prfx_udata, H5AC__NO_FLAGS_SET)))
        H5E_THROW(H5E_CANTPROTECT, "unable to load heap prefix");

    /* Get the pointer to the heap */
    heap = prfx->heap;

    /* Check if heap has separate data block */
    if(!heap->single_cache_obj)
        /* Protect the local heap data block */
        if(NULL == (dblk = (H5HL_dblk_t *)H5AC_protect(f, H5AC_LHEAP_DBLK, heap->dblk_addr, heap, H5AC__NO_FLAGS_SET)))
            H5E_THROW(H5E_CANTPROTECT, "unable to load heap data block");

    /* Set the flags for releasing the prefix and data block */
    cache_flags |= H5AC__DIRTIED_FLAG | H5AC__DELETED_FLAG | H5AC__FREE_FILE_SPACE_FLAG;

CATCH
    /* Release the data block from the cache, now deleted */
    if(dblk && heap && H5AC_unprotect(f, H5AC_LHEAP_DBLK, heap->dblk_addr, dblk, cache_flags) < 0)
        H5E_THROW(H5E_CANTUNPROTECT, "unable to release local heap data block");

    /* Release the prefix from the cache, now deleted */
    if(prfx && heap && H5AC_unprotect(f, H5AC_LHEAP_PRFX, heap->prfx_addr, prfx, cache_flags) < 0)
        H5E_THROW(H5E_CANTUNPROTECT, "unable to release local heap prefix");

END_FUNC(PRIV) /* end H5HL_delete() */


/*-------------------------------------------------------------------------
 * Function:    H5HL_get_size
 *
 * Purpose:     Retrieves the current size of a heap
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              Nov  7 2005
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PRIV, ERR,
herr_t, SUCCEED, FAIL,
H5HL_get_size(H5F_t *f, haddr_t addr, size_t *size))

    H5HL_cache_prfx_ud_t prfx_udata;    /* User data for protecting local heap prefix */
    H5HL_prfx_t *prfx = NULL;           /* Local heap prefix */
    H5HL_t *heap;                       /* Heap data structure */

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
    if(NULL == (prfx = (H5HL_prfx_t *)H5AC_protect(f, H5AC_LHEAP_PRFX, addr, &prfx_udata, H5AC__READ_ONLY_FLAG)))
        H5E_THROW(H5E_CANTPROTECT, "unable to load heap prefix");

    /* Get the pointer to the heap */
    heap = prfx->heap;

    /* Set the size to return */
    *size = heap->dblk_size;

CATCH
    if(prfx && FAIL == H5AC_unprotect(f, H5AC_LHEAP_PRFX, heap->prfx_addr, prfx, H5AC__NO_FLAGS_SET))
        H5E_THROW(H5E_CANTUNPROTECT, "unable to release local heap prefix");

END_FUNC(PRIV) /* end H5HL_get_size() */


/*-------------------------------------------------------------------------
 * Function:    H5HL_heapsize
 *
 * Purpose:     Compute the size in bytes of the specified instance of
 *              H5HL_t via H5HL_size()
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Vailin Choi
 *              June 19 2007
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PRIV, ERR,
herr_t, SUCCEED, FAIL,
H5HL_heapsize(H5F_t *f, haddr_t addr, hsize_t *heap_size))

    H5HL_cache_prfx_ud_t prfx_udata;    /* User data for protecting local heap prefix */
    H5HL_prfx_t *prfx = NULL;           /* Local heap prefix */
    H5HL_t *heap;                       /* Heap data structure */

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
    if(NULL == (prfx = (H5HL_prfx_t *)H5AC_protect(f, H5AC_LHEAP_PRFX, addr, &prfx_udata, H5AC__READ_ONLY_FLAG)))
        H5E_THROW(H5E_CANTPROTECT, "unable to load heap prefix");

    /* Get the pointer to the heap */
    heap = prfx->heap;

    /* Accumulate the size of the local heap */
    *heap_size += (hsize_t)(heap->prfx_size + heap->dblk_size);

CATCH
    if(prfx && FAIL == H5AC_unprotect(f, H5AC_LHEAP_PRFX, heap->prfx_addr, prfx, H5AC__NO_FLAGS_SET))
        H5E_THROW(H5E_CANTUNPROTECT, "unable to release local heap prefix");

END_FUNC(PRIV) /* end H5HL_heapsize() */

