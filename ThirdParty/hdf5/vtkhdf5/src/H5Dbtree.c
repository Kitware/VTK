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

/* Programmer: 	Robb Matzke <matzke@llnl.gov>
 *	       	Wednesday, October  8, 1997
 *
 * Purpose:	v1 B-tree indexed (chunked) I/O functions.  The chunks are
 *              given a multi-dimensional index which is used as a lookup key
 *              in a B-tree that maps chunk index to disk address.
 *
 */

/****************/
/* Module Setup */
/****************/

#define H5D_PACKAGE		/*suppress error about including H5Dpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Bprivate.h"		/* B-link trees				*/
#include "H5Dpkg.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* Files				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MFprivate.h"	/* File space management		*/
#include "H5Oprivate.h"		/* Object headers		  	*/
#include "H5Sprivate.h"         /* Dataspaces                           */
#include "H5Vprivate.h"		/* Vector and array functions		*/

/****************/
/* Local Macros */
/****************/

/*
 * Given a B-tree node return the dimensionality of the chunks pointed to by
 * that node.
 */
#define H5D_BTREE_NDIMS(X)	(((X)->sizeof_rkey-8)/8)

/******************/
/* Local Typedefs */
/******************/

/*
 * B-tree key.	A key contains the minimum logical N-dimensional coordinates and
 * the logical size of the chunk to which this key refers.  The
 * fastest-varying dimension is assumed to reference individual bytes of the
 * array, so a 100-element 1-d array of 4-byte integers would really be a 2-d
 * array with the slow varying dimension of size 100 and the fast varying
 * dimension of size 4 (the storage dimensionality has very little to do with
 * the real dimensionality).
 *
 * Only the first few values of the OFFSET and SIZE fields are actually
 * stored on disk, depending on the dimensionality.
 *
 * The chunk's file address is part of the B-tree and not part of the key.
 */
typedef struct H5D_btree_key_t {
    uint32_t	nbytes;				/*size of stored data	*/
    hsize_t	offset[H5O_LAYOUT_NDIMS];	/*logical offset to start*/
    unsigned	filter_mask;			/*excluded filters	*/
} H5D_btree_key_t;

/* B-tree callback info for iteration over chunks */
typedef struct H5D_btree_it_ud_t {
    H5D_chunk_common_ud_t common;		/* Common info for B-tree user data (must be first) */
    H5D_chunk_cb_func_t cb;                     /* Chunk callback routine */
    void		*udata;	                /* User data for chunk callback routine */
} H5D_btree_it_ud_t;

/* B-tree callback info for debugging */
typedef struct H5D_btree_dbg_t {
    H5D_chunk_common_ud_t common;               /* Common info for B-tree user data (must be first) */
    unsigned            ndims;                  /* Number of dimensions */
} H5D_btree_dbg_t;


/********************/
/* Local Prototypes */
/********************/

static herr_t H5D_btree_shared_create(const H5F_t *f, H5O_storage_chunk_t *store,
    unsigned ndims);

/* B-tree iterator callbacks */
static int H5D_btree_idx_iterate_cb(H5F_t *f, hid_t dxpl_id, const void *left_key,
    haddr_t addr, const void *right_key, void *_udata);

/* B-tree callbacks */
static H5RC_t *H5D_btree_get_shared(const H5F_t *f, const void *_udata);
static herr_t H5D_btree_new_node(H5F_t *f, hid_t dxpl_id, H5B_ins_t, void *_lt_key,
    void *_udata, void *_rt_key, haddr_t *addr_p /*out*/);
static int H5D_btree_cmp2(void *_lt_key, void *_udata, void *_rt_key);
static int H5D_btree_cmp3(void *_lt_key, void *_udata, void *_rt_key);
static htri_t H5D_btree_found(H5F_t *f, hid_t dxpl_id, haddr_t addr,
    const void *_lt_key, void *_udata);
static H5B_ins_t H5D_btree_insert(H5F_t *f, hid_t dxpl_id, haddr_t addr,
    void *_lt_key, hbool_t *lt_key_changed, void *_md_key, void *_udata,
    void *_rt_key, hbool_t *rt_key_changed, haddr_t *new_node/*out*/);
static H5B_ins_t H5D_btree_remove( H5F_t *f, hid_t dxpl_id, haddr_t addr,
    void *_lt_key, hbool_t *lt_key_changed, void *_udata, void *_rt_key,
    hbool_t *rt_key_changed);
static herr_t H5D_btree_decode_key(const H5B_shared_t *shared, const uint8_t *raw,
    void *_key);
static herr_t H5D_btree_encode_key(const H5B_shared_t *shared, uint8_t *raw,
    const void *_key);
static herr_t H5D_btree_debug_key(FILE *stream, int indent, int fwidth,
    const void *key, const void *udata);

/* Chunked layout indexing callbacks */
static herr_t H5D_btree_idx_init(const H5D_chk_idx_info_t *idx_info,
    const H5S_t *space, haddr_t dset_ohdr_addr);
static herr_t H5D_btree_idx_create(const H5D_chk_idx_info_t *idx_info);
static hbool_t H5D_btree_idx_is_space_alloc(const H5O_storage_chunk_t *storage);
static herr_t H5D_btree_idx_insert(const H5D_chk_idx_info_t *idx_info,
    H5D_chunk_ud_t *udata);
static herr_t H5D_btree_idx_get_addr(const H5D_chk_idx_info_t *idx_info,
    H5D_chunk_ud_t *udata);
static int H5D_btree_idx_iterate(const H5D_chk_idx_info_t *idx_info,
    H5D_chunk_cb_func_t chunk_cb, void *chunk_udata);
static herr_t H5D_btree_idx_remove(const H5D_chk_idx_info_t *idx_info,
    H5D_chunk_common_ud_t *udata);
static herr_t H5D_btree_idx_delete(const H5D_chk_idx_info_t *idx_info);
static herr_t H5D_btree_idx_copy_setup(const H5D_chk_idx_info_t *idx_info_src,
    const H5D_chk_idx_info_t *idx_info_dst);
static herr_t H5D_btree_idx_copy_shutdown(H5O_storage_chunk_t *storage_src,
    H5O_storage_chunk_t *storage_dst, hid_t dxpl_id);
static herr_t H5D_btree_idx_size(const H5D_chk_idx_info_t *idx_info,
    hsize_t *size);
static herr_t H5D_btree_idx_reset(H5O_storage_chunk_t *storage, hbool_t reset_addr);
static herr_t H5D_btree_idx_dump(const H5O_storage_chunk_t *storage,
    FILE *stream);
static herr_t H5D_btree_idx_dest(const H5D_chk_idx_info_t *idx_info);


/*********************/
/* Package Variables */
/*********************/

/* v1 B-tree indexed chunk I/O ops */
const H5D_chunk_ops_t H5D_COPS_BTREE[1] = {{
    H5D_btree_idx_init,
    H5D_btree_idx_create,
    H5D_btree_idx_is_space_alloc,
    H5D_btree_idx_insert,
    H5D_btree_idx_get_addr,
    NULL,
    H5D_btree_idx_iterate,
    H5D_btree_idx_remove,
    H5D_btree_idx_delete,
    H5D_btree_idx_copy_setup,
    H5D_btree_idx_copy_shutdown,
    H5D_btree_idx_size,
    H5D_btree_idx_reset,
    H5D_btree_idx_dump,
    H5D_btree_idx_dest
}};


/*****************************/
/* Library Private Variables */
/*****************************/

/* inherits B-tree like properties from H5B */
H5B_class_t H5B_BTREE[1] = {{
    H5B_CHUNK_ID,		/*id			*/
    sizeof(H5D_btree_key_t),	/*sizeof_nkey		*/
    H5D_btree_get_shared,	/*get_shared		*/
    H5D_btree_new_node,		/*new			*/
    H5D_btree_cmp2,		/*cmp2			*/
    H5D_btree_cmp3,		/*cmp3			*/
    H5D_btree_found,		/*found			*/
    H5D_btree_insert,		/*insert		*/
    FALSE,			/*follow min branch?	*/
    FALSE,			/*follow max branch?	*/
    H5B_LEFT,                   /*critical key          */
    H5D_btree_remove,           /*remove		*/
    H5D_btree_decode_key,	/*decode		*/
    H5D_btree_encode_key,	/*encode		*/
    H5D_btree_debug_key,	/*debug			*/
}};


/*******************/
/* Local Variables */
/*******************/


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_get_shared
 *
 * Purpose:	Returns the shared B-tree info for the specified UDATA.
 *
 * Return:	Success:	Pointer to the raw B-tree page for this dataset
 *
 *		Failure:	Can't fail
 *
 * Programmer:	Quincey Koziol
 *		Monday, July  5, 2004
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static H5RC_t *
H5D_btree_get_shared(const H5F_t UNUSED *f, const void *_udata)
{
    const H5D_chunk_common_ud_t *udata = (const H5D_chunk_common_ud_t *) _udata;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_btree_get_shared)

    HDassert(udata);
    HDassert(udata->storage);
    HDassert(udata->storage->idx_type == H5D_CHUNK_BTREE);
    HDassert(udata->storage->u.btree.shared);

    /* Return the pointer to the ref-count object */
    FUNC_LEAVE_NOAPI(udata->storage->u.btree.shared)
} /* end H5D_btree_get_shared() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_new_node
 *
 * Purpose:	Adds a new entry to an i-storage B-tree.  We can assume that
 *		the domain represented by UDATA doesn't intersect the domain
 *		already represented by the B-tree.
 *
 * Return:	Success:	Non-negative. The address of leaf is returned
 *				through the ADDR argument.  It is also added
 *				to the UDATA.
 *
 * 		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		Tuesday, October 14, 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_new_node(H5F_t *f, hid_t dxpl_id, H5B_ins_t op,
		    void *_lt_key, void *_udata, void *_rt_key,
		    haddr_t *addr_p/*out*/)
{
    H5D_btree_key_t	*lt_key = (H5D_btree_key_t *) _lt_key;
    H5D_btree_key_t	*rt_key = (H5D_btree_key_t *) _rt_key;
    H5D_chunk_ud_t	*udata = (H5D_chunk_ud_t *) _udata;
    unsigned		u;
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_btree_new_node)

    /* check args */
    HDassert(f);
    HDassert(lt_key);
    HDassert(rt_key);
    HDassert(udata);
    HDassert(udata->common.layout->ndims > 0 && udata->common.layout->ndims < H5O_LAYOUT_NDIMS);
    HDassert(addr_p);

    /* Allocate new storage */
    HDassert(udata->nbytes > 0);
    H5_CHECK_OVERFLOW(udata->nbytes, uint32_t, hsize_t);
    if(HADDR_UNDEF == (*addr_p = H5MF_alloc(f, H5FD_MEM_DRAW, dxpl_id, (hsize_t)udata->nbytes)))
        HGOTO_ERROR(H5E_IO, H5E_CANTINIT, FAIL, "couldn't allocate new file storage")
    udata->addr = *addr_p;

    /*
     * The left key describes the storage of the UDATA chunk being
     * inserted into the tree.
     */
    lt_key->nbytes = udata->nbytes;
    lt_key->filter_mask = udata->filter_mask;
    for(u = 0; u < udata->common.layout->ndims; u++)
        lt_key->offset[u] = udata->common.offset[u];

    /*
     * The right key might already be present.  If not, then add a zero-width
     * chunk.
     */
    if(H5B_INS_LEFT != op) {
        rt_key->nbytes = 0;
        rt_key->filter_mask = 0;
        for(u = 0; u < udata->common.layout->ndims; u++) {
            HDassert(udata->common.offset[u] + udata->common.layout->dim[u] >
                udata->common.offset[u]);
            rt_key->offset[u] = udata->common.offset[u] + udata->common.layout->dim[u];
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_new_node() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_cmp2
 *
 * Purpose:	Compares two keys sort of like strcmp().  The UDATA pointer
 *		is only to supply extra information not carried in the keys
 *		(in this case, the dimensionality) and is not compared
 *		against the keys.
 *
 * Return:	Success:	-1 if LT_KEY is less than RT_KEY;
 *				1 if LT_KEY is greater than RT_KEY;
 *				0 if LT_KEY and RT_KEY are equal.
 *
 *		Failure:	FAIL (same as LT_KEY<RT_KEY)
 *
 * Programmer:	Robb Matzke
 *		Thursday, November  6, 1997
 *
 *-------------------------------------------------------------------------
 */
static int
H5D_btree_cmp2(void *_lt_key, void *_udata, void *_rt_key)
{
    H5D_btree_key_t	*lt_key = (H5D_btree_key_t *) _lt_key;
    H5D_btree_key_t	*rt_key = (H5D_btree_key_t *) _rt_key;
    H5D_chunk_common_ud_t	*udata = (H5D_chunk_common_ud_t *) _udata;
    int		ret_value;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_btree_cmp2)

    HDassert(lt_key);
    HDassert(rt_key);
    HDassert(udata);
    HDassert(udata->layout->ndims > 0 && udata->layout->ndims <= H5O_LAYOUT_NDIMS);

    /* Compare the offsets but ignore the other fields */
    ret_value = H5V_vector_cmp_u(udata->layout->ndims, lt_key->offset, rt_key->offset);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_cmp2() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_cmp3
 *
 * Purpose:	Compare the requested datum UDATA with the left and right
 *		keys of the B-tree.
 *
 * Return:	Success:	negative if the min_corner of UDATA is less
 *				than the min_corner of LT_KEY.
 *
 *				positive if the min_corner of UDATA is
 *				greater than or equal the min_corner of
 *				RT_KEY.
 *
 *				zero otherwise.	 The min_corner of UDATA is
 *				not necessarily contained within the address
 *				space represented by LT_KEY, but a key that
 *				would describe the UDATA min_corner address
 *				would fall lexicographically between LT_KEY
 *				and RT_KEY.
 *
 *		Failure:	FAIL (same as UDATA < LT_KEY)
 *
 * Programmer:	Robb Matzke
 *		Wednesday, October  8, 1997
 *
 *-------------------------------------------------------------------------
 */
static int
H5D_btree_cmp3(void *_lt_key, void *_udata, void *_rt_key)
{
    H5D_btree_key_t	*lt_key = (H5D_btree_key_t *) _lt_key;
    H5D_btree_key_t	*rt_key = (H5D_btree_key_t *) _rt_key;
    H5D_chunk_common_ud_t	*udata = (H5D_chunk_common_ud_t *) _udata;
    int		ret_value = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_btree_cmp3)

    HDassert(lt_key);
    HDassert(rt_key);
    HDassert(udata);
    HDassert(udata->layout->ndims > 0 && udata->layout->ndims <= H5O_LAYOUT_NDIMS);

    /* Special case for faster checks on 1-D chunks */
    /* (Checking for ndims==2 because last dimension is the datatype size) */
    /* The additional checking for the right key is necessary due to the */
    /* slightly odd way the library initializes the right-most node in the */
    /* indexed storage B-tree... */
    /* (Dump the B-tree with h5debug to look at it) -QAK */
    if(udata->layout->ndims == 2) {
        if(udata->offset[0] > rt_key->offset[0])
            ret_value = 1;
        else if(udata->offset[0] == rt_key->offset[0] &&
                udata->offset[1] >= rt_key->offset[1])
            ret_value = 1;
        else if(udata->offset[0] < lt_key->offset[0])
            ret_value = (-1);
    } /* end if */
    else {
        if(H5V_vector_ge_u(udata->layout->ndims, udata->offset, rt_key->offset))
            ret_value = 1;
        else if(H5V_vector_lt_u(udata->layout->ndims, udata->offset, lt_key->offset))
            ret_value = (-1);
    } /* end else */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_cmp3() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_found
 *
 * Purpose:	This function is called when the B-tree search engine has
 *		found the leaf entry that points to a chunk of storage that
 *		contains the beginning of the logical address space
 *		represented by UDATA.  The LT_KEY is the left key (the one
 *		that describes the chunk) and RT_KEY is the right key (the
 *		one that describes the next or last chunk).
 *
 * Note:	It's possible that the chunk isn't really found.  For
 *		instance, in a sparse dataset the requested chunk might fall
 *		between two stored chunks in which case this function is
 *		called with the maximum stored chunk indices less than the
 *		requested chunk indices.
 *
 * Return:	Non-negative (TRUE/FALSE) on success with information about the
 *              chunk returned through the UDATA argument. Negative on failure.
 *
 * Programmer:	Robb Matzke
 *		Thursday, October  9, 1997
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static htri_t
H5D_btree_found(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, haddr_t addr, const void *_lt_key,
		 void *_udata)
{
    H5D_chunk_ud_t	   *udata = (H5D_chunk_ud_t *) _udata;
    const H5D_btree_key_t *lt_key = (const H5D_btree_key_t *) _lt_key;
    unsigned		u;
    htri_t      ret_value = TRUE;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_btree_found)

    /* Check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(udata);
    HDassert(lt_key);

    /* Is this *really* the requested chunk? */
    for(u = 0; u < udata->common.layout->ndims; u++)
        if(udata->common.offset[u] >= lt_key->offset[u] + udata->common.layout->dim[u])
            HGOTO_DONE(FALSE)

    /* Initialize return values */
    HDassert(lt_key->nbytes > 0);
    udata->addr = addr;
    udata->nbytes = lt_key->nbytes;
    udata->filter_mask = lt_key->filter_mask;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_found() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_insert
 *
 * Purpose:	This function is called when the B-tree insert engine finds
 *		the node to use to insert new data.  The UDATA argument
 *		points to a struct that describes the logical addresses being
 *		added to the file.  This function allocates space for the
 *		data and returns information through UDATA describing a
 *		file chunk to receive (part of) the data.
 *
 *		The LT_KEY is always the key describing the chunk of file
 *		memory at address ADDR. On entry, UDATA describes the logical
 *		addresses for which storage is being requested (through the
 *		`offset' and `size' fields). On return, UDATA describes the
 *		logical addresses contained in a chunk on disk.
 *
 * Return:	Success:	An insertion command for the caller, one of
 *				the H5B_INS_* constants.  The address of the
 *				new chunk is returned through the NEW_NODE
 *				argument.
 *
 *		Failure:	H5B_INS_ERROR
 *
 * Programmer:	Robb Matzke
 *		Thursday, October  9, 1997
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static H5B_ins_t
H5D_btree_insert(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *_lt_key,
		  hbool_t *lt_key_changed,
		  void *_md_key, void *_udata, void *_rt_key,
		  hbool_t UNUSED *rt_key_changed,
		  haddr_t *new_node_p/*out*/)
{
    H5D_btree_key_t	*lt_key = (H5D_btree_key_t *) _lt_key;
    H5D_btree_key_t	*md_key = (H5D_btree_key_t *) _md_key;
    H5D_btree_key_t	*rt_key = (H5D_btree_key_t *) _rt_key;
    H5D_chunk_ud_t	*udata = (H5D_chunk_ud_t *) _udata;
    int		cmp;
    unsigned		u;
    H5B_ins_t		ret_value;

    FUNC_ENTER_NOAPI_NOINIT(H5D_btree_insert)

    /* check args */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(lt_key);
    HDassert(lt_key_changed);
    HDassert(md_key);
    HDassert(udata);
    HDassert(rt_key);
    HDassert(new_node_p);

    cmp = H5D_btree_cmp3(lt_key, udata, rt_key);
    HDassert(cmp <= 0);

    if(cmp < 0) {
        /* Negative indices not supported yet */
        HGOTO_ERROR(H5E_STORAGE, H5E_UNSUPPORTED, H5B_INS_ERROR, "internal error")

    } else if(H5V_vector_eq_u(udata->common.layout->ndims,
				udata->common.offset, lt_key->offset) &&
	       lt_key->nbytes > 0) {
        /*
         * Already exists.  If the new size is not the same as the old size
         * then we should reallocate storage.
         */
        if(lt_key->nbytes != udata->nbytes) {
/* Currently, the old chunk data is "thrown away" after the space is reallocated,
 * so avoid data copy in H5MF_realloc() call by just free'ing the space and
 * allocating new space.
 *
 * This should keep the file smaller also, by freeing the space and then
 * allocating new space, instead of vice versa (in H5MF_realloc).
 *
 * QAK - 11/19/2002
 */
#ifdef OLD_WAY
            if(HADDR_UNDEF == (*new_node_p = H5MF_realloc(f, H5FD_MEM_DRAW, addr,
                      (hsize_t)lt_key->nbytes, (hsize_t)udata->nbytes)))
                HGOTO_ERROR(H5E_STORAGE, H5E_NOSPACE, H5B_INS_ERROR, "unable to reallocate chunk storage")
#else /* OLD_WAY */
            H5_CHECK_OVERFLOW(lt_key->nbytes, uint32_t, hsize_t);
            if(H5MF_xfree(f, H5FD_MEM_DRAW, dxpl_id, addr, (hsize_t)lt_key->nbytes) < 0)
                HGOTO_ERROR(H5E_STORAGE, H5E_CANTFREE, H5B_INS_ERROR, "unable to free chunk")
            H5_CHECK_OVERFLOW(udata->nbytes, uint32_t, hsize_t);
            if(HADDR_UNDEF == (*new_node_p = H5MF_alloc(f, H5FD_MEM_DRAW, dxpl_id, (hsize_t)udata->nbytes)))
                HGOTO_ERROR(H5E_STORAGE, H5E_NOSPACE, H5B_INS_ERROR, "unable to reallocate chunk")
#endif /* OLD_WAY */
            lt_key->nbytes = udata->nbytes;
            lt_key->filter_mask = udata->filter_mask;
            *lt_key_changed = TRUE;
            udata->addr = *new_node_p;
            ret_value = H5B_INS_CHANGE;
        } else {
            udata->addr = addr;
            ret_value = H5B_INS_NOOP;
        }

    } else if (H5V_hyper_disjointp(udata->common.layout->ndims,
				   lt_key->offset, udata->common.layout->dim,
				   udata->common.offset, udata->common.layout->dim)) {
        HDassert(H5V_hyper_disjointp(udata->common.layout->ndims,
				   rt_key->offset, udata->common.layout->dim,
				   udata->common.offset, udata->common.layout->dim));
        /*
         * Split this node, inserting the new new node to the right of the
         * current node.  The MD_KEY is where the split occurs.
         */
        md_key->nbytes = udata->nbytes;
        md_key->filter_mask = udata->filter_mask;
        for(u = 0; u < udata->common.layout->ndims; u++) {
            HDassert(0 == udata->common.offset[u] % udata->common.layout->dim[u]);
            md_key->offset[u] = udata->common.offset[u];
        } /* end for */

        /*
         * Allocate storage for the new chunk
         */
        H5_CHECK_OVERFLOW(udata->nbytes, uint32_t, hsize_t);
        if(HADDR_UNDEF == (*new_node_p = H5MF_alloc(f, H5FD_MEM_DRAW, dxpl_id, (hsize_t)udata->nbytes)))
            HGOTO_ERROR(H5E_STORAGE, H5E_NOSPACE, H5B_INS_ERROR, "file allocation failed")
        udata->addr = *new_node_p;
        ret_value = H5B_INS_RIGHT;

    } else {
        HGOTO_ERROR(H5E_IO, H5E_UNSUPPORTED, H5B_INS_ERROR, "internal error")
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_insert() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_remove
 *
 * Purpose:	Removes chunks that are no longer necessary in the B-tree.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer: Robb Matzke
 *             Pedro Vicente, pvn@ncsa.uiuc.edu
 * 		March 28, 2002
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static H5B_ins_t
H5D_btree_remove(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *_lt_key /*in,out */ ,
	hbool_t *lt_key_changed /*out */ ,
	void UNUSED * _udata /*in,out */ ,
	void UNUSED * _rt_key /*in,out */ ,
	hbool_t *rt_key_changed /*out */ )
{
    H5D_btree_key_t    *lt_key = (H5D_btree_key_t *)_lt_key;
    H5B_ins_t ret_value=H5B_INS_REMOVE; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_btree_remove)

    /* Remove raw data chunk from file */
    H5_CHECK_OVERFLOW(lt_key->nbytes, uint32_t, hsize_t);
    if(H5MF_xfree(f, H5FD_MEM_DRAW, dxpl_id, addr, (hsize_t)lt_key->nbytes) < 0)
        HGOTO_ERROR(H5E_STORAGE, H5E_CANTFREE, H5B_INS_ERROR, "unable to free chunk")

    /* Mark keys as unchanged */
    *lt_key_changed = FALSE;
    *rt_key_changed = FALSE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_remove() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_decode_key
 *
 * Purpose:	Decodes a raw key into a native key for the B-tree
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Friday, October 10, 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_decode_key(const H5B_shared_t *shared, const uint8_t *raw, void *_key)
{
    H5D_btree_key_t	*key = (H5D_btree_key_t *) _key;
    size_t		ndims;
    unsigned		u;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_btree_decode_key)

    /* check args */
    HDassert(shared);
    HDassert(raw);
    HDassert(key);
    ndims = H5D_BTREE_NDIMS(shared);
    HDassert(ndims <= H5O_LAYOUT_NDIMS);

    /* decode */
    UINT32DECODE(raw, key->nbytes);
    UINT32DECODE(raw, key->filter_mask);
    for(u = 0; u < ndims; u++)
	UINT64DECODE(raw, key->offset[u]);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D_btree_decode_key() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_encode_key
 *
 * Purpose:	Encode a key from native format to raw format.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Friday, October 10, 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_encode_key(const H5B_shared_t *shared, uint8_t *raw, const void *_key)
{
    const H5D_btree_key_t *key = (const H5D_btree_key_t *)_key;
    size_t		ndims;
    unsigned		u;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_btree_encode_key)

    /* check args */
    HDassert(shared);
    HDassert(raw);
    HDassert(key);
    ndims = H5D_BTREE_NDIMS(shared);
    HDassert(ndims <= H5O_LAYOUT_NDIMS);

    /* encode */
    UINT32ENCODE(raw, key->nbytes);
    UINT32ENCODE(raw, key->filter_mask);
    for(u = 0; u < ndims; u++)
	UINT64ENCODE(raw, key->offset[u]);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D_btree_encode_key() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_debug_key
 *
 * Purpose:	Prints a key.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, April 16, 1998
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5D_btree_debug_key(FILE *stream, int indent, int fwidth, const void *_key,
    const void *_udata)
{
    const H5D_btree_key_t	*key = (const H5D_btree_key_t *)_key;
    const H5D_btree_dbg_t	*udata = (const H5D_btree_dbg_t *)_udata;
    unsigned		u;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_btree_debug_key)

    HDassert(key);

    HDfprintf(stream, "%*s%-*s %u bytes\n", indent, "", fwidth, "Chunk size:", (unsigned)key->nbytes);
    HDfprintf(stream, "%*s%-*s 0x%08x\n", indent, "", fwidth, "Filter mask:", key->filter_mask);
    HDfprintf(stream, "%*s%-*s {", indent, "", fwidth, "Logical offset:");
    for(u = 0; u < udata->ndims; u++)
        HDfprintf(stream, "%s%Hd", u?", ":"", key->offset[u]);
    HDfputs("}\n", stream);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D_btree_debug_key() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_shared_create
 *
 * Purpose:	Create & initialize B-tree shared info
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 27, 2004
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_shared_create(const H5F_t *f, H5O_storage_chunk_t *store, unsigned ndims)
{
    H5B_shared_t *shared;               /* Shared B-tree node info */
    size_t	sizeof_rkey;	        /* Size of raw (disk) key	     */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_btree_shared_create)

    /* Set the raw key size */
    sizeof_rkey = 4 +			/*storage size		*/
                4 +			/*filter mask		*/
                ndims * 8;		/*dimension indices	*/

    /* Allocate & initialize global info for the shared structure */
    if(NULL == (shared = H5B_shared_new(f, H5B_BTREE, sizeof_rkey)))
	HGOTO_ERROR(H5E_BTREE, H5E_NOSPACE, FAIL, "memory allocation failed for shared B-tree info")

    /* Set up the "local" information for this dataset's chunks */
        /* <none> */

    /* Make shared B-tree info reference counted */
    if(NULL == (store->u.btree.shared = H5RC_create(shared, H5B_shared_free)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't create ref-count wrapper for shared B-tree info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_shared_create() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_idx_init
 *
 * Purpose:	Initialize the indexing information for a dataset.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, May 18, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_idx_init(const H5D_chk_idx_info_t *idx_info, const H5S_t UNUSED *space,
    haddr_t UNUSED dset_ohdr_addr)
{
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_btree_idx_init)

    /* Check args */
    HDassert(idx_info);
    HDassert(idx_info->f);
    HDassert(idx_info->pline);
    HDassert(idx_info->layout);
    HDassert(idx_info->storage);
    HDassert(H5F_addr_defined(dset_ohdr_addr));

    /* Allocate the shared structure */
    if(H5D_btree_shared_create(idx_info->f, idx_info->storage, idx_info->layout->ndims) < 0)
	HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't create wrapper for shared B-tree info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_idx_init() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_idx_create
 *
 * Purpose:	Creates a new indexed-storage B-tree and initializes the
 *		layout struct with information about the storage.  The
 *		struct should be immediately written to the object header.
 *
 *		This function must be called before passing LAYOUT to any of
 *		the other indexed storage functions!
 *
 * Return:	Non-negative on success (with the LAYOUT argument initialized
 *		and ready to write to an object header). Negative on failure.
 *
 * Programmer:	Robb Matzke
 *		Tuesday, October 21, 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_idx_create(const H5D_chk_idx_info_t *idx_info)
{
    H5D_chunk_common_ud_t udata;             /* User data for B-tree callback */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_btree_idx_create)

    /* Check args */
    HDassert(idx_info);
    HDassert(idx_info->f);
    HDassert(idx_info->pline);
    HDassert(idx_info->layout);
    HDassert(idx_info->storage);
    HDassert(!H5F_addr_defined(idx_info->storage->idx_addr));

    /* Initialize "user" data for B-tree callbacks, etc. */
    udata.layout = idx_info->layout;
    udata.storage = idx_info->storage;

    /* Create the v1 B-tree for the chunk index */
    if(H5B_create(idx_info->f, idx_info->dxpl_id, H5B_BTREE, &udata, &(idx_info->storage->idx_addr)/*out*/) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't create B-tree")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_idx_create() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_idx_is_space_alloc
 *
 * Purpose:	Query if space is allocated for index method
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Thursday, January 15, 2009
 *
 *-------------------------------------------------------------------------
 */
static hbool_t
H5D_btree_idx_is_space_alloc(const H5O_storage_chunk_t *storage)
{
    hbool_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_btree_idx_is_space_alloc)

    /* Check args */
    HDassert(storage);

    /* Set return value */
    ret_value = (hbool_t)H5F_addr_defined(storage->idx_addr);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_idx_is_space_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_idx_insert
 *
 * Purpose:	Create the chunk it if it doesn't exist, or reallocate the
 *              chunk if its size changed.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_idx_insert(const H5D_chk_idx_info_t *idx_info, H5D_chunk_ud_t *udata)
{
    herr_t	ret_value = SUCCEED;		/* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_btree_idx_insert)

    HDassert(idx_info);
    HDassert(idx_info->f);
    HDassert(idx_info->pline);
    HDassert(idx_info->layout);
    HDassert(idx_info->storage);
    HDassert(H5F_addr_defined(idx_info->storage->idx_addr));
    HDassert(udata);

    /*
     * Create the chunk it if it doesn't exist, or reallocate the chunk if
     * its size changed.
     */
    if(H5B_insert(idx_info->f, idx_info->dxpl_id, H5B_BTREE, idx_info->storage->idx_addr, udata) < 0)
        HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to allocate chunk")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_btree_idx_insert() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_idx_get_addr
 *
 * Purpose:	Get the file address of a chunk if file space has been
 *		assigned.  Save the retrieved information in the udata
 *		supplied.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Albert Cheng
 *              June 27, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_idx_get_addr(const H5D_chk_idx_info_t *idx_info, H5D_chunk_ud_t *udata)
{
    herr_t	ret_value = SUCCEED;	/* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_btree_idx_get_addr)

    HDassert(idx_info);
    HDassert(idx_info->f);
    HDassert(idx_info->pline);
    HDassert(idx_info->layout);
    HDassert(idx_info->layout->ndims > 0);
    HDassert(idx_info->storage);
    HDassert(H5F_addr_defined(idx_info->storage->idx_addr));
    HDassert(udata);

    /* Go get the chunk information from the B-tree */
    if(H5B_find(idx_info->f, idx_info->dxpl_id, H5B_BTREE, idx_info->storage->idx_addr, udata) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get chunk info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_btree_idx_get_addr() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_idx_iterate_cb
 *
 * Purpose:	Translate the B-tree specific chunk record into a generic
 *              form and make the callback to the generic chunk callback
 *              routine.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, May 20, 2008
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
H5D_btree_idx_iterate_cb(H5F_t UNUSED *f, hid_t UNUSED dxpl_id,
    const void *_lt_key, haddr_t addr, const void UNUSED *_rt_key,
    void *_udata)
{
    H5D_btree_it_ud_t	*udata = (H5D_btree_it_ud_t *)_udata; /* User data */
    const H5D_btree_key_t	*lt_key = (const H5D_btree_key_t *)_lt_key; /* B-tree key for chunk */
    H5D_chunk_rec_t chunk_rec;  /* Generic chunk record for callback */
    int ret_value;              /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR(H5D_btree_idx_iterate_cb)

    /* Sanity check for memcpy() */
    HDcompile_assert(offsetof(H5D_chunk_rec_t, nbytes) == offsetof(H5D_btree_key_t, nbytes));
    HDcompile_assert(sizeof(chunk_rec.nbytes) == sizeof(lt_key->nbytes));
    HDcompile_assert(offsetof(H5D_chunk_rec_t, offset) == offsetof(H5D_btree_key_t, offset));
    HDcompile_assert(sizeof(chunk_rec.offset) == sizeof(lt_key->offset));
    HDcompile_assert(offsetof(H5D_chunk_rec_t, filter_mask) == offsetof(H5D_btree_key_t, filter_mask));
    HDcompile_assert(sizeof(chunk_rec.filter_mask) == sizeof(lt_key->filter_mask));

    /* Compose generic chunk record for callback */
    HDmemcpy(&chunk_rec, lt_key, sizeof(*lt_key));
    chunk_rec.chunk_addr = addr;

    /* Make "generic chunk" callback */
    if((ret_value = (udata->cb)(&chunk_rec, udata->udata)) < 0)
        HERROR(H5E_DATASET, H5E_CALLBACK, "failure in generic chunk iterator callback");

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_btree_idx_iterate_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_idx_iterate
 *
 * Purpose:	Iterate over the chunks in an index, making a callback
 *              for each one.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, May 20, 2008
 *
 *-------------------------------------------------------------------------
 */
static int
H5D_btree_idx_iterate(const H5D_chk_idx_info_t *idx_info,
    H5D_chunk_cb_func_t chunk_cb, void *chunk_udata)
{
    H5D_btree_it_ud_t	udata;  /* User data for B-tree iterator callback */
    int ret_value;              /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR(H5D_btree_idx_iterate)

    HDassert(idx_info);
    HDassert(idx_info->f);
    HDassert(idx_info->pline);
    HDassert(idx_info->layout);
    HDassert(idx_info->storage);
    HDassert(H5F_addr_defined(idx_info->storage->idx_addr));
    HDassert(chunk_cb);
    HDassert(chunk_udata);

    /* Initialize userdata */
    HDmemset(&udata, 0, sizeof udata);
    udata.common.layout = idx_info->layout;
    udata.common.storage = idx_info->storage;
    udata.cb = chunk_cb;
    udata.udata = chunk_udata;

    /* Iterate over existing chunks */
    if((ret_value = H5B_iterate(idx_info->f, idx_info->dxpl_id, H5B_BTREE, idx_info->storage->idx_addr, H5D_btree_idx_iterate_cb, &udata)) < 0)
        HERROR(H5E_DATASET, H5E_BADITER, "unable to iterate over chunk B-tree");

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_idx_iterate() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_idx_remove
 *
 * Purpose:	Remove chunk from index.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, May 22, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_idx_remove(const H5D_chk_idx_info_t *idx_info, H5D_chunk_common_ud_t *udata)
{
    herr_t	ret_value = SUCCEED;		/* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_btree_idx_remove)

    HDassert(idx_info);
    HDassert(idx_info->f);
    HDassert(idx_info->pline);
    HDassert(idx_info->layout);
    HDassert(idx_info->storage);
    HDassert(H5F_addr_defined(idx_info->storage->idx_addr));
    HDassert(udata);

    /* Remove the chunk from the v1 B-tree index and release the space for the
     * chunk (in the B-tree callback).
     */
    if(H5B_remove(idx_info->f, idx_info->dxpl_id, H5B_BTREE, idx_info->storage->idx_addr, udata) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTDELETE, FAIL, "unable to remove chunk entry")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5D_btree_idx_remove() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_idx_delete
 *
 * Purpose:	Delete index and raw data storage for entire dataset
 *              (i.e. all chunks)
 *
 * Return:	Success:	Non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Thursday, March 20, 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_idx_delete(const H5D_chk_idx_info_t *idx_info)
{
    herr_t ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_btree_idx_delete)

    /* Sanity checks */
    HDassert(idx_info);
    HDassert(idx_info->f);
    HDassert(idx_info->pline);
    HDassert(idx_info->layout);
    HDassert(idx_info->storage);

    /* Check if the index data structure has been allocated */
    if(H5F_addr_defined(idx_info->storage->idx_addr)) {
        H5O_storage_chunk_t tmp_storage;  /* Local copy of storage info */
        H5D_chunk_common_ud_t udata;            /* User data for B-tree operations */

        /* Set up temporary chunked storage info */
        tmp_storage = *idx_info->storage;

        /* Set up the shared structure */
        if(H5D_btree_shared_create(idx_info->f, &tmp_storage, idx_info->layout->ndims) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "can't create wrapper for shared B-tree info")

        /* Set up B-tree user data */
        HDmemset(&udata, 0, sizeof udata);
        udata.layout = idx_info->layout;
        udata.storage = &tmp_storage;

        /* Delete entire B-tree */
        if(H5B_delete(idx_info->f, idx_info->dxpl_id, H5B_BTREE, tmp_storage.idx_addr, &udata) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTDELETE, FAIL, "unable to delete chunk B-tree")

        /* Release the shared B-tree page */
        if(NULL == tmp_storage.u.btree.shared)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "ref-counted page nil")
        if(H5RC_DEC(tmp_storage.u.btree.shared) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "unable to decrement ref-counted page")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_idx_delete() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_idx_copy_setup
 *
 * Purpose:	Set up any necessary information for copying chunks
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, May 29, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_idx_copy_setup(const H5D_chk_idx_info_t *idx_info_src,
    const H5D_chk_idx_info_t *idx_info_dst)
{
    herr_t      ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_btree_idx_copy_setup)

    HDassert(idx_info_src);
    HDassert(idx_info_src->f);
    HDassert(idx_info_src->pline);
    HDassert(idx_info_src->layout);
    HDassert(idx_info_src->storage);
    HDassert(idx_info_dst);
    HDassert(idx_info_dst->f);
    HDassert(idx_info_dst->pline);
    HDassert(idx_info_dst->layout);
    HDassert(idx_info_dst->storage);
    HDassert(!H5F_addr_defined(idx_info_dst->storage->idx_addr));

    /* Create shared B-tree info for each file */
    if(H5D_btree_shared_create(idx_info_src->f, idx_info_src->storage, idx_info_src->layout->ndims) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't create wrapper for source shared B-tree info")
    if(H5D_btree_shared_create(idx_info_dst->f, idx_info_dst->storage, idx_info_dst->layout->ndims) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't create wrapper for destination shared B-tree info")

    /* Create the root of the B-tree that describes chunked storage in the dest. file */
    if(H5D_btree_idx_create(idx_info_dst) < 0)
        HGOTO_ERROR(H5E_IO, H5E_CANTINIT, FAIL, "unable to initialize chunked storage")
    HDassert(H5F_addr_defined(idx_info_dst->storage->idx_addr));

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_idx_copy_setup() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_idx_copy_shutdown
 *
 * Purpose:	Shutdown any information from copying chunks
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, May 29, 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_idx_copy_shutdown(H5O_storage_chunk_t *storage_src,
    H5O_storage_chunk_t *storage_dst,
    hid_t UNUSED dxpl_id)
{
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_btree_idx_copy_shutdown)

    HDassert(storage_src);
    HDassert(storage_dst);

    /* Decrement refcount on shared B-tree info */
    if(H5RC_DEC(storage_src->u.btree.shared) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTDEC, FAIL, "unable to decrement ref-counted page")
    if(H5RC_DEC(storage_dst->u.btree.shared) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTDEC, FAIL, "unable to decrement ref-counted page")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_idx_copy_shutdown() */


/*-------------------------------------------------------------------------
 * Function:    H5D_btree_idx_size
 *
 * Purpose:     Retrieve the amount of index storage for chunked dataset
 *
 * Return:      Success:        Non-negative
 *              Failure:        negative
 *
 * Programmer:  Vailin Choi
 *              June 8, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_idx_size(const H5D_chk_idx_info_t *idx_info, hsize_t *index_size)
{
    H5D_chunk_common_ud_t udata;              /* User-data for loading B-tree nodes */
    H5B_info_t bt_info;                 /* B-tree info */
    hbool_t shared_init = FALSE;        /* Whether shared B-tree info is initialized */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5D_btree_idx_size, FAIL)

    /* Check args */
    HDassert(idx_info);
    HDassert(idx_info->f);
    HDassert(idx_info->pline);
    HDassert(idx_info->layout);
    HDassert(idx_info->storage);
    HDassert(index_size);

    /* Initialize the shared info for the B-tree traversal */
    if(H5D_btree_shared_create(idx_info->f, idx_info->storage, idx_info->layout->ndims) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't create wrapper for shared B-tree info")
    shared_init = TRUE;

    /* Initialize B-tree node user-data */
    HDmemset(&udata, 0, sizeof udata);
    udata.layout = idx_info->layout;
    udata.storage = idx_info->storage;

    /* Get metadata information for B-tree */
    if(H5B_get_info(idx_info->f, idx_info->dxpl_id, H5B_BTREE, idx_info->storage->idx_addr, &bt_info, NULL, &udata) < 0)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTINIT, FAIL, "unable to iterate over chunk B-tree")

    /* Set the size of the B-tree */
    *index_size = bt_info.size;

done:
    if(shared_init) {
        if(NULL == idx_info->storage->u.btree.shared)
            HDONE_ERROR(H5E_IO, H5E_CANTFREE, FAIL, "ref-counted page nil")
        if(H5RC_DEC(idx_info->storage->u.btree.shared) < 0)
            HDONE_ERROR(H5E_IO, H5E_CANTFREE, FAIL, "unable to decrement ref-counted page")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_idx_size() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_idx_reset
 *
 * Purpose:	Reset indexing information.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, January 15, 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_idx_reset(H5O_storage_chunk_t *storage, hbool_t reset_addr)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_btree_idx_reset)

    HDassert(storage);

    /* Reset index info */
    if(reset_addr)
	storage->idx_addr = HADDR_UNDEF;
    storage->u.btree.shared = NULL;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D_btree_idx_reset() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_idx_dump
 *
 * Purpose:	Dump indexing information to a stream.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, January 15, 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_idx_dump(const H5O_storage_chunk_t *storage, FILE *stream)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5D_btree_idx_dump)

    HDassert(storage);
    HDassert(stream);

    HDfprintf(stream, "    Address: %a\n", storage->idx_addr);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5D_btree_idx_dump() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_idx_dest
 *
 * Purpose:	Release indexing information in memory.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_btree_idx_dest(const H5D_chk_idx_info_t *idx_info)
{
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_btree_idx_dest)

    HDassert(idx_info);
    HDassert(idx_info->f);
    HDassert(idx_info->pline);
    HDassert(idx_info->layout);
    HDassert(idx_info->storage);

    /* Free the raw B-tree node buffer */
    if(NULL == idx_info->storage->u.btree.shared)
        HGOTO_ERROR(H5E_IO, H5E_CANTFREE, FAIL, "ref-counted page nil")
    if(H5RC_DEC(idx_info->storage->u.btree.shared) < 0)
	HGOTO_ERROR(H5E_IO, H5E_CANTFREE, FAIL, "unable to decrement ref-counted page")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_idx_dest() */


/*-------------------------------------------------------------------------
 * Function:	H5D_btree_debug
 *
 * Purpose:	Debugs a B-tree node for indexed raw data storage.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, April 16, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_btree_debug(H5F_t *f, hid_t dxpl_id, haddr_t addr, FILE * stream, int indent,
		 int fwidth, unsigned ndims)
{
    H5D_btree_dbg_t     udata;          /* User data for B-tree callback */
    H5O_storage_chunk_t storage;        /* Storage information for B-tree callback */
    hbool_t     shared_init = FALSE;    /* Whether B-tree shared info is initialized */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5D_btree_debug, FAIL)

    /* Reset "fake" storage info */
    HDmemset(&storage, 0, sizeof(storage));
    storage.idx_type = H5D_CHUNK_BTREE;

    /* Allocate the shared structure */
    if(H5D_btree_shared_create(f, &storage, ndims) < 0)
	HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't create wrapper for shared B-tree info")
    shared_init = TRUE;

    /* Set up user data for callback */
    udata.common.layout = NULL;
    udata.common.storage = &storage;
    udata.common.offset = NULL;
    udata.ndims = ndims;

    /* Dump the records for the B-tree */
    (void)H5B_debug(f, dxpl_id, addr, stream, indent, fwidth, H5B_BTREE, &udata);

done:
    if(shared_init) {
        /* Free the raw B-tree node buffer */
        if(NULL == storage.u.btree.shared)
            HDONE_ERROR(H5E_IO, H5E_CANTFREE, FAIL, "ref-counted page nil")
        else
            if(H5RC_DEC(storage.u.btree.shared) < 0)
                HDONE_ERROR(H5E_IO, H5E_CANTFREE, FAIL, "unable to decrement ref-counted page")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_btree_debug() */

