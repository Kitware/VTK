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

/* Programmer:  Quincey Koziol
 *              Thursday, March  1, 2007
 *
 * Purpose:	A message holding non-default v1 B-tree 'K' value
 *              information in the superblock extension.
 */

#include "H5Omodule.h" /* This source code file is part of the H5O module */

#include "H5private.h"   /* Generic Functions			*/
#include "H5Eprivate.h"  /* Error handling		  	*/
#include "H5Opkg.h"      /* Object headers			*/
#include "H5MMprivate.h" /* Memory management			*/

static void * H5O__btreek_decode(H5F_t *f, H5O_t *open_oh, unsigned mesg_flags, unsigned *ioflags,
                                 size_t p_size, const uint8_t *p);
static herr_t H5O__btreek_encode(H5F_t *f, hbool_t disable_shared, uint8_t *p, const void *_mesg);
static void * H5O__btreek_copy(const void *_mesg, void *_dest);
static size_t H5O__btreek_size(const H5F_t *f, hbool_t disable_shared, const void *_mesg);
static herr_t H5O__btreek_debug(H5F_t *f, const void *_mesg, FILE *stream, int indent, int fwidth);

/* This message derives from H5O message class */
const H5O_msg_class_t H5O_MSG_BTREEK[1] = {{
    H5O_BTREEK_ID,          /*message id number                     */
    "v1 B-tree 'K' values", /*message name for debugging            */
    sizeof(H5O_btreek_t),   /*native message size                   */
    0,                      /* messages are sharable?               */
    H5O__btreek_decode,     /*decode message                        */
    H5O__btreek_encode,     /*encode message                        */
    H5O__btreek_copy,       /*copy the native value                 */
    H5O__btreek_size,       /*raw message size			*/
    NULL,                   /*free internal memory			*/
    NULL,                   /* free method				*/
    NULL,                   /* file delete method			*/
    NULL,                   /* link method				*/
    NULL,                   /*set share method		        */
    NULL,                   /*can share method		        */
    NULL,                   /* pre copy native value to file	*/
    NULL,                   /* copy native value to file		*/
    NULL,                   /* post copy native value to file	*/
    NULL,                   /* get creation index		        */
    NULL,                   /* set creation index		        */
    H5O__btreek_debug       /*debug the message			*/
}};

/* Current version of v1 B-tree 'K' value information */
#define H5O_BTREEK_VERSION 0

/*-------------------------------------------------------------------------
 * Function:	H5O__btreek_decode
 *
 * Purpose:	Decode a shared message table message and return a pointer
 *              to a newly allocated H5O_btreek_t struct.
 *
 * Return:	Success:	Ptr to new message in native struct.
 *		Failure:	NULL
 *
 * Programmer:  Quincey Koziol
 *              Mar  1, 2007
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O__btreek_decode(H5F_t H5_ATTR_UNUSED *f, H5O_t H5_ATTR_UNUSED *open_oh, unsigned H5_ATTR_UNUSED mesg_flags,
                   unsigned H5_ATTR_UNUSED *ioflags, size_t H5_ATTR_UNUSED p_size, const uint8_t *p)
{
    H5O_btreek_t *mesg;             /* Native message */
    void *        ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(f);
    HDassert(p);

    /* Version of message */
    if (*p++ != H5O_BTREEK_VERSION)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "bad version number for message")

    /* Allocate space for message */
    if (NULL == (mesg = (H5O_btreek_t *)H5MM_calloc(sizeof(H5O_btreek_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for v1 B-tree 'K' message")

    /* Retrieve non-default B-tree 'K' values */
    UINT16DECODE(p, mesg->btree_k[H5B_CHUNK_ID]);
    UINT16DECODE(p, mesg->btree_k[H5B_SNODE_ID]);
    UINT16DECODE(p, mesg->sym_leaf_k);

    /* Set return value */
    ret_value = (void *)mesg;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__btreek_decode() */

/*-------------------------------------------------------------------------
 * Function:	H5O__btreek_encode
 *
 * Purpose:	Encode a v1 B-tree 'K' value message.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Mar  1, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__btreek_encode(H5F_t H5_ATTR_UNUSED *f, hbool_t H5_ATTR_UNUSED disable_shared, uint8_t *p,
                   const void *_mesg)
{
    const H5O_btreek_t *mesg = (const H5O_btreek_t *)_mesg;

    FUNC_ENTER_STATIC_NOERR

    /* Sanity check */
    HDassert(f);
    HDassert(p);
    HDassert(mesg);

    /* Store version and non-default v1 B-tree 'K' values */
    *p++ = H5O_BTREEK_VERSION;
    UINT16ENCODE(p, mesg->btree_k[H5B_CHUNK_ID]);
    UINT16ENCODE(p, mesg->btree_k[H5B_SNODE_ID]);
    UINT16ENCODE(p, mesg->sym_leaf_k);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O__btreek_encode() */

/*-------------------------------------------------------------------------
 * Function:	H5O__btreek_copy
 *
 * Purpose:	Copies a message from _MESG to _DEST, allocating _DEST if
 *		necessary.
 *
 * Return:	Success:	Ptr to _DEST
 *		Failure:	NULL
 *
 * Programmer:  Quincey Koziol
 *              Mar  1, 2007
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O__btreek_copy(const void *_mesg, void *_dest)
{
    const H5O_btreek_t *mesg      = (const H5O_btreek_t *)_mesg;
    H5O_btreek_t *      dest      = (H5O_btreek_t *)_dest;
    void *              ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(mesg);

    if (!dest && NULL == (dest = (H5O_btreek_t *)H5MM_malloc(sizeof(H5O_btreek_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                    "memory allocation failed for shared message table message")

    /* All this message requires is a shallow copy */
    *dest = *mesg;

    /* Set return value */
    ret_value = dest;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__btreek_copy() */

/*-------------------------------------------------------------------------
 * Function:	H5O__btreek_size
 *
 * Purpose:	Returns the size of the raw message in bytes not counting the
 *		message type or size fields, but only the data fields.
 *
 * Return:	Success:	Message data size in bytes w/o alignment.
 *		Failure:	0
 *
 * Programmer:  Quincey Koziol
 *              Mar  1, 2007
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O__btreek_size(const H5F_t H5_ATTR_UNUSED *f, hbool_t H5_ATTR_UNUSED disable_shared,
                 const void H5_ATTR_UNUSED *_mesg)
{
    size_t ret_value = 0;

    FUNC_ENTER_STATIC_NOERR

    /* Sanity check */
    HDassert(f);

    ret_value = 1 + /* Version number */
                2 + /* Chunked storage internal B-tree 'K' value */
                2 + /* Symbol table node internal B-tree 'K' value */
                2;  /* Symbol table node leaf 'K' value */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__btreek_size() */

/*-------------------------------------------------------------------------
 * Function:	H5O__btreek_debug
 *
 * Purpose:	Prints debugging info for the message.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Mar  1, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__btreek_debug(H5F_t H5_ATTR_UNUSED *f, const void *_mesg, FILE *stream, int indent, int fwidth)
{
    const H5O_btreek_t *mesg = (const H5O_btreek_t *)_mesg;

    FUNC_ENTER_STATIC_NOERR

    /* Sanity check */
    HDassert(f);
    HDassert(mesg);
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
              "Chunked storage internal B-tree 'K' value:", mesg->btree_k[H5B_CHUNK_ID]);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
              "Symbol table node internal B-tree 'K' value:", mesg->btree_k[H5B_SNODE_ID]);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
              "Symbol table node leaf 'K' value:", mesg->sym_leaf_k);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O__btreek_debug() */
