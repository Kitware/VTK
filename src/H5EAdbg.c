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
 * Created:        H5EAdbg.c
 *            Sep 11 2008
 *            Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:        Dump debugging information about an extensible array.
 *
 *-------------------------------------------------------------------------
 */

/**********************/
/* Module Declaration */
/**********************/

#include "H5EAmodule.h"         /* This source code file is part of the H5EA module */


/***********************/
/* Other Packages Used */
/***********************/


/***********/
/* Headers */
/***********/
#include "H5private.h"        /* Generic Functions            */
#include "H5Eprivate.h"        /* Error handling              */
#include "H5EApkg.h"        /* Extensible Arrays            */


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
 * Function:    H5EA__hdr_debug
 *
 * Purpose:    Prints debugging info about a extensible array header.
 *
 * Return:    Non-negative on success/Negative on failure
 *
 * Programmer:    Quincey Koziol
 *        koziol@hdfgroup.org
 *        Sep 11 2008
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
herr_t, SUCCEED, FAIL,
H5EA__hdr_debug(H5F_t *f, haddr_t addr, FILE *stream, int indent,
    int fwidth, const H5EA_class_t *cls, haddr_t obj_addr))

    /* Local variables */
    H5EA_hdr_t *hdr = NULL;     /* Shared extensible array header */
    void *dbg_ctx = NULL;       /* Extensible array debugging context */

    /* Check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(H5F_addr_defined(obj_addr));
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);
    HDassert(cls);

    /* Check for debugging context callback available */
    if(cls->crt_dbg_ctx)
        /* Create debugging context */
        if(NULL == (dbg_ctx = cls->crt_dbg_ctx(f, obj_addr)))
            H5E_THROW(H5E_CANTGET, "unable to create fixed array debugging context")

    /* Load the extensible array header */
    if(NULL == (hdr = H5EA__hdr_protect(f, addr, dbg_ctx, H5AC__READ_ONLY_FLAG)))
    H5E_THROW(H5E_CANTPROTECT, "unable to load extensible array header")

    /* Print opening message */
    HDfprintf(stream, "%*sExtensible Array Header...\n", indent, "");

    /* Print the values */
    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
        "Array class ID:",  hdr->cparam.cls->name);
    HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
        "Header size:",
        hdr->size);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
        "Raw Element Size:",
        (unsigned)hdr->cparam.raw_elmt_size);
    HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
        "Native Element Size (on this platform):",
        hdr->cparam.cls->nat_elmt_size);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
        "Log2(Max. # of elements in array):",
        (unsigned)hdr->cparam.max_nelmts_bits);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
        "# of elements in index block:",
        (unsigned)hdr->cparam.idx_blk_elmts);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
        "Min. # of elements per data block:",
        (unsigned)hdr->cparam.data_blk_min_elmts);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
        "Min. # of data block pointers for a super block:",
        (unsigned)hdr->cparam.sup_blk_min_data_ptrs);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
        "Log2(Max. # of elements in data block page):",
        (unsigned)hdr->cparam.max_dblk_page_nelmts_bits);
    HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
        "Highest element index stored (+1):",
        hdr->stats.stored.max_idx_set);
    HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
        "Number of super blocks created:",
        hdr->stats.stored.nsuper_blks);
    HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
        "Number of data blocks created:",
        hdr->stats.stored.ndata_blks);
    HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
        "Number of elements 'realized':",
        hdr->stats.stored.nelmts);
    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
        "Index Block Address:",
        hdr->idx_blk_addr);

CATCH
    if(dbg_ctx && cls->dst_dbg_ctx(dbg_ctx) < 0)
        H5E_THROW(H5E_CANTRELEASE, "unable to release extensible array debugging context")
    if(hdr && H5EA__hdr_unprotect(hdr, H5AC__NO_FLAGS_SET) < 0)
    H5E_THROW(H5E_CANTUNPROTECT, "unable to release extensible array header")

END_FUNC(PKG)   /* end H5EA__hdr_debug() */


/*-------------------------------------------------------------------------
 * Function:    H5EA__iblock_debug
 *
 * Purpose:    Prints debugging info about a extensible array index block.
 *
 * Return:    Non-negative on success/Negative on failure
 *
 * Programmer:    Quincey Koziol
 *        koziol@hdfgroup.org
 *        Sep 11 2008
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
herr_t, SUCCEED, FAIL,
H5EA__iblock_debug(H5F_t *f, haddr_t H5_ATTR_UNUSED addr, FILE *stream, int indent,
    int fwidth, const H5EA_class_t *cls, haddr_t hdr_addr, haddr_t obj_addr))

    /* Local variables */
    H5EA_hdr_t *hdr = NULL;         /* Shared extensible array header */
    H5EA_iblock_t *iblock = NULL;   /* Extensible array index block */
    void *dbg_ctx = NULL;           /* Extensible array context */

    /* Check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);
    HDassert(cls);
    HDassert(H5F_addr_defined(hdr_addr));
    HDassert(H5F_addr_defined(obj_addr));

    /* Check for debugging context callback available */
    if(cls->crt_dbg_ctx)
        /* Create debugging context */
        if(NULL == (dbg_ctx = cls->crt_dbg_ctx(f, obj_addr)))
            H5E_THROW(H5E_CANTGET, "unable to create extensible array debugging context")

    /* Load the extensible array header */
    if(NULL == (hdr = H5EA__hdr_protect(f, hdr_addr, dbg_ctx, H5AC__READ_ONLY_FLAG)))
    H5E_THROW(H5E_CANTPROTECT, "unable to load extensible array header")

    /* Sanity check */
    HDassert(H5F_addr_eq(hdr->idx_blk_addr, addr));

    /* Protect index block */
    if(NULL == (iblock = H5EA__iblock_protect(hdr, H5AC__READ_ONLY_FLAG)))
        H5E_THROW(H5E_CANTPROTECT, "unable to protect extensible array index block, address = %llu", (unsigned long long)hdr->idx_blk_addr)

    /* Print opening message */
    HDfprintf(stream, "%*sExtensible Array Index Block...\n", indent, "");

    /* Print the values */
    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
        "Array class ID:",  hdr->cparam.cls->name);
    HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
        "Index Block size:",
        iblock->size);
    HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
        "# of data block addresses in index block:",
        iblock->ndblk_addrs);
    HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
        "# of super block addresses in index block:",
        iblock->nsblk_addrs);

    /* Check if there are any elements in index block */
    if(hdr->cparam.idx_blk_elmts > 0) {
        unsigned u;             /* Local index variable */

        /* Print the elements in the index block */
        HDfprintf(stream, "%*sElements in Index Block:\n", indent, "");
        for(u = 0; u < hdr->cparam.idx_blk_elmts; u++) {
            /* Call the class's 'debug' callback */
            if((hdr->cparam.cls->debug)(stream, (indent + 3), MAX(0, (fwidth - 3)),
                    (hsize_t)u,
                    ((uint8_t *)iblock->elmts) + (hdr->cparam.cls->nat_elmt_size * u)) < 0)
                H5E_THROW(H5E_CANTGET, "can't get element for debugging")
        } /* end for */
    } /* end if */

    /* Check if there are any data block addresses in index block */
    if(iblock->ndblk_addrs > 0) {
        char temp_str[128];     /* Temporary string, for formatting */
        unsigned u;             /* Local index variable */

        /* Print the data block addresses in the index block */
        HDfprintf(stream, "%*sData Block Addresses in Index Block:\n", indent, "");
        for(u = 0; u < iblock->ndblk_addrs; u++) {
            /* Print address */
            HDsprintf(temp_str, "Address #%u:", u);
            HDfprintf(stream, "%*s%-*s %a\n", (indent + 3), "", MAX(0, (fwidth - 3)),
                temp_str,
                iblock->dblk_addrs[u]);
        } /* end for */
    } /* end if */

    /* Check if there are any super block addresses in index block */
    if(iblock->nsblk_addrs > 0) {
        char temp_str[128];     /* Temporary string, for formatting */
        unsigned u;             /* Local index variable */

        /* Print the super block addresses in the index block */
        HDfprintf(stream, "%*sSuper Block Addresses in Index Block:\n", indent, "");
        for(u = 0; u < iblock->nsblk_addrs; u++) {
            /* Print address */
            HDsprintf(temp_str, "Address #%u:", u);
            HDfprintf(stream, "%*s%-*s %a\n", (indent + 3), "", MAX(0, (fwidth - 3)),
                temp_str,
                iblock->sblk_addrs[u]);
        } /* end for */
    } /* end if */

CATCH
    if(dbg_ctx && cls->dst_dbg_ctx(dbg_ctx) < 0)
        H5E_THROW(H5E_CANTRELEASE, "unable to release extensible array debugging context")
    if(iblock && H5EA__iblock_unprotect(iblock, H5AC__NO_FLAGS_SET) < 0)
        H5E_THROW(H5E_CANTUNPROTECT, "unable to release extensible array index block")
    if(hdr && H5EA__hdr_unprotect(hdr, H5AC__NO_FLAGS_SET) < 0)
    H5E_THROW(H5E_CANTUNPROTECT, "unable to release extensible array header")

END_FUNC(PKG)   /* end H5EA__iblock_debug() */


/*-------------------------------------------------------------------------
 * Function:    H5EA__sblock_debug
 *
 * Purpose:    Prints debugging info about a extensible array super block.
 *
 * Return:    Non-negative on success/Negative on failure
 *
 * Programmer:    Quincey Koziol
 *        koziol@hdfgroup.org
 *        Sep 30 2008
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
herr_t, SUCCEED, FAIL,
H5EA__sblock_debug(H5F_t *f, haddr_t addr, FILE *stream, int indent,
    int fwidth, const H5EA_class_t *cls, haddr_t hdr_addr, unsigned sblk_idx, haddr_t obj_addr))

    /* Local variables */
    H5EA_hdr_t *hdr = NULL;         /* Shared extensible array header */
    H5EA_sblock_t *sblock = NULL;   /* Extensible array super block */
    void *dbg_ctx = NULL;           /* Extensible array context */

    /* Check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);
    HDassert(cls);
    HDassert(H5F_addr_defined(hdr_addr));
    HDassert(H5F_addr_defined(obj_addr));

    /* Check for debugging context callback available */
    if(cls->crt_dbg_ctx)
        /* Create debugging context */
        if(NULL == (dbg_ctx = cls->crt_dbg_ctx(f, obj_addr)))
            H5E_THROW(H5E_CANTGET, "unable to create extensible array debugging context")

    /* Load the extensible array header */
    if(NULL == (hdr = H5EA__hdr_protect(f, hdr_addr, dbg_ctx, H5AC__READ_ONLY_FLAG)))
    H5E_THROW(H5E_CANTPROTECT, "unable to load extensible array header")

    /* Protect super block */
    /* (Note: setting parent of super block to 'hdr' for this operation should be OK -QAK) */
    if(NULL == (sblock = H5EA__sblock_protect(hdr, (H5EA_iblock_t *)hdr, addr, sblk_idx, H5AC__READ_ONLY_FLAG)))
        H5E_THROW(H5E_CANTPROTECT, "unable to protect extensible array super block, address = %llu", (unsigned long long)addr)

    /* Print opening message */
    HDfprintf(stream, "%*sExtensible Array Super Block...\n", indent, "");

    /* Print the values */
    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
        "Array class ID:",  hdr->cparam.cls->name);
    HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
        "Super Block size:",
        sblock->size);
    HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
        "# of data block addresses in super block:",
        sblock->ndblks);
    HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
        "# of elements in data blocks from this super block:",
        sblock->dblk_nelmts);

    /* Check if there are any data block addresses in super block */
    if(sblock->ndblks > 0) {
        char temp_str[128];     /* Temporary string, for formatting */
        unsigned u;             /* Local index variable */

        /* Print the data block addresses in the super block */
        HDfprintf(stream, "%*sData Block Addresses in Super Block:\n", indent, "");
        for(u = 0; u < sblock->ndblks; u++) {
            /* Print address */
            HDsprintf(temp_str, "Address #%u:", u);
            HDfprintf(stream, "%*s%-*s %a\n", (indent + 3), "", MAX(0, (fwidth - 3)),
                temp_str,
                sblock->dblk_addrs[u]);
        } /* end for */
    } /* end if */

CATCH
    if(dbg_ctx && cls->dst_dbg_ctx(dbg_ctx) < 0)
        H5E_THROW(H5E_CANTRELEASE, "unable to release extensible array debugging context")
    if(sblock && H5EA__sblock_unprotect(sblock, H5AC__NO_FLAGS_SET) < 0)
        H5E_THROW(H5E_CANTUNPROTECT, "unable to release extensible array super block")
    if(hdr && H5EA__hdr_unprotect(hdr, H5AC__NO_FLAGS_SET) < 0)
    H5E_THROW(H5E_CANTUNPROTECT, "unable to release extensible array header")

END_FUNC(PKG)   /* end H5EA__sblock_debug() */


/*-------------------------------------------------------------------------
 * Function:    H5EA__dblock_debug
 *
 * Purpose:    Prints debugging info about a extensible array data block.
 *
 * Return:    Non-negative on success/Negative on failure
 *
 * Programmer:    Quincey Koziol
 *        koziol@hdfgroup.org
 *        Sep 22 2008
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
herr_t, SUCCEED, FAIL,
H5EA__dblock_debug(H5F_t *f, haddr_t addr, FILE *stream, int indent,
    int fwidth, const H5EA_class_t *cls, haddr_t hdr_addr, size_t dblk_nelmts, haddr_t obj_addr))

    /* Local variables */
    H5EA_hdr_t *hdr = NULL;             /* Shared extensible array header */
    H5EA_dblock_t *dblock = NULL;       /* Extensible array data block */
    void *dbg_ctx = NULL;               /* Extensible array context */
    size_t u;                           /* Local index variable */

    /* Check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);
    HDassert(cls);
    HDassert(H5F_addr_defined(hdr_addr));
    HDassert(H5F_addr_defined(obj_addr));
    HDassert(dblk_nelmts > 0);

    /* Check for debugging context callback available */
    if(cls->crt_dbg_ctx)
        /* Create debugging context */
        if(NULL == (dbg_ctx = cls->crt_dbg_ctx(f, obj_addr)))
            H5E_THROW(H5E_CANTGET, "unable to create extensible array debugging context")

    /* Load the extensible array header */
    if(NULL == (hdr = H5EA__hdr_protect(f, hdr_addr, dbg_ctx, H5AC__READ_ONLY_FLAG)))
    H5E_THROW(H5E_CANTPROTECT, "unable to load extensible array header")

    /* Protect data block */
    /* (Note: setting parent of data block to 'hdr' for this operation should be OK -QAK) */
    if(NULL == (dblock = H5EA__dblock_protect(hdr, hdr, addr, dblk_nelmts, H5AC__READ_ONLY_FLAG)))
        H5E_THROW(H5E_CANTPROTECT, "unable to protect extensible array data block, address = %llu", (unsigned long long)addr)

    /* Print opening message */
    HDfprintf(stream, "%*sExtensible Array data Block...\n", indent, "");

    /* Print the values */
    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
        "Array class ID:",  hdr->cparam.cls->name);
    HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
        "Data Block size:",
        dblock->size);


    /* Print the elements in the index block */
    HDfprintf(stream, "%*sElements:\n", indent, "");
    for(u = 0; u < dblk_nelmts; u++) {
        /* Call the class's 'debug' callback */
        if((hdr->cparam.cls->debug)(stream, (indent + 3), MAX(0, (fwidth - 3)),
                (hsize_t)u,
                ((uint8_t *)dblock->elmts) + (hdr->cparam.cls->nat_elmt_size * u)) < 0)
            H5E_THROW(H5E_CANTGET, "can't get element for debugging")
    } /* end for */

CATCH
    if(dbg_ctx && cls->dst_dbg_ctx(dbg_ctx) < 0)
        H5E_THROW(H5E_CANTRELEASE, "unable to release extensible array debugging context")
    if(dblock && H5EA__dblock_unprotect(dblock, H5AC__NO_FLAGS_SET) < 0)
        H5E_THROW(H5E_CANTUNPROTECT, "unable to release extensible array data block")
    if(hdr && H5EA__hdr_unprotect(hdr, H5AC__NO_FLAGS_SET) < 0)
    H5E_THROW(H5E_CANTUNPROTECT, "unable to release extensible array header")

END_FUNC(PKG)   /* end H5EA__dblock_debug() */

