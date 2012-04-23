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

/* Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Monday, March  6, 2006
 *
 * Purpose:	Fractal heap metadata statistics functions.
 *
 */

/****************/
/* Module Setup */
/****************/

#define H5HF_PACKAGE		/*suppress error about including H5HFpkg  */

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5HFpkg.h"		/* Fractal heaps			*/

/****************/
/* Local Macros */
/****************/


/********************/
/* Package Typedefs */
/********************/


/******************/
/* Local Typedefs */
/******************/


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
 * Function:	H5HF_stat_info
 *
 * Purpose:	Retrieve metadata statistics for the fractal heap
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, March  6, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_stat_info(const H5HF_t *fh, H5HF_stat_t *stats)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5HF_stat_info)

    /* Check arguments. */
    HDassert(fh);
    HDassert(stats);

    /* Report statistics for fractal heap */
    stats->man_size = fh->hdr->man_size;
    stats->man_alloc_size = fh->hdr->man_alloc_size;
    stats->man_iter_off = fh->hdr->man_iter_off;
    stats->man_nobjs = fh->hdr->man_nobjs;
    stats->man_free_space = fh->hdr->total_man_free;
    stats->huge_size = fh->hdr->huge_size;
    stats->huge_nobjs = fh->hdr->huge_nobjs;
    stats->tiny_size = fh->hdr->tiny_size;
    stats->tiny_nobjs = fh->hdr->tiny_nobjs;
/* XXX: Add more metadata statistics for the heap */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5HF_stat_info() */


/*-------------------------------------------------------------------------
 * Function:    H5HF_size
 *
 * Purpose:     Retrieve storage info for:
 *			1. fractal heap
 *			2. btree storage used by huge objects in fractal heap
 *			3. free space storage info
 *
 * Return:      non-negative on success, negative on error
 *
 * Programmer:  Vailin Choi
 *              July 12 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_size(const H5HF_t *fh, hid_t dxpl_id, hsize_t *heap_size)
{
    H5HF_hdr_t *hdr;                    /* Fractal heap header */
    H5B2_t      *bt2 = NULL;            /* v2 B-tree handle for index */
    hsize_t	meta_size = 0;		/* free space storage size */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5HF_size, FAIL)

    /*
     * Check arguments.
     */
    HDassert(fh);
    HDassert(heap_size);

    /* Get "convenience" pointer to fractal heap header */
    hdr = fh->hdr;

    /* Add in values already known */
    *heap_size += hdr->heap_size;               /* Heap header */
    *heap_size += hdr->man_alloc_size;          /* Direct block storage for "managed" objects */
    *heap_size += hdr->huge_size;               /* "huge" object storage */

    /* Check for indirect blocks for managed objects */
    if(H5F_addr_defined(hdr->man_dtable.table_addr) && hdr->man_dtable.curr_root_rows != 0)
        if(H5HF_man_iblock_size(hdr->f, dxpl_id, hdr, hdr->man_dtable.table_addr, hdr->man_dtable.curr_root_rows, NULL, 0, heap_size) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTGET, FAIL, "unable to get fractal heap storage info for indirect block")

    /* Check for B-tree storage of huge objects in fractal heap */
    if(H5F_addr_defined(hdr->huge_bt2_addr)) {
        /* Open the huge object index v2 B-tree */
        if(NULL == (bt2 = H5B2_open(hdr->f, dxpl_id, hdr->huge_bt2_addr, hdr->f)))
            HGOTO_ERROR(H5E_HEAP, H5E_CANTOPENOBJ, FAIL, "unable to open v2 B-tree for tracking 'huge' objects")

        /* Get the B-tree storage */
        if(H5B2_size(bt2, dxpl_id, heap_size) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTGET, FAIL, "can't retrieve B-tree storage info")
    } /* end if */

    /* Get storage for free-space tracking info */
    if(H5F_addr_defined(hdr->fs_addr)) {
        if(H5HF_space_size(hdr, dxpl_id, &meta_size) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTGET, FAIL, "can't retrieve FS meta storage info")
	*heap_size += meta_size;
    } /* end if */

done:
    /* Release resources */
    if(bt2 && H5B2_close(bt2, dxpl_id) < 0)
        HDONE_ERROR(H5E_HEAP, H5E_CANTCLOSEOBJ, FAIL, "can't close v2 B-tree for tracking 'huge' objects")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_size() */

