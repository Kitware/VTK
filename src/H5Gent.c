/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/****************/
/* Module Setup */
/****************/

#include "H5Gmodule.h" /* This source code file is part of the H5G module */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions			*/
#include "H5Eprivate.h"  /* Error handling		  	*/
#include "H5Fprivate.h"  /* File access				*/
#include "H5Gpkg.h"      /* Groups		  		*/
#include "H5HLprivate.h" /* Local Heaps				*/
#include "H5MMprivate.h" /* Memory management			*/

/****************/
/* Local Macros */
/****************/

/******************/
/* Local Typedefs */
/******************/

/********************/
/* Package Typedefs */
/********************/

/********************/
/* Local Prototypes */
/********************/

/*********************/
/* Package Variables */
/*********************/

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/*-------------------------------------------------------------------------
 * Function:    H5G__ent_decode_vec
 *
 * Purpose:     Same as H5G_ent_decode() except it does it for an array of
 *              symbol table entries.
 *
 * Return:      Success:        Non-negative, with *pp pointing to the first byte
 *                              after the last symbol.
 *
 *              Failure:        Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G__ent_decode_vec(const H5F_t *f, const uint8_t **pp, const uint8_t *p_end, H5G_entry_t *ent, unsigned n)
{
    unsigned u;                   /* Local index variable */
    herr_t   ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* check arguments */
    assert(f);
    assert(pp);
    assert(ent);

    /* decode entries */
    for (u = 0; u < n; u++) {
        if (*pp > p_end)
            HGOTO_ERROR(H5E_SYM, H5E_CANTDECODE, FAIL, "ran off the end of the image buffer");
        if (H5G_ent_decode(f, pp, ent + u, p_end) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTDECODE, FAIL, "can't decode");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G__ent_decode_vec() */

/*-------------------------------------------------------------------------
 * Function:    H5G_ent_decode
 *
 * Purpose:     Decodes a symbol table entry pointed to by `*pp'.
 *
 * Return:      Success:        Non-negative with *pp pointing to the first byte
 *                              following the symbol table entry.
 *
 *              Failure:        Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_ent_decode(const H5F_t *f, const uint8_t **pp, H5G_entry_t *ent, const uint8_t *p_end)
{
    const uint8_t *p_ret = *pp;
    uint32_t       tmp;
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check arguments */
    assert(f);
    assert(pp);
    assert(ent);

    if (H5_IS_BUFFER_OVERFLOW(*pp, ent->name_off, p_end))
        HGOTO_ERROR(H5E_FILE, H5E_OVERFLOW, FAIL, "image pointer is out of bounds");

    /* decode header */
    H5F_DECODE_LENGTH(f, *pp, ent->name_off);

    if (H5_IS_BUFFER_OVERFLOW(*pp, H5F_SIZEOF_ADDR(f) + sizeof(uint32_t), p_end))
        HGOTO_ERROR(H5E_FILE, H5E_OVERFLOW, FAIL, "image pointer is out of bounds");

    H5F_addr_decode(f, pp, &(ent->header));
    UINT32DECODE(*pp, tmp);
    *pp += 4; /*reserved*/

    if (H5_IS_BUFFER_OVERFLOW(*pp, 1, p_end))
        HGOTO_ERROR(H5E_FILE, H5E_OVERFLOW, FAIL, "image pointer is out of bounds");

    ent->type = (H5G_cache_type_t)tmp;

    /* decode scratch-pad */
    switch (ent->type) {
        case H5G_NOTHING_CACHED:
            break;

        case H5G_CACHED_STAB:
            assert(2 * H5F_SIZEOF_ADDR(f) <= H5G_SIZEOF_SCRATCH);
            if (H5_IS_BUFFER_OVERFLOW(*pp, H5F_SIZEOF_ADDR(f) * 2, p_end))
                HGOTO_ERROR(H5E_FILE, H5E_OVERFLOW, FAIL, "image pointer is out of bounds");
            H5F_addr_decode(f, pp, &(ent->cache.stab.btree_addr));
            H5F_addr_decode(f, pp, &(ent->cache.stab.heap_addr));
            break;

        case H5G_CACHED_SLINK:
            if (H5_IS_BUFFER_OVERFLOW(*pp, sizeof(uint32_t), p_end))
                HGOTO_ERROR(H5E_FILE, H5E_OVERFLOW, FAIL, "image pointer is out of bounds");
            UINT32DECODE(*pp, ent->cache.slink.lval_offset);
            break;

        case H5G_CACHED_ERROR:
        case H5G_NCACHED:
        default:
            HGOTO_ERROR(H5E_SYM, H5E_BADVALUE, FAIL, "unknown symbol table entry cache type");
    } /* end switch */

    *pp = p_ret + H5G_SIZEOF_ENTRY_FILE(f);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_ent_decode() */

/*-------------------------------------------------------------------------
 * Function:    H5G__ent_encode_vec
 *
 * Purpose:     Same as H5G_ent_encode() except it does it for an array of
 *              symbol table entries.
 *
 * Return:      Success:        Non-negative, with *pp pointing to the first byte
 *                              after the last symbol.
 *
 *              Failure:        Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G__ent_encode_vec(const H5F_t *f, uint8_t **pp, const H5G_entry_t *ent, unsigned n)
{
    unsigned u;                   /* Local index variable */
    herr_t   ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* check arguments */
    assert(f);
    assert(pp);
    assert(ent);

    /* encode entries */
    for (u = 0; u < n; u++)
        if (H5G_ent_encode(f, pp, ent + u) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTENCODE, FAIL, "can't encode");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5G__ent_encode_vec() */

/*-------------------------------------------------------------------------
 * Function:    H5G_ent_encode
 *
 * Purpose:     Encodes the specified symbol table entry into the buffer
 *              pointed to by *pp.
 *
 * Return:      Success:        Non-negative, with *pp pointing to the first byte
 *                              after the symbol table entry.
 *
 *              Failure:        Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_ent_encode(const H5F_t *f, uint8_t **pp, const H5G_entry_t *ent)
{
    uint8_t *p_ret     = *pp + H5G_SIZEOF_ENTRY_FILE(f);
    herr_t   ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check arguments */
    assert(f);
    assert(pp);

    /* Check for actual entry to encode */
    if (ent) {
        /* encode header */
        H5F_ENCODE_LENGTH(f, *pp, ent->name_off);
        H5F_addr_encode(f, pp, ent->header);
        UINT32ENCODE(*pp, ent->type);
        UINT32ENCODE(*pp, 0); /*reserved*/

        /* encode scratch-pad */
        switch (ent->type) {
            case H5G_NOTHING_CACHED:
                break;

            case H5G_CACHED_STAB:
                assert(2 * H5F_SIZEOF_ADDR(f) <= H5G_SIZEOF_SCRATCH);
                H5F_addr_encode(f, pp, ent->cache.stab.btree_addr);
                H5F_addr_encode(f, pp, ent->cache.stab.heap_addr);
                break;

            case H5G_CACHED_SLINK:
                UINT32ENCODE(*pp, ent->cache.slink.lval_offset);
                break;

            case H5G_CACHED_ERROR:
            case H5G_NCACHED:
            default:
                HGOTO_ERROR(H5E_SYM, H5E_BADVALUE, FAIL, "unknown symbol table entry cache type");
        } /* end switch */
    }     /* end if */
    else {
        H5F_ENCODE_LENGTH(f, *pp, 0);
        H5F_addr_encode(f, pp, HADDR_UNDEF);
        UINT32ENCODE(*pp, H5G_NOTHING_CACHED);
        UINT32ENCODE(*pp, 0); /*reserved*/
    }                         /* end else */

    /* fill with zero */
    if (*pp < p_ret)
        memset(*pp, 0, (size_t)(p_ret - *pp));
    *pp = p_ret;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_ent_encode() */

/*-------------------------------------------------------------------------
 * Function:    H5G__ent_copy
 *
 * Purpose:     Do a deep copy of symbol table entries
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Notes:       'depth' parameter determines how much of the group entry
 *              structure we want to copy.  The values are:
 *                  H5_COPY_SHALLOW - Copy all the fields from the source
 *                      to the destination, including the user path and
 *                      canonical path. (Destination "takes ownership" of
 *                      user and canonical paths)
 *                  H5_COPY_DEEP - Copy all the fields from the source to
 *                      the destination, deep copying the user and canonical
 *                      paths.
 *
 *-------------------------------------------------------------------------
 */
void
H5G__ent_copy(H5G_entry_t *dst, H5G_entry_t *src, H5_copy_depth_t depth)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Check arguments */
    assert(src);
    assert(dst);
    assert(depth == H5_COPY_SHALLOW || depth == H5_COPY_DEEP);

    /* Copy the top level information */
    H5MM_memcpy(dst, src, sizeof(H5G_entry_t));

    /* Deep copy the names */
    if (depth == H5_COPY_DEEP) {
        /* Nothing currently */
    }
    else if (depth == H5_COPY_SHALLOW) {
        H5G__ent_reset(src);
    } /* end if */

    FUNC_LEAVE_NOAPI_VOID
} /* end H5G__ent_copy() */

/*-------------------------------------------------------------------------
 * Function:	H5G__ent_reset
 *
 * Purpose:	Reset a symbol table entry to an empty state
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 *-------------------------------------------------------------------------
 */
void
H5G__ent_reset(H5G_entry_t *ent)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Check arguments */
    assert(ent);

    /* Clear the symbol table entry to an empty state */
    memset(ent, 0, sizeof(H5G_entry_t));
    ent->header = HADDR_UNDEF;

    FUNC_LEAVE_NOAPI_VOID
} /* end H5G__ent_reset() */

/*-------------------------------------------------------------------------
 * Function:	H5G__ent_to_link
 *
 * Purpose:     Convert a symbol table entry to a link
 *
 * Return:	Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G__ent_to_link(const H5G_entry_t *ent, const H5HL_t *heap, H5O_link_t *lnk)
{
    const char *name;                /* Pointer to link name in heap */
    size_t      block_size;          /* Size of the heap block */
    bool        dup_soft  = false;   /* xstrdup the symbolic link name or not */
    herr_t      ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* check arguments */
    assert(ent);
    assert(heap);
    assert(lnk);

    /* Initialize structure and set (default) common info for link */
    lnk->type         = H5L_TYPE_ERROR;
    lnk->corder_valid = false; /* Creation order not valid for this link */
    lnk->corder       = 0;
    lnk->cset         = H5F_DEFAULT_CSET;
    lnk->name         = NULL;

    /* Get the size of the heap block */
    block_size = H5HL_heap_get_size(heap);

    /* Get pointer to link's name in the heap */
    if (NULL == (name = (const char *)H5HL_offset_into(heap, ent->name_off)))
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "unable to get symbol table link name");

    if (NULL == (lnk->name = H5MM_strndup(name, (block_size - ent->name_off))))
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "unable to duplicate link name");
    if (!*lnk->name)
        HGOTO_ERROR(H5E_SYM, H5E_BADVALUE, FAIL, "invalid link name");

    /* Object is a symbolic or hard link */
    if (ent->type == H5G_CACHED_SLINK) {
        const char *s; /* Pointer to link value */

        if (NULL == (s = (const char *)H5HL_offset_into(heap, ent->cache.slink.lval_offset)))
            HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "unable to get symbolic link name");

        /* Copy the link value */
        if (NULL == (lnk->u.soft.name = H5MM_strndup(s, (block_size - ent->cache.slink.lval_offset))))
            HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "unable to duplicate symbolic link name");

        dup_soft = true;

        /* Set link type */
        lnk->type = H5L_TYPE_SOFT;
    } /* end if */
    else {
        /* Set address of object */
        lnk->u.hard.addr = ent->header;

        /* Set link type */
        lnk->type = H5L_TYPE_HARD;
    } /* end else */

done:
    if (ret_value < 0) {
        if (lnk->name)
            H5MM_xfree(lnk->name);
        if (ent->type == H5G_CACHED_SLINK && dup_soft)
            H5MM_xfree(lnk->u.soft.name);
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G__ent_to_link() */

/*-------------------------------------------------------------------------
 * Function:    H5G__ent_debug
 *
 * Purpose:     Prints debugging information about a symbol table entry.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G__ent_debug(const H5G_entry_t *ent, FILE *stream, int indent, int fwidth, const H5HL_t *heap)
{
    const char *lval = NULL;
    int         nested_indent, nested_fwidth;

    FUNC_ENTER_PACKAGE_NOERR

    /* Calculate the indent & field width values for nested information */
    nested_indent = indent + 3;
    nested_fwidth = MAX(0, fwidth - 3);

    fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
            "Name offset into private heap:", (unsigned long)(ent->name_off));

    fprintf(stream, "%*s%-*s %" PRIuHADDR "\n", indent, "", fwidth, "Object header address:", ent->header);

    fprintf(stream, "%*s%-*s ", indent, "", fwidth, "Cache info type:");
    switch (ent->type) {
        case H5G_NOTHING_CACHED:
            fprintf(stream, "Nothing Cached\n");
            break;

        case H5G_CACHED_STAB:
            fprintf(stream, "Symbol Table\n");

            fprintf(stream, "%*s%-*s\n", indent, "", fwidth, "Cached entry information:");
            fprintf(stream, "%*s%-*s %" PRIuHADDR "\n", nested_indent, "", nested_fwidth,
                    "B-tree address:", ent->cache.stab.btree_addr);

            fprintf(stream, "%*s%-*s %" PRIuHADDR "\n", nested_indent, "", nested_fwidth,
                    "Heap address:", ent->cache.stab.heap_addr);
            break;

        case H5G_CACHED_SLINK:
            fprintf(stream, "Symbolic Link\n");
            fprintf(stream, "%*s%-*s\n", indent, "", fwidth, "Cached information:");
            fprintf(stream, "%*s%-*s %lu\n", nested_indent, "", nested_fwidth,
                    "Link value offset:", (unsigned long)(ent->cache.slink.lval_offset));
            if (heap) {
                lval = (const char *)H5HL_offset_into(heap, ent->cache.slink.lval_offset);
                fprintf(stream, "%*s%-*s %s\n", nested_indent, "", nested_fwidth,
                        "Link value:", (lval == NULL) ? "" : lval);
            } /* end if */
            else
                fprintf(stream, "%*s%-*s\n", nested_indent, "", nested_fwidth,
                        "Warning: Invalid heap address given, name not displayed!");
            break;

        case H5G_CACHED_ERROR:
        case H5G_NCACHED:
        default:
            fprintf(stream, "*** Unknown symbol type %d\n", ent->type);
            break;
    } /* end switch */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5G__ent_debug() */
