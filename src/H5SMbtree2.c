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

/****************/
/* Module Setup */
/****************/

#define H5O_PACKAGE		/*suppress error about including H5Opkg 	  */
#define H5SM_PACKAGE		/*suppress error about including H5SMpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Opkg.h"             /* Object Headers                       */
#include "H5SMpkg.h"            /* Shared object header messages        */


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Local Prototypes */
/********************/

/* v2 B-tree callbacks */
static void *H5SM_bt2_crt_context(void *udata);
static herr_t H5SM_bt2_dst_context(void *ctx);
static herr_t H5SM_bt2_store(void *native, const void *udata);
static herr_t H5SM_bt2_debug(FILE *stream, const H5F_t *f, hid_t dxpl_id,
    int indent, int fwidth, const void *record, const void *_udata);
static void *H5SM_bt2_crt_dbg_context(H5F_t *f, hid_t dxpl_id, haddr_t addr);


/*****************************/
/* Library Private Variables */
/*****************************/
/* v2 B-tree class for SOHM indexes*/
const H5B2_class_t H5SM_INDEX[1]={{   /* B-tree class information */
    H5B2_SOHM_INDEX_ID,               /* Type of B-tree */
    "H5B2_SOHM_INDEX_ID",             /* Name of B-tree class */
    sizeof(H5SM_sohm_t),              /* Size of native record */
    H5SM_bt2_crt_context,             /* Create client callback context */
    H5SM_bt2_dst_context,             /* Destroy client callback context */
    H5SM_bt2_store,                   /* Record storage callback */
    H5SM_message_compare,             /* Record comparison callback */
    H5SM_message_encode,              /* Record encoding callback */
    H5SM_message_decode,              /* Record decoding callback */
    H5SM_bt2_debug,                   /* Record debugging callback */
    H5SM_bt2_crt_dbg_context,	      /* Create debugging context */
    H5SM_bt2_dst_context 	      /* Destroy debugging context */
}};


/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage the H5SM_bt2_ctx_t struct */
H5FL_DEFINE_STATIC(H5SM_bt2_ctx_t);



/*-------------------------------------------------------------------------
 * Function:	H5SM_bt2_crt_context
 *
 * Purpose:	Create client callback context
 *
 * Return:	Success:	non-NULL
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Thursday, November 26, 2009
 *
 *-------------------------------------------------------------------------
 */
static void *
H5SM_bt2_crt_context(void *_f)
{
    H5F_t *f = (H5F_t *)_f;     /* User data for building callback context */
    H5SM_bt2_ctx_t *ctx;        /* Callback context structure */
    void *ret_value;            /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5SM_bt2_crt_context)

    /* Sanity check */
    HDassert(f);

    /* Allocate callback context */
    if(NULL == (ctx = H5FL_MALLOC(H5SM_bt2_ctx_t)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, NULL, "can't allocate callback context")

    /* Determine the size of addresses & lengths in the file */
    ctx->sizeof_addr = H5F_SIZEOF_ADDR(f);

    /* Set return value */
    ret_value = ctx;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5SM_bt2_crt_context() */


/*-------------------------------------------------------------------------
 * Function:	H5SM_bt2_dst_context
 *
 * Purpose:	Destroy client callback context
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Thursday, November 26, 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5SM_bt2_dst_context(void *_ctx)
{
    H5SM_bt2_ctx_t *ctx = (H5SM_bt2_ctx_t *)_ctx;       /* Callback context structure */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5SM_bt2_dst_context)

    /* Sanity check */
    HDassert(ctx);

    /* Release callback context */
    ctx = H5FL_FREE(H5SM_bt2_ctx_t, ctx);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5SM_bt2_dst_context() */


/*-------------------------------------------------------------------------
 * Function:	H5SM_bt2_store
 *
 * Purpose:	Store a H5SM_sohm_t SOHM message in the B-tree.  The message
 *              comes in UDATA as a H5SM_mesg_key_t* and is copied to
 *              NATIVE as a H5SM_sohm_t.
 *
 * Return:	Non-negative on success
 *              Negative on failure
 *
 * Programmer:	James Laird
 *              Monday, November 6, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5SM_bt2_store(void *native, const void *udata)
{
    const H5SM_mesg_key_t *key = (const H5SM_mesg_key_t *)udata;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5SM_bt2_store)

    /* Copy the source message to the B-tree */
    *(H5SM_sohm_t *)native = key->message;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5SM_bt2_store */


/*-------------------------------------------------------------------------
 * Function:	H5SM_bt2_debug
 *
 * Purpose:	Print debugging information for a H5SM_sohm_t.
 *
 * Return:	Non-negative on success
 *              Negative on failure
 *
 * Programmer:	James Laird
 *              Monday, November 6, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5SM_bt2_debug(FILE *stream, const H5F_t UNUSED *f, hid_t UNUSED dxpl_id,
    int indent, int fwidth, const void *record, const void UNUSED *_udata)
{
    const H5SM_sohm_t *sohm = (const H5SM_sohm_t *)record;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5SM_bt2_debug)

    if(sohm->location == H5SM_IN_HEAP)
        HDfprintf(stream, "%*s%-*s {%a, %lo, %Hx}\n", indent, "", fwidth,
            "Shared Message in heap:",
            sohm->u.heap_loc.fheap_id, sohm->hash, sohm->u.heap_loc.ref_count);
    else {
        HDassert(sohm->location == H5SM_IN_OH);
        HDfprintf(stream, "%*s%-*s {%a, %lo, %Hx, %Hx}\n", indent, "", fwidth,
            "Shared Message in OH:",
            sohm->u.mesg_loc.oh_addr, sohm->hash, sohm->msg_type_id, sohm->u.mesg_loc.index);
    } /* end else */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5SM_bt2_debug */


/*-------------------------------------------------------------------------
 * Function:	H5SM_bt2_crt_dbg_context
 *
 * Purpose:	Create context for debugging callback
 *
 * Return:	Success:	non-NULL
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, December 1, 2009
 *
 *-------------------------------------------------------------------------
 */
static void *
H5SM_bt2_crt_dbg_context(H5F_t *f, hid_t UNUSED dxpl_id, haddr_t UNUSED addr)
{
    H5SM_bt2_ctx_t *ctx;        /* Callback context structure */
    void *ret_value;            /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5SM_bt2_crt_dbg_context)

    /* Sanity check */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));

    /* Allocate callback context */
    if(NULL == (ctx = H5FL_MALLOC(H5SM_bt2_ctx_t)))
        HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, NULL, "can't allocate callback context")

    /* Determine the size of addresses & lengths in the file */
    ctx->sizeof_addr = H5F_SIZEOF_ADDR(f);

    /* Set return value */
    ret_value = ctx;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5SM_bt2_crt_dbg_context() */


/*-------------------------------------------------------------------------
 * Function:	H5SM_bt2_convert_to_list_op
 *
 * Purpose:	An H5B2_remove_t callback function to convert a SOHM
 *              B-tree index to a list.
 *
 *              Inserts this record into the list passed through op_data.
 *
 * Return:	Non-negative on success
 *              Negative on failure
 *
 * Programmer:	James Laird
 *              Monday, November 6, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5SM_bt2_convert_to_list_op(const void * record, void *op_data)
{
    const H5SM_sohm_t *message = (const H5SM_sohm_t *)record;
    const H5SM_list_t *list = (const H5SM_list_t *)op_data;
    size_t mesg_idx;            /* Index of message to modify */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5SM_bt2_convert_to_list_op)

    /* Sanity checks */
    HDassert(record);
    HDassert(op_data);

    /* Get the message index, and increment the # of messages in list */
    mesg_idx = list->header->num_messages++;
    HDassert(list->header->num_messages <= list->header->list_max);

    /* Insert this message at the end of the list */
    HDassert(list->messages[mesg_idx].location == H5SM_NO_LOC);
    HDassert(message->location != H5SM_NO_LOC);
    HDmemcpy(&(list->messages[mesg_idx]), message, sizeof(H5SM_sohm_t));

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5SM_bt2_convert_to_list_op() */

