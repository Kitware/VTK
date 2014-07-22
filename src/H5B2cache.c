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
 * Created:		H5B2cache.c
 *			Jan 31 2005
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Implement v2 B-tree metadata cache methods.
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
#include "H5WBprivate.h"        /* Wrapped Buffers                      */


/****************/
/* Local Macros */
/****************/

/* B-tree format version #'s */
#define H5B2_HDR_VERSION 0              /* Header */
#define H5B2_INT_VERSION 0              /* Internal node */
#define H5B2_LEAF_VERSION 0             /* Leaf node */

/* Size of stack buffer for serialized headers */
#define H5B2_HDR_BUF_SIZE               128


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

/* Metadata cache callbacks */
static H5B2_hdr_t *H5B2__cache_hdr_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *udata);
static herr_t H5B2__cache_hdr_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr, H5B2_hdr_t *hdr, unsigned UNUSED * flags_ptr);
static herr_t H5B2__cache_hdr_dest(H5F_t *f, H5B2_hdr_t *hdr);
static herr_t H5B2__cache_hdr_clear(H5F_t *f, H5B2_hdr_t *hdr, hbool_t destroy);
static herr_t H5B2__cache_hdr_size(const H5F_t *f, const H5B2_hdr_t *hdr, size_t *size_ptr);
static H5B2_internal_t *H5B2__cache_internal_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *udata);
static herr_t H5B2__cache_internal_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr, H5B2_internal_t *i, unsigned UNUSED * flags_ptr);
static herr_t H5B2__cache_internal_dest(H5F_t *f, H5B2_internal_t *internal);
static herr_t H5B2__cache_internal_clear(H5F_t *f, H5B2_internal_t *i, hbool_t destroy);
static herr_t H5B2__cache_internal_size(const H5F_t *f, const H5B2_internal_t *i, size_t *size_ptr);
static H5B2_leaf_t *H5B2__cache_leaf_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *udata);
static herr_t H5B2__cache_leaf_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr, H5B2_leaf_t *l, unsigned UNUSED * flags_ptr);
static herr_t H5B2__cache_leaf_dest(H5F_t *f, H5B2_leaf_t *leaf);
static herr_t H5B2__cache_leaf_clear(H5F_t *f, H5B2_leaf_t *l, hbool_t destroy);
static herr_t H5B2__cache_leaf_size(const H5F_t *f, const H5B2_leaf_t *l, size_t *size_ptr);


/*********************/
/* Package Variables */
/*********************/

/* H5B2 inherits cache-like properties from H5AC */
const H5AC_class_t H5AC_BT2_HDR[1] = {{
    H5AC_BT2_HDR_ID,
    (H5AC_load_func_t)H5B2__cache_hdr_load,
    (H5AC_flush_func_t)H5B2__cache_hdr_flush,
    (H5AC_dest_func_t)H5B2__cache_hdr_dest,
    (H5AC_clear_func_t)H5B2__cache_hdr_clear,
    (H5AC_size_func_t)H5B2__cache_hdr_size,
}};

/* H5B2 inherits cache-like properties from H5AC */
const H5AC_class_t H5AC_BT2_INT[1] = {{
    H5AC_BT2_INT_ID,
    (H5AC_load_func_t)H5B2__cache_internal_load,
    (H5AC_flush_func_t)H5B2__cache_internal_flush,
    (H5AC_dest_func_t)H5B2__cache_internal_dest,
    (H5AC_clear_func_t)H5B2__cache_internal_clear,
    (H5AC_size_func_t)H5B2__cache_internal_size,
}};

/* H5B2 inherits cache-like properties from H5AC */
const H5AC_class_t H5AC_BT2_LEAF[1] = {{
    H5AC_BT2_LEAF_ID,
    (H5AC_load_func_t)H5B2__cache_leaf_load,
    (H5AC_flush_func_t)H5B2__cache_leaf_flush,
    (H5AC_dest_func_t)H5B2__cache_leaf_dest,
    (H5AC_clear_func_t)H5B2__cache_leaf_clear,
    (H5AC_size_func_t)H5B2__cache_leaf_size,
}};


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5B2__cache_hdr_load
 *
 * Purpose:	Loads a B-tree header from the disk.
 *
 * Return:	Success:	Pointer to a new B-tree.
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 1 2005
 *
 *-------------------------------------------------------------------------
 */
static H5B2_hdr_t *
H5B2__cache_hdr_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *_udata)
{
    H5B2_hdr_t		*hdr = NULL;    /* B-tree header */
    H5B2_hdr_cache_ud_t *udata = (H5B2_hdr_cache_ud_t *)_udata;
    H5B2_create_t       cparam;         /* B-tree creation parameters */
    H5B2_subid_t        id;		/* ID of B-tree class, as found in file */
    uint16_t            depth;          /* Depth of B-tree */
    uint32_t            stored_chksum;  /* Stored metadata checksum value */
    uint32_t            computed_chksum; /* Computed metadata checksum value */
    H5WB_t              *wb = NULL;     /* Wrapped buffer for header data */
    uint8_t             hdr_buf[H5B2_HDR_BUF_SIZE]; /* Buffer for header */
    uint8_t		*buf;           /* Pointer to header buffer */
    const uint8_t	*p;             /* Pointer into raw data buffer */
    H5B2_hdr_t		*ret_value;     /* Return value */

    FUNC_ENTER_STATIC

    /* Check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(udata);

    /* Allocate new B-tree header and reset cache info */
    if(NULL == (hdr = H5B2_hdr_alloc(udata->f)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTALLOC, NULL, "allocation failed for B-tree header")

    /* Wrap the local buffer for serialized header info */
    if(NULL == (wb = H5WB_wrap(hdr_buf, sizeof(hdr_buf))))
        HGOTO_ERROR(H5E_BTREE, H5E_CANTINIT, NULL, "can't wrap buffer")

    /* Get a pointer to a buffer that's large enough for header */
    if(NULL == (buf = (uint8_t *)H5WB_actual(wb, hdr->hdr_size)))
        HGOTO_ERROR(H5E_BTREE, H5E_NOSPACE, NULL, "can't get actual buffer")

    /* Read header from disk */
    if(H5F_block_read(f, H5FD_MEM_BTREE, addr, hdr->hdr_size, dxpl_id, buf) < 0)
	HGOTO_ERROR(H5E_BTREE, H5E_READERROR, NULL, "can't read B-tree header")

    /* Get temporary pointer to serialized header */
    p = buf;

    /* Magic number */
    if(HDmemcmp(p, H5B2_HDR_MAGIC, (size_t)H5_SIZEOF_MAGIC))
	HGOTO_ERROR(H5E_BTREE, H5E_BADVALUE, NULL, "wrong B-tree header signature")
    p += H5_SIZEOF_MAGIC;

    /* Version */
    if(*p++ != H5B2_HDR_VERSION)
	HGOTO_ERROR(H5E_BTREE, H5E_BADRANGE, NULL, "wrong B-tree header version")

    /* B-tree class */
    id = (H5B2_subid_t)*p++;
    if(id >= H5B2_NUM_BTREE_ID)
	HGOTO_ERROR(H5E_BTREE, H5E_BADTYPE, NULL, "incorrect B-tree type")

    /* Node size (in bytes) */
    UINT32DECODE(p, cparam.node_size);

    /* Raw key size (in bytes) */
    UINT16DECODE(p, cparam.rrec_size);

    /* Depth of tree */
    UINT16DECODE(p, depth);

    /* Split & merge %s */
    cparam.split_percent = *p++;
    cparam.merge_percent = *p++;

    /* Root node pointer */
    H5F_addr_decode(udata->f, (const uint8_t **)&p, &(hdr->root.addr));
    UINT16DECODE(p, hdr->root.node_nrec);
    H5F_DECODE_LENGTH(udata->f, p, hdr->root.all_nrec);

    /* Metadata checksum */
    UINT32DECODE(p, stored_chksum);

    /* Sanity check */
    HDassert((size_t)(p - (const uint8_t *)buf) == hdr->hdr_size);

    /* Compute checksum on entire header */
    computed_chksum = H5_checksum_metadata(buf, (hdr->hdr_size - H5B2_SIZEOF_CHKSUM), 0);

    /* Verify checksum */
    if(stored_chksum != computed_chksum)
	HGOTO_ERROR(H5E_BTREE, H5E_BADVALUE, NULL, "incorrect metadata checksum for v2 B-tree header")

    /* Initialize B-tree header info */
    cparam.cls = H5B2_client_class_g[id];
    if(H5B2_hdr_init(hdr, &cparam, udata->ctx_udata, depth) < 0)
	HGOTO_ERROR(H5E_BTREE, H5E_CANTINIT, NULL, "can't initialize B-tree header info")

    /* Set the B-tree header's address */
    hdr->addr = addr;

    /* Set return value */
    ret_value = hdr;

done:
    /* Release resources */
    if(wb && H5WB_unwrap(wb) < 0)
        HDONE_ERROR(H5E_BTREE, H5E_CLOSEERROR, NULL, "can't close wrapped buffer")
    if(!ret_value && hdr)
        if(H5B2_hdr_free(hdr) < 0)
            HDONE_ERROR(H5E_BTREE, H5E_CANTRELEASE, NULL, "can't release v2 B-tree header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2__cache_hdr_load() */ /*lint !e818 Can't make udata a pointer to const */


/*-------------------------------------------------------------------------
 * Function:	H5B2__cache_hdr_flush
 *
 * Purpose:	Flushes a dirty B-tree header to disk.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 1 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B2__cache_hdr_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr,
    H5B2_hdr_t *hdr, unsigned UNUSED * flags_ptr)
{
    H5WB_t      *wb = NULL;             /* Wrapped buffer for header data */
    uint8_t     hdr_buf[H5B2_HDR_BUF_SIZE]; /* Buffer for header */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(hdr);

    if(hdr->cache_info.is_dirty) {
        uint8_t	*buf;           /* Pointer to header buffer */
        uint8_t *p;             /* Pointer into raw data buffer */
        uint32_t metadata_chksum; /* Computed metadata checksum value */

        /* Set the B-tree header's file context for this operation */
        hdr->f = f;

        /* Wrap the local buffer for serialized header info */
        if(NULL == (wb = H5WB_wrap(hdr_buf, sizeof(hdr_buf))))
            HGOTO_ERROR(H5E_BTREE, H5E_CANTINIT, FAIL, "can't wrap buffer")

        /* Get a pointer to a buffer that's large enough for header */
        if(NULL == (buf = (uint8_t *)H5WB_actual(wb, hdr->hdr_size)))
            HGOTO_ERROR(H5E_BTREE, H5E_NOSPACE, FAIL, "can't get actual buffer")

        /* Get temporary pointer to serialized header */
        p = buf;

        /* Magic number */
        HDmemcpy(p, H5B2_HDR_MAGIC, (size_t)H5_SIZEOF_MAGIC);
        p += H5_SIZEOF_MAGIC;

        /* Version # */
        *p++ = H5B2_HDR_VERSION;

        /* B-tree type */
        *p++ = hdr->cls->id;

        /* Node size (in bytes) */
        UINT32ENCODE(p, hdr->node_size);

        /* Raw key size (in bytes) */
        UINT16ENCODE(p, hdr->rrec_size);

        /* Depth of tree */
        UINT16ENCODE(p, hdr->depth);

        /* Split & merge %s */
        H5_CHECK_OVERFLOW(hdr->split_percent, /* From: */ unsigned, /* To: */ uint8_t);
        *p++ = (uint8_t)hdr->split_percent;
        H5_CHECK_OVERFLOW(hdr->merge_percent, /* From: */ unsigned, /* To: */ uint8_t);
        *p++ = (uint8_t)hdr->merge_percent;

        /* Root node pointer */
        H5F_addr_encode(f, &p, hdr->root.addr);
        UINT16ENCODE(p, hdr->root.node_nrec);
        H5F_ENCODE_LENGTH(f, p, hdr->root.all_nrec);

        /* Compute metadata checksum */
        metadata_chksum = H5_checksum_metadata(buf, (hdr->hdr_size - H5B2_SIZEOF_CHKSUM), 0);

        /* Metadata checksum */
        UINT32ENCODE(p, metadata_chksum);

	/* Write the B-tree header. */
        HDassert((size_t)(p - buf) == hdr->hdr_size);
	if(H5F_block_write(f, H5FD_MEM_BTREE, addr, hdr->hdr_size, dxpl_id, buf) < 0)
	    HGOTO_ERROR(H5E_BTREE, H5E_CANTFLUSH, FAIL, "unable to save B-tree header to disk")

	hdr->cache_info.is_dirty = FALSE;
    } /* end if */

    if(destroy)
        if(H5B2__cache_hdr_dest(f, hdr) < 0)
	    HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to destroy B-tree header")

done:
    /* Release resources */
    if(wb && H5WB_unwrap(wb) < 0)
        HDONE_ERROR(H5E_BTREE, H5E_CLOSEERROR, FAIL, "can't close wrapped buffer")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2__cache_hdr_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5B2__cache_hdr_dest
 *
 * Purpose:	Destroys a B-tree header in memory.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 1 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B2__cache_hdr_dest(H5F_t *f, H5B2_hdr_t *hdr)
{
    herr_t ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_STATIC

    /* Check arguments */
    HDassert(hdr);
    HDassert(hdr->rc == 0);

    /* If we're going to free the space on disk, the address must be valid */
    HDassert(!hdr->cache_info.free_file_space_on_destroy || H5F_addr_defined(hdr->cache_info.addr));

    /* Check for freeing file space for B-tree header */
    if(hdr->cache_info.free_file_space_on_destroy) {
        /* Release the space on disk */
        /* (XXX: Nasty usage of internal DXPL value! -QAK) */
        if(H5MF_xfree(f, H5FD_MEM_BTREE, H5AC_dxpl_id, hdr->cache_info.addr, (hsize_t)hdr->hdr_size) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to free v2 B-tree header")
    } /* end if */

    /* Release B-tree header info */
    if(H5B2_hdr_free(hdr) < 0)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to free v2 B-tree header info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2__cache_hdr_dest() */


/*-------------------------------------------------------------------------
 * Function:	H5B2__cache_hdr_clear
 *
 * Purpose:	Mark a B-tree header in memory as non-dirty.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb  1 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B2__cache_hdr_clear(H5F_t *f, H5B2_hdr_t *hdr, hbool_t destroy)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    /*
     * Check arguments.
     */
    HDassert(hdr);

    /* Reset the dirty flag.  */
    hdr->cache_info.is_dirty = FALSE;

    if(destroy)
        if(H5B2__cache_hdr_dest(f, hdr) < 0)
	    HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to destroy B-tree header")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2__cache_hdr_clear() */


/*-------------------------------------------------------------------------
 * Function:	H5B2__cache_hdr_size
 *
 * Purpose:	Compute the size in bytes of a B-tree header
 *		on disk, and return it in *size_ptr.  On failure,
 *		the value of *size_ptr is undefined.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 1 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B2__cache_hdr_size(const H5F_t UNUSED *f, const H5B2_hdr_t *hdr, size_t *size_ptr)
{
    FUNC_ENTER_STATIC_NOERR

    /* check arguments */
    HDassert(f);
    HDassert(size_ptr);

    /* Set size value */
    *size_ptr = hdr->hdr_size;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5B2__cache_hdr_size() */


/*-------------------------------------------------------------------------
 * Function:	H5B2__cache_internal_load
 *
 * Purpose:	Loads a B-tree internal node from the disk.
 *
 * Return:	Success:	Pointer to a new B-tree internal node.
 *              Failure:        NULL
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 2 2005
 *
 *-------------------------------------------------------------------------
 */
static H5B2_internal_t *
H5B2__cache_internal_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *_udata)
{
    H5B2_internal_cache_ud_t *udata = (H5B2_internal_cache_ud_t *)_udata;     /* Pointer to user data */
    H5B2_internal_t	*internal = NULL;       /* Internal node read */
    const uint8_t	*p;             /* Pointer into raw data buffer */
    uint8_t		*native;        /* Pointer to native record info */
    H5B2_node_ptr_t	*int_node_ptr;  /* Pointer to node pointer info */
    uint32_t            stored_chksum;  /* Stored metadata checksum value */
    uint32_t            computed_chksum; /* Computed metadata checksum value */
    unsigned		u;              /* Local index variable */
    H5B2_internal_t	*ret_value;     /* Return value */

    FUNC_ENTER_STATIC

    /* Check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(udata);

    /* Allocate new internal node and reset cache info */
    if(NULL == (internal = H5FL_MALLOC(H5B2_internal_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
    HDmemset(&internal->cache_info, 0, sizeof(H5AC_info_t));

    /* Set the B-tree header's file context for this operation */
    udata->hdr->f = f;

    /* Increment ref. count on B-tree header */
    if(H5B2_hdr_incr(udata->hdr) < 0)
	HGOTO_ERROR(H5E_BTREE, H5E_CANTINC, NULL, "can't increment ref. count on B-tree header")

    /* Share B-tree information */
    internal->hdr = udata->hdr;

    /* Read header from disk */
    if(H5F_block_read(f, H5FD_MEM_BTREE, addr, udata->hdr->node_size, dxpl_id, udata->hdr->page) < 0)
	HGOTO_ERROR(H5E_BTREE, H5E_READERROR, NULL, "can't read B-tree internal node")

    p = udata->hdr->page;

    /* Magic number */
    if(HDmemcmp(p, H5B2_INT_MAGIC, (size_t)H5_SIZEOF_MAGIC))
        HGOTO_ERROR(H5E_BTREE, H5E_BADVALUE, NULL, "wrong B-tree internal node signature")
    p += H5_SIZEOF_MAGIC;

    /* Version */
    if(*p++ != H5B2_INT_VERSION)
	HGOTO_ERROR(H5E_BTREE, H5E_BADRANGE, NULL, "wrong B-tree internal node version")

    /* B-tree type */
    if(*p++ != (uint8_t)udata->hdr->cls->id)
	HGOTO_ERROR(H5E_BTREE, H5E_BADTYPE, NULL, "incorrect B-tree type")

    /* Allocate space for the native keys in memory */
    if(NULL == (internal->int_native = (uint8_t *)H5FL_FAC_MALLOC(udata->hdr->node_info[udata->depth].nat_rec_fac)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for B-tree internal native keys")

    /* Allocate space for the node pointers in memory */
    if(NULL == (internal->node_ptrs = (H5B2_node_ptr_t *)H5FL_FAC_MALLOC(udata->hdr->node_info[udata->depth].node_ptr_fac)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for B-tree internal node pointers")

    /* Set the number of records in the leaf & it's depth */
    internal->nrec = udata->nrec;
    internal->depth = udata->depth;

    /* Deserialize records for internal node */
    native = internal->int_native;
    for(u = 0; u < internal->nrec; u++) {
        /* Decode record */
        if((udata->hdr->cls->decode)(p, native, udata->hdr->cb_ctx) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDECODE, NULL, "unable to decode B-tree record")

        /* Move to next record */
        p += udata->hdr->rrec_size;
        native += udata->hdr->cls->nrec_size;
    } /* end for */

    /* Deserialize node pointers for internal node */
    int_node_ptr = internal->node_ptrs;
    for(u = 0; u < (unsigned)(internal->nrec + 1); u++) {
        /* Decode node pointer */
        H5F_addr_decode(udata->f, (const uint8_t **)&p, &(int_node_ptr->addr));
        UINT64DECODE_VAR(p, int_node_ptr->node_nrec, udata->hdr->max_nrec_size);
        if(udata->depth > 1)
            UINT64DECODE_VAR(p, int_node_ptr->all_nrec, udata->hdr->node_info[udata->depth - 1].cum_max_nrec_size)
        else
            int_node_ptr->all_nrec = int_node_ptr->node_nrec;

        /* Move to next node pointer */
        int_node_ptr++;
    } /* end for */

    /* Compute checksum on internal node */
    computed_chksum = H5_checksum_metadata(udata->hdr->page, (size_t)(p - (const uint8_t *)udata->hdr->page), 0);

    /* Metadata checksum */
    UINT32DECODE(p, stored_chksum);

    /* Sanity check parsing */
    HDassert((size_t)(p - (const uint8_t *)udata->hdr->page) <= udata->hdr->node_size);

    /* Verify checksum */
    if(stored_chksum != computed_chksum)
	HGOTO_ERROR(H5E_BTREE, H5E_BADVALUE, NULL, "incorrect metadata checksum for v2 internal node")

    /* Set return value */
    ret_value = internal;

done:
    if(!ret_value && internal)
        if(H5B2_internal_free(internal) < 0)
            HDONE_ERROR(H5E_BTREE, H5E_CANTFREE, NULL, "unable to destroy B-tree internal node")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2__cache_internal_load() */ /*lint !e818 Can't make udata a pointer to const */


/*-------------------------------------------------------------------------
 * Function:	H5B2__cache_internal_flush
 *
 * Purpose:	Flushes a dirty B-tree internal node to disk.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 3 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B2__cache_internal_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr, H5B2_internal_t *internal, unsigned UNUSED * flags_ptr)
{
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(internal);
    HDassert(internal->hdr);

    if(internal->cache_info.is_dirty) {
        uint8_t *p;             /* Pointer into raw data buffer */
        uint8_t *native;        /* Pointer to native record info */
        H5B2_node_ptr_t *int_node_ptr;      /* Pointer to node pointer info */
        uint32_t metadata_chksum; /* Computed metadata checksum value */
        unsigned u;             /* Local index variable */

        /* Set the B-tree header's file context for this operation */
        internal->hdr->f = f;

        p = internal->hdr->page;

        /* Magic number */
        HDmemcpy(p, H5B2_INT_MAGIC, (size_t)H5_SIZEOF_MAGIC);
        p += H5_SIZEOF_MAGIC;

        /* Version # */
        *p++ = H5B2_INT_VERSION;

        /* B-tree type */
        *p++ = internal->hdr->cls->id;
        HDassert((size_t)(p - internal->hdr->page) == (H5B2_INT_PREFIX_SIZE - H5B2_SIZEOF_CHKSUM));

        /* Serialize records for internal node */
        native = internal->int_native;
        for(u = 0; u < internal->nrec; u++) {
            /* Encode record */
            if((internal->hdr->cls->encode)(p, native, internal->hdr->cb_ctx) < 0)
                HGOTO_ERROR(H5E_BTREE, H5E_CANTENCODE, FAIL, "unable to encode B-tree record")

            /* Move to next record */
            p += internal->hdr->rrec_size;
            native += internal->hdr->cls->nrec_size;
        } /* end for */

        /* Serialize node pointers for internal node */
        int_node_ptr = internal->node_ptrs;
        for(u = 0; u < (unsigned)(internal->nrec + 1); u++) {
            /* Encode node pointer */
            H5F_addr_encode(f, &p, int_node_ptr->addr);
            UINT64ENCODE_VAR(p, int_node_ptr->node_nrec, internal->hdr->max_nrec_size);
            if(internal->depth > 1)
                UINT64ENCODE_VAR(p, int_node_ptr->all_nrec, internal->hdr->node_info[internal->depth - 1].cum_max_nrec_size);

            /* Move to next node pointer */
            int_node_ptr++;
        } /* end for */

        /* Compute metadata checksum */
        metadata_chksum = H5_checksum_metadata(internal->hdr->page, (size_t)(p - internal->hdr->page), 0);

        /* Metadata checksum */
        UINT32ENCODE(p, metadata_chksum);

	/* Write the B-tree internal node */
        HDassert((size_t)(p - internal->hdr->page) <= internal->hdr->node_size);
	if(H5F_block_write(f, H5FD_MEM_BTREE, addr, internal->hdr->node_size, dxpl_id, internal->hdr->page) < 0)
	    HGOTO_ERROR(H5E_BTREE, H5E_CANTFLUSH, FAIL, "unable to save B-tree internal node to disk")

	internal->cache_info.is_dirty = FALSE;
    } /* end if */

    if(destroy)
        if(H5B2__cache_internal_dest(f, internal) < 0)
	    HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to destroy B-tree internal node")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2__cache_internal_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5B2__cache_internal_dest
 *
 * Purpose:	Destroys a B-tree internal node in memory.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 2 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B2__cache_internal_dest(H5F_t *f, H5B2_internal_t *internal)
{
    herr_t ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_STATIC

    /* Check arguments */
    HDassert(f);
    HDassert(internal);
    HDassert(internal->hdr);

    /* If we're going to free the space on disk, the address must be valid */
    HDassert(!internal->cache_info.free_file_space_on_destroy || H5F_addr_defined(internal->cache_info.addr));

    /* Check for freeing file space for B-tree internal node */
    if(internal->cache_info.free_file_space_on_destroy) {
        /* Release the space on disk */
        /* (XXX: Nasty usage of internal DXPL value! -QAK) */
        if(H5MF_xfree(f, H5FD_MEM_BTREE, H5AC_dxpl_id, internal->cache_info.addr, (hsize_t)internal->hdr->node_size) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to free v2 B-tree internal node")
    } /* end if */

    /* Release v2 b-tree internal node */
    if(H5B2_internal_free(internal) < 0)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to release v2 B-tree internal node")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2__cache_internal_dest() */


/*-------------------------------------------------------------------------
 * Function:	H5B2__cache_internal_clear
 *
 * Purpose:	Mark a B-tree internal node in memory as non-dirty.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb  2 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B2__cache_internal_clear(H5F_t *f, H5B2_internal_t *internal, hbool_t destroy)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /*
     * Check arguments.
     */
    HDassert(internal);

    /* Reset the dirty flag.  */
    internal->cache_info.is_dirty = FALSE;

    if(destroy)
        if(H5B2__cache_internal_dest(f, internal) < 0)
	    HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to destroy B-tree internal node")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2__cache_internal_clear() */


/*-------------------------------------------------------------------------
 * Function:	H5B2__cache_internal_size
 *
 * Purpose:	Compute the size in bytes of a B-tree internal node
 *		on disk, and return it in *size_ptr.  On failure,
 *		the value of *size_ptr is undefined.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 2 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B2__cache_internal_size(const H5F_t UNUSED *f, const H5B2_internal_t *internal, size_t *size_ptr)
{
    FUNC_ENTER_STATIC_NOERR

    /* check arguments */
    HDassert(internal);
    HDassert(internal->hdr);
    HDassert(size_ptr);

    /* Set size value */
    *size_ptr = internal->hdr->node_size;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5B2__cache_internal_size() */


/*-------------------------------------------------------------------------
 * Function:	H5B2__cache_leaf_load
 *
 * Purpose:	Loads a B-tree leaf from the disk.
 *
 * Return:	Success:	Pointer to a new B-tree leaf node.
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 2 2005
 *
 *-------------------------------------------------------------------------
 */
static H5B2_leaf_t *
H5B2__cache_leaf_load(H5F_t UNUSED *f, hid_t dxpl_id, haddr_t addr, void *_udata)
{
    H5B2_leaf_cache_ud_t *udata = (H5B2_leaf_cache_ud_t *)_udata;
    H5B2_leaf_t		*leaf = NULL;   /* Pointer to lead node loaded */
    const uint8_t	*p;             /* Pointer into raw data buffer */
    uint8_t		*native;        /* Pointer to native keys */
    uint32_t            stored_chksum;  /* Stored metadata checksum value */
    uint32_t            computed_chksum; /* Computed metadata checksum value */
    unsigned		u;              /* Local index variable */
    H5B2_leaf_t		*ret_value;     /* Return value */

    FUNC_ENTER_STATIC

    /* Check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(udata);

    /* Allocate new leaf node and reset cache info */
    if(NULL == (leaf = H5FL_MALLOC(H5B2_leaf_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
    HDmemset(&leaf->cache_info, 0, sizeof(H5AC_info_t));

    /* Set the B-tree header's file context for this operation */
    udata->hdr->f = udata->f;

    /* Increment ref. count on B-tree header */
    if(H5B2_hdr_incr(udata->hdr) < 0)
	HGOTO_ERROR(H5E_BTREE, H5E_CANTINC, NULL, "can't increment ref. count on B-tree header")

    /* Share B-tree header information */
    leaf->hdr = udata->hdr;

    /* Read header from disk */
    if(H5F_block_read(udata->f, H5FD_MEM_BTREE, addr, udata->hdr->node_size, dxpl_id, udata->hdr->page) < 0)
	HGOTO_ERROR(H5E_BTREE, H5E_READERROR, NULL, "can't read B-tree leaf node")

    p = udata->hdr->page;

    /* Magic number */
    if(HDmemcmp(p, H5B2_LEAF_MAGIC, (size_t)H5_SIZEOF_MAGIC))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, NULL, "wrong B-tree leaf node signature")
    p += H5_SIZEOF_MAGIC;

    /* Version */
    if(*p++ != H5B2_LEAF_VERSION)
	HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, NULL, "wrong B-tree leaf node version")

    /* B-tree type */
    if(*p++ != (uint8_t)udata->hdr->cls->id)
	HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, NULL, "incorrect B-tree type")

    /* Allocate space for the native keys in memory */
    if(NULL == (leaf->leaf_native = (uint8_t *)H5FL_FAC_MALLOC(udata->hdr->node_info[0].nat_rec_fac)))
	HGOTO_ERROR(H5E_BTREE, H5E_NOSPACE, NULL, "memory allocation failed for B-tree leaf native keys")

    /* Set the number of records in the leaf */
    leaf->nrec = udata->nrec;

    /* Deserialize records for leaf node */
    native = leaf->leaf_native;
    for(u = 0; u < leaf->nrec; u++) {
        /* Decode record */
        if((udata->hdr->cls->decode)(p, native, udata->hdr->cb_ctx) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTENCODE, NULL, "unable to decode B-tree record")

        /* Move to next record */
        p += udata->hdr->rrec_size;
        native += udata->hdr->cls->nrec_size;
    } /* end for */

    /* Compute checksum on internal node */
    computed_chksum = H5_checksum_metadata(udata->hdr->page, (size_t)(p - (const uint8_t *)udata->hdr->page), 0);

    /* Metadata checksum */
    UINT32DECODE(p, stored_chksum);

    /* Sanity check parsing */
    HDassert((size_t)(p - (const uint8_t *)udata->hdr->page) <= udata->hdr->node_size);

    /* Verify checksum */
    if(stored_chksum != computed_chksum)
	HGOTO_ERROR(H5E_BTREE, H5E_BADVALUE, NULL, "incorrect metadata checksum for v2 leaf node")

    /* Set return value */
    ret_value = leaf;

done:
    if(!ret_value && leaf)
        if(H5B2_leaf_free(leaf) < 0)
            HDONE_ERROR(H5E_BTREE, H5E_CANTFREE, NULL, "unable to destroy B-tree leaf node")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2__cache_leaf_load() */ /*lint !e818 Can't make udata a pointer to const */


/*-------------------------------------------------------------------------
 * Function:	H5B2__cache_leaf_flush
 *
 * Purpose:	Flushes a dirty B-tree leaf node to disk.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 2 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B2__cache_leaf_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr, H5B2_leaf_t *leaf, unsigned UNUSED * flags_ptr)
{
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(leaf);
    HDassert(leaf->hdr);

    if(leaf->cache_info.is_dirty) {
        uint8_t *p;             /* Pointer into raw data buffer */
        uint8_t *native;        /* Pointer to native keys */
        uint32_t metadata_chksum; /* Computed metadata checksum value */
        unsigned u;             /* Local index variable */

        /* Set the B-tree header's file context for this operation */
        leaf->hdr->f = f;

        p = leaf->hdr->page;

        /* magic number */
        HDmemcpy(p, H5B2_LEAF_MAGIC, (size_t)H5_SIZEOF_MAGIC);
        p += H5_SIZEOF_MAGIC;

        /* version # */
        *p++ = H5B2_LEAF_VERSION;

        /* b-tree type */
        *p++ = leaf->hdr->cls->id;
        HDassert((size_t)(p - leaf->hdr->page) == (H5B2_LEAF_PREFIX_SIZE - H5B2_SIZEOF_CHKSUM));

        /* Serialize records for leaf node */
        native = leaf->leaf_native;
        for(u = 0; u < leaf->nrec; u++) {
            /* Encode record */
            if((leaf->hdr->cls->encode)(p, native, leaf->hdr->cb_ctx) < 0)
                HGOTO_ERROR(H5E_BTREE, H5E_CANTENCODE, FAIL, "unable to encode B-tree record")

            /* Move to next record */
            p += leaf->hdr->rrec_size;
            native += leaf->hdr->cls->nrec_size;
        } /* end for */

        /* Compute metadata checksum */
        metadata_chksum = H5_checksum_metadata(leaf->hdr->page, (size_t)(p - leaf->hdr->page), 0);

        /* Metadata checksum */
        UINT32ENCODE(p, metadata_chksum);

	/* Write the B-tree leaf node */
        HDassert((size_t)(p - leaf->hdr->page) <= leaf->hdr->node_size);
	if(H5F_block_write(f, H5FD_MEM_BTREE, addr, leaf->hdr->node_size, dxpl_id, leaf->hdr->page) < 0)
	    HGOTO_ERROR(H5E_BTREE, H5E_CANTFLUSH, FAIL, "unable to save B-tree leaf node to disk")

	leaf->cache_info.is_dirty = FALSE;
    } /* end if */

    if(destroy)
        if(H5B2__cache_leaf_dest(f, leaf) < 0)
	    HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to destroy B-tree leaf node")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2__cache_leaf_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5B2__cache_leaf_dest
 *
 * Purpose:	Destroys a B-tree leaf node in memory.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 2 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B2__cache_leaf_dest(H5F_t *f, H5B2_leaf_t *leaf)
{
    herr_t ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_STATIC

    /* Check arguments */
    HDassert(f);
    HDassert(leaf);
    HDassert(leaf->hdr);

    /* If we're going to free the space on disk, the address must be valid */
    HDassert(!leaf->cache_info.free_file_space_on_destroy || H5F_addr_defined(leaf->cache_info.addr));

    /* Check for freeing file space for B-tree leaf node */
    if(leaf->cache_info.free_file_space_on_destroy) {
        /* Release the space on disk */
        /* (XXX: Nasty usage of internal DXPL value! -QAK) */
        if(H5MF_xfree(f, H5FD_MEM_BTREE, H5AC_dxpl_id, leaf->cache_info.addr, (hsize_t)leaf->hdr->node_size) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to free v2 B-tree leaf node")
    } /* end if */

    /* Destroy v2 b-tree leaf node */
    if(H5B2_leaf_free(leaf) < 0)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to destroy B-tree leaf node")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2__cache_leaf_dest() */


/*-------------------------------------------------------------------------
 * Function:	H5B2__cache_leaf_clear
 *
 * Purpose:	Mark a B-tree leaf node in memory as non-dirty.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb  2 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B2__cache_leaf_clear(H5F_t *f, H5B2_leaf_t *leaf, hbool_t destroy)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /*
     * Check arguments.
     */
    HDassert(leaf);

    /* Reset the dirty flag.  */
    leaf->cache_info.is_dirty = FALSE;

    if(destroy)
        if(H5B2__cache_leaf_dest(f, leaf) < 0)
	    HGOTO_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to destroy B-tree leaf node")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2__cache_leaf_clear() */


/*-------------------------------------------------------------------------
 * Function:	H5B2__cache_leaf_size
 *
 * Purpose:	Compute the size in bytes of a B-tree leaf node
 *		on disk, and return it in *size_ptr.  On failure,
 *		the value of *size_ptr is undefined.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 2 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B2__cache_leaf_size(const H5F_t UNUSED *f, const H5B2_leaf_t *leaf, size_t *size_ptr)
{
    FUNC_ENTER_STATIC_NOERR

    /* check arguments */
    HDassert(leaf);
    HDassert(leaf->hdr);
    HDassert(size_ptr);

    /* Set size value */
    *size_ptr = leaf->hdr->node_size;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5B2__cache_leaf_size() */

