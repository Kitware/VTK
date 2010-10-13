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

/****************/
/* Module Setup */
/****************/

#define H5F_PACKAGE		/*suppress error about including H5Fpkg 	  */
#define H5SM_PACKAGE		/*suppress error about including H5SMpkg	  */
#define H5SM_TESTING		/*suppress warning about H5SM testing funcs*/


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5ACprivate.h"	/* Metadata cache			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"		/* File access                          */
#include "H5SMpkg.h"            /* Shared object header messages        */


/****************/
/* Local Macros */
/****************/


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
 * Function:    H5SM_get_mesg_count_test
 *
 * Purpose:     Retrieve the number of messages tracked of a certain type
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, January  3, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5SM_get_mesg_count_test(H5F_t *f, hid_t dxpl_id, unsigned type_id,
    size_t *mesg_count)
{
    H5SM_master_table_t *table = NULL;  /* SOHM master table */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5SM_get_mesg_count_test)

    /* Sanity check */
    HDassert(f);
    HDassert(mesg_count);

    /* Check for shared messages being enabled */
    if(H5F_addr_defined(f->shared->sohm_addr)) {
        H5SM_index_header_t *header;        /* Index header for message type */
        ssize_t index_num;                  /* Table index for message type */

        /* Look up the master SOHM table */
        if(NULL == (table = (H5SM_master_table_t *)H5AC_protect(f, dxpl_id, H5AC_SOHM_TABLE, f->shared->sohm_addr, f, H5AC_READ)))
            HGOTO_ERROR(H5E_SOHM, H5E_CANTPROTECT, FAIL, "unable to load SOHM master table")

        /* Find the correct index for this message type */
        if((index_num = H5SM_get_index(table, type_id)) < 0)
            HGOTO_ERROR(H5E_SOHM, H5E_NOTFOUND, FAIL, "unable to find correct SOHM index")
        header = &(table->indexes[index_num]);

        /* Set the message count for the type */
        *mesg_count = header->num_messages;
    } /* end if */
    else
        /* No shared messages of any type */
        *mesg_count = 0;

done:
    /* Release resources */
    if(table && H5AC_unprotect(f, dxpl_id, H5AC_SOHM_TABLE, f->shared->sohm_addr, table, H5AC__NO_FLAGS_SET) < 0)
	HDONE_ERROR(H5E_SOHM, H5E_CANTUNPROTECT, FAIL, "unable to close SOHM master table")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5SM_get_mesg_count_test() */

