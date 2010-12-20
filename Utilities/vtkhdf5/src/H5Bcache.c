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
 * Created:		H5Bcache.c
 *			Oct 31 2005
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Implement B-tree metadata cache methods.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5B_PACKAGE		/*suppress error about including H5Bpkg  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Bpkg.h"		/* B-link trees				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5MFprivate.h"	/* File memory management		*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Local Prototypes */
/********************/

/* Metadata cache callbacks */
static H5B_t *H5B_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *udata);
static herr_t H5B_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr, H5B_t *b, unsigned UNUSED * flags_ptr);
static herr_t H5B_dest(H5F_t *f, H5B_t *bt);
static herr_t H5B_clear(H5F_t *f, H5B_t *b, hbool_t destroy);
static herr_t H5B_compute_size(const H5F_t *f, const H5B_t *bt, size_t *size_ptr);


/*********************/
/* Package Variables */
/*********************/

/* H5B inherits cache-like properties from H5AC */
const H5AC_class_t H5AC_BT[1] = {{
    H5AC_BT_ID,
    (H5AC_load_func_t)H5B_load,
    (H5AC_flush_func_t)H5B_flush,
    (H5AC_dest_func_t)H5B_dest,
    (H5AC_clear_func_t)H5B_clear,
    (H5AC_size_func_t)H5B_compute_size,
}};

/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5B_load
 *
 * Purpose:	Loads a B-tree node from the disk.
 *
 * Return:	Success:	Pointer to a new B-tree node.
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jun 23 1997
 *
 *-------------------------------------------------------------------------
 */
static H5B_t *
H5B_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *_udata)
{
    H5B_t *bt = NULL;           /* Pointer to the deserialized B-tree node */
    H5B_cache_ud_t *udata = (H5B_cache_ud_t *)_udata;       /* User data for callback */
    H5B_shared_t *shared;       /* Pointer to shared B-tree info */
    const uint8_t *p;           /* Pointer into raw data buffer */
    uint8_t *native;            /* Pointer to native keys */
    unsigned u;                 /* Local index variable */
    H5B_t *ret_value;           /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5B_load)

    /* Check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(udata);

    if(NULL == (bt = H5FL_MALLOC(H5B_t)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTALLOC, NULL, "can't allocate B-tree struct")
    HDmemset(&bt->cache_info, 0, sizeof(H5AC_info_t));

    /* Set & increment the ref-counted "shared" B-tree information for the node */
    bt->rc_shared = udata->rc_shared;
    H5RC_INC(bt->rc_shared);

    /* Get a pointer to the shared info, for convenience */
    shared = (H5B_shared_t *)H5RC_GET_OBJ(bt->rc_shared);
    HDassert(shared);

    /* Allocate space for the native keys and child addresses */
    if(NULL == (bt->native = H5FL_BLK_MALLOC(native_block, shared->sizeof_keys)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTALLOC, NULL, "can't allocate buffer for native keys")
    if(NULL == (bt->child = H5FL_SEQ_MALLOC(haddr_t, (size_t)shared->two_k)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTALLOC, NULL, "can't allocate buffer for child addresses")

    if(H5F_block_read(f, H5FD_MEM_BTREE, addr, shared->sizeof_rnode, dxpl_id, shared->page) < 0)
	HGOTO_ERROR(H5E_BTREE, H5E_READERROR, NULL, "can't read B-tree node")

    /* Set the pointer into the raw data buffer */
    p = shared->page;

    /* magic number */
    if(HDmemcmp(p, H5B_MAGIC, (size_t)H5_SIZEOF_MAGIC))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, NULL, "wrong B-tree signature")
    p += 4;

    /* node type and level */
    if(*p++ != (uint8_t)udata->type->id)
	HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, NULL, "incorrect B-tree node type")
    bt->level = *p++;

    /* entries used */
    UINT16DECODE(p, bt->nchildren);

    /* sibling pointers */
    H5F_addr_decode(udata->f, (const uint8_t **)&p, &(bt->left));
    H5F_addr_decode(udata->f, (const uint8_t **)&p, &(bt->right));

    /* the child/key pairs */
    native = bt->native;
    for(u = 0; u < bt->nchildren; u++) {
        /* Decode native key value */
        if((udata->type->decode)(shared, p, native) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDECODE, NULL, "unable to decode key")
        p += shared->sizeof_rkey;
        native += udata->type->sizeof_nkey;

        /* Decode address value */
        H5F_addr_decode(udata->f, (const uint8_t **)&p, bt->child + u);
    } /* end for */

    /* Decode final key */
    if(bt->nchildren > 0) {
        /* Decode native key value */
        if((udata->type->decode)(shared, p, native) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDECODE, NULL, "unable to decode key")
    } /* end if */

    /* Set return value */
    ret_value = bt;

done:
    if(!ret_value && bt)
        if(H5B_node_dest(bt) < 0)
            HDONE_ERROR(H5E_BTREE, H5E_CANTFREE, NULL, "unable to destroy B-tree node")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B_load() */  /*lint !e818 Can't make udata a pointer to const */


/*-------------------------------------------------------------------------
 * Function:	H5B_flush
 *
 * Purpose:	Flushes a dirty B-tree node to disk.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Jun 23 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr, H5B_t *bt, unsigned UNUSED * flags_ptr)
{
    H5B_shared_t *shared;       /* Pointer to shared B-tree info */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5B_flush)

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(bt);
    shared = (H5B_shared_t *)H5RC_GET_OBJ(bt->rc_shared);
    HDassert(shared);
    HDassert(shared->type);
    HDassert(shared->type->encode);

    if(bt->cache_info.is_dirty) {
        uint8_t    *p;              /* Pointer into raw data buffer */
        uint8_t    *native;         /* Pointer to native keys */
        unsigned    u;              /* Local index variable */

        p = shared->page;

        /* magic number */
        HDmemcpy(p, H5B_MAGIC, (size_t)H5_SIZEOF_MAGIC);
        p += 4;

        /* node type and level */
        *p++ = (uint8_t)shared->type->id;
        H5_CHECK_OVERFLOW(bt->level, unsigned, uint8_t);
        *p++ = (uint8_t)bt->level;

        /* entries used */
        UINT16ENCODE(p, bt->nchildren);

        /* sibling pointers */
        H5F_addr_encode(f, &p, bt->left);
        H5F_addr_encode(f, &p, bt->right);

        /* child keys and pointers */
        native = bt->native;
        for(u = 0; u < bt->nchildren; ++u) {
            /* encode the key */
            if(shared->type->encode(shared, p, native) < 0)
                HGOTO_ERROR(H5E_BTREE, H5E_CANTENCODE, FAIL, "unable to encode B-tree key")
            p += shared->sizeof_rkey;
            native += shared->type->sizeof_nkey;

            /* encode the child address */
            H5F_addr_encode(f, &p, bt->child[u]);
        } /* end for */
        if(bt->nchildren > 0) {
            /* Encode the final key */
            if(shared->type->encode(shared, p, native) < 0)
                HGOTO_ERROR(H5E_BTREE, H5E_CANTENCODE, FAIL, "unable to encode B-tree key")
        } /* end if */

	/*
         * Write the disk page.	We always write the header, but we don't
         * bother writing data for the child entries that don't exist or
         * for the final unchanged children.
	 */
	if(H5F_block_write(f, H5FD_MEM_BTREE, addr, shared->sizeof_rnode, dxpl_id, shared->page) < 0)
	    HGOTO_ERROR(H5E_BTREE, H5E_CANTFLUSH, FAIL, "unable to save B-tree node to disk")

	bt->cache_info.is_dirty = FALSE;
    } /* end if */

    if(destroy)
        if(H5B_dest(f, bt) < 0)
	    HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to destroy B-tree node")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5B_dest
 *
 * Purpose:	Destroys a B-tree node in memory.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Jan 15 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B_dest(H5F_t *f, H5B_t *bt)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5B_dest)

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(bt);
    HDassert(bt->rc_shared);

    /* If we're going to free the space on disk, the address must be valid */
    HDassert(!bt->cache_info.free_file_space_on_destroy || H5F_addr_defined(bt->cache_info.addr));

    /* Check for freeing file space for B-tree node */
    if(bt->cache_info.free_file_space_on_destroy) {
        H5B_shared_t *shared;               /* Pointer to shared B-tree info */

        /* Get the pointer to the shared B-tree info */
        shared = (H5B_shared_t *)H5RC_GET_OBJ(bt->rc_shared);
        HDassert(shared);

        /* Release the space on disk */
        /* (XXX: Nasty usage of internal DXPL value! -QAK) */
        if(H5MF_xfree(f, H5FD_MEM_BTREE, H5AC_dxpl_id, bt->cache_info.addr, (hsize_t)shared->sizeof_rnode) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to free B-tree node")
    } /* end if */

    /* Destroy B-tree node */
    if(H5B_node_dest(bt) < 0)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to destroy B-tree node")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B_dest() */


/*-------------------------------------------------------------------------
 * Function:	H5B_clear
 *
 * Purpose:	Mark a B-tree node in memory as non-dirty.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar 20 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B_clear(H5F_t *f, H5B_t *bt, hbool_t destroy)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5B_clear)

    /*
     * Check arguments.
     */
    HDassert(bt);

    /* Reset the dirty flag.  */
    bt->cache_info.is_dirty = FALSE;

    if(destroy)
        if(H5B_dest(f, bt) < 0)
	    HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to destroy B-tree node")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B_clear() */


/*-------------------------------------------------------------------------
 * Function:	H5B_compute_size
 *
 * Purpose:	Compute the size in bytes of the specified instance of
 *		H5B_t on disk, and return it in *len_ptr.  On failure,
 *		the value of *len_ptr is undefined.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	John Mainzer
 *		5/13/04
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B_compute_size(const H5F_t UNUSED *f, const H5B_t *bt, size_t *size_ptr)
{
    H5B_shared_t        *shared;        /* Pointer to shared B-tree info */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5B_compute_size)

    /* check arguments */
    HDassert(f);
    HDassert(bt);
    HDassert(bt->rc_shared);
    shared = (H5B_shared_t *)H5RC_GET_OBJ(bt->rc_shared);
    HDassert(shared);
    HDassert(shared->type);
    HDassert(size_ptr);

    /* Set size value */
    *size_ptr = shared->sizeof_rnode;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5B_compute_size() */

