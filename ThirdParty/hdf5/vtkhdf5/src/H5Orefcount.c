/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:             H5Orefcount.c
 *                      Mar 10 2007
 *                      Quincey Koziol
 *
 * Purpose:             Object ref. count messages.
 *
 *-------------------------------------------------------------------------
 */

#include "H5Omodule.h" /* This source code file is part of the H5O module */

#include "H5private.h"   /* Generic Functions			*/
#include "H5Eprivate.h"  /* Error handling		  	*/
#include "H5FLprivate.h" /* Free lists                           */
#include "H5Opkg.h"      /* Object headers			*/

/* PRIVATE PROTOTYPES */
static void * H5O__refcount_decode(H5F_t *f, H5O_t *open_oh, unsigned mesg_flags, unsigned *ioflags,
                                   size_t p_size, const uint8_t *p);
static herr_t H5O__refcount_encode(H5F_t *f, hbool_t disable_shared, uint8_t *p, const void *_mesg);
static void * H5O__refcount_copy(const void *_mesg, void *_dest);
static size_t H5O__refcount_size(const H5F_t *f, hbool_t disable_shared, const void *_mesg);
static herr_t H5O__refcount_free(void *_mesg);
static herr_t H5O__refcount_pre_copy_file(H5F_t *file_src, const void *mesg_src, hbool_t *deleted,
                                          const H5O_copy_t *cpy_info, void *udata);
static herr_t H5O__refcount_debug(H5F_t *f, const void *_mesg, FILE *stream, int indent, int fwidth);

/* This message derives from H5O message class */
const H5O_msg_class_t H5O_MSG_REFCOUNT[1] = {{
    H5O_REFCOUNT_ID,             /*message id number             */
    "refcount",                  /*message name for debugging    */
    sizeof(H5O_refcount_t),      /*native message size           */
    0,                           /* messages are sharable?       */
    H5O__refcount_decode,        /*decode message                */
    H5O__refcount_encode,        /*encode message                */
    H5O__refcount_copy,          /*copy the native value         */
    H5O__refcount_size,          /*size of symbol table entry    */
    NULL,                        /*default reset method          */
    H5O__refcount_free,          /* free method			*/
    NULL,                        /* file delete method		*/
    NULL,                        /* link method			*/
    NULL,                        /*set share method		*/
    NULL,                        /*can share method		*/
    H5O__refcount_pre_copy_file, /* pre copy native value to file */
    NULL,                        /* copy native value to file    */
    NULL,                        /* post copy native value to file */
    NULL,                        /* get creation index		*/
    NULL,                        /* set creation index		*/
    H5O__refcount_debug          /*debug the message             */
}};

/* Current version of ref. count information */
#define H5O_REFCOUNT_VERSION 0

/* Declare a free list to manage the H5O_refcount_t struct */
H5FL_DEFINE_STATIC(H5O_refcount_t);

/*-------------------------------------------------------------------------
 * Function:    H5O__refcount_decode
 *
 * Purpose:     Decode a message and return a pointer to a newly allocated one.
 *
 * Return:      Success:        Ptr to new message in native form.
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              Mar 10 2007
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O__refcount_decode(H5F_t H5_ATTR_UNUSED *f, H5O_t H5_ATTR_UNUSED *open_oh,
                     unsigned H5_ATTR_UNUSED mesg_flags, unsigned H5_ATTR_UNUSED *ioflags,
                     size_t H5_ATTR_UNUSED p_size, const uint8_t *p)
{
    H5O_refcount_t *refcount  = NULL; /* Reference count */
    void *          ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* check args */
    HDassert(f);
    HDassert(p);

    /* Version of message */
    if (*p++ != H5O_REFCOUNT_VERSION)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "bad version number for message")

    /* Allocate space for message */
    if (NULL == (refcount = H5FL_MALLOC(H5O_refcount_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Get ref. count for object */
    UINT32DECODE(p, *refcount)

    /* Set return value */
    ret_value = refcount;

done:
    if (ret_value == NULL && refcount != NULL)
        refcount = H5FL_FREE(H5O_refcount_t, refcount);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__refcount_decode() */

/*-------------------------------------------------------------------------
 * Function:    H5O__refcount_encode
 *
 * Purpose:     Encodes a message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Mar 10 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__refcount_encode(H5F_t H5_ATTR_UNUSED *f, hbool_t H5_ATTR_UNUSED disable_shared, uint8_t *p,
                     const void *_mesg)
{
    const H5O_refcount_t *refcount = (const H5O_refcount_t *)_mesg;

    FUNC_ENTER_STATIC_NOERR

    /* check args */
    HDassert(f);
    HDassert(p);
    HDassert(refcount);

    /* Message version */
    *p++ = H5O_REFCOUNT_VERSION;

    /* Object's ref. count */
    UINT32ENCODE(p, *refcount);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O__refcount_encode() */

/*-------------------------------------------------------------------------
 * Function:    H5O__refcount_copy
 *
 * Purpose:     Copies a message from _MESG to _DEST, allocating _DEST if
 *              necessary.
 *
 * Return:      Success:        Ptr to _DEST
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              Mar 10 2007
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O__refcount_copy(const void *_mesg, void *_dest)
{
    const H5O_refcount_t *refcount  = (const H5O_refcount_t *)_mesg;
    H5O_refcount_t *      dest      = (H5O_refcount_t *)_dest;
    void *                ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* check args */
    HDassert(refcount);
    if (!dest && NULL == (dest = H5FL_MALLOC(H5O_refcount_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* copy */
    *dest = *refcount;

    /* Set return value */
    ret_value = dest;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__refcount_copy() */

/*-------------------------------------------------------------------------
 * Function:    H5O__refcount_size
 *
 * Purpose:     Returns the size of the raw message in bytes not counting
 *              the message type or size fields, but only the data fields.
 *              This function doesn't take into account alignment.
 *
 * Return:      Success:        Message data size in bytes without alignment.
 *              Failure:        zero
 *
 * Programmer:  Quincey Koziol
 *              Mar 10 2007
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O__refcount_size(const H5F_t H5_ATTR_UNUSED *f, hbool_t H5_ATTR_UNUSED disable_shared,
                   const void H5_ATTR_UNUSED *_mesg)
{
    size_t ret_value = 0; /* Return value */

    FUNC_ENTER_STATIC_NOERR

    /* Set return value */
    ret_value = 1    /* Version */
                + 4; /* Ref. count */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__refcount_size() */

/*-------------------------------------------------------------------------
 * Function:	H5O__refcount_free
 *
 * Purpose:	Frees the message
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, March 10, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__refcount_free(void *mesg)
{
    FUNC_ENTER_STATIC_NOERR

    HDassert(mesg);

    mesg = H5FL_FREE(H5O_refcount_t, mesg);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O__refcount_free() */

/*-------------------------------------------------------------------------
 * Function:    H5O__refcount_pre_copy_file
 *
 * Purpose:     Perform any necessary actions before copying message between
 *              files.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Saturday, March 10, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__refcount_pre_copy_file(H5F_t H5_ATTR_UNUSED *file_src, const void H5_ATTR_UNUSED *native_src,
                            hbool_t *deleted, const H5O_copy_t H5_ATTR_UNUSED *cpy_info,
                            void H5_ATTR_UNUSED *udata)
{
    FUNC_ENTER_STATIC_NOERR

    /* check args */
    HDassert(deleted);
    HDassert(cpy_info);

    /* Always delete this message when copying objects between files.  Let
     *  the copy routine set the correct ref. count.
     */
    *deleted = TRUE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O__refcount_pre_copy_file() */

/*-------------------------------------------------------------------------
 * Function:    H5O__refcount_debug
 *
 * Purpose:     Prints debugging info for a message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Mar  6 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__refcount_debug(H5F_t H5_ATTR_UNUSED *f, const void *_mesg, FILE *stream, int indent, int fwidth)
{
    const H5O_refcount_t *refcount = (const H5O_refcount_t *)_mesg;

    FUNC_ENTER_STATIC_NOERR

    /* check args */
    HDassert(f);
    HDassert(refcount);
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth, "Number of links:", (unsigned)*refcount);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O__refcount_debug() */
