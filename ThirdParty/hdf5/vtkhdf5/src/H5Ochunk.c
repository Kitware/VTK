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
 * Created:		H5Ochunk.c
 *			Jul 13 2008
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		Object header chunk routines.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5O_PACKAGE		/*suppress error about including H5Opkg	  */

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Opkg.h"             /* Object headers			*/


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

/* Declare the free list for H5O_chunk_proxy_t's */
H5FL_DEFINE(H5O_chunk_proxy_t);


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5O_chunk_add
 *
 * Purpose:	Add new chunk for object header to metadata cache
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jul 13 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_chunk_add(H5F_t *f, hid_t dxpl_id, H5O_t *oh, unsigned idx)
{
    H5O_chunk_proxy_t *chk_proxy = NULL;       /* Proxy for chunk, to mark it dirty in the cache */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5O_chunk_add, FAIL)

    /* check args */
    HDassert(f);
    HDassert(oh);
    HDassert(idx < oh->nchunks);
    HDassert(idx > 0);

    /* Allocate space for the object header data structure */
    if(NULL == (chk_proxy = H5FL_CALLOC(H5O_chunk_proxy_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Set the values in the chunk proxy */
    chk_proxy->oh = oh;
    chk_proxy->chunkno = idx;

    /* Increment reference count on object header */
    if(H5O_inc_rc(oh) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINC, FAIL, "can't increment reference count on object header")

    /* Insert the chunk proxy into the cache */
    if(H5AC_set(f, dxpl_id, H5AC_OHDR_CHK, oh->chunk[idx].addr, chk_proxy, H5AC__NO_FLAGS_SET) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "unable to cache object header chunk")
    chk_proxy = NULL;

done:
    if(ret_value < 0)
        if(chk_proxy)
            chk_proxy = H5FL_FREE(H5O_chunk_proxy_t, chk_proxy);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_chunk_add() */


/*-------------------------------------------------------------------------
 * Function:	H5O_chunk_protect
 *
 * Purpose:	Protect an object header chunk for modifications
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jul 17 2008
 *
 *-------------------------------------------------------------------------
 */
H5O_chunk_proxy_t *
H5O_chunk_protect(H5F_t *f, hid_t dxpl_id, H5O_t *oh, unsigned idx)
{
    H5O_chunk_proxy_t *chk_proxy;       /* Proxy for protected chunk */
    H5O_chunk_proxy_t *ret_value;       /* Return value */

    FUNC_ENTER_NOAPI(H5O_chunk_protect, NULL)

    /* check args */
    HDassert(f);
    HDassert(oh);
    HDassert(idx < oh->nchunks);

    /* Check for protecting first chunk */
    if(0 == idx) {
        /* Create new "fake" chunk proxy for first chunk */
        /* (since the first chunk is already handled by the H5O_t object) */
        if(NULL == (chk_proxy = H5FL_CALLOC(H5O_chunk_proxy_t)))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTALLOC, NULL, "memory allocation failed")

        /* Increment reference count on object header */
        if(H5O_inc_rc(oh) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTINC, NULL, "can't increment reference count on object header")

        /* Set chunk proxy fields */
        chk_proxy->oh = oh;
        chk_proxy->chunkno = idx;
    } /* end if */
    else {
        H5O_chk_cache_ud_t chk_udata;       /* User data for loading chunk */

        /* Construct the user data for protecting chunk proxy */
        /* (and _not_ decoding it) */
        HDmemset(&chk_udata, 0, sizeof(chk_udata));
        chk_udata.oh = oh;
        chk_udata.chunkno = idx;
        chk_udata.chunk_size = oh->chunk[idx].size;

        /* Get the chunk proxy */
        if(NULL == (chk_proxy = (H5O_chunk_proxy_t *)H5AC_protect(f, dxpl_id, H5AC_OHDR_CHK, oh->chunk[idx].addr, &chk_udata, H5AC_WRITE)))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, NULL, "unable to load object header chunk")

        /* Sanity check */
        HDassert(chk_proxy->oh == oh);
        HDassert(chk_proxy->chunkno == idx);
    } /* end else */

    /* Set return value */
    ret_value = chk_proxy;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_chunk_protect() */


/*-------------------------------------------------------------------------
 * Function:	H5O_chunk_unprotect
 *
 * Purpose:	Unprotect an object header chunk after modifications
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jul 17 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_chunk_unprotect(H5F_t *f, hid_t dxpl_id, H5O_chunk_proxy_t *chk_proxy,
    unsigned chk_flags)
{
    herr_t ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_NOAPI(H5O_chunk_unprotect, FAIL)

    /* check args */
    HDassert(f);
    HDassert(chk_proxy);
    HDassert(!(chk_flags & (unsigned)~(H5AC__DIRTIED_FLAG | H5AC__SIZE_CHANGED_FLAG)));

    /* Check for releasing first chunk */
    if(0 == chk_proxy->chunkno) {
        /* Check for resizing the first chunk */
        if(chk_flags & H5AC__SIZE_CHANGED_FLAG) {
            /* Resize object header in cache */
            if(H5AC_resize_entry(chk_proxy->oh, chk_proxy->oh->chunk[0].size) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTRESIZE, FAIL, "unable to resize chunk in cache")
        } /* end if */

        /* Check for dirtying the first chunk */
        if(chk_flags & H5AC__DIRTIED_FLAG) {
            /* Mark object header as dirty in cache */
            if(H5AC_mark_entry_dirty(chk_proxy->oh) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTMARKDIRTY, FAIL, "unable to mark object header as dirty")
        } /* end else/if */

        /* Decrement reference count of object header */
        if(H5O_dec_rc(chk_proxy->oh) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTDEC, FAIL, "can't decrement reference count on object header")

        /* Free fake chunk proxy */
        chk_proxy = H5FL_FREE(H5O_chunk_proxy_t, chk_proxy);
    } /* end if */
    else {
        /* Release the chunk proxy from the cache, marking it dirty */
        if(H5AC_unprotect(f, dxpl_id, H5AC_OHDR_CHK, chk_proxy->oh->chunk[chk_proxy->chunkno].addr, chk_proxy, chk_flags) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header chunk")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_chunk_unprotect() */


/*-------------------------------------------------------------------------
 * Function:	H5O_chunk_update_idx
 *
 * Purpose:	Update the chunk index for a chunk proxy
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jul 13 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_chunk_update_idx(H5F_t *f, hid_t dxpl_id, H5O_t *oh, unsigned idx)
{
    H5O_chunk_proxy_t *chk_proxy;       /* Proxy for chunk, to mark it dirty in the cache */
    H5O_chk_cache_ud_t chk_udata;       /* User data for loading chunk */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5O_chunk_update_idx, FAIL)

    /* check args */
    HDassert(f);
    HDassert(oh);
    HDassert(idx < oh->nchunks);
    HDassert(idx > 0);

    /* Construct the user data for protecting chunk proxy */
    /* (and _not_ decoding it) */
    HDmemset(&chk_udata, 0, sizeof(chk_udata));
    chk_udata.oh = oh;
    chk_udata.chunkno = idx;
    chk_udata.chunk_size = oh->chunk[idx].size;

    /* Get the chunk proxy */
    if(NULL == (chk_proxy = (H5O_chunk_proxy_t *)H5AC_protect(f, dxpl_id, H5AC_OHDR_CHK, oh->chunk[idx].addr, &chk_udata, H5AC_WRITE)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header chunk")

    /* Update index for chunk proxy in cache */
    chk_proxy->chunkno = idx;

    /* Release the chunk proxy from the cache, marking it deleted */
    if(H5AC_unprotect(f, dxpl_id, H5AC_OHDR_CHK, oh->chunk[idx].addr, chk_proxy, H5AC__DIRTIED_FLAG) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header chunk")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_chunk_update_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5O_chunk_delete
 *
 * Purpose:	Notify metadata cache that a chunk has been deleted
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jul 13 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_chunk_delete(H5F_t *f, hid_t dxpl_id, H5O_t *oh, unsigned idx)
{
    H5O_chunk_proxy_t *chk_proxy;       /* Proxy for chunk, to mark it dirty in the cache */
    H5O_chk_cache_ud_t chk_udata;       /* User data for loading chunk */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5O_chunk_delete, FAIL)

    /* check args */
    HDassert(f);
    HDassert(oh);
    HDassert(idx < oh->nchunks);
    HDassert(idx > 0);

    /* Construct the user data for protecting chunk proxy */
    /* (and _not_ decoding it) */
    HDmemset(&chk_udata, 0, sizeof(chk_udata));
    chk_udata.oh = oh;
    chk_udata.chunkno = idx;
    chk_udata.chunk_size = oh->chunk[idx].size;

    /* Get the chunk proxy */
    if(NULL == (chk_proxy = (H5O_chunk_proxy_t *)H5AC_protect(f, dxpl_id, H5AC_OHDR_CHK, oh->chunk[idx].addr, &chk_udata, H5AC_WRITE)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header chunk")

    /* Sanity check */
    HDassert(chk_proxy->oh == oh);
    HDassert(chk_proxy->chunkno == idx);

    /* Release the chunk proxy from the cache, marking it deleted */
    if(H5AC_unprotect(f, dxpl_id, H5AC_OHDR_CHK, oh->chunk[idx].addr, chk_proxy, (H5AC__DIRTIED_FLAG | H5AC__DELETED_FLAG | H5AC__FREE_FILE_SPACE_FLAG)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header chunk")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_chunk_delete() */

