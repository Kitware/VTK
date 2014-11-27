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
 * Created:		H5Oalloc.c
 *			Nov 17 2006
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		Object header allocation routines.
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
#include "H5FLprivate.h"	/* Free lists                           */
#include "H5MFprivate.h"	/* File memory management		*/
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

static herr_t H5O_add_gap(H5F_t *f, H5O_t *oh, unsigned chunkno,
    hbool_t *chk_dirtied, size_t idx, uint8_t *new_gap_loc, size_t new_gap_size);
static herr_t H5O_eliminate_gap(H5O_t *oh, hbool_t *chk_dirtied,
    H5O_mesg_t *mesg, uint8_t *new_gap_loc, size_t new_gap_size);
static herr_t H5O_alloc_null(H5F_t *f, hid_t dxpl_id, H5O_t *oh, size_t null_idx,
    const H5O_msg_class_t *new_type, void *new_native, size_t new_size);
static htri_t H5O_alloc_extend_chunk(H5F_t *f, hid_t dxpl_id, H5O_t *oh,
    unsigned chunkno, size_t size, size_t *msg_idx);
static herr_t H5O_alloc_new_chunk(H5F_t *f, hid_t dxpl_id, H5O_t *oh, size_t size,
    size_t *new_idx);
static htri_t H5O_move_cont(H5F_t *f, hid_t dxpl_id, H5O_t *oh, unsigned cont_u);
static htri_t H5O_move_msgs_forward(H5F_t *f, hid_t dxpl_id, H5O_t *oh);
static htri_t H5O_merge_null(H5F_t *f, hid_t dxpl_id, H5O_t *oh);
static htri_t H5O_remove_empty_chunks(H5F_t *f, hid_t dxpl_id, H5O_t *oh);
static herr_t H5O_alloc_shrink_chunk(H5F_t *f, hid_t dxpl_id, H5O_t *oh,
    unsigned chunkno);


/*********************/
/* Package Variables */
/*********************/

/* Declare extern the free list for H5O_cont_t's */
H5FL_EXTERN(H5O_cont_t);


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:    H5O_add_gap
 *
 * Purpose:     Add a gap to a chunk
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              koziol@hdfgroup.org
 *              Oct 17 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_add_gap(H5F_t *f, H5O_t *oh, unsigned chunkno, hbool_t *chk_dirtied,
    size_t idx, uint8_t *new_gap_loc, size_t new_gap_size)
{
    hbool_t merged_with_null;           /* Whether the gap was merged with a null message */
    size_t u;                           /* Local index variable */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(oh);
    HDassert(oh->version > H5O_VERSION_1);
    HDassert(chk_dirtied);
    HDassert(new_gap_loc);
    HDassert(new_gap_size);

#ifndef NDEBUG
if(chunkno > 0) {
    unsigned chk_proxy_status = 0;         /* Object header chunk proxy entry cache status */

    /* Check the object header chunk proxy's status in the metadata cache */
    if(H5AC_get_entry_status(f, oh->chunk[chunkno].addr, &chk_proxy_status) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to check metadata cache status for object header chunk proxy")

    /* Make certain that object header is protected */
    HDassert(chk_proxy_status & H5AC_ES__IS_PROTECTED);
} /* end if */
#endif /* NDEBUG */

    /* Check for existing null message in chunk */
    merged_with_null = FALSE;
    for(u = 0; u < oh->nmesgs && !merged_with_null; u++) {
        /* Find a null message in the chunk with the new gap */
        /* (a null message that's not the one we are eliminating) */
        if(H5O_NULL_ID == oh->mesg[u].type->id && oh->mesg[u].chunkno == chunkno
                && u != idx) {
            /* Sanity check - chunks with null messages shouldn't have a gap */
            HDassert(oh->chunk[chunkno].gap == 0);

            /* Eliminate the gap in the chunk */
            if(H5O_eliminate_gap(oh, chk_dirtied, &oh->mesg[u], new_gap_loc, new_gap_size) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "can't eliminate gap in chunk")

            /* Set flag to indicate that the gap was handled */
            merged_with_null = TRUE;
        } /* end if */
    } /* end for */

    /* If we couldn't find a null message in the chunk, move the gap to the end */
    if(!merged_with_null) {
        /* Adjust message offsets after new gap forward in chunk */
        for(u = 0; u < oh->nmesgs; u++)
            if(oh->mesg[u].chunkno == chunkno && oh->mesg[u].raw > new_gap_loc)
                oh->mesg[u].raw -= new_gap_size;

        /* Slide raw message info forward in chunk image */
        HDmemmove(new_gap_loc, new_gap_loc + new_gap_size,
                (size_t)((oh->chunk[chunkno].image + (oh->chunk[chunkno].size - H5O_SIZEOF_CHKSUM_OH(oh))) - (new_gap_loc + new_gap_size)));

        /* Add existing gap size to new gap size */
        new_gap_size += oh->chunk[chunkno].gap;

        /* Merging with existing gap will allow for a new null message */
        if(new_gap_size >= (size_t)H5O_SIZEOF_MSGHDR_OH(oh)) {
            H5O_mesg_t *null_msg;       /* Pointer to new null message */

            /* Check if we need to extend message table to hold the new null message */
            if(oh->nmesgs >= oh->alloc_nmesgs)
                if(H5O_alloc_msgs(oh, (size_t)1) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate more space for messages")

            /* Increment new gap size */
            oh->chunk[chunkno].gap += new_gap_size;

            /* Create new null message, with the tail of the previous null message */
            null_msg = &(oh->mesg[oh->nmesgs++]);
            null_msg->type = H5O_MSG_NULL;
            null_msg->native = NULL;
            null_msg->raw_size = new_gap_size - (size_t)H5O_SIZEOF_MSGHDR_OH(oh);
            null_msg->raw = (oh->chunk[chunkno].image + oh->chunk[chunkno].size)
                    - (H5O_SIZEOF_CHKSUM_OH(oh) + null_msg->raw_size);
            null_msg->chunkno = chunkno;

            /* Zero out new null message's raw data */
            if(null_msg->raw_size)
                HDmemset(null_msg->raw, 0, null_msg->raw_size);

            /* Mark message as dirty */
            null_msg->dirty = TRUE;

            /* Reset size of gap in chunk */
            oh->chunk[chunkno].gap = 0;
        } /* end if */
        else
            oh->chunk[chunkno].gap = new_gap_size;

        /* Mark the chunk as modified */
        *chk_dirtied = TRUE;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_add_gap() */


/*-------------------------------------------------------------------------
 * Function:    H5O_eliminate_gap
 *
 * Purpose:     Eliminate a gap in a chunk with a null message.
 *
 * Note:        Sometimes this happens as a result of converting an existing
 *              non-null message to a null message, so we zero out the gap
 *              here, even though it might already be zero (when we're adding
 *              a gap to a chunk with an existing null message).  (Mostly,
 *              this just simplifies the code, esp. with the necessary chunk
 *              locking -QAK)
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              koziol@hdfgroup.org
 *              Oct 17 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_eliminate_gap(H5O_t *oh, hbool_t *chk_dirtied, H5O_mesg_t *mesg,
    uint8_t *gap_loc, size_t gap_size)
{
    uint8_t *move_start, *move_end;     /* Pointers to area of messages to move */
    hbool_t null_before_gap;            /* Flag whether the null message is before the gap or not */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(oh);
    HDassert(oh->version > H5O_VERSION_1);
    HDassert(chk_dirtied);
    HDassert(mesg);
    HDassert(gap_loc);
    HDassert(gap_size);

    /* Check if the null message is before or after the gap produced */
    null_before_gap = (hbool_t)(mesg->raw < gap_loc);

    /* Set up information about region of messages to move */
    if(null_before_gap) {
        move_start = mesg->raw + mesg->raw_size;
        move_end = gap_loc;
    } /* end if */
    else {
        move_start = gap_loc + gap_size;
        move_end = mesg->raw - H5O_SIZEOF_MSGHDR_OH(oh);
    } /* end else */

    /* Check for messages between null message and gap */
    if(move_end > move_start) {
        unsigned u;                 /* Local index variable */

        /* Look for messages that need to move, to adjust raw pointers in chunk */
        /* (this doesn't change the moved messages 'dirty' state) */
        for(u = 0; u < oh->nmesgs; u++) {
            uint8_t *msg_start;     /* Start of encoded message in chunk */

            msg_start = oh->mesg[u].raw - H5O_SIZEOF_MSGHDR_OH(oh);
            if(oh->mesg[u].chunkno == mesg->chunkno
                    && (msg_start >= move_start && msg_start < move_end)) {
                /* Move message's raw pointer in appropriate direction */
                if(null_before_gap)
                    oh->mesg[u].raw += gap_size;
                else
                    oh->mesg[u].raw -= gap_size;
            } /* end if */
        } /* end for */

        /* Slide raw message info in chunk image */
        if(null_before_gap)
            /* Slide messages down */
            HDmemmove(move_start + gap_size, move_start, (size_t)(move_end - move_start));
        else {
            /* Slide messages up */
            HDmemmove(move_start - gap_size, move_start, (size_t)(move_end - move_start));

            /* Adjust start of null message */
            mesg->raw -= gap_size;
        } /* end else */
    }
    else if(move_end == move_start && !null_before_gap) {
	/* Slide null message up */
	HDmemmove(move_start - gap_size, move_start, mesg->raw_size + (size_t)H5O_SIZEOF_MSGHDR_OH(oh));

	/* Adjust start of null message */
	mesg->raw -= gap_size;
    } /* end if */

    /* Zero out addition to null message */
    HDmemset(mesg->raw + mesg->raw_size, 0, gap_size);

    /* Adjust size of null message */
    mesg->raw_size += gap_size;

    /* Set the gap size to zero for the chunk */
    oh->chunk[mesg->chunkno].gap = 0;

    /* Mark null message as dirty */
    mesg->dirty = TRUE;
    *chk_dirtied = TRUE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5O_eliminate_gap() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5O_alloc_null
 *
 * Purpose:     Allocate room for a new message from a null message
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct 22 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_alloc_null(H5F_t *f, hid_t dxpl_id, H5O_t *oh, size_t null_idx,
    const H5O_msg_class_t *new_type, void *new_native, size_t new_size)
{
    H5O_chunk_proxy_t *chk_proxy = NULL;        /* Chunk that message is in */
    hbool_t chk_dirtied = FALSE;        /* Flags for unprotecting chunk */
    H5O_mesg_t *alloc_msg;              /* Pointer to null message to allocate out of */
    herr_t ret_value = SUCCEED; 	/* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(oh);
    HDassert(new_type);
    HDassert(new_size);

    /* Point to null message to allocate out of */
    alloc_msg = &oh->mesg[null_idx];

    /* Protect chunk */
    if(NULL == (chk_proxy = H5O_chunk_protect(f, dxpl_id, oh, alloc_msg->chunkno)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header chunk")

    /* Check if there's a need to split the null message */
    if(alloc_msg->raw_size > new_size) {
        /* Check for producing a gap in the chunk */
        if((alloc_msg->raw_size - new_size) < (size_t)H5O_SIZEOF_MSGHDR_OH(oh)) {
            size_t gap_size = alloc_msg->raw_size - new_size;     /* Size of gap produced */

            /* Adjust the size of the null message being eliminated */
            alloc_msg->raw_size = new_size;

            /* Add the gap to the chunk */
            if(H5O_add_gap(f, oh, alloc_msg->chunkno, &chk_dirtied, null_idx, alloc_msg->raw + alloc_msg->raw_size, gap_size) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "can't insert gap in chunk")
        } /* end if */
        else {
            size_t  new_mesg_size = new_size + (size_t)H5O_SIZEOF_MSGHDR_OH(oh); /* Total size of newly allocated message */
            H5O_mesg_t *null_msg;       /* Pointer to new null message */

            /* Check if we need to extend message table to hold the new null message */
            if(oh->nmesgs >= oh->alloc_nmesgs) {
                if(H5O_alloc_msgs(oh, (size_t)1) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate more space for messages")

                /* "Retarget" 'alloc_msg' pointer into newly re-allocated array of messages */
                alloc_msg = &oh->mesg[null_idx];
            } /* end if */

            /* Create new null message, with the tail of the previous null message */
            null_msg = &(oh->mesg[oh->nmesgs++]);
            null_msg->type = H5O_MSG_NULL;
            null_msg->native = NULL;
            null_msg->raw = alloc_msg->raw + new_mesg_size;
            null_msg->raw_size = alloc_msg->raw_size - new_mesg_size;
            null_msg->chunkno = alloc_msg->chunkno;

            /* Mark the message as dirty */
            null_msg->dirty = TRUE;
            chk_dirtied = TRUE;

            /* Check for gap in new null message's chunk */
            if(oh->chunk[null_msg->chunkno].gap > 0) {
                unsigned null_chunkno = null_msg->chunkno;   /* Chunk w/gap */

                /* Eliminate the gap in the chunk */
                if(H5O_eliminate_gap(oh, &chk_dirtied, null_msg,
                        ((oh->chunk[null_chunkno].image + oh->chunk[null_chunkno].size) - (H5O_SIZEOF_CHKSUM_OH(oh) + oh->chunk[null_chunkno].gap)),
                        oh->chunk[null_chunkno].gap) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTREMOVE, FAIL, "can't eliminate gap in chunk")
            } /* end if */

            /* Set the size of the new "real" message */
            alloc_msg->raw_size = new_size;
        } /* end else */
    } /* end if */

    /* Initialize the new message */
    alloc_msg->type = new_type;
    alloc_msg->native = new_native;

    /* Mark the new message as dirty */
    alloc_msg->dirty = TRUE;
    chk_dirtied = TRUE;

done:
    /* Release chunk */
    if(chk_proxy && H5O_chunk_unprotect(f, dxpl_id, chk_proxy, chk_dirtied) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_alloc_null() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5O_alloc_msgs
 *
 * Purpose:     Allocate more messages for a header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Nov 21 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_alloc_msgs(H5O_t *oh, size_t min_alloc)
{
    size_t old_alloc;                   /* Old number of messages allocated */
    size_t na;                          /* New number of messages allocated */
    H5O_mesg_t *new_mesg;               /* Pointer to new message array */
    herr_t ret_value = SUCCEED; 	/* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(oh);

    /* Initialize number of messages information */
    old_alloc = oh->alloc_nmesgs;
    na = oh->alloc_nmesgs + MAX(oh->alloc_nmesgs, min_alloc);   /* At least double */

    /* Attempt to allocate more memory */
    if(NULL == (new_mesg = H5FL_SEQ_REALLOC(H5O_mesg_t, oh->mesg, na)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Update ohdr information */
    oh->alloc_nmesgs = na;
    oh->mesg = new_mesg;

    /* Set new object header info to zeros */
    HDmemset(&oh->mesg[old_alloc], 0, (oh->alloc_nmesgs - old_alloc) * sizeof(H5O_mesg_t));

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_alloc_msgs() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5O_alloc_extend_chunk
 *
 * Purpose:     Attempt to extend a chunk that is allocated on disk.
 *
 *              If the extension is successful, and if the last message
 *		of the chunk is the null message, then that message will
 *		be extended with the chunk.  Otherwise a new null message
 *		is created.
 *
 *              f is the file in which the chunk will be written.  It is
 *              included to ensure that there is enough space to extend
 *              this chunk.
 *
 * Return:      TRUE:		The chunk has been extended, and *msg_idx
 *				contains the message index for null message
 *				which is large enough to hold size bytes.
 *
 *		FALSE:		The chunk cannot be extended, and *msg_idx
 *				is undefined.
 *
 *		FAIL:		Some internal error has been detected.
 *
 * Programmer:  John Mainzer -- 8/16/05
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5O_alloc_extend_chunk(H5F_t *f, hid_t dxpl_id, H5O_t *oh, unsigned chunkno,
    size_t size, size_t *msg_idx)
{
    H5O_chunk_proxy_t *chk_proxy = NULL;    /* Chunk that message is in */
    hbool_t     chk_dirtied = FALSE;        /* Flag for unprotecting chunk */
    size_t      delta;          /* Change in chunk's size */
    size_t      aligned_size = H5O_ALIGN_OH(oh, size);
    uint8_t     *old_image;     /* Old address of chunk's image in memory */
    size_t      old_size;       /* Old size of chunk */
    htri_t      extended;       /* If chunk can be extended */
    size_t      extend_msg = 0;     /* Index of null message to extend */
    hbool_t     extended_msg = FALSE;   /* Whether an existing message was extended */
    uint8_t     new_size_flags = 0;     /* New chunk #0 size flags */
    hbool_t     adjust_size_flags = FALSE;      /* Whether to adjust the chunk #0 size flags */
    size_t      extra_prfx_size = 0; /* Extra bytes added to object header prefix */
    size_t      u;              /* Local index variable */
    htri_t      ret_value = TRUE; 	/* return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f != NULL);
    HDassert(oh != NULL);
    HDassert(chunkno < oh->nchunks);
    HDassert(size > 0);
    HDassert(msg_idx != NULL);
    HDassert(H5F_addr_defined(oh->chunk[chunkno].addr));

    /* Test to see if the specified chunk ends with a null messages.
     * If successful, set the index of the the null message in extend_msg.
     */
    for(u = 0; u < oh->nmesgs; u++) {
        /* Check for null message at end of proper chunk */
        /* (account for possible checksum at end of chunk) */
        if(oh->mesg[u].chunkno == chunkno && H5O_NULL_ID == oh->mesg[u].type->id &&
                ((oh->mesg[u].raw + oh->mesg[u].raw_size)
                == ((oh->chunk[chunkno].image + oh->chunk[chunkno].size) -
                    (oh->chunk[chunkno].gap + H5O_SIZEOF_CHKSUM_OH(oh))))) {

            extend_msg = u;
            extended_msg = TRUE;
            break;
        } /* end if */
    } /* end for */

    /* If we can extend an existing null message, adjust the delta appropriately */
    if(extended_msg) {
        HDassert(oh->chunk[chunkno].gap == 0);
        delta = aligned_size - oh->mesg[extend_msg].raw_size;
    } /* end if */
    else
        delta = (aligned_size + (size_t)H5O_SIZEOF_MSGHDR_OH(oh)) - oh->chunk[chunkno].gap;
    delta = H5O_ALIGN_OH(oh, delta);

    /* Check for changing the chunk #0 data size enough to need adjusting the flags */
    if(oh->version > H5O_VERSION_1 && chunkno == 0) {
        uint64_t chunk0_size;           /* Size of chunk 0's data */
        size_t   orig_prfx_size = (size_t)1 << (oh->flags & H5O_HDR_CHUNK0_SIZE); /* Original prefix size */

        HDassert(oh->chunk[0].size >= (size_t)H5O_SIZEOF_HDR(oh));
        chunk0_size = oh->chunk[0].size - (size_t)H5O_SIZEOF_HDR(oh);

        /* Check for moving to a 8-byte size encoding */
        if(orig_prfx_size < 8 && (chunk0_size + delta) > 4294967295) {
            extra_prfx_size = 8 - orig_prfx_size;
            new_size_flags = H5O_HDR_CHUNK0_8;
            adjust_size_flags = TRUE;
        } /* end if */
        /* Check for moving to a 4-byte size encoding */
        else if(orig_prfx_size < 4 && (chunk0_size + delta) > 65535) {
            extra_prfx_size = 4 - orig_prfx_size;
            new_size_flags = H5O_HDR_CHUNK0_4;
            adjust_size_flags = TRUE;
        } /* end if */
        /* Check for moving to a 2-byte size encoding */
        else if(orig_prfx_size < 2 && (chunk0_size + delta) > 255) {
            extra_prfx_size = 2 - orig_prfx_size;
            new_size_flags = H5O_HDR_CHUNK0_2;
            adjust_size_flags = TRUE;
        } /* end if */
    } /* end if */

    /* Protect chunk */
    if(NULL == (chk_proxy = H5O_chunk_protect(f, dxpl_id, oh, chunkno)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header chunk")

    /* Determine whether the chunk can be extended */
    extended = H5MF_try_extend(f, dxpl_id, H5FD_MEM_OHDR, oh->chunk[chunkno].addr,
                                 (hsize_t)(oh->chunk[chunkno].size), (hsize_t)(delta + extra_prfx_size));
    if(extended < 0) /* error */
        HGOTO_ERROR(H5E_OHDR, H5E_CANTEXTEND, FAIL, "can't tell if we can extend chunk")
    else if(extended == FALSE)     /* can't extend -- we are done */
        HGOTO_DONE(FALSE)

    /* Adjust object header prefix flags */
    if(adjust_size_flags) {
        oh->flags = (uint8_t)(oh->flags & ~H5O_HDR_CHUNK0_SIZE);
        oh->flags |= new_size_flags;

        /* Mark object header as dirty in cache */
        if(H5AC_mark_entry_dirty(oh) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTMARKDIRTY, FAIL, "unable to mark object header as dirty")
    } /* end if */

    /* If we can extend an existing null message, take care of that */
    if(extended_msg) {
        /* Adjust message size of existing null message */
        oh->mesg[extend_msg].raw_size += delta;
    } /* end if */
    /* Create new null message for end of chunk */
    else {
        /* Create a new null message */
        if(oh->nmesgs >= oh->alloc_nmesgs)
            if(H5O_alloc_msgs(oh, (size_t)1) < 0)
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate more space for messages")

        /* Set extension message */
        extend_msg = oh->nmesgs++;

        /* Initialize new null message */
        oh->mesg[extend_msg].type = H5O_MSG_NULL;
        oh->mesg[extend_msg].native = NULL;
        oh->mesg[extend_msg].raw = ((oh->chunk[chunkno].image + oh->chunk[chunkno].size)
                - (H5O_SIZEOF_CHKSUM_OH(oh) + oh->chunk[chunkno].gap))
                + H5O_SIZEOF_MSGHDR_OH(oh);
        oh->mesg[extend_msg].raw_size = (delta + oh->chunk[chunkno].gap) - (size_t)H5O_SIZEOF_MSGHDR_OH(oh);
        oh->mesg[extend_msg].chunkno = chunkno;
    } /* end else */

    /* Mark the extended message as dirty */
    oh->mesg[extend_msg].dirty = TRUE;
    chk_dirtied = TRUE;

    /* Allocate more memory space for chunk's image */
    old_image = oh->chunk[chunkno].image;
    old_size = oh->chunk[chunkno].size;
    oh->chunk[chunkno].size += delta + extra_prfx_size;
    oh->chunk[chunkno].image = H5FL_BLK_REALLOC(chunk_image, old_image, oh->chunk[chunkno].size);
    oh->chunk[chunkno].gap = 0;
    if(NULL == oh->chunk[chunkno].image)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Wipe new space for chunk */
    HDmemset(oh->chunk[chunkno].image + old_size, 0, oh->chunk[chunkno].size - old_size);

    /* Move chunk 0 data up if the size flags changed */
    if(adjust_size_flags)
        HDmemmove(oh->chunk[0].image + H5O_SIZEOF_HDR(oh) - H5O_SIZEOF_CHKSUM_OH(oh),
                oh->chunk[0].image + H5O_SIZEOF_HDR(oh) - H5O_SIZEOF_CHKSUM_OH(oh) - extra_prfx_size,
                old_size - (size_t)H5O_SIZEOF_HDR(oh) + extra_prfx_size);

    /* Spin through existing messages, adjusting them */
    for(u = 0; u < oh->nmesgs; u++) {
        /* Adjust raw addresses for messages in this chunk to reflect new 'image' address */
        if(oh->mesg[u].chunkno == chunkno)
            oh->mesg[u].raw = oh->chunk[chunkno].image + extra_prfx_size + (oh->mesg[u].raw - old_image);

        /* Find continuation message which points to this chunk and adjust chunk's size */
        /* (Chunk 0 doesn't have a continuation message that points to it and
         * it's size is directly encoded in the object header) */
        if(chunkno > 0 && (H5O_CONT_ID == oh->mesg[u].type->id) &&
                (((H5O_cont_t *)(oh->mesg[u].native))->chunkno == chunkno)) {
            H5O_chunk_proxy_t *chk_proxy2 = NULL;       /* Chunk that continuation message is in */
            hbool_t chk_dirtied2 = FALSE;               /* Flag for unprotecting chunk */
            unsigned cont_chunkno = oh->mesg[u].chunkno;    /* Chunk # for continuation message */

            /* Protect chunk containing continuation message */
            if(NULL == (chk_proxy2 = H5O_chunk_protect(f, dxpl_id, oh, cont_chunkno)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header chunk")

            /* Adjust size in continuation message */
            HDassert(((H5O_cont_t *)(oh->mesg[u].native))->size == old_size);
            ((H5O_cont_t *)(oh->mesg[u].native))->size = oh->chunk[chunkno].size;

            /* Flag continuation message as dirty */
            oh->mesg[u].dirty = TRUE;
            chk_dirtied2 = TRUE;

            /* Release chunk containing continuation message */
            if(H5O_chunk_unprotect(f, dxpl_id, chk_proxy2, chk_dirtied2) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")
        } /* end if */
    } /* end for */

    /* Resize the chunk in the cache */
    if(H5O_chunk_resize(oh, chk_proxy) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTRESIZE, FAIL, "unable to resize object header chunk")

    /* Set new message index */
    *msg_idx = extend_msg;

done:
    /* Release chunk */
    if(chk_proxy && H5O_chunk_unprotect(f, dxpl_id, chk_proxy, chk_dirtied) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_alloc_extend_chunk() */


/*-------------------------------------------------------------------------
 * Function:    H5O_alloc_new_chunk
 *
 * Purpose:     Allocates a new chunk for the object header, including
 *		file space.
 *
 *              One of the other chunks will get an object continuation
 *		message.  If there isn't room in any other chunk for the
 *		object continuation message, then some message from
 *		another chunk is moved into this chunk to make room.
 *
 *              SIZE need not be aligned.
 *
 * Note:	The algorithm for finding a message to replace with a
 *		continuation message is still fairly limited.  It's possible
 *		that two (or more) messages smaller than a continuation message
 *		might occupy a chunk and need to be moved in order to make
 *		room for the continuation message.
 *
 *		Also, we aren't checking for NULL messages in front of another
 *		message right now...
 *
 * Return:      Success:        Index number of the null message for the
 *                              new chunk.  The null message will be at
 *                              least SIZE bytes not counting the message
 *                              ID or size fields.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  7 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_alloc_new_chunk(H5F_t *f, hid_t dxpl_id, H5O_t *oh, size_t size, size_t *new_idx)
{
    /* Struct for storing information about "best" messages to allocate from */
    typedef struct {
        int msgno;                      /* Index in message array */
        size_t gap_size;                /* Size of any "gap" in the chunk immediately after message */
        size_t null_size;               /* Size of any null message in the chunk immediately after message */
        size_t total_size;              /* Total size of "available" space around message */
        unsigned null_msgno;            /* Message index of null message immediately after message */
    } alloc_info;

    H5O_mesg_t *curr_msg;               /* Pointer to current message to operate on */
    H5O_chunk_proxy_t *chk_proxy;       /* Chunk that message is in */
    size_t      cont_size;              /*continuation message size     */
    size_t      multi_size = 0;         /* Size of all the messages in the last chunk */
    int         found_null = (-1);      /* Best fit null message         */
    alloc_info  found_attr = {-1, 0, 0, 0, 0};      /* Best fit attribute message    */
    alloc_info  found_other = {-1, 0, 0, 0, 0};     /* Best fit other message        */
    size_t      idx;                    /* Message number */
    uint8_t     *p = NULL;              /*ptr into new chunk            */
    H5O_cont_t  *cont = NULL;           /*native continuation message   */
    unsigned    chunkno;                /* Chunk allocated */
    haddr_t	new_chunk_addr;
    unsigned    u;                      /* Local index variable */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(oh);
    HDassert(size > 0);
    size = H5O_ALIGN_OH(oh, size);

    /*
     * Find the smallest null message that will hold an object
     * continuation message.  Failing that, find the smallest message
     * that could be moved to make room for the continuation message.
     *
     * Don't ever move continuation message from one chunk to another.
     *
     * Avoid moving attributes when possible to preserve their
     * ordering (although ordering is *not* guaranteed!).
     *
     */
    cont_size = H5O_ALIGN_OH(oh, (size_t)(H5F_SIZEOF_ADDR(f) + H5F_SIZEOF_SIZE(f)));
    for(u = 0, curr_msg = &oh->mesg[0]; u < oh->nmesgs; u++, curr_msg++) {
        if(curr_msg->type->id == H5O_NULL_ID) {
            if(cont_size == curr_msg->raw_size) {
                found_null = (int)u;
                break;
            }  /* end if */
            else if(curr_msg->raw_size > cont_size &&
                    (found_null < 0 || curr_msg->raw_size < oh->mesg[found_null].raw_size))
                found_null = (int)u;
        } /* end if */
        else if(curr_msg->type->id == H5O_CONT_ID) {
            /* Don't consider continuation messages (for now) */
        } /* end if */
        else if(curr_msg->locked) {
            /* Don't consider locked messages */
        } /* end if */
        else {
            unsigned msg_chunkno = curr_msg->chunkno;         /* Chunk that the message is in */
            uint8_t *end_chunk_data = (oh->chunk[msg_chunkno].image + oh->chunk[msg_chunkno].size) - (H5O_SIZEOF_CHKSUM_OH(oh) + oh->chunk[msg_chunkno].gap);     /* End of message data in chunk */
            uint8_t *end_msg = curr_msg->raw + curr_msg->raw_size;  /* End of current message */
            size_t gap_size = 0;            /* Size of gap after current message */
            size_t null_size = 0;           /* Size of NULL message after current message */
            unsigned null_msgno = 0;        /* Index of NULL message after current message */
            size_t total_size;              /* Total size of available space "around" current message */

            /* Check if the message is the last one in the chunk */
            if(end_msg == end_chunk_data)
                gap_size = oh->chunk[msg_chunkno].gap;
            else {
                H5O_mesg_t *tmp_msg;    /* Temp. pointer to message to operate on */
                unsigned v;             /* Local index variable */

                /* Check for null message after this message, in same chunk */
                for(v = 0, tmp_msg = &oh->mesg[0]; v < oh->nmesgs; v++, tmp_msg++) {
                    if(tmp_msg->type->id == H5O_NULL_ID && (tmp_msg->raw - H5O_SIZEOF_MSGHDR_OH(oh)) == end_msg) {
                        null_msgno = v;
                        null_size = (size_t)H5O_SIZEOF_MSGHDR_OH(oh) + tmp_msg->raw_size;
                        break;
                    } /* end if */

                    /* XXX: Should also check for NULL message in front of current message... */

                } /* end for */
            } /* end else */

            /* Add up current message's total available space */
            total_size = curr_msg->raw_size + gap_size + null_size;

            /* Check if message is large enough to hold continuation info */
            if(total_size >= cont_size) {
                if(curr_msg->type->id == H5O_ATTR_ID) {
                    if(found_attr.msgno < 0 || total_size < found_attr.total_size) {
                        found_attr.msgno = (int)u;
                        found_attr.gap_size = gap_size;
                        found_attr.null_size = null_size;
                        found_attr.total_size = total_size;
                        found_attr.null_msgno = null_msgno;
                    } /* end if */
                } /* end if */
                else {
                    if(found_other.msgno < 0 || total_size < found_other.total_size) {
                        found_other.msgno = (int)u;
                        found_other.gap_size = gap_size;
                        found_other.null_size = null_size;
                        found_other.total_size = total_size;
                        found_other.null_msgno = null_msgno;
                    } /* end if */
                } /* end else */
            } /* end if */
            else if(found_null < 0 && found_attr.msgno < 0 && found_other.msgno < 0 && msg_chunkno == oh->nchunks - 1)
                /* Keep track of the total size of smaller messages in the last
                 * chunk, in case we need to move more than 1 message.
                 */
                multi_size += curr_msg->raw_size + (size_t)H5O_SIZEOF_MSGHDR_OH(oh);
        } /* end else */
    } /* end for */
    if(found_null >= 0 || found_attr.msgno >= 0 || found_other.msgno >= 0)
        multi_size = 0;

    /*
     * If we must move some other message to make room for the null
     * message, then make sure the new chunk has enough room for that
     * other message.
     *
     * Move other messages first, and attributes only as a last resort.
     *
     * If all else fails, move every message in the last chunk.
     *
     */
    if(multi_size == 0) {
        if(found_null < 0) {
            if(found_other.msgno < 0)
                found_other = found_attr;

            HDassert(found_other.msgno >= 0);
            size += (size_t)H5O_SIZEOF_MSGHDR_OH(oh) + oh->mesg[found_other.msgno].raw_size;
        } /* end if */
    } /* end if */
    else
        size += multi_size;

    /*
     * The total chunk size must include the requested space plus enough
     * for the message header.  This must be at least some minimum and
     * aligned propertly.
     */
    size = MAX(H5O_MIN_SIZE, size + (size_t)H5O_SIZEOF_MSGHDR_OH(oh));
    HDassert(size == H5O_ALIGN_OH(oh, size));

    /*
     * The total chunk size must include enough space for the checksum
     * on the chunk and the continuation chunk magic #. (which are only present
     * in later versions of the object header)
     */
    size +=  H5O_SIZEOF_CHKHDR_OH(oh);

    /* allocate space in file to hold the new chunk */
    new_chunk_addr = H5MF_alloc(f, H5FD_MEM_OHDR, dxpl_id, (hsize_t)size);
    if(HADDR_UNDEF == new_chunk_addr)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate space for new chunk")

    /*
     * Create the new chunk giving it a file address.
     */
    if(oh->nchunks >= oh->alloc_nchunks) {
        size_t na = MAX(H5O_NCHUNKS, oh->alloc_nchunks * 2);        /* Double # of chunks allocated */
        H5O_chunk_t *x;

        if(NULL == (x = H5FL_SEQ_REALLOC(H5O_chunk_t, oh->chunk, na)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
        oh->alloc_nchunks = na;
        oh->chunk = x;
    } /* end if */

    chunkno = oh->nchunks++;
    oh->chunk[chunkno].addr = new_chunk_addr;
    oh->chunk[chunkno].size = size;
    oh->chunk[chunkno].gap = 0;
    if(NULL == (oh->chunk[chunkno].image = p = H5FL_BLK_CALLOC(chunk_image, size)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* If this is a later version of the object header format, put the magic
     *  # at the beginning of the chunk image.
     */
    if(oh->version > H5O_VERSION_1) {
        HDmemcpy(p, H5O_CHK_MAGIC, (size_t)H5_SIZEOF_MAGIC);
        p += H5_SIZEOF_MAGIC;
    } /* end if */

    /*
     * Make sure we have enough space for all possible new messages
     * that could be generated below.
     */
    if(oh->nmesgs + 3 > oh->alloc_nmesgs)
        if(H5O_alloc_msgs(oh, (size_t)3) < 0)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate more space for messages")

    /* Check if we need to move multiple messages, in order to make room for the new message */
    if(multi_size > 0) {
        /* Move all non-null messages in the last chunk to the new chunk.  This
         * should be extremely rare so we don't care too much about minimizing
         * the space used.
         */
        H5O_mesg_t *null_msg;       /* Pointer to new null message */

        /* Protect last chunk */
        if(NULL == (chk_proxy = H5O_chunk_protect(f, dxpl_id, oh, chunkno - 1)))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header chunk")

        /* Copy each message to the new location */
        for(u = 0, curr_msg = &oh->mesg[0]; u < oh->nmesgs; u++, curr_msg++)
            if(curr_msg->chunkno == chunkno - 1) {
                if(curr_msg->type->id == H5O_NULL_ID) {
                    /* Delete the null message */
                    if(u < oh->nmesgs - 1)
                        HDmemmove(curr_msg, curr_msg + 1, ((oh->nmesgs - 1) - u) * sizeof(H5O_mesg_t));
                    oh->nmesgs--;
                } /* end if */
                else {
                    /* Copy the raw data */
                    HDmemcpy(p, curr_msg->raw - (size_t)H5O_SIZEOF_MSGHDR_OH(oh),
                        curr_msg->raw_size + (size_t)H5O_SIZEOF_MSGHDR_OH(oh));

                    /* Update the message info */
                    curr_msg->chunkno = chunkno;
                    curr_msg->raw = p + H5O_SIZEOF_MSGHDR_OH(oh);

                    /* Account for copied message in new chunk */
                    p += (size_t)H5O_SIZEOF_MSGHDR_OH(oh) + curr_msg->raw_size;
                    size -= (size_t)H5O_SIZEOF_MSGHDR_OH(oh) + curr_msg->raw_size;
                } /* end else */
            } /* end if */

        /* Create a null message spanning the entire last chunk */
        found_null = (int)oh->nmesgs++;
        null_msg = &(oh->mesg[found_null]);
        null_msg->type = H5O_MSG_NULL;
        null_msg->dirty = TRUE;
        null_msg->native = NULL;
        null_msg->raw = oh->chunk[chunkno - 1].image
                + ((chunkno == 1) ? H5O_SIZEOF_HDR(oh) : H5O_SIZEOF_CHKHDR_OH(oh))
                - H5O_SIZEOF_CHKSUM_OH(oh) + H5O_SIZEOF_MSGHDR_OH(oh);
        null_msg->raw_size = oh->chunk[chunkno - 1].size
                - ((chunkno == 1) ? (size_t)H5O_SIZEOF_HDR(oh) : (size_t)H5O_SIZEOF_CHKHDR_OH(oh))
                - (size_t)H5O_SIZEOF_MSGHDR_OH(oh);
        null_msg->chunkno = chunkno - 1;

        HDassert(null_msg->raw_size >= cont_size);

        /* Remove any gap in the chunk */
        oh->chunk[chunkno - 1].gap = 0;

        /* Release chunk, marking it dirty */
        if(H5O_chunk_unprotect(f, dxpl_id, chk_proxy, TRUE) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")
    } else if(found_null < 0) {
        /* Move message (that will be replaced with continuation message)
         *  to new chunk, if necessary.
         */
        H5O_mesg_t *null_msg;       /* Pointer to new null message */

        /* Protect chunk */
        if(NULL == (chk_proxy = H5O_chunk_protect(f, dxpl_id, oh, oh->mesg[found_other.msgno].chunkno)))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header chunk")

        /* Create null message for space that message to copy currently occupies */
        found_null = (int)oh->nmesgs++;
        null_msg = &(oh->mesg[found_null]);
        null_msg->type = H5O_MSG_NULL;
        null_msg->native = NULL;
        null_msg->raw = oh->mesg[found_other.msgno].raw;
        null_msg->raw_size = oh->mesg[found_other.msgno].raw_size;
        null_msg->chunkno = oh->mesg[found_other.msgno].chunkno;

        /* Copy the message to move (& its prefix) to its new location */
        HDmemcpy(p, oh->mesg[found_other.msgno].raw - H5O_SIZEOF_MSGHDR_OH(oh),
                 oh->mesg[found_other.msgno].raw_size + (size_t)H5O_SIZEOF_MSGHDR_OH(oh));

        /* Switch moved message to point to new location */
        oh->mesg[found_other.msgno].raw = p + H5O_SIZEOF_MSGHDR_OH(oh);
        oh->mesg[found_other.msgno].chunkno = chunkno;

        /* Account for copied message in new chunk */
        p += (size_t)H5O_SIZEOF_MSGHDR_OH(oh) + oh->mesg[found_other.msgno].raw_size;
        size -= (size_t)H5O_SIZEOF_MSGHDR_OH(oh) + oh->mesg[found_other.msgno].raw_size;

        /* Add any available space after the message to move to the new null message */
        if(found_other.gap_size > 0) {
            /* Absorb a gap after the moved message */
            HDassert(oh->chunk[null_msg->chunkno].gap == found_other.gap_size);
            null_msg->raw_size += found_other.gap_size;
            oh->chunk[null_msg->chunkno].gap = 0;
        } /* end if */
        else if(found_other.null_size > 0) {
            H5O_mesg_t *old_null_msg = &oh->mesg[found_other.null_msgno]; /* Pointer to NULL message to eliminate */

            /* Absorb a null message after the moved message */
            HDassert((null_msg->raw + null_msg->raw_size) == (old_null_msg->raw - H5O_SIZEOF_MSGHDR_OH(oh)));
            null_msg->raw_size += found_other.null_size;

            /* Release any information/memory for message */
            H5O_msg_free_mesg(old_null_msg);

            /* Remove null message from list of messages */
            if(found_other.null_msgno < (oh->nmesgs - 1))
                HDmemmove(old_null_msg, old_null_msg + 1, ((oh->nmesgs - 1) - found_other.null_msgno) * sizeof(H5O_mesg_t));

            /* Decrement # of messages */
            /* (Don't bother reducing size of message array for now -QAK) */
            oh->nmesgs--;

            /* Adjust message index for new NULL message */
            found_null--;
        } /* end if */

        /* Mark the new null message as dirty */
        null_msg->dirty = TRUE;

        /* Release chunk, marking it dirty */
        if(H5O_chunk_unprotect(f, dxpl_id, chk_proxy, TRUE) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")
    } /* end if */
    HDassert(found_null >= 0);

    /* Create null message for [rest of] space in new chunk */
    /* (account for chunk's magic # & checksum) */
    idx = oh->nmesgs++;
    oh->mesg[idx].type = H5O_MSG_NULL;
    oh->mesg[idx].dirty = TRUE;
    oh->mesg[idx].native = NULL;
    oh->mesg[idx].raw = p + H5O_SIZEOF_MSGHDR_OH(oh);
    oh->mesg[idx].raw_size = size - (size_t)(H5O_SIZEOF_CHKHDR_OH(oh) + H5O_SIZEOF_MSGHDR_OH(oh));
    oh->mesg[idx].chunkno = chunkno;

    /* Insert the new chunk into the cache */
    if(H5O_chunk_add(f, dxpl_id, oh, chunkno) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "can't add new chunk to cache")

    /* Initialize the continuation information */
    if(NULL == (cont = H5FL_MALLOC(H5O_cont_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
    cont->addr = oh->chunk[chunkno].addr;
    cont->size = oh->chunk[chunkno].size;
    cont->chunkno = chunkno;

    /* Split the null message and point at continuation message */
    if(H5O_alloc_null(f, dxpl_id, oh, (size_t)found_null, H5O_MSG_CONT, cont, cont_size) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "can't split null message")

    /* Set new message index value */
    *new_idx = idx;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_alloc_new_chunk() */


/*-------------------------------------------------------------------------
 * Function:    H5O_alloc
 *
 * Purpose:     Allocate enough space in the object header for this message.
 *
 * Return:      Success:        Index of message
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_alloc(H5F_t *f, hid_t dxpl_id, H5O_t *oh, const H5O_msg_class_t *type,
    const void *mesg, size_t *mesg_idx)
{
    size_t raw_size;            /* Raw size of message */
    size_t aligned_size;        /* Size of message including alignment */
    size_t idx;                 /* Index of message which fits allocation */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(oh);
    HDassert(type);
    HDassert(mesg);
    HDassert(mesg_idx);

    /* Compute the size needed to store the message in the object header */
    raw_size = (type->raw_size)(f, FALSE, mesg);
    if(0 == raw_size)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "can't compute object header message size")
    if(raw_size >= H5O_MESG_MAX_SIZE)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "object header message is too large")
    aligned_size = H5O_ALIGN_OH(oh, raw_size);

    /* look for a null message which is large enough */
    for(idx = 0; idx < oh->nmesgs; idx++)
        if(H5O_NULL_ID == oh->mesg[idx].type->id && oh->mesg[idx].raw_size >= aligned_size)
            break;

    /* if we didn't find one, then allocate more header space */
    if(idx >= oh->nmesgs) {
        unsigned        chunkno;

        /* check to see if we can extend one of the chunks.  If we can,
         * do so.  Otherwise, we will have to allocate a new chunk.
         *
         * Note that in this new version of this function, all chunks
         * must have file space allocated to them.
         */
        for(chunkno = 0; chunkno < oh->nchunks; chunkno++) {
            htri_t	tri_result;     /* Status from attempting to extend chunk */

            if((tri_result = H5O_alloc_extend_chunk(f, dxpl_id, oh, chunkno, raw_size, &idx)) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTEXTEND, FAIL, "H5O_alloc_extend_chunk failed unexpectedly")
            if(tri_result == TRUE)
		break;
        } /* end for */

        /* If we were not able to extend a chunk, create a new one */
        if(idx >= oh->nmesgs)
            if(H5O_alloc_new_chunk(f, dxpl_id, oh, raw_size, &idx) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_NOSPACE, FAIL, "unable to create a new object header data chunk")
    } /* end if */
    HDassert(idx < oh->nmesgs);

    /* Split the null message and point at continuation message */
    if(H5O_alloc_null(f, dxpl_id, oh, idx, type, NULL, aligned_size) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "can't split null message")

    /* Mark object header as dirty in cache */
    if(H5AC_mark_entry_dirty(oh) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTMARKDIRTY, FAIL, "unable to mark object header as dirty")

    /* Set message index value */
    *mesg_idx = idx;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_alloc() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5O_release_mesg
 *
 * Purpose:     Convert a message into a null message
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct 22 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_release_mesg(H5F_t *f, hid_t dxpl_id, H5O_t *oh, H5O_mesg_t *mesg,
    hbool_t adj_link)
{
    H5O_chunk_proxy_t *chk_proxy = NULL;    /* Chunk that message is in */
    hbool_t chk_dirtied = FALSE;            /* Flag for unprotecting chunk */
    herr_t ret_value = SUCCEED; 	    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(f);
    HDassert(oh);
    HDassert(mesg);

    /* Check if we should operate on the message */
    if(adj_link) {
        /* Free any space referred to in the file from this message */
        if(H5O_delete_mesg(f, dxpl_id, oh, mesg) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "unable to delete file space for object header message")
    } /* end if */

    /* Protect chunk */
    if(NULL == (chk_proxy = H5O_chunk_protect(f, dxpl_id, oh, mesg->chunkno)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to protect object header chunk")

    /* Free any native information */
    H5O_msg_free_mesg(mesg);

    /* Change message type to nil and zero it */
    mesg->type = H5O_MSG_NULL;
    HDassert(mesg->raw + mesg->raw_size <= (oh->chunk[mesg->chunkno].image + oh->chunk[mesg->chunkno].size) - (H5O_SIZEOF_CHKSUM_OH(oh) + oh->chunk[mesg->chunkno].gap));
    HDmemset(mesg->raw, 0, mesg->raw_size);

    /* Clear message flags */
    mesg->flags = 0;

    /* Mark the message as modified */
    mesg->dirty = TRUE;
    chk_dirtied = TRUE;

    /* Check if chunk has a gap currently */
    if(oh->chunk[mesg->chunkno].gap) {
        /* Eliminate the gap in the chunk */
        if(H5O_eliminate_gap(oh, &chk_dirtied, mesg,
                ((oh->chunk[mesg->chunkno].image + oh->chunk[mesg->chunkno].size) - (H5O_SIZEOF_CHKSUM_OH(oh) + oh->chunk[mesg->chunkno].gap)),
                oh->chunk[mesg->chunkno].gap) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTREMOVE, FAIL, "can't eliminate gap in chunk")
    } /* end if */

done:
    /* Release chunk, if not already done */
    if(chk_proxy && H5O_chunk_unprotect(f, dxpl_id, chk_proxy, chk_dirtied) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_release_mesg() */


/*-------------------------------------------------------------------------
 * Function:    H5O_move_cont
 *
 * Purpose:     Check and move message(s) forward into a continuation message
 *
 * Return:      Success:        non-negative (TRUE/FALSE)
 *              Failure:        negative
 *
 * Programmer:  Vailin Choi
 *		Feb. 2009
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5O_move_cont(H5F_t *f, hid_t dxpl_id, H5O_t *oh, unsigned cont_u)
{
    H5O_chunk_proxy_t *chk_proxy = NULL;        /* Chunk that continuation message is in */
    H5O_mesg_t 	*cont_msg;	/* Pointer to the continuation message */
    unsigned   	deleted_chunkno;       	/* Chunk # to delete */
    hbool_t     chk_dirtied = FALSE;    /* Flags for unprotecting chunk */
    htri_t 	ret_value = TRUE;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check arguments. */
    HDassert(f);
    HDassert(oh);

    /* Get initial information */
    cont_msg = &oh->mesg[cont_u];
    H5O_LOAD_NATIVE(f, dxpl_id, 0, oh, cont_msg, FAIL)
    deleted_chunkno = ((H5O_cont_t *)(cont_msg->native))->chunkno;

    /* Check if continuation message is pointing to the last chunk */
    if(deleted_chunkno == (oh->nchunks - 1)) {
        size_t nonnull_size;	/* Total size of nonnull messages in the chunk pointed to by cont message */
        H5O_mesg_t *curr_msg;   /* Pointer to the current message to operate on */
        size_t gap_size;        /* Size of gap produced */
        size_t v;	        /* Local index variable */

        /* Spin through messages */
        nonnull_size = 0;
        for(v = 0, curr_msg = &oh->mesg[0]; v < oh->nmesgs; v++, curr_msg++) {
            if(curr_msg->chunkno == deleted_chunkno) {
                /* If there's a locked message, we can't move all messages out of
                 *  chunk to delete, so get out now.
                 */
                if(curr_msg->locked)
                    HGOTO_DONE(FALSE)

                /* Find size of all non-null messages in the chunk pointed to by the continuation message */
                if(curr_msg->type->id != H5O_NULL_ID) {
                    HDassert(curr_msg->type->id != H5O_CONT_ID);
                    nonnull_size += curr_msg->raw_size + (size_t)H5O_SIZEOF_MSGHDR_OH(oh);
                } /* end if */
            } /* end if */
        } /* end for */

        /* Size of gap in chunk w/continuation message */
        gap_size = oh->chunk[cont_msg->chunkno].gap;

        /* Check if messages can fit into the continuation message + gap size */
        /* (Could count any null messages in the chunk w/the continuation
         *      message also, but that is pretty complex. -QAK)
         */
        if(nonnull_size && nonnull_size <= (gap_size + cont_msg->raw_size + (size_t)H5O_SIZEOF_MSGHDR_OH(oh))) {
            uint8_t *move_start, *move_end;	/* Pointers to area of messages to move */
            unsigned cont_chunkno;              /* Chunk number for continuation message */

            /* Get continuation info */
            move_start = cont_msg->raw - H5O_SIZEOF_MSGHDR_OH(oh);
            move_end = cont_msg->raw + cont_msg->raw_size;
            cont_chunkno = cont_msg->chunkno;

            /* Convert continuation message into a null message.  Do not delete
             * the target chunk yet, so we can still copy messages from it. */
            if(H5O_release_mesg(f, dxpl_id, oh, cont_msg, FALSE) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "unable to convert into null message")

            /* Protect chunk */
            if(NULL == (chk_proxy = H5O_chunk_protect(f, dxpl_id, oh, cont_chunkno)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to protect object header chunk")

            /* Move message(s) forward into continuation message */
            for(v = 0, curr_msg = &oh->mesg[0]; v < oh->nmesgs; v++, curr_msg++)
                /* Look for messages in chunk to delete */
                if(curr_msg->chunkno == deleted_chunkno) {
                    /* Move messages out of chunk to delete */
                    if(curr_msg->type->id != H5O_NULL_ID) {
                        size_t move_size;	/* Size of the message to be moved */

                        /* Compute size of message to move */
                        move_size = curr_msg->raw_size + (size_t)H5O_SIZEOF_MSGHDR_OH(oh);

                        /* Move message out of deleted chunk */
                        HDmemcpy(move_start, curr_msg->raw - H5O_SIZEOF_MSGHDR_OH(oh), move_size);
                        curr_msg->raw = move_start + H5O_SIZEOF_MSGHDR_OH(oh);
                        curr_msg->chunkno = cont_chunkno;
                        chk_dirtied = TRUE;

                        /* Adjust location to move messages to */
                        move_start += move_size;
                    } /* end else */
                } /* end if */

            /* Delete the target chunk */
            if(H5O_chunk_delete(f, dxpl_id, oh, deleted_chunkno) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "unable to remove chunk from cache")

            HDassert(move_start <= (move_end + gap_size));

            /* Check if there is space remaining in the continuation message */
            /* (The remaining space can be gap or a null message) */
            gap_size += (size_t)(move_end - move_start);
            if(gap_size >= (size_t)H5O_SIZEOF_MSGHDR_OH(oh)) {
                /* Adjust size of null (was continuation) message */
                cont_msg->raw_size = gap_size - (size_t)H5O_SIZEOF_MSGHDR_OH(oh);
                cont_msg->raw = move_start + H5O_SIZEOF_MSGHDR_OH(oh);
                cont_msg->dirty = TRUE;
                chk_dirtied = TRUE;
            } /* end if */
            else {
                /* Check if there is space that should be a gap */
                if(gap_size > 0) {
                    /* Convert remnant into gap in chunk */
                    if(H5O_add_gap(f, oh, cont_chunkno, &chk_dirtied, cont_u, move_start, gap_size) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "can't insert gap in chunk")
                } /* end if */

                /* Release any information/memory for continuation message */
                H5O_msg_free_mesg(cont_msg);
                if(cont_u < (oh->nmesgs - 1))
                    HDmemmove(&oh->mesg[cont_u], &oh->mesg[cont_u + 1], ((oh->nmesgs - 1) - cont_u) * sizeof(H5O_mesg_t));
                oh->nmesgs--;
            } /* end else */

            /* Move message(s) forward into continuation message */
            /*      Note: unsigned v wrapping around at the end */
            for(v = oh->nmesgs - 1, curr_msg = &oh->mesg[v]; v < oh->nmesgs; v--, curr_msg--)
                /* Look for messages in chunk to delete */
                if(curr_msg->chunkno == deleted_chunkno) {
                    /* Remove all null messages in deleted chunk from list of messages */
                    if(curr_msg->type->id == H5O_NULL_ID) {
                        /* Release any information/memory for message */
                        H5O_msg_free_mesg(curr_msg);
                        chk_dirtied = TRUE;

                        /* Remove from message list */
                        if(v < (oh->nmesgs - 1))
                            HDmemmove(&oh->mesg[v], &oh->mesg[v + 1], ((oh->nmesgs - 1) - v) * sizeof(H5O_mesg_t));
                        oh->nmesgs--;
                    } /* end if */
                } /* end if */

            /* Remove chunk from list of chunks */
            oh->chunk[deleted_chunkno].image = H5FL_BLK_FREE(chunk_image, oh->chunk[deleted_chunkno].image);
            oh->nchunks--;
        }  /* end if */
        else
            ret_value = FALSE;
    }  /* end if */
    else
        ret_value = FALSE;

done:
    /* Release chunk, if not already done */
    if(chk_proxy && H5O_chunk_unprotect(f, dxpl_id, chk_proxy, chk_dirtied) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_move_cont() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5O_move_msgs_forward
 *
 * Purpose:     Move messages toward first chunk
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Oct 17 2005
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5O_move_msgs_forward(H5F_t *f, hid_t dxpl_id, H5O_t *oh)
{
    H5O_chunk_proxy_t *null_chk_proxy = NULL;  /* Chunk that null message is in */
    H5O_chunk_proxy_t *curr_chk_proxy = NULL;  /* Chunk that message is in */
    hbool_t null_chk_dirtied = FALSE;  /* Flags for unprotecting null chunk */
    hbool_t curr_chk_dirtied = FALSE;  /* Flags for unprotecting curr chunk */
    hbool_t packed_msg;                 /* Flag to indicate that messages were packed */
    hbool_t did_packing = FALSE;        /* Whether any messages were packed */
    htri_t ret_value; 	                /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(oh);

    /* Loop until no messages packed */
    /* (Double loop is not very efficient, but it would be some extra work to
     *      add a list of messages to each chunk -QAK)
     */
    do {
        H5O_mesg_t *curr_msg;       /* Pointer to current message to operate on */
        unsigned	u;              /* Local index variable */

        /* Reset packed messages flag */
        packed_msg = FALSE;

        /* Scan through messages for messages that can be moved earlier in chunks */
        for(u = 0, curr_msg = &oh->mesg[0]; u < oh->nmesgs; u++, curr_msg++) {
            if(H5O_NULL_ID == curr_msg->type->id) {
                H5O_chunk_t *chunk;     /* Pointer to chunk that null message is in */

                /* Check if null message is not last in chunk */
                chunk = &(oh->chunk[curr_msg->chunkno]);
                if((curr_msg->raw + curr_msg->raw_size)
                        != ((chunk->image + chunk->size) - (H5O_SIZEOF_CHKSUM_OH(oh) + chunk->gap))) {
                    H5O_mesg_t *nonnull_msg;       /* Pointer to current message to operate on */
                    unsigned	v;              /* Local index variable */

                    /* Loop over messages again, looking for the message in the chunk after the null message */
                    for(v = 0, nonnull_msg = &oh->mesg[0]; v < oh->nmesgs; v++, nonnull_msg++) {
                        /* Locate message that is immediately after the null message */
                        if((curr_msg->chunkno == nonnull_msg->chunkno) &&
                                ((curr_msg->raw + curr_msg->raw_size) == (nonnull_msg->raw - H5O_SIZEOF_MSGHDR_OH(oh)))) {
                            /* Don't swap messages if the second message is also a null message */
                            /* (We'll merge them together later, in another routine) */
                            if(H5O_NULL_ID != nonnull_msg->type->id) {
                                /* Protect chunk */
                                if(NULL == (null_chk_proxy = H5O_chunk_protect(f, dxpl_id, oh, curr_msg->chunkno)))
                                    HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header chunk")

                                /* Copy raw data for non-null message to new location */
                                HDmemmove(curr_msg->raw - H5O_SIZEOF_MSGHDR_OH(oh),
                                    nonnull_msg->raw - H5O_SIZEOF_MSGHDR_OH(oh), nonnull_msg->raw_size + (size_t)H5O_SIZEOF_MSGHDR_OH(oh));

                                /* Adjust non-null message's offset in chunk */
                                nonnull_msg->raw = curr_msg->raw;

                                /* Adjust null message's offset in chunk */
                                curr_msg->raw = nonnull_msg->raw +
                                        nonnull_msg->raw_size + H5O_SIZEOF_MSGHDR_OH(oh);

                                /* Mark null message dirty */
                                /* (since we need to re-encode its message header) */
                                curr_msg->dirty = TRUE;

                                /* Release chunk, marking it dirty */
                                if(H5O_chunk_unprotect(f, dxpl_id, null_chk_proxy, TRUE) < 0)
                                    HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")
                                null_chk_proxy = NULL;

                                /* Set the flag to indicate that the null message
                                 * was packed - if its not at the end its chunk,
                                 * we'll move it again on the next pass.
                                 */
                                packed_msg = TRUE;
                            } /* end if */

                            /* Break out of loop */
                            break;
                        } /* end if */
                    } /* end for */
                    /* Should have been message after null message */
                    HDassert(v < oh->nmesgs);
                } /* end if */
            } /* end if */
            else {
                H5O_mesg_t *null_msg;   /* Pointer to current message to operate on */
                size_t   v;             /* Local index variable */

                /* Check if messages in chunk pointed to can replace continuation message */
                if(H5O_CONT_ID == curr_msg->type->id) {
                    htri_t status;      /* Status from moving messages */

		    if((status = H5O_move_cont(f, dxpl_id, oh, u)) < 0)
			HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "Error in moving messages into cont message")
		    else if(status > 0) { /* Message(s) got moved into "continuation" message */
                        packed_msg = TRUE;
			break;
		    } /* end else-if */
		} /* end if */

                /* Don't let locked messages be moved into earlier chunk */
                if(!curr_msg->locked) {
                    /* Loop over messages again, looking for large enough null message in earlier chunk */
                    for(v = 0, null_msg = &oh->mesg[0]; v < oh->nmesgs; v++, null_msg++) {
                        if(H5O_NULL_ID == null_msg->type->id && curr_msg->chunkno > null_msg->chunkno
                                && curr_msg->raw_size <= null_msg->raw_size) {
                            unsigned old_chunkno;   /* Old message information */
                            uint8_t *old_raw;

                            /* Keep old information about non-null message */
                            old_chunkno = curr_msg->chunkno;
                            old_raw = curr_msg->raw;

                            /* Protect chunks */
                            if(NULL == (null_chk_proxy = H5O_chunk_protect(f, dxpl_id, oh, null_msg->chunkno)))
                                HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header chunk")
                            if(NULL == (curr_chk_proxy = H5O_chunk_protect(f, dxpl_id, oh, curr_msg->chunkno)))
                                HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header chunk")

                            /* Copy raw data for non-null message to new chunk */
                            HDmemcpy(null_msg->raw - H5O_SIZEOF_MSGHDR_OH(oh), curr_msg->raw - H5O_SIZEOF_MSGHDR_OH(oh), curr_msg->raw_size + (size_t)H5O_SIZEOF_MSGHDR_OH(oh));

                            /* Point non-null message at null message's space */
                            curr_msg->chunkno = null_msg->chunkno;
                            curr_msg->raw = null_msg->raw;
                            curr_chk_dirtied = TRUE;

                            /* Change information for null message */
                            if(curr_msg->raw_size == null_msg->raw_size) {
                                /* Point null message at old non-null space */
                                /* (Instead of freeing it and allocating new message) */
                                null_msg->chunkno = old_chunkno;
                                null_msg->raw = old_raw;

                                /* Mark null message dirty */
                                null_msg->dirty = TRUE;
                                null_chk_dirtied = TRUE;

                                /* Release current chunk, marking it dirty */
                                if(H5O_chunk_unprotect(f, dxpl_id, curr_chk_proxy, curr_chk_dirtied) < 0)
                                    HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")
                                curr_chk_proxy = NULL;
                                curr_chk_dirtied = FALSE;

                                /* Check for gap in null message's chunk */
                                if(oh->chunk[old_chunkno].gap > 0) {
                                    /* Eliminate the gap in the chunk */
                                    if(H5O_eliminate_gap(oh, &null_chk_dirtied, null_msg,
                                            ((oh->chunk[old_chunkno].image + oh->chunk[old_chunkno].size) - (H5O_SIZEOF_CHKSUM_OH(oh) + oh->chunk[old_chunkno].gap)),
                                            oh->chunk[old_chunkno].gap) < 0)
                                        HGOTO_ERROR(H5E_OHDR, H5E_CANTREMOVE, FAIL, "can't eliminate gap in chunk")
                                } /* end if */

                                /* Release null chunk, marking it dirty */
                                if(H5O_chunk_unprotect(f, dxpl_id, null_chk_proxy, null_chk_dirtied) < 0)
                                    HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")
                                null_chk_proxy = NULL;
                                null_chk_dirtied = FALSE;
                            } /* end if */
                            else {
                                size_t new_null_msg;          /* Message index for new null message */

                                /* Check if null message is large enough to still exist */
                                if((null_msg->raw_size - curr_msg->raw_size) < (size_t)H5O_SIZEOF_MSGHDR_OH(oh)) {
                                    size_t gap_size = null_msg->raw_size - curr_msg->raw_size;     /* Size of gap produced */

                                    /* Adjust the size of the null message being eliminated */
                                    null_msg->raw_size = curr_msg->raw_size;

                                    /* Mark null message dirty */
                                    null_msg->dirty = TRUE;
                                    null_chk_dirtied = TRUE;

                                    /* Add the gap to the chunk */
                                    if(H5O_add_gap(f, oh, null_msg->chunkno, &null_chk_dirtied, v, null_msg->raw + null_msg->raw_size, gap_size) < 0)
                                        HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "can't insert gap in chunk")

                                    /* Re-use message # for new null message taking place of non-null message */
                                    new_null_msg = v;
                                } /* end if */
                                else {
                                    /* Adjust null message's size & offset */
                                    null_msg->raw += curr_msg->raw_size + (size_t)H5O_SIZEOF_MSGHDR_OH(oh);
                                    null_msg->raw_size -= curr_msg->raw_size + (size_t)H5O_SIZEOF_MSGHDR_OH(oh);

                                    /* Mark null message dirty */
                                    null_msg->dirty = TRUE;
                                    null_chk_dirtied = TRUE;

                                    /* Create new null message for previous location of non-null message */
                                    if(oh->nmesgs >= oh->alloc_nmesgs) {
                                        if(H5O_alloc_msgs(oh, (size_t)1) < 0)
                                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "can't allocate more space for messages")

                                        /* "Retarget" 'curr_msg' pointer into newly re-allocated array of messages */
                                        curr_msg = &oh->mesg[u];
                                    } /* end if */

                                    /* Get message # for new null message */
                                    new_null_msg = oh->nmesgs++;
                                } /* end else */

                                /* Release null message's chunk, marking it dirty */
                                if(H5O_chunk_unprotect(f, dxpl_id, null_chk_proxy, null_chk_dirtied) < 0)
                                    HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")
                                null_chk_proxy = NULL;
                                null_chk_dirtied = FALSE;

                                /* Initialize new null message to take over non-null message's location */
                                oh->mesg[new_null_msg].type = H5O_MSG_NULL;
                                oh->mesg[new_null_msg].native = NULL;
                                oh->mesg[new_null_msg].raw = old_raw;
                                oh->mesg[new_null_msg].raw_size = curr_msg->raw_size;
                                oh->mesg[new_null_msg].chunkno = old_chunkno;

                                /* Mark new null message dirty */
                                oh->mesg[new_null_msg].dirty = TRUE;
                                curr_chk_dirtied = TRUE;

                                /* Check for gap in new null message's chunk */
                                if(oh->chunk[old_chunkno].gap > 0) {
                                    /* Eliminate the gap in the chunk */
                                    if(H5O_eliminate_gap(oh, &curr_chk_dirtied, &oh->mesg[new_null_msg],
                                            ((oh->chunk[old_chunkno].image + oh->chunk[old_chunkno].size) - (H5O_SIZEOF_CHKSUM_OH(oh) + oh->chunk[old_chunkno].gap)),
                                            oh->chunk[old_chunkno].gap) < 0)
                                        HGOTO_ERROR(H5E_OHDR, H5E_CANTREMOVE, FAIL, "can't eliminate gap in chunk")
                                } /* end if */

                                /* Release new null message's chunk, marking it dirty */
                                if(H5O_chunk_unprotect(f, dxpl_id, curr_chk_proxy, curr_chk_dirtied) < 0)
                                    HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")
                                curr_chk_proxy = NULL;
                                curr_chk_dirtied = FALSE;
                            } /* end else */

                            /* Indicate that we packed messages */
                            packed_msg = TRUE;

                            /* Break out of loop */
                            /* (If it's possible to move message to even earlier chunk
                             *      we'll get it on the next pass - QAK)
                             */
                            break;
                        } /* end if */
                    } /* end for */
                } /* end if */

                /* If we packed messages, get out of loop and start over */
                /* (Don't know if this has any benefit one way or the other -QAK) */
                if(packed_msg)
                    break;
            } /* end else */
        } /* end for */

        /* If we did any packing, remember that */
        if(packed_msg)
            did_packing = TRUE;
    } while(packed_msg);

    /* Set return value */
    ret_value = (htri_t)did_packing;

done:
    if(null_chk_proxy && H5O_chunk_unprotect(f, dxpl_id, null_chk_proxy, null_chk_dirtied) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect null object header chunk")
    if(curr_chk_proxy && H5O_chunk_unprotect(f, dxpl_id, curr_chk_proxy, curr_chk_dirtied) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect current object header chunk")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_move_msgs_forward() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5O_merge_null
 *
 * Purpose:     Merge neighboring null messages in an object header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Oct 10 2005
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5O_merge_null(H5F_t *f, hid_t dxpl_id, H5O_t *oh)
{
    hbool_t merged_msg;                 /* Flag to indicate that messages were merged */
    hbool_t did_merging = FALSE;        /* Whether any messages were merged */
    htri_t ret_value; 	                /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(oh != NULL);

    /* Loop until no messages merged */
    /* (Double loop is not very efficient, but it would be some extra work to add
     *      a list of messages to each chunk -QAK)
     */
    do {
        H5O_mesg_t *curr_msg;       /* Pointer to current message to operate on */
        unsigned	u;              /* Local index variable */

        /* Reset merged messages flag */
        merged_msg = FALSE;

        /* Scan messages for adjacent null messages & merge them */
        for(u = 0, curr_msg = &oh->mesg[0]; u < oh->nmesgs; u++, curr_msg++) {
            if(H5O_NULL_ID == curr_msg->type->id) {
                H5O_mesg_t *curr_msg2;       /* Pointer to current message to operate on */
                unsigned	v;              /* Local index variable */

                /* Should be no gaps in chunk with null message */
                HDassert(oh->chunk[curr_msg->chunkno].gap == 0);

                /* Loop over messages again, looking for null message in same chunk */
                for(v = 0, curr_msg2 = &oh->mesg[0]; v < oh->nmesgs; v++, curr_msg2++) {
                    if(u != v && H5O_NULL_ID == curr_msg2->type->id && curr_msg->chunkno == curr_msg2->chunkno) {
                        ssize_t adj_raw;        /* Amount to adjust raw message pointer */
                        size_t adj_raw_size;    /* Amount to adjust raw message size */

                        /* Check for second message after first message */
                        if((curr_msg->raw + curr_msg->raw_size) == (curr_msg2->raw - H5O_SIZEOF_MSGHDR_OH(oh))) {
                            /* Extend first null message length to cover second null message */
                            adj_raw = 0;
                            adj_raw_size = (size_t)H5O_SIZEOF_MSGHDR_OH(oh) + curr_msg2->raw_size;

                            /* Message has been merged */
                            merged_msg = TRUE;
                        } /* end if */
                        /* Check for second message before first message */
                        else if((curr_msg->raw - H5O_SIZEOF_MSGHDR_OH(oh)) == (curr_msg2->raw + curr_msg2->raw_size)) {
                            /* Adjust first message address and extend length to cover second message */
                            adj_raw = -((ssize_t)((size_t)H5O_SIZEOF_MSGHDR_OH(oh) + curr_msg2->raw_size));
                            adj_raw_size = (size_t)H5O_SIZEOF_MSGHDR_OH(oh) + curr_msg2->raw_size;

                            /* Message has been merged */
                            merged_msg = TRUE;
                        } /* end if */

                        /* Second message has been merged, delete it */
                        if(merged_msg) {
                            H5O_chunk_proxy_t *curr_chk_proxy;        /* Chunk that message is in */

                            /* Release any information/memory for second message */
                            H5O_msg_free_mesg(curr_msg2);

                            /* Protect chunk */
                            if(NULL == (curr_chk_proxy = H5O_chunk_protect(f, dxpl_id, oh, curr_msg->chunkno)))
                                HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header chunk")

                            /* Adjust first message address and extend length to cover second message */
                            curr_msg->raw += adj_raw;
                            curr_msg->raw_size += adj_raw_size;

                            /* Mark first message as dirty */
                            curr_msg->dirty = TRUE;

                            /* Release new null message's chunk, marking it dirty */
                            if(H5O_chunk_unprotect(f, dxpl_id, curr_chk_proxy, TRUE) < 0)
                                HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")

                            /* Remove second message from list of messages */
                            if(v < (oh->nmesgs - 1))
                                HDmemmove(&oh->mesg[v], &oh->mesg[v + 1], ((oh->nmesgs - 1) - v) * sizeof(H5O_mesg_t));

                            /* Decrement # of messages */
                            /* (Don't bother reducing size of message array for now -QAK) */
                            oh->nmesgs--;

                            /* If the merged message is too large, shrink the chunk */
                            if(curr_msg->raw_size >= H5O_MESG_MAX_SIZE)
                                if(H5O_alloc_shrink_chunk(f, dxpl_id, oh, curr_msg->chunkno) < 0)
                                    HGOTO_ERROR(H5E_OHDR, H5E_CANTPACK, FAIL, "unable to shrink chunk")

                            /* Get out of loop */
                            break;
                        } /* end if */
                    } /* end if */
                } /* end for */

                /* Get out of loop if we merged messages */
                if(merged_msg)
                    break;
            } /* end if */
        } /* end for */

        /* If we did any merging, remember that */
        if(merged_msg)
            did_merging = TRUE;
    } while(merged_msg);

    /* Set return value */
    ret_value = (htri_t)did_merging;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_merge_null() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5O_remove_empty_chunks
 *
 * Purpose:     Attempt to eliminate empty chunks from object header.
 *
 *              This examines a chunk to see if it's empty
 *              and removes it (and the continuation message that points to it)
 *              from the object header.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Oct 17 2005
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5O_remove_empty_chunks(H5F_t *f, hid_t dxpl_id, H5O_t *oh)
{
    hbool_t deleted_chunk;              /* Whether to a chunk was deleted */
    hbool_t did_deleting = FALSE;       /* Whether any chunks were deleted */
    htri_t ret_value; 	                /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(oh != NULL);

    /* Loop until no chunks are freed */
    do {
        H5O_mesg_t *null_msg;       /* Pointer to null message found */
        H5O_mesg_t *cont_msg;       /* Pointer to continuation message found */
        unsigned	u, v;       /* Local index variables */

        /* Reset 'chunk deleted' flag */
        deleted_chunk = FALSE;

        /* Scan messages for null messages that fill an entire chunk */
        for(u = 0, null_msg = &oh->mesg[0]; u < oh->nmesgs; u++, null_msg++) {
            /* If a null message takes up an entire object header chunk (and
             * its not the "base" chunk), delete that chunk from object header
             */
            if(H5O_NULL_ID == null_msg->type->id && null_msg->chunkno > 0 &&
                    ((size_t)H5O_SIZEOF_MSGHDR_OH(oh) + null_msg->raw_size)
                         == (oh->chunk[null_msg->chunkno].size - H5O_SIZEOF_CHKHDR_OH(oh))) {
                H5O_mesg_t *curr_msg;           /* Pointer to current message to operate on */
                unsigned null_msg_no;           /* Message # for null message */
                unsigned deleted_chunkno;       /* Chunk # to delete */

                /* Locate continuation message that points to chunk */
                for(v = 0, cont_msg = &oh->mesg[0]; v < oh->nmesgs; v++, cont_msg++) {
                    if(H5O_CONT_ID == cont_msg->type->id) {
                        /* Decode current continuation message if necessary */
                        H5O_LOAD_NATIVE(f, dxpl_id, 0, oh, cont_msg, FAIL)

                        /* Check if the chunkno needs to be set */
                        /* (should only occur when the continuation message is first decoded) */
                        if(0 == ((H5O_cont_t *)(cont_msg->native))->chunkno) {
                            unsigned w;         /* Local index variable */

                            /* Find chunk that this continuation message points to */
                            for(w = 0; w < oh->nchunks; w++)
                                if(oh->chunk[w].addr == ((H5O_cont_t *)(cont_msg->native))->addr) {
                                    ((H5O_cont_t *)(cont_msg->native))->chunkno = w;
                                    break;
                                } /* end if */
                            HDassert(((H5O_cont_t *)(cont_msg->native))->chunkno > 0);
                        } /* end if */

                        /* Check for correct chunk to delete */
                        if(oh->chunk[null_msg->chunkno].addr == ((H5O_cont_t *)(cont_msg->native))->addr)
                            break;
                    } /* end if */
                } /* end for */
                /* Must be a continuation message that points to chunk containing null message */
                HDassert(v < oh->nmesgs);
                HDassert(cont_msg);
                HDassert(((H5O_cont_t *)(cont_msg->native))->chunkno == null_msg->chunkno);

                /* Initialize information about null message */
                null_msg_no = u;
                deleted_chunkno = null_msg->chunkno;

                /* Convert continuation message into a null message */
                if(H5O_release_mesg(f, dxpl_id, oh, cont_msg, TRUE) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "unable to convert into null message")

                /*
                 * Remove chunk from object header's data structure
                 */

                /* Free memory for chunk image */
                oh->chunk[null_msg->chunkno].image = H5FL_BLK_FREE(chunk_image, oh->chunk[null_msg->chunkno].image);

                /* Remove chunk from list of chunks */
                if(null_msg->chunkno < (oh->nchunks - 1)) {
                    HDmemmove(&oh->chunk[null_msg->chunkno], &oh->chunk[null_msg->chunkno + 1], ((oh->nchunks - 1) - null_msg->chunkno) * sizeof(H5O_chunk_t));

                    /* Adjust chunk number for any chunk proxies that are in the cache */
                    for(u = null_msg->chunkno; u < (oh->nchunks - 1); u++) {
                        unsigned chk_proxy_status = 0;  /* Metadata cache status of chunk proxy for chunk */

                        /* Check the chunk proxy's status in the metadata cache */
                        if(H5AC_get_entry_status(f, oh->chunk[u].addr, &chk_proxy_status) < 0)
                            HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to check metadata cache status for chunk proxy")

                        /* If the entry is in the cache, update its chunk index */
                        if(chk_proxy_status & H5AC_ES__IN_CACHE) {
                            if(H5O_chunk_update_idx(f, dxpl_id, oh, u) < 0)
                                HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "unable to update index for chunk proxy")
                        } /* end if */
                    } /* end for */
                } /* end if */

                /* Decrement # of chunks */
                /* (Don't bother reducing size of chunk array for now -QAK) */
                oh->nchunks--;

                /*
                 * Delete null message (in empty chunk that was be freed) from list of messages
                 */

                /* Release any information/memory for message */
                H5O_msg_free_mesg(null_msg);

                /* Remove null message from list of messages */
                if(null_msg_no < (oh->nmesgs - 1))
                    HDmemmove(&oh->mesg[null_msg_no], &oh->mesg[null_msg_no + 1], ((oh->nmesgs - 1) - null_msg_no) * sizeof(H5O_mesg_t));

                /* Decrement # of messages */
                /* (Don't bother reducing size of message array for now -QAK) */
                oh->nmesgs--;

                /* Adjust chunk # for messages in chunks after deleted chunk */
                for(u = 0, curr_msg = &oh->mesg[0]; u < oh->nmesgs; u++, curr_msg++) {
                    /* Sanity check - there should be no messages in deleted chunk */
                    HDassert(curr_msg->chunkno != deleted_chunkno);

                    /* Adjust chunk index for messages in later chunks */
                    if(curr_msg->chunkno > deleted_chunkno)
                        curr_msg->chunkno--;

                    /* Check for continuation message */
                    if(H5O_CONT_ID == curr_msg->type->id) {
                        /* Decode current continuation message if necessary */
                        H5O_LOAD_NATIVE(f, dxpl_id, 0, oh, curr_msg, FAIL)

                        /* Check if the chunkno needs to be set */
                        /* (should only occur when the continuation message is first decoded) */
                        if(0 == ((H5O_cont_t *)(curr_msg->native))->chunkno) {
                            unsigned w;         /* Local index variable */

                            /* Find chunk that this continuation message points to */
                            for(w = 0; w < oh->nchunks; w++)
                                if(oh->chunk[w].addr == ((H5O_cont_t *)(curr_msg->native))->addr) {
                                    ((H5O_cont_t *)(curr_msg->native))->chunkno = w;
                                    break;
                                } /* end if */
                            HDassert(((H5O_cont_t *)(curr_msg->native))->chunkno > 0);
                        } /* end if */
                        else {
                            /* Check for pointer to chunk after deleted chunk */
                            if(((H5O_cont_t *)(curr_msg->native))->chunkno > deleted_chunkno)
                                ((H5O_cont_t *)(curr_msg->native))->chunkno--;
                        } /* end else */
                    } /* end if */
                } /* end for */

                /* Found chunk to delete */
                deleted_chunk = TRUE;
                break;
            } /* end if */
        } /* end for */

        /* If we deleted any chunks, remember that */
        if(deleted_chunk)
            did_deleting = TRUE;
    } while(deleted_chunk);

    /* Set return value */
    ret_value = (htri_t)did_deleting;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_remove_empty_chunks() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5O_condense_header
 *
 * Purpose:     Attempt to eliminate empty chunks from object header.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Oct  4 2005
 *
 * Modifications:
 *   Feb. 2009: Vailin Choi
 *      Add 2 more parameters to H5O_move_msgs_forward() for moving
 *	messages forward into "continuation" message
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_condense_header(H5F_t *f, H5O_t *oh, hid_t dxpl_id)
{
    hbool_t rescan_header;              /* Whether to rescan header */
    htri_t result;                      /* Result from packing/merging/etc */
    herr_t ret_value = SUCCEED; 	/* return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(oh != NULL);

    /* Loop until no changed to the object header messages & chunks */
    do {
        /* Reset 'rescan chunks' flag */
        rescan_header = FALSE;

        /* Scan for messages that can be moved earlier in chunks */
        result = H5O_move_msgs_forward(f, dxpl_id, oh);
        if(result < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTPACK, FAIL, "can't move header messages forward")
        if(result > 0)
            rescan_header = TRUE;

        /* Scan for adjacent null messages & merge them */
        result = H5O_merge_null(f, dxpl_id, oh);
        if(result < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTPACK, FAIL, "can't pack null header messages")
        if(result > 0)
            rescan_header = TRUE;

        /* Scan for empty chunks to remove */
        result = H5O_remove_empty_chunks(f, dxpl_id, oh);
        if(result < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTPACK, FAIL, "can't remove empty chunk")
        if(result > 0)
            rescan_header = TRUE;
    } while(rescan_header);
#ifdef H5O_DEBUG
H5O_assert(oh);
#endif /* H5O_DEBUG */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_condense_header() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5O_alloc_shrink_chunk
 *
 * Purpose:     Shrinks a chunk, removing all null messages and any gap.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Neil Fortner
 *		nfortne2@hdfgroup.org
 *		Oct 20 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_alloc_shrink_chunk(H5F_t *f, hid_t dxpl_id, H5O_t *oh, unsigned chunkno)
{
    H5O_chunk_t *chunk = &oh->chunk[chunkno];   /* Chunk to shrink */
    H5O_chunk_proxy_t *chk_proxy = NULL;        /* Metadata cache proxy for chunk to shrink */
    H5O_mesg_t  *curr_msg;                      /* Current message to examine */
    uint8_t     *old_image = chunk->image;      /* Old address of chunk's image in memory */
    size_t      old_size = chunk->size;         /* Old size of chunk */
    size_t      new_size = chunk->size - chunk->gap; /* Size of shrunk chunk */
    size_t      total_msg_size;                 /* Size of the messages in this chunk */
    size_t      min_chunk_size = H5O_ALIGN_OH(oh, H5O_MIN_SIZE); /* Minimum chunk size */
    size_t      sizeof_chksum = H5O_SIZEOF_CHKSUM_OH(oh); /* Size of chunk checksum */
    size_t      sizeof_msghdr = H5O_SIZEOF_MSGHDR_OH(oh); /* Size of message header */
    uint8_t     new_size_flags = 0;             /* New chunk #0 size flags */
    hbool_t     adjust_size_flags = FALSE;      /* Whether to adjust the chunk #0 size flags */
    size_t      less_prfx_size = 0;             /* Bytes removed from object header prefix */
    size_t      u;                              /* Index */
    herr_t      ret_value = SUCCEED;            /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f);
    HDassert(oh);

    /* Protect chunk */
    if(NULL == (chk_proxy = H5O_chunk_protect(f, dxpl_id, oh, chunkno)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to protect object header chunk")

    /* Loop backwards to increase the chance of seeing more null messages at the
     * end of the chunk.  Note that we rely on unsigned u wrapping around at the
     * end.
     */
    for(u = oh->nmesgs - 1, curr_msg = &oh->mesg[u]; u < oh->nmesgs; u--, curr_msg--) {
        if((H5O_NULL_ID == curr_msg->type->id) && (chunkno == curr_msg->chunkno)) {
            size_t shrink_size = curr_msg->raw_size + sizeof_msghdr; /* Amount to shrink the chunk by */

            /* If the current message is not at the end of the chunk, copy the
                * data after it (except the checksum).
                */
            if(curr_msg->raw + curr_msg->raw_size
                < old_image + new_size - sizeof_chksum) {
                unsigned    v;              /* Index */
                H5O_mesg_t  *curr_msg2;
                uint8_t     *src = curr_msg->raw + curr_msg->raw_size; /* Source location */

                /* Slide down the raw data */
                HDmemmove(curr_msg->raw - sizeof_msghdr, src,
                    (size_t)(old_image + new_size - sizeof_chksum - src));

                /* Update the raw data pointers for messages after this one */
                for(v = 0, curr_msg2 = &oh->mesg[0]; v < oh->nmesgs; v++, curr_msg2++)
                    if((chunkno == curr_msg2->chunkno) && (curr_msg2->raw > curr_msg->raw))
                        curr_msg2->raw -= shrink_size;
            } /* end if */

            /* Adjust the new chunk size */
            new_size -= shrink_size;

            /* Release any information/memory for the message */
            H5O_msg_free_mesg(curr_msg);

            /* Remove the deleted null message from list of messages */
            if(u < (oh->nmesgs - 1))
                HDmemmove(&oh->mesg[u], &oh->mesg[u+1], ((oh->nmesgs - 1) - u) * sizeof(H5O_mesg_t));

            /* Decrement # of messages */
            /* (Don't bother reducing size of message array for now) */
            oh->nmesgs--;
        } /* end if */
    } /* end for */

    /* Check if the chunk is too small, extend if necessary */
    total_msg_size = new_size - (size_t)(chunkno == 0 ? H5O_SIZEOF_HDR(oh) : H5O_SIZEOF_CHKHDR_OH(oh));
    if(total_msg_size < min_chunk_size) {
        HDassert(oh->alloc_nmesgs > oh->nmesgs);
        oh->nmesgs++;

        /* Initialize new null message to make the chunk large enough */
        oh->mesg[oh->nmesgs].type = H5O_MSG_NULL;
        oh->mesg[oh->nmesgs].dirty = TRUE;
        oh->mesg[oh->nmesgs].native = NULL;
        oh->mesg[oh->nmesgs].raw = old_image + new_size + sizeof_msghdr - sizeof_chksum;
        oh->mesg[oh->nmesgs].raw_size = MAX(H5O_ALIGN_OH(oh, min_chunk_size - total_msg_size),
            sizeof_msghdr) - sizeof_msghdr;
        oh->mesg[oh->nmesgs].chunkno = chunkno;

        /* update the new chunk size */
        new_size += oh->mesg[oh->nmesgs].raw_size + sizeof_msghdr;
    } /* end if */

    /* Check for changing the chunk #0 data size enough to need adjusting the flags */
    if(oh->version > H5O_VERSION_1 && chunkno == 0) {
        uint64_t chunk0_newsize = new_size - (size_t)H5O_SIZEOF_HDR(oh);  /* New size of chunk 0's data */
        size_t   orig_prfx_size = (size_t)1 << (oh->flags & H5O_HDR_CHUNK0_SIZE); /* Original prefix size */

        /* Check for moving to a 1-byte size encoding */
        if(orig_prfx_size > 1 && chunk0_newsize <= 255) {
            less_prfx_size = orig_prfx_size - 1;
            new_size_flags = H5O_HDR_CHUNK0_1;
            adjust_size_flags = TRUE;
        } /* end if */
        /* Check for moving to a 2-byte size encoding */
        else if(orig_prfx_size > 2 && chunk0_newsize <= 65535) {
            less_prfx_size = orig_prfx_size - 2;
            new_size_flags = H5O_HDR_CHUNK0_2;
            adjust_size_flags = TRUE;
        } /* end if */
        /* Check for moving to a 4-byte size encoding */
        else if(orig_prfx_size > 4 && chunk0_newsize <= 4294967295) {
            less_prfx_size = orig_prfx_size - 4;
            new_size_flags = H5O_HDR_CHUNK0_4;
            adjust_size_flags = TRUE;
        } /* end if */
    } /* end if */

    if(adjust_size_flags) {
        /* Adjust object header prefix flags */
        oh->flags = (uint8_t)(oh->flags & ~H5O_HDR_CHUNK0_SIZE);
        oh->flags |= new_size_flags;

        /* Slide chunk 0 data down */
        HDmemmove(chunk->image + H5O_SIZEOF_HDR(oh) - sizeof_chksum,
            chunk->image + H5O_SIZEOF_HDR(oh) - sizeof_chksum + less_prfx_size,
            new_size - (size_t)H5O_SIZEOF_HDR(oh));

        /* Adjust chunk size */
        new_size -= less_prfx_size;
    } /* end if */

    /* Allocate less memory space for chunk's image */
    chunk->size = new_size;
    chunk->image = H5FL_BLK_REALLOC(chunk_image, old_image, chunk->size);
    chunk->gap = 0;
    if(NULL == oh->chunk[chunkno].image)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Spin through existing messages, adjusting them */
    for(u = 0, curr_msg = &oh->mesg[0]; u < oh->nmesgs; u++, curr_msg++) {
        if(adjust_size_flags || (chunk->image != old_image))
            /* Adjust raw addresses for messages in this chunk to reflect new 'image' address */
            if(curr_msg->chunkno == chunkno)
                curr_msg->raw = chunk->image - less_prfx_size + (curr_msg->raw - old_image);

        /* Find continuation message which points to this chunk and adjust chunk's size */
        /* (Chunk 0 doesn't have a continuation message that points to it and
         * its size is directly encoded in the object header) */
        if(chunkno > 0 && (H5O_CONT_ID == curr_msg->type->id) &&
                (((H5O_cont_t *)(curr_msg->native))->chunkno == chunkno)) {
            H5O_chunk_proxy_t *cont_chk_proxy;        /* Chunk that message is in */

            /* Protect chunk */
            if(NULL == (cont_chk_proxy = H5O_chunk_protect(f, dxpl_id, oh, curr_msg->chunkno)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to protect object header chunk")

            /* Adjust size of continuation message */
            HDassert(((H5O_cont_t *)(curr_msg->native))->size == old_size);
            ((H5O_cont_t *)(curr_msg->native))->size = chunk->size;

            /* Flag continuation message as dirty */
            curr_msg->dirty = TRUE;

            /* Release chunk, marking it dirty */
            if(H5O_chunk_unprotect(f, dxpl_id, cont_chk_proxy, TRUE) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")
        } /* end if */
    } /* end for */

    HDassert(new_size <= old_size);

    /* Resize the chunk in the cache */
    if(H5O_chunk_resize(oh, chk_proxy) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTRESIZE, FAIL, "unable to resize object header chunk")

    /* Free the unused space in the file */
    if(H5MF_xfree(f, H5FD_MEM_OHDR, dxpl_id, chunk->addr + new_size, (hsize_t)(old_size - new_size)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to shrink object header chunk")

done:
    /* Release chunk, marking it dirty */
    if(chk_proxy && H5O_chunk_unprotect(f, dxpl_id, chk_proxy, TRUE) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_alloc_shrink_chunk() */

