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
 * Purpose:	v2 B-tree metadata statistics functions.
 *
 */

/****************/
/* Module Setup */
/****************/

#define H5B2_PACKAGE		/*suppress error about including H5B2pkg  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5B2pkg.h"		/* v2 B-trees				*/
#include "H5Eprivate.h"		/* Error handling		  	*/


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
 * Function:	H5B2_stat_info
 *
 * Purpose:	Retrieve metadata statistics for a v2 B-tree
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, March  6, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_stat_info(H5B2_t *bt2, H5B2_stat_t *info)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5B2_stat_info)

    /* Check arguments. */
    HDassert(info);

    /* Get information about the B-tree */
    info->depth = bt2->hdr->depth;
    info->nrecords = bt2->hdr->root.all_nrec;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5B2_stat_info() */


/*-------------------------------------------------------------------------
 * Function:    H5B2_size
 *
 * Purpose:     Iterate over all the records in the B-tree, collecting
 *              storage info.
 *
 * Return:      non-negative on success, negative on error
 *
 * Programmer:  Vailin Choi
 *              June 19 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_size(H5B2_t *bt2, hid_t dxpl_id, hsize_t *btree_size)
{
    H5B2_hdr_t	*hdr;                   /* Pointer to the B-tree header */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5B2_size, FAIL)

    /* Check arguments. */
    HDassert(bt2);
    HDassert(btree_size);

    /* Set the shared v2 B-tree header's file context for this operation */
    bt2->hdr->f = bt2->f;

    /* Get the v2 B-tree header */
    hdr = bt2->hdr;

    /* Add size of header to B-tree metadata total */
    *btree_size += H5B2_HEADER_SIZE(hdr);

    /* Iterate through records */
    if(hdr->root.node_nrec > 0) {
        /* Check for root node being a leaf */
        if(hdr->depth == 0)
            *btree_size += hdr->node_size;
        else
            /* Iterate through nodes */
            if(H5B2_node_size(hdr, dxpl_id, hdr->depth, &hdr->root, btree_size) < 0)
                HGOTO_ERROR(H5E_BTREE, H5E_CANTLIST, FAIL, "node iteration failed")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5B2_size() */

