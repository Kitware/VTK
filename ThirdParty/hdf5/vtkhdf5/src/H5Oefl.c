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

/*
 * Programmer: Robb Matzke
 *	       Tuesday, November 25, 1997
 */

#include "H5Omodule.h" /* This source code file is part of the H5O module */

#include "H5private.h"   /* Generic Functions			*/
#include "H5Eprivate.h"  /* Error handling		  	*/
#include "H5Fprivate.h"  /* File access				*/
#include "H5HLprivate.h" /* Local Heaps				*/
#include "H5MMprivate.h" /* Memory management			*/
#include "H5Opkg.h"      /* Object headers			*/

/* PRIVATE PROTOTYPES */
static void * H5O__efl_decode(H5F_t *f, H5O_t *open_oh, unsigned mesg_flags, unsigned *ioflags, size_t p_size,
                              const uint8_t *p);
static herr_t H5O__efl_encode(H5F_t *f, hbool_t disable_shared, uint8_t *p, const void *_mesg);
static void * H5O__efl_copy(const void *_mesg, void *_dest);
static size_t H5O__efl_size(const H5F_t *f, hbool_t disable_shared, const void *_mesg);
static herr_t H5O__efl_reset(void *_mesg);
static void * H5O__efl_copy_file(H5F_t *file_src, void *mesg_src, H5F_t *file_dst, hbool_t *recompute_size,
                                 unsigned *mesg_flags, H5O_copy_t *cpy_info, void *udata);
static herr_t H5O__efl_debug(H5F_t *f, const void *_mesg, FILE *stream, int indent, int fwidth);

/* This message derives from H5O message class */
const H5O_msg_class_t H5O_MSG_EFL[1] = {{
    H5O_EFL_ID,           /*message id number		*/
    "external file list", /*message name for debugging    */
    sizeof(H5O_efl_t),    /*native message size	    	*/
    0,                    /* messages are sharable?       */
    H5O__efl_decode,      /*decode message		*/
    H5O__efl_encode,      /*encode message		*/
    H5O__efl_copy,        /*copy native value		*/
    H5O__efl_size,        /*size of message on disk	*/
    H5O__efl_reset,       /*reset method		    	*/
    NULL,                 /* free method			*/
    NULL,                 /* file delete method		*/
    NULL,                 /* link method			*/
    NULL,                 /*set share method		*/
    NULL,                 /*can share method		*/
    NULL,                 /* pre copy native value to file */
    H5O__efl_copy_file,   /* copy native value to file    */
    NULL,                 /* post copy native value to file    */
    NULL,                 /* get creation index		*/
    NULL,                 /* set creation index		*/
    H5O__efl_debug        /*debug the message		*/
}};

#define H5O_EFL_VERSION 1

/*-------------------------------------------------------------------------
 * Function:    H5O__efl_decode
 *
 * Purpose:	Decode an external file list message and return a pointer to
 *          the message (and some other data).
 *
 * Return:  Success:    Ptr to a new message struct.
 *
 *          Failure:    NULL
 *
 * Programmer:	Robb Matzke
 *              Tuesday, November 25, 1997
 *
 * Modification:
 *              Raymond Lu
 *              11 April 2011
 *              We allow zero dimension size starting from the 1.8.7 release.
 *              The dataset size of external storage can be zero.
 *-------------------------------------------------------------------------
 */
static void *
H5O__efl_decode(H5F_t *f, H5O_t H5_ATTR_UNUSED *open_oh, unsigned H5_ATTR_UNUSED mesg_flags,
                unsigned H5_ATTR_UNUSED *ioflags, size_t H5_ATTR_UNUSED p_size, const uint8_t *p)
{
    H5O_efl_t * mesg = NULL;
    int         version;
    const char *s = NULL;
    H5HL_t *    heap;
    size_t      u;                /* Local index variable */
    void *      ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* Check args */
    HDassert(f);
    HDassert(p);

    if (NULL == (mesg = (H5O_efl_t *)H5MM_calloc(sizeof(H5O_efl_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Version */
    version = *p++;
    if (version != H5O_EFL_VERSION)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "bad version number for external file list message")

    /* Reserved */
    p += 3;

    /* Number of slots */
    UINT16DECODE(p, mesg->nalloc);
    HDassert(mesg->nalloc > 0);
    UINT16DECODE(p, mesg->nused);
    HDassert(mesg->nused <= mesg->nalloc);

    /* Heap address */
    H5F_addr_decode(f, &p, &(mesg->heap_addr));

#ifndef NDEBUG
    HDassert(H5F_addr_defined(mesg->heap_addr));

    if (NULL == (heap = H5HL_protect(f, mesg->heap_addr, H5AC__READ_ONLY_FLAG)))
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, NULL, "unable to read protect link value")

    s = (const char *)H5HL_offset_into(heap, 0);

    HDassert(s && !*s);

    if (H5HL_unprotect(heap) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, NULL, "unable to read unprotect link value")
    heap = NULL;
#endif

    /* Decode the file list */
    mesg->slot = (H5O_efl_entry_t *)H5MM_calloc(mesg->nalloc * sizeof(H5O_efl_entry_t));
    if (NULL == mesg->slot)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    if (NULL == (heap = H5HL_protect(f, mesg->heap_addr, H5AC__READ_ONLY_FLAG)))
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, NULL, "unable to read protect link value")
    for (u = 0; u < mesg->nused; u++) {
        /* Name */
        H5F_DECODE_LENGTH(f, p, mesg->slot[u].name_offset);

        if ((s = (const char *)H5HL_offset_into(heap, mesg->slot[u].name_offset)) == NULL)
            HGOTO_ERROR(H5E_SYM, H5E_CANTGET, NULL, "unable to get external file name")
        if (*s == (char)'\0')
            HGOTO_ERROR(H5E_SYM, H5E_CANTGET, NULL, "invalid external file name")
        mesg->slot[u].name = H5MM_xstrdup(s);
        HDassert(mesg->slot[u].name);

        /* File offset */
        H5F_DECODE_LENGTH(f, p, mesg->slot[u].offset);

        /* Size */
        H5F_DECODE_LENGTH(f, p, mesg->slot[u].size);
    } /* end for */

    if (H5HL_unprotect(heap) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, NULL, "unable to read unprotect link value")
    heap = NULL;

    /* Set return value */
    ret_value = mesg;

done:
    if (ret_value == NULL)
        if (mesg != NULL)
            H5MM_xfree(mesg);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__efl_decode() */

/*-------------------------------------------------------------------------
 * Function:	H5O__efl_encode
 *
 * Purpose:	Encodes a message.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Tuesday, November 25, 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__efl_encode(H5F_t *f, hbool_t H5_ATTR_UNUSED disable_shared, uint8_t *p, const void *_mesg)
{
    const H5O_efl_t *mesg = (const H5O_efl_t *)_mesg;
    size_t           u; /* Local index variable */

    FUNC_ENTER_STATIC_NOERR

    /* check args */
    HDassert(f);
    HDassert(mesg);
    HDassert(p);

    /* Version */
    *p++ = H5O_EFL_VERSION;

    /* Reserved */
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;

    /* Number of slots */
    HDassert(mesg->nalloc > 0);
    UINT16ENCODE(p, mesg->nused); /*yes, twice*/
    HDassert(mesg->nused > 0 && mesg->nused <= mesg->nalloc);
    UINT16ENCODE(p, mesg->nused);

    /* Heap address */
    HDassert(H5F_addr_defined(mesg->heap_addr));
    H5F_addr_encode(f, &p, mesg->heap_addr);

    /* Encode file list */
    for (u = 0; u < mesg->nused; u++) {
        /*
         * The name should have been added to the heap when the dataset was
         * created.
         */
        HDassert(mesg->slot[u].name_offset);
        H5F_ENCODE_LENGTH(f, p, mesg->slot[u].name_offset);
        H5F_ENCODE_LENGTH(f, p, (hsize_t)mesg->slot[u].offset);
        H5F_ENCODE_LENGTH(f, p, mesg->slot[u].size);
    } /* end for */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O__efl_encode() */

/*-------------------------------------------------------------------------
 * Function:	H5O__efl_copy
 *
 * Purpose:	Copies a message from _MESG to _DEST, allocating _DEST if
 *		necessary.
 *
 * Return:	Success:	Ptr to _DEST
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		Tuesday, November 25, 1997
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O__efl_copy(const void *_mesg, void *_dest)
{
    const H5O_efl_t *mesg = (const H5O_efl_t *)_mesg;
    H5O_efl_t *      dest = (H5O_efl_t *)_dest;
    size_t           u;                      /* Local index variable */
    hbool_t          slot_allocated = FALSE; /* Flag to indicate that dynamic allocation has begun */
    void *           ret_value      = NULL;  /* Return value */

    FUNC_ENTER_STATIC

    /* check args */
    HDassert(mesg);

    /* Allocate destination message, if necessary */
    if (!dest && NULL == (dest = (H5O_efl_t *)H5MM_calloc(sizeof(H5O_efl_t))))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTALLOC, NULL, "can't allocate efl message")

    /* copy */
    *dest = *mesg;

    /* Deep copy allocated information */
    if (dest->nalloc > 0) {
        if (NULL == (dest->slot = (H5O_efl_entry_t *)H5MM_calloc(dest->nalloc * sizeof(H5O_efl_entry_t))))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTALLOC, NULL, "can't allocate efl message slots")
        slot_allocated = TRUE;
        for (u = 0; u < mesg->nused; u++) {
            dest->slot[u] = mesg->slot[u];
            if (NULL == (dest->slot[u].name = H5MM_xstrdup(mesg->slot[u].name)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTALLOC, NULL, "can't allocate efl message slot name")
        } /* end for */
    }     /* end if */

    /* Set return value */
    ret_value = dest;

done:
    if (NULL == ret_value) {
        if (slot_allocated) {
            for (u = 0; u < dest->nused; u++)
                if (dest->slot[u].name != NULL && dest->slot[u].name != mesg->slot[u].name)
                    dest->slot[u].name = (char *)H5MM_xfree(dest->slot[u].name);
            dest->slot = (H5O_efl_entry_t *)H5MM_xfree(dest->slot);
        } /* end if */
        if (NULL == _dest)
            dest = (H5O_efl_t *)H5MM_xfree(dest);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__efl_copy() */

/*-------------------------------------------------------------------------
 * Function:	H5O__efl_size
 *
 * Purpose:	Returns the size of the raw message in bytes not counting the
 *		message type or size fields, but only the data fields.	This
 *		function doesn't take into account message alignment. This
 *		function doesn't count unused slots.
 *
 * Return:	Success:	Message data size in bytes.
 *
 *		Failure:	0
 *
 * Programmer:	Robb Matzke
 *		Tuesday, November 25, 1997
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O__efl_size(const H5F_t *f, hbool_t H5_ATTR_UNUSED disable_shared, const void *_mesg)
{
    const H5O_efl_t *mesg      = (const H5O_efl_t *)_mesg;
    size_t           ret_value = 0;

    FUNC_ENTER_STATIC_NOERR

    /* check args */
    HDassert(f);
    HDassert(mesg);

    ret_value = (size_t)H5F_SIZEOF_ADDR(f) +                /*heap address	*/
                2 +                                         /*slots allocated*/
                2 +                                         /*num slots used*/
                4 +                                         /*reserved	*/
                mesg->nused * ((size_t)H5F_SIZEOF_SIZE(f) + /*name offset	*/
                               (size_t)H5F_SIZEOF_SIZE(f) + /*file offset	*/
                               (size_t)H5F_SIZEOF_SIZE(f)); /*file size	*/

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__efl_size() */

/*-------------------------------------------------------------------------
 * Function:	H5O__efl_reset
 *
 * Purpose:	Frees internal pointers and resets the message to an
 *		initialial state.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Tuesday, November 25, 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__efl_reset(void *_mesg)
{
    H5O_efl_t *mesg = (H5O_efl_t *)_mesg;
    size_t     u; /* Local index variable */

    FUNC_ENTER_STATIC_NOERR

    /* check args */
    HDassert(mesg);

    /* reset */
    if (mesg->slot) {
        for (u = 0; u < mesg->nused; u++) {
            mesg->slot[u].name        = (char *)H5MM_xfree(mesg->slot[u].name);
            mesg->slot[u].name_offset = 0;
        } /* end for */
        mesg->slot = (H5O_efl_entry_t *)H5MM_xfree(mesg->slot);
    } /* end if */
    mesg->heap_addr = HADDR_UNDEF;
    mesg->nused = mesg->nalloc = 0;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O__efl_reset() */

/*-------------------------------------------------------------------------
 * Function:	H5O_efl_total_size
 *
 * Purpose:	Return the total size of the external file list by summing
 *		the sizes of all of the files.
 *
 * Return:	Success:	Total reserved size for external data.
 *
 *		Failure:	0
 *
 * Programmer:	Robb Matzke
 *              Tuesday, March  3, 1998
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5O_efl_total_size(H5O_efl_t *efl)
{
    hsize_t ret_value = 0, tmp;

    FUNC_ENTER_NOAPI(0)

    if (efl->nused > 0 && H5O_EFL_UNLIMITED == efl->slot[efl->nused - 1].size)
        ret_value = H5O_EFL_UNLIMITED;
    else {
        size_t u; /* Local index variable */

        for (u = 0; u < efl->nused; u++, ret_value = tmp) {
            tmp = ret_value + efl->slot[u].size;
            if (tmp <= ret_value)
                HGOTO_ERROR(H5E_EFL, H5E_OVERFLOW, 0, "total external storage size overflowed");
        } /* end for */
    }     /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_efl_total_size() */

/*-------------------------------------------------------------------------
 * Function:    H5O__efl_copy_file
 *
 * Purpose:     Copies an efl message from _MESG to _DEST in file
 *
 * Return:      Success:        Ptr to _DEST
 *
 *              Failure:        NULL
 *
 * Programmer:  Peter Cao
 *              September 29, 2005
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O__efl_copy_file(H5F_t H5_ATTR_UNUSED *file_src, void *mesg_src, H5F_t *file_dst,
                   hbool_t H5_ATTR_UNUSED *recompute_size, unsigned H5_ATTR_UNUSED *mesg_flags,
                   H5O_copy_t H5_ATTR_UNUSED *cpy_info, void H5_ATTR_UNUSED *_udata)
{
    H5O_efl_t *efl_src = (H5O_efl_t *)mesg_src;
    H5O_efl_t *efl_dst = NULL;
    H5HL_t *   heap    = NULL; /* Pointer to local heap for EFL file names */
    size_t     idx, size, name_offset, heap_size;
    void *     ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC_TAG(H5AC__COPIED_TAG)

    /* check args */
    HDassert(efl_src);
    HDassert(file_dst);

    /* Allocate space for the destination efl */
    if (NULL == (efl_dst = (H5O_efl_t *)H5MM_calloc(sizeof(H5O_efl_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Copy the "top level" information */
    H5MM_memcpy(efl_dst, efl_src, sizeof(H5O_efl_t));

    /* Determine size needed for destination heap */
    heap_size = H5HL_ALIGN(1); /* "empty" name */
    for (idx = 0; idx < efl_src->nused; idx++)
        heap_size += H5HL_ALIGN(HDstrlen(efl_src->slot[idx].name) + 1);

    /* Create name heap */
    if (H5HL_create(file_dst, heap_size, &efl_dst->heap_addr /*out*/) < 0)
        HGOTO_ERROR(H5E_EFL, H5E_CANTINIT, NULL, "can't create heap")

    /* Pin the heap down in memory */
    if (NULL == (heap = H5HL_protect(file_dst, efl_dst->heap_addr, H5AC__NO_FLAGS_SET)))
        HGOTO_ERROR(H5E_EFL, H5E_PROTECT, NULL, "unable to protect EFL file name heap")

    /* Insert "empty" name first */
    if (H5HL_insert(file_dst, heap, (size_t)1, "", &name_offset) < 0)
        HGOTO_ERROR(H5E_EFL, H5E_CANTINSERT, NULL, "can't insert file name into heap")
    HDassert(0 == name_offset);

    /* allocate array of external file entries */
    if (efl_src->nalloc > 0) {
        size = efl_src->nalloc * sizeof(H5O_efl_entry_t);
        if ((efl_dst->slot = (H5O_efl_entry_t *)H5MM_calloc(size)) == NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

        /* copy content from the source. Need to update later */
        H5MM_memcpy(efl_dst->slot, efl_src->slot, size);
    } /* end if */

    /* copy the name from the source */
    for (idx = 0; idx < efl_src->nused; idx++) {
        efl_dst->slot[idx].name = H5MM_xstrdup(efl_src->slot[idx].name);
        if (H5HL_insert(file_dst, heap, HDstrlen(efl_dst->slot[idx].name) + 1, efl_dst->slot[idx].name,
                        &(efl_dst->slot[idx].name_offset)) < 0)
            HGOTO_ERROR(H5E_EFL, H5E_CANTINSERT, NULL, "can't insert file name into heap")
    }

    /* Set return value */
    ret_value = efl_dst;

done:
    /* Release resources */
    if (heap && H5HL_unprotect(heap) < 0)
        HDONE_ERROR(H5E_EFL, H5E_PROTECT, NULL, "unable to unprotect EFL file name heap")
    if (!ret_value)
        if (efl_dst)
            H5MM_xfree(efl_dst);

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5O__efl_copy_file() */

/*-------------------------------------------------------------------------
 * Function:	H5O__efl_debug
 *
 * Purpose:	Prints debugging info for a message.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Tuesday, November 25, 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__efl_debug(H5F_t H5_ATTR_UNUSED *f, const void *_mesg, FILE *stream, int indent, int fwidth)
{
    const H5O_efl_t *mesg = (const H5O_efl_t *)_mesg;
    size_t           u;

    FUNC_ENTER_STATIC_NOERR

    /* check args */
    HDassert(f);
    HDassert(mesg);
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %" PRIuHADDR "\n", indent, "", fwidth, "Heap address:", mesg->heap_addr);

    HDfprintf(stream, "%*s%-*s %zu/%zu\n", indent, "", fwidth, "Slots used/allocated:", mesg->nused,
              mesg->nalloc);

    for (u = 0; u < mesg->nused; u++) {
        char buf[64];

        HDsnprintf(buf, sizeof(buf), "File %zu", u);
        HDfprintf(stream, "%*s%s:\n", indent, "", buf);

        HDfprintf(stream, "%*s%-*s \"%s\"\n", indent + 3, "", MAX(fwidth - 3, 0),
                  "Name:", mesg->slot[u].name);

        HDfprintf(stream, "%*s%-*s %zu\n", indent + 3, "", MAX(fwidth - 3, 0),
                  "Name offset:", mesg->slot[u].name_offset);

        HDfprintf(stream, "%*s%-*s %" PRIdMAX "\n", indent + 3, "", MAX(fwidth - 3, 0),
                  "Offset of data in file:", (intmax_t)(mesg->slot[u].offset));

        HDfprintf(stream, "%*s%-*s %" PRIuHSIZE "\n", indent + 3, "", MAX(fwidth - 3, 0),
                  "Bytes reserved for data:", (mesg->slot[u].size));
    } /* end for */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O__efl_debug() */
