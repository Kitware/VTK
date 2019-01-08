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
 * Created:		H5EAdblock.c
 *			Sep 11 2008
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		Data block routines for extensible arrays.
 *
 *-------------------------------------------------------------------------
 */

/**********************/
/* Module Declaration */
/**********************/

#include "H5EAmodule.h"         /* This source code file is part of the H5EA module */


/***********************/
/* Other Packages Used */
/***********************/


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5EApkg.h"		/* Extensible Arrays			*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5MFprivate.h"	/* File memory management		*/
#include "H5VMprivate.h"		/* Vectors and arrays 			*/


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

/* Declare a free list to manage the H5EA_dblock_t struct */
H5FL_DEFINE_STATIC(H5EA_dblock_t);



/*-------------------------------------------------------------------------
 * Function:	H5EA__dblock_alloc
 *
 * Purpose:	Allocate extensible array data block
 *
 * Return:	Non-NULL pointer to data block on success/NULL on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Sep 11 2008
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
H5EA_dblock_t *, NULL, NULL,
H5EA__dblock_alloc(H5EA_hdr_t *hdr, void *parent, size_t nelmts))

    /* Local variables */
    H5EA_dblock_t *dblock = NULL;          /* Extensible array data block */

    /* Check arguments */
    HDassert(hdr);
    HDassert(parent);
    HDassert(nelmts > 0);

    /* Allocate memory for the data block */
    if(NULL == (dblock = H5FL_CALLOC(H5EA_dblock_t)))
	H5E_THROW(H5E_CANTALLOC, "memory allocation failed for extensible array data block")

    /* Share common array information */
    if(H5EA__hdr_incr(hdr) < 0)
	H5E_THROW(H5E_CANTINC, "can't increment reference count on shared array header")
    dblock->hdr = hdr;

    /* Set non-zero internal fields */
    dblock->parent = parent;
    dblock->nelmts = nelmts;

    /* Check if the data block is not going to be paged */
    if(nelmts > hdr->dblk_page_nelmts) {
        /* Set the # of pages in the direct block */
        dblock->npages = nelmts / hdr->dblk_page_nelmts;
        HDassert(nelmts == (dblock->npages * hdr->dblk_page_nelmts));
    } /* end if */
    else {
        /* Allocate buffer for elements in data block */
        if(NULL == (dblock->elmts = H5EA__hdr_alloc_elmts(hdr, nelmts)))
            H5E_THROW(H5E_CANTALLOC, "memory allocation failed for data block element buffer")
    } /* end else */

    /* Set the return value */
    ret_value = dblock;

CATCH
    if(!ret_value)
        if(dblock && H5EA__dblock_dest(dblock) < 0)
            H5E_THROW(H5E_CANTFREE, "unable to destroy extensible array data block")

END_FUNC(PKG)   /* end H5EA__dblock_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5EA__dblock_create
 *
 * Purpose:	Creates a new extensible array data block in the file
 *
 * Return:	Valid file address on success/HADDR_UNDEF on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Sep  9 2008
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
haddr_t, HADDR_UNDEF, HADDR_UNDEF,
H5EA__dblock_create(H5EA_hdr_t *hdr, void *parent, hbool_t *stats_changed,
    hsize_t dblk_off, size_t nelmts))

    /* Local variables */
    H5EA_dblock_t *dblock = NULL;       /* Extensible array data block */
    haddr_t dblock_addr;                /* Extensible array data block address */
    hbool_t inserted = FALSE;           /* Whether the header was inserted into cache */

    /* Sanity check */
    HDassert(hdr);
    HDassert(stats_changed);
    HDassert(nelmts > 0);

    /* Allocate the data block */
    if(NULL == (dblock = H5EA__dblock_alloc(hdr, parent, nelmts)))
	H5E_THROW(H5E_CANTALLOC, "memory allocation failed for extensible array data block")

    /* Set size of data block on disk */
    dblock->size = H5EA_DBLOCK_SIZE(dblock);

    /* Set offset of block in array's address space */
    dblock->block_off = dblk_off;

    /* Allocate space for the data block on disk */
    if(HADDR_UNDEF == (dblock_addr = H5MF_alloc(hdr->f, H5FD_MEM_EARRAY_DBLOCK, (hsize_t)dblock->size)))
	H5E_THROW(H5E_CANTALLOC, "file allocation failed for extensible array data block")
    dblock->addr = dblock_addr;

    /* Don't initialize elements if paged */
    if(!dblock->npages)
        /* Clear any elements in data block to fill value */
        if((hdr->cparam.cls->fill)(dblock->elmts, (size_t)dblock->nelmts) < 0)
            H5E_THROW(H5E_CANTSET, "can't set extensible array data block elements to class's fill value")

    /* Cache the new extensible array data block */
    if(H5AC_insert_entry(hdr->f, H5AC_EARRAY_DBLOCK, dblock_addr, dblock, H5AC__NO_FLAGS_SET) < 0)
	H5E_THROW(H5E_CANTINSERT, "can't add extensible array data block to cache")
    inserted = TRUE;

    /* Add data block as child of 'top' proxy */
    if(hdr->top_proxy) {
        if(H5AC_proxy_entry_add_child(hdr->top_proxy, hdr->f, dblock) < 0)
            H5E_THROW(H5E_CANTSET, "unable to add extensible array entry as child of array proxy")
        dblock->top_proxy = hdr->top_proxy;
    } /* end if */

    /* Update extensible array data block statistics */
    hdr->stats.stored.ndata_blks++;
    hdr->stats.stored.data_blk_size += dblock->size;

    /* Increment count of elements "realized" */
    hdr->stats.stored.nelmts += nelmts;

    /* Mark the statistics as changed */
    *stats_changed = TRUE;

    /* Set address of data block to return */
    ret_value = dblock_addr;

CATCH
    if(!H5F_addr_defined(ret_value))
        if(dblock) {
            /* Remove from cache, if inserted */
            if(inserted)
                if(H5AC_remove_entry(dblock) < 0)
                    H5E_THROW(H5E_CANTREMOVE, "unable to remove extensible array data block from cache")

            /* Release data block's disk space */
            if(H5F_addr_defined(dblock->addr) && H5MF_xfree(hdr->f, H5FD_MEM_EARRAY_DBLOCK, dblock->addr, (hsize_t)dblock->size) < 0)
                H5E_THROW(H5E_CANTFREE, "unable to release extensible array data block")

            /* Destroy data block */
            if(H5EA__dblock_dest(dblock) < 0)
                H5E_THROW(H5E_CANTFREE, "unable to destroy extensible array data block")
        } /* end if */

END_FUNC(PKG)   /* end H5EA__dblock_create() */


/*-------------------------------------------------------------------------
 * Function:	H5EA__dblock_sblk_idx
 *
 * Purpose:	Compute the index of the super block where the element is
 *              located.
 *
 * Return:	Super block index on success/Can't fail
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Sep 11 2008
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, NOERR,
unsigned, 0, -,
H5EA__dblock_sblk_idx(const H5EA_hdr_t *hdr, hsize_t idx))

    /* Local variables */
    unsigned sblk_idx;      /* Which superblock does this index fall in? */

    /* Sanity check */
    HDassert(hdr);
    HDassert(idx >= hdr->cparam.idx_blk_elmts);

    /* Adjust index for elements in index block */
    idx -= hdr->cparam.idx_blk_elmts;

    /* Determine the superblock information for the index */
    H5_CHECK_OVERFLOW(idx, /*From:*/hsize_t, /*To:*/uint64_t);
    sblk_idx = H5VM_log2_gen((uint64_t)((idx / hdr->cparam.data_blk_min_elmts) + 1));

    /* Set return value */
    ret_value = sblk_idx;

END_FUNC(PKG)   /* end H5EA__dblock_sblk_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5EA__dblock_protect
 *
 * Purpose:	Convenience wrapper around protecting extensible array data block
 *
 * Return:	Non-NULL pointer to data block on success/NULL on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Sep 18 2008
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
H5EA_dblock_t *, NULL, NULL,
H5EA__dblock_protect(H5EA_hdr_t *hdr, void *parent, haddr_t dblk_addr,
    size_t dblk_nelmts, unsigned flags))

    /* Local variables */
    H5EA_dblock_t *dblock;              /* Extensible array data block */
    H5EA_dblock_cache_ud_t udata;       /* Information needed for loading data block */

    /* Sanity check */
    HDassert(hdr);
    HDassert(H5F_addr_defined(dblk_addr));
    HDassert(dblk_nelmts);

    /* only the H5AC__READ_ONLY_FLAG may be set */
    HDassert((flags & (unsigned)(~H5AC__READ_ONLY_FLAG)) == 0);

    /* Set up user data */
    udata.hdr = hdr;
    udata.parent = parent;
    udata.nelmts = dblk_nelmts;
    udata.dblk_addr = dblk_addr;

    /* Protect the data block */
    if(NULL == (dblock = (H5EA_dblock_t *)H5AC_protect(hdr->f, H5AC_EARRAY_DBLOCK, dblk_addr, &udata, flags)))
        H5E_THROW(H5E_CANTPROTECT, "unable to protect extensible array data block, address = %llu", (unsigned long long)dblk_addr)

    /* Create top proxy, if it doesn't exist */
    if(hdr->top_proxy && NULL == dblock->top_proxy) {
        /* Add data block as child of 'top' proxy */
        if(H5AC_proxy_entry_add_child(hdr->top_proxy, hdr->f, dblock) < 0)
            H5E_THROW(H5E_CANTSET, "unable to add extensible array entry as child of array proxy")
        dblock->top_proxy = hdr->top_proxy;
    } /* end if */

    /* Set return value */
    ret_value = dblock;

CATCH

    /* Clean up on error */
    if(!ret_value) {
        /* Release the data block, if it was protected */
        if(dblock && H5AC_unprotect(hdr->f, H5AC_EARRAY_DBLOCK, dblock->addr, dblock, H5AC__NO_FLAGS_SET) < 0)
            H5E_THROW(H5E_CANTUNPROTECT, "unable to unprotect extensible array data block, address = %llu", (unsigned long long)dblock->addr)
    } /* end if */

END_FUNC(PKG)   /* end H5EA__dblock_protect() */


/*-------------------------------------------------------------------------
 * Function:	H5EA__dblock_unprotect
 *
 * Purpose:	Convenience wrapper around unprotecting extensible array data block
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Sep 11 2008
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
herr_t, SUCCEED, FAIL,
H5EA__dblock_unprotect(H5EA_dblock_t *dblock, unsigned cache_flags))

    /* Local variables */

    /* Sanity check */
    HDassert(dblock);

    /* Unprotect the data block */
    if(H5AC_unprotect(dblock->hdr->f, H5AC_EARRAY_DBLOCK, dblock->addr, dblock, cache_flags) < 0)
        H5E_THROW(H5E_CANTUNPROTECT, "unable to unprotect extensible array data block, address = %llu", (unsigned long long)dblock->addr)

CATCH

END_FUNC(PKG)   /* end H5EA__dblock_unprotect() */


/*-------------------------------------------------------------------------
 * Function:	H5EA__dblock_delete
 *
 * Purpose:	Delete a data block
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Sep 22 2008
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
herr_t, SUCCEED, FAIL,
H5EA__dblock_delete(H5EA_hdr_t *hdr, void *parent, haddr_t dblk_addr,
    size_t dblk_nelmts))

    /* Local variables */
    H5EA_dblock_t *dblock = NULL;       /* Pointer to data block */

    /* Sanity check */
    HDassert(hdr);
    HDassert(parent);
    HDassert(H5F_addr_defined(dblk_addr));
    HDassert(dblk_nelmts > 0);

    /* Protect data block */
    if(NULL == (dblock = H5EA__dblock_protect(hdr, parent, dblk_addr, dblk_nelmts, H5AC__NO_FLAGS_SET)))
        H5E_THROW(H5E_CANTPROTECT, "unable to protect extensible array data block, address = %llu", (unsigned long long)dblk_addr)

    /* Check if this is a paged data block */
    if(dblk_nelmts > hdr->dblk_page_nelmts) {
        size_t npages = dblk_nelmts / hdr->dblk_page_nelmts;    /* Number of pages in data block */
        haddr_t dblk_page_addr;         /* Address of each data block page */
        size_t dblk_page_size;          /* Size of each data block page */
        size_t u;                       /* Local index variable */

        /* Set up initial state */
        dblk_page_addr = dblk_addr + H5EA_DBLOCK_PREFIX_SIZE(dblock);
        dblk_page_size = (hdr->dblk_page_nelmts * hdr->cparam.raw_elmt_size)
                + H5EA_SIZEOF_CHKSUM;

        /* Iterate over pages in data block */
        for(u = 0; u < npages; u++) {
            /* Evict the data block page from the metadata cache */
            /* (OK to call if it doesn't exist in the cache) */
            if(H5AC_expunge_entry(hdr->f, H5AC_EARRAY_DBLK_PAGE, dblk_page_addr, H5AC__NO_FLAGS_SET) < 0)
                H5E_THROW(H5E_CANTEXPUNGE, "unable to remove array data block page from metadata cache")

            /* Advance to next page address */
            dblk_page_addr += dblk_page_size;
        } /* end for */
    } /* end if */

CATCH
    /* Finished deleting data block in metadata cache */
    if(dblock && H5EA__dblock_unprotect(dblock, H5AC__DIRTIED_FLAG | H5AC__DELETED_FLAG | H5AC__FREE_FILE_SPACE_FLAG) < 0)
        H5E_THROW(H5E_CANTUNPROTECT, "unable to release extensible array data block")

END_FUNC(PKG)   /* end H5EA__dblock_delete() */


/*-------------------------------------------------------------------------
 * Function:	H5EA__dblock_dest
 *
 * Purpose:	Destroys an extensible array data block in memory.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Sep 11 2008
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
herr_t, SUCCEED, FAIL,
H5EA__dblock_dest(H5EA_dblock_t *dblock))

    /* Sanity check */
    HDassert(dblock);
    HDassert(!dblock->has_hdr_depend);

    /* Check if shared header field has been initialized */
    if(dblock->hdr) {
        /* Check if we've got elements in the data block */
        if(dblock->elmts && !dblock->npages) {
            /* Free buffer for data block elements */
            HDassert(dblock->nelmts > 0);
            if(H5EA__hdr_free_elmts(dblock->hdr, dblock->nelmts, dblock->elmts) < 0)
                H5E_THROW(H5E_CANTFREE, "unable to free extensible array data block element buffer")
            dblock->elmts = NULL;
            dblock->nelmts = 0;
        } /* end if */

        /* Decrement reference count on shared info */
        if(H5EA__hdr_decr(dblock->hdr) < 0)
            H5E_THROW(H5E_CANTDEC, "can't decrement reference count on shared array header")
        dblock->hdr = NULL;
    } /* end if */

    /* Sanity check */
    HDassert(NULL == dblock->top_proxy);

    /* Free the data block itself */
    dblock = H5FL_FREE(H5EA_dblock_t, dblock);

CATCH

END_FUNC(PKG)   /* end H5EA__dblock_dest() */

