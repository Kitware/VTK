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
 * Purpose:	Free-space manager testing functions.
 *
 */

/****************/
/* Module Setup */
/****************/

#define H5FS_PACKAGE		/*suppress error about including H5FSpkg  */
#define H5FS_TESTING		/*suppress warning about H5FS testing funcs */

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FSpkg.h"		/* Free-space manager			*/

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
 * Function:    H5FS_get_cparam_test
 *
 * Purpose:     Retrieve the parameters used to create the free-space manager
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  similar to H5HF_get_cparam_test()
 *		Vailin Choi; August 25th, 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_get_cparam_test(const H5FS_t *frsp, H5FS_create_t *cparam)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FS_get_cparam_test)

    /* Check arguments. */
    HDassert(frsp);
    HDassert(cparam);

    cparam->client = frsp->client;
    cparam->shrink_percent = frsp->shrink_percent;
    cparam->expand_percent = frsp->expand_percent;
    cparam->max_sect_addr = frsp->max_sect_addr;
    cparam->max_sect_size = frsp->max_sect_size;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5FS_get_cparam_test() */

/*-------------------------------------------------------------------------
 * Function:    H5FS_cmp_cparam_test
 *
 * Purpose:     Compare the parameters used to create the fractal heap
 *
 * Return:      Success:        non-negative
 *              Failure:        negative
 *
 * Programmer:  similar to H5HF_cmp_cparam_test()
 *		Vailin Choi; August 25th, 2008
 *
 *-------------------------------------------------------------------------
 */
int
H5FS_cmp_cparam_test(const H5FS_create_t *cparam1, const H5FS_create_t *cparam2)
{
    int ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5FS_cmp_cparam_test)

    /* Check arguments. */
    HDassert(cparam1);
    HDassert(cparam2);

    if(cparam1->client < cparam2->client)
        HGOTO_DONE(-1)
    else if(cparam1->client > cparam2->client)
        HGOTO_DONE(1)

    if(cparam1->shrink_percent < cparam2->shrink_percent)
        HGOTO_DONE(-1)
    else if(cparam1->shrink_percent > cparam2->shrink_percent)
        HGOTO_DONE(1)

    if(cparam1->expand_percent < cparam2->expand_percent)
        HGOTO_DONE(-1)
    else if(cparam1->expand_percent > cparam2->expand_percent)
        HGOTO_DONE(1)

    if(cparam1->max_sect_size < cparam2->max_sect_size)
        HGOTO_DONE(-1)
    else if(cparam1->max_sect_size > cparam2->max_sect_size)
        HGOTO_DONE(1)

    if(cparam1->max_sect_addr < cparam2->max_sect_addr)
        HGOTO_DONE(-1)
    else if(cparam1->max_sect_addr > cparam2->max_sect_addr)
        HGOTO_DONE(1)

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5FS_cmp_cparam_test */
