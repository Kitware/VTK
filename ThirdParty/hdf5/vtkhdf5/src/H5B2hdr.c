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
 * Created:		H5B2int.c
 *			Feb 27 2006
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Internal routines for managing v2 B-trees.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5B2_PACKAGE		/*suppress error about including H5B2pkg  */

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5B2pkg.h"		/* v2 B-trees				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5MFprivate.h"	/* File memory management		*/
#include "H5VMprivate.h"		/* Vectors and arrays 			*/

/****************/
/* Local Macros */
/****************/

/* Number of records that fit into leaf node */
#define H5B2_NUM_LEAF_REC(n, r) \
    (((n) - H5B2_LEAF_PREFIX_SIZE) / (r))

/* Uncomment this macro to enable extra sanity checking */
/* #define H5B2_DEBUG */

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

/* Declare a free list to manage the H5B2_hdr_t struct */
H5FL_DEFINE_STATIC(H5B2_hdr_t);

/* Declare a free list to manage B-tree node pages to/from disk */
H5FL_BLK_DEFINE_STATIC(node_page);

/* Declare a free list to manage the 'size_t' sequence information */
H5FL_SEQ_DEFINE_STATIC(size_t);

/* Declare a free list to manage the 'H5B2_node_info_t' sequence information */
H5FL_SEQ_DEFINE(H5B2_node_info_t);



/*-------------------------------------------------------------------------
 * Function:	H5B2_hdr_init
 *
 * Purpose:	Allocate & initialize B-tree header info
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb  2 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_hdr_init(H5B2_hdr_t *hdr, const H5B2_create_t *cparam, void *ctx_udata,
    uint16_t depth)
{
    size_t sz_max_nrec;                 /* Temporary variable for range checking */
    unsigned u_max_nrec_size;           /* Temporary variable for range checking */
    unsigned u;                         /* Local index variable */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /*
     * Check arguments.
     */
    HDassert(hdr);
    HDassert(cparam);
    HDassert(cparam->cls);
    HDassert((cparam->cls->crt_context && cparam->cls->dst_context) ||
        (NULL == cparam->cls->crt_context && NULL == cparam->cls->dst_context));
    HDassert(cparam->node_size > 0);
    HDassert(cparam->rrec_size > 0);
    HDassert(cparam->merge_percent > 0 && cparam->merge_percent <= 100);
    HDassert(cparam->split_percent > 0 && cparam->split_percent <= 100);
    HDassert(cparam->merge_percent < (cparam->split_percent / 2));

    /* Initialize basic information */
    hdr->rc = 0;
    hdr->pending_delete = FALSE;

    /* Assign dynamic information */
    hdr->depth = depth;

    /* Assign user's information */
    hdr->split_percent = cparam->split_percent;
    hdr->merge_percent = cparam->merge_percent;
    hdr->node_size = cparam->node_size;
    hdr->rrec_size = cparam->rrec_size;

    /* Assign common type information */
    hdr->cls = cparam->cls;

    /* Allocate "page" for node I/O */
    if(NULL == (hdr->page = H5FL_BLK_MALLOC(node_page, hdr->node_size)))
        HGOTO_ERROR(H5E_BTREE, H5E_NOSPACE, FAIL, "memory allocation failed")
#ifdef H5_CLEAR_MEMORY
HDmemset(hdr->page, 0, hdr->node_size);
#endif /* H5_CLEAR_MEMORY */

    /* Allocate array of node info structs */
    if(NULL == (hdr->node_info = H5FL_SEQ_MALLOC(H5B2_node_info_t, (size_t)(hdr->depth + 1))))
        HGOTO_ERROR(H5E_BTREE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Initialize leaf node info */
    sz_max_nrec = H5B2_NUM_LEAF_REC(hdr->node_size, hdr->rrec_size);
    H5_ASSIGN_OVERFLOW(/* To: */ hdr->node_info[0].max_nrec, /* From: */ sz_max_nrec, /* From: */ size_t, /* To: */ unsigned)
    hdr->node_info[0].split_nrec = (hdr->node_info[0].max_nrec * hdr->split_percent) / 100;
    hdr->node_info[0].merge_nrec = (hdr->node_info[0].max_nrec * hdr->merge_percent) / 100;
    hdr->node_info[0].cum_max_nrec = hdr->node_info[0].max_nrec;
    hdr->node_info[0].cum_max_nrec_size = 0;
    if(NULL == (hdr->node_info[0].nat_rec_fac = H5FL_fac_init(hdr->cls->nrec_size * hdr->node_info[0].max_nrec)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTINIT, FAIL, "can't create node native key block factory")
    hdr->node_info[0].node_ptr_fac = NULL;

    /* Allocate array of pointers to internal node native keys */
    /* (uses leaf # of records because its the largest) */
    if(NULL == (hdr->nat_off = H5FL_SEQ_MALLOC(size_t, (size_t)hdr->node_info[0].max_nrec)))
        HGOTO_ERROR(H5E_BTREE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Initialize offsets in native key block */
    /* (uses leaf # of records because its the largest) */
    for(u = 0; u < hdr->node_info[0].max_nrec; u++)
        hdr->nat_off[u] = hdr->cls->nrec_size * u;

    /* Compute size to store # of records in each node */
    /* (uses leaf # of records because its the largest) */
    u_max_nrec_size = H5VM_limit_enc_size((uint64_t)hdr->node_info[0].max_nrec);
    H5_ASSIGN_OVERFLOW(/* To: */ hdr->max_nrec_size, /* From: */ u_max_nrec_size, /* From: */ unsigned, /* To: */ uint8_t)
    HDassert(hdr->max_nrec_size <= H5B2_SIZEOF_RECORDS_PER_NODE);

    /* Initialize internal node info */
    if(depth > 0) {
        for(u = 1; u < (unsigned)(depth + 1); u++) {
            sz_max_nrec = H5B2_NUM_INT_REC(hdr, u);
            H5_ASSIGN_OVERFLOW(/* To: */ hdr->node_info[u].max_nrec, /* From: */ sz_max_nrec, /* From: */ size_t, /* To: */ unsigned)
            HDassert(hdr->node_info[u].max_nrec <= hdr->node_info[u - 1].max_nrec);

            hdr->node_info[u].split_nrec = (hdr->node_info[u].max_nrec * hdr->split_percent) / 100;
            hdr->node_info[u].merge_nrec = (hdr->node_info[u].max_nrec * hdr->merge_percent) / 100;

            hdr->node_info[u].cum_max_nrec = ((hdr->node_info[u].max_nrec + 1) *
                hdr->node_info[u - 1].cum_max_nrec) + hdr->node_info[u].max_nrec;
            u_max_nrec_size = H5VM_limit_enc_size((uint64_t)hdr->node_info[u].cum_max_nrec);
            H5_ASSIGN_OVERFLOW(/* To: */ hdr->node_info[u].cum_max_nrec_size, /* From: */ u_max_nrec_size, /* From: */ unsigned, /* To: */ uint8_t)

            if(NULL == (hdr->node_info[u].nat_rec_fac = H5FL_fac_init(hdr->cls->nrec_size * hdr->node_info[u].max_nrec)))
                HGOTO_ERROR(H5E_BTREE, H5E_CANTINIT, FAIL, "can't create node native key block factory")
            if(NULL == (hdr->node_info[u].node_ptr_fac = H5FL_fac_init(sizeof(H5B2_node_ptr_t) * (hdr->node_info[u].max_nrec + 1))))
                HGOTO_ERROR(H5E_BTREE, H5E_CANTINIT, FAIL, "can't create internal 'branch' node node pointer block factory")
        } /* end for */
    } /* end if */

    /* Create the callback context, if the callback exists */
    if(hdr->cls->crt_context) {
        if(NULL == (hdr->cb_ctx = (*hdr->cls->crt_context)(ctx_udata)))
            HGOTO_ERROR(H5E_BTREE, H5E_CANTCREATE, FAIL, "unable to create v2 B-tree client callback context")
    } /* end if */

done:
    if(ret_value < 0)
        if(H5B2_hdr_free(hdr) < 0)
            HDONE_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to free shared v2 B-tree info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2_hdr_init() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_hdr_alloc
 *
 * Purpose:	Allocate B-tree header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct 27 2009
 *
 *-------------------------------------------------------------------------
 */
H5B2_hdr_t *
H5B2_hdr_alloc(H5F_t *f)
{
    H5B2_hdr_t *hdr = NULL;             /* v2 B-tree header */
    H5B2_hdr_t *ret_value;              /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /*
     * Check arguments.
     */
    HDassert(f);

    /* Allocate space for the shared information */
    if(NULL == (hdr = H5FL_CALLOC(H5B2_hdr_t)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTALLOC, NULL, "memory allocation failed for B-tree header")

    /* Assign non-zero information */
    hdr->f = f;
    hdr->sizeof_addr = H5F_SIZEOF_ADDR(f);
    hdr->sizeof_size = H5F_SIZEOF_SIZE(f);
    hdr->hdr_size = H5B2_HEADER_SIZE(hdr);
    hdr->root.addr = HADDR_UNDEF;

    /* Set return value */
    ret_value = hdr;

done:

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2_hdr_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_hdr_create
 *
 * Purpose:	Create new fractal heap header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar 21 2006
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5B2_hdr_create(H5F_t *f, hid_t dxpl_id, const H5B2_create_t *cparam,
    void *ctx_udata)
{
    H5B2_hdr_t *hdr = NULL;     /* The new v2 B-tree header information */
    haddr_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(cparam);

    /* Allocate v2 B-tree header */
    if(NULL == (hdr = H5B2_hdr_alloc(f)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTALLOC, HADDR_UNDEF, "allocation failed for B-tree header")

    /* Initialize shared B-tree info */
    if(H5B2_hdr_init(hdr, cparam, ctx_udata, (uint16_t)0) < 0)
	HGOTO_ERROR(H5E_BTREE, H5E_CANTINIT, HADDR_UNDEF, "can't create shared B-tree info")

    /* Allocate space for the header on disk */
    if(HADDR_UNDEF == (hdr->addr = H5MF_alloc(f, H5FD_MEM_BTREE, dxpl_id, (hsize_t)hdr->hdr_size)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTALLOC, HADDR_UNDEF, "file allocation failed for B-tree header")

    /* Cache the new B-tree node */
    if(H5AC_insert_entry(f, dxpl_id, H5AC_BT2_HDR, hdr->addr, hdr, H5AC__NO_FLAGS_SET) < 0)
	HGOTO_ERROR(H5E_BTREE, H5E_CANTINSERT, HADDR_UNDEF, "can't add B-tree header to cache")

    /* Set address of v2 B-tree header to return */
    ret_value = hdr->addr;

done:
    if(!H5F_addr_defined(ret_value) && hdr)
        if(H5B2_hdr_free(hdr) < 0)
            HDONE_ERROR(H5E_BTREE, H5E_CANTRELEASE, HADDR_UNDEF, "unable to release v2 B-tree header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2_hdr_create() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_hdr_incr
 *
 * Purpose:	Increment reference count on B-tree header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct 13 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_hdr_incr(H5B2_hdr_t *hdr)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity checks */
    HDassert(hdr);

    /* Mark header as un-evictable when a B-tree node is depending on it */
    if(hdr->rc == 0)
        if(H5AC_pin_protected_entry(hdr) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTPIN, FAIL, "unable to pin v2 B-tree header")

    /* Increment reference count on B-tree header */
    hdr->rc++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2_incr_hdr() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_hdr_decr
 *
 * Purpose:	Decrement reference count on B-tree header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct 13 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_hdr_decr(H5B2_hdr_t *hdr)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(hdr);
    HDassert(hdr->rc > 0);

    /* Decrement reference count on B-tree header */
    hdr->rc--;

    /* Mark header as evictable again when no nodes depend on it */
    if(hdr->rc == 0)
        if(H5AC_unpin_entry(hdr) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPIN, FAIL, "unable to unpin v2 B-tree header")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2_hdr_decr() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_hdr_fuse_incr
 *
 * Purpose:	Increment file reference count on shared v2 B-tree header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct 27 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_hdr_fuse_incr(H5B2_hdr_t *hdr)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(hdr);

    /* Increment file reference count on shared header */
    hdr->file_rc++;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5B2_hdr_fuse_incr() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_hdr_fuse_decr
 *
 * Purpose:	Decrement file reference count on shared v2 B-tree header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct 27 2009
 *
 *-------------------------------------------------------------------------
 */
size_t
H5B2_hdr_fuse_decr(H5B2_hdr_t *hdr)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(hdr);
    HDassert(hdr->file_rc);

    /* Decrement file reference count on shared header */
    hdr->file_rc--;

    FUNC_LEAVE_NOAPI(hdr->file_rc)
} /* end H5B2_hdr_fuse_decr() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_hdr_dirty
 *
 * Purpose:	Mark B-tree header as dirty
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct 13 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_hdr_dirty(H5B2_hdr_t *hdr)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(hdr);

    /* Mark B-tree header as dirty in cache */
    if(H5AC_mark_entry_dirty(hdr) < 0)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTMARKDIRTY, FAIL, "unable to mark v2 B-tree header as dirty")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2_hdr_dirty() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_hdr_free
 *
 * Purpose:	Free B-tree header info
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb  2 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_hdr_free(H5B2_hdr_t *hdr)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(hdr);

    /* Destroy the callback context */
    if(hdr->cb_ctx) {
        if((*hdr->cls->dst_context)(hdr->cb_ctx) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTRELEASE, FAIL, "can't destroy v2 B-tree client callback context")
        hdr->cb_ctx = NULL;
    } /* end if */

    /* Free the B-tree node buffer */
    if(hdr->page)
        hdr->page = H5FL_BLK_FREE(node_page, hdr->page);

    /* Free the array of offsets into the native key block */
    if(hdr->nat_off)
        hdr->nat_off = H5FL_SEQ_FREE(size_t, hdr->nat_off);

    /* Release the node info */
    if(hdr->node_info) {
        unsigned u;             /* Local index variable */

        /* Destroy free list factories */
        for(u = 0; u < (unsigned)(hdr->depth + 1); u++) {
            if(hdr->node_info[u].nat_rec_fac)
                if(H5FL_fac_term(hdr->node_info[u].nat_rec_fac) < 0)
                    HGOTO_ERROR(H5E_BTREE, H5E_CANTRELEASE, FAIL, "can't destroy node's native record block factory")
            if(hdr->node_info[u].node_ptr_fac)
                if(H5FL_fac_term(hdr->node_info[u].node_ptr_fac) < 0)
                    HGOTO_ERROR(H5E_BTREE, H5E_CANTRELEASE, FAIL, "can't destroy node's node pointer block factory")
        } /* end for */

        /* Free the array of node info structs */
        hdr->node_info = H5FL_SEQ_FREE(H5B2_node_info_t, hdr->node_info);
    } /* end if */

    /* Free B-tree header info */
    hdr = H5FL_FREE(H5B2_hdr_t, hdr);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2_hdr_free() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_hdr_delete
 *
 * Purpose:	Delete a v2 B-tree, starting with the header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct 15 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_hdr_delete(H5B2_hdr_t *hdr, hid_t dxpl_id)
{
    unsigned cache_flags = H5AC__NO_FLAGS_SET;  /* Flags for unprotecting v2 B-tree header */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(hdr);

#ifndef NDEBUG
{
    unsigned hdr_status = 0;         /* v2 B-tree header's status in the metadata cache */

    /* Check the v2 B-tree header's status in the metadata cache */
    if(H5AC_get_entry_status(hdr->f, hdr->addr, &hdr_status) < 0)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTGET, FAIL, "unable to check metadata cache status for v2 B-tree header")

    /* Sanity checks on v2 B-tree header */
    HDassert(hdr_status & H5AC_ES__IN_CACHE);
    HDassert(hdr_status & H5AC_ES__IS_PROTECTED);
} /* end block */
#endif /* NDEBUG */

    /* Delete all nodes in B-tree */
    if(H5F_addr_defined(hdr->root.addr))
        if(H5B2_delete_node(hdr, dxpl_id, hdr->depth, &hdr->root, hdr->remove_op, hdr->remove_op_data) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDELETE, FAIL, "unable to delete B-tree nodes")

    /* Indicate that the heap header should be deleted & file space freed */
    cache_flags |= H5AC__DIRTIED_FLAG | H5AC__DELETED_FLAG | H5AC__FREE_FILE_SPACE_FLAG;

done:
    /* Unprotect the header with appropriate flags */
    if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_HDR, hdr->addr, hdr, cache_flags) < 0)
        HDONE_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2_hdr_delete() */

