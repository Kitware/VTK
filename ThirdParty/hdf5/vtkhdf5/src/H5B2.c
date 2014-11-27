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
 * Created:		H5B2.c
 *			Jan 31 2005
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Implements a B-tree, with several modifications from
 *                      the "standard" methods.
 *
 *                      Please see the documentation in:
 *                      doc/html/TechNotes/Btrees.html for a full description
 *                      of how they work, etc.
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

/* v2 B-tree client ID to class mapping */

/* Remember to add client ID to H5B2_subid_t in H5B2private.h when adding a new
 * client class..
 */
extern const H5B2_class_t H5B2_TEST[1];
extern const H5B2_class_t H5HF_HUGE_BT2_INDIR[1];
extern const H5B2_class_t H5HF_HUGE_BT2_FILT_INDIR[1];
extern const H5B2_class_t H5HF_HUGE_BT2_DIR[1];
extern const H5B2_class_t H5HF_HUGE_BT2_FILT_DIR[1];
extern const H5B2_class_t H5G_BT2_NAME[1];
extern const H5B2_class_t H5G_BT2_CORDER[1];
extern const H5B2_class_t H5SM_INDEX[1];
extern const H5B2_class_t H5A_BT2_NAME[1];
extern const H5B2_class_t H5A_BT2_CORDER[1];

const H5B2_class_t *const H5B2_client_class_g[] = {
    H5B2_TEST,			/* 0 - H5B2_TEST_ID 			*/
    H5HF_HUGE_BT2_INDIR,	/* 1 - H5B2_FHEAP_HUGE_INDIR_ID 	*/
    H5HF_HUGE_BT2_FILT_INDIR,	/* 2 - H5B2_FHEAP_HUGE_FILT_INDIR_ID 	*/
    H5HF_HUGE_BT2_DIR,		/* 3 - H5B2_FHEAP_HUGE_DIR_ID 		*/
    H5HF_HUGE_BT2_FILT_DIR,	/* 4 - H5B2_FHEAP_HUGE_FILT_DIR_ID 	*/
    H5G_BT2_NAME,		/* 5 - H5B2_GRP_DENSE_NAME_ID 		*/
    H5G_BT2_CORDER,		/* 6 - H5B2_GRP_DENSE_CORDER_ID 	*/
    H5SM_INDEX,			/* 7 - H5B2_SOHM_INDEX_ID 		*/
    H5A_BT2_NAME,		/* 8 - H5B2_ATTR_DENSE_NAME_ID 		*/
    H5A_BT2_CORDER,		/* 9 - H5B2_ATTR_DENSE_CORDER_ID 	*/
};


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage the H5B2_t struct */
H5FL_DEFINE_STATIC(H5B2_t);



/*-------------------------------------------------------------------------
 * Function:	H5B2_create
 *
 * Purpose:	Creates a new empty B-tree in the file.
 *
 * Return:	Non-negative on success (with address of new B-tree
 *              filled in), negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Jan 31 2005
 *
 *-------------------------------------------------------------------------
 */
H5B2_t *
H5B2_create(H5F_t *f, hid_t dxpl_id, const H5B2_create_t *cparam, void *ctx_udata)
{
    H5B2_t	*bt2 = NULL;            /* Pointer to the B-tree */
    H5B2_hdr_t	*hdr = NULL;            /* Pointer to the B-tree header */
    H5B2_hdr_cache_ud_t cache_udata;    /* User-data for callback */
    haddr_t     hdr_addr;               /* B-tree header address */
    H5B2_t	*ret_value;             /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(cparam);

    /* H5B2 interface sanity check */
    HDcompile_assert(H5B2_NUM_BTREE_ID == NELMTS(H5B2_client_class_g));

    /* Create shared v2 B-tree header */
    if(HADDR_UNDEF == (hdr_addr = H5B2_hdr_create(f, dxpl_id, cparam, ctx_udata)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTINIT, NULL, "can't create v2 B-tree header")

    /* Create v2 B-tree wrapper */
    if(NULL == (bt2 = H5FL_MALLOC(H5B2_t)))
        HGOTO_ERROR(H5E_BTREE, H5E_CANTALLOC, NULL, "memory allocation failed for v2 B-tree info")

    /* Look up the B-tree header */
    cache_udata.f = f;
    cache_udata.ctx_udata = ctx_udata;
    if(NULL == (hdr = (H5B2_hdr_t *)H5AC_protect(f, dxpl_id, H5AC_BT2_HDR, hdr_addr, &cache_udata, H5AC_WRITE)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTPROTECT, NULL, "unable to load B-tree header")

    /* Point v2 B-tree wrapper at header and bump it's ref count */
    bt2->hdr = hdr;
    if(H5B2_hdr_incr(bt2->hdr) < 0)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTINC, NULL, "can't increment reference count on shared v2 B-tree header")

    /* Increment # of files using this v2 B-tree header */
    if(H5B2_hdr_fuse_incr(bt2->hdr) < 0)
	HGOTO_ERROR(H5E_BTREE, H5E_CANTINC, NULL, "can't increment file reference count on shared v2 B-tree header")

    /* Set file pointer for this v2 B-tree open context */
    bt2->f = f;

    /* Set the return value */
    ret_value = bt2;

done:
    if(hdr && H5AC_unprotect(f, dxpl_id, H5AC_BT2_HDR, hdr_addr, hdr, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, NULL, "unable to release v2 B-tree header")
    if(!ret_value && bt2)
        if(H5B2_close(bt2, dxpl_id) < 0)
            HDONE_ERROR(H5E_BTREE, H5E_CANTCLOSEOBJ, NULL, "unable to close v2 B-tree")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2_create() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_open
 *
 * Purpose:	Opens an existing v2 B-tree in the file.
 *
 * Return:	Pointer to v2 B-tree wrapper on success
 *              NULL on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct 15 2009
 *
 *-------------------------------------------------------------------------
 */
H5B2_t *
H5B2_open(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *ctx_udata)
{
    H5B2_t	*bt2 = NULL;            /* Pointer to the B-tree */
    H5B2_hdr_t	*hdr = NULL;            /* Pointer to the B-tree header */
    H5B2_hdr_cache_ud_t cache_udata;    /* User-data for callback */
    H5B2_t	*ret_value;             /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check arguments. */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));

    /* Look up the B-tree header */
    cache_udata.f = f;
    cache_udata.ctx_udata = ctx_udata;
    if(NULL == (hdr = (H5B2_hdr_t *)H5AC_protect(f, dxpl_id, H5AC_BT2_HDR, addr, &cache_udata, H5AC_READ)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTPROTECT, NULL, "unable to load B-tree header")

    /* Check for pending heap deletion */
    if(hdr->pending_delete)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTOPENOBJ, NULL, "can't open v2 B-tree pending deletion")

    /* Create v2 B-tree info */
    if(NULL == (bt2 = H5FL_MALLOC(H5B2_t)))
        HGOTO_ERROR(H5E_BTREE, H5E_CANTALLOC, NULL, "memory allocation failed for v2 B-tree info")

    /* Point v2 B-tree wrapper at header */
    bt2->hdr = hdr;
    if(H5B2_hdr_incr(bt2->hdr) < 0)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTINC, NULL, "can't increment reference count on shared v2 B-tree header")

    /* Increment # of files using this v2 B-tree header */
    if(H5B2_hdr_fuse_incr(bt2->hdr) < 0)
	HGOTO_ERROR(H5E_BTREE, H5E_CANTINC, NULL, "can't increment file reference count on shared v2 B-tree header")

    /* Set file pointer for this v2 B-tree open context */
    bt2->f = f;

    /* Set the return value */
    ret_value = bt2;

done:
    if(hdr && H5AC_unprotect(f, dxpl_id, H5AC_BT2_HDR, addr, hdr, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, NULL, "unable to release v2 B-tree header")
    if(!ret_value && bt2)
        if(H5B2_close(bt2, dxpl_id) < 0)
            HDONE_ERROR(H5E_BTREE, H5E_CANTCLOSEOBJ, NULL, "unable to close v2 B-tree")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2_open() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_insert
 *
 * Purpose:	Adds a new record to the B-tree.
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
H5B2_insert(H5B2_t *bt2, hid_t dxpl_id, void *udata)
{
    H5B2_hdr_t	*hdr;                   /* Pointer to the B-tree header */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments. */
    HDassert(bt2);
    HDassert(udata);

    /* Set the shared v2 B-tree header's file context for this operation */
    bt2->hdr->f = bt2->f;

    /* Get the v2 B-tree header */
    hdr = bt2->hdr;

    /* Check if the root node is allocated yet */
    if(!H5F_addr_defined(hdr->root.addr)) {
        /* Create root node as leaf node in B-tree */
        if(H5B2_create_leaf(hdr, dxpl_id, &(hdr->root)) < 0)
	    HGOTO_ERROR(H5E_BTREE, H5E_CANTINIT, FAIL, "unable to create root node")
    } /* end if */
    /* Check if we need to split the root node (equiv. to a 1->2 node split) */
    else if(hdr->root.node_nrec == hdr->node_info[hdr->depth].split_nrec) {
        /* Split root node */
        if(H5B2_split_root(hdr, dxpl_id) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTSPLIT, FAIL, "unable to split root node")
    } /* end if */

    /* Attempt to insert record into B-tree */
    if(hdr->depth > 0) {
        if(H5B2_insert_internal(hdr, dxpl_id, hdr->depth, NULL, &hdr->root, udata) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTINSERT, FAIL, "unable to insert record into B-tree internal node")
    } /* end if */
    else {
        if(H5B2_insert_leaf(hdr, dxpl_id, &hdr->root, udata) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTINSERT, FAIL, "unable to insert record into B-tree leaf node")
    } /* end else */

    /* Mark B-tree header as dirty */
    if(H5B2_hdr_dirty(hdr) < 0)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTMARKDIRTY, FAIL, "unable to mark B-tree header dirty")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2_insert() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_get_addr
 *
 * Purpose:	Get the address of a v2 B-tree
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Nov  5 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_get_addr(const H5B2_t *bt2, haddr_t *addr_p)
{
    FUNC_ENTER_NOAPI_NOERR

    /*
     * Check arguments.
     */
    HDassert(bt2);
    HDassert(addr_p);

    /* Retrieve the header address for this v2 B-tree */
    *addr_p = bt2->hdr->addr;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5B2_get_addr() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_iterate
 *
 * Purpose:	Iterate over all the records in the B-tree, in "in-order"
 *		order, making a callback for each record.
 *
 *              If the callback returns non-zero, the iteration breaks out
 *              without finishing all the records.
 *
 * Return:	Value from callback: non-negative on success, negative on error
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 11 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_iterate(H5B2_t *bt2, hid_t dxpl_id, H5B2_operator_t op, void *op_data)
{
    H5B2_hdr_t	*hdr;                   /* Pointer to the B-tree header */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOERR

    /* Check arguments. */
    HDassert(bt2);
    HDassert(op);

    /* Set the shared v2 B-tree header's file context for this operation */
    bt2->hdr->f = bt2->f;

    /* Get the v2 B-tree header */
    hdr = bt2->hdr;

    /* Iterate through records */
    if(hdr->root.node_nrec > 0) {
        /* Iterate through nodes */
        if((ret_value = H5B2_iterate_node(hdr, dxpl_id, hdr->depth, &hdr->root, op, op_data)) < 0)
            HERROR(H5E_BTREE, H5E_CANTLIST, "node iteration failed");
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2_iterate() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_find
 *
 * Purpose:	Locate the specified information in a B-tree and return
 *		that information by calling the provided 'OP' routine with an
 *		OP_DATA pointer.  The UDATA parameter points to data passed
 *		to the key comparison function.
 *
 *              The 'OP' routine is called with the record found and the
 *              OP_DATA pointer, to allow caller to return information about
 *              the record.
 *
 *              If 'OP' is NULL, then this routine just returns "TRUE" when
 *              a record is present in the B-tree.
 *
 * Return:	Non-negative (TRUE/FALSE) on success, negative on failure.
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 23 2005
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5B2_find(H5B2_t *bt2, hid_t dxpl_id, void *udata, H5B2_found_t op,
    void *op_data)
{
    H5B2_hdr_t	*hdr;                   /* Pointer to the B-tree header */
    H5B2_node_ptr_t curr_node_ptr;      /* Node pointer info for current node */
    unsigned    depth;                  /* Current depth of the tree */
    int         cmp;                    /* Comparison value of records */
    unsigned    idx;                    /* Location of record which matches key */
    htri_t	ret_value = TRUE;       /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments. */
    HDassert(bt2);

    /* Set the shared v2 B-tree header's file context for this operation */
    bt2->hdr->f = bt2->f;

    /* Get the v2 B-tree header */
    hdr = bt2->hdr;

    /* Make copy of the root node pointer to start search with */
    curr_node_ptr = hdr->root;

    /* Current depth of the tree */
    depth = hdr->depth;

    /* Check for empty tree */
    if(curr_node_ptr.node_nrec == 0)
        HGOTO_DONE(FALSE)

    /* Walk down B-tree to find record or leaf node where record is located */
    cmp = -1;
    while(depth > 0 && cmp != 0) {
        H5B2_internal_t *internal;          /* Pointer to internal node in B-tree */
        H5B2_node_ptr_t next_node_ptr;      /* Node pointer info for next node */

        /* Lock B-tree current node */
        if(NULL == (internal = H5B2_protect_internal(hdr, dxpl_id, curr_node_ptr.addr, curr_node_ptr.node_nrec, depth, H5AC_READ)))
            HGOTO_ERROR(H5E_BTREE, H5E_CANTPROTECT, FAIL, "unable to load B-tree internal node")

        /* Locate node pointer for child */
        cmp = H5B2_locate_record(hdr->cls, internal->nrec, hdr->nat_off, internal->int_native, udata, &idx);
        if(cmp > 0)
            idx++;

        if(cmp != 0) {
            /* Get node pointer for next node to search */
            next_node_ptr=internal->node_ptrs[idx];

            /* Unlock current node */
            if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_INT, curr_node_ptr.addr, internal, H5AC__NO_FLAGS_SET) < 0)
                HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")

            /* Set pointer to next node to load */
            curr_node_ptr = next_node_ptr;
        } /* end if */
        else {
            /* Make callback for current record */
            if(op && (op)(H5B2_INT_NREC(internal, hdr, idx), op_data) < 0) {
                /* Unlock current node */
                if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_INT, curr_node_ptr.addr, internal, H5AC__NO_FLAGS_SET) < 0)
                    HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")

                HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL, "'found' callback failed for B-tree find operation")
            } /* end if */

            /* Unlock current node */
            if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_INT, curr_node_ptr.addr, internal, H5AC__NO_FLAGS_SET) < 0)
                HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")

            /* Indicate record found */
            HGOTO_DONE(TRUE)
        } /* end else */

        /* Decrement depth we're at in B-tree */
        depth--;
    } /* end while */

    {
        H5B2_leaf_t *leaf;          /* Pointer to leaf node in B-tree */

        /* Lock B-tree leaf node */
        if(NULL == (leaf = H5B2_protect_leaf(hdr, dxpl_id, curr_node_ptr.addr, curr_node_ptr.node_nrec, H5AC_READ)))
            HGOTO_ERROR(H5E_BTREE, H5E_CANTPROTECT, FAIL, "unable to protect B-tree leaf node")

        /* Locate record */
        cmp = H5B2_locate_record(hdr->cls, leaf->nrec, hdr->nat_off, leaf->leaf_native, udata, &idx);

        if(cmp != 0) {
            /* Unlock leaf node */
            if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_LEAF, curr_node_ptr.addr, leaf, H5AC__NO_FLAGS_SET) < 0)
                HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")

            /* Record not found */
            HGOTO_DONE(FALSE)
        } /* end if */
        else {
            /* Make callback for current record */
            if(op && (op)(H5B2_LEAF_NREC(leaf, hdr, idx), op_data) < 0) {
                /* Unlock current node */
                if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_LEAF, curr_node_ptr.addr, leaf, H5AC__NO_FLAGS_SET) < 0)
                    HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")

                HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL, "'found' callback failed for B-tree find operation")
            } /* end if */
        } /* end else */

        /* Unlock current node */
        if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_LEAF, curr_node_ptr.addr, leaf, H5AC__NO_FLAGS_SET) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")
    } /* end block */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2_find() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_index
 *
 * Purpose:	Locate the IDX'th record in a B-tree according to the
 *              ordering used by the B-tree.  The IDX values are 0-based.
 *
 *              The 'OP' routine is called with the record found and the
 *              OP_DATA pointer, to allow caller to return information about
 *              the record.
 *
 * Return:	Non-negative on success, negative on failure.
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 23 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_index(H5B2_t *bt2, hid_t dxpl_id, H5_iter_order_t order, hsize_t idx,
    H5B2_found_t op, void *op_data)
{
    H5B2_hdr_t	*hdr;                   /* Pointer to the B-tree header */
    H5B2_node_ptr_t curr_node_ptr;      /* Node pointer info for current node */
    unsigned    depth;                  /* Current depth of the tree */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments. */
    HDassert(bt2);
    HDassert(op);

    /* Set the shared v2 B-tree header's file context for this operation */
    bt2->hdr->f = bt2->f;

    /* Get the v2 B-tree header */
    hdr = bt2->hdr;

    /* Make copy of the root node pointer to start search with */
    curr_node_ptr = hdr->root;

    /* Current depth of the tree */
    depth = hdr->depth;

    /* Check for empty tree */
    if(curr_node_ptr.node_nrec == 0)
        HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL, "B-tree has no records")

    /* Check for index greater than the number of records in the tree */
    if(idx >= curr_node_ptr.all_nrec)
        HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL, "B-tree doesn't have that many records")

    /* Check for reverse indexing and map requested index to appropriate forward index */
    if(order == H5_ITER_DEC)
        idx = curr_node_ptr.all_nrec - (idx + 1);

    /* Walk down B-tree to find record or leaf node where record is located */
    while(depth > 0) {
        H5B2_internal_t *internal;          /* Pointer to internal node in B-tree */
        H5B2_node_ptr_t next_node_ptr;      /* Node pointer info for next node */
        unsigned u;                         /* Local index variable */

        /* Lock B-tree current node */
        if(NULL == (internal = H5B2_protect_internal(hdr, dxpl_id, curr_node_ptr.addr, curr_node_ptr.node_nrec, depth, H5AC_READ)))
            HGOTO_ERROR(H5E_BTREE, H5E_CANTPROTECT, FAIL, "unable to load B-tree internal node")

        /* Search for record with correct index */
        for(u = 0; u < internal->nrec; u++) {
            /* Check if record is in child node */
            if(internal->node_ptrs[u].all_nrec > idx) {
                /* Get node pointer for next node to search */
                next_node_ptr = internal->node_ptrs[u];

                /* Unlock current node */
                if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_INT, curr_node_ptr.addr, internal, H5AC__NO_FLAGS_SET) < 0)
                    HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")

                /* Set pointer to next node to load */
                curr_node_ptr = next_node_ptr;

                /* Break out of for loop */
                break;
            } /* end if */

            /* Check if record is in this node */
            if(internal->node_ptrs[u].all_nrec == idx) {
                /* Make callback for current record */
                if((op)(H5B2_INT_NREC(internal, hdr, u), op_data) < 0) {
                    /* Unlock current node */
                    if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_INT, curr_node_ptr.addr, internal, H5AC__NO_FLAGS_SET) < 0)
                        HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")

                    HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL, "'found' callback failed for B-tree find operation")
                } /* end if */

                /* Unlock current node */
                if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_INT, curr_node_ptr.addr, internal, H5AC__NO_FLAGS_SET) < 0)
                    HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")

                HGOTO_DONE(SUCCEED);
            } /* end if */

            /* Decrement index we are looking for to account for the node we
             * just advanced past.
             */
            idx -= (internal->node_ptrs[u].all_nrec + 1);
        } /* end for */

        /* Check last node pointer */
        if(u == internal->nrec) {
            /* Check if record is in child node */
            if(internal->node_ptrs[u].all_nrec > idx) {
                /* Get node pointer for next node to search */
                next_node_ptr = internal->node_ptrs[u];

                /* Unlock current node */
                if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_INT, curr_node_ptr.addr, internal, H5AC__NO_FLAGS_SET) < 0)
                    HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")

                /* Set pointer to next node to load */
                curr_node_ptr = next_node_ptr;
            } /* end if */
            else
                /* Index that is greater than the number of records in the tree? */
                HDassert("Index off end of tree??" && 0);
        } /* end if */

        /* Decrement depth we're at in B-tree */
        depth--;
    } /* end while */

    {
        H5B2_leaf_t *leaf;          /* Pointer to leaf node in B-tree */

        /* Lock B-tree leaf node */
        if(NULL == (leaf = H5B2_protect_leaf(hdr, dxpl_id, curr_node_ptr.addr, curr_node_ptr.node_nrec, H5AC_READ)))
            HGOTO_ERROR(H5E_BTREE, H5E_CANTPROTECT, FAIL, "unable to protect B-tree leaf node")

        /* Sanity check index */
        HDassert(idx < leaf->nrec);

        /* Make callback for correct record */
        if((op)(H5B2_LEAF_NREC(leaf, hdr, idx), op_data) < 0) {
            /* Unlock current node */
            if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_LEAF, curr_node_ptr.addr, leaf, H5AC__NO_FLAGS_SET) < 0)
                HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")

            HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL, "'found' callback failed for B-tree find operation")
        } /* end if */

        /* Unlock current node */
        if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_LEAF, curr_node_ptr.addr, leaf, H5AC__NO_FLAGS_SET) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")
    } /* end block */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2_index() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_remove
 *
 * Purpose:	Removes a record from a B-tree.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 25 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_remove(H5B2_t *bt2, hid_t dxpl_id, void *udata, H5B2_remove_t op,
    void *op_data)
{
    H5B2_hdr_t	*hdr;                   /* Pointer to the B-tree header */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments. */
    HDassert(bt2);

    /* Set the shared v2 B-tree header's file context for this operation */
    bt2->hdr->f = bt2->f;

    /* Get the v2 B-tree header */
    hdr = bt2->hdr;

    /* Check for empty B-tree */
    if(0 == hdr->root.all_nrec)
        HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL, "record is not in B-tree")

    /* Attempt to remove record from B-tree */
    if(hdr->depth > 0) {
        hbool_t depth_decreased = FALSE;  /* Flag to indicate whether the depth of the B-tree decreased */

        if(H5B2_remove_internal(hdr, dxpl_id, &depth_decreased, NULL, hdr->depth,
                &(hdr->cache_info), NULL, &hdr->root, udata, op, op_data) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDELETE, FAIL, "unable to remove record from B-tree internal node")

        /* Check for decreasing the depth of the B-tree */
        if(depth_decreased) {
            /* Destroy free list factories for previous depth */
            if(hdr->node_info[hdr->depth].nat_rec_fac)
                if(H5FL_fac_term(hdr->node_info[hdr->depth].nat_rec_fac) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't destroy node's native record block factory")
            if(hdr->node_info[hdr->depth].node_ptr_fac)
                if(H5FL_fac_term(hdr->node_info[hdr->depth].node_ptr_fac) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't destroy node's node pointer block factory")

            HDassert((uint16_t)(hdr->depth - depth_decreased) < hdr->depth);
            hdr->depth = (uint16_t)(hdr->depth - depth_decreased);
        } /* end for */
    } /* end if */
    else {
        if(H5B2_remove_leaf(hdr, dxpl_id, &hdr->root, udata, op, op_data) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDELETE, FAIL, "unable to remove record from B-tree leaf node")
    } /* end else */

    /* Decrement # of records in B-tree */
    hdr->root.all_nrec--;

    /* Mark B-tree header as dirty */
    if(H5B2_hdr_dirty(hdr) < 0)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTMARKDIRTY, FAIL, "unable to mark B-tree header dirty")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2_remove() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_remove_by_idx
 *
 * Purpose:	Removes the n'th record from a B-tree.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Nov 14 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_remove_by_idx(H5B2_t *bt2, hid_t dxpl_id, H5_iter_order_t order,
    hsize_t idx, H5B2_remove_t op, void *op_data)
{
    H5B2_hdr_t	*hdr;                   /* Pointer to the B-tree header */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments. */
    HDassert(bt2);

    /* Set the shared v2 B-tree header's file context for this operation */
    bt2->hdr->f = bt2->f;

    /* Get the v2 B-tree header */
    hdr = bt2->hdr;

    /* Check for empty B-tree */
    if(0 == hdr->root.all_nrec)
        HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL, "record is not in B-tree")

    /* Check for index greater than the number of records in the tree */
    if(idx >= hdr->root.all_nrec)
        HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL, "B-tree doesn't have that many records")

    /* Check for reverse indexing and map requested index to appropriate forward index */
    if(H5_ITER_DEC == order)
        idx = hdr->root.all_nrec - (idx + 1);

    /* Attempt to remove record from B-tree */
    if(hdr->depth > 0) {
        hbool_t depth_decreased = FALSE;  /* Flag to indicate whether the depth of the B-tree decreased */

        if(H5B2_remove_internal_by_idx(hdr, dxpl_id, &depth_decreased, NULL, hdr->depth,
                &(hdr->cache_info), NULL, &hdr->root, idx, op, op_data) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDELETE, FAIL, "unable to remove record from B-tree internal node")

        /* Check for decreasing the depth of the B-tree */
        if(depth_decreased) {
            /* Destroy free list factories for previous depth */
            if(hdr->node_info[hdr->depth].nat_rec_fac)
                if(H5FL_fac_term(hdr->node_info[hdr->depth].nat_rec_fac) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't destroy node's native record block factory")
            if(hdr->node_info[hdr->depth].node_ptr_fac)
                if(H5FL_fac_term(hdr->node_info[hdr->depth].node_ptr_fac) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't destroy node's node pointer block factory")

            HDassert((uint16_t)(hdr->depth - depth_decreased) < hdr->depth);
            hdr->depth = (uint16_t)(hdr->depth - depth_decreased);
        } /* end for */
    } /* end if */
    else {
        if(H5B2_remove_leaf_by_idx(hdr, dxpl_id, &hdr->root, (unsigned)idx, op, op_data) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDELETE, FAIL, "unable to remove record from B-tree leaf node")
    } /* end else */

    /* Decrement # of records in B-tree */
    hdr->root.all_nrec--;

    /* Mark B-tree header as dirty */
    if(H5B2_hdr_dirty(hdr) < 0)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTMARKDIRTY, FAIL, "unable to mark B-tree header dirty")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2_remove_by_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_get_nrec
 *
 * Purpose:	Retrieves the number of records in a B-tree
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 25 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_get_nrec(const H5B2_t *bt2, hsize_t *nrec)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check arguments. */
    HDassert(bt2);
    HDassert(nrec);

    /* Get B-tree number of records */
    *nrec = bt2->hdr->root.all_nrec;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5B2_get_nrec() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_neighbor
 *
 * Purpose:	Locate a record relative to the specified information in a
 *              B-tree and return that information by filling in fields of the
 *              caller-supplied UDATA pointer depending on the type of leaf node
 *		requested.  The UDATA can point to additional data passed
 *		to the key comparison function.
 *
 *              The 'OP' routine is called with the record found and the
 *              OP_DATA pointer, to allow caller to return information about
 *              the record.
 *
 *              The RANGE indicates whether to search for records less than or
 *              equal to, or greater than or equal to the information passed
 *              in with UDATA.
 *
 * Return:	Non-negative on success, negative on failure.
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar  8 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_neighbor(H5B2_t *bt2, hid_t dxpl_id, H5B2_compare_t range, void *udata,
    H5B2_found_t op, void *op_data)
{
    H5B2_hdr_t	*hdr;                   /* Pointer to the B-tree header */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments. */
    HDassert(bt2);
    HDassert(op);

    /* Set the shared v2 B-tree header's file context for this operation */
    bt2->hdr->f = bt2->f;

    /* Get the v2 B-tree header */
    hdr = bt2->hdr;

    /* Check for empty tree */
    if(!H5F_addr_defined(hdr->root.addr))
        HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL, "B-tree has no records")

    /* Attempt to find neighbor record in B-tree */
    if(hdr->depth > 0) {
        if(H5B2_neighbor_internal(hdr, dxpl_id, hdr->depth, &hdr->root, NULL, range, udata, op, op_data) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL, "unable to find neighbor record in B-tree internal node")
    } /* end if */
    else {
        if(H5B2_neighbor_leaf(hdr, dxpl_id, &hdr->root, NULL, range, udata, op, op_data) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL, "unable to find neighbor record in B-tree leaf node")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2_neighbor() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_modify
 *
 * Purpose:	Locate the specified information in a B-tree and modify it.
 *		The UDATA points to additional data passed
 *		to the key comparison function for locating the record to
 *              modify.
 *
 *              The 'OP' routine is called with the record found and the
 *              OP_DATA pointer, to allow caller to modify information about
 *              the record.
 *
 * Return:	Non-negative on success, negative on failure.
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar 10 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_modify(H5B2_t *bt2, hid_t dxpl_id, void *udata, H5B2_modify_t op,
    void *op_data)
{
    H5B2_hdr_t	*hdr;                   /* Pointer to the B-tree header */
    H5B2_node_ptr_t curr_node_ptr;      /* Node pointer info for current node */
    unsigned    depth;                  /* Current depth of the tree */
    int         cmp;                    /* Comparison value of records */
    unsigned    idx;                    /* Location of record which matches key */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments. */
    HDassert(bt2);
    HDassert(op);

    /* Set the shared v2 B-tree header's file context for this operation */
    bt2->hdr->f = bt2->f;

    /* Get the v2 B-tree header */
    hdr = bt2->hdr;

    /* Make copy of the root node pointer to start search with */
    curr_node_ptr = hdr->root;

    /* Current depth of the tree */
    depth = hdr->depth;

    /* Check for empty tree */
    if(0 == curr_node_ptr.node_nrec)
        HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL, "B-tree has no records")

    /* Walk down B-tree to find record or leaf node where record is located */
    cmp = -1;
    while(depth > 0 && cmp != 0) {
        unsigned internal_flags = H5AC__NO_FLAGS_SET;
        H5B2_internal_t *internal;          /* Pointer to internal node in B-tree */
        H5B2_node_ptr_t next_node_ptr;      /* Node pointer info for next node */

        /* Lock B-tree current node */
        if(NULL == (internal = H5B2_protect_internal(hdr, dxpl_id, curr_node_ptr.addr, curr_node_ptr.node_nrec, depth, H5AC_WRITE)))
            HGOTO_ERROR(H5E_BTREE, H5E_CANTPROTECT, FAIL, "unable to load B-tree internal node")

        /* Locate node pointer for child */
        cmp = H5B2_locate_record(hdr->cls, internal->nrec, hdr->nat_off, internal->int_native, udata, &idx);
        if(cmp > 0)
            idx++;

        if(cmp != 0) {
            /* Get node pointer for next node to search */
            next_node_ptr = internal->node_ptrs[idx];

            /* Unlock current node */
            if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_INT, curr_node_ptr.addr, internal, H5AC__NO_FLAGS_SET) < 0)
                HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")

            /* Set pointer to next node to load */
            curr_node_ptr = next_node_ptr;
        } /* end if */
        else {
            hbool_t changed;            /* Whether the 'modify' callback changed the record */

            /* Make callback for current record */
            if((op)(H5B2_INT_NREC(internal, hdr, idx), op_data, &changed) < 0) {
                /* Make certain that the callback didn't modify the value if it failed */
                HDassert(changed == FALSE);

                /* Unlock current node */
                if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_INT, curr_node_ptr.addr, internal, H5AC__NO_FLAGS_SET) < 0)
                    HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")

                HGOTO_ERROR(H5E_BTREE, H5E_CANTMODIFY, FAIL, "'modify' callback failed for B-tree find operation")
            } /* end if */

            /* Mark the node as dirty if it changed */
            internal_flags |= changed ? H5AC__DIRTIED_FLAG : 0;

            /* Unlock current node */
            if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_INT, curr_node_ptr.addr, internal, internal_flags) < 0)
                HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")

            HGOTO_DONE(SUCCEED);
        } /* end else */

        /* Decrement depth we're at in B-tree */
        depth--;
    } /* end while */

    {
        H5B2_leaf_t *leaf;      /* Pointer to leaf node in B-tree */
        unsigned leaf_flags = H5AC__NO_FLAGS_SET;   /* Flags for unprotecting the leaf node */
        hbool_t changed = FALSE;/* Whether the 'modify' callback changed the record */

        /* Lock B-tree leaf node */
        if(NULL == (leaf = H5B2_protect_leaf(hdr, dxpl_id, curr_node_ptr.addr, curr_node_ptr.node_nrec, H5AC_WRITE)))
            HGOTO_ERROR(H5E_BTREE, H5E_CANTPROTECT, FAIL, "unable to protect B-tree leaf node")

        /* Locate record */
        cmp = H5B2_locate_record(hdr->cls, leaf->nrec, hdr->nat_off, leaf->leaf_native, udata, &idx);

        if(cmp != 0) {
            /* Unlock leaf node */
            if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_LEAF, curr_node_ptr.addr, leaf, H5AC__NO_FLAGS_SET) < 0)
                HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")

            /* Note: don't push error on stack, leave that to next higher level,
             *      since many times the B-tree is searched in order to determine
             *      if an object exists in the B-tree or not. -QAK
             */
#ifdef OLD_WAY
                HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL, "key not found in leaf node")
#else /* OLD_WAY */
                HGOTO_DONE(FAIL)
#endif /* OLD_WAY */
        } /* end if */
        else {
            /* Make callback for current record */
            if((op)(H5B2_LEAF_NREC(leaf, hdr, idx), op_data, &changed) < 0) {
                /* Make certain that the callback didn't modify the value if it failed */
                HDassert(changed == FALSE);

                /* Unlock current node */
                if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_LEAF, curr_node_ptr.addr, leaf, H5AC__NO_FLAGS_SET) < 0)
                    HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")

                HGOTO_ERROR(H5E_BTREE, H5E_CANTMODIFY, FAIL, "'modify' callback failed for B-tree find operation")
            } /* end if */
        } /* end else */

        /* Mark the node as dirty if it changed */
        leaf_flags |= (changed ? H5AC__DIRTIED_FLAG : 0);

        /* Unlock current node */
        if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_BT2_LEAF, curr_node_ptr.addr, leaf, leaf_flags) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release B-tree node")
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2_modify() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_close
 *
 * Purpose:	Close a v2 B-tree
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
H5B2_close(H5B2_t *bt2, hid_t dxpl_id)
{
    haddr_t bt2_addr = HADDR_UNDEF;     /* Address of v2 B-tree (for deletion) */
    hbool_t pending_delete = FALSE;     /* Whether the v2 B-tree is pending deletion */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check arguments. */
    HDassert(bt2);
    HDassert(bt2->f);

    /* Decrement file reference & check if this is the last open v2 B-tree using the shared B-tree header */
    if(0 == H5B2_hdr_fuse_decr(bt2->hdr)) {
        /* Set the shared v2 B-tree header's file context for this operation */
        bt2->hdr->f = bt2->f;

        /* Check for pending B-tree deletion */
        if(bt2->hdr->pending_delete) {
            /* Set local info, so B-tree deletion can occur after decrementing the
             *  header's ref count
             */
            pending_delete = TRUE;
            bt2_addr = bt2->hdr->addr;
        } /* end if */
    } /* end if */

    /* Check for pending v2 B-tree deletion */
    if(pending_delete) {
        H5B2_hdr_t *hdr;            /* Another pointer to v2 B-tree header */

        /* Sanity check */
        HDassert(H5F_addr_defined(bt2_addr));

#ifndef NDEBUG
{
    unsigned hdr_status = 0;         /* Header's status in the metadata cache */

    /* Check the header's status in the metadata cache */
    if(H5AC_get_entry_status(bt2->f, bt2_addr, &hdr_status) < 0)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTGET, FAIL, "unable to check metadata cache status for v2 B-tree header, address = %llu", (unsigned long long)bt2_addr)

    /* Sanity checks on header */
    HDassert(hdr_status & H5AC_ES__IN_CACHE);
    HDassert(hdr_status & H5AC_ES__IS_PINNED);
    HDassert(!(hdr_status & H5AC_ES__IS_PROTECTED));
}
#endif /* NDEBUG */

        /* Lock the v2 B-tree header into memory */
        /* (OK to pass in NULL for callback context, since we know the header must be in the cache) */
        if(NULL == (hdr = (H5B2_hdr_t *)H5AC_protect(bt2->f, dxpl_id, H5AC_BT2_HDR, bt2_addr, NULL, H5AC_WRITE)))
            HGOTO_ERROR(H5E_BTREE, H5E_CANTPROTECT, FAIL, "unable to protect v2 B-tree header")

        /* Set the shared v2 B-tree header's file context for this operation */
        hdr->f = bt2->f;

        /* Decrement the reference count on the B-tree header */
        /* (don't put in H5B2_hdr_fuse_decr() as the B-tree header may be evicted
         *  immediately -QAK)
         */
        if(H5B2_hdr_decr(bt2->hdr) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDEC, FAIL, "can't decrement reference count on shared v2 B-tree header")

        /* Delete v2 B-tree, starting with header (unprotects header) */
        if(H5B2_hdr_delete(hdr, dxpl_id) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDELETE, FAIL, "unable to delete v2 B-tree")
    } /* end if */
    else {
        /* Decrement the reference count on the B-tree header */
        /* (don't put in H5B2_hdr_fuse_decr() as the B-tree header may be evicted
         *  immediately -QAK)
         */
        if(H5B2_hdr_decr(bt2->hdr) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDEC, FAIL, "can't decrement reference count on shared v2 B-tree header")

    } /* end else */

    /* Release the v2 B-tree wrapper */
    bt2 = H5FL_FREE(H5B2_t, bt2);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2_close() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_delete
 *
 * Purpose:	Delete an entire B-tree from a file.
 *
 *              The 'OP' routine is called for each record and the
 *              OP_DATA pointer, to allow caller to perform an operation as
 *              each record is removed from the B-tree.
 *
 *              If 'OP' is NULL, the records are just removed in the process
 *              of deleting the B-tree.
 *
 * Note:	The records are _not_ guaranteed to be visited in order.
 *
 * Return:	Non-negative on success, negative on failure.
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar  9 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_delete(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *ctx_udata,
    H5B2_remove_t op, void *op_data)
{
    H5B2_hdr_t	*hdr = NULL;            /* Pointer to the B-tree header */
    H5B2_hdr_cache_ud_t cache_udata;    /* User-data for callback */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments. */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));

    /* Lock the v2 B-tree header into memory */
#ifdef QAK
HDfprintf(stderr, "%s: addr = %a\n", FUNC, addr);
#endif /* QAK */
    cache_udata.f = f;
    cache_udata.ctx_udata = ctx_udata;
    if(NULL == (hdr = (H5B2_hdr_t *)H5AC_protect(f, dxpl_id, H5AC_BT2_HDR, addr, &cache_udata, H5AC_WRITE)))
        HGOTO_ERROR(H5E_BTREE, H5E_CANTPROTECT, FAIL, "unable to protect v2 B-tree header")

    /* Remember the callback & context for later */
    hdr->remove_op = op;
    hdr->remove_op_data = op_data;

    /* Check for files using shared v2 B-tree header */
    if(hdr->file_rc)
        hdr->pending_delete = TRUE;
    else {
        /* Set the shared v2 B-tree header's file context for this operation */
        hdr->f = f;

        /* Delete v2 B-tree now, starting with header (unprotects header) */
        if(H5B2_hdr_delete(hdr, dxpl_id) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDELETE, FAIL, "unable to delete v2 B-tree")
        hdr = NULL;
    } /* end if */

done:
    /* Unprotect the header, if an error occurred */
    if(hdr && H5AC_unprotect(f, dxpl_id, H5AC_BT2_HDR, addr, hdr, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_BTREE, H5E_CANTUNPROTECT, FAIL, "unable to release v2 B-tree header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2_delete() */

