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
 * Created:             H5Ostab.c
 *                      Aug  6 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Symbol table messages.
 *
 *-------------------------------------------------------------------------
 */

#define H5G_PACKAGE		/*suppress error about including H5Gpkg	  */
#define H5O_PACKAGE		/*suppress error about including H5Opkg	  */

#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free lists                           */
#include "H5Gpkg.h"		/* Groups				*/
#include "H5HLprivate.h"	/* Local Heaps				*/
#include "H5Opkg.h"             /* Object headers			*/


/* PRIVATE PROTOTYPES */
static void *H5O_stab_decode(H5F_t *f, hid_t dxpl_id, H5O_t *open_oh,
    unsigned mesg_flags, unsigned *ioflags, const uint8_t *p);
static herr_t H5O_stab_encode(H5F_t *f, hbool_t disable_shared, uint8_t *p, const void *_mesg);
static void *H5O_stab_copy(const void *_mesg, void *_dest);
static size_t H5O_stab_size(const H5F_t *f, hbool_t disable_shared, const void *_mesg);
static herr_t H5O_stab_free(void *_mesg);
static herr_t H5O_stab_delete(H5F_t *f, hid_t dxpl_id, H5O_t *open_oh, void *_mesg);
static void *H5O_stab_copy_file(H5F_t *file_src, void *native_src,
    H5F_t *file_dst, hbool_t *recompute_size, H5O_copy_t *cpy_info, void *udata,
    hid_t dxpl_id);
static herr_t H5O_stab_post_copy_file(const H5O_loc_t *src_oloc, const void *mesg_src, H5O_loc_t *dst_oloc,
    void *mesg_dst, hid_t dxpl_id, H5O_copy_t *cpy_info);
static herr_t H5O_stab_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg,
    FILE * stream, int indent, int fwidth);

/* This message derives from H5O message class */
const H5O_msg_class_t H5O_MSG_STAB[1] = {{
    H5O_STAB_ID,            	/*message id number             */
    "stab",                 	/*message name for debugging    */
    sizeof(H5O_stab_t),     	/*native message size           */
    0,				/* messages are sharable?       */
    H5O_stab_decode,        	/*decode message                */
    H5O_stab_encode,        	/*encode message                */
    H5O_stab_copy,          	/*copy the native value         */
    H5O_stab_size,          	/*size of symbol table entry    */
    NULL,                   	/*default reset method          */
    H5O_stab_free,	        /* free method			*/
    H5O_stab_delete,	        /* file delete method		*/
    NULL,			/* link method			*/
    NULL, 			/*set share method		*/
    NULL,		    	/*can share method		*/
    NULL,			/* pre copy native value to file */
    H5O_stab_copy_file,		/* copy native value to file    */
    H5O_stab_post_copy_file,	/* post copy native value to file    */
    NULL,			/* get creation index		*/
    NULL,			/* set creation index		*/
    H5O_stab_debug         	/*debug the message             */
}};

/* Declare a free list to manage the H5O_stab_t struct */
H5FL_DEFINE_STATIC(H5O_stab_t);


/*-------------------------------------------------------------------------
 * Function:    H5O_stab_decode
 *
 * Purpose:     Decode a symbol table message and return a pointer to
 *              a newly allocated one.
 *
 * Return:      Success:        Ptr to new message in native order.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_stab_decode(H5F_t *f, hid_t UNUSED dxpl_id, H5O_t UNUSED *open_oh,
    unsigned UNUSED mesg_flags, unsigned UNUSED *ioflags, const uint8_t *p)
{
    H5O_stab_t          *stab = NULL;
    void                *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_stab_decode)

    /* check args */
    HDassert(f);
    HDassert(p);

    /* decode */
    if(NULL == (stab = H5FL_CALLOC(H5O_stab_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
    H5F_addr_decode(f, &p, &(stab->btree_addr));
    H5F_addr_decode(f, &p, &(stab->heap_addr));

    /* Set return value */
    ret_value = stab;

done:
    if(ret_value == NULL) {
        if(stab != NULL)
            stab = H5FL_FREE(H5O_stab_t, stab);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_stab_decode() */


/*-------------------------------------------------------------------------
 * Function:    H5O_stab_encode
 *
 * Purpose:     Encodes a symbol table message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_stab_encode(H5F_t *f, hbool_t UNUSED disable_shared, uint8_t *p, const void *_mesg)
{
    const H5O_stab_t       *stab = (const H5O_stab_t *) _mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_stab_encode);

    /* check args */
    assert(f);
    assert(p);
    assert(stab);

    /* encode */
    H5F_addr_encode(f, &p, stab->btree_addr);
    H5F_addr_encode(f, &p, stab->heap_addr);

    FUNC_LEAVE_NOAPI(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5O_stab_copy
 *
 * Purpose:     Copies a message from _MESG to _DEST, allocating _DEST if
 *              necessary.
 *
 * Return:      Success:        Ptr to _DEST
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_stab_copy(const void *_mesg, void *_dest)
{
    const H5O_stab_t       *stab = (const H5O_stab_t *) _mesg;
    H5O_stab_t             *dest = (H5O_stab_t *) _dest;
    void                *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_stab_copy)

    /* check args */
    HDassert(stab);
    if(!dest && NULL == (dest = H5FL_MALLOC(H5O_stab_t)))
	HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* copy */
    *dest = *stab;

    /* Set return value */
    ret_value = dest;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_stab_copy() */


/*-------------------------------------------------------------------------
 * Function:    H5O_stab_size
 *
 * Purpose:     Returns the size of the raw message in bytes not counting
 *              the message type or size fields, but only the data fields.
 *              This function doesn't take into account alignment.
 *
 * Return:      Success:        Message data size in bytes without alignment.
 *
 *              Failure:        zero
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O_stab_size(const H5F_t *f, hbool_t UNUSED disable_shared, const void UNUSED *_mesg)
{
    size_t ret_value;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_stab_size);

    /* Set return value */
    ret_value=2 * H5F_SIZEOF_ADDR(f);

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5O_stab_free
 *
 * Purpose:	Free's the message
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, March 30, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_stab_free(void *mesg)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_stab_free)

    HDassert(mesg);

    mesg = H5FL_FREE(H5O_stab_t, mesg);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_stab_free() */


/*-------------------------------------------------------------------------
 * Function:    H5O_stab_delete
 *
 * Purpose:     Free file space referenced by message
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, March 20, 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_stab_delete(H5F_t *f, hid_t dxpl_id, H5O_t UNUSED *open_oh, void *mesg)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_stab_delete)

    /* check args */
    HDassert(f);
    HDassert(mesg);

    /* Free the file space for the symbol table */
    if(H5G_stab_delete(f, dxpl_id, (const H5O_stab_t *)mesg) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to free symbol table")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_stab_delete() */


/*-------------------------------------------------------------------------
 * Function:    H5O_stab_copy_file
 *
 * Purpose:     Copies a message from _MESG to _DEST in file
 *
 * Return:      Success:        Ptr to _DEST
 *
 *              Failure:        NULL
 *
 * Programmer:  Peter Cao
 *              September 10, 2005
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_stab_copy_file(H5F_t *file_src, void *native_src, H5F_t *file_dst,
    hbool_t UNUSED *recompute_size, H5O_copy_t UNUSED *cpy_info, void UNUSED *udata,
    hid_t dxpl_id)
{
    H5O_stab_t          *stab_src = (H5O_stab_t *) native_src;
    H5O_stab_t          *stab_dst = NULL;
    size_t              size_hint;              /* Local heap initial size */
    void                *ret_value;             /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_stab_copy_file)

    /* check args */
    HDassert(stab_src);
    HDassert(file_dst);

    /* Allocate space for the destination stab */
    if(NULL == (stab_dst = H5FL_MALLOC(H5O_stab_t)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Get the old local heap's size and use that as the hint for the new heap */
    if(H5HL_get_size(file_src, dxpl_id, stab_src->heap_addr, &size_hint) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_CANTGETSIZE, NULL, "can't query local heap size")

    /* Create components of symbol table message */
    if(H5G_stab_create_components(file_dst, stab_dst, size_hint, dxpl_id) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, NULL, "can't create symbol table components")

    /* Set return value */
    ret_value = stab_dst;

done:
    if(!ret_value)
        if(stab_dst)
            stab_dst = H5FL_FREE(H5O_stab_t, stab_dst);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_stab_copy_file() */


/*-------------------------------------------------------------------------
 * Function:    H5O_stab_post_copy_file
 *
 * Purpose:     Finish copying a message from between files
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Peter Cao
 *              September 28, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_stab_post_copy_file(const H5O_loc_t *src_oloc, const void *mesg_src, H5O_loc_t *dst_oloc,
    void *mesg_dst, hid_t dxpl_id, H5O_copy_t *cpy_info)
{
    const H5O_stab_t    *stab_src = (const H5O_stab_t *)mesg_src;
    H5O_stab_t          *stab_dst = (H5O_stab_t *)mesg_dst;
    H5G_bt_it_cpy_t     udata;      /* B-tree user data */
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_stab_post_copy_file)

    /* check args */
    HDassert(stab_src);
    HDassert(H5F_addr_defined(dst_oloc->addr));
    HDassert(dst_oloc->file);
    HDassert(stab_dst);
    HDassert(cpy_info);

    /* If we are performing a 'shallow hierarchy' copy, get out now */
    if(cpy_info->max_depth >= 0 && cpy_info->curr_depth >= cpy_info->max_depth)
        HGOTO_DONE(SUCCEED)

    /* Set up B-tree iteration user data */
    udata.src_oloc = src_oloc;
    udata.src_heap_addr = stab_src->heap_addr;
    udata.dst_file = dst_oloc->file;
    udata.dst_stab = stab_dst;
    udata.cpy_info = cpy_info;

    /* Iterate over objects in group, copying them */
    if((H5B_iterate(src_oloc->file, dxpl_id, H5B_SNODE, stab_src->btree_addr, H5G_node_copy, &udata)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "iteration operator failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_stab_post_copy_file() */


/*-------------------------------------------------------------------------
 * Function:    H5O_stab_debug
 *
 * Purpose:     Prints debugging info for a symbol table message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  6 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_stab_debug(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, const void *_mesg, FILE * stream,
	       int indent, int fwidth)
{
    const H5O_stab_t       *stab = (const H5O_stab_t *) _mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_stab_debug);

    /* check args */
    assert(f);
    assert(stab);
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
	      "B-tree address:", stab->btree_addr);

    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
	      "Name heap address:", stab->heap_addr);

    FUNC_LEAVE_NOAPI(SUCCEED);
}
