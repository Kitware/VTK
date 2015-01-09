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

/*
 * Purpose:	Free-space metadata statistics functions.
 *
 */

/****************/
/* Module Setup */
/****************/

#define H5FS_PACKAGE		/*suppress error about including H5FSpkg  */

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FSpkg.h"		/* Free-space manager 			*/

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
 * Function:    H5FS_stat_info
 *
 * Purpose:     Retrieve metadata statistics for the free-space manager
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        does not fail
 *
 * Programmer:  Vailin Choi
 *		August 25th, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_stat_info(const H5F_t *f, const H5FS_t *frsp, H5FS_stat_t *stats)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check arguments. */
    HDassert(frsp);
    HDassert(stats);

    /* Report statistics for free space */
    stats->tot_space = frsp->tot_space;
    stats->tot_sect_count = frsp->tot_sect_count;
    stats->serial_sect_count = frsp->serial_sect_count;
    stats->ghost_sect_count = frsp->ghost_sect_count;
    stats->addr = frsp->addr;
    stats->hdr_size = (size_t)H5FS_HEADER_SIZE(f);
    stats->sect_addr = frsp->sect_addr;
    stats->alloc_sect_size = frsp->alloc_sect_size;
    stats->sect_size = frsp->sect_size;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5FS_stat_info() */

