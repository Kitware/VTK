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

/* Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Wednesday, October  8, 1997
 *
 * Purpose:     Messages related to data layout.
 */

#define H5D_PACKAGE		/*suppress error about including H5Dpkg	  */
#define H5O_PACKAGE		/*suppress error about including H5Opkg	  */

#include "H5private.h"		/* Generic Functions			*/
#include "H5Dpkg.h"		/* Dataset functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5MFprivate.h"	/* File space management		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Opkg.h"             /* Object headers			*/
#include "H5Pprivate.h"		/* Property lists			*/


/* Local macros */


/* PRIVATE PROTOTYPES */
static void *H5O_layout_decode(H5F_t *f, hid_t dxpl_id, H5O_t *open_oh,
    unsigned mesg_flags, unsigned *ioflags, const uint8_t *p);
static herr_t H5O_layout_encode(H5F_t *f, hbool_t disable_shared, uint8_t *p, const void *_mesg);
static void *H5O_layout_copy(const void *_mesg, void *_dest);
static size_t H5O_layout_size(const H5F_t *f, hbool_t disable_shared, const void *_mesg);
static herr_t H5O_layout_reset(void *_mesg);
static herr_t H5O_layout_free(void *_mesg);
static herr_t H5O_layout_delete(H5F_t *f, hid_t dxpl_id, H5O_t *open_oh,
    void *_mesg);
static void *H5O_layout_copy_file(H5F_t *file_src, void *mesg_src,
    H5F_t *file_dst, hbool_t *recompute_size, unsigned *mesg_flags,
    H5O_copy_t *cpy_info, void *udata, hid_t dxpl_id);
static herr_t H5O_layout_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg, FILE * stream,
			       int indent, int fwidth);

/* This message derives from H5O message class */
const H5O_msg_class_t H5O_MSG_LAYOUT[1] = {{
    H5O_LAYOUT_ID,          	/*message id number             */
    "layout",               	/*message name for debugging    */
    sizeof(H5O_layout_t),   	/*native message size           */
    0,				/* messages are sharable?       */
    H5O_layout_decode,      	/*decode message                */
    H5O_layout_encode,      	/*encode message                */
    H5O_layout_copy,        	/*copy the native value         */
    H5O_layout_size,        	/*size of message on disk       */
    H5O_layout_reset,		/*reset method                  */
    H5O_layout_free,        	/*free the struct		*/
    H5O_layout_delete,	        /* file delete method		*/
    NULL,			/* link method			*/
    NULL,			/*set share method		*/
    NULL,		    	/*can share method		*/
    NULL,			/* pre copy native value to file */
    H5O_layout_copy_file,	/* copy native value to file    */
    NULL,		        /* post copy native value to file */
    NULL,			/* get creation index		*/
    NULL,			/* set creation index		*/
    H5O_layout_debug       	/*debug the message             */
}};


/* Declare a free list to manage the H5O_layout_t struct */
H5FL_DEFINE(H5O_layout_t);


/*-------------------------------------------------------------------------
 * Function:    H5O_layout_decode
 *
 * Purpose:     Decode an data layout message and return a pointer to a
 *              new one created with malloc().
 *
 * Return:      Success:        Ptr to new message in native order.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_layout_decode(H5F_t *f, hid_t UNUSED dxpl_id, H5O_t UNUSED *open_oh,
    unsigned UNUSED mesg_flags, unsigned UNUSED *ioflags, const uint8_t *p)
{
    H5O_layout_t           *mesg = NULL;
    unsigned               u;
    void                   *ret_value;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f);
    HDassert(p);

    /* decode */
    if(NULL == (mesg = H5FL_CALLOC(H5O_layout_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    mesg->version = *p++;
    if(mesg->version < H5O_LAYOUT_VERSION_1 || mesg->version > H5O_LAYOUT_VERSION_3)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "bad version number for layout message")

    if(mesg->version < H5O_LAYOUT_VERSION_3) {
        unsigned	ndims;			/* Num dimensions in chunk           */

        /* Dimensionality */
        ndims = *p++;
        if(ndims > H5O_LAYOUT_NDIMS)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "dimensionality is too large")

        /* Layout class */
        mesg->type = (H5D_layout_t)*p++;
        HDassert(H5D_CONTIGUOUS == mesg->type || H5D_CHUNKED == mesg->type || H5D_COMPACT == mesg->type);

        /* Reserved bytes */
        p += 5;

        /* Address */
        if(mesg->type == H5D_CONTIGUOUS) {
            H5F_addr_decode(f, &p, &(mesg->storage.u.contig.addr));

            /* Set the layout operations */
            mesg->ops = H5D_LOPS_CONTIG;
        } /* end if */
        else if(mesg->type == H5D_CHUNKED) {
            H5F_addr_decode(f, &p, &(mesg->storage.u.chunk.idx_addr));

            /* Set the layout operations */
            mesg->ops = H5D_LOPS_CHUNK;

            /* Set the chunk operations */
            /* (Only "btree" indexing type currently supported in this version) */
            mesg->storage.u.chunk.idx_type = H5D_CHUNK_BTREE;
            mesg->storage.u.chunk.ops = H5D_COPS_BTREE;
        } /* end if */
        else {
            /* Sanity check */
            HDassert(mesg->type == H5D_COMPACT);

            /* Set the layout operations */
            mesg->ops = H5D_LOPS_COMPACT;
        } /* end else */

        /* Read the size */
        if(mesg->type != H5D_CHUNKED) {
            /* Don't compute size of contiguous storage here, due to possible
             * truncation of the dimension sizes when they were stored in this
             * version of the layout message.  Compute the contiguous storage
             * size in the dataset code, where we've got the dataspace
             * information available also.  - QAK 5/26/04
             */
            p += ndims * 4;     /* Skip over dimension sizes (32-bit quantities) */
        } /* end if */
        else {
            mesg->u.chunk.ndims=ndims;
            for(u = 0; u < ndims; u++)
                UINT32DECODE(p, mesg->u.chunk.dim[u]);

            /* Compute chunk size */
            for(u = 1, mesg->u.chunk.size = mesg->u.chunk.dim[0]; u < ndims; u++)
                mesg->u.chunk.size *= mesg->u.chunk.dim[u];
        } /* end if */

        if(mesg->type == H5D_COMPACT) {
            UINT32DECODE(p, mesg->storage.u.compact.size);
            if(mesg->storage.u.compact.size > 0) {
                if(NULL == (mesg->storage.u.compact.buf = H5MM_malloc(mesg->storage.u.compact.size)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for compact data buffer")
                HDmemcpy(mesg->storage.u.compact.buf, p, mesg->storage.u.compact.size);
                p += mesg->storage.u.compact.size;
            } /* end if */
        } /* end if */
    } /* end if */
    else {
        /* Layout class */
        mesg->type = (H5D_layout_t)*p++;

        /* Interpret the rest of the message according to the layout class */
        switch(mesg->type) {
            case H5D_COMPACT:
                UINT16DECODE(p, mesg->storage.u.compact.size);
                if(mesg->storage.u.compact.size > 0) {
                    if(NULL == (mesg->storage.u.compact.buf = H5MM_malloc(mesg->storage.u.compact.size)))
                        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for compact data buffer")
                    HDmemcpy(mesg->storage.u.compact.buf, p, mesg->storage.u.compact.size);
                    p += mesg->storage.u.compact.size;
                } /* end if */

                /* Set the layout operations */
                mesg->ops = H5D_LOPS_COMPACT;
                break;

            case H5D_CONTIGUOUS:
                H5F_addr_decode(f, &p, &(mesg->storage.u.contig.addr));
                H5F_DECODE_LENGTH(f, p, mesg->storage.u.contig.size);

                /* Set the layout operations */
                mesg->ops = H5D_LOPS_CONTIG;
                break;

            case H5D_CHUNKED:
                /* Dimensionality */
                mesg->u.chunk.ndims = *p++;
                if(mesg->u.chunk.ndims > H5O_LAYOUT_NDIMS)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "dimensionality is too large")

                /* B-tree address */
                H5F_addr_decode(f, &p, &(mesg->storage.u.chunk.idx_addr));

                /* Chunk dimensions */
                for(u = 0; u < mesg->u.chunk.ndims; u++)
                    UINT32DECODE(p, mesg->u.chunk.dim[u]);

                /* Compute chunk size */
                for(u = 1, mesg->u.chunk.size = mesg->u.chunk.dim[0]; u < mesg->u.chunk.ndims; u++)
                    mesg->u.chunk.size *= mesg->u.chunk.dim[u];

                /* Set the chunk operations */
                /* (Only "btree" indexing type supported with v3 of message format) */
                mesg->storage.u.chunk.idx_type = H5D_CHUNK_BTREE;
                mesg->storage.u.chunk.ops = H5D_COPS_BTREE;

                /* Set the layout operations */
                mesg->ops = H5D_LOPS_CHUNK;
                break;

            case H5D_LAYOUT_ERROR:
            case H5D_NLAYOUTS:
            default:
                HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "Invalid layout class")
        } /* end switch */
    } /* end else */

    /* Set return value */
    ret_value = mesg;

done:
    if(ret_value == NULL)
        if(mesg)
            mesg = H5FL_FREE(H5O_layout_t, mesg);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_layout_decode() */


/*-------------------------------------------------------------------------
 * Function:    H5O_layout_encode
 *
 * Purpose:     Encodes a message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 * Note:
 *      Quincey Koziol, 2004-5-21
 *      We write out version 3 messages by default now.
 *
 * Modifications:
 * 	Robb Matzke, 1998-07-20
 *	Rearranged the message to add a version number at the beginning.
 *
 *	Raymond Lu, 2002-2-26
 *	Added version number 2 case depends on if space has been allocated
 *	at the moment when layout header message is updated.
 *
 *      Quincey Koziol, 2004-5-21
 *      Added version number 3 case to straighten out problems with contiguous
 *      layout's sizes (was encoding them as 4-byte values when they were
 *      really n-byte values (where n usually is 8)) and additionally clean up
 *      the information written out.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_layout_encode(H5F_t *f, hbool_t UNUSED disable_shared, uint8_t *p, const void *_mesg)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    unsigned               u;
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f);
    HDassert(mesg);
    HDassert(p);

    /* Message version */
    *p++ = (uint8_t)H5O_LAYOUT_VERSION_3;

    /* Layout class */
    *p++ = mesg->type;

    /* Write out layout class specific information */
    switch(mesg->type) {
        case H5D_COMPACT:
            /* Size of raw data */
            UINT16ENCODE(p, mesg->storage.u.compact.size);

            /* Raw data */
            if(mesg->storage.u.compact.size > 0) {
                if(mesg->storage.u.compact.buf)
                    HDmemcpy(p, mesg->storage.u.compact.buf, mesg->storage.u.compact.size);
                else
                    HDmemset(p, 0, mesg->storage.u.compact.size);
                p += mesg->storage.u.compact.size;
            } /* end if */
            break;

        case H5D_CONTIGUOUS:
            H5F_addr_encode(f, &p, mesg->storage.u.contig.addr);
            H5F_ENCODE_LENGTH(f, p, mesg->storage.u.contig.size);
            break;

        case H5D_CHUNKED:
            /* Number of dimensions */
            HDassert(mesg->u.chunk.ndims > 0 && mesg->u.chunk.ndims <= H5O_LAYOUT_NDIMS);
            *p++ = (uint8_t)mesg->u.chunk.ndims;

            /* B-tree address */
            H5F_addr_encode(f, &p, mesg->storage.u.chunk.idx_addr);

            /* Dimension sizes */
            for(u = 0; u < mesg->u.chunk.ndims; u++)
                UINT32ENCODE(p, mesg->u.chunk.dim[u]);
            break;

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        default:
            HGOTO_ERROR(H5E_OHDR, H5E_CANTENCODE, FAIL, "Invalid layout class")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_layout_encode() */


/*-------------------------------------------------------------------------
 * Function:    H5O_layout_copy
 *
 * Purpose:     Copies a message from _MESG to _DEST, allocating _DEST if
 *              necessary.
 *
 * Return:      Success:        Ptr to _DEST
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_layout_copy(const void *_mesg, void *_dest)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    H5O_layout_t           *dest = (H5O_layout_t *) _dest;
    void                   *ret_value;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(mesg);

    /* Allocate destination message, if necessary */
    if(!dest && NULL == (dest = H5FL_MALLOC(H5O_layout_t)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTALLOC, NULL, "layout message allocation failed")

    /* copy */
    *dest = *mesg;

    /* Deep copy the buffer for compact datasets also */
    if(mesg->type == H5D_COMPACT && mesg->storage.u.compact.size > 0) {
        /* Allocate memory for the raw data */
        if(NULL == (dest->storage.u.compact.buf = H5MM_malloc(dest->storage.u.compact.size)))
            HGOTO_ERROR(H5E_OHDR, H5E_NOSPACE, NULL, "unable to allocate memory for compact dataset")

        /* Copy over the raw data */
        HDmemcpy(dest->storage.u.compact.buf, mesg->storage.u.compact.buf, dest->storage.u.compact.size);
    } /* end if */

    /* Reset the pointer of the chunked storage index but not the address */
    if(dest->type == H5D_CHUNKED && dest->storage.u.chunk.ops)
	H5D_chunk_idx_reset(&dest->storage.u.chunk, FALSE);

    /* Set return value */
    ret_value = dest;

done:
    if(ret_value == NULL)
        if(NULL == _dest)
            dest = H5FL_FREE(H5O_layout_t, dest);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_layout_copy() */


/*-------------------------------------------------------------------------
 * Function:    H5O_layout_size
 *
 * Purpose:     Returns the size of the raw message in bytes.  If it's
 *              compact dataset, the data part is also included.
 *              This function doesn't take into account message alignment.
 *
 * Return:      Success:        Message data size in bytes
 *
 *              Failure:        0
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O_layout_size(const H5F_t *f, hbool_t UNUSED disable_shared, const void *_mesg)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    size_t                  ret_value;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(f);
    HDassert(mesg);

    /* Compute serialized size */
    /* (including possibly compact data) */
    ret_value = H5D__layout_meta_size(f, mesg, TRUE);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_layout_size() */


/*-------------------------------------------------------------------------
 * Function:	H5O_layout_reset
 *
 * Purpose:	Frees resources within a data type message, but doesn't free
 *		the message itself.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Friday, September 13, 2002
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_layout_reset(void *_mesg)
{
    H5O_layout_t     *mesg = (H5O_layout_t *)_mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(mesg) {
        /* Free the compact storage buffer */
        if(H5D_COMPACT == mesg->type)
            mesg->storage.u.compact.buf = H5MM_xfree(mesg->storage.u.compact.buf);

        /* Reset the message */
        mesg->type = H5D_CONTIGUOUS;
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_layout_reset() */


/*-------------------------------------------------------------------------
 * Function:	H5O_layout_free
 *
 * Purpose:	Free's the message
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Saturday, March 11, 2000
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_layout_free(void *_mesg)
{
    H5O_layout_t     *mesg = (H5O_layout_t *) _mesg;
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(mesg);

    /* Free resources within the message */
    if(H5O_layout_reset(mesg) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to free message resources")

    mesg = H5FL_FREE(H5O_layout_t, mesg);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_layout_free() */


/*-------------------------------------------------------------------------
 * Function:    H5O_layout_delete
 *
 * Purpose:     Free file space referenced by message
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, March 19, 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_layout_delete(H5F_t *f, hid_t dxpl_id, H5O_t *open_oh, void *_mesg)
{
    H5O_layout_t *mesg = (H5O_layout_t *) _mesg;
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f);
    HDassert(open_oh);
    HDassert(mesg);

    /* Perform different actions, depending on the type of storage */
    switch(mesg->type) {
        case H5D_COMPACT:       /* Compact data storage */
            /* Nothing required */
            break;

        case H5D_CONTIGUOUS:    /* Contiguous block on disk */
            /* Free the file space for the raw data */
            if(H5D__contig_delete(f, dxpl_id, &mesg->storage) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to free raw data")
            break;

        case H5D_CHUNKED:       /* Chunked blocks on disk */
            /* Free the file space for the index & chunk raw data */
            if(H5D__chunk_delete(f, dxpl_id, open_oh, &mesg->storage) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to free raw data")
            break;

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        default:
            HGOTO_ERROR(H5E_OHDR, H5E_BADTYPE, FAIL, "not valid storage type")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_layout_delete() */


/*-------------------------------------------------------------------------
 * Function:    H5O_layout_copy_file
 *
 * Purpose:     Copies a message from _MESG to _DEST in file
 *
 * Return:      Success:        Ptr to _DEST
 *
 *              Failure:        NULL
 *
 * Programmer:  Peter Cao
 *              July 23, 2005
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_layout_copy_file(H5F_t *file_src, void *mesg_src, H5F_t *file_dst,
    hbool_t UNUSED *recompute_size, unsigned UNUSED *mesg_flags,
    H5O_copy_t *cpy_info, void *_udata, hid_t dxpl_id)
{
    H5D_copy_file_ud_t *udata = (H5D_copy_file_ud_t *)_udata;   /* Dataset copying user data */
    H5O_layout_t       *layout_src = (H5O_layout_t *) mesg_src;
    H5O_layout_t       *layout_dst = NULL;
    hbool_t             copied = FALSE;                         /* Whether the data was copied */
    void               *ret_value;                              /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(file_src);
    HDassert(layout_src);
    HDassert(file_dst);

    /* Allocate space for the destination layout */
    if(NULL == (layout_dst = H5FL_MALLOC(H5O_layout_t)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTALLOC, NULL, "memory allocation failed")

    /* Copy the "top level" information */
    *layout_dst = *layout_src;

    /* Copy the layout type specific information */
    switch(layout_src->type) {
        case H5D_COMPACT:
	    if(layout_src->storage.u.compact.buf) {
                /* copy compact raw data */
                if(H5D__compact_copy(file_src, &layout_src->storage.u.compact, file_dst, &layout_dst->storage.u.compact, udata->src_dtype, cpy_info, dxpl_id) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, NULL, "unable to copy chunked storage")
                copied = TRUE;
	    } /* end if */
            break;

        case H5D_CONTIGUOUS:
            /* Compute the size of the contiguous storage for versions of the
             * layout message less than version 3 because versions 1 & 2 would
             * truncate the dimension sizes to 32-bits of information. - QAK 5/26/04
             */
            if(layout_src->version < H5O_LAYOUT_VERSION_3)
                layout_dst->storage.u.contig.size = H5S_extent_nelem(udata->src_space_extent) *
                                        H5T_get_size(udata->src_dtype);

            if(H5D__contig_is_space_alloc(&layout_src->storage)) {
                /* copy contiguous raw data */
                if(H5D__contig_copy(file_src, &layout_src->storage.u.contig, file_dst, &layout_dst->storage.u.contig, udata->src_dtype, cpy_info, dxpl_id) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, NULL, "unable to copy contiguous storage")
                copied = TRUE;
            } /* end if */
            break;

        case H5D_CHUNKED:
            if(H5D__chunk_is_space_alloc(&layout_src->storage)) {
                /* Create chunked layout */
                if(H5D__chunk_copy(file_src, &layout_src->storage.u.chunk, &layout_src->u.chunk, file_dst, &layout_dst->storage.u.chunk, udata->src_space_extent, udata->src_dtype, udata->common.src_pline, cpy_info, dxpl_id) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, NULL, "unable to copy chunked storage")
                copied = TRUE;
            } /* end if */
            break;

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        default:
            HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "Invalid layout class")
    } /* end switch */

    /* Check if copy routine was invoked (which frees the source datatype) */
    if(copied)
        udata->src_dtype = NULL;

    /* Set return value */
    ret_value = layout_dst;

done:
    if(!ret_value)
	if(layout_dst)
	    layout_dst = H5FL_FREE(H5O_layout_t, layout_dst);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_layout_copy_file() */


/*-------------------------------------------------------------------------
 * Function:    H5O_layout_debug
 *
 * Purpose:     Prints debugging info for a message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_layout_debug(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, const void *_mesg,
    FILE * stream, int indent, int fwidth)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    unsigned                    u;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(f);
    HDassert(mesg);
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
              "Version:", mesg->version);
    switch(mesg->type) {
        case H5D_CHUNKED:
            HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                      "Type:", "Chunked");

            /* Chunk # of dims & size */
            HDfprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
                      "Number of dimensions:",
                      (unsigned long)(mesg->u.chunk.ndims));
            HDfprintf(stream, "%*s%-*s {", indent, "", fwidth, "Size:");
            for(u = 0; u < mesg->u.chunk.ndims; u++)
                HDfprintf(stream, "%s%lu", u ? ", " : "", (unsigned long)(mesg->u.chunk.dim[u]));
            HDfprintf(stream, "}\n");

            /* Index information */
            switch(mesg->storage.u.chunk.idx_type) {
                case H5D_CHUNK_BTREE:
                    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                              "Index Type:", "v1 B-tree");
                    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
                              "B-tree address:", mesg->storage.u.chunk.idx_addr);
                    break;

                default:
                    HDfprintf(stream, "%*s%-*s %s (%u)\n", indent, "", fwidth,
                              "Index Type:", "Unknown", (unsigned)mesg->storage.u.chunk.idx_type);
                    break;
            } /* end switch */
            break;

        case H5D_CONTIGUOUS:
            HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                      "Type:", "Contiguous");
            HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
                      "Data address:", mesg->storage.u.contig.addr);
            HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
                      "Data Size:", mesg->storage.u.contig.size);
            break;

        case H5D_COMPACT:
            HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                      "Type:", "Compact");
            HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
                      "Data Size:", mesg->storage.u.compact.size);
            break;

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        default:
            HDfprintf(stream, "%*s%-*s %s (%u)\n", indent, "", fwidth,
                      "Type:", "Unknown", (unsigned)mesg->type);
            break;
    } /* end switch */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_layout_debug() */

