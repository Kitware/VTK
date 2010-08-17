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

/* Programmer: Robb Matzke <matzke@llnl.gov>
 *	       Friday, September 19, 1997
 *
 */
#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */
#define H5G_PACKAGE		/*suppress error about including H5Gpkg	  */


/* Packages needed by this file... */
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"		/* File access				*/
#include "H5Gpkg.h"		/* Groups		  		*/
#include "H5HLprivate.h"	/* Local Heaps				*/
#include "H5MMprivate.h"	/* Memory management			*/

/* Private typedefs */
/* User data for finding link information from B-tree */
typedef struct {
    /* downward */
    const char *name;           /* Name to search for */
    H5HL_t *heap;               /* Local heap for group */

    /* upward */
    H5O_link_t *lnk;            /* Caller's link location */
} H5G_stab_fnd_ud_t;

/* Data passed through B-tree iteration for looking up a name by index */
typedef struct H5G_bt_it_gnbi_t {
    /* downward */
    H5G_bt_it_idx_common_t common; /* Common information for "by index" lookup  */
    H5HL_t *heap;               /*symbol table heap 			     */

    /* upward */
    char         *name;         /*member name to be returned                 */
} H5G_bt_it_gnbi_t;

#ifndef H5_NO_DEPRECATED_SYMBOLS
/* Data passed through B-tree iteration for looking up a type by index */
typedef struct H5G_bt_it_gtbi_t {
    /* downward */
    H5G_bt_it_idx_common_t common; /* Common information for "by index" lookup  */
    H5F_t       *f;             /* Pointer to file that symbol table is in */
    hid_t       dxpl_id;        /* DXPL for operation */

    /* upward */
    H5G_obj_t    type;          /*member type to be returned                 */
} H5G_bt_it_gtbi_t;
#endif /* H5_NO_DEPRECATED_SYMBOLS */

/* Data passed through B-tree iteration for looking up a link by index */
typedef struct H5G_bt_it_lbi_t {
    /* downward */
    H5G_bt_it_idx_common_t common; /* Common information for "by index" lookup  */
    H5HL_t *heap;               /*symbol table heap 			     */

    /* upward */
    H5O_link_t *lnk;            /*link to be returned                        */
    hbool_t     found;      	/*whether we found the link                  */
} H5G_bt_it_lbi_t;

/* Private prototypes */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_create_components
 *
 * Purpose:	Creates the components for a new, empty, symbol table (name heap
 *		and B-tree).  The caller can specify an initial size for the
 *		name heap.
 *
 *		In order for the B-tree to operate correctly, the first
 *		item in the heap is the empty string, and must appear at
 *		heap offset zero.
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
H5G_stab_create_components(H5F_t *f, H5O_stab_t *stab, size_t size_hint, hid_t dxpl_id)
{
    H5HL_t *heap = NULL;            /* Pointer to local heap */
    size_t name_offset;	            /* Offset of "" name */
    herr_t ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_create_components, FAIL)

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(stab);
    HDassert(size_hint > 0);

    /* Create the B-tree */
    if(H5B_create(f, dxpl_id, H5B_SNODE, NULL, &(stab->btree_addr)/*out*/) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create B-tree")

    /* Create symbol table private heap */
    if(H5HL_create(f, dxpl_id, size_hint, &(stab->heap_addr)/*out*/) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create heap")

    /* Pin the heap down in memory */
    if(NULL == (heap = H5HL_protect(f, dxpl_id, stab->heap_addr, H5AC_WRITE)))
        HGOTO_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to protect symbol table heap")

    /* Insert name into the heap */
    if((size_t)(-1) == (name_offset = H5HL_insert(f, dxpl_id, heap, (size_t)1, "")))
	HGOTO_ERROR(H5E_SYM, H5E_CANTINSERT, FAIL, "can't insert name into heap")

    /*
     * B-tree's won't work if the first name isn't at the beginning
     * of the heap.
     */
    HDassert(0 == name_offset);

done:
    /* Release resources */
    if(heap && H5HL_unprotect(heap) < 0)
        HDONE_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to unprotect symbol table heap")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_create_components() */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_create
 *
 * Purpose:	Creates a new empty symbol table (object header, name heap,
 *		and B-tree).  The caller can specify an initial size for the
 *		name heap.  The object header of the group is opened for
 *		write access.
 *
 *		In order for the B-tree to operate correctly, the first
 *		item in the heap is the empty string, and must appear at
 *		heap offset zero.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug  1 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_create(H5O_loc_t *grp_oloc, hid_t dxpl_id, const H5O_ginfo_t *ginfo,
    H5O_stab_t *stab)
{
    size_t      heap_hint;              /* Local heap size hint */
    size_t      size_hint;              /* Local heap size hint */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_create, FAIL)

    /*
     * Check arguments.
     */
    HDassert(grp_oloc);
    HDassert(stab);

    /* Adjust the size hint, if necessary */
    if(ginfo->lheap_size_hint == 0)
        heap_hint = 8 +         /* "null" name inserted for B-tree */
                (ginfo->est_num_entries * H5HL_ALIGN(ginfo->est_name_len + 1)) +    /* estimated size of names for links, aligned for inserting into local heap */
                H5HL_SIZEOF_FREE(grp_oloc->file);       /* Free list entry in local heap */
    else
        heap_hint = ginfo->lheap_size_hint;
    size_hint = MAX(heap_hint, H5HL_SIZEOF_FREE(grp_oloc->file) + 2);

    /* Go create the B-tree & local heap */
    if(H5G_stab_create_components(grp_oloc->file, stab, size_hint, dxpl_id) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create symbol table components")

    /*
     * Insert the symbol table message into the object header and the symbol
     * table entry.
     */
    if(H5O_msg_create(grp_oloc, H5O_STAB_ID, 0, H5O_UPDATE_TIME, stab, dxpl_id) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't create message")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_create() */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_insert_real
 *
 * Purpose:	Insert a new symbol into a table.
 *		The name of the new symbol is NAME and its symbol
 *		table entry is OBJ_LNK.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@uiuc.edu
 *		Nov  7 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_insert_real(H5F_t *f, H5O_stab_t *stab, const char *name,
    H5O_link_t *obj_lnk, hid_t dxpl_id)
{
    H5HL_t       *heap = NULL;          /* Pointer to local heap */
    H5G_bt_ins_t udata;		        /* Data to pass through B-tree	*/
    herr_t       ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_insert_real, FAIL)

    /* check arguments */
    HDassert(f);
    HDassert(stab);
    HDassert(name && *name);
    HDassert(obj_lnk);

    /* Pin the heap down in memory */
    if(NULL == (heap = H5HL_protect(f, dxpl_id, stab->heap_addr, H5AC_WRITE)))
        HGOTO_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to protect symbol table heap")

    /* Initialize data to pass through B-tree */
    udata.common.name = name;
    udata.common.heap = heap;
    udata.lnk = obj_lnk;

    /* Insert into symbol table */
    if(H5B_insert(f, dxpl_id, H5B_SNODE, stab->btree_addr, &udata) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINSERT, FAIL, "unable to insert entry")

done:
    /* Release resources */
    if(heap && H5HL_unprotect(heap) < 0)
        HDONE_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to unprotect symbol table heap")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_insert_real() */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_insert
 *
 * Purpose:	Insert a new symbol into the table described by GRP_ENT in
 *		file F.	 The name of the new symbol is NAME and its symbol
 *		table entry is OBJ_ENT.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug  1 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_insert(const H5O_loc_t *grp_oloc, const char *name, H5O_link_t *obj_lnk,
    hid_t dxpl_id)
{
    H5O_stab_t		stab;		/* Symbol table message		*/
    herr_t              ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_insert, FAIL)

    /* check arguments */
    HDassert(grp_oloc && grp_oloc->file);
    HDassert(name && *name);
    HDassert(obj_lnk);

    /* Retrieve symbol table message */
    if(NULL == H5O_msg_read(grp_oloc, H5O_STAB_ID, &stab, dxpl_id))
        HGOTO_ERROR(H5E_SYM, H5E_BADMESG, FAIL, "not a symbol table")

    if(H5G_stab_insert_real(grp_oloc->file, &stab, name, obj_lnk, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, H5_ITER_ERROR, "unable to insert the name")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_insert() */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_remove
 *
 * Purpose:	Remove NAME from a symbol table.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, September 17, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_remove(H5O_loc_t *loc, hid_t dxpl_id, H5RS_str_t *grp_full_path_r,
    const char *name)
{
    H5HL_t      *heap = NULL;           /* Pointer to local heap */
    H5O_stab_t	stab;		        /*symbol table message		*/
    H5G_bt_rm_t	udata;		        /*data to pass through B-tree	*/
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_remove, FAIL)

    HDassert(loc && loc->file);
    HDassert(name && *name);

    /* Read in symbol table message */
    if(NULL == H5O_msg_read(loc, H5O_STAB_ID, &stab, dxpl_id))
        HGOTO_ERROR(H5E_SYM, H5E_BADMESG, FAIL, "not a symbol table")

    /* Pin the heap down in memory */
    if(NULL == (heap = H5HL_protect(loc->file, dxpl_id, stab.heap_addr, H5AC_WRITE)))
        HGOTO_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to protect symbol table heap")

    /* Initialize data to pass through B-tree */
    udata.common.name = name;
    udata.common.heap = heap;
    udata.grp_full_path_r = grp_full_path_r;

    /* Remove from symbol table */
    if(H5B_remove(loc->file, dxpl_id, H5B_SNODE, stab.btree_addr, &udata) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to remove entry")

done:
    /* Release resources */
    if(heap && H5HL_unprotect(heap) < 0)
        HDONE_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to unprotect symbol table heap")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_remove() */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_remove_by_idx
 *
 * Purpose:	Remove NAME from a symbol table, according to the name index.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, November 15, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_remove_by_idx(H5O_loc_t *grp_oloc, hid_t dxpl_id, H5RS_str_t *grp_full_path_r,
    H5_iter_order_t order, hsize_t n)
{
    H5HL_t      *heap = NULL;           /* Pointer to local heap */
    H5O_stab_t	stab;		        /* Symbol table message		*/
    H5G_bt_rm_t udata;		        /* Data to pass through B-tree	*/
    H5O_link_t  obj_lnk;                /* Object's link within group */
    hbool_t     lnk_copied = FALSE;     /* Whether the link was copied */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_remove_by_idx, FAIL)

    HDassert(grp_oloc && grp_oloc->file);

    /* Look up name of link to remove, by index */
    if(H5G_stab_lookup_by_idx(grp_oloc, order, n, &obj_lnk, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't get link information")
    lnk_copied = TRUE;

    /* Read in symbol table message */
    if(NULL == H5O_msg_read(grp_oloc, H5O_STAB_ID, &stab, dxpl_id))
        HGOTO_ERROR(H5E_SYM, H5E_BADMESG, FAIL, "not a symbol table")

    /* Pin the heap down in memory */
    if(NULL == (heap = H5HL_protect(grp_oloc->file, dxpl_id, stab.heap_addr, H5AC_WRITE)))
        HGOTO_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to protect symbol table heap")

    /* Initialize data to pass through B-tree */
    udata.common.name = obj_lnk.name;
    udata.common.heap = heap;
    udata.grp_full_path_r = grp_full_path_r;

    /* Remove link from symbol table */
    if(H5B_remove(grp_oloc->file, dxpl_id, H5B_SNODE, stab.btree_addr, &udata) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to remove entry")

done:
    /* Release resources */
    if(heap && H5HL_unprotect(heap) < 0)
        HDONE_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to unprotect symbol table heap")

    /* Reset the link information, if we have a copy */
    if(lnk_copied)
        H5O_msg_reset(H5O_LINK_ID, &obj_lnk);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_remove_by_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_delete
 *
 * Purpose:	Delete entire symbol table information from file
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, March 20, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_delete(H5F_t *f, hid_t dxpl_id, const H5O_stab_t *stab)
{
    H5HL_t *heap = NULL;                /* Pointer to local heap */
    H5G_bt_rm_t	udata;		        /*data to pass through B-tree	*/
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_delete, FAIL)

    HDassert(f);
    HDassert(stab);
    HDassert(H5F_addr_defined(stab->btree_addr));
    HDassert(H5F_addr_defined(stab->heap_addr));

    /* Pin the heap down in memory */
    if(NULL == (heap = H5HL_protect(f, dxpl_id, stab->heap_addr, H5AC_WRITE)))
        HGOTO_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to protect symbol table heap")

    /* Set up user data for B-tree deletion */
    udata.common.name = NULL;
    udata.common.heap = heap;

    /* Delete entire B-tree */
    if(H5B_delete(f, dxpl_id, H5B_SNODE, stab->btree_addr, &udata) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTDELETE, FAIL, "unable to delete symbol table B-tree")

    /* Release resources */
    if(H5HL_unprotect(heap) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to unprotect symbol table heap")
    heap = NULL;

    /* Delete local heap for names */
    if(H5HL_delete(f, dxpl_id, stab->heap_addr) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTDELETE, FAIL, "unable to delete symbol table heap")

done:
    /* Release resources */
    if(heap && H5HL_unprotect(heap) < 0)
        HDONE_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to unprotect symbol table heap")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_delete() */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_iterate
 *
 * Purpose:	Iterate over the objects in a group
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, October  3, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_iterate(const H5O_loc_t *oloc, hid_t dxpl_id, H5_iter_order_t order,
    hsize_t skip, hsize_t *last_lnk, H5G_lib_iterate_t op, void *op_data)
{
    H5HL_t *heap = NULL;                        /* Local heap for group */
    H5O_stab_t stab;		                /* Info about symbol table */
    H5G_link_table_t ltable = {0, NULL};        /* Link table */
    herr_t ret_value;                           /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_iterate, FAIL)

    /* Sanity check */
    HDassert(oloc);
    HDassert(op);

    /* Get the B-tree info */
    if(NULL == H5O_msg_read(oloc, H5O_STAB_ID, &stab, dxpl_id))
	HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "unable to determine local heap address")

    /* Pin the heap down in memory */
    if(NULL == (heap = H5HL_protect(oloc->file, dxpl_id, stab.heap_addr, H5AC_READ)))
        HGOTO_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to protect symbol table heap")

    /* Check on iteration order */
    /* ("native" iteration order is increasing for this link storage mechanism) */
    if(order != H5_ITER_DEC) {
        H5G_bt_it_it_t	udata;                  /* User data to pass to B-tree callback */

        /* Build udata to pass through H5B_iterate() to H5G_node_iterate() */
        udata.heap = heap;
        udata.skip = skip;
        udata.final_ent = last_lnk;
        udata.op = op;
        udata.op_data = op_data;

        /* Iterate over the group members */
        if((ret_value = H5B_iterate(oloc->file, dxpl_id, H5B_SNODE, stab.btree_addr, H5G_node_iterate, &udata)) < 0)
            HERROR(H5E_SYM, H5E_CANTNEXT, "iteration operator failed");

        /* Check for too high of a starting index (ex post facto :-) */
        /* (Skipping exactly as many entries as are in the group is currently an error) */
        if(skip > 0 && skip >= *last_lnk)
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index specified")
    } /* end if */
    else {
        H5G_bt_it_bt_t udata;                   /* User data to pass to B-tree callback */

        /* Build udata to pass through H5B_iterate() to H5G_node_build_table() */
        udata.alloc_nlinks = 0;
        udata.heap = heap;
        udata.ltable = &ltable;

        /* Iterate over the group members */
        if(H5B_iterate(oloc->file, dxpl_id, H5B_SNODE, stab.btree_addr, H5G_node_build_table, &udata) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "unable to build link table")

        /* Check for skipping out of bounds */
        if(skip > 0 && (size_t)skip >= ltable.nlinks)
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "index out of bound")

        /* Sort link table in correct iteration order */
        if(H5G_link_sort_table(&ltable, H5_INDEX_NAME, order) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTSORT, FAIL, "error sorting link messages")

        /* Iterate over links in table */
        if((ret_value = H5G_link_iterate_table(&ltable, skip, last_lnk, op, op_data)) < 0)
            HERROR(H5E_SYM, H5E_CANTNEXT, "iteration operator failed");
    } /* end else */

done:
    /* Release resources */
    if(heap && H5HL_unprotect(heap) < 0)
        HDONE_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to unprotect symbol table heap")
    if(ltable.lnks && H5G_link_release_table(&ltable) < 0)
        HDONE_ERROR(H5E_SYM, H5E_CANTFREE, FAIL, "unable to release link table")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_iterate() */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_count
 *
 * Purpose:	Count the # of links in a group
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, September  6, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_count(H5O_loc_t *oloc, hsize_t *num_objs, hid_t dxpl_id)
{
    H5O_stab_t		stab;		        /* Info about symbol table */
    herr_t		ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5G_stab_count, FAIL)

    /* Sanity check */
    HDassert(oloc);
    HDassert(num_objs);

    /* Reset the number of objects in the group */
    *num_objs = 0;

    /* Get the B-tree info */
    if(NULL == H5O_msg_read(oloc, H5O_STAB_ID, &stab, dxpl_id))
	HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "unable to determine local heap address")

    /* Iterate over the group members */
    if(H5B_iterate(oloc->file, dxpl_id, H5B_SNODE, stab.btree_addr, H5G_node_sumup, num_objs) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "iteration operator failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_count() */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_bh_size
 *
 * Purpose:	Retrieve storage for btree and heap (1.6)
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi
 *		June 25 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_bh_size(H5F_t *f, hid_t dxpl_id, const H5O_stab_t *stab,
    H5_ih_info_t *bh_info)
{
    hsize_t     snode_size;             /* Symbol table node size */
    H5B_info_t  bt_info;                /* B-tree node info */
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(H5G_stab_bh_size, FAIL)

    /* Sanity check */
    HDassert(f);
    HDassert(stab);
    HDassert(bh_info);

    /* Set up user data for B-tree iteration */
    snode_size = 0;

    /* Get the B-tree & symbol table node size info */
    if(H5B_get_info(f, dxpl_id, H5B_SNODE, stab->btree_addr, &bt_info, H5G_node_iterate_size, &snode_size) < 0)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTINIT, FAIL, "iteration operator failed")

    /* Add symbol table & B-tree node sizes to index info */
    bh_info->index_size += snode_size + bt_info.size;

    /* Get the size of the local heap for the group */
    if(H5HL_heapsize(f, dxpl_id, stab->heap_addr, &(bh_info->heap_size)) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, FAIL, "iteration operator failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_bh_size() */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_get_name_by_idx_cb
 *
 * Purpose:     Callback for B-tree iteration 'by index' info query to
 *              retrieve the name of a link
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Nov  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_stab_get_name_by_idx_cb(const H5G_entry_t *ent, void *_udata)
{
    H5G_bt_it_gnbi_t	*udata = (H5G_bt_it_gnbi_t *)_udata;
    size_t name_off;                    /* Offset of name in heap */
    const char *name;                   /* Pointer to name string in heap */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_stab_get_name_by_idx_cb)

    /* Sanity check */
    HDassert(ent);
    HDassert(udata && udata->heap);

    /* Get name offset in heap */
    name_off = ent->name_off;
    name = (const char *)H5HL_offset_into(udata->heap, name_off);
    HDassert(name);
    udata->name = H5MM_strdup(name);
    HDassert(udata->name);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5G_stab_get_name_by_idx_cb */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_get_name_by_idx
 *
 * Purpose:     Returns the name of objects in the group by giving index.
 *
 * Return:	Success:        Non-negative, length of name
 *		Failure:	Negative
 *
 * Programmer:	Raymond Lu
 *	        Nov 20, 2002
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5G_stab_get_name_by_idx(H5O_loc_t *oloc, H5_iter_order_t order, hsize_t n,
    char* name, size_t size, hid_t dxpl_id)
{
    H5HL_t *heap = NULL;        /* Pointer to local heap */
    H5O_stab_t	stab;	        /* Info about local heap & B-tree */
    H5G_bt_it_gnbi_t udata;     /* Iteration information */
    ssize_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_get_name_by_idx, FAIL)

    /* Sanity check */
    HDassert(oloc);

    /* Get the B-tree & local heap info */
    if(NULL == H5O_msg_read(oloc, H5O_STAB_ID, &stab, dxpl_id))
	HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "unable to determine local heap address")

    /* Pin the heap down in memory */
    if(NULL == (heap = H5HL_protect(oloc->file, dxpl_id, stab.heap_addr, H5AC_READ)))
        HGOTO_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to protect symbol table heap")

    /* Remap index for decreasing iteration order */
    if(order == H5_ITER_DEC) {
        hsize_t nlinks = 0;           /* Number of links in group */

        /* Iterate over the symbol table nodes, to count the links */
        if(H5B_iterate(oloc->file, dxpl_id, H5B_SNODE, stab.btree_addr, H5G_node_sumup, &nlinks) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "iteration operator failed")

        /* Map decreasing iteration order index to increasing iteration order index */
        n = nlinks - (n + 1);
    } /* end if */

    /* Set iteration information */
    udata.common.idx = n;
    udata.common.num_objs = 0;
    udata.common.op = H5G_stab_get_name_by_idx_cb;
    udata.heap = heap;
    udata.name = NULL;

    /* Iterate over the group members */
    if(H5B_iterate(oloc->file, dxpl_id, H5B_SNODE, stab.btree_addr, H5G_node_by_idx, &udata) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "iteration operator failed")

    /* If we don't know the name now, we almost certainly went out of bounds */
    if(udata.name == NULL)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "index out of bound")

    /* Get the length of the name */
    ret_value = (ssize_t)HDstrlen(udata.name);

    /* Copy the name into the user's buffer, if given */
    if(name) {
        HDstrncpy(name, udata.name, MIN((size_t)(ret_value + 1), size));
        if((size_t)ret_value >= size)
            name[size - 1]='\0';
    } /* end if */

done:
    /* Release resources */
    if(heap && H5HL_unprotect(heap) < 0)
        HDONE_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to unprotect symbol table heap")

    /* Free the duplicated name */
    if(udata.name != NULL)
        H5MM_xfree(udata.name);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_get_name_by_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_lookup_cb
 *
 * Purpose:     B-tree 'find' callback to retrieve location for an object
 *
 * Return:	Success:        Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Sep 20, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_stab_lookup_cb(const H5G_entry_t *ent, void *_udata)
{
    H5G_stab_fnd_ud_t *udata = (H5G_stab_fnd_ud_t *)_udata;   /* 'User data' passed in */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_stab_lookup_cb)

    /* Check for setting link info */
    if(udata->lnk)
        /* Convert the entry to a link */
        if(H5G_ent_to_link(udata->lnk, udata->heap, ent, udata->name) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTCONVERT, FAIL, "unable to convert symbol table entry to link")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_lookup_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_lookup
 *
 * Purpose:	Look up an object relative to a group, using symbol table
 *
 * Return:	Non-negative (TRUE/FALSE) on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Sep 20 2005
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5G_stab_lookup(H5O_loc_t *grp_oloc, const char *name, H5O_link_t *lnk,
    hid_t dxpl_id)
{
    H5HL_t *heap = NULL;                /* Pointer to local heap */
    H5G_bt_lkp_t bt_udata;              /* Data to pass through B-tree	*/
    H5G_stab_fnd_ud_t udata;            /* 'User data' to give to callback */
    H5O_stab_t stab;		        /* Symbol table message		*/
    htri_t     ret_value;       /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_lookup, FAIL)

    /* check arguments */
    HDassert(grp_oloc && grp_oloc->file);
    HDassert(name && *name);
    HDassert(lnk);

    /* Retrieve the symbol table message for the group */
    if(NULL == H5O_msg_read(grp_oloc, H5O_STAB_ID, &stab, dxpl_id))
        HGOTO_ERROR(H5E_SYM, H5E_BADMESG, FAIL, "can't read message")

    /* Pin the heap down in memory */
    if(NULL == (heap = H5HL_protect(grp_oloc->file, dxpl_id, stab.heap_addr, H5AC_READ)))
        HGOTO_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to protect symbol table heap")

    /* Set up user data to pass to 'find' operation callback */
    udata.name = name;
    udata.lnk = lnk;
    udata.heap = heap;

    /* Set up the user data for actual B-tree find operation */
    bt_udata.common.name = name;
    bt_udata.common.heap = heap;
    bt_udata.op = H5G_stab_lookup_cb;
    bt_udata.op_data = &udata;

    /* Search the B-tree */
    if((ret_value = H5B_find(grp_oloc->file, dxpl_id, H5B_SNODE, stab.btree_addr, &bt_udata)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "not found")

done:
    /* Release resources */
    if(heap && H5HL_unprotect(heap) < 0)
        HDONE_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to unprotect symbol table heap")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_lookup() */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_lookup_by_idx_cb
 *
 * Purpose:     Callback for B-tree iteration 'by index' info query to
 *              retrieve the link
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Nov  9, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_stab_lookup_by_idx_cb(const H5G_entry_t *ent, void *_udata)
{
    H5G_bt_it_lbi_t *udata = (H5G_bt_it_lbi_t *)_udata;
    const char *name;                   /* Pointer to name string in heap */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_stab_lookup_by_idx_cb)

    /* Sanity check */
    HDassert(ent);
    HDassert(udata && udata->heap);

    /* Get a pointer to the link name */
    name = (const char *)H5HL_offset_into(udata->heap, ent->name_off);
    HDassert(name);

    /* Convert the entry to a link */
    if(H5G_ent_to_link(udata->lnk, udata->heap, ent, name) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTCONVERT, FAIL, "unable to convert symbol table entry to link")
    udata->found = TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_lookup_by_idx_cb */


/*-------------------------------------------------------------------------
 * Function:	H5G_stab_lookup_by_idx
 *
 * Purpose:	Look up an object in a group, according to the name index
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Nov  7 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_lookup_by_idx(H5O_loc_t *grp_oloc, H5_iter_order_t order, hsize_t n,
    H5O_link_t *lnk, hid_t dxpl_id)
{
    H5HL_t *heap = NULL;                /* Pointer to local heap */
    H5G_bt_it_lbi_t udata;              /* Iteration information */
    H5O_stab_t stab;		        /* Symbol table message */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_lookup_by_idx, FAIL)

    /* check arguments */
    HDassert(grp_oloc && grp_oloc->file);
    HDassert(lnk);

    /* Get the B-tree & local heap info */
    if(NULL == H5O_msg_read(grp_oloc, H5O_STAB_ID, &stab, dxpl_id))
	HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "unable to determine local heap address")

    /* Pin the heap down in memory */
    if(NULL == (heap = H5HL_protect(grp_oloc->file, dxpl_id, stab.heap_addr, H5AC_READ)))
        HGOTO_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to protect symbol table heap")

    /* Remap index for decreasing iteration order */
    if(order == H5_ITER_DEC) {
        hsize_t nlinks = 0;           /* Number of links in group */

        /* Iterate over the symbol table nodes, to count the links */
        if(H5B_iterate(grp_oloc->file, dxpl_id, H5B_SNODE, stab.btree_addr, H5G_node_sumup, &nlinks) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "iteration operator failed")

        /* Map decreasing iteration order index to increasing iteration order index */
        n = nlinks - (n + 1);
    } /* end if */

    /* Set iteration information */
    udata.common.idx = n;
    udata.common.num_objs = 0;
    udata.common.op = H5G_stab_lookup_by_idx_cb;
    udata.heap = heap;
    udata.lnk = lnk;
    udata.found = FALSE;

    /* Iterate over the group members */
    if(H5B_iterate(grp_oloc->file, dxpl_id, H5B_SNODE, stab.btree_addr, H5G_node_by_idx, &udata) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "iteration operator failed")

    /* If we didn't find the link, we almost certainly went out of bounds */
    if(!udata.found)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "index out of bound")

done:
    /* Release resources */
    if(heap && H5HL_unprotect(heap) < 0)
        HDONE_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to unprotect symbol table heap")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_lookup_by_idx() */

#ifndef H5_STRICT_FORMAT_CHECKS

/*-------------------------------------------------------------------------
 * Function:	H5G_stab_valid
 *
 * Purpose:	Verify that a group's symbol table message is valid.  If
 *              provided, the addresses in alt_stab will be tried if the
 *              addresses in the group's stab message are invalid, and
 *              the stab message will be updated if necessary.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Neil Fortner
 *		nfortne2@hdfgroup.org
 *		Mar 17, 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_stab_valid(H5O_loc_t *grp_oloc, hid_t dxpl_id, H5O_stab_t *alt_stab)
{
    H5O_stab_t  stab;                   /* Current symbol table */
    H5HL_t      *heap = NULL;           /* Pointer to local heap */
    hbool_t     changed = FALSE;        /* Whether stab has been modified */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_valid, FAIL)

    /* Read the symbol table message */
    if(NULL == H5O_msg_read(grp_oloc, H5O_STAB_ID, &stab, dxpl_id))
        HGOTO_ERROR(H5E_SYM, H5E_BADMESG, FAIL, "unable to read symbol table message");

    /* Check if the symbol table message's b-tree address is valid */
    if(H5B_valid(grp_oloc->file, dxpl_id, H5B_SNODE, stab.btree_addr) < 0) {
        /* Address is invalid, try the b-tree address in the alternate symbol
         * table message */
        if(!alt_stab || H5B_valid(grp_oloc->file, dxpl_id, H5B_SNODE, alt_stab->btree_addr) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL, "unable to locate b-tree")
        else {
            /* The alternate symbol table's b-tree address is valid.  Adjust the
             * symbol table message in the group. */
            stab.btree_addr = alt_stab->btree_addr;
            changed = TRUE;
        } /* end else */
    } /* end if */

    /* Check if the symbol table message's heap address is valid */
    if(NULL == (heap = H5HL_protect(grp_oloc->file, dxpl_id, stab.heap_addr, H5AC_READ))) {
        /* Address is invalid, try the heap address in the alternate symbol
         * table message */
        if(!alt_stab || NULL == (heap = H5HL_protect(grp_oloc->file, dxpl_id, alt_stab->heap_addr, H5AC_READ)))
            HGOTO_ERROR(H5E_HEAP, H5E_NOTFOUND, FAIL, "unable to locate heap")
        else {
            /* The alternate symbol table's heap address is valid.  Adjust the
             * symbol table message in the group. */
            stab.heap_addr = alt_stab->heap_addr;
            changed = TRUE;
        } /* end else */
    } /* end if */

    /* Update the symbol table message and clear errors if necessary */
    if(changed) {
        H5E_clear_stack(NULL);
        if(H5O_msg_write(grp_oloc, H5O_STAB_ID, 0, H5O_UPDATE_TIME | H5O_UPDATE_FORCE, &stab, dxpl_id) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to correct symbol table message")
    } /* end if */

done:
    /* Release resources */
    if(heap && H5HL_unprotect(heap) < 0)
        HDONE_ERROR(H5E_SYM, H5E_PROTECT, FAIL, "unable to unprotect symbol table heap")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_valid */
#endif /* H5_STRICT_FORMAT_CHECKS */

#ifndef H5_NO_DEPRECATED_SYMBOLS

/*-------------------------------------------------------------------------
 * Function:	H5G_stab_get_type_by_idx_cb
 *
 * Purpose:     Callback for B-tree iteration 'by index' info query to
 *              retrieve the type of an object
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Nov  7, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_stab_get_type_by_idx_cb(const H5G_entry_t *ent, void *_udata)
{
    H5G_bt_it_gtbi_t	*udata = (H5G_bt_it_gtbi_t *)_udata;
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_stab_get_type_by_idx_cb)

    /* Sanity check */
    HDassert(ent);
    HDassert(udata);

    /* Check for a soft link */
    switch(ent->type) {
        case H5G_CACHED_SLINK:
            udata->type = H5G_LINK;
            break;

        case H5G_CACHED_ERROR:
        case H5G_NOTHING_CACHED:
        case H5G_CACHED_STAB:
        case H5G_NCACHED:
        default:
            {
                H5O_loc_t tmp_oloc;             /* Temporary object location */
                H5O_type_t obj_type;            /* Type of object at location */

                /* Build temporary object location */
                tmp_oloc.file = udata->f;
                HDassert(H5F_addr_defined(ent->header));
                tmp_oloc.addr = ent->header;

                /* Get the type of the object */
                if(H5O_obj_type(&tmp_oloc, &obj_type, udata->dxpl_id) < 0)
                    HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't get object type")
                udata->type = H5G_map_obj_type(obj_type);
            }
            break;
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_get_type_by_idx_cb */


/*-------------------------------------------------------------------------
 * Function:	H5G_get_objtype_by_idx
 *
 * Purpose:     Private function for H5Gget_objtype_by_idx.
 *              Returns the type of objects in the group by giving index.
 *
 * Return:	Success:        H5G_GROUP(1), H5G_DATASET(2), H5G_TYPE(3)
 *
 *		Failure:	UNKNOWN
 *
 * Programmer:	Raymond Lu
 *	        Nov 20, 2002
 *
 *-------------------------------------------------------------------------
 */
H5G_obj_t
H5G_stab_get_type_by_idx(H5O_loc_t *oloc, hsize_t idx, hid_t dxpl_id)
{
    H5O_stab_t		stab;	        /* Info about local heap & B-tree */
    H5G_bt_it_gtbi_t	udata;          /* User data for B-tree callback */
    H5G_obj_t		ret_value;      /* Return value */

    FUNC_ENTER_NOAPI(H5G_stab_get_type_by_idx, H5G_UNKNOWN)

    /* Sanity check */
    HDassert(oloc);

    /* Get the B-tree & local heap info */
    if(NULL == H5O_msg_read(oloc, H5O_STAB_ID, &stab, dxpl_id))
	HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, H5G_UNKNOWN, "unable to determine local heap address")

    /* Set iteration information */
    udata.common.idx = idx;
    udata.common.num_objs = 0;
    udata.common.op = H5G_stab_get_type_by_idx_cb;
    udata.f = oloc->file;
    udata.dxpl_id = dxpl_id;
    udata.type = H5G_UNKNOWN;

    /* Iterate over the group members */
    if(H5B_iterate(oloc->file, dxpl_id, H5B_SNODE, stab.btree_addr, H5G_node_by_idx, &udata) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5G_UNKNOWN, "iteration operator failed")

    /* If we don't know the type now, we almost certainly went out of bounds */
    if(udata.type == H5G_UNKNOWN)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5G_UNKNOWN, "index out of bound")

    /* Set the return value */
    ret_value = udata.type;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_stab_get_type_by_idx() */
#endif /* H5_NO_DEPRECATED_SYMBOLS */

