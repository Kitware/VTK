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

/* Programmer:  Quincey Koziol <koziol@hdfgroup.org>
 *              Wednesday, July 9, 2003
 *
 * Purpose:     Local Heap object debugging functions.
 */

/****************/
/* Module Setup */
/****************/

#include "H5HLmodule.h"         /* This source code file is part of the H5HL module */

/***********/
/* Headers */
/***********/

#include "H5private.h"      /* Generic Functions            */
#include "H5Eprivate.h"     /* Error handling               */
#include "H5HLpkg.h"        /* Local heaps                  */
#include "H5MMprivate.h"    /* Memory management            */


/*-------------------------------------------------------------------------
 * Function:    H5HL_debug
 *
 * Purpose:     Prints debugging information about a heap.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Robb Matzke
 *              Aug  1 1997
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PRIV, ERR,
herr_t, SUCCEED, FAIL,
H5HL_debug(H5F_t *f, haddr_t addr, FILE * stream, int indent, int fwidth))

    H5HL_t		*h = NULL;
    int			free_block;
    H5HL_free_t		*freelist;
    uint8_t		*marker = NULL;
    size_t		amount_free = 0;

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    if(NULL == (h = (H5HL_t *)H5HL_protect(f, addr, H5AC__READ_ONLY_FLAG)))
        H5E_THROW(H5E_CANTPROTECT, "unable to load/protect local heap");

    HDfprintf(stream, "%*sLocal Heap...\n", indent, "");
    HDfprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
	    "Header size (in bytes):",
	    (unsigned long)h->prfx_size);
    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
              "Address of heap data:",
              h->dblk_addr);
    HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
            "Data bytes allocated for heap:",
            h->dblk_size);

    /* Traverse the free list and check that all free blocks fall within
     * the heap and that no two free blocks point to the same region of
     * the heap.
     */
    if(NULL == (marker = (uint8_t *)H5MM_calloc(h->dblk_size)))
        H5E_THROW(H5E_CANTALLOC, "memory allocation failed");

    HDfprintf(stream, "%*sFree Blocks (offset, size):\n", indent, "");
    for(free_block = 0, freelist = h->freelist; freelist; freelist = freelist->next, free_block++) {
        char temp_str[32];

        HDsnprintf(temp_str, sizeof(temp_str), "Block #%d:", free_block);
        HDfprintf(stream, "%*s%-*s %8Zu, %8Zu\n", indent+3, "", MAX(0,fwidth-9), temp_str, freelist->offset, freelist->size);
        if((freelist->offset + freelist->size) > h->dblk_size)
            HDfprintf(stream, "***THAT FREE BLOCK IS OUT OF BOUNDS!\n");
        else {
            int	overlap = 0;
            size_t i;

            for(i = 0; i < freelist->size; i++) {
                if(marker[freelist->offset + i])
                    overlap++;
                marker[freelist->offset + i] = 1;
            } /* end for */
            if(overlap)
                HDfprintf(stream, "***THAT FREE BLOCK OVERLAPPED A PREVIOUS ONE!\n");
            else
                amount_free += freelist->size;
	    } /* end else */
    } /* end for */

    if(h->dblk_size)
        HDfprintf(stream, "%*s%-*s %.2f%%\n", indent, "", fwidth, "Percent of heap used:",
		        ((double)100.0f * (double)(h->dblk_size - amount_free) / (double)h->dblk_size));

    /* Print the data in a VMS-style octal dump */
    H5_buffer_dump(stream, indent, h->dblk_image, marker, (size_t)0, h->dblk_size);

CATCH
    if(h && FAIL == H5HL_unprotect(h))
        H5E_THROW(H5E_CANTUNPROTECT, "unable to release/unprotect local heap");

    if(marker && NULL != (marker = (uint8_t *)H5MM_xfree(marker)))
        H5E_THROW(H5E_CANTFREE, "can't free marker buffer");

END_FUNC(PRIV) /* end H5HL_debug() */

