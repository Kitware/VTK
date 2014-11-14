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
 * Created:		H5Omessage.c
 *			Dec  3 2006
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		Object header message routines.
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
#include "H5Aprivate.h"		/* Attributes	  			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* File access				*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Opkg.h"             /* Object headers			*/
#include "H5SMprivate.h"        /* Shared object header messages        */


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/

/* User data for iteration while removing a message */
typedef struct {
    H5F_t      *f;              /* Pointer to file for insertion */
    hid_t dxpl_id;              /* DXPL during iteration */
    int sequence;               /* Sequence # to search for */
    unsigned nfailed;           /* # of failed message removals */
    H5O_operator_t op;          /* Callback routine for removal operations */
    void *op_data;              /* Callback data for removal operations */
    hbool_t adj_link;           /* Whether to adjust links when removing messages */
} H5O_iter_rm_t;


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

static herr_t H5O_msg_reset_real(const H5O_msg_class_t *type, void *native);
static herr_t H5O_msg_remove_cb(H5O_t *oh, H5O_mesg_t *mesg/*in,out*/,
    unsigned sequence, unsigned *oh_modified, void *_udata/*in,out*/);
static herr_t H5O_copy_mesg(H5F_t *f, hid_t dxpl_id, H5O_t *oh, size_t idx,
    const H5O_msg_class_t *type, const void *mesg, unsigned mesg_flags,
    unsigned update_flags);


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5O_msg_create
 *
 * Purpose:	Create a new object header message
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Dec  1 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_create(const H5O_loc_t *loc, unsigned type_id, unsigned mesg_flags,
    unsigned update_flags, void *mesg, hid_t dxpl_id)
{
    H5O_t *oh = NULL;                   /* Object header */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(loc);
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    HDassert(0 == (mesg_flags & ~H5O_MSG_FLAG_BITS));
    HDassert(mesg);

    /* Pin the object header */
    if(NULL == (oh = H5O_pin(loc, dxpl_id)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPIN, FAIL, "unable to pin object header")

    /* Go append message to object header */
    if(H5O_msg_append_oh(loc->file, dxpl_id, oh, type_id, mesg_flags, update_flags, mesg) < 0)
	HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, FAIL, "unable to append to object header")

done:
    if(oh && H5O_unpin(oh) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPIN, FAIL, "unable to unpin object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_create() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_append_oh
 *
 * Purpose:	Simplified version of H5O_msg_create, used when creating a new
 *              object header message (usually during object creation) and
 *              several messages will be added to the object header at once.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Dec 31 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_append_oh(H5F_t *f, hid_t dxpl_id, H5O_t *oh, unsigned type_id,
    unsigned mesg_flags, unsigned update_flags, void *mesg)
{
    const H5O_msg_class_t *type;        /* Original H5O class type for the ID */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(f);
    HDassert(oh);
    HDassert(H5O_ATTR_ID != type_id);   /* Attributes are modified in another routine */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);
    HDassert(0 == (mesg_flags & ~H5O_MSG_FLAG_BITS));
    HDassert(mesg);

    /* Append new message to object header */
    if(H5O_msg_append_real(f, dxpl_id, oh, type, mesg_flags, update_flags, mesg) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTINSERT, FAIL, "unable to create new message in header")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_append_oh() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_append_real
 *
 * Purpose:	Append a new message to an object header.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Dec  8 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_append_real(H5F_t *f, hid_t dxpl_id, H5O_t *oh, const H5O_msg_class_t *type,
    unsigned mesg_flags, unsigned update_flags, void *mesg)
{
    size_t idx;                         /* Index of message to modify */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(f);
    HDassert(oh);
    HDassert(type);
    HDassert(0 == (mesg_flags & ~H5O_MSG_FLAG_BITS));
    HDassert(mesg);

    /* Allocate space for a new message */
    if(H5O_msg_alloc(f, dxpl_id, oh, type, &mesg_flags, mesg, &idx) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_NOSPACE, FAIL, "unable to create new message")

    /* Copy the information for the message */
    if(H5O_copy_mesg(f, dxpl_id, oh, idx, type, mesg, mesg_flags, update_flags) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to write message")
#ifdef H5O_DEBUG
H5O_assert(oh);
#endif /* H5O_DEBUG */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_append_real() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_write
 *
 * Purpose:	Modifies an existing message or creates a new message.
 *
 *              The UPDATE_FLAGS argument are flags that allow the caller
 *              to skip updating the modification time or reseting the message
 *              data.  This is useful when several calls to H5O_msg_write will be
 *              made in a sequence.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug  6 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_write(const H5O_loc_t *loc, unsigned type_id, unsigned mesg_flags,
    unsigned update_flags, void *mesg, hid_t dxpl_id)
{
    H5O_t *oh = NULL;                   /* Object header to use */
    const H5O_msg_class_t *type;        /* Actual H5O class type for the ID */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(loc);
    HDassert(loc->file);
    HDassert(H5F_addr_defined(loc->addr));
    HDassert(H5O_ATTR_ID != type_id);   /* Attributes are modified in another routine */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);
    HDassert(mesg);
    HDassert(0 == (mesg_flags & ~H5O_MSG_FLAG_BITS));

    /* Pin the object header */
    if(NULL == (oh = H5O_pin(loc, dxpl_id)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPIN, FAIL, "unable to pin object header")

    /* Call the "real" modify routine */
    if(H5O_msg_write_real(loc->file, dxpl_id, oh, type, mesg_flags, update_flags, mesg) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, FAIL, "unable to write object header message")

done:
    if(oh && H5O_unpin(oh) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPIN, FAIL, "unable to unpin object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_write() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_write_oh
 *
 * Purpose:	Modifies an existing message or creates a new message.
 *
 *              The UPDATE_FLAGS argument are flags that allow the caller
 *              to skip updating the modification time or reseting the message
 *              data.  This is useful when several calls to H5O_msg_write will be
 *              made in a sequence.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Dec  6 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_write_oh(H5F_t *f, hid_t dxpl_id, H5O_t *oh, unsigned type_id,
    unsigned mesg_flags, unsigned update_flags, void *mesg)
{
    const H5O_msg_class_t *type;        /* Actual H5O class type for the ID */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(f);
    HDassert(oh);
    HDassert(H5O_ATTR_ID != type_id);   /* Attributes are modified in another routine */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);
    HDassert(mesg);
    HDassert(0 == (mesg_flags & ~H5O_MSG_FLAG_BITS));

    /* Call the "real" modify routine */
    if(H5O_msg_write_real(f, dxpl_id, oh, type, mesg_flags, update_flags, mesg) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, FAIL, "unable to write object header message")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_write_oh() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_write_real
 *
 * Purpose:	Modifies an existing message or creates a new message.
 *
 *              The UPDATE_FLAGS argument are flags that allow the caller
 *              to skip updating the modification time or reseting the message
 *              data.  This is useful when several calls to H5O_msg_write will be
 *              made in a sequence.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug  6 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_write_real(H5F_t *f, hid_t dxpl_id, H5O_t *oh, const H5O_msg_class_t *type,
    unsigned mesg_flags, unsigned update_flags, void *mesg)
{
    H5O_mesg_t         *idx_msg;        /* Pointer to message to modify */
    size_t		idx;            /* Index of message to modify */
    herr_t	        ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f);
    HDassert(oh);
    HDassert(type);
    HDassert(type != H5O_MSG_ATTR);
    HDassert(mesg);
    HDassert(0 == (mesg_flags & ~H5O_MSG_FLAG_BITS));

    /* Locate message of correct type */
    for(idx = 0, idx_msg = &oh->mesg[0]; idx < oh->nmesgs; idx++, idx_msg++)
	if(type == idx_msg->type)
            break;
    if(idx == oh->nmesgs)
        HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "message type not found")

    /* Check for modifying a constant message */
    if(!(update_flags & H5O_UPDATE_FORCE) && (idx_msg->flags & H5O_MSG_FLAG_CONSTANT))
	HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, FAIL, "unable to modify constant message")
    /* This message is shared, but it's being modified. */
    else if((idx_msg->flags & H5O_MSG_FLAG_SHARED) || (idx_msg->flags & H5O_MSG_FLAG_SHAREABLE)) {
        htri_t status;              /* Status of "try share" call */

         /* First, sanity check to make sure it's not a committed message;
          *     these can't ever be modified.
          */
        HDassert(((H5O_shared_t *)idx_msg->native)->type != H5O_SHARE_TYPE_COMMITTED);

        /* Also, sanity check that a message doesn't switch status from being
         *      shared (or sharable) to being unsharable.  (Which could cause
         *      a message to increase in size in the object header)
         */
        HDassert(!(mesg_flags & H5O_MSG_FLAG_DONTSHARE));

        /* Remove the old message from the SOHM index */
        /* (It would be more efficient to try to share the message first, then
         *      delete it (avoiding thrashing the index in the case the ref.
         *      count on the message is one), but this causes problems when
         *      the location of the object changes (from in another object's
         *      header to the SOHM heap), so just delete it first -QAK)
         */
        if(H5SM_delete(f, dxpl_id, oh, (H5O_shared_t *)idx_msg->native) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "unable to delete message from SOHM index")

        /* If we're replacing a shared message, the new message must be shared
         * (or else it may increase in size!), so pass in NULL for the OH
         * location.
         *
         * XXX: This doesn't handle freeing extra space in object header from
         *      a message shrinking.
         */
        if((status = H5SM_try_share(f, dxpl_id, ((mesg_flags & H5O_MSG_FLAG_SHARED) ? NULL : oh), 0, idx_msg->type->id, mesg, &mesg_flags)) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_BADMESG, FAIL, "error while trying to share message")
        if(status == FALSE && (mesg_flags & H5O_MSG_FLAG_SHARED))
            HGOTO_ERROR(H5E_OHDR, H5E_BADMESG, FAIL, "message changed sharing status")
    } /* end if */

    /* Copy the information for the message */
    if(H5O_copy_mesg(f, dxpl_id, oh, idx, type, mesg, mesg_flags, update_flags) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to write message")
#ifdef H5O_DEBUG
H5O_assert(oh);
#endif /* H5O_DEBUG */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_write_real() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_read
 *
 * Purpose:	Reads a message from an object header and returns a pointer
 *		to it.	The caller will usually supply the memory through
 *		MESG and the return value will be MESG.	 But if MESG is
 *		the null pointer, then this function will malloc() memory
 *		to hold the result and return its pointer instead.
 *
 * Return:	Success:	Ptr to message in native format.  The message
 *				should be freed by calling H5O_msg_reset().  If
 *				MESG is a null pointer then the caller should
 *				also call H5MM_xfree() on the return value
 *				after calling H5O_msg_reset().
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug  6 1997
 *
 *-------------------------------------------------------------------------
 */
void *
H5O_msg_read(const H5O_loc_t *loc, unsigned type_id, void *mesg,
    hid_t dxpl_id)
{
    H5O_t *oh = NULL;                   /* Object header to use */
    void *ret_value;                    /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* check args */
    HDassert(loc);
    HDassert(loc->file);
    HDassert(H5F_addr_defined(loc->addr));
    HDassert(type_id < NELMTS(H5O_msg_class_g));

    /* Get the object header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_READ)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, NULL, "unable to protect object header")

    /* Call the "real" read routine */
    if(NULL == (ret_value = H5O_msg_read_oh(loc->file, dxpl_id, oh, type_id, mesg)))
	HGOTO_ERROR(H5E_OHDR, H5E_READERROR, NULL, "unable to read object header message")

done:
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, NULL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_read() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_read_oh
 *
 * Purpose:	Reads a message from an object header and returns a pointer
 *		to it.	The caller will usually supply the memory through
 *		MESG and the return value will be MESG.	 But if MESG is
 *		the null pointer, then this function will malloc() memory
 *		to hold the result and return its pointer instead.
 *
 * Return:	Success:	Ptr to message in native format.  The message
 *				should be freed by calling H5O_msg_reset().  If
 *				MESG is a null pointer then the caller should
 *				also call H5MM_xfree() on the return value
 *				after calling H5O_msg_reset().
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug  6 1997
 *
 *-------------------------------------------------------------------------
 */
void *
H5O_msg_read_oh(H5F_t *f, hid_t dxpl_id, H5O_t *oh, unsigned type_id,
    void *mesg)
{
    const H5O_msg_class_t *type;        /* Actual H5O class type for the ID */
    unsigned       idx;                 /* Message's index in object header */
    void           *ret_value = NULL;

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f);
    HDassert(oh);
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);

    /* Scan through the messages looking for the right one */
    for(idx = 0; idx < oh->nmesgs; idx++)
	if(type == oh->mesg[idx].type)
            break;
    if(idx == oh->nmesgs)
        HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, NULL, "message type not found")

    /*
     * Decode the message if necessary.  If the message is shared then retrieve
     * native message through the shared interface.
     */
    H5O_LOAD_NATIVE(f, dxpl_id, 0, oh, &(oh->mesg[idx]), NULL)

    /*
     * The object header caches the native message (along with
     * the raw message) so we must copy the native message before
     * returning.
     */
    if(NULL == (ret_value = (type->copy)(oh->mesg[idx].native, mesg)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "unable to copy message to user space")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_read_oh() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_reset
 *
 * Purpose:	Some message data structures have internal fields that
 *		need to be freed.  This function does that if appropriate
 *		but doesn't free NATIVE.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug 12 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_reset(unsigned type_id, void *native)
{
    const H5O_msg_class_t *type;        /* Actual H5O class type for the ID */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);

    /* Call the "real" reset routine */
    if(H5O_msg_reset_real(type, native) < 0)
	HGOTO_ERROR(H5E_OHDR, H5E_CANTRESET, FAIL, "unable to reset object header")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_reset() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_reset_real
 *
 * Purpose:	Some message data structures have internal fields that
 *		need to be freed.  This function does that if appropriate
 *		but doesn't free NATIVE.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug 12 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_msg_reset_real(const H5O_msg_class_t *type, void *native)
{
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(type);

    if(native) {
	if(type->reset) {
	    if((type->reset)(native) < 0)
		HGOTO_ERROR(H5E_OHDR, H5E_CANTRELEASE, FAIL, "reset method failed")
	} /* end if */
        else
	    HDmemset(native, 0, type->native_size);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_reset_real() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_free
 *
 * Purpose:	Similar to H5O_msg_reset() except it also frees the message
 *		pointer.
 *
 * Return:	Success:	NULL
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 *-------------------------------------------------------------------------
 */
void *
H5O_msg_free(unsigned type_id, void *mesg)
{
    const H5O_msg_class_t *type;            /* Actual H5O class type for the ID */
    void * ret_value;                   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);

    /* Call the "real" free routine */
    ret_value = H5O_msg_free_real(type, mesg);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_free() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_free_mesg
 *
 * Purpose:	Call H5O_msg_free_real() on a message.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, Sep  6, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_free_mesg(H5O_mesg_t *mesg)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(mesg);

    /* Free any native information */
    mesg->native = H5O_msg_free_real(mesg->type, mesg->native);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_msg_free_mesg() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_free_real
 *
 * Purpose:	Similar to H5O_msg_reset() except it also frees the message
 *		pointer.
 *
 * Return:	Success:	NULL
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 *-------------------------------------------------------------------------
 */
void *
H5O_msg_free_real(const H5O_msg_class_t *type, void *msg_native)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(type);

    if(msg_native) {
        H5O_msg_reset_real(type, msg_native);
        if(NULL != (type->free))
            (type->free)(msg_native);
        else
            H5MM_xfree(msg_native);
    } /* end if */

    FUNC_LEAVE_NOAPI(NULL)
} /* end H5O_msg_free_real() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_copy
 *
 * Purpose:	Copies a message.  If MESG is is the null pointer then a null
 *		pointer is returned with no error.
 *
 * Return:	Success:	Ptr to the new message
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Thursday, May 21, 1998
 *
 *-------------------------------------------------------------------------
 */
void *
H5O_msg_copy(unsigned type_id, const void *mesg, void *dst)
{
    const H5O_msg_class_t *type;            /* Actual H5O class type for the ID */
    void	*ret_value;             /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* check args */
    HDassert(mesg);
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);

    /* Call the message class's copy routine */
    if(NULL == (ret_value = (type->copy)(mesg, dst)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "unable to copy object header message")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_copy() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_count
 *
 * Purpose:	Counts the number of messages in an object header which are a
 *		certain type.
 *
 * Return:	Success:	Number of messages of specified type.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Tuesday, April 21, 1998
 *
 *-------------------------------------------------------------------------
 */
int
H5O_msg_count(const H5O_loc_t *loc, unsigned type_id, hid_t dxpl_id)
{
    H5O_t *oh = NULL;           /* Object header to operate on */
    const H5O_msg_class_t *type;        /* Actual H5O class type for the ID */
    unsigned msg_count;         /* Message count */
    int	ret_value;              /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(loc);
    HDassert(loc->file);
    HDassert(H5F_addr_defined(loc->addr));
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);

    /* Load the object header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_READ)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to protect object header")

    /* Count the messages of the correct type */
    msg_count = H5O_msg_count_real(oh, type);
    H5_ASSIGN_OVERFLOW(ret_value, msg_count, unsigned, int);

done:
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_count() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_count_real
 *
 * Purpose:	Counts the number of messages in an object header which are a
 *		certain type.
 *
 * Return:	Success:	Number of messages of specified type.
 *
 *		Failure:	(can't fail)
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, February  6, 2007
 *
 *-------------------------------------------------------------------------
 */
unsigned
H5O_msg_count_real(const H5O_t *oh, const H5O_msg_class_t *type)
{
    unsigned u;                 /* Local index variable */
    unsigned ret_value;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(oh);
    HDassert(type);

    /* Loop over all messages, counting the ones of the type looked for */
    for(u = ret_value = 0; u < oh->nmesgs; u++)
	if(oh->mesg[u].type == type)
            ret_value++;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_count_real() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_exists
 *
 * Purpose:	Determines if a particular message exists in an object
 *		header without trying to decode the message.
 *
 * Return:	Success:	FALSE if the message does not exist; TRUE if
 *				th message exists.
 *
 *		Failure:	FAIL if the existence of the message could
 *				not be determined due to some error such as
 *				not being able to read the object header.
 *
 * Programmer:	Robb Matzke
 *              Monday, November  2, 1998
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5O_msg_exists(const H5O_loc_t *loc, unsigned type_id, hid_t dxpl_id)
{
    H5O_t	*oh = NULL;             /* Object header for location */
    htri_t      ret_value;              /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(loc);
    HDassert(loc->file);
    HDassert(type_id < NELMTS(H5O_msg_class_g));

    /* Load the object header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_READ)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to protect object header")

    /* Call the "real" exists routine */
    if((ret_value = H5O_msg_exists_oh(oh, type_id)) < 0)
	HGOTO_ERROR(H5E_OHDR, H5E_READERROR, FAIL, "unable to verify object header message")

done:
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_exists() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_exists_oh
 *
 * Purpose:	Determines if a particular message exists in an object
 *		header without trying to decode the message.
 *
 * Return:	Success:	FALSE if the message does not exist; TRUE if
 *				th message exists.
 *
 *		Failure:	FAIL if the existence of the message could
 *				not be determined due to some error such as
 *				not being able to read the object header.
 *
 * Programmer:	Robb Matzke
 *              Monday, November  2, 1998
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5O_msg_exists_oh(const H5O_t *oh, unsigned type_id)
{
    const H5O_msg_class_t *type;        /* Actual H5O class type for the ID */
    unsigned	u;                      /* Local index variable */
    htri_t      ret_value = FALSE;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(oh);
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);

    /* Scan through the messages looking for the right one */
    for(u = 0; u < oh->nmesgs; u++)
	if(type == oh->mesg[u].type)
            HGOTO_DONE(TRUE)

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_exists_oh() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_remove
 *
 * Purpose:	Removes the specified message from the object header.
 *		If sequence is H5O_ALL (-1) then all messages of the
 *		specified type are removed.  Removing a message causes
 *		the sequence numbers to change for subsequent messages of
 *		the same type.
 *
 *		No attempt is made to join adjacent free areas of the
 *		object header into a single larger free area.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug 28 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_remove(const H5O_loc_t *loc, unsigned type_id, int sequence, hbool_t adj_link,
    hid_t dxpl_id)
{
    H5O_t *oh = NULL;                   /* Pointer to actual object header */
    const H5O_msg_class_t *type;            /* Actual H5O class type for the ID */
    herr_t      ret_value;              /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(loc);
    HDassert(loc->file);
    HDassert(H5F_addr_defined(loc->addr));
    HDassert(H5O_ATTR_ID != type_id);   /* Attributes are modified in another routine */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);

    /* Pin the object header */
    if(NULL == (oh = H5O_pin(loc, dxpl_id)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPIN, FAIL, "unable to pin object header")

    /* Call the "real" remove routine */
    if((ret_value = H5O_msg_remove_real(loc->file, oh, type, sequence, NULL, NULL, adj_link, dxpl_id)) < 0)
	HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "unable to remove object header message")

done:
    if(oh && H5O_unpin(oh) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPIN, FAIL, "unable to unpin object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_remove() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_remove_op
 *
 * Purpose:	Removes messages from the object header that a callback
 *              routine indicates should be removed.
 *
 *		No attempt is made to join adjacent free areas of the
 *		object header into a single larger free area.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Sep  6 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_remove_op(const H5O_loc_t *loc, unsigned type_id, int sequence,
    H5O_operator_t op, void *op_data, hbool_t adj_link, hid_t dxpl_id)
{
    H5O_t *oh = NULL;                   /* Pointer to actual object header */
    const H5O_msg_class_t *type;            /* Actual H5O class type for the ID */
    herr_t      ret_value;              /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(loc);
    HDassert(loc->file);
    HDassert(H5F_addr_defined(loc->addr));
    HDassert(H5O_ATTR_ID != type_id);   /* Attributes are modified in another routine */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);

    /* Pin the object header */
    if(NULL == (oh = H5O_pin(loc, dxpl_id)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPIN, FAIL, "unable to pin object header")

    /* Call the "real" remove routine */
    if((ret_value = H5O_msg_remove_real(loc->file, oh, type, sequence, op, op_data, adj_link, dxpl_id)) < 0)
	HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "unable to remove object header message")

done:
    if(oh && H5O_unpin(oh) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPIN, FAIL, "unable to unpin object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_remove_op() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_remove_cb
 *
 * Purpose:	Object header iterator callback routine to remove messages
 *              of a particular type that match a particular sequence number,
 *              or all messages if the sequence number is H5O_ALL (-1).
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Sep  6 2005
 *
 * Modifications:
 *	Vailin Choi; Sept 2011
 *	Indicate that the object header is modified and might possibly need
 *	to condense messages in the object header 
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_msg_remove_cb(H5O_t *oh, H5O_mesg_t *mesg/*in,out*/, unsigned sequence,
    unsigned *oh_modified, void *_udata/*in,out*/)
{
    H5O_iter_rm_t *udata = (H5O_iter_rm_t *)_udata;   /* Operator user data */
    htri_t try_remove = FALSE;         /* Whether to try removing a message */
    herr_t ret_value = H5_ITER_CONT;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(mesg);

    /* Check for callback routine */
    if(udata->op) {
        /* Call the iterator callback */
        if((try_remove = (udata->op)(mesg->native, sequence, udata->op_data)) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, H5_ITER_ERROR, "object header message deletion callback failed")
    } /* end if */
    else {
        /* If there's no callback routine, does the sequence # match? */
        if((int)sequence == udata->sequence || H5O_ALL == udata->sequence)
            try_remove = TRUE;
    } /* end else */

    /* Try removing the message, if indicated */
    if(try_remove) {
        /*
         * Keep track of how many times we failed trying to remove constant
         * messages.
         * (OK to remove constant messages - QAK)
         */
        /* Convert message into a null message */
        if(H5O_release_mesg(udata->f, udata->dxpl_id, oh, mesg, udata->adj_link) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, H5_ITER_ERROR, "unable to release message")

        /* Indicate that the object header was modified & might need to condense messages in object header */
        *oh_modified = H5O_MODIFY_CONDENSE;

        /* Break out now, if we've found the correct message */
        if(udata->sequence == H5O_FIRST || udata->sequence != H5O_ALL)
            HGOTO_DONE(H5_ITER_STOP)
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_remove_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_remove_real
 *
 * Purpose:	Removes the specified message from the object header.
 *		If sequence is H5O_ALL (-1) then all messages of the
 *		specified type are removed.  Removing a message causes
 *		the sequence numbers to change for subsequent messages of
 *		the same type.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug 28 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_remove_real(H5F_t *f, H5O_t *oh, const H5O_msg_class_t *type,
    int sequence, H5O_operator_t app_op, void *op_data, hbool_t adj_link,
    hid_t dxpl_id)
{
    H5O_iter_rm_t udata;                /* User data for iterator */
    H5O_mesg_operator_t op;             /* Wrapper for operator */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(f);
    HDassert(oh);
    HDassert(type);

    /* Make certain we are allowed to modify the file */
    if(0 == (H5F_INTENT(f) & H5F_ACC_RDWR))
	HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, FAIL, "no write intent on file")

    /* Set up iterator operator data */
    udata.f = f;
    udata.dxpl_id = dxpl_id;
    udata.sequence = sequence;
    udata.nfailed = 0;
    udata.op = app_op;
    udata.op_data = op_data;
    udata.adj_link = adj_link;

    /* Iterate over the messages, deleting appropriate one(s) */
    op.op_type = H5O_MESG_OP_LIB;
    op.u.lib_op = H5O_msg_remove_cb;
    if(H5O_msg_iterate_real(f, oh, type, &op, &udata, dxpl_id) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "error iterating over messages")

    /* Fail if we tried to remove any constant messages */
    if(udata.nfailed)
	HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to remove constant message(s)")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_remove_real() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_iterate
 *
 * Purpose:	Iterate through object headers of a certain type.
 *
 * Return:	Returns a negative value if something is wrong, the return
 *      value of the last operator if it was non-zero, or zero if all
 *      object headers were processed.
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Nov 19 2004
 *
 * Description:
 *      This function interates over the object headers of an object
 *  specified with 'loc' of type 'type_id'.  For each object header of the
 *  object, the 'op_data' and some additional information (specified below) are
 *  passed to the 'op' function.
 *      The operation receives a pointer to the object header message for the
 *  object being iterated over ('mesg'), and the pointer to the operator data
 *  passed in to H5O_msg_iterate ('op_data').  The return values from an operator
 *  are:
 *      A. Zero causes the iterator to continue, returning zero when all
 *          object headers of that type have been processed.
 *      B. Positive causes the iterator to immediately return that positive
 *          value, indicating short-circuit success.
 *      C. Negative causes the iterator to immediately return that value,
 *          indicating failure.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_iterate(const H5O_loc_t *loc, unsigned type_id,
    const H5O_mesg_operator_t *op, void *op_data, hid_t dxpl_id)
{
    H5O_t *oh = NULL;               /* Pointer to actual object header */
    const H5O_msg_class_t *type;    /* Actual H5O class type for the ID */
    herr_t ret_value;               /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(loc);
    HDassert(loc->file);
    HDassert(H5F_addr_defined(loc->addr));
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);
    HDassert(op);

    /* Protect the object header to iterate over */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_READ)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to protect object header")

    /* Call the "real" iterate routine */
    if((ret_value = H5O_msg_iterate_real(loc->file, oh, type, op, op_data, dxpl_id)) < 0)
        HERROR(H5E_OHDR, H5E_BADITER, "unable to iterate over object header messages");

done:
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_iterate() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_iterate_real
 *
 * Purpose:	Iterate through object headers of a certain type.
 *
 * Return:	Returns a negative value if something is wrong, the return
 *      value of the last operator if it was non-zero, or zero if all
 *      object headers were processed.
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Sep  6 2005
 *
 * Description:
 *      This function interates over the object headers of an object
 *  specified with 'ent' of type 'type_id'.  For each object header of the
 *  object, the 'op_data' and some additional information (specified below) are
 *  passed to the 'op' function.
 *      The operation receives a pointer to the object header message for the
 *  object being iterated over ('mesg'), and the pointer to the operator data
 *  passed in to H5O_msg_iterate ('op_data').  The return values from an operator
 *  are:
 *      A. Zero causes the iterator to continue, returning zero when all
 *          object headers of that type have been processed.
 *      B. Positive causes the iterator to immediately return that positive
 *          value, indicating short-circuit success.
 *      C. Negative causes the iterator to immediately return that value,
 *          indicating failure.
 *
 * Modifications:
 *	Vailin Choi; September 2011
 *	Change "oh_modified" from boolean to unsigned so as to know:
 *	1) object header is just modified
 *	2) object header is modified and possibly need to condense messages there
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_iterate_real(H5F_t *f, H5O_t *oh, const H5O_msg_class_t *type,
    const H5O_mesg_operator_t *op, void *op_data, hid_t dxpl_id)
{
    H5O_mesg_t         *idx_msg;        /* Pointer to current message */
    unsigned		idx;            /* Absolute index of current message in all messages */
    unsigned		sequence;       /* Relative index of current message for messages of type */
    unsigned		oh_modified = 0;    /* Whether the callback modified the object header */
    herr_t              ret_value = H5_ITER_CONT;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f);
    HDassert(oh);
    HDassert(type);
    HDassert(op);
    HDassert(op->u.app_op);

    /* Iterate over messages */
    for(sequence = 0, idx = 0, idx_msg = &oh->mesg[0]; idx < oh->nmesgs && !ret_value; idx++, idx_msg++) {
	if(type == idx_msg->type) {
            /* Decode the message if necessary.  */
            H5O_LOAD_NATIVE(f, dxpl_id, 0, oh, idx_msg, FAIL)

            /* Check for making an "internal" (i.e. within the H5O package) callback */
            if(op->op_type == H5O_MESG_OP_LIB)
                ret_value = (op->u.lib_op)(oh, idx_msg, sequence, &oh_modified, op_data);
            else
                ret_value = (op->u.app_op)(idx_msg->native, sequence, op_data);

            /* Check for iterator callback indicating to get out of loop */
            if(ret_value != 0)
                break;

            /* Increment sequence value for message type */
            sequence++;
        } /* end if */
    } /* end for */

    /* Check for error from iterator */
    if(ret_value < 0)
        HERROR(H5E_OHDR, H5E_CANTLIST, "iterator function failed");

done:
    /* Check if object message was modified */
    if(oh_modified) {
        /* Try to condense object header info if the flag indicates so */
        /* (Since this routine is used to remove messages from an
         *  object header, the header will be condensed after each
         *  message removal)
         */
	if(oh_modified & H5O_MODIFY_CONDENSE) {
	    if(H5O_condense_header(f, oh, dxpl_id) < 0)
		HDONE_ERROR(H5E_OHDR, H5E_CANTPACK, FAIL, "can't pack object header")
	}

        /* Mark object header as changed */
        if(H5O_touch_oh(f, dxpl_id, oh, FALSE) < 0)
            HDONE_ERROR(H5E_OHDR, H5E_CANTUPDATE, FAIL, "unable to update time on object")

        /* Mark object header as dirty in cache */
        if(H5AC_mark_entry_dirty(oh) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTMARKDIRTY, FAIL, "unable to mark object header as dirty")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_iterate_real() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_raw_size
 *
 * Purpose:	Call the 'raw_size' method for a
 *              particular class of object header.
 *
 * Return:	Size of message on success, 0 on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb 13 2003
 *
 *-------------------------------------------------------------------------
 */
size_t
H5O_msg_raw_size(const H5F_t *f, unsigned type_id, hbool_t disable_shared,
    const void *mesg)
{
    const H5O_msg_class_t *type;        /* Actual H5O class type for the ID */
    size_t ret_value;                   /* Return value */

    FUNC_ENTER_NOAPI(0)

    /* Check args */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);
    HDassert(type->raw_size);
    HDassert(f);
    HDassert(mesg);

    /* Compute the raw data size for the mesg */
    if(0 == (ret_value = (type->raw_size)(f, disable_shared, mesg)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOUNT, 0, "unable to determine size of message")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_raw_size() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_size_f
 *
 * Purpose:	Calculate the final size of an encoded message in an object
 *              header.
 *
 * Note:	This routine assumes that the message size will be used in the
 *              creation of a new object header.
 *
 * Return:	Size of message on success, 0 on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Sep  6 2005
 *
 *-------------------------------------------------------------------------
 */
size_t
H5O_msg_size_f(const H5F_t *f, hid_t ocpl_id, unsigned type_id,
    const void *mesg, size_t extra_raw)
{
    const H5O_msg_class_t *type; /* Actual H5O class type for the ID */
    H5P_genplist_t *ocpl;       /* Object Creation Property list */
    uint8_t oh_flags;           /* Object header status flags */
    size_t ret_value;           /* Return value */

    FUNC_ENTER_NOAPI(0)

    /* Check args */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);
    HDassert(type->raw_size);
    HDassert(f);
    HDassert(mesg);

    /* Get the property list */
    if(NULL == (ocpl = (H5P_genplist_t *)H5I_object(ocpl_id)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, 0, "not a property list")

    /* Get any object header status flags set by properties */
    if(H5P_get(ocpl, H5O_CRT_OHDR_FLAGS_NAME, &oh_flags) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, 0, "can't get object header flags")


    /* Compute the raw data size for the mesg */
    if((ret_value = (type->raw_size)(f, FALSE, mesg)) == 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOUNT, 0, "unable to determine size of message")

    /* Add in "extra" raw space */
    ret_value += extra_raw;

    /* Adjust size for alignment, if necessary */
    ret_value = (size_t)H5O_ALIGN_F(f, ret_value);

    /* Add space for message header */
    ret_value += (size_t)H5O_SIZEOF_MSGHDR_F(f,
            (H5F_STORE_MSG_CRT_IDX(f) || oh_flags & H5O_HDR_ATTR_CRT_ORDER_TRACKED));

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_size_f() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_size_oh
 *
 * Purpose:	Calculate the final size of an encoded message in an object
 *              header.
 *
 * Note:	This routine assumes that the message is already used in
 *              an object header.
 *
 * Return:	Size of message on success, 0 on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Mar  7 2007
 *
 *-------------------------------------------------------------------------
 */
size_t
H5O_msg_size_oh(const H5F_t *f, const H5O_t *oh, unsigned type_id,
    const void *mesg, size_t extra_raw)
{
    const H5O_msg_class_t *type; /* Actual H5O class type for the ID */
    size_t ret_value;           /* Return value */

    FUNC_ENTER_NOAPI(0)

    /* Check args */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);
    HDassert(type->raw_size);
    HDassert(f);
    HDassert(mesg);

    /* Compute the raw data size for the mesg */
    if((ret_value = (type->raw_size)(f, FALSE, mesg)) == 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOUNT, 0, "unable to determine size of message")

    /* Add in "extra" raw space */
    ret_value += extra_raw;

    /* Adjust size for alignment, if necessary */
    ret_value = (size_t)H5O_ALIGN_OH(oh, ret_value);

    /* Add space for message header */
    ret_value += (size_t)H5O_SIZEOF_MSGHDR_OH(oh);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_size_oh() */


/*-------------------------------------------------------------------------
 * Function:    H5O_msg_can_share
 *
 * Purpose:     Call the 'can share' method for a
 *              particular class of object header.  This returns TRUE
 *              if the message is allowed to be put in the shared message
 *              heap and false otherwise (e.g., for committed or immutable
 *              datatypes).
 *
 * Return:      Object can be shared:        TRUE
 *              Object cannot be shared:    FALSE
 *
 * Programmer:  James Laird
 *              January 12 2007
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5O_msg_can_share(unsigned type_id, const void *mesg)
{
    const H5O_msg_class_t *type;    /* Actual H5O class type for the ID */
    htri_t ret_value;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);
    HDassert(mesg);

    /* If there is a can_share callback, use it */
    if(type->can_share)
        ret_value = (type->can_share)(mesg);
    else {
        /* Otherwise, the message can be shared if messages of this type are
         * shareable in general; i.e., if they have the "is_sharable" flag
         * in the "share_flags" class member set.
         */
        ret_value = (type->share_flags & H5O_SHARE_IS_SHARABLE) ? TRUE : FALSE;
    } /* end else */

    /* If the message is shareable, both copy_file and post_copy_file must be
     * defined */
    HDassert((type->post_copy_file && type->copy_file) || ret_value == FALSE);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_can_share() */


/*-------------------------------------------------------------------------
 * Function:    H5O_msg_can_share_in_ohdr
 *
 * Purpose:     Check if the message class allows its messages to be shared
 *              in the object's header.
 *
 * Return:      Object can be shared:        TRUE
 *              Object cannot be shared:    FALSE
 *
 * Programmer:  Quincey Koziol
 *              March 15 2007
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5O_msg_can_share_in_ohdr(unsigned type_id)
{
    const H5O_msg_class_t *type;    /* Actual H5O class type for the ID */
    htri_t ret_value;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);

    /* Otherwise, the message can be shared if messages of this type are
     * shareable in general; i.e., if they have the "is_sharable" flag
     * in the "share_flags" class member set.
     */
    ret_value = (type->share_flags & H5O_SHARE_IN_OHDR) ? TRUE : FALSE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_can_share_in_ohdr() */


/*-------------------------------------------------------------------------
 * Function:    H5O_msg_is_shared
 *
 * Purpose:     Call the 'is_shared' method for a
 *              particular class of object header.
 *
 * Return:      Object is shared:        TRUE
 *              Object is not shared:    FALSE
 *
 * Programmer:  James Laird
 *              jlaird@ncsa.uiuc.edu
 *              April 5 2006
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5O_msg_is_shared(unsigned type_id, const void *mesg)
{
    const H5O_msg_class_t *type;    /* Actual H5O class type for the ID */
    htri_t ret_value;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);
    HDassert(mesg);

    /* If messages in a class aren't sharable, then obviously this message isn't shared! :-) */
    if(type->share_flags & H5O_SHARE_IS_SHARABLE)
        ret_value = H5O_IS_STORED_SHARED(((const H5O_shared_t *)mesg)->type);
    else
        ret_value = FALSE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_is_shared() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_set_share
 *
 * Purpose:	Set the shared information for an object header message.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	James Laird
 *		jlaird@hdfgroup.org
 *		November 1 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_set_share(unsigned type_id, const H5O_shared_t *share, void *mesg)
{
    const H5O_msg_class_t *type;        /* Actual H5O class type for the ID */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);
    HDassert(type->share_flags & H5O_SHARE_IS_SHARABLE);
    HDassert(mesg);
    HDassert(share);
    HDassert(share->type != H5O_SHARE_TYPE_UNSHARED);

    /* If there's a special action for this class that needs to be performed
     *  when setting the shared component, do that
     */
    if(type->set_share) {
        if((type->set_share)(mesg, share) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "unable to set shared message information")
    } /* end if */
    else {
        /* Set this message as the shared component for the message, wiping out
         * any information that was there before
         */
        if(H5O_set_shared((H5O_shared_t *)mesg, share) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "unable to set shared message information")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_set_share() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_reset_share
 *
 * Purpose:	Reset the shared information for an object header message.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	James Laird
 *		jlaird@hdfgroup.org
 *		Oct 17 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_reset_share(unsigned type_id, void *mesg)
{
    const H5O_msg_class_t *type;        /* Actual H5O class type for the ID */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);
    HDassert(type->share_flags & H5O_SHARE_IS_SHARABLE);
    HDassert(mesg);

    /* Reset the shared component in the message to zero. */
    HDmemset((H5O_shared_t *)mesg, 0, sizeof(H5O_shared_t));

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_msg_reset_share() */


/*-------------------------------------------------------------------------
 * Function:    H5O_msg_get_crt_index
 *
 * Purpose:     Call the 'get creation index' method for a message.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              March 15 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_get_crt_index(unsigned type_id, const void *mesg, H5O_msg_crt_idx_t *crt_idx)
{
    const H5O_msg_class_t *type;    /* Actual H5O class type for the ID */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);
    HDassert(mesg);
    HDassert(crt_idx);

    /* If there is a "get_crt_index callback, use it */
    if(type->get_crt_index) {
        /* Retrieve the creation index from the native message */
        if((type->get_crt_index)(mesg, crt_idx) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to retrieve creation index")
    } /* end if */
    else
        *crt_idx = 0;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_get_crt_index() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_encode
 *
 * Purpose:	Encode an object(data type and simple data space only)
 *              description into a buffer.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Raymond Lu
 *		slu@ncsa.uiuc.edu
 *		July 13, 2004
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_encode(H5F_t *f, unsigned type_id, hbool_t disable_shared,
    unsigned char *buf, const void *mesg)
{
    const H5O_msg_class_t *type;        /* Actual H5O class type for the ID */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(f);
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);

    /* Encode */
    if((type->encode)(f, disable_shared, buf, mesg) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTENCODE, FAIL, "unable to encode message")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_encode() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_decode
 *
 * Purpose:	Decode a binary object description and return a new
 *              object handle.
 *
 * Return:	Success:        Pointer to object(data type or space)
 *
 *		Failure:	NULL
 *
 * Programmer:	Raymond Lu
 *		slu@ncsa.uiuc.edu
 *		July 14, 2004
 *
 * Modifications: Neil Fortner
 *              Feb 4 2009
 *              Added open_oh parameter.  This parameter is optional and
 *              contains this message's protected object header
 *
 *-------------------------------------------------------------------------
 */
void *
H5O_msg_decode(H5F_t *f, hid_t dxpl_id, H5O_t *open_oh, unsigned type_id,
    const unsigned char *buf)
{
    const H5O_msg_class_t   *type;      /* Actual H5O class type for the ID */
    void *ret_value;                    /* Return value */
    unsigned ioflags = 0;               /* Flags for decode routine */

    FUNC_ENTER_NOAPI(NULL)

    /* check args */
    HDassert(f);
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);

    /* decode */
    if((ret_value = (type->decode)(f, dxpl_id, open_oh, 0, &ioflags, buf)) == NULL)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTDECODE, NULL, "unable to decode message")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_decode() */


/*-------------------------------------------------------------------------
 * Function:    H5O_msg_copy_file
 *
 * Purpose:     Copies a message to file.  If MESG is is the null pointer then a null
 *              pointer is returned with no error.
 *
 *              Attempts to share the message in the destination and sets
 *              SHARED to TRUE or FALSE depending on whether this succeeds.
 *
 * Return:      Success:        Ptr to the new message
 *
 *              Failure:        NULL
 *
 * Programmer:  Peter Cao
 *              June 4, 2005
 *
 *-------------------------------------------------------------------------
 */
void *
H5O_msg_copy_file(const H5O_msg_class_t *type, H5F_t *file_src,
    void *native_src, H5F_t *file_dst, hbool_t *recompute_size,
    unsigned *mesg_flags, H5O_copy_t *cpy_info, void *udata, hid_t dxpl_id)
{
    void        *ret_value;

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(type);
    HDassert(type->copy_file);
    HDassert(file_src);
    HDassert(native_src);
    HDassert(file_dst);
    HDassert(recompute_size);
    HDassert(cpy_info);

    /* The copy_file callback will return an H5O_shared_t only if the message
     * to be copied is a committed datatype.
     */
    if(NULL == (ret_value = (type->copy_file)(file_src, native_src, file_dst, recompute_size, mesg_flags, cpy_info, udata, dxpl_id)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, NULL, "unable to copy object header message to file")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_copy_file() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_alloc
 *
 * Purpose:	Create a new message in an object header
 *
 * Return:	Success:	Index of message
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Friday, September  3, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_alloc(H5F_t *f, hid_t dxpl_id, H5O_t *oh, const H5O_msg_class_t *type,
    unsigned *mesg_flags, void *native, size_t *mesg_idx)
{
    size_t new_idx;             /* New index for message */
    htri_t shared_mesg;         /* Should this message be stored in the Shared Message table? */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f);
    HDassert(oh);
    HDassert(mesg_flags);
    HDassert(!(*mesg_flags & H5O_MSG_FLAG_SHARED));
    HDassert(type);
    HDassert(native);
    HDassert(mesg_idx);

    /* Check if message is already shared */
    if((shared_mesg = H5O_msg_is_shared(type->id, native)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "error determining if message is shared")
    else if(shared_mesg > 0) {
        /* Increment message's reference count */
        if(type->link && (type->link)(f, dxpl_id, oh, native) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_LINKCOUNT, FAIL, "unable to adjust shared message ref count")
        *mesg_flags |= H5O_MSG_FLAG_SHARED;
    } /* end if */
    else {
        /* Attempt to share message */
        if(H5SM_try_share(f, dxpl_id, oh, 0, type->id, native, mesg_flags) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, FAIL, "error determining if message should be shared")
    } /* end else */

    /* Allocate space in the object header for the message */
    if(H5O_alloc(f, dxpl_id, oh, type, native, &new_idx) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to allocate space for message")

    /* Get the message's "creation index", if it has one */
    if(type->get_crt_index) {
        /* Retrieve the creation index from the native message */
        if((type->get_crt_index)(native, &oh->mesg[new_idx].crt_idx) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to retrieve creation index")
    } /* end if */

    /* Set new message index */
    *mesg_idx = new_idx;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5O_copy_mesg
 *
 * Purpose:	Make a copy of the native object for an object header's
 *              native message info
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Friday, September  3, 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_copy_mesg(H5F_t *f, hid_t dxpl_id, H5O_t *oh, size_t idx,
    const H5O_msg_class_t *type, const void *mesg, unsigned mesg_flags,
    unsigned update_flags)
{
    H5O_chunk_proxy_t *chk_proxy = NULL;        /* Chunk that message is in */
    H5O_mesg_t *idx_msg = &oh->mesg[idx];       /* Pointer to message to modify */
    hbool_t chk_dirtied = FALSE;                /* Flag for unprotecting chunk */
    herr_t      ret_value = SUCCEED;            /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f);
    HDassert(oh);
    HDassert(type);
    HDassert(type->copy);
    HDassert(mesg);

    /* Protect chunk */
    if(NULL == (chk_proxy = H5O_chunk_protect(f, dxpl_id, oh, idx_msg->chunkno)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to protect object header chunk")

    /* Reset existing native information for the header's message */
    H5O_msg_reset_real(type, idx_msg->native);

    /* Copy the native object for the message */
    if(NULL == (idx_msg->native = (type->copy)(mesg, idx_msg->native)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to copy message to object header")

    /* Update the message flags */
    idx_msg->flags = (uint8_t)mesg_flags;

    /* Mark the message as modified */
    idx_msg->dirty = TRUE;
    chk_dirtied = TRUE;

    /* Release chunk */
    if(H5O_chunk_unprotect(f, dxpl_id, chk_proxy, chk_dirtied) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header chunk")
    chk_proxy = NULL;

    /* Update the modification time, if requested */
    if(update_flags & H5O_UPDATE_TIME)
        if(H5O_touch_oh(f, dxpl_id, oh, FALSE) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTUPDATE, FAIL, "unable to update time on object")

done:
    /* Release chunk, if not already released */
    if(chk_proxy && H5O_chunk_unprotect(f, dxpl_id, chk_proxy, chk_dirtied) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header chunk")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_copy_mesg() */


/*-------------------------------------------------------------------------
 * Function:    H5O_msg_delete
 *
 * Purpose:     Calls a message's delete callback.
 *
 *              This is mostly redundant with H5O_delete_mesg below,
 *              but H5O_delete_mesg only works on messages in object headers
 *              (while the shared message code needs to delete messages in
 *              the heap).
 *
 *              open_oh is a pointer to a currently open object header so
 *              that the library doesn't try to re-protect it.  If there is
 *              no such object header, it should be NULL.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  James Laird
 *              December 21, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_delete(H5F_t *f, hid_t dxpl_id, H5O_t *open_oh, unsigned type_id,
    void *mesg)
{
    const H5O_msg_class_t   *type;      /* Actual H5O class type for the ID */
    herr_t ret_value = SUCCEED;                    /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* check args */
    HDassert(f);
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);

    /* delete */
    if((type->del) && (type->del)(f, dxpl_id, open_oh, mesg) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "unable to delete file space for object header message")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_delete() */


/*-------------------------------------------------------------------------
 * Function:	H5O_delete_mesg
 *
 * Purpose:	Internal function to:
 *              Delete an object header message from a file.  This frees the file
 *              space used for anything referred to in the object header message.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		September 26 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_delete_mesg(H5F_t *f, hid_t dxpl_id, H5O_t *oh, H5O_mesg_t *mesg)
{
    const H5O_msg_class_t *type = mesg->type;  /* Type of object to free */
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(f);
    HDassert(mesg);
    HDassert(oh);

    /* Check if there is a file space deletion callback for this type of message */
    if(type->del) {
        /* Decode the message if necessary. */
        H5O_LOAD_NATIVE(f, dxpl_id, H5O_DECODEIO_NOCHANGE, oh, mesg, FAIL)

        if((type->del)(f, dxpl_id, oh, mesg->native) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "unable to delete file space for object header message")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_delete_mesg() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_flush
 *
 * Purpose:	Flushes a message for an object header.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		May 14 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_flush(H5F_t *f, H5O_t *oh, H5O_mesg_t *mesg)
{
    uint8_t	*p;             /* Temporary pointer to encode with */
    unsigned    msg_id;         /* ID for message */
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(f);
    HDassert(oh);

    /* Point into message's chunk's image */
    p = mesg->raw - H5O_SIZEOF_MSGHDR_OH(oh);

    /* Retrieve actual message ID, for unknown messages */
    if(mesg->type == H5O_MSG_UNKNOWN)
        msg_id = *(H5O_unknown_t *)(mesg->native);
    else
        msg_id = (uint8_t)mesg->type->id;

    /* Encode the message prefix */
    if(oh->version == H5O_VERSION_1)
        UINT16ENCODE(p, msg_id)
    else
        *p++ = (uint8_t)msg_id;
    HDassert(mesg->raw_size < H5O_MESG_MAX_SIZE);
    UINT16ENCODE(p, mesg->raw_size);
    *p++ = mesg->flags;

    /* Only encode reserved bytes for version 1 of format */
    if(oh->version == H5O_VERSION_1) {
        *p++ = 0; /*reserved*/
        *p++ = 0; /*reserved*/
        *p++ = 0; /*reserved*/
    } /* end for */
    /* Only encode creation index for version 2+ of format */
    else {
        /* Only encode creation index if they are being tracked */
        if(oh->flags & H5O_HDR_ATTR_CRT_ORDER_TRACKED)
            UINT16ENCODE(p, mesg->crt_idx);
    } /* end else */
    HDassert(p == mesg->raw);

#ifndef NDEBUG
    /* Make certain that null messages aren't in chunks w/gaps */
    if(H5O_NULL_ID == msg_id)
        HDassert(oh->chunk[mesg->chunkno].gap == 0);
    else
        /* Non-null messages should always have a native pointer */
        HDassert(mesg->native);
#endif /* NDEBUG */

    /* Encode the message itself, if it's not an "unknown" message */
    if(mesg->native && mesg->type != H5O_MSG_UNKNOWN) {
        /*
         * Encode the message.  If the message is shared then we
         * encode a Shared Object message instead of the object
         * which is being shared.
         */
        HDassert(mesg->raw >= oh->chunk[mesg->chunkno].image);
        HDassert(mesg->raw_size == H5O_ALIGN_OH(oh, mesg->raw_size));
        HDassert(mesg->raw + mesg->raw_size <=
               oh->chunk[mesg->chunkno].image + (oh->chunk[mesg->chunkno].size - H5O_SIZEOF_CHKSUM_OH(oh)));
#ifndef NDEBUG
/* Sanity check that the message won't overwrite past it's allocated space */
{
    size_t msg_size;

    msg_size = mesg->type->raw_size(f, FALSE, mesg->native);
    msg_size = H5O_ALIGN_OH(oh, msg_size);
    HDassert(msg_size <= mesg->raw_size);
}
#endif /* NDEBUG */
        HDassert(mesg->type->encode);
        if((mesg->type->encode)(f, FALSE, mesg->raw, mesg->native) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTENCODE, FAIL, "unable to encode object header message")
    } /* end if */

    /* Mark the message as clean now */
    mesg->dirty = FALSE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_flush() */


/*-------------------------------------------------------------------------
 * Function:	H5O_flush_msgs
 *
 * Purpose:	Flushes messages for object header.
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
H5O_flush_msgs(H5F_t *f, H5O_t *oh)
{
    H5O_mesg_t *curr_msg;       /* Pointer to current message being operated on */
    unsigned	u;              /* Local index variable */
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(f);
    HDassert(oh);

    /* Encode any dirty messages */
    for(u = 0, curr_msg = &oh->mesg[0]; u < oh->nmesgs; u++, curr_msg++)
        if(curr_msg->dirty)
            if(H5O_msg_flush(f, oh, curr_msg) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTENCODE, FAIL, "unable to encode object header message")

    /* Sanity check for the correct # of messages in object header */
    if(oh->nmesgs != u)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTFLUSH, FAIL, "corrupt object header - too few messages")

#ifndef NDEBUG
        /* Reset the number of messages dirtied by decoding, as they have all
         * been flushed */
        oh->ndecode_dirtied = 0;
#endif /* NDEBUG */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_flush_msgs() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_chunkno
 *
 * Purpose:	Queries the object header chunk index for a message.
 *
 * Return:	Success:	>=0 value indicating the chunk number for
 *                              the message
 *		Failure:	<0
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Apr 22 2010
 *
 *-------------------------------------------------------------------------
 */
int
H5O_msg_get_chunkno(const H5O_loc_t *loc, unsigned type_id, hid_t dxpl_id)
{
    H5O_t *oh = NULL;                   /* Object header to use */
    const H5O_msg_class_t *type;        /* Actual H5O class type for the ID */
    H5O_mesg_t *idx_msg;                /* Pointer to message to modify */
    unsigned idx;                       /* Index of message to modify */
    int ret_value;                      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(loc);
    HDassert(loc->file);
    HDassert(H5F_addr_defined(loc->addr));
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);

    /* Get the object header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_READ)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to protect object header")

    /* Locate message of correct type */
    for(idx = 0, idx_msg = &oh->mesg[0]; idx < oh->nmesgs; idx++, idx_msg++)
	if(type == idx_msg->type)
            break;
    if(idx == oh->nmesgs)
        HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "message type not found")

    /* Set return value */
    H5_ASSIGN_OVERFLOW(ret_value, idx_msg->chunkno, unsigned, int);

done:
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_get_chunkno() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_lock
 *
 * Purpose:	Locks a message into a particular chunk, preventing it from
 *              being moved into another chunk.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Apr 22 2010
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_lock(const H5O_loc_t *loc, unsigned type_id, hid_t dxpl_id)
{
    H5O_t *oh = NULL;                   /* Object header to use */
    const H5O_msg_class_t *type;        /* Actual H5O class type for the ID */
    H5O_mesg_t *idx_msg;                /* Pointer to message to modify */
    unsigned idx;                       /* Index of message to modify */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(loc);
    HDassert(loc->file);
    HDassert(H5F_addr_defined(loc->addr));
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);

    /* Get the object header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_READ)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to protect object header")

    /* Locate message of correct type */
    for(idx = 0, idx_msg = &oh->mesg[0]; idx < oh->nmesgs; idx++, idx_msg++)
	if(type == idx_msg->type)
            break;
    if(idx == oh->nmesgs)
        HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "message type not found")

    /* Fail if the message is already locked */
    if(idx_msg->locked)
	HGOTO_ERROR(H5E_OHDR, H5E_CANTLOCK, FAIL, "message already locked")

    /* Make the message locked */
    idx_msg->locked = TRUE;

done:
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_lock() */


/*-------------------------------------------------------------------------
 * Function:	H5O_msg_unlock
 *
 * Purpose:	Unlocks a message, allowing it to be moved into another chunk.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Apr 22 2010
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_msg_unlock(const H5O_loc_t *loc, unsigned type_id, hid_t dxpl_id)
{
    H5O_t *oh = NULL;                   /* Object header to use */
    const H5O_msg_class_t *type;        /* Actual H5O class type for the ID */
    H5O_mesg_t *idx_msg;                /* Pointer to message to modify */
    unsigned idx;                       /* Index of message to modify */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(loc);
    HDassert(loc->file);
    HDassert(H5F_addr_defined(loc->addr));
    HDassert(type_id < NELMTS(H5O_msg_class_g));
    type = H5O_msg_class_g[type_id];    /* map the type ID to the actual type object */
    HDassert(type);

    /* Get the object header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_READ)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to protect object header")

    /* Locate message of correct type */
    for(idx = 0, idx_msg = &oh->mesg[0]; idx < oh->nmesgs; idx++, idx_msg++)
	if(type == idx_msg->type)
            break;
    if(idx == oh->nmesgs)
        HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "message type not found")

    /* Fail if the message is not locked */
    if(!idx_msg->locked)
	HGOTO_ERROR(H5E_OHDR, H5E_CANTUNLOCK, FAIL, "message not locked")

    /* Make the message unlocked */
    idx_msg->locked = FALSE;

done:
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_msg_unlock() */

