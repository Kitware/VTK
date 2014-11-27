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
 *              Wednesday, September 30, 1998
 *
 * Purpose:	The fill message indicates a bit pattern to use for
 *		uninitialized data points of a dataset.
 */

#define H5O_PACKAGE		/*suppress error about including H5Opkg	  */

#include "H5private.h"		/* Generic Functions			*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free Lists				*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Opkg.h"             /* Object headers			*/
#include "H5Pprivate.h"		/* Property lists			*/
#include "H5Sprivate.h"		/* Dataspaces				*/


static void  *H5O_fill_old_decode(H5F_t *f, hid_t dxpl_id, H5O_t *open_oh,
    unsigned mesg_flags, unsigned *ioflags, const uint8_t *p);
static herr_t H5O_fill_old_encode(H5F_t *f, uint8_t *p, const void *_mesg);
static size_t H5O_fill_old_size(const H5F_t *f, const void *_mesg);
static void  *H5O_fill_new_decode(H5F_t *f, hid_t dxpl_id, H5O_t *open_oh,
    unsigned mesg_flags, unsigned *ioflags, const uint8_t *p);
static herr_t H5O_fill_new_encode(H5F_t *f, uint8_t *p, const void *_mesg);
static size_t H5O_fill_new_size(const H5F_t *f, const void *_mesg);
static void  *H5O_fill_copy(const void *_mesg, void *_dest);
static herr_t H5O_fill_reset(void *_mesg);
static herr_t H5O_fill_free(void *_mesg);
static herr_t H5O_fill_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg, FILE *stream,
			     int indent, int fwidth);

/* Set up & include shared message "interface" info */
#define H5O_SHARED_TYPE			H5O_MSG_FILL
#define H5O_SHARED_DECODE		H5O_fill_shared_decode
#define H5O_SHARED_DECODE_REAL		H5O_fill_old_decode
#define H5O_SHARED_ENCODE		H5O_fill_shared_encode
#define H5O_SHARED_ENCODE_REAL		H5O_fill_old_encode
#define H5O_SHARED_SIZE			H5O_fill_shared_size
#define H5O_SHARED_SIZE_REAL		H5O_fill_old_size
#define H5O_SHARED_DELETE		H5O_fill_shared_delete
#undef H5O_SHARED_DELETE_REAL
#define H5O_SHARED_LINK			H5O_fill_shared_link
#undef H5O_SHARED_LINK_REAL
#define H5O_SHARED_COPY_FILE		H5O_fill_shared_copy_file
#undef H5O_SHARED_COPY_FILE_REAL
#define H5O_SHARED_POST_COPY_FILE	H5O_fill_shared_post_copy_file
#undef H5O_SHARED_POST_COPY_FILE_REAL
#undef  H5O_SHARED_POST_COPY_FILE_UPD
#define H5O_SHARED_DEBUG		H5O_fill_shared_debug
#define H5O_SHARED_DEBUG_REAL		H5O_fill_debug
#include "H5Oshared.h"			/* Shared Object Header Message Callbacks */

/* Set up & include shared message "interface" info */
/* (Kludgy 'undef's in order to re-include the H5Oshared.h header) */
#undef H5O_SHARED_TYPE
#define H5O_SHARED_TYPE			H5O_MSG_FILL_NEW
#undef H5O_SHARED_DECODE
#define H5O_SHARED_DECODE		H5O_fill_new_shared_decode
#undef H5O_SHARED_DECODE_REAL
#define H5O_SHARED_DECODE_REAL		H5O_fill_new_decode
#undef H5O_SHARED_ENCODE
#define H5O_SHARED_ENCODE		H5O_fill_new_shared_encode
#undef H5O_SHARED_ENCODE_REAL
#define H5O_SHARED_ENCODE_REAL		H5O_fill_new_encode
#undef H5O_SHARED_SIZE
#define H5O_SHARED_SIZE			H5O_fill_new_shared_size
#undef H5O_SHARED_SIZE_REAL
#define H5O_SHARED_SIZE_REAL		H5O_fill_new_size
#undef H5O_SHARED_DELETE
#define H5O_SHARED_DELETE		H5O_fill_new_shared_delete
#undef H5O_SHARED_DELETE_REAL
#undef H5O_SHARED_LINK
#define H5O_SHARED_LINK			H5O_fill_new_shared_link
#undef H5O_SHARED_LINK_REAL
#undef H5O_SHARED_COPY_FILE
#define H5O_SHARED_COPY_FILE		H5O_fill_new_shared_copy_file
#undef H5O_SHARED_COPY_FILE_REAL
#undef H5O_SHARED_POST_COPY_FILE
#define H5O_SHARED_POST_COPY_FILE	H5O_fill_new_shared_post_copy_file
#undef H5O_SHARED_POST_COPY_FILE_REAL
#undef  H5O_SHARED_POST_COPY_FILE_UPD
#undef H5O_SHARED_DEBUG
#define H5O_SHARED_DEBUG		H5O_fill_new_shared_debug
#undef H5O_SHARED_DEBUG_REAL
#define H5O_SHARED_DEBUG_REAL		H5O_fill_debug
#undef H5Oshared_H
#include "H5Oshared.h"			/* Shared Object Header Message Callbacks */

/* This message derives from H5O message class, for old fill value before version 1.5 */
const H5O_msg_class_t H5O_MSG_FILL[1] = {{
    H5O_FILL_ID,                /*message id number                     */
    "fill",                     /*message name for debugging            */
    sizeof(H5O_fill_t),		/*native message size                   */
    H5O_SHARE_IS_SHARABLE|H5O_SHARE_IN_OHDR,	/* messages are sharable?       */
    H5O_fill_shared_decode,	/*decode message                        */
    H5O_fill_shared_encode,	/*encode message                        */
    H5O_fill_copy,		/*copy the native value                 */
    H5O_fill_shared_size,	/*raw message size			*/
    H5O_fill_reset,		/*free internal memory			*/
    H5O_fill_free,		/* free method				*/
    H5O_fill_shared_delete,	/* file delete method			*/
    H5O_fill_shared_link,	/* link method				*/
    NULL,			/* set share method			*/
    NULL,		    	/*can share method		*/
    NULL,			/* pre copy native value to file	*/
    H5O_fill_shared_copy_file,	/* copy native value to file		*/
    H5O_fill_shared_post_copy_file,	/* post copy native value to file	*/
    NULL,			/* get creation index		*/
    NULL,			/* set creation index		*/
    H5O_fill_shared_debug       /*debug the message			*/
}};

/* This message derives from H5O message class, for new fill value after version 1.4  */
const H5O_msg_class_t H5O_MSG_FILL_NEW[1] = {{
    H5O_FILL_NEW_ID,		/*message id number			*/
    "fill_new", 		/*message name for debugging		*/
    sizeof(H5O_fill_t),		/*native message size			*/
    H5O_SHARE_IS_SHARABLE|H5O_SHARE_IN_OHDR,	/* messages are sharable?       */
    H5O_fill_new_shared_decode,	/*decode message			*/
    H5O_fill_new_shared_encode,	/*encode message			*/
    H5O_fill_copy,		/*copy the native value			*/
    H5O_fill_new_shared_size,	/*raw message size			*/
    H5O_fill_reset,		/*free internal memory			*/
    H5O_fill_free,		/* free method				*/
    H5O_fill_new_shared_delete,	/* file delete method			*/
    H5O_fill_new_shared_link,	/* link method				*/
    NULL,			/* set share method			*/
    NULL,		    	/*can share method		*/
    NULL,			/* pre copy native value to file	*/
    H5O_fill_new_shared_copy_file, /* copy native value to file		*/
    H5O_fill_new_shared_post_copy_file,	/* post copy native value to file	*/
    NULL,			/* get creation index		*/
    NULL,			/* set creation index		*/
    H5O_fill_new_shared_debug	/*debug the message			*/
}};

/* Masks, shift values & flags for fill value message */
#define H5O_FILL_MASK_ALLOC_TIME        0x03
#define H5O_FILL_SHIFT_ALLOC_TIME       0
#define H5O_FILL_MASK_FILL_TIME         0x03
#define H5O_FILL_SHIFT_FILL_TIME        2
#define H5O_FILL_FLAG_UNDEFINED_VALUE   0x10
#define H5O_FILL_FLAG_HAVE_VALUE        0x20
#define H5O_FILL_FLAGS_ALL              (H5O_FILL_MASK_ALLOC_TIME | (H5O_FILL_MASK_FILL_TIME << H5O_FILL_SHIFT_FILL_TIME) | H5O_FILL_FLAG_UNDEFINED_VALUE | H5O_FILL_FLAG_HAVE_VALUE)

/* Declare a free list to manage the H5O_fill_t struct */
H5FL_DEFINE(H5O_fill_t);

/* Declare extern the free list to manage blocks of type conversion data */
H5FL_BLK_EXTERN(type_conv);


/*-------------------------------------------------------------------------
 * Function:	H5O_fill_new_decode
 *
 * Purpose:	Decode a new fill value message.  The new fill value
 * 		message is fill value plus space allocation time and
 * 		fill value writing time and whether fill value is defined.
 *
 * Return:	Success:	Ptr to new message in native struct.
 *		Failure:	NULL
 *
 * Programmer:  Raymond Lu
 *              Feb 26, 2002
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_fill_new_decode(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, H5O_t UNUSED *open_oh,
    unsigned UNUSED mesg_flags, unsigned UNUSED *ioflags, const uint8_t *p)
{
    H5O_fill_t	*fill = NULL;
    void	*ret_value;

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(f);
    HDassert(p);

    if(NULL == (fill = H5FL_CALLOC(H5O_fill_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for fill value message")

    /* Version */
    fill->version = *p++;
    if(fill->version < H5O_FILL_VERSION_1 || fill->version > H5O_FILL_VERSION_LATEST)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "bad version number for fill value message")

    /* Decode each version */
    if(fill->version < H5O_FILL_VERSION_3) {
        /* Space allocation time */
        fill->alloc_time = (H5D_alloc_time_t)*p++;

        /* Fill value write time */
        fill->fill_time = (H5D_fill_time_t)*p++;

        /* Whether fill value is defined */
        fill->fill_defined = *p++;

        /* Only decode fill value information if one is defined */
        if(fill->fill_defined) {
            INT32DECODE(p, fill->size);
            if(fill->size > 0) {
                H5_CHECK_OVERFLOW(fill->size, ssize_t, size_t);
                if(NULL == (fill->buf = H5MM_malloc((size_t)fill->size)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for fill value")
                HDmemcpy(fill->buf, p, (size_t)fill->size);
            } /* end if */
        } /* end if */
        else
            fill->size = (-1);
    } /* end if */
    else {
        unsigned flags;          /* Status flags */

        /* Flags */
        flags = *p++;

        /* Check for unknown flags */
        if(flags & (unsigned)~H5O_FILL_FLAGS_ALL)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "unknown flag for fill value message")

        /* Space allocation time */
        fill->alloc_time = (H5D_alloc_time_t)((flags >> H5O_FILL_SHIFT_ALLOC_TIME) & H5O_FILL_MASK_ALLOC_TIME);

        /* Fill value write time */
        fill->fill_time = (H5D_fill_time_t)((flags >> H5O_FILL_SHIFT_FILL_TIME) & H5O_FILL_MASK_FILL_TIME);

        /* Check for undefined fill value */
        if(flags & H5O_FILL_FLAG_UNDEFINED_VALUE) {
            /* Sanity check */
            HDassert(!(flags & H5O_FILL_FLAG_HAVE_VALUE));

            /* Set value for "undefined" fill value */
            fill->size = (-1);
        } /* end if */
        else if(flags & H5O_FILL_FLAG_HAVE_VALUE) {
            /* Fill value size */
            UINT32DECODE(p, fill->size);

            /* Fill value */
            H5_CHECK_OVERFLOW(fill->size, ssize_t, size_t);
            if(NULL == (fill->buf = H5MM_malloc((size_t)fill->size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for fill value")
            HDmemcpy(fill->buf, p, (size_t)fill->size);

            /* Set the "defined" flag */
            fill->fill_defined = TRUE;
        } /* end else */
        else {
            /* Set the "defined" flag */
            fill->fill_defined = TRUE;
        } /* end else */
    } /* end else */

    /* Set return value */
    ret_value = (void *)fill;

done:
    if(!ret_value && fill) {
        if(fill->buf)
            H5MM_xfree(fill->buf);
	fill = H5FL_FREE(H5O_fill_t, fill);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_fill_new_decode() */


/*-------------------------------------------------------------------------
 * Function:    H5O_fill_old_decode
 *
 * Purpose:     Decode an old fill value message.
 *
 * Return:      Success:        Ptr to new message in native struct.
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, September 30, 1998
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_fill_old_decode(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, H5O_t UNUSED *open_oh,
    unsigned UNUSED mesg_flags, unsigned UNUSED *ioflags, const uint8_t *p)
{
    H5O_fill_t *fill = NULL;		/* Decoded fill value message */
    void *ret_value;                    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(f);
    HDassert(p);

    if(NULL == (fill = H5FL_CALLOC(H5O_fill_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for fill value message")

    /* Set non-zero default fields */
    fill->version = H5O_FILL_VERSION_2;
    fill->alloc_time = H5D_ALLOC_TIME_LATE;
    fill->fill_time = H5D_FILL_TIME_IFSET;

    /* Fill value size */
    UINT32DECODE(p, fill->size);

    /* Only decode the fill value itself if there is one */
    if(fill->size > 0) {
        if(NULL == (fill->buf = H5MM_malloc((size_t)fill->size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for fill value")
        HDmemcpy(fill->buf, p, (size_t)fill->size);
        fill->fill_defined = TRUE;
    } /* end if */
    else
        fill->size = (-1);

    /* Set return value */
    ret_value = (void*)fill;

done:
    if(!ret_value && fill) {
        if(fill->buf)
            H5MM_xfree(fill->buf);
	fill = H5FL_FREE(H5O_fill_t, fill);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_fill_old_decode() */


/*-------------------------------------------------------------------------
 * Function:	H5O_fill_new_encode
 *
 * Purpose:	Encode a new fill value message.  The new fill value
 *              message is fill value plus space allocation time and
 *              fill value writing time and whether fill value is defined.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Raymond Lu
 *              Feb 26, 2002
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_fill_new_encode(H5F_t UNUSED *f, uint8_t *p, const void *_fill)
{
    const H5O_fill_t	*fill = (const H5O_fill_t *)_fill;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(f);
    HDassert(p);
    HDassert(fill && NULL == fill->type);

    /* Version */
    *p++ = (uint8_t)fill->version;

    if(fill->version < H5O_FILL_VERSION_3) {
        /* Space allocation time */
        *p++ = fill->alloc_time;

        /* Fill value writing time */
        *p++ = fill->fill_time;

        /* Whether fill value is defined */
        *p++ = (uint8_t)fill->fill_defined;

        /* Only write out the size and fill value if it is defined */
        if(fill->fill_defined) {
            UINT32ENCODE(p, fill->size);
            if(fill->size > 0)
                if(fill->buf) {
                    H5_CHECK_OVERFLOW(fill->size, ssize_t, size_t);
                    HDmemcpy(p, fill->buf, (size_t)fill->size);
                } /* end if */
        } /* end if */
    } /* end if */
    else {
        uint8_t flags = 0;      /* Fill value setting flags */

        /* Encode space allocation time */
        HDassert(fill->alloc_time == (H5O_FILL_MASK_ALLOC_TIME & fill->alloc_time));
        flags = (uint8_t)(flags | ((H5O_FILL_MASK_ALLOC_TIME & fill->alloc_time) << H5O_FILL_SHIFT_ALLOC_TIME));

        /* Encode fill value writing time */
        HDassert(fill->fill_time == (H5O_FILL_MASK_FILL_TIME & fill->fill_time));
        flags = (uint8_t)(flags | ((H5O_FILL_MASK_FILL_TIME & fill->fill_time) << H5O_FILL_SHIFT_FILL_TIME));

        /* Check if we need to encode a fill value size */
        if(fill->size < 0) {
            /* Indicate that the fill value has been "undefined" by the user */
            flags |= H5O_FILL_FLAG_UNDEFINED_VALUE;

            /* Flags */
            *p++ = (uint8_t)flags;

            /* Sanity check */
            HDassert(!fill->buf);
        } /* end if */
        else if(fill->size > 0) {
            /* Indicate that a fill value size is present */
            flags |= H5O_FILL_FLAG_HAVE_VALUE;

            /* Flags */
            *p++ = (uint8_t)flags;

            /* Encode the size of fill value */
            INT32ENCODE(p, fill->size);

            /* Encode the fill value */
            HDassert(fill->buf);
            H5_CHECK_OVERFLOW(fill->size, ssize_t, size_t);
            HDmemcpy(p, fill->buf, (size_t)fill->size);
        } /* end if */
        else {
            /* Flags */
            *p++ = (uint8_t)flags;

            /* Sanity check */
            HDassert(!fill->buf);
        } /* end else */
    } /* end else */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_fill_new_encode() */


/*-------------------------------------------------------------------------
 * Function:    H5O_fill_old_encode
 *
 * Purpose:     Encode an old fill value message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, October  1, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_fill_old_encode(H5F_t UNUSED *f, uint8_t *p, const void *_fill)
{
    const H5O_fill_t *fill = (const H5O_fill_t *)_fill;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(f);
    HDassert(p);
    HDassert(fill && NULL == fill->type);

    UINT32ENCODE(p, fill->size);
    if(fill->buf)
        HDmemcpy(p, fill->buf, (size_t)fill->size);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_fill_old_encode() */


/*-------------------------------------------------------------------------
 * Function:	H5O_fill_copy
 *
 * Purpose:	Copies a message from _MESG to _DEST, allocating _DEST if
 *		necessary.  The new fill value message is fill value plus
 *		space allocation time and fill value writing time and
 *		whether fill value is defined.
 *
 * Return:	Success:	Ptr to _DEST
 *		Failure:	NULL
 *
 * Programmer:  Raymond Lu
 *              Feb 26, 2002
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_fill_copy(const void *_src, void *_dst)
{
    const H5O_fill_t	*src = (const H5O_fill_t *)_src;
    H5O_fill_t		*dst = (H5O_fill_t *)_dst;
    void		*ret_value;

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(src);

    if(!dst && NULL == (dst = H5FL_MALLOC(H5O_fill_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for fill message")

    /* Shallow copy basic fields */
    *dst = *src;

    /* Copy data type of fill value */
    if(src->type) {
        if(NULL == (dst->type = H5T_copy(src->type, H5T_COPY_TRANSIENT)))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, NULL, "can't copy datatype")
    } /* end if */
    else
        dst->type = NULL;

    /* Copy fill value and its size */
    if(src->buf) {
        H5_CHECK_OVERFLOW(src->size, ssize_t, size_t);
	if(NULL == (dst->buf = H5MM_malloc((size_t)src->size)))
	    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for fill value")
	HDmemcpy(dst->buf, src->buf, (size_t)src->size);

        /* Check for needing to convert/copy fill value */
        if(src->type) {
            H5T_path_t *tpath;      /* Conversion information */

            /* Set up type conversion function */
            if(NULL == (tpath = H5T_path_find(src->type, dst->type, NULL, NULL, H5AC_ind_dxpl_id, FALSE)))
                HGOTO_ERROR(H5E_OHDR, H5E_UNSUPPORTED, NULL, "unable to convert between src and dst data types")

            /* If necessary, convert fill value datatypes (which copies VL components, etc.) */
            if(!H5T_path_noop(tpath)) {
                hid_t dst_id, src_id;       /* Source & destination datatypes for type conversion */
                uint8_t *bkg_buf = NULL;    /* Background conversion buffer */
                size_t bkg_size;            /* Size of background buffer */

                /* Wrap copies of types to convert */
                dst_id = H5I_register(H5I_DATATYPE, H5T_copy(dst->type, H5T_COPY_TRANSIENT), FALSE);
                if(dst_id < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "unable to copy/register datatype")
                src_id = H5I_register(H5I_DATATYPE, H5T_copy(src->type, H5T_COPY_ALL), FALSE);
                if(src_id < 0) {
                    H5I_dec_ref(dst_id);
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "unable to copy/register datatype")
                } /* end if */

                /* Allocate a background buffer */
                bkg_size = MAX(H5T_get_size(dst->type), H5T_get_size(src->type));
                if(H5T_path_bkg(tpath) && NULL == (bkg_buf = H5FL_BLK_CALLOC(type_conv, bkg_size))) {
                    H5I_dec_ref(src_id);
                    H5I_dec_ref(dst_id);
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
                } /* end if */

                /* Convert fill value */
                if(H5T_convert(tpath, src_id, dst_id, (size_t)1, (size_t)0, (size_t)0, dst->buf, bkg_buf, H5AC_ind_dxpl_id) < 0) {
                    H5I_dec_ref(src_id);
                    H5I_dec_ref(dst_id);
                    if(bkg_buf)
                        bkg_buf = H5FL_BLK_FREE(type_conv, bkg_buf);
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTCONVERT, NULL, "datatype conversion failed")
                } /* end if */

                /* Release the background buffer */
                H5I_dec_ref(src_id);
                H5I_dec_ref(dst_id);
                if(bkg_buf)
                    bkg_buf = H5FL_BLK_FREE(type_conv, bkg_buf);
            } /* end if */
        } /* end if */
    } /* end if */
    else
        dst->buf = NULL;

    /* Set return value */
    ret_value = dst;

done:
    if(!ret_value && dst) {
        if(dst->buf)
            H5MM_xfree(dst->buf);
	if(dst->type)
            H5T_close(dst->type);
	if(!_dst)
            dst = H5FL_FREE(H5O_fill_t, dst);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_fill_copy() */


/*-------------------------------------------------------------------------
 * Function:	H5O_fill_new_size
 *
 * Purpose:	Returns the size of the raw message in bytes not counting the
 *		message type or size fields, but only the data fields.  This
 *		function doesn't take into account alignment.  The new fill
 *		value message is fill value plus space allocation time and
 *              fill value writing time and whether fill value is defined.
 *
 * Return:	Success:	Message data size in bytes w/o alignment.
 *		Failure:	0
 *
 * Programmer:  Raymond Lu
 *              Feb 26, 2002
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O_fill_new_size(const H5F_t UNUSED *f, const void *_fill)
{
    const H5O_fill_t	*fill = (const H5O_fill_t *)_fill;
    size_t		ret_value;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(f);
    HDassert(fill);

    /* Determine size for different versions */
    if(fill->version < H5O_FILL_VERSION_3) {
        ret_value = 1 +	 		/* Version number        */
                    1 + 		/* Space allocation time */
                    1 + 		/* Fill value write time */
                    1; 			/* Fill value defined    */
        if(fill->fill_defined)
            ret_value += 4 +	/* Fill value size	 */
                    (fill->size > 0 ? (size_t)fill->size : 0);	/* Size of fill value	 */
    } /* end if */
    else {
        ret_value = 1 +	 		/* Version number        */
                    1;			/* Status flags          */
        if(fill->size > 0)
            ret_value += 4 +		/* Fill value size	 */
                    (size_t)fill->size;		/* Size of fill value	 */
    } /* end else */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_fill_new_size() */


/*-------------------------------------------------------------------------
 * Function:    H5O_fill_old_size
 *
 * Purpose:     Returns the size of the raw message in bytes not counting the
 *              message type or size fields, but only the data fields.  This
 *              function doesn't take into account alignment.
 *
 * Return:      Success:        Message data size in bytes w/o alignment.
 *              Failure:        0
 *
 * Programmer:  Robb Matzke
 *              Thursday, October  1, 1998
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O_fill_old_size(const H5F_t UNUSED *f, const void *_fill)
{
    const H5O_fill_t *fill = (const H5O_fill_t *)_fill;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(fill);

    FUNC_LEAVE_NOAPI(4 + (size_t)fill->size)
} /* end H5O_fill_old_size() */


/*-------------------------------------------------------------------------
 * Function:	H5O_fill_reset_dyn
 *
 * Purpose:	Resets dynamic fill value fields
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, January 22, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_fill_reset_dyn(H5O_fill_t *fill)
{
    hid_t fill_type_id = -1;            /* Datatype ID for fill value datatype when reclaiming VL fill values */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(fill);

    if(fill->buf) {
        if(fill->type && H5T_detect_class(fill->type, H5T_VLEN, FALSE) > 0) {
            H5T_t *fill_type;           /* Copy of fill value datatype */
            H5S_t *fill_space;          /* Scalar dataspace for fill value element */

            /* Copy the fill value datatype and get an ID for it */
            if(NULL == (fill_type = H5T_copy(fill->type, H5T_COPY_TRANSIENT)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to copy fill value datatype")
            if((fill_type_id = H5I_register(H5I_DATATYPE, fill_type, FALSE)) < 0) {
                H5T_close(fill_type);
                HGOTO_ERROR(H5E_OHDR, H5E_CANTREGISTER, FAIL, "unable to register fill value datatype")
            } /* end if */

            /* Create a scalar dataspace for the fill value element */
            if(NULL == (fill_space = H5S_create(H5S_SCALAR)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTCREATE, FAIL, "can't create scalar dataspace")

            /* Reclaim any variable length components of the fill value */
            if(H5D_vlen_reclaim(fill_type_id, fill_space, H5P_DATASET_XFER_DEFAULT, fill->buf) < 0) {
                H5S_close(fill_space);
                HGOTO_ERROR(H5E_OHDR, H5E_BADITER, FAIL, "unable to reclaim variable-length fill value data")
            } /* end if */

            /* Release the scalar fill value dataspace */
            H5S_close(fill_space);
        } /* end if */

        /* Release the fill value buffer now */
        fill->buf = H5MM_xfree(fill->buf);
    } /* end if */
    fill->size = 0;
    if(fill->type) {
	H5T_close(fill->type);
	fill->type = NULL;
    } /* end if */

done:
    if(fill_type_id > 0 && H5I_dec_ref(fill_type_id) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTDEC, FAIL, "unable to decrement ref count for temp ID")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_fill_reset_dyn() */


/*-------------------------------------------------------------------------
 * Function:	H5O_fill_reset
 *
 * Purpose:	Resets a message to an initial state.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, October  1, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_fill_reset(void *_fill)
{
    H5O_fill_t	*fill = (H5O_fill_t *)_fill;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(fill);

    /* Reset dynamic fields */
    H5O_fill_reset_dyn(fill);

    /* Reset value fields */
    fill->alloc_time   = H5D_ALLOC_TIME_LATE;
    fill->fill_time    = H5D_FILL_TIME_IFSET;
    fill->fill_defined = FALSE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_fill_reset() */


/*-------------------------------------------------------------------------
 * Function:	H5O_fill_free
 *
 * Purpose:	Frees the message
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, December 5, 2002
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_fill_free(void *fill)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(fill);

    fill = H5FL_FREE(H5O_fill_t, fill);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_fill_free() */


/*-------------------------------------------------------------------------
 * Function:	H5O_fill_debug
 *
 * Purpose:	Prints debugging info for the message.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, October  1, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_fill_debug(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, const void *_fill, FILE *stream,
	       int indent, int fwidth)
{
    const H5O_fill_t *fill = (const H5O_fill_t *)_fill;
    H5D_fill_value_t fill_status;       /* Whether the fill value is defined */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(f);
    HDassert(fill);
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s ", indent, "", fwidth, "Space Allocation Time:");
    switch(fill->alloc_time) {
        case H5D_ALLOC_TIME_EARLY:
            fprintf(stream,"Early\n");
            break;

        case H5D_ALLOC_TIME_LATE:
            fprintf(stream,"Late\n");
            break;

        case H5D_ALLOC_TIME_INCR:
            fprintf(stream,"Incremental\n");
            break;

        case H5D_ALLOC_TIME_DEFAULT:
        case H5D_ALLOC_TIME_ERROR:
        default:
            fprintf(stream,"Unknown!\n");
            break;
    } /* end switch */
    HDfprintf(stream, "%*s%-*s ", indent, "", fwidth, "Fill Time:");
    switch(fill->fill_time) {
        case H5D_FILL_TIME_ALLOC:
            fprintf(stream,"On Allocation\n");
            break;

        case H5D_FILL_TIME_NEVER:
            fprintf(stream,"Never\n");
            break;

        case H5D_FILL_TIME_IFSET:
            fprintf(stream,"If Set\n");
            break;

        case H5D_FILL_TIME_ERROR:
        default:
            fprintf(stream,"Unknown!\n");
            break;

    } /* end switch */
    HDfprintf(stream, "%*s%-*s ", indent, "", fwidth, "Fill Value Defined:");
    H5P_is_fill_value_defined((const H5O_fill_t *)fill, &fill_status);
    switch(fill_status) {
        case H5D_FILL_VALUE_UNDEFINED:
            fprintf(stream,"Undefined\n");
            break;

        case H5D_FILL_VALUE_DEFAULT:
            fprintf(stream,"Default\n");
            break;

        case H5D_FILL_VALUE_USER_DEFINED:
            fprintf(stream,"User Defined\n");
            break;

        case H5D_FILL_VALUE_ERROR:
        default:
            fprintf(stream,"Unknown!\n");
            break;

    } /* end switch */
    HDfprintf(stream, "%*s%-*s %Zd\n", indent, "", fwidth,
	      "Size:", fill->size);
    HDfprintf(stream, "%*s%-*s ", indent, "", fwidth, "Data type:");
    if(fill->type) {
	H5T_debug(fill->type, stream);
	fprintf(stream, "\n");
    } /* end if */
    else
	fprintf(stream, "<dataset type>\n");

    FUNC_LEAVE_NOAPI(SUCCEED);
} /* end H5O_fill_debug() */


/*-------------------------------------------------------------------------
 * Function:	H5O_fill_convert
 *
 * Purpose:	Convert a fill value from whatever data type it currently has
 *		to the specified dataset type.  The `type' field of the fill
 *		value struct will be set to NULL to indicate that it has the
 *		same type as the dataset.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, October  1, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_fill_convert(H5O_fill_t *fill, H5T_t *dset_type, hbool_t *fill_changed, hid_t dxpl_id)
{
    H5T_path_t		*tpath;			    /* Type conversion info	*/
    void		*buf = NULL, *bkg = NULL;   /* Conversion buffers	*/
    hid_t		src_id = -1, dst_id = -1;   /* Datatype identifiers	*/
    herr_t      	ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(fill);
    HDassert(dset_type);
    HDassert(fill_changed);

    /* No-op cases */
    if(!fill->buf || !fill->type || 0 == H5T_cmp(fill->type, dset_type, FALSE)) {
        /* Don't need datatype for fill value */
	if(fill->type)
            H5T_close(fill->type);
	fill->type = NULL;

        /* Note that the fill value info has changed */
        *fill_changed = TRUE;

	HGOTO_DONE(SUCCEED);
    } /* end if */

    /*
     * Can we convert between source and destination data types?
     */
    if(NULL == (tpath = H5T_path_find(fill->type, dset_type, NULL, NULL, dxpl_id, FALSE)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to convert between src and dst datatypes")

    /* Don't bother doing anything if there will be no actual conversion */
    if(!H5T_path_noop(tpath)) {
        if((src_id = H5I_register(H5I_DATATYPE, H5T_copy(fill->type, H5T_COPY_ALL), FALSE)) < 0 ||
                (dst_id = H5I_register(H5I_DATATYPE, H5T_copy(dset_type, H5T_COPY_ALL), FALSE)) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to copy/register data type")

        /*
         * Datatype conversions are always done in place, so we need a buffer
         * that is large enough for both source and destination.
         */
        if(H5T_get_size(fill->type) >= H5T_get_size(dset_type))
            buf = fill->buf;
        else {
            if(NULL == (buf = H5MM_malloc(H5T_get_size(dset_type))))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for type conversion")
            HDmemcpy(buf, fill->buf, H5T_get_size(fill->type));
        } /* end else */

        /* Use CALLOC here to clear the buffer in case later the library thinks there's
         * data in the background. */
        if(H5T_path_bkg(tpath) && NULL == (bkg = H5MM_calloc(H5T_get_size(dset_type))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for type conversion")

        /* Do the conversion */
        if(H5T_convert(tpath, src_id, dst_id, (size_t)1, (size_t)0, (size_t)0, buf, bkg, dxpl_id) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "datatype conversion failed")

        /* Update the fill message */
        if(buf != fill->buf) {
            H5T_vlen_reclaim_elmt(fill->buf, fill->type, dxpl_id);
            H5MM_xfree(fill->buf);
            fill->buf = buf;
        } /* end if */
        H5T_close(fill->type);
        fill->type = NULL;
        H5_ASSIGN_OVERFLOW(fill->size, H5T_get_size(dset_type), size_t, ssize_t);

        /* Note that the fill value info has changed */
        *fill_changed = TRUE;
    } /* end if */

done:
    if(src_id >= 0 && H5I_dec_ref(src_id) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTDEC, FAIL, "unable to decrement ref count for temp ID")
    if(dst_id >= 0 && H5I_dec_ref(dst_id) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTDEC, FAIL, "unable to decrement ref count for temp ID")
    if(buf != fill->buf)
        H5MM_xfree(buf);
    if(bkg)
        H5MM_xfree(bkg);

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5O_fill_convert() */


/*-------------------------------------------------------------------------
 * Function:    H5O_fill_set_latest_version
 *
 * Purpose:     Set the encoding for a fill value to the latest version.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, July 24, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_fill_set_latest_version(H5O_fill_t *fill)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(fill);

    /* Set encoding of fill value to latest version */
    fill->version = H5O_FILL_VERSION_LATEST;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_fill_set_latest_version() */

