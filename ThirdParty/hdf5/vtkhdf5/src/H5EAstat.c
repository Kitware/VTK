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
 * Created:		H5EAstat.c
 *			Sep 11 2008
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:	        Extensible array metadata statistics functions.
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
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5EApkg.h"		/* Extensible Arrays			*/


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
 * Function:	H5EA_get_stats
 *
 * Purpose:	Query the metadata stats of an array
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Aug 21 2008
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PRIV, NOERR,
herr_t, SUCCEED, -,
H5EA_get_stats(const H5EA_t *ea, H5EA_stat_t *stats))

    /* Local variables */

#ifdef QAK
HDfprintf(stderr, "%s: Called\n", FUNC);
#endif /* QAK */

    /*
     * Check arguments.
     */
    HDassert(ea);
    HDassert(stats);

    /* Copy extensible array statistics */
    HDmemcpy(stats, &ea->hdr->stats, sizeof(ea->hdr->stats));

END_FUNC(PRIV)  /* end H5EA_get_stats() */

