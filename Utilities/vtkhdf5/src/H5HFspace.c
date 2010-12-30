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

/*-------------------------------------------------------------------------
 *
 * Created:		H5HFspace.c
 *			May  2 2006
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Space allocation routines for fractal heaps.
 *
 *-------------------------------------------------------------------------
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

#define H5HF_FSPACE_SHRINK      80              /* Percent of "normal" size to shrink serialized free space size */
#define H5HF_FSPACE_EXPAND      120             /* Percent of "normal" size to expand serialized free space size */
#define H5HF_FSPACE_THRHD_DEF 	1             	/* Default: no alignment threshold */
#define H5HF_FSPACE_ALIGN_DEF   1             	/* Default: no alignment */

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
 * Function:	H5HF_space_start
 *
 * Purpose:	"Start up" free space for heap - open existing free space
 *              structure if one exists, otherwise create a new free space
 *              structure
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May  2 2006
 *
 * Modifications:
 *	Vailin Choi, July 29th, 2008
 *	  Pass values of alignment and threshold to FS_create() and FS_open()
 *	  for handling alignment.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_space_start(H5HF_hdr_t *hdr, hid_t dxpl_id, hbool_t may_create)
{
    const H5FS_section_class_t *classes[] = { /* Free space section classes implemented for fractal heap */
        H5HF_FSPACE_SECT_CLS_SINGLE,
        H5HF_FSPACE_SECT_CLS_FIRST_ROW,
        H5HF_FSPACE_SECT_CLS_NORMAL_ROW,
        H5HF_FSPACE_SECT_CLS_INDIRECT};
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_space_start)

    /*
     * Check arguments.
     */
    HDassert(hdr);

    /* Check for creating free space info for the heap */
    if(H5F_addr_defined(hdr->fs_addr)) {
        /* Open an existing free space structure for the heap */
        if(NULL == (hdr->fspace = H5FS_open(hdr->f, dxpl_id, hdr->fs_addr,
                NELMTS(classes), classes, hdr, H5HF_FSPACE_THRHD_DEF, H5HF_FSPACE_ALIGN_DEF)))
            HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, FAIL, "can't initialize free space info")
    } /* end if */
    else {
        /* Check if we are allowed to create the free space manager */
        if(may_create) {
            H5FS_create_t fs_create;        /* Free space creation parameters */

            /* Set the free space creation parameters */
            fs_create.client = H5FS_CLIENT_FHEAP_ID;
            fs_create.shrink_percent = H5HF_FSPACE_SHRINK;
            fs_create.expand_percent = H5HF_FSPACE_EXPAND;
            fs_create.max_sect_size = hdr->man_dtable.cparam.max_direct_size;
            fs_create.max_sect_addr = hdr->man_dtable.cparam.max_index;

            /* Create the free space structure for the heap */
            if(NULL == (hdr->fspace = H5FS_create(hdr->f, dxpl_id, &hdr->fs_addr,
                    &fs_create, NELMTS(classes), classes, hdr, H5HF_FSPACE_THRHD_DEF, H5HF_FSPACE_ALIGN_DEF)))
                HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, FAIL, "can't initialize free space info")
            HDassert(H5F_addr_defined(hdr->fs_addr));
        } /* end if */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_space_start() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_space_add
 *
 * Purpose:	Add a section to the free space for the heap
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May 15 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_space_add(H5HF_hdr_t *hdr, hid_t dxpl_id, H5HF_free_section_t *node,
    unsigned flags)
{
    H5HF_sect_add_ud1_t udata;          /* User data for free space manager 'add' */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_space_add)

    /*
     * Check arguments.
     */
    HDassert(hdr);
    HDassert(node);

    /* Check if the free space for the heap has been initialized */
    if(!hdr->fspace)
        if(H5HF_space_start(hdr, dxpl_id, TRUE) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, FAIL, "can't initialize heap free space")

    /* Construct user data */
    udata.hdr = hdr;
    udata.dxpl_id = dxpl_id;

    /* Add to the free space for the heap */
    if(H5FS_sect_add(hdr->f, dxpl_id, hdr->fspace, (H5FS_section_info_t *)node, flags, &udata) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTINSERT, FAIL, "can't add section to heap free space")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_space_add() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_space_find
 *
 * Purpose:	Attempt to find space in a fractal heap
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May  2 2006
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5HF_space_find(H5HF_hdr_t *hdr, hid_t dxpl_id, hsize_t request, H5HF_free_section_t **node)
{
    htri_t node_found = FALSE;  /* Whether an existing free list node was found */
    htri_t ret_value;           /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_space_find)

    /*
     * Check arguments.
     */
    HDassert(hdr);
    HDassert(request);
    HDassert(node);

    /* Check if the free space for the heap has been initialized */
    if(!hdr->fspace)
        if(H5HF_space_start(hdr, dxpl_id, FALSE) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, FAIL, "can't initialize heap free space")

    /* Search for free space in the heap */
    if(hdr->fspace)
        if((node_found = H5FS_sect_find(hdr->f, dxpl_id, hdr->fspace, request, (H5FS_section_info_t **)node)) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTALLOC, FAIL, "can't locate free space in fractal heap")

    /* Set return value */
    ret_value = node_found;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_space_find() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_space_size
 *
 * Purpose:	Query the size of the heap's free space info on disk
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		August 14 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_space_size(H5HF_hdr_t *hdr, hid_t dxpl_id, hsize_t *fs_size)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_space_size)

    /*
     * Check arguments.
     */
    HDassert(hdr);
    HDassert(fs_size);

    /* Check if the free space for the heap has been initialized */
    if(!hdr->fspace)
        if(H5HF_space_start(hdr, dxpl_id, FALSE) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTINIT, FAIL, "can't initialize heap free space")

    /* Get free space metadata size */
    if(hdr->fspace) {
        if(H5FS_size(hdr->f, hdr->fspace, fs_size) < 0)
            HGOTO_ERROR(H5E_FSPACE, H5E_CANTGET, FAIL, "can't retrieve FS meta storage info")
    } /* end if */
    else
        *fs_size = 0;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_space_size() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_space_remove
 *
 * Purpose:	Remove a section from the free space for the heap
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		July 24 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_space_remove(H5HF_hdr_t *hdr, hid_t dxpl_id, H5HF_free_section_t *node)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_space_remove)

    /*
     * Check arguments.
     */
    HDassert(hdr);
    HDassert(hdr->fspace);
    HDassert(node);

    /* Remove from the free space for the heap */
    if(H5FS_sect_remove(hdr->f, dxpl_id, hdr->fspace, (H5FS_section_info_t *)node) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTREMOVE, FAIL, "can't remove section from heap free space")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_space_remove() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_space_close
 *
 * Purpose:	Close the free space for the heap
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May  2 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_space_close(H5HF_hdr_t *hdr, hid_t dxpl_id)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_space_close)

    /*
     * Check arguments.
     */
    HDassert(hdr);

    /* Check if the free space was ever opened */
    if(hdr->fspace) {
        hsize_t nsects;         /* Number of sections for this heap */

        /* Retrieve the number of sections for this heap */
        if(H5FS_sect_stats(hdr->fspace, NULL, &nsects) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTCOUNT, FAIL, "can't query free space section count")
#ifdef QAK
HDfprintf(stderr, "%s: nsects = %Hu\n", FUNC, nsects);
#endif /* QAK */

        /* Close the free space for the heap */
        if(H5FS_close(hdr->f, dxpl_id, hdr->fspace) < 0)
            HGOTO_ERROR(H5E_HEAP, H5E_CANTRELEASE, FAIL, "can't release free space info")
        hdr->fspace = NULL;

        /* Check if we can delete the free space manager for this heap */
        if(!nsects) {
            if(H5FS_delete(hdr->f, dxpl_id, hdr->fs_addr) < 0)
                HGOTO_ERROR(H5E_HEAP, H5E_CANTDELETE, FAIL, "can't delete free space info")
            hdr->fs_addr = HADDR_UNDEF;
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_space_close() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_space_delete
 *
 * Purpose:	Delete the free space manager for the heap
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Aug  7 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_space_delete(H5HF_hdr_t *hdr, hid_t dxpl_id)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_space_delete)

    /*
     * Check arguments.
     */
    HDassert(hdr);

    /* Delete the free space manager */
    if(H5FS_delete(hdr->f, dxpl_id, hdr->fs_addr) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTFREE, FAIL, "can't delete to free space manager")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_space_delete() */


/*-------------------------------------------------------------------------
 * Function:	H5HF_space_change_sect_class
 *
 * Purpose:	Change a section's class
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		July 10 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5HF_space_sect_change_class(H5HF_hdr_t *hdr, hid_t dxpl_id, H5HF_free_section_t *sect, unsigned new_class)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5HF_space_sect_change_class)
#ifdef QAK
HDfprintf(stderr, "%s: Called\n", FUNC);
#endif /* QAK */

    /*
     * Check arguments.
     */
    HDassert(hdr);
    HDassert(hdr->fspace);
    HDassert(sect);

    /* Notify the free space manager that a section has changed class */
    if(H5FS_sect_change_class(hdr->f, dxpl_id, hdr->fspace, (H5FS_section_info_t *)sect, new_class) < 0)
        HGOTO_ERROR(H5E_HEAP, H5E_CANTMODIFY, FAIL, "can't modify class of free space section")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5HF_space_sect_change_class() */

