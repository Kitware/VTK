/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:             H5Oginfo.c
 *                      Aug 23 2005
 *                      Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:             Group Information messages.
 *
 *-------------------------------------------------------------------------
 */

#include "H5Omodule.h"          /* This source code file is part of the H5O module */


#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free lists                           */
#include "H5Opkg.h"             /* Object headers			*/


/* PRIVATE PROTOTYPES */
static void *H5O_ginfo_decode(H5F_t *f, H5O_t *open_oh, unsigned mesg_flags,
    unsigned *ioflags, size_t p_size, const uint8_t *p);
static herr_t H5O_ginfo_encode(H5F_t *f, hbool_t disable_shared, uint8_t *p, const void *_mesg);
static void *H5O_ginfo_copy(const void *_mesg, void *_dest);
static size_t H5O_ginfo_size(const H5F_t *f, hbool_t disable_shared, const void *_mesg);
static herr_t H5O__ginfo_free(void *_mesg);
static herr_t H5O__ginfo_debug(H5F_t *f, const void *_mesg, FILE * stream,
    int indent, int fwidth);

/* This message derives from H5O message class */
const H5O_msg_class_t H5O_MSG_GINFO[1] = {{
    H5O_GINFO_ID,            	/*message id number             */
    "ginfo",                 	/*message name for debugging    */
    sizeof(H5O_ginfo_t),     	/*native message size           */
    0,				/* messages are sharable?       */
    H5O_ginfo_decode,        	/*decode message                */
    H5O_ginfo_encode,        	/*encode message                */
    H5O_ginfo_copy,          	/*copy the native value         */
    H5O_ginfo_size,          	/*size of symbol table entry    */
    NULL,                   	/*default reset method          */
    H5O__ginfo_free,	        /* free method			*/
    NULL,	        	/* file delete method		*/
    NULL,			/* link method			*/
    NULL, 			/*set share method		*/
    NULL,		    	/*can share method		*/
    NULL,			/* pre copy native value to file */
    NULL,			/* copy native value to file    */
    NULL,			/* post copy native value to file    */
    NULL,			/* get creation index		*/
    NULL,			/* set creation index		*/
    H5O__ginfo_debug          	/*debug the message             */
}};

/* Current version of group info information */
#define H5O_GINFO_VERSION 	0

/* Flags for group info flag encoding */
#define H5O_GINFO_STORE_PHASE_CHANGE    0x01
#define H5O_GINFO_STORE_EST_ENTRY_INFO  0x02
#define H5O_GINFO_ALL_FLAGS             (H5O_GINFO_STORE_PHASE_CHANGE | H5O_GINFO_STORE_EST_ENTRY_INFO)

/* Declare a free list to manage the H5O_ginfo_t struct */
H5FL_DEFINE_STATIC(H5O_ginfo_t);


/*-------------------------------------------------------------------------
 * Function:    H5O_ginfo_decode
 *
 * Purpose:     Decode a message and return a pointer to
 *              a newly allocated one.
 *
 * Return:      Success:        Ptr to new message in native order.
 *
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              Aug 30 2005
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_ginfo_decode(H5F_t H5_ATTR_UNUSED *f, H5O_t H5_ATTR_UNUSED *open_oh,
    unsigned H5_ATTR_UNUSED mesg_flags, unsigned H5_ATTR_UNUSED *ioflags,
    size_t H5_ATTR_UNUSED p_size, const uint8_t *p)
{
    H5O_ginfo_t         *ginfo = NULL;  /* Pointer to group information message */
    unsigned char       flags;          /* Flags for encoding group info */
    void                *ret_value = NULL;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(p);

    /* Version of message */
    if(*p++ != H5O_GINFO_VERSION)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "bad version number for message")

    /* Allocate space for message */
    if(NULL == (ginfo = H5FL_CALLOC(H5O_ginfo_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Get the flags for the group */
    flags = *p++;
    if(flags & ~H5O_GINFO_ALL_FLAGS)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "bad flag value for message")
    ginfo->store_link_phase_change = (flags & H5O_GINFO_STORE_PHASE_CHANGE) ? TRUE : FALSE;
    ginfo->store_est_entry_info = (flags & H5O_GINFO_STORE_EST_ENTRY_INFO) ? TRUE : FALSE;

    /* Get the max. # of links to store compactly & the min. # of links to store densely */
    if(ginfo->store_link_phase_change) {
        UINT16DECODE(p, ginfo->max_compact)
        UINT16DECODE(p, ginfo->min_dense)
    } /* end if */
    else {
        ginfo->max_compact = H5G_CRT_GINFO_MAX_COMPACT;
        ginfo->min_dense = H5G_CRT_GINFO_MIN_DENSE;
    } /* end else */

    /* Get the estimated # of entries & name lengths */
    if(ginfo->store_est_entry_info) {
        UINT16DECODE(p, ginfo->est_num_entries)
        UINT16DECODE(p, ginfo->est_name_len)
    } /* end if */
    else {
        ginfo->est_num_entries = H5G_CRT_GINFO_EST_NUM_ENTRIES;
        ginfo->est_name_len = H5G_CRT_GINFO_EST_NAME_LEN;
    } /* end if */

    /* Set return value */
    ret_value = ginfo;

done:
    if(ret_value == NULL)
        if(ginfo != NULL)
            ginfo = H5FL_FREE(H5O_ginfo_t, ginfo);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_ginfo_decode() */


/*-------------------------------------------------------------------------
 * Function:    H5O_ginfo_encode
 *
 * Purpose:     Encodes a message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              Aug 30 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_ginfo_encode(H5F_t H5_ATTR_UNUSED *f, hbool_t H5_ATTR_UNUSED disable_shared, uint8_t *p, const void *_mesg)
{
    const H5O_ginfo_t  *ginfo = (const H5O_ginfo_t *) _mesg;
    unsigned char       flags;          /* Flags for encoding group info */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(p);
    HDassert(ginfo);

    /* Message version */
    *p++ = H5O_GINFO_VERSION;

    /* The flags for the group info */
    flags = ginfo->store_link_phase_change ?  H5O_GINFO_STORE_PHASE_CHANGE : 0;
    flags = (unsigned char)(flags | (ginfo->store_est_entry_info ?  H5O_GINFO_STORE_EST_ENTRY_INFO : 0));
    *p++ = flags;

    /* Store the max. # of links to store compactly & the min. # of links to store densely */
    if(ginfo->store_link_phase_change) {
        UINT16ENCODE(p, ginfo->max_compact)
        UINT16ENCODE(p, ginfo->min_dense)
    } /* end if */

    /* Estimated # of entries & name lengths */
    if(ginfo->store_est_entry_info) {
        UINT16ENCODE(p, ginfo->est_num_entries)
        UINT16ENCODE(p, ginfo->est_name_len)
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_ginfo_encode() */


/*-------------------------------------------------------------------------
 * Function:    H5O_ginfo_copy
 *
 * Purpose:     Copies a message from _MESG to _DEST, allocating _DEST if
 *              necessary.
 *
 * Return:      Success:        Ptr to _DEST
 *
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              Aug 30 2005
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_ginfo_copy(const void *_mesg, void *_dest)
{
    const H5O_ginfo_t   *ginfo = (const H5O_ginfo_t *)_mesg;
    H5O_ginfo_t         *dest = (H5O_ginfo_t *)_dest;
    void                *ret_value = NULL;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(ginfo);
    if(!dest && NULL == (dest = H5FL_MALLOC(H5O_ginfo_t)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* copy */
    *dest = *ginfo;

    /* Set return value */
    ret_value = dest;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_ginfo_copy() */


/*-------------------------------------------------------------------------
 * Function:    H5O_ginfo_size
 *
 * Purpose:     Returns the size of the raw message in bytes not counting
 *              the message type or size fields, but only the data fields.
 *              This function doesn't take into account alignment.
 *
 * Return:      Success:        Message data size in bytes without alignment.
 *
 *              Failure:        zero
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              Aug 30 2005
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O_ginfo_size(const H5F_t H5_ATTR_UNUSED *f, hbool_t H5_ATTR_UNUSED disable_shared, const void *_mesg)
{
    const H5O_ginfo_t   *ginfo = (const H5O_ginfo_t *)_mesg;
    size_t ret_value = 0;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Set return value */
    ret_value = 1 +                     /* Version */
                1 +                     /* Flags */
                (ginfo->store_link_phase_change ? (
            (size_t)(2 +                 /* "Max compact" links */
                    2)                   /* "Min dense" links */
                ) : 0) +                /* "Min dense" links */
                (ginfo->store_est_entry_info ? (
            (size_t)(2 +                 /* Estimated # of entries in group */
                    2)                   /* Estimated length of name of entry in group */
                ) : 0);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_ginfo_size() */


/*-------------------------------------------------------------------------
 * Function:	H5O__ginfo_free
 *
 * Purpose:	Frees the message
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, August 30, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__ginfo_free(void *mesg)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(mesg);

    mesg = H5FL_FREE(H5O_ginfo_t, mesg);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O__ginfo_free() */


/*-------------------------------------------------------------------------
 * Function:    H5O__ginfo_debug
 *
 * Purpose:     Prints debugging info for a message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              Aug 30 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__ginfo_debug(H5F_t H5_ATTR_UNUSED *f, const void *_mesg, FILE *stream,
    int indent, int fwidth)
{
    const H5O_ginfo_t       *ginfo = (const H5O_ginfo_t *) _mesg;

    FUNC_ENTER_STATIC_NOERR

    /* check args */
    HDassert(f);
    HDassert(ginfo);
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Max. compact links:", ginfo->max_compact);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Min. dense links:", ginfo->min_dense);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Estimated # of objects in group:", ginfo->est_num_entries);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Estimated length of object in group's name:", ginfo->est_name_len);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O__ginfo_debug() */

