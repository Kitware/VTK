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
 * Created:		H5FSdbg.c
 *			May  9 2006
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Dump debugging information about a free space manager
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5FS_PACKAGE		/*suppress error about including H5FSpkg  */
#define H5HF_DEBUGGING          /* Need access to fractal heap debugging routines */
#define H5MF_DEBUGGING          /* Need access to file space debugging routines */

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FSpkg.h"		/* File free space			*/
#include "H5HFprivate.h"	/* Fractal heaps			*/
#include "H5MFprivate.h"	/* File memory management		*/

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
 * Function:	H5FS_debug
 *
 * Purpose:	Prints debugging info about a free space manager.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May  9 2006
 *
 * Modifications:
 *	Vailin Choi, July 29th, 2008
 *	  Add H5FS_CLIENT_FILE_ID for File Memory Management
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_debug(H5F_t *f, hid_t dxpl_id, haddr_t addr, FILE *stream, int indent, int fwidth)
{
    H5FS_t	*fspace = NULL;         /* Free space header info */
    H5FS_hdr_cache_ud_t cache_udata;    /* User-data for cache callback */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    /* Initialize user data for protecting the free space manager */
    cache_udata.f = f;
    cache_udata.nclasses = 0;
    cache_udata.classes = NULL;
    cache_udata.cls_init_udata = NULL;
    cache_udata.addr = addr;

    /*
     * Load the free space header.
     */
    if(NULL == (fspace = (H5FS_t *)H5AC_protect(f, dxpl_id, H5AC_FSPACE_HDR, addr, &cache_udata, H5AC_READ)))
	HGOTO_ERROR(H5E_FSPACE, H5E_CANTLOAD, FAIL, "unable to load free space header")

    /* Print opening message */
    HDfprintf(stream, "%*sFree Space Header...\n", indent, "");

    /*
     * Print the values.
     */
    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
	      "Free space client:",
	      (fspace->client == H5FS_CLIENT_FHEAP_ID ? "Fractal heap" :
	      (fspace->client == H5FS_CLIENT_FILE_ID ? "File" : "Unknown")));
    HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
	      "Total free space tracked:",
	      fspace->tot_space);
    HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
	      "Total number of free space sections tracked:",
	      fspace->tot_sect_count);
    HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
	      "Number of serializable free space sections tracked:",
	      fspace->serial_sect_count);
    HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
	      "Number of ghost free space sections tracked:",
	      fspace->ghost_sect_count);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Number of free space section classes:",
	      fspace->nclasses);
    HDfprintf(stream, "%*s%-*s %u%%\n", indent, "", fwidth,
	      "Shrink percent:",
	      fspace->shrink_percent);
    HDfprintf(stream, "%*s%-*s %u%%\n", indent, "", fwidth,
	      "Expand percent:",
	      fspace->expand_percent);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "# of bits for section address space:",
	      fspace->max_sect_addr);
    HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
	      "Maximum section size:",
	      fspace->max_sect_size);
    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
	      "Serialized sections address:",
	      fspace->sect_addr);
    HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
	      "Serialized sections size used:",
	      fspace->sect_size);
    HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
	      "Serialized sections size allocated:",
	      fspace->alloc_sect_size);

done:
    if(fspace && H5AC_unprotect(f, dxpl_id, H5AC_FSPACE_HDR, addr, fspace, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_FSPACE, H5E_PROTECT, FAIL, "unable to release free space header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FS_debug() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_sect_debug
 *
 * Purpose:	Prints debugging info about a free space section.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May 30 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_sect_debug(const H5FS_t *fspace, const H5FS_section_info_t *sect, FILE *stream, int indent, int fwidth)
{
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /*
     * Check arguments.
     */
    HDassert(fspace);
    HDassert(sect);
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    /* Call the section's debugging routine */
    if(fspace->sect_cls[sect->type].debug)
        if((fspace->sect_cls[sect->type].debug)(sect, stream, indent, fwidth) < 0)
            HGOTO_ERROR(H5E_FSPACE, H5E_BADITER, FAIL, "can't dump section's debugging info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FS_sect_debug() */


/*-------------------------------------------------------------------------
 * Function:	H5FS_sects_debug
 *
 * Purpose:	Prints debugging info about the free space sections.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		May  9 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_sects_debug(H5F_t *f, hid_t dxpl_id, haddr_t UNUSED addr, FILE *stream, int indent, int fwidth,
    haddr_t fs_addr, haddr_t client_addr)
{
    H5FS_t	*fspace = NULL;         /* Free space header info */
    H5FS_client_t client;               /* The client of the free space */
    H5FS_hdr_cache_ud_t cache_udata;    /* User-data for cache callback */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);
    HDassert(H5F_addr_defined(fs_addr));
    HDassert(H5F_addr_defined(client_addr));

    /* Initialize user data for protecting the free space manager */
    cache_udata.f = f;
    cache_udata.nclasses = 0;
    cache_udata.classes = NULL;
    cache_udata.cls_init_udata = NULL;
    cache_udata.addr = fs_addr;

    /*
     * Load the free space header.
     */
    if(NULL == (fspace = (H5FS_t *)H5AC_protect(f, dxpl_id, H5AC_FSPACE_HDR, fs_addr, &cache_udata, H5AC_READ)))
	HGOTO_ERROR(H5E_FSPACE, H5E_CANTLOAD, FAIL, "unable to load free space header")

    /* Retrieve the client id */
    client = fspace->client;

    /* Release the free space header */
    /* (set the "deleted" flag for the unprotect, so the cache entry is removed
     *  and reloaded later, with the correct client information -QAK)
     */
    if(H5AC_unprotect(f, dxpl_id, H5AC_FSPACE_HDR, fs_addr, fspace, H5AC__DELETED_FLAG) < 0)
        HDONE_ERROR(H5E_FSPACE, H5E_PROTECT, FAIL, "unable to release free space header")
    fspace = NULL;

    /* Print opening message */
    HDfprintf(stream, "%*sFree Space Sections...\n", indent, "");

    /*
     * Print the values.
     */
    switch(client) {
        case H5FS_CLIENT_FHEAP_ID:
            if(H5HF_sects_debug(f, dxpl_id, client_addr, stream, indent + 3, MAX(0, fwidth - 3)) < 0)
                HGOTO_ERROR(H5E_FSPACE, H5E_SYSTEM, FAIL, "unable to dump fractal heap free space sections")
            break;

        case H5FS_CLIENT_FILE_ID:
#ifdef NOT_YET
            if(H5MF_sects_debug(f, dxpl_id, fs_addr, stream, indent + 3, MAX(0, fwidth - 3)) < 0)
                HGOTO_ERROR(H5E_FSPACE, H5E_SYSTEM, FAIL, "unable to dump file free space sections")
#endif /* NOT_YET */
            break;

        case H5FS_NUM_CLIENT_ID:
        default:
            HDfprintf(stream, "Unknown client!\n");
            break;
    } /* end switch */

done:
    if(fspace && H5AC_unprotect(f, dxpl_id, H5AC_FSPACE_HDR, fs_addr, fspace, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_FSPACE, H5E_PROTECT, FAIL, "unable to release free space header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FS_sects_debug() */

