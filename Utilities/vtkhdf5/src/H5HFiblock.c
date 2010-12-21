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
 * Created:		H5HFiblock.c
 *			Apr 10 2006
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Indirect block routines for fractal heaps.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5HF_PACKAGE		/*suppress error about including H5HFpkg  */

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* File access				*/
#include "H5HFpkg.h"		/* Fractal heaps			*/
#include "H5MFprivate.h"	/* File memory management		*/
#include "H5Vprivate.h"		/* Vectors and arrays 			*/

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
static herr_t H5HF_iblock_pin(H5HF_indirect_t *iblock);
static herr_t H5HF_iblock_unpin(H5HF_indirect_t *iblock);
static herr_t H5HF_man_iblock_root_halve(H5HF_indirect_t *root_iblock, hid_t dxpl_id);
static herr_t H5HF_man_iblock_root_revert(H5HF_indirect_t *root_iblock, hid_t dxpl_id);


/*********************/
/* Package Variables */
/*********************/

/* Declare a free list to manage the H5HF_indirect_t struct */
H5FL_DEFINE(H5HF_indirect_t);

/* Declare a free list to manage the H5HF_indirect_ent_t sequence information */
H5FL_SEQ_DEFINE(H5HF_indirect_ent_t);

/* Declare a free list to manage the H5HF_indirect_filt_ent_t sequence information */
H5FL_SEQ_DEFINE(H5HF_indirect_filt_ent_t);

/* Declare a free list to manage the H5HF_indirect_t * sequence information */
H5FL_SEQ_DEFINE(H5HF_indirect_ptr_t);


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5HF_iblock_pin
 *
 * Purpose:	Pin an indirect block in memory
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Aug 17 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_iblock_pin(H5HF_indirect_t *iblock)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_iblock_pin)

    /* Sanity checks */
    HDassert(iblock);

    /* Mark block as un-evictable */
    if(H5AC_pin_protected_entry(iblock) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTPIN, FAIL, "unable to pin fractal heap indirect block")

    /* If this indirect block has a parent, update it's child iblock pointer */
    if(iblock->parent) {
        H5HF_indirect_t *par_iblock = iblock->parent;    /* Parent indirect block */
        unsigned indir_idx;             /* Index in parent's child iblock pointer array */

        /* Sanity check */
        HDassert(par_iblock->child_iblocks);
        HDassert(iblock->par_entry >= (iblock->hdr->man_dtable.max_direct_rows
                * iblock->hdr->man_dtable.cparam.width));

        /* Compute index in parent's child iblock pointer array */
        indir_idx = iblock->par_entry - (iblock->hdr->man_dtable.max_direct_rows
                * iblock->hdr->man_dtable.cparam.width);

        /* Set pointer to pinned indirect block in parent */
        HDassert(par_iblock->child_iblocks[indir_idx] == NULL);
        par_iblock->child_iblocks[indir_idx] = iblock;
    } /* end if */
    else {
        /* Check for pinning the root indirect block */
        if(iblock->block_off == 0) {
            HDassert(iblock->hdr->root_iblock == NULL);
            iblock->hdr->root_iblock = iblock;
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_iblock_pin() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_iblock_unpin
 *
 * Purpose:	Unpin an indirect block in the metadata cache
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Aug 17 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_iblock_unpin(H5HF_indirect_t *iblock)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_iblock_unpin)

    /* Sanity check */
    HDassert(iblock);

    /* If this indirect block has a parent, reset it's child iblock pointer */
    if(iblock->parent) {
        H5HF_indirect_t *par_iblock = iblock->parent;    /* Parent indirect block */
        unsigned indir_idx;             /* Index in parent's child iblock pointer array */

        /* Sanity check */
        HDassert(par_iblock->child_iblocks);
        HDassert(iblock->par_entry >= (iblock->hdr->man_dtable.max_direct_rows
                * iblock->hdr->man_dtable.cparam.width));

        /* Compute index in parent's child iblock pointer array */
        indir_idx = iblock->par_entry - (iblock->hdr->man_dtable.max_direct_rows
                * iblock->hdr->man_dtable.cparam.width);

        /* Reset pointer to pinned child indirect block in parent */
        HDassert(par_iblock->child_iblocks[indir_idx]);
        par_iblock->child_iblocks[indir_idx] = NULL;
    } /* end if */
    else {
        /* Check for unpinning the root indirect block */
        if(iblock->block_off == 0) {
            HDassert(iblock->hdr->root_iblock);
            iblock->hdr->root_iblock = NULL;
        } /* end if */
    } /* end if */

    /* Mark block as evictable again */
    if(H5AC_unpin_entry(iblock) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTUNPIN, FAIL, "unable to unpin fractal heap indirect block")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_iblock_unpin() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_iblock_incr
 *
 * Purpose:	Increment reference count on shared indirect block
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar 27 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_iblock_incr(H5HF_indirect_t *iblock)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_iblock_incr)

    /* Sanity checks */
    HDassert(iblock);
    HDassert(iblock->block_off == 0 || iblock->parent);

    /* Mark block as un-evictable when a child block is depending on it */
    if(iblock->rc == 0)
        if(H5HF_iblock_pin(iblock) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTPIN, FAIL, "unable to pin fractal heap indirect block")

    /* Increment reference count on shared indirect block */
    iblock->rc++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_iblock_incr() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_iblock_decr
 *
 * Purpose:	Decrement reference count on shared indirect block
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar 27 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_iblock_decr(H5HF_indirect_t *iblock)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_iblock_decr)

    /* Sanity check */
    HDassert(iblock);

    /* Decrement reference count on shared indirect block */
    iblock->rc--;

    /* Mark block as evictable again when no child blocks depend on it */
    if(iblock->rc == 0) {
        if(H5HF_iblock_unpin(iblock) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTUNPIN, FAIL, "unable to unpin fractal heap indirect block")

        if(iblock->nchildren == 0) {
            /* Check for deleting root indirect block (and no root direct block) */
            if(iblock->block_off == 0 && iblock->hdr->man_dtable.curr_root_rows > 0) {
                /* Reset root pointer information */
                iblock->hdr->man_dtable.curr_root_rows = 0;
                iblock->hdr->man_dtable.table_addr = HADDR_UNDEF;

                /* Reset header information back to "empty heap" state */
                if(H5HF_hdr_empty(iblock->hdr) < 0)
                    HGOTO_ERROR(H5E_HEAP, H5E_CANTSHRINK, FAIL, "can't make heap empty")
            } /* end if */

            /* Detach from parent indirect block */
            if(iblock->parent) {
                /* Detach from parent indirect block */
                if(H5HF_man_iblock_detach(iblock->parent, H5AC_dxpl_id, iblock->par_entry) < 0)
                    HGOTO_ERROR(H5E_HEAP, H5E_CANTATTACH, FAIL, "can't detach from parent indirect block")
                iblock->parent = NULL;
                iblock->par_entry = 0;
            } /* end if */

            /* Evict the indirect block from the metadata cache */
            if(H5AC_expunge_entry(iblock->hdr->f, H5AC_dxpl_id, H5AC_FHEAP_IBLOCK, iblock->addr, H5AC__FREE_FILE_SPACE_FLAG) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTREMOVE, FAIL, "unable to remove indirect block from cache")
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_iblock_decr() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_iblock_dirty
 *
 * Purpose:	Mark indirect block as dirty
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar 21 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_iblock_dirty(H5HF_indirect_t *iblock)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_iblock_dirty)

    /* Sanity check */
    HDassert(iblock);

    /* Mark indirect block as dirty in cache */
    if(H5AC_mark_entry_dirty(iblock) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTMARKDIRTY, FAIL, "unable to mark fractal heap indirect block as dirty")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_iblock_dirty() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_man_iblock_root_create
 *
 * Purpose:	Create root indirect block
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May  2 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_man_iblock_root_create(H5HF_hdr_t *hdr, hid_t dxpl_id, size_t min_dblock_size)
{
    H5HF_indirect_t *iblock;        /* Pointer to indirect block */
    haddr_t iblock_addr;            /* Indirect block's address */
    hsize_t acc_dblock_free;        /* Accumulated free space in direct blocks */
    hbool_t have_direct_block;      /* Flag to indicate a direct block already exists */
    hbool_t did_protect;            /* Whether we protected the indirect block or not */
    unsigned nrows;                 /* Number of rows for root indirect block */
    unsigned u;                     /* Local index variable */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_man_iblock_root_create)

    /* Check for allocating entire root indirect block initially */
    if(hdr->man_dtable.cparam.start_root_rows == 0)
        nrows = hdr->man_dtable.max_root_rows;
    else {
        unsigned rows_needed;   /* Number of rows needed to get to direct block size */
        unsigned block_row_off; /* Row offset from larger block sizes */

        nrows = hdr->man_dtable.cparam.start_root_rows;

        block_row_off = H5V_log2_of2((uint32_t)min_dblock_size) - H5V_log2_of2((uint32_t)hdr->man_dtable.cparam.start_block_size);
        if(block_row_off > 0)
            block_row_off++;        /* Account for the pair of initial rows of the initial block size */
        rows_needed = 1 + block_row_off;
        if(nrows < rows_needed)
            nrows = rows_needed;
    } /* end else */

    /* Allocate root indirect block */
    if(H5HF_man_iblock_create(hdr, dxpl_id, NULL, 0, nrows, hdr->man_dtable.max_root_rows, &iblock_addr) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, FAIL, "can't allocate fractal heap indirect block")

    /* Move current direct block (used as root) into new indirect block */

    /* Lock new indirect block */
    if(NULL == (iblock = H5HF_man_iblock_protect(hdr, dxpl_id, iblock_addr, nrows, NULL, 0, FALSE, H5AC_WRITE, &did_protect)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, FAIL, "unable to protect fractal heap indirect block")

    /* Check if there's already a direct block as root) */
    have_direct_block = H5F_addr_defined(hdr->man_dtable.table_addr);
    if(have_direct_block) {
        H5HF_direct_t *dblock;          /* Pointer to direct block to query */

        /* Lock first (root) direct block */
        if(NULL == (dblock = H5HF_man_dblock_protect(hdr, dxpl_id, hdr->man_dtable.table_addr, hdr->man_dtable.cparam.start_block_size, NULL, 0, H5AC_WRITE)))
            HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, FAIL, "unable to protect fractal heap direct block")

        /* Attach direct block to new root indirect block */
        dblock->parent = iblock;
        dblock->par_entry = 0;
        if(H5HF_man_iblock_attach(iblock, 0, hdr->man_dtable.table_addr) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTATTACH, FAIL, "can't attach root direct block to parent indirect block")

        /* Check for I/O filters on this heap */
        if(hdr->filter_len > 0) {
            /* Set the pipeline filter information from the header */
            iblock->filt_ents[0].size = hdr->pline_root_direct_size;
            iblock->filt_ents[0].filter_mask = hdr->pline_root_direct_filter_mask;

            /* Reset the header's pipeline information */
            hdr->pline_root_direct_size = 0;
            hdr->pline_root_direct_filter_mask = 0;
        } /* end if */

        /* Unlock first (previously the root) direct block */
        if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_FHEAP_DBLOCK, hdr->man_dtable.table_addr, dblock, H5AC__NO_FLAGS_SET) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTUNPROTECT, FAIL, "unable to release fractal heap direct block")
        dblock = NULL;
    } /* end if */

    /* Start iterator at correct location */
    if(H5HF_hdr_start_iter(hdr, iblock, (hsize_t)(have_direct_block ? hdr->man_dtable.cparam.start_block_size : 0), have_direct_block) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, FAIL, "can't initialize block iterator")

    /* Check for skipping over direct blocks, in order to get to large enough block */
    if(min_dblock_size > hdr->man_dtable.cparam.start_block_size) {
        /* Add skipped blocks to heap's free space */
        if(H5HF_hdr_skip_blocks(hdr, dxpl_id, iblock, have_direct_block,
                ((nrows - 1) * hdr->man_dtable.cparam.width) - have_direct_block) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTDEC, FAIL, "can't add skipped blocks to heap's free space")
    } /* end if */

    /* Mark indirect block as modified */
    if(H5HF_iblock_dirty(iblock) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTDIRTY, FAIL, "can't mark indirect block as dirty")

    /* Unprotect root indirect block (it's pinned by the iterator though) */
    if(H5HF_man_iblock_unprotect(iblock, dxpl_id, H5AC__DIRTIED_FLAG, did_protect) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTUNPROTECT, FAIL, "unable to release fractal heap indirect block")
    iblock = NULL;

    /* Point heap header at new indirect block */
    hdr->man_dtable.curr_root_rows = nrows;
    hdr->man_dtable.table_addr = iblock_addr;

    /* Compute free space in direct blocks referenced from entries in root indirect block */
    acc_dblock_free = 0;
    for(u = 0; u < nrows; u++)
        acc_dblock_free += hdr->man_dtable.row_tot_dblock_free[u] * hdr->man_dtable.cparam.width;

    /* Account for potential initial direct block */
    if(have_direct_block)
        acc_dblock_free -= hdr->man_dtable.row_tot_dblock_free[0];

    /* Extend heap to cover new root indirect block */
    if(H5HF_hdr_adjust_heap(hdr, hdr->man_dtable.row_block_off[nrows], (hssize_t)acc_dblock_free) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTEXTEND, FAIL, "can't increase space to cover root direct block")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_man_iblock_root_create() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_man_iblock_root_double
 *
 * Purpose:	Double size of root indirect block
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Apr 17 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_man_iblock_root_double(H5HF_hdr_t *hdr, hid_t dxpl_id, size_t min_dblock_size)
{
    H5HF_indirect_t *iblock;    /* Pointer to root indirect block */
    haddr_t new_addr;           /* New address of indirect block */
    hsize_t acc_dblock_free;    /* Accumulated free space in direct blocks */
    hsize_t next_size;          /* The previous value of the "next size" for the new block iterator */
    hsize_t old_iblock_size;    /* Old size of indirect block */
    unsigned next_row;          /* The next row to allocate block in */
    unsigned next_entry;        /* The previous value of the "next entry" for the new block iterator */
    unsigned new_next_entry = 0;/* The new value of the "next entry" for the new block iterator */
    unsigned min_nrows = 0;     /* Min. # of direct rows */
    unsigned old_nrows;         /* Old # of rows */
    unsigned new_nrows;         /* New # of rows */
    hbool_t skip_direct_rows = FALSE;   /* Whether we are skipping direct rows */
    size_t u;                   /* Local index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_man_iblock_root_double)

    /* Get "new block" iterator information */
    if(H5HF_man_iter_curr(&hdr->next_block, &next_row, NULL, &next_entry, &iblock) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTGET, FAIL, "unable to retrieve current block iterator location")
    next_size = hdr->man_dtable.row_block_size[next_row];

    /* Make certain the iterator is at the root indirect block */
    HDassert(iblock->parent == NULL);
    HDassert(iblock->block_off == 0);

    /* Keep this for later */
    old_nrows = iblock->nrows;

    /* Check for skipping over direct block rows */
    if(iblock->nrows < hdr->man_dtable.max_direct_rows && min_dblock_size > next_size) {
        /* Sanity check */
        HDassert(min_dblock_size > hdr->man_dtable.cparam.start_block_size);

        /* Set flag */
        skip_direct_rows = TRUE;

        /* Make certain we allocate at least the required row for the block requested */
        min_nrows = 1 + H5HF_dtable_size_to_row(&hdr->man_dtable, min_dblock_size);

        /* Set the information for the next block, of the appropriate size */
        new_next_entry = (min_nrows - 1) * hdr->man_dtable.cparam.width;
    } /* end if */

    /* Compute new # of rows in indirect block */
    new_nrows = MAX(min_nrows, MIN(2 * iblock->nrows, iblock->max_rows));

    /* Check if the indirect block is NOT currently allocated in temp. file space */
    /* (temp. file space does not need to be freed) */
    if(!H5F_IS_TMP_ADDR(hdr->f, iblock->addr)) {
/* Currently, the old block data is "thrown away" after the space is reallocated,
* to avoid data copy in H5MF_realloc() call by just free'ing the space and
* allocating new space.
*
* This also keeps the file smaller, by freeing the space and then
* allocating new space, instead of vice versa (in H5MF_realloc).
*
* QAK - 3/14/2006
*/
        /* Free previous indirect block disk space */
        if(H5MF_xfree(hdr->f, H5FD_MEM_FHEAP_IBLOCK, dxpl_id, iblock->addr, (hsize_t)iblock->size) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "unable to free fractal heap indirect block file space")
    } /* end if */

    /* Compute size of buffer needed for new indirect block */
    iblock->nrows = new_nrows;
    old_iblock_size = iblock->size;
    iblock->size = H5HF_MAN_INDIRECT_SIZE(hdr, iblock);

    /* Allocate [temporary] space for the new indirect block on disk */
    if(H5F_USE_TMP_SPACE(hdr->f)) {
        if(HADDR_UNDEF == (new_addr = H5MF_alloc_tmp(hdr->f, (hsize_t)iblock->size)))
            HGOTO_ERROR(H5E_HEAP, H5E_NOSPACE, FAIL, "file allocation failed for fractal heap indirect block")
    } /* end if */
    else {
        if(HADDR_UNDEF == (new_addr = H5MF_alloc(hdr->f, H5FD_MEM_FHEAP_IBLOCK, dxpl_id, (hsize_t)iblock->size)))
            HGOTO_ERROR(H5E_HEAP, H5E_NOSPACE, FAIL, "file allocation failed for fractal heap indirect block")
    } /* end else */

    /* Resize pinned indirect block in the cache, if its changed size */
    if(old_iblock_size != iblock->size) {
        if(H5AC_resize_entry(iblock, (size_t)iblock->size) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTRESIZE, FAIL, "unable to resize fractal heap indirect block")
    } /* end if */

    /* Move object in cache, if it actually was relocated */
    if(H5F_addr_ne(iblock->addr, new_addr)) {
        if(H5AC_move_entry(hdr->f, H5AC_FHEAP_IBLOCK, iblock->addr, new_addr) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTMOVE, FAIL, "unable to move fractal heap root indirect block")
        iblock->addr = new_addr;
    } /* end if */

    /* Re-allocate child block entry array */
    if(NULL == (iblock->ents = H5FL_SEQ_REALLOC(H5HF_indirect_ent_t, iblock->ents, (size_t)(iblock->nrows * hdr->man_dtable.cparam.width))))
        HGOTO_ERROR(H5E_HEAP, H5E_NOSPACE, FAIL, "memory allocation failed for direct entries")

    /* Check for skipping over rows and add free section for skipped rows */
    if(skip_direct_rows) {
        /* Add skipped blocks to heap's free space */
        if(H5HF_hdr_skip_blocks(hdr, dxpl_id, iblock, next_entry, (new_next_entry - next_entry)) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTDEC, FAIL, "can't add skipped blocks to heap's free space")
    } /* end if */

    /* Initialize new direct block entries in rows added */
    acc_dblock_free = 0;
    for(u = (old_nrows * hdr->man_dtable.cparam.width); u < (iblock->nrows * hdr->man_dtable.cparam.width); u++) {
        unsigned row = u / hdr->man_dtable.cparam.width;        /* Row for current entry */

        iblock->ents[u].addr = HADDR_UNDEF;
        acc_dblock_free += hdr->man_dtable.row_tot_dblock_free[row];
    } /* end for */

    /* Check for needing to re-allocate filtered entry array */
    if(hdr->filter_len > 0 && old_nrows < hdr->man_dtable.max_direct_rows) {
        unsigned dir_rows;      /* Number of direct rows in this indirect block */

        /* Compute the number of direct rows for this indirect block */
        dir_rows = MIN(iblock->nrows, hdr->man_dtable.max_direct_rows);
        HDassert(dir_rows > old_nrows);

        /* Re-allocate filtered direct block entry array */
        if(NULL == (iblock->filt_ents = H5FL_SEQ_REALLOC(H5HF_indirect_filt_ent_t, iblock->filt_ents, (size_t)(dir_rows * hdr->man_dtable.cparam.width))))
            HGOTO_ERROR(H5E_HEAP, H5E_NOSPACE, FAIL, "memory allocation failed for filtered direct entries")

        /* Initialize new entries allocated */
        for(u = (old_nrows * hdr->man_dtable.cparam.width); u < (dir_rows * hdr->man_dtable.cparam.width); u++) {
            iblock->filt_ents[u].size = 0;
            iblock->filt_ents[u].filter_mask = 0;
        } /* end for */
    } /* end if */

    /* Check for needing to re-allocate child iblock pointer array */
    if(iblock->nrows > hdr->man_dtable.max_direct_rows) {
        unsigned indir_rows;      /* Number of indirect rows in this indirect block */
        unsigned old_indir_rows;  /* Previous number of indirect rows in this indirect block */

        /* Compute the number of direct rows for this indirect block */
        indir_rows = iblock->nrows - hdr->man_dtable.max_direct_rows;

        /* Re-allocate child indirect block array */
        if(NULL == (iblock->child_iblocks = H5FL_SEQ_REALLOC(H5HF_indirect_ptr_t, iblock->child_iblocks, (size_t)(indir_rows * hdr->man_dtable.cparam.width))))
            HGOTO_ERROR(H5E_HEAP, H5E_NOSPACE, FAIL, "memory allocation failed for filtered direct entries")

        /* Compute the previous # of indirect rows in this block */
        if(old_nrows < hdr->man_dtable.max_direct_rows)
            old_indir_rows = 0;
        else
            old_indir_rows = old_nrows - hdr->man_dtable.max_direct_rows;

        /* Initialize new entries allocated */
        for(u = (old_indir_rows * hdr->man_dtable.cparam.width); u < (indir_rows * hdr->man_dtable.cparam.width); u++)
            iblock->child_iblocks[u] = NULL;
    } /* end if */

    /* Mark indirect block as dirty */
    if(H5HF_iblock_dirty(iblock) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTDIRTY, FAIL, "can't mark indirect block as dirty")

    /* Update other shared header info */
    hdr->man_dtable.curr_root_rows = new_nrows;
    hdr->man_dtable.table_addr = new_addr;

    /* Extend heap to cover new root indirect block */
    if(H5HF_hdr_adjust_heap(hdr, 2 * hdr->man_dtable.row_block_off[new_nrows - 1], (hssize_t)acc_dblock_free) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTEXTEND, FAIL, "can't increase space to cover root direct block")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_man_iblock_root_double() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_man_iblock_root_halve
 *
 * Purpose:	Halve size of root indirect block
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Jun 12 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_man_iblock_root_halve(H5HF_indirect_t *iblock, hid_t dxpl_id)
{
    H5HF_hdr_t *hdr = iblock->hdr;      /* Pointer to heap header */
    haddr_t new_addr;                   /* New address of indirect block */
    hsize_t acc_dblock_free;            /* Accumulated free space in direct blocks */
    hsize_t old_size;                   /* Old size of indirect block */
    unsigned max_child_row;             /* Row for max. child entry */
    unsigned old_nrows;                 /* Old # of rows */
    unsigned new_nrows;                 /* New # of rows */
    unsigned u;                         /* Local index variable */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_man_iblock_root_halve)

    /* Sanity check */
    HDassert(iblock);
    HDassert(iblock->parent == NULL);
    HDassert(hdr);

    /* Compute maximum row used by child of indirect block */
    max_child_row = iblock->max_child / hdr->man_dtable.cparam.width;

    /* Compute new # of rows in root indirect block */
    new_nrows = 1 << (1 + H5V_log2_gen((uint64_t)max_child_row));

    /* Check if the indirect block is NOT currently allocated in temp. file space */
    /* (temp. file space does not need to be freed) */
    if(!H5F_IS_TMP_ADDR(hdr->f, iblock->addr)) {
/* Currently, the old block data is "thrown away" after the space is reallocated,
* to avoid data copy in H5MF_realloc() call by just free'ing the space and
* allocating new space.
*
* This also keeps the file smaller, by freeing the space and then
* allocating new space, instead of vice versa (in H5MF_realloc).
*
* QAK - 6/12/2006
*/
        /* Free previous indirect block disk space */
        if(H5MF_xfree(hdr->f, H5FD_MEM_FHEAP_IBLOCK, dxpl_id, iblock->addr, (hsize_t)iblock->size) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "unable to free fractal heap indirect block file space")
    } /* end if */

    /* Compute free space in rows to delete */
    acc_dblock_free = 0;
    for(u = new_nrows; u < iblock->nrows; u++)
        acc_dblock_free += hdr->man_dtable.row_tot_dblock_free[u] * hdr->man_dtable.cparam.width;

    /* Compute size of buffer needed for new indirect block */
    old_nrows = iblock->nrows;
    iblock->nrows = new_nrows;
    old_size = iblock->size;
    iblock->size = H5HF_MAN_INDIRECT_SIZE(hdr, iblock);

    /* Allocate [temporary] space for the new indirect block on disk */
    if(H5F_USE_TMP_SPACE(hdr->f)) {
        if(HADDR_UNDEF == (new_addr = H5MF_alloc_tmp(hdr->f, (hsize_t)iblock->size)))
            HGOTO_ERROR(H5E_HEAP, H5E_NOSPACE, FAIL, "file allocation failed for fractal heap indirect block")
    } /* end if */
    else {
        if(HADDR_UNDEF == (new_addr = H5MF_alloc(hdr->f, H5FD_MEM_FHEAP_IBLOCK, dxpl_id, (hsize_t)iblock->size)))
            HGOTO_ERROR(H5E_HEAP, H5E_NOSPACE, FAIL, "file allocation failed for fractal heap indirect block")
    } /* end else */

    /* Resize pinned indirect block in the cache, if it has changed size */
    if(old_size != iblock->size) {
        if(H5AC_resize_entry(iblock, (size_t)iblock->size) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTRESIZE, FAIL, "unable to resize fractal heap indirect block")
    } /* end if */

    /* Move object in cache, if it actually was relocated */
    if(H5F_addr_ne(iblock->addr, new_addr)) {
        if(H5AC_move_entry(hdr->f, H5AC_FHEAP_IBLOCK, iblock->addr, new_addr) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTSPLIT, FAIL, "unable to move fractal heap root indirect block")
        iblock->addr = new_addr;
    } /* end if */

    /* Re-allocate child block entry array */
    if(NULL == (iblock->ents = H5FL_SEQ_REALLOC(H5HF_indirect_ent_t, iblock->ents, (size_t)(iblock->nrows * hdr->man_dtable.cparam.width))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for direct entries")

    /* Check for needing to re-allocate filtered entry array */
    if(hdr->filter_len > 0 && new_nrows < hdr->man_dtable.max_direct_rows) {
        /* Re-allocate filtered direct block entry array */
        if(NULL == (iblock->filt_ents = H5FL_SEQ_REALLOC(H5HF_indirect_filt_ent_t, iblock->filt_ents, (size_t)(iblock->nrows * hdr->man_dtable.cparam.width))))
            HGOTO_ERROR(H5E_HEAP, H5E_NOSPACE, FAIL, "memory allocation failed for filtered direct entries")
    } /* end if */

    /* Check for needing to re-allocate child iblock pointer array */
    if(old_nrows > hdr->man_dtable.max_direct_rows) {
        /* Check for shrinking away child iblock pointer array */
        if(iblock->nrows > hdr->man_dtable.max_direct_rows) {
            unsigned indir_rows;      /* Number of indirect rows in this indirect block */

            /* Compute the number of direct rows for this indirect block */
            indir_rows = iblock->nrows - hdr->man_dtable.max_direct_rows;

            /* Re-allocate child indirect block array */
            if(NULL == (iblock->child_iblocks = H5FL_SEQ_REALLOC(H5HF_indirect_ptr_t, iblock->child_iblocks, (size_t)(indir_rows * hdr->man_dtable.cparam.width))))
                HGOTO_ERROR(H5E_HEAP, H5E_NOSPACE, FAIL, "memory allocation failed for filtered direct entries")
        } /* end if */
        else
            iblock->child_iblocks = (H5HF_indirect_ptr_t *)H5FL_SEQ_FREE(H5HF_indirect_ptr_t, iblock->child_iblocks);
    } /* end if */

    /* Mark indirect block as dirty */
    if(H5HF_iblock_dirty(iblock) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTDIRTY, FAIL, "can't mark indirect block as dirty")

    /* Update other shared header info */
    hdr->man_dtable.curr_root_rows = new_nrows;
    hdr->man_dtable.table_addr = new_addr;

    /* Shrink heap to only cover new root indirect block */
    if(H5HF_hdr_adjust_heap(hdr, 2 * hdr->man_dtable.row_block_off[new_nrows - 1], -(hssize_t)acc_dblock_free) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTSHRINK, FAIL, "can't reduce space to cover root direct block")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_man_iblock_root_halve() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_man_iblock_root_revert
 *
 * Purpose:	Revert root indirect block back to root direct block
 *
 * Note:	Any sections left pointing to the  old root indirect block
 *              will be cleaned up by the free space manager
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May 31 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5HF_man_iblock_root_revert(H5HF_indirect_t *root_iblock, hid_t dxpl_id)
{
    H5HF_hdr_t *hdr;                    /* Pointer to heap's header */
    H5HF_direct_t *dblock = NULL;       /* Pointer to new root indirect block */
    haddr_t dblock_addr;                /* Direct block's address in the file */
    size_t dblock_size;                 /* Direct block's size */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_man_iblock_root_revert)

    /*
     * Check arguments.
     */
    HDassert(root_iblock);

    /* Set up local convenience variables */
    hdr = root_iblock->hdr;
    dblock_addr = root_iblock->ents[0].addr;
    dblock_size = hdr->man_dtable.cparam.start_block_size;

    /* Get pointer to last direct block */
    if(NULL == (dblock = H5HF_man_dblock_protect(hdr, dxpl_id, dblock_addr, dblock_size, root_iblock, 0, H5AC_WRITE)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, FAIL, "unable to protect fractal heap direct block")
    HDassert(dblock->parent == root_iblock);
    HDassert(dblock->par_entry == 0);

    /* Check for I/O filters on this heap */
    if(hdr->filter_len > 0) {
        /* Set the header's pipeline information from the indirect block */
        hdr->pline_root_direct_size = root_iblock->filt_ents[0].size;
        hdr->pline_root_direct_filter_mask = root_iblock->filt_ents[0].filter_mask;
    } /* end if */

    /* Detach direct block from parent */
    if(H5HF_man_iblock_detach(dblock->parent, dxpl_id, 0) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTATTACH, FAIL, "can't detach direct block from parent indirect block")
    dblock->parent = NULL;
    dblock->par_entry = 0;

    /* Point root at direct block */
    hdr->man_dtable.curr_root_rows = 0;
    hdr->man_dtable.table_addr = dblock_addr;

    /* Reset 'next block' iterator */
    if(H5HF_hdr_reset_iter(hdr, (hsize_t)dblock_size) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTRELEASE, FAIL, "can't reset block iterator")

    /* Extend heap to just cover first direct block */
    if(H5HF_hdr_adjust_heap(hdr, (hsize_t)hdr->man_dtable.cparam.start_block_size, (hssize_t)hdr->man_dtable.row_tot_dblock_free[0]) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTEXTEND, FAIL, "can't increase space to cover root direct block")

done:
    if(dblock && H5AC_unprotect(hdr->f, dxpl_id, H5AC_FHEAP_DBLOCK, dblock_addr, dblock, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_HEAP, H5E_CANTUNPROTECT, FAIL, "unable to release fractal heap direct block")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_man_iblock_root_revert() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_man_iblock_alloc_row
 *
 * Purpose:	Allocate a "single" section for an object, out of a
 *              "row" section.
 *
 * Note:	Creates necessary direct & indirect blocks
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		July  6 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_man_iblock_alloc_row(H5HF_hdr_t *hdr, hid_t dxpl_id, H5HF_free_section_t **sec_node)
{
    H5HF_indirect_t *iblock = NULL;     /* Pointer to indirect block */
    H5HF_free_section_t *old_sec_node = *sec_node; /* Pointer to old indirect section node */
    unsigned dblock_entry;              /* Entry for direct block */
    hbool_t iblock_held = FALSE;        /* Flag to indicate that indirect block is held */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_man_iblock_alloc_row)

    /*
     * Check arguments.
     */
    HDassert(hdr);
    HDassert(sec_node && old_sec_node);
    HDassert(old_sec_node->u.row.row < hdr->man_dtable.max_direct_rows);

    /* Check for serialized section */
    if(old_sec_node->sect_info.state == H5FS_SECT_SERIALIZED) {
        /* Revive indirect section */
        if(H5HF_sect_row_revive(hdr, dxpl_id, old_sec_node) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTREVIVE, FAIL, "can't revive indirect section")
    } /* end if */

    /* Get a pointer to the indirect block covering the section */
    if(NULL == (iblock = H5HF_sect_row_get_iblock(old_sec_node)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTGET, FAIL, "can't retrieve indirect block for row section")

    /* Hold indirect block in memory, until direct block can point to it */
    if(H5HF_iblock_incr(iblock) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTINC, FAIL, "can't increment reference count on shared indirect block")
    iblock_held = TRUE;

    /* Reduce (& possibly re-add) 'row' section */
    if(H5HF_sect_row_reduce(hdr, dxpl_id, old_sec_node, &dblock_entry) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTSHRINK, FAIL, "can't reduce row section node")

    /* Create direct block & single section */
    if(H5HF_man_dblock_create(dxpl_id, hdr, iblock, dblock_entry, NULL, sec_node) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, FAIL, "can't allocate fractal heap direct block")

done:
    /* Release hold on indirect block */
    if(iblock_held)
        if(H5HF_iblock_decr(iblock) < 0)
            HDONE_ERROR(H5E_HEAP, H5E_CANTDEC, FAIL, "can't decrement reference count on shared indirect block")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_man_iblock_alloc_row() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_man_iblock_create
 *
 * Purpose:	Allocate & initialize a managed indirect block
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar  6 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_man_iblock_create(H5HF_hdr_t *hdr, hid_t dxpl_id, H5HF_indirect_t *par_iblock,
    unsigned par_entry, unsigned nrows, unsigned max_rows, haddr_t *addr_p)
{
    H5HF_indirect_t *iblock = NULL;     /* Pointer to indirect block */
    size_t u;                           /* Local index variable */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_man_iblock_create)

    /*
     * Check arguments.
     */
    HDassert(hdr);
    HDassert(nrows > 0);
    HDassert(addr_p);

    /*
     * Allocate file and memory data structures.
     */
    if(NULL == (iblock = H5FL_MALLOC(H5HF_indirect_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for fractal heap indirect block")

    /* Reset the metadata cache info for the heap header */
    HDmemset(&iblock->cache_info, 0, sizeof(H5AC_info_t));

    /* Share common heap information */
    iblock->hdr = hdr;
    if(H5HF_hdr_incr(hdr) < 0)
	HGOTO_ERROR(H5E_HEAP, H5E_CANTINC, FAIL, "can't increment reference count on shared heap header")

    /* Set info for direct block */
    iblock->rc = 0;
    iblock->nrows = nrows;
    iblock->max_rows = max_rows;

    /* Compute size of buffer needed for indirect block */
    iblock->size = H5HF_MAN_INDIRECT_SIZE(hdr, iblock);

    /* Allocate child block entry array */
    if(NULL == (iblock->ents = H5FL_SEQ_MALLOC(H5HF_indirect_ent_t, (size_t)(iblock->nrows * hdr->man_dtable.cparam.width))))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for block entries")

    /* Initialize indirect block entry tables */
    for(u = 0; u < (iblock->nrows * hdr->man_dtable.cparam.width); u++)
        iblock->ents[u].addr = HADDR_UNDEF;

    /* Check for I/O filters to apply to this heap */
    if(hdr->filter_len > 0) {
        unsigned dir_rows;      /* Number of direct rows in this indirect block */

        /* Compute the number of direct rows for this indirect block */
        dir_rows = MIN(iblock->nrows, hdr->man_dtable.max_direct_rows);

        /* Allocate & initialize indirect block filtered entry array */
        if(NULL == (iblock->filt_ents = H5FL_SEQ_CALLOC(H5HF_indirect_filt_ent_t, (size_t)(dir_rows * hdr->man_dtable.cparam.width))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for block entries")
    } /* end if */
    else
        iblock->filt_ents = NULL;

    /* Check if we have any indirect block children */
    if(iblock->nrows > hdr->man_dtable.max_direct_rows) {
        unsigned indir_rows;      /* Number of indirect rows in this indirect block */

        /* Compute the number of indirect rows for this indirect block */
        indir_rows = iblock->nrows - hdr->man_dtable.max_direct_rows;

        /* Allocate & initialize child indirect block pointer array */
        if(NULL == (iblock->child_iblocks = H5FL_SEQ_CALLOC(H5HF_indirect_ptr_t, (size_t)(indir_rows * hdr->man_dtable.cparam.width))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for block entries")
    } /* end if */
    else
        iblock->child_iblocks = NULL;

    /* Allocate [temporary] space for the indirect block on disk */
    if(H5F_USE_TMP_SPACE(hdr->f)) {
        if(HADDR_UNDEF == (*addr_p = H5MF_alloc_tmp(hdr->f, (hsize_t)iblock->size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "file allocation failed for fractal heap indirect block")
    } /* end if */
    else {
        if(HADDR_UNDEF == (*addr_p = H5MF_alloc(hdr->f, H5FD_MEM_FHEAP_IBLOCK, dxpl_id, (hsize_t)iblock->size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "file allocation failed for fractal heap indirect block")
    } /* end else */
    iblock->addr = *addr_p;

    /* Attach to parent indirect block, if there is one */
    iblock->parent = par_iblock;
    iblock->par_entry = par_entry;
    if(iblock->parent) {
        /* Attach new block to parent */
        if(H5HF_man_iblock_attach(iblock->parent, par_entry, *addr_p) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTATTACH, FAIL, "can't attach indirect block to parent indirect block")

        /* Compute the indirect block's offset in the heap's address space */
        /* (based on parent's block offset) */
        iblock->block_off = par_iblock->block_off;
        iblock->block_off += hdr->man_dtable.row_block_off[par_entry / hdr->man_dtable.cparam.width];
        iblock->block_off += hdr->man_dtable.row_block_size[par_entry / hdr->man_dtable.cparam.width] * (par_entry % hdr->man_dtable.cparam.width);
    } /* end if */
    else
        iblock->block_off = 0;  /* Must be the root indirect block... */

    /* Update indirect block's statistics */
    iblock->nchildren = 0;
    iblock->max_child = 0;

    /* Cache the new indirect block */
    if(H5AC_set(hdr->f, dxpl_id, H5AC_FHEAP_IBLOCK, *addr_p, iblock, H5AC__NO_FLAGS_SET) < 0)
	HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, FAIL, "can't add fractal heap indirect block to cache")

done:
    if(ret_value < 0)
        if(iblock)
            if(H5HF_man_iblock_dest(iblock) < 0)
                HDONE_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "unable to destroy fractal heap indirect block")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_man_iblock_create() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_man_iblock_protect
 *
 * Purpose:	Convenience wrapper around H5AC_protect on an indirect block
 *
 * Return:	Pointer to indirect block on success, NULL on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Apr 17 2006
 *
 *-------------------------------------------------------------------------
 */
H5HF_indirect_t *
H5HF_man_iblock_protect(H5HF_hdr_t *hdr, hid_t dxpl_id, haddr_t iblock_addr,
    unsigned iblock_nrows, H5HF_indirect_t *par_iblock, unsigned par_entry,
    hbool_t must_protect, H5AC_protect_t rw, hbool_t *did_protect)
{
    H5HF_parent_t par_info;             /* Parent info for loading block */
    H5HF_indirect_t *iblock = NULL;     /* Indirect block from cache */
    hbool_t should_protect = FALSE;     /* Whether we should protect the indirect block or not */
    H5HF_indirect_t *ret_value;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_man_iblock_protect)

    /*
     * Check arguments.
     */
    HDassert(hdr);
    HDassert(H5F_addr_defined(iblock_addr));
    HDassert(iblock_nrows > 0);
    HDassert(did_protect);

    /* Check if we are allowed to use existing pinned iblock pointer */
    if(!must_protect) {
        /* Check for this block already being pinned */
        if(par_iblock) {
            unsigned indir_idx;             /* Index in parent's child iblock pointer array */

            /* Sanity check */
            HDassert(par_iblock->child_iblocks);
            HDassert(par_entry >= (hdr->man_dtable.max_direct_rows
                    * hdr->man_dtable.cparam.width));

            /* Compute index in parent's child iblock pointer array */
            indir_idx = par_entry - (hdr->man_dtable.max_direct_rows
                    * hdr->man_dtable.cparam.width);

            /* Check for pointer to pinned indirect block in parent */
            if(par_iblock->child_iblocks[indir_idx])
                iblock = par_iblock->child_iblocks[indir_idx];
            else
                should_protect = TRUE;
        } /* end if */
        else {
            /* Check for root indirect block */
            if(H5F_addr_eq(iblock_addr, hdr->man_dtable.table_addr)) {
                /* Check for pointer to pinned indirect block in root */
                if(hdr->root_iblock)
                    iblock = hdr->root_iblock;
                else
                    should_protect = TRUE;
            } /* end if */
            else
                should_protect = TRUE;
        } /* end else */
    } /* end if */

    /* Check for protecting indirect block */
    if(must_protect || should_protect) {
        H5HF_iblock_cache_ud_t cache_udata; /* User-data for callback */

        /* Set up parent info */
        par_info.hdr = hdr;
        par_info.iblock = par_iblock;
        par_info.entry = par_entry;

        /* Set up user data for protect call */
        cache_udata.f = hdr->f;
        cache_udata.par_info = &par_info;
        cache_udata.nrows = &iblock_nrows;

        /* Protect the indirect block */
        if(NULL == (iblock = (H5HF_indirect_t *)H5AC_protect(hdr->f, dxpl_id, H5AC_FHEAP_IBLOCK, iblock_addr, &cache_udata, rw)))
            HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, NULL, "unable to protect fractal heap indirect block")

        /* Set the indirect block's address */
        iblock->addr = iblock_addr;

        /* Indicate that the indirect block was protected */
        *did_protect = TRUE;
    } /* end if */
    else
        /* Indicate that the indirect block was _not_ protected */
        *did_protect = FALSE;

    /* Set the return value */
    ret_value = iblock;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_man_iblock_protect() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_man_iblock_unprotect
 *
 * Purpose:	Convenience wrapper around H5AC_unprotect on an indirect block
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Aug 17 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_man_iblock_unprotect(H5HF_indirect_t *iblock, hid_t dxpl_id,
    unsigned cache_flags, hbool_t did_protect)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_man_iblock_unprotect)

    /*
     * Check arguments.
     */
    HDassert(iblock);

    /* Check if we previously protected this indirect block */
    /* (as opposed to using an existing pointer to a pinned child indirect block) */
    if(did_protect) {
        /* Unprotect the indirect block */
        if(H5AC_unprotect(iblock->hdr->f, dxpl_id, H5AC_FHEAP_IBLOCK, iblock->addr, iblock, cache_flags) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTUNPROTECT, FAIL, "unable to release fractal heap indirect block")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_man_iblock_unprotect() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_man_iblock_attach
 *
 * Purpose:	Attach a block to an indirect block
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May 30 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_man_iblock_attach(H5HF_indirect_t *iblock, unsigned entry, haddr_t child_addr)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_man_iblock_attach)

    /*
     * Check arguments.
     */
    HDassert(iblock);
    HDassert(H5F_addr_defined(child_addr));
    HDassert(!H5F_addr_defined(iblock->ents[entry].addr));

    /* Increment the reference count on this indirect block */
    if(H5HF_iblock_incr(iblock) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTINC, FAIL, "can't increment reference count on shared indirect block")

    /* Point at the direct block */
    iblock->ents[entry].addr = child_addr;

    /* Check for I/O filters on this heap */
    if(iblock->hdr->filter_len > 0) {
        unsigned row;           /* Row for entry */

        /* Sanity check */
        HDassert(iblock->filt_ents);

        /* Compute row for entry */
        row = entry / iblock->hdr->man_dtable.cparam.width;

        /* If this is a direct block, set its initial size */
        if(row < iblock->hdr->man_dtable.max_direct_rows)
            iblock->filt_ents[entry].size = iblock->hdr->man_dtable.row_block_size[row];
    } /* end if */

    /* Check for max. entry used */
    if(entry > iblock->max_child)
        iblock->max_child = entry;

    /* Increment the # of child blocks */
    iblock->nchildren++;

    /* Mark indirect block as modified */
    if(H5HF_iblock_dirty(iblock) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTDIRTY, FAIL, "can't mark indirect block as dirty")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_man_iblock_attach() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_man_iblock_detach
 *
 * Purpose:	Detach a block from an indirect block
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May 31 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_man_iblock_detach(H5HF_indirect_t *iblock, hid_t dxpl_id, unsigned entry)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_man_iblock_detach)

    /*
     * Check arguments.
     */
    HDassert(iblock);
    HDassert(iblock->nchildren);

    /* Reset address of entry */
    iblock->ents[entry].addr = HADDR_UNDEF;

    /* Check for I/O filters on this heap */
    if(iblock->hdr->filter_len > 0) {
        unsigned row;           /* Row for entry */

        /* Sanity check */
        HDassert(iblock->filt_ents);

        /* Compute row for entry */
        row = entry / iblock->hdr->man_dtable.cparam.width;

        /* If this is a direct block, reset its initial size */
        if(row < iblock->hdr->man_dtable.max_direct_rows) {
            iblock->filt_ents[entry].size = 0;
            iblock->filt_ents[entry].filter_mask = 0;
        } /* end if */
    } /* end if */

    /* Decrement the # of child blocks */
    /* (If the number of children drop to 0, the indirect block will be
     *  removed from the heap when its ref. count drops to zero and the
     *  metadata cache calls the indirect block destructor)
     */
    iblock->nchildren--;

    /* Reduce the max. entry used, if necessary */
    if(entry == iblock->max_child) {
        if(iblock->nchildren > 0)
            while(!H5F_addr_defined(iblock->ents[iblock->max_child].addr))
                iblock->max_child--;
        else
            iblock->max_child = 0;
    } /* end if */

    /* If this is the root indirect block handle some special cases */
    if(iblock->block_off == 0) {
        /* If the number of children drops to 1, and that child is the first
         *      direct block in the heap, convert the heap back to using a root
         *      direct block
         */
        if(iblock->nchildren == 1 && H5F_addr_defined(iblock->ents[0].addr))
            if(H5HF_man_iblock_root_revert(iblock, dxpl_id) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTSHRINK, FAIL, "can't convert root indirect block back to root direct block")

        /* Check for reducing size of root indirect block */
        if(iblock->nchildren > 0 && iblock->hdr->man_dtable.cparam.start_root_rows != 0
                && entry > iblock->max_child) {
            unsigned max_child_row;         /* Row for max. child entry */

            /* Compute information needed for determining whether to reduce size of root indirect block */
            max_child_row = iblock->max_child / iblock->hdr->man_dtable.cparam.width;

            /* Check if the root indirect block should be reduced */
            if(iblock->nrows > 1 && max_child_row <= (iblock->nrows / 2))
                if(H5HF_man_iblock_root_halve(iblock, dxpl_id) < 0)
                    HGOTO_ERROR(H5E_HEAP, H5E_CANTSHRINK, FAIL, "can't reduce size of root indirect block")
        } /* end if */
    } /* end if */

    /* Mark indirect block as modified */
    if(H5HF_iblock_dirty(iblock) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTDIRTY, FAIL, "can't mark indirect block as dirty")

    /* Decrement the reference count on this indirect block */
    /* (should be last, so that potential 'unpin' on this indirect block
     *  doesn't invalidate the 'iblock' variable)
     */
    if(H5HF_iblock_decr(iblock) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTDEC, FAIL, "can't decrement reference count on shared indirect block")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_man_iblock_detach() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_man_iblock_entry_addr
 *
 * Purpose:	Retrieve the address of an indirect block's child
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		July 10 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_man_iblock_entry_addr(H5HF_indirect_t *iblock, unsigned entry, haddr_t *child_addr)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5HF_man_iblock_entry_addr)

    /*
     * Check arguments.
     */
    HDassert(iblock);
    HDassert(child_addr);

    /* Retrieve address of entry */
    *child_addr = iblock->ents[entry].addr;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5HF_man_iblock_entry_addr() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_man_iblock_delete
 *
 * Purpose:	Delete a managed indirect block
 *
 * Note:	This routine does _not_ modify any indirect block that points
 *              to this indirect block, it is assumed that the whole heap is
 *              being deleted in a top-down fashion.
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Aug  7 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_man_iblock_delete(H5HF_hdr_t *hdr, hid_t dxpl_id, haddr_t iblock_addr,
    unsigned iblock_nrows, H5HF_indirect_t *par_iblock, unsigned par_entry)
{
    H5HF_indirect_t *iblock;            /* Pointer to indirect block */
    unsigned row, col;                  /* Current row & column in indirect block */
    unsigned entry;                     /* Current entry in row */
    unsigned cache_flags = H5AC__NO_FLAGS_SET;      /* Flags for unprotecting indirect block */
    hbool_t did_protect;                /* Whether we protected the indirect block or not */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_man_iblock_delete)

    /*
     * Check arguments.
     */
    HDassert(hdr);
    HDassert(H5F_addr_defined(iblock_addr));
    HDassert(iblock_nrows > 0);

    /* Lock indirect block */
    if(NULL == (iblock = H5HF_man_iblock_protect(hdr, dxpl_id, iblock_addr, iblock_nrows, par_iblock, par_entry, TRUE, H5AC_WRITE, &did_protect)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTPROTECT, FAIL, "unable to protect fractal heap indirect block")
    HDassert(iblock->nchildren > 0);
    HDassert(did_protect == TRUE);

    /* Iterate over rows in this indirect block */
    entry = 0;
    for(row = 0; row < iblock->nrows; row++) {
        /* Iterate over entries in this row */
        for(col = 0; col < hdr->man_dtable.cparam.width; col++, entry++) {
            /* Check for child entry at this position */
            if(H5F_addr_defined(iblock->ents[entry].addr)) {
                hsize_t row_block_size;         /* The size of blocks in this row */

                /* Get the row's block size */
                row_block_size = (hsize_t)hdr->man_dtable.row_block_size[row];

                /* Are we in a direct or indirect block row */
                if(row < hdr->man_dtable.max_direct_rows) {
                    hsize_t dblock_size;        /* Size of direct block on disk */

                    /* Check for I/O filters on this heap */
                    if(hdr->filter_len > 0)
                        dblock_size = iblock->filt_ents[entry].size;
                    else
                        dblock_size = row_block_size;

                    /* Delete child direct block */
                    if(H5HF_man_dblock_delete(hdr->f, dxpl_id, iblock->ents[entry].addr, dblock_size) < 0)
                        HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "unable to release fractal heap child direct block")
                } /* end if */
                else {
                    unsigned child_nrows;   /* Number of rows in new indirect block */

                    /* Compute # of rows in next child indirect block to use */
                    child_nrows = H5HF_dtable_size_to_rows(&hdr->man_dtable, row_block_size);

                    /* Delete child indirect block */
                    if(H5HF_man_iblock_delete(hdr, dxpl_id, iblock->ents[entry].addr, child_nrows, iblock, entry) < 0)
                        HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "unable to release fractal heap child indirect block")
                } /* end else */
            } /* end if */
        } /* end for */
    } /* end row */

#ifndef NDEBUG
{
    unsigned iblock_status = 0;         /* Indirect block's status in the metadata cache */

    /* Check the indirect block's status in the metadata cache */
    if(H5AC_get_entry_status(hdr->f, iblock_addr, &iblock_status) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTGET, FAIL, "unable to check metadata cache status for indirect block")

    /* Check if indirect block is pinned */
    HDassert(!(iblock_status & H5AC_ES__IS_PINNED));
}
#endif /* NDEBUG */

    /* Indicate that the indirect block should be deleted & file space freed */
    cache_flags |= H5AC__DIRTIED_FLAG | H5AC__DELETED_FLAG | H5AC__FREE_FILE_SPACE_FLAG;

done:
    /* Unprotect the indirect block, with appropriate flags */
    if(iblock && H5HF_man_iblock_unprotect(iblock, dxpl_id, cache_flags, did_protect) < 0)
        HDONE_ERROR(H5E_HEAP, H5E_CANTUNPROTECT, FAIL, "unable to release fractal heap indirect block")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_man_iblock_delete() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5HF_man_iblock_size
 *
 * Purpose:     Gather storage used for the indirect block in fractal heap
 *
 * Return:      non-negative on success, negative on error
 *
 * Programmer:  Vailin Choi
 *              July 12 2007
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_man_iblock_size(H5F_t *f, hid_t dxpl_id, H5HF_hdr_t *hdr, haddr_t iblock_addr,
    unsigned nrows, H5HF_indirect_t *par_iblock, unsigned par_entry, hsize_t *heap_size)
{
    H5HF_indirect_t     *iblock = NULL;         /* Pointer to indirect block */
    hbool_t             did_protect;            /* Whether we protected the indirect block or not */
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5HF_man_iblock_size, FAIL)

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(hdr);
    HDassert(H5F_addr_defined(iblock_addr));
    HDassert(heap_size);

    /* Protect the indirect block */
    if(NULL == (iblock = H5HF_man_iblock_protect(hdr, dxpl_id, iblock_addr, nrows, par_iblock, par_entry, FALSE, H5AC_READ, &did_protect)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTLOAD, FAIL, "unable to load fractal heap indirect block")

    /* Accumulate size of this indirect block */
    *heap_size += iblock->size;

    /* Indirect entries in this indirect block */
    if(iblock->nrows > hdr->man_dtable.max_direct_rows) {
	unsigned    first_row_bits;     /* Number of bits used bit addresses in first row */
        unsigned    num_indirect_rows;  /* Number of rows of blocks in each indirect block */
        unsigned    entry;              /* Current entry in row */
        size_t      u;                  /* Local index variable */

        entry = hdr->man_dtable.max_direct_rows * hdr->man_dtable.cparam.width;
	first_row_bits = H5V_log2_of2((uint32_t)hdr->man_dtable.cparam.start_block_size) +
			    H5V_log2_of2(hdr->man_dtable.cparam.width);
	num_indirect_rows =
            (H5V_log2_gen(hdr->man_dtable.row_block_size[hdr->man_dtable.max_direct_rows]) - first_row_bits) + 1;
        for(u = hdr->man_dtable.max_direct_rows; u < iblock->nrows; u++, num_indirect_rows++) {
            size_t      v;                      /* Local index variable */

	    for(v = 0; v < hdr->man_dtable.cparam.width; v++, entry++)
		if(H5F_addr_defined(iblock->ents[entry].addr))
		    if(H5HF_man_iblock_size(f, dxpl_id, hdr, iblock->ents[entry].addr, num_indirect_rows, iblock, entry, heap_size) < 0)
			HGOTO_ERROR(H5E_HEAP, H5E_CANTLOAD, FAIL, "unable to get fractal heap storage info for indirect block")
        } /* end for */
    } /* end if */

done:
    /* Release the indirect block */
    if(iblock && H5HF_man_iblock_unprotect(iblock, dxpl_id, H5AC__NO_FLAGS_SET, did_protect) < 0)
	HGOTO_ERROR(H5E_HEAP, H5E_CANTUNPROTECT, FAIL, "unable to release fractal heap indirect block")
    iblock = NULL;

   FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_man_iblock_size() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_man_iblock_dest
 *
 * Purpose:	Destroys a fractal heap indirect block in memory.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar  6 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_man_iblock_dest(H5HF_indirect_t *iblock)
{
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_man_iblock_dest)

    /*
     * Check arguments.
     */
    HDassert(iblock);
    HDassert(iblock->rc == 0);

    /* Decrement reference count on shared info */
    HDassert(iblock->hdr);
    if(H5HF_hdr_decr(iblock->hdr) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTDEC, FAIL, "can't decrement reference count on shared heap header")
    if(iblock->parent)
        if(H5HF_iblock_decr(iblock->parent) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTDEC, FAIL, "can't decrement reference count on shared indirect block")

    /* Release entry tables */
    if(iblock->ents)
        iblock->ents = H5FL_SEQ_FREE(H5HF_indirect_ent_t, iblock->ents);
    if(iblock->filt_ents)
        iblock->filt_ents = H5FL_SEQ_FREE(H5HF_indirect_filt_ent_t, iblock->filt_ents);
    if(iblock->child_iblocks)
        iblock->child_iblocks = H5FL_SEQ_FREE(H5HF_indirect_ptr_t, iblock->child_iblocks);

    /* Free fractal heap indirect block info */
    iblock = H5FL_FREE(H5HF_indirect_t, iblock);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_man_iblock_dest() */

