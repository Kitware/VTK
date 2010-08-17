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

/*
 * Programmer:	Robb Matzke <matzke@llnl.gov>
 *		Wednesday, April 15, 1998
 *
 * Purpose:	Data filter pipeline message.
 */

#define H5O_PACKAGE		/*suppress error about including H5Opkg	  */
#define H5Z_PACKAGE		/*suppress error about including H5Zpkg	  */

#include "H5private.h"		/* Generic Functions			*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free Lists				*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Opkg.h"             /* Object headers			*/
#include "H5Zpkg.h"		/* Data filters				*/


/* PRIVATE PROTOTYPES */
static herr_t H5O_pline_encode(H5F_t *f, uint8_t *p, const void *mesg);
static void *H5O_pline_decode(H5F_t *f, hid_t dxpl_id, H5O_t *open_oh,
    unsigned mesg_flags, unsigned *ioflags, const uint8_t *p);
static void *H5O_pline_copy(const void *_mesg, void *_dest);
static size_t H5O_pline_size(const H5F_t *f, const void *_mesg);
static herr_t H5O_pline_reset(void *_mesg);
static herr_t H5O_pline_free(void *_mesg);
static herr_t H5O_pline_pre_copy_file(H5F_t *file_src,
    const void *mesg_src, hbool_t *deleted, const H5O_copy_t *cpy_info, void *_udata);
static herr_t H5O_pline_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg,
    FILE * stream, int indent, int fwidth);

/* Set up & include shared message "interface" info */
#define H5O_SHARED_TYPE			H5O_MSG_PLINE
#define H5O_SHARED_DECODE		H5O_pline_shared_decode
#define H5O_SHARED_DECODE_REAL		H5O_pline_decode
#define H5O_SHARED_ENCODE		H5O_pline_shared_encode
#define H5O_SHARED_ENCODE_REAL		H5O_pline_encode
#define H5O_SHARED_SIZE			H5O_pline_shared_size
#define H5O_SHARED_SIZE_REAL		H5O_pline_size
#define H5O_SHARED_DELETE		H5O_pline_shared_delete
#undef H5O_SHARED_DELETE_REAL
#define H5O_SHARED_LINK			H5O_pline_shared_link
#undef H5O_SHARED_LINK_REAL
#define H5O_SHARED_COPY_FILE		H5O_pline_shared_copy_file
#undef H5O_SHARED_COPY_FILE_REAL
#define H5O_SHARED_POST_COPY_FILE	H5O_pline_shared_post_copy_file
#undef H5O_SHARED_POST_COPY_FILE_REAL
#define H5O_SHARED_DEBUG		H5O_pline_shared_debug
#define H5O_SHARED_DEBUG_REAL		H5O_pline_debug
#include "H5Oshared.h"			/* Shared Object Header Message Callbacks */

/* This message derives from H5O message class */
const H5O_msg_class_t H5O_MSG_PLINE[1] = {{
    H5O_PLINE_ID,		/* message id number		*/
    "filter pipeline",		/* message name for debugging	*/
    sizeof(H5O_pline_t),	/* native message size		*/
    H5O_SHARE_IS_SHARABLE|H5O_SHARE_IN_OHDR,	/* messages are sharable?       */
    H5O_pline_shared_decode,	/* decode message		*/
    H5O_pline_shared_encode,	/* encode message		*/
    H5O_pline_copy,		/* copy the native value	*/
    H5O_pline_shared_size,	/* size of raw message		*/
    H5O_pline_reset,		/* reset method			*/
    H5O_pline_free,		/* free method			*/
    H5O_pline_shared_delete,    /* file delete method		*/
    H5O_pline_shared_link,	/* link method			*/
    NULL,			/* set share method		*/
    NULL,		    	/*can share method		*/
    H5O_pline_pre_copy_file,	/* pre copy native value to file */
    H5O_pline_shared_copy_file,	/* copy native value to file    */
    NULL,			/* post copy native value to file    */
    NULL,			/* get creation index		*/
    NULL,			/* set creation index		*/
    H5O_pline_shared_debug	/* debug the message		*/
}};


/* Declare a free list to manage the H5O_pline_t struct */
H5FL_DEFINE(H5O_pline_t);


/*-------------------------------------------------------------------------
 * Function:	H5O_pline_decode
 *
 * Purpose:	Decodes a filter pipeline message.
 *
 * Return:	Success:	Ptr to the native message.
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 15, 1998
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_pline_decode(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, H5O_t UNUSED *open_oh,
    unsigned UNUSED mesg_flags, unsigned UNUSED *ioflags, const uint8_t *p)
{
    H5O_pline_t		*pline = NULL;          /* Pipeline message */
    H5Z_filter_info_t   *filter;                /* Filter to decode */
    size_t		name_length;            /* Length of filter name */
    size_t		i;                      /* Local index variable */
    void		*ret_value;             /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_pline_decode)

    /* check args */
    HDassert(p);

    /* Allocate space for I/O pipeline message */
    if(NULL == (pline = H5FL_CALLOC(H5O_pline_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Version */
    pline->version = *p++;
    if(pline->version < H5O_PLINE_VERSION_1 || pline->version > H5O_PLINE_VERSION_LATEST)
	HGOTO_ERROR(H5E_PLINE, H5E_CANTLOAD, NULL, "bad version number for filter pipeline message")

    /* Number of filters */
    pline->nused = *p++;
    if(pline->nused > H5Z_MAX_NFILTERS)
	HGOTO_ERROR(H5E_PLINE, H5E_CANTLOAD, NULL, "filter pipeline message has too many filters")

    /* Reserved */
    if(pline->version == H5O_PLINE_VERSION_1)
        p += 6;

    /* Allocate array for filters */
    pline->nalloc = pline->nused;
    if(NULL == (pline->filter = (H5Z_filter_info_t *)H5MM_calloc(pline->nalloc * sizeof(pline->filter[0]))))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Decode filters */
    for(i = 0, filter = &pline->filter[0]; i < pline->nused; i++, filter++) {
        /* Filter ID */
	UINT16DECODE(p, filter->id);

        /* Length of filter name */
        if(pline->version > H5O_PLINE_VERSION_1 && filter->id < H5Z_FILTER_RESERVED)
            name_length = 0;
        else {
            UINT16DECODE(p, name_length);
            if(pline->version == H5O_PLINE_VERSION_1 && name_length % 8)
                HGOTO_ERROR(H5E_PLINE, H5E_CANTLOAD, NULL, "filter name length is not a multiple of eight")
        } /* end if */

        /* Filter flags */
	UINT16DECODE(p, filter->flags);

        /* Number of filter parameters ("client data elements") */
	UINT16DECODE(p, filter->cd_nelmts);

        /* Filter name, if there is one */
	if(name_length) {
            size_t actual_name_length;          /* Actual length of name */

            /* Determine actual name length (without padding, but with null terminator) */
	    actual_name_length = HDstrlen((const char *)p) + 1;
	    HDassert(actual_name_length <= name_length);

            /* Allocate space for the filter name, or use the internal buffer */
            if(actual_name_length > H5Z_COMMON_NAME_LEN) {
                filter->name = (char *)H5MM_malloc(actual_name_length);
                if(NULL == filter->name)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for filter name")
            } /* end if */
            else
                filter->name = filter->_name;

	    HDstrcpy(filter->name, (const char *)p);
	    p += name_length;
	} /* end if */

        /* Filter parameters */
	if(filter->cd_nelmts) {
            size_t	j;              /* Local index variable */

            /* Allocate space for the client data elements, or use the internal buffer */
            if(filter->cd_nelmts > H5Z_COMMON_CD_VALUES) {
                filter->cd_values = (unsigned *)H5MM_malloc(filter->cd_nelmts * sizeof(unsigned));
                if(NULL == filter->cd_values)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for client data")
            } /* end if */
            else
                filter->cd_values = filter->_cd_values;

	    /*
	     * Read the client data values and the padding
	     */
	    for(j = 0; j < filter->cd_nelmts; j++)
		UINT32DECODE(p, filter->cd_values[j]);
            if(pline->version == H5O_PLINE_VERSION_1)
                if(filter->cd_nelmts % 2)
                    p += 4; /*padding*/
	} /* end if */
    } /* end for */

    /* Set return value */
    ret_value = pline;

done:
    if(NULL == ret_value && pline) {
        H5O_pline_reset(pline);
        H5O_pline_free(pline);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_pline_decode() */


/*-------------------------------------------------------------------------
 * Function:	H5O_pline_encode
 *
 * Purpose:	Encodes message MESG into buffer P.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 15, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_pline_encode(H5F_t UNUSED *f, uint8_t *p/*out*/, const void *mesg)
{
    const H5O_pline_t	*pline = (const H5O_pline_t*)mesg;      /* Pipeline message to encode */
    const       H5Z_filter_info_t *filter;      /* Filter to encode */
    size_t	i, j;                   /* Local index variables */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_pline_encode)

    /* Check args */
    HDassert(p);
    HDassert(mesg);

    /* Message header */
    *p++ = pline->version;
    *p++ = (uint8_t)(pline->nused);
    if(pline->version == H5O_PLINE_VERSION_1) {
        *p++ = 0;	/*reserved 1*/
        *p++ = 0;	/*reserved 2*/
        *p++ = 0;	/*reserved 3*/
        *p++ = 0;	/*reserved 4*/
        *p++ = 0;	/*reserved 5*/
        *p++ = 0;	/*reserved 6*/
    } /* end if */

    /* Encode filters */
    for(i = 0, filter = &pline->filter[0]; i < pline->nused; i++, filter++) {
        const char	*name;                  /* Filter name */
        size_t		name_length;            /* Length of filter name */

        /* Filter ID */
	UINT16ENCODE(p, filter->id);

        /* Skip writing the name length & name if the filter is an internal filter */
        if(pline->version > H5O_PLINE_VERSION_1 && filter->id < H5Z_FILTER_RESERVED) {
            name_length = 0;
            name = NULL;
        } /* end if */
        else {
            H5Z_class2_t	*cls;                   /* Filter class */

            /*
             * Get the filter name.  If the pipeline message has a name in it then
             * use that one.  Otherwise try to look up the filter and get the name
             * as it was registered.
             */
            if(NULL == (name = filter->name) && (cls = H5Z_find(filter->id)))
                name = cls->name;
            name_length = name ? HDstrlen(name) + 1 : 0;

            /* Filter name length */
            UINT16ENCODE(p, pline->version == H5O_PLINE_VERSION_1 ? H5O_ALIGN_OLD(name_length) : name_length);
        } /* end else */

        /* Filter flags */
	UINT16ENCODE(p, filter->flags);

        /* # of filter parameters */
	UINT16ENCODE(p, filter->cd_nelmts);

        /* Encode name, if there is one to encode */
	if(name_length > 0) {
            /* Store name, with null terminator */
	    HDmemcpy(p, name, name_length);
	    p += name_length;

            /* Pad out name to alignment, in older versions */
            if(pline->version == H5O_PLINE_VERSION_1)
                while(name_length++ % 8)
                    *p++ = 0;
	} /* end if */

        /* Filter parameters */
	for(j = 0; j < filter->cd_nelmts; j++)
	    UINT32ENCODE(p, filter->cd_values[j]);

        /* Align the parameters for older versions of the format */
        if(pline->version == H5O_PLINE_VERSION_1)
            if(filter->cd_nelmts % 2)
                UINT32ENCODE(p, 0);
    } /* end for */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_pline_encode() */


/*-------------------------------------------------------------------------
 * Function:	H5O_pline_copy
 *
 * Purpose:	Copies a filter pipeline message from SRC to DST allocating
 *		DST if necessary.  If DST is already allocated then we assume
 *		that it isn't initialized.
 *
 * Return:	Success:	Ptr to DST or allocated result.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 15, 1998
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_pline_copy(const void *_src, void *_dst/*out*/)
{
    const H5O_pline_t	*src = (const H5O_pline_t *)_src;       /* Source pipeline message */
    H5O_pline_t		*dst = (H5O_pline_t *)_dst;             /* Destination pipeline message */
    size_t		i;                      /* Local index variable */
    H5O_pline_t		*ret_value;             /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_pline_copy)

    /* Allocate pipeline message, if not provided */
    if(!dst && NULL == (dst = H5FL_MALLOC(H5O_pline_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Shallow copy basic fields */
    *dst = *src;

    /* Copy over filters, if any */
    dst->nalloc = dst->nused;
    if(dst->nalloc) {
        /* Allocate array to hold filters */
	if(NULL == (dst->filter = (H5Z_filter_info_t *)H5MM_calloc(dst->nalloc * sizeof(dst->filter[0]))))
	    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

        /* Deep-copy filters */
        for(i = 0; i < src->nused; i++) {
            /* Basic filter information */
            dst->filter[i] = src->filter[i];

            /* Filter name */
            if(src->filter[i].name) {
                size_t namelen;         /* Length of source filter name, including null terminator  */

                namelen = HDstrlen(src->filter[i].name) + 1;

                /* Allocate space for the filter name, or use the internal buffer */
                if(namelen > H5Z_COMMON_NAME_LEN) {
                    dst->filter[i].name = (char *)H5MM_malloc(namelen);
                    if(NULL == dst->filter[i].name)
                        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for filter name")

                    /* Copy name */
                    HDstrcpy(dst->filter[i].name, src->filter[i].name);
                } /* end if */
                else
                    dst->filter[i].name = dst->filter[i]._name;
            } /* end if */

            /* Filter parameters */
            if(src->filter[i].cd_nelmts > 0) {
                /* Allocate space for the client data elements, or use the internal buffer */
                if(src->filter[i].cd_nelmts > H5Z_COMMON_CD_VALUES) {
                    if(NULL == (dst->filter[i].cd_values = (unsigned *)H5MM_malloc(src->filter[i].cd_nelmts* sizeof(unsigned))))
                        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

                    HDmemcpy(dst->filter[i].cd_values, src->filter[i].cd_values,
                            src->filter[i].cd_nelmts * sizeof(unsigned));
                } /* end if */
                else
                    dst->filter[i].cd_values = dst->filter[i]._cd_values;
            } /* end if */
        } /* end for */
    } /* end if */
    else
	dst->filter = NULL;

    /* Set return value */
    ret_value = dst;

done:
    if(!ret_value && dst) {
        H5O_pline_reset(dst);
	if(!_dst)
            H5O_pline_free(dst);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_pline_copy() */


/*-------------------------------------------------------------------------
 * Function:	H5O_pline_size
 *
 * Purpose:	Determines the size of a raw filter pipeline message.
 *
 * Return:	Success:	Size of message.
 *
 *		Failure:	zero
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 15, 1998
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O_pline_size(const H5F_t UNUSED *f, const void *mesg)
{
    const H5O_pline_t	*pline = (const H5O_pline_t*)mesg;      /* Pipeline message */
    size_t i;                   /* Local index variable */
    size_t ret_value;           /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_pline_size)

    /* Message header */
    ret_value = 1 +			/*version			*/
	   1 +				/*number of filters		*/
	   (pline->version == H5O_PLINE_VERSION_1 ? 6 : 0);	/*reserved			*/

    /* Calculate size of each filter in pipeline */
    for(i = 0; i < pline->nused; i++) {
        size_t	name_len;               /* Length of filter name */
        const char *name;               /* Filter name */

        /* Don't write the name length & name if the filter is an internal filter */
        if(pline->version > H5O_PLINE_VERSION_1 && pline->filter[i].id < H5Z_FILTER_RESERVED)
            name_len = 0;
        else {
            H5Z_class2_t	*cls;                   /* Filter class */

            /* Get the name of the filter, same as done with H5O_pline_encode() */
            if(NULL == (name = pline->filter[i].name) && (cls = H5Z_find(pline->filter[i].id)))
                name = cls->name;
            name_len = name ? HDstrlen(name) + 1 : 0;
        } /* end else */

	ret_value += 2 +			/*filter identification number	*/
		((pline->version == H5O_PLINE_VERSION_1 || pline->filter[i].id >= H5Z_FILTER_RESERVED) ? 2 : 0) +				/*name length			*/
		2 +				/*flags				*/
		2 +				/*number of client data values	*/
		(pline->version == H5O_PLINE_VERSION_1 ? H5O_ALIGN_OLD(name_len) : name_len);	/*length of the filter name	*/

	ret_value += pline->filter[i].cd_nelmts * 4;
        if(pline->version == H5O_PLINE_VERSION_1)
            if(pline->filter[i].cd_nelmts % 2)
                ret_value += 4;
    } /* end for */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_pline_size() */


/*-------------------------------------------------------------------------
 * Function:	H5O_pline_reset
 *
 * Purpose:	Resets a filter pipeline message by clearing all filters.
 *		The MESG buffer is not freed.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 15, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_pline_reset(void *mesg)
{
    H5O_pline_t	*pline = (H5O_pline_t*)mesg;    /* Pipeline message */
    size_t	i;                              /* Local index variable */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_pline_reset)

    HDassert(pline);

    /* Free information for each filter */
    for(i = 0; i < pline->nused; i++) {
        if(pline->filter[i].name && pline->filter[i].name != pline->filter[i]._name)
            HDassert((HDstrlen(pline->filter[i].name) + 1) > H5Z_COMMON_NAME_LEN);
        if(pline->filter[i].name != pline->filter[i]._name)
            pline->filter[i].name = (char *)H5MM_xfree(pline->filter[i].name);
        if(pline->filter[i].cd_values && pline->filter[i].cd_values != pline->filter[i]._cd_values)
            HDassert(pline->filter[i].cd_nelmts > H5Z_COMMON_CD_VALUES);
        if(pline->filter[i].cd_values != pline->filter[i]._cd_values)
            pline->filter[i].cd_values = (unsigned *)H5MM_xfree(pline->filter[i].cd_values);
    } /* end for */

    /* Free filter array */
    if(pline->filter)
        pline->filter = (H5Z_filter_info_t *)H5MM_xfree(pline->filter);

    /* Reset # of filters */
    pline->nused = pline->nalloc = 0;

    /* Reset version # of pipeline message */
    pline->version = H5O_PLINE_VERSION_1;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_pline_reset() */


/*-------------------------------------------------------------------------
 * Function:	H5O_pline_free
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
H5O_pline_free(void *mesg)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_pline_free)

    HDassert(mesg);

    mesg = H5FL_FREE(H5O_pline_t, mesg);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_pline_free() */


/*-------------------------------------------------------------------------
 * Function:    H5O_pline_pre_copy_file
 *
 * Purpose:     Perform any necessary actions before copying message between
 *              files
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Peter Cao
 *              December 27, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_pline_pre_copy_file(H5F_t UNUSED *file_src, const void *mesg_src,
    hbool_t UNUSED *deleted, const H5O_copy_t UNUSED *cpy_info, void *_udata)
{
    const H5O_pline_t *pline_src = (const H5O_pline_t *)mesg_src;    /* Source datatype */
    H5O_copy_file_ud_common_t *udata = (H5O_copy_file_ud_common_t *)_udata; /* Object copying user data */
    herr_t             ret_value = SUCCEED;                     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5O_pline_pre_copy_file)

    /* check args */
    HDassert(pline_src);

    /* If the user data is non-NULL, assume we are copying a dataset or group
     * and make a copy of the filter pipeline for later in
     * the object copying process.
     */
    if(udata)
        if(NULL == (udata->src_pline = (H5O_pline_t *)H5O_pline_copy(pline_src, NULL)))
            HGOTO_ERROR(H5E_PLINE, H5E_CANTINIT, FAIL, "unable to copy")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_pline_pre_copy_file() */


/*-------------------------------------------------------------------------
 * Function:	H5O_pline_debug
 *
 * Purpose:	Prints debugging information for filter pipeline message MESG
 *		on output stream STREAM.  Each line is indented INDENT
 *		characters and the field name takes up FWIDTH characters.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 15, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_pline_debug(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, const void *mesg, FILE *stream,
		int indent, int fwidth)
{
    const H5O_pline_t	*pline = (const H5O_pline_t *)mesg;
    size_t		i, j;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5O_pline_debug)

    /* check args */
    HDassert(f);
    HDassert(pline);
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %Zu/%Zu\n", indent, "", fwidth,
	    "Number of filters:",
	    pline->nused,
	    pline->nalloc);

    /* Loop over all the filters */
    for(i = 0; i < pline->nused; i++) {
	char		name[32];

	sprintf(name, "Filter at position %u", (unsigned)i);
	HDfprintf(stream, "%*s%-*s\n", indent, "", fwidth, name);
	HDfprintf(stream, "%*s%-*s 0x%04x\n", indent + 3, "", MAX(0, fwidth - 3),
		"Filter identification:",
		(unsigned)(pline->filter[i].id));
	if(pline->filter[i].name)
	    HDfprintf(stream, "%*s%-*s \"%s\"\n", indent + 3, "", MAX(0, fwidth - 3),
		    "Filter name:",
		    pline->filter[i].name);
	else
	    HDfprintf(stream, "%*s%-*s NONE\n", indent + 3, "", MAX(0, fwidth - 3),
		    "Filter name:");
	HDfprintf(stream, "%*s%-*s 0x%04x\n", indent + 3, "", MAX(0, fwidth - 3),
		"Flags:",
		pline->filter[i].flags);
	HDfprintf(stream, "%*s%-*s %Zu\n", indent + 3, "", MAX(0, fwidth - 3),
		"Num CD values:",
		pline->filter[i].cd_nelmts);

        /* Filter parameters */
	for(j = 0; j < pline->filter[i].cd_nelmts; j++) {
	    char	field_name[32];

	    sprintf(field_name, "CD value %lu", (unsigned long)j);
	    HDfprintf(stream, "%*s%-*s %u\n", indent + 6, "", MAX(0, fwidth - 6),
		    field_name,
		    pline->filter[i].cd_values[j]);
	} /* end for */
    } /* end for */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_pline_debug() */


/*-------------------------------------------------------------------------
 * Function:    H5O_pline_set_latest_version
 *
 * Purpose:     Set the encoding for a I/O filter pipeline to the latest version.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, July 24, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_pline_set_latest_version(H5O_pline_t *pline)
{
    FUNC_ENTER_NOAPI_NOFUNC(H5O_pline_set_latest_version)

    /* Sanity check */
    HDassert(pline);

    /* Set encoding of I/O pipeline to latest version */
    pline->version = H5O_PLINE_VERSION_LATEST;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_pline_set_latest_version() */

