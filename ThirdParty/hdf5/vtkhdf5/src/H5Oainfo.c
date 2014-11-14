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
 * Created:             H5Oainfo.c
 *                      Mar  6 2007
 *                      Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:             Attribute Information messages.
 *
 *-------------------------------------------------------------------------
 */

#define H5A_PACKAGE		/*suppress error about including H5Apkg	  */
#define H5O_PACKAGE		/*suppress error about including H5Opkg	  */

#include "H5private.h"		/* Generic Functions			*/
#include "H5Apkg.h"             /* Attributes				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free lists                           */
#include "H5Opkg.h"             /* Object headers			*/


/* PRIVATE PROTOTYPES */
static void *H5O_ainfo_decode(H5F_t *f, hid_t dxpl_id, H5O_t *open_oh,
    unsigned mesg_flags, unsigned *ioflags, const uint8_t *p);
static herr_t H5O_ainfo_encode(H5F_t *f, hbool_t disable_shared, uint8_t *p, const void *_mesg);
static void *H5O_ainfo_copy(const void *_mesg, void *_dest);
static size_t H5O_ainfo_size(const H5F_t *f, hbool_t disable_shared, const void *_mesg);
static herr_t H5O_ainfo_free(void *_mesg);
static herr_t H5O_ainfo_delete(H5F_t *f, hid_t dxpl_id, H5O_t *open_oh,
    void *_mesg);
static herr_t H5O_ainfo_pre_copy_file(H5F_t *file_src, const void *mesg_src,
    hbool_t *deleted, const H5O_copy_t *cpy_info, void *udata);
static void *H5O_ainfo_copy_file(H5F_t *file_src, void *mesg_src,
    H5F_t *file_dst, hbool_t *recompute_size, unsigned *mesg_flags,
    H5O_copy_t *cpy_info, void *udata, hid_t dxpl_id);
static herr_t H5O_ainfo_post_copy_file(const H5O_loc_t *src_oloc,
    const void *mesg_src, H5O_loc_t *dst_oloc, void *mesg_dst,
    unsigned *mesg_flags, hid_t dxpl_id, H5O_copy_t *cpy_info);
static herr_t H5O_ainfo_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg,
			     FILE * stream, int indent, int fwidth);

/* This message derives from H5O message class */
const H5O_msg_class_t H5O_MSG_AINFO[1] = {{
    H5O_AINFO_ID,            	/*message id number             */
    "ainfo",                 	/*message name for debugging    */
    sizeof(H5O_ainfo_t),     	/*native message size           */
    0,				/* messages are sharable?       */
    H5O_ainfo_decode,        	/*decode message                */
    H5O_ainfo_encode,        	/*encode message                */
    H5O_ainfo_copy,          	/*copy the native value         */
    H5O_ainfo_size,          	/*size of symbol table entry    */
    NULL,                   	/*default reset method          */
    H5O_ainfo_free,	        /* free method			*/
    H5O_ainfo_delete,	        /* file delete method		*/
    NULL,			/* link method			*/
    NULL,			/*set share method		*/
    NULL,		    	/*can share method		*/
    H5O_ainfo_pre_copy_file,	/* pre copy native value to file */
    H5O_ainfo_copy_file,	/* copy native value to file    */
    H5O_ainfo_post_copy_file,   /* post copy native value to file */
    NULL,			/* get creation index		*/
    NULL,			/* set creation index		*/
    H5O_ainfo_debug          	/*debug the message             */
}};

/* Current version of attribute info information */
#define H5O_AINFO_VERSION 	0

/* Flags for attribute info flag encoding */
#define H5O_AINFO_TRACK_CORDER          0x01
#define H5O_AINFO_INDEX_CORDER          0x02
#define H5O_AINFO_ALL_FLAGS             (H5O_AINFO_TRACK_CORDER | H5O_AINFO_INDEX_CORDER)

/* Declare a free list to manage the H5O_ainfo_t struct */
H5FL_DEFINE_STATIC(H5O_ainfo_t);


/*-------------------------------------------------------------------------
 * Function:    H5O_ainfo_decode
 *
 * Purpose:     Decode a message and return a pointer to a newly allocated one.
 *
 * Return:      Success:        Ptr to new message in native form.
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              koziol@hdfgroup.org
 *              Mar  6 2007
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_ainfo_decode(H5F_t *f, hid_t UNUSED dxpl_id, H5O_t UNUSED *open_oh,
    unsigned UNUSED mesg_flags, unsigned UNUSED *ioflags, const uint8_t *p)
{
    H5O_ainfo_t	*ainfo = NULL;  /* Attribute info */
    unsigned char flags;        /* Flags for encoding attribute info */
    void        *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f);
    HDassert(p);

    /* Version of message */
    if(*p++ != H5O_AINFO_VERSION)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "bad version number for message")

    /* Allocate space for message */
    if(NULL == (ainfo = H5FL_MALLOC(H5O_ainfo_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Get the flags for the message */
    flags = *p++;
    if(flags & ~H5O_AINFO_ALL_FLAGS)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "bad flag value for message")
    ainfo->track_corder = (flags & H5O_AINFO_TRACK_CORDER) ? TRUE : FALSE;
    ainfo->index_corder = (flags & H5O_AINFO_INDEX_CORDER) ? TRUE : FALSE;

    /* Set the number of attributes on the object to an invalid value, so we query it later */
    ainfo->nattrs = HSIZET_MAX;

    /* Max. creation order value for the object */
    if(ainfo->track_corder)
        UINT16DECODE(p, ainfo->max_crt_idx)
    else
        ainfo->max_crt_idx = H5O_MAX_CRT_ORDER_IDX;

    /* Address of fractal heap to store "dense" attributes */
    H5F_addr_decode(f, &p, &(ainfo->fheap_addr));

    /* Address of v2 B-tree to index names of attributes (names are always indexed) */
    H5F_addr_decode(f, &p, &(ainfo->name_bt2_addr));

    /* Address of v2 B-tree to index creation order of links, if there is one */
    if(ainfo->index_corder)
        H5F_addr_decode(f, &p, &(ainfo->corder_bt2_addr));
    else
        ainfo->corder_bt2_addr = HADDR_UNDEF;

    /* Set return value */
    ret_value = ainfo;

done:
    if(ret_value == NULL && ainfo != NULL)
        ainfo = H5FL_FREE(H5O_ainfo_t, ainfo);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_ainfo_decode() */


/*-------------------------------------------------------------------------
 * Function:    H5O_ainfo_encode
 *
 * Purpose:     Encodes a message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              koziol@hdfgroup.org
 *              Mar  6 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_ainfo_encode(H5F_t *f, hbool_t UNUSED disable_shared, uint8_t *p, const void *_mesg)
{
    const H5O_ainfo_t   *ainfo = (const H5O_ainfo_t *)_mesg;
    unsigned char       flags;          /* Flags for encoding attribute info */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(f);
    HDassert(p);
    HDassert(ainfo);

    /* Message version */
    *p++ = H5O_AINFO_VERSION;

    /* The flags for the attribute indices */
    flags = ainfo->track_corder ? H5O_AINFO_TRACK_CORDER : 0;
    flags = (unsigned char)(flags | (ainfo->index_corder ? H5O_AINFO_INDEX_CORDER : 0));
    *p++ = flags;

    /* Max. creation order value for the object */
    if(ainfo->track_corder)
        UINT16ENCODE(p, ainfo->max_crt_idx);

    /* Address of fractal heap to store "dense" attributes */
    H5F_addr_encode(f, &p, ainfo->fheap_addr);

    /* Address of v2 B-tree to index names of attributes */
    H5F_addr_encode(f, &p, ainfo->name_bt2_addr);

    /* Address of v2 B-tree to index creation order of attributes, if they are indexed */
    if(ainfo->index_corder)
        H5F_addr_encode(f, &p, ainfo->corder_bt2_addr);
    else
        HDassert(!H5F_addr_defined(ainfo->corder_bt2_addr));

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_ainfo_encode() */


/*-------------------------------------------------------------------------
 * Function:    H5O_ainfo_copy
 *
 * Purpose:     Copies a message from _MESG to _DEST, allocating _DEST if
 *              necessary.
 *
 * Return:      Success:        Ptr to _DEST
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              koziol@hdfgroup.org
 *              Mar  6 2007
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_ainfo_copy(const void *_mesg, void *_dest)
{
    const H5O_ainfo_t   *ainfo = (const H5O_ainfo_t *)_mesg;
    H5O_ainfo_t         *dest = (H5O_ainfo_t *) _dest;
    void                *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(ainfo);
    if(!dest && NULL == (dest = H5FL_MALLOC(H5O_ainfo_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* copy */
    *dest = *ainfo;

    /* Set return value */
    ret_value = dest;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_ainfo_copy() */


/*-------------------------------------------------------------------------
 * Function:    H5O_ainfo_size
 *
 * Purpose:     Returns the size of the raw message in bytes not counting
 *              the message type or size fields, but only the data fields.
 *              This function doesn't take into account alignment.
 *
 * Return:      Success:        Message data size in bytes without alignment.
 *              Failure:        zero
 *
 * Programmer:  Quincey Koziol
 *              koziol@hdfgroup.org
 *              Mar  6 2007
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O_ainfo_size(const H5F_t *f, hbool_t UNUSED disable_shared, const void *_mesg)
{
    const H5O_ainfo_t   *ainfo = (const H5O_ainfo_t *)_mesg;
    size_t ret_value;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Set return value */
    ret_value = (size_t)(1                       /* Version */
                + 1                     /* Index flags */
                + (ainfo->track_corder ? 2 : 0) /* Curr. max. creation order value */
                + H5F_SIZEOF_ADDR(f)    /* Address of fractal heap to store "dense" attributes */
                + H5F_SIZEOF_ADDR(f)    /* Address of v2 B-tree for indexing names of attributes */
                + (ainfo->index_corder ? H5F_SIZEOF_ADDR(f) : 0));   /* Address of v2 B-tree for indexing creation order values of attributes */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_ainfo_size() */


/*-------------------------------------------------------------------------
 * Function:	H5O_ainfo_free
 *
 * Purpose:	Free's the message
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, March  6, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_ainfo_free(void *mesg)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(mesg);

    mesg = H5FL_FREE(H5O_ainfo_t, mesg);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_ainfo_free() */


/*-------------------------------------------------------------------------
 * Function:    H5O_ainfo_delete
 *
 * Purpose:     Free file space referenced by message
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, March  6, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_ainfo_delete(H5F_t *f, hid_t dxpl_id, H5O_t UNUSED *open_oh, void *_mesg)
{
    H5O_ainfo_t *ainfo = (H5O_ainfo_t *)_mesg;
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f);
    HDassert(ainfo);

    /* If the object is using "dense" attribute storage, delete it */
    if(H5F_addr_defined(ainfo->fheap_addr))
        if(H5A_dense_delete(f, dxpl_id, ainfo) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to free dense attribute storage")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_ainfo_delete() */


/*-------------------------------------------------------------------------
 * Function:    H5O_ainfo_pre_copy_file
 *
 * Purpose:     Perform any necessary actions before copying message between
 *              files.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Friday, March  9, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_ainfo_pre_copy_file(H5F_t UNUSED *file_src, const void UNUSED *native_src,
    hbool_t *deleted, const H5O_copy_t *cpy_info, void UNUSED *udata)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(deleted);
    HDassert(cpy_info);

    /* If we are not copying attributes into the destination file, indicate
     *  that this message should be deleted.
     */
    if(cpy_info->copy_without_attr)
        *deleted = TRUE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_ainfo_pre_copy_file() */


/*-------------------------------------------------------------------------
 * Function:    H5O_ainfo_copy_file
 *
 * Purpose:     Copies a message from _MESG to _DEST in file
 *
 * Return:      Success:        Ptr to _DEST
 *              Failure:        NULL
 *
 * Programmer:  Peter Cao
 *              July 18, 2007
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_ainfo_copy_file(H5F_t *file_src, void *mesg_src, H5F_t *file_dst,
    hbool_t UNUSED *recompute_size, unsigned UNUSED *mesg_flags,
    H5O_copy_t *cpy_info, void UNUSED *udata, hid_t dxpl_id)
{
    H5O_ainfo_t *ainfo_src = (H5O_ainfo_t *)mesg_src;
    H5O_ainfo_t *ainfo_dst = NULL;
    void        *ret_value;             /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(file_src);
    HDassert(ainfo_src);
    HDassert(file_dst);
    HDassert(cpy_info);
    HDassert(!cpy_info->copy_without_attr);

    /* Allocate space for the destination message */
    if(NULL == (ainfo_dst = H5FL_MALLOC(H5O_ainfo_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Copy the top level of the information */
    *ainfo_dst = *ainfo_src;

    if(H5F_addr_defined(ainfo_src->fheap_addr)) {
        /* copy dense attribute */

        if(H5A_dense_create(file_dst, dxpl_id, ainfo_dst) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "unable to create dense storage for attributes")
    } /* end if */

    /* Set return value */
    ret_value = ainfo_dst;

done:
    /* Release destination attribute information on failure */
    if(!ret_value && ainfo_dst)
        ainfo_dst = H5FL_FREE(H5O_ainfo_t, ainfo_dst);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_ainfo_copy_file() */


/*-------------------------------------------------------------------------
 * Function:    H5O_ainfo_post_copy_file
 *
 * Purpose:     Finish copying a message from between files.
 *              We have to copy the values of a reference attribute in the
 *              post copy because H5O_post_copy_file() fails in the case that
 *              an object may have a reference attribute that points to the
 *              object itself.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Peter Cao
 *              July 25, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_ainfo_post_copy_file(const H5O_loc_t *src_oloc, const void *mesg_src,
    H5O_loc_t *dst_oloc, void *mesg_dst, unsigned UNUSED *mesg_flags,
    hid_t dxpl_id, H5O_copy_t *cpy_info)
{
    const H5O_ainfo_t *ainfo_src = (const H5O_ainfo_t *)mesg_src;
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(ainfo_src);

    if(H5F_addr_defined(ainfo_src->fheap_addr)) {
        if(H5A_dense_post_copy_file_all(src_oloc, ainfo_src, dst_oloc,
                (H5O_ainfo_t *)mesg_dst, dxpl_id, cpy_info) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTCOPY, FAIL, "can't copy attribute")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_ainfo_post_copy_file() */


/*-------------------------------------------------------------------------
 * Function:    H5O_ainfo_debug
 *
 * Purpose:     Prints debugging info for a message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              koziol@hdfgroup.org
 *              Mar  6 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_ainfo_debug(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, const void *_mesg, FILE * stream,
	       int indent, int fwidth)
{
    const H5O_ainfo_t       *ainfo = (const H5O_ainfo_t *) _mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(f);
    HDassert(ainfo);
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
	      "Number of attributes:", ainfo->nattrs);
    HDfprintf(stream, "%*s%-*s %t\n", indent, "", fwidth,
	      "Track creation order of attributes:", ainfo->track_corder);
    HDfprintf(stream, "%*s%-*s %t\n", indent, "", fwidth,
	      "Index creation order of attributes:", ainfo->index_corder);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Max. creation index value:", (unsigned)ainfo->max_crt_idx);
    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
	      "'Dense' attribute storage fractal heap address:", ainfo->fheap_addr);
    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
	      "'Dense' attribute storage name index v2 B-tree address:", ainfo->name_bt2_addr);
    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
	      "'Dense' attribute storage creation order index v2 B-tree address:", ainfo->corder_bt2_addr);


    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_ainfo_debug() */

