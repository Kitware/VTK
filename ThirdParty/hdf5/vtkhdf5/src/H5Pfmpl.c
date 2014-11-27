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
 * Created:		H5Pmtpl.c
 *			November  1 2006
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		File mount property list class routines
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/
#define H5P_PACKAGE		/*suppress error about including H5Ppkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5Ppkg.h"		/* Property lists		  	*/


/****************/
/* Local Macros */
/****************/

/* ======================== File Mount properties ====================*/
/* Definition for whether absolute symlinks local to file. */
#define H5F_MNT_SYM_LOCAL_SIZE		sizeof(hbool_t)
#define H5F_MNT_SYM_LOCAL_DEF	 	FALSE


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

/* Property class callbacks */
static herr_t H5P_fmnt_reg_prop(H5P_genclass_t *pclass);


/*********************/
/* Package Variables */
/*********************/

/* File mount property list class library initialization object */
const H5P_libclass_t H5P_CLS_FMNT[1] = {{
    "file mount",		/* Class name for debugging     */
    H5P_TYPE_FILE_MOUNT,        /* Class type                   */
    &H5P_CLS_ROOT_g,		/* Parent class ID              */
    &H5P_CLS_FILE_MOUNT_g,	/* Pointer to class ID          */
    &H5P_LST_FILE_MOUNT_g,	/* Pointer to default property list ID */
    H5P_fmnt_reg_prop,		/* Default property registration routine */
    NULL,		        /* Class creation callback      */
    NULL,		        /* Class creation callback info */
    NULL,			/* Class copy callback          */
    NULL,		        /* Class copy callback info     */
    NULL,			/* Class close callback         */
    NULL 		        /* Class close callback info    */
}};


/*****************************/
/* Library Private Variables */
/*****************************/



/*-------------------------------------------------------------------------
 * Function:    H5P_fmnt_reg_prop
 *
 * Purpose:     Register the file mount property list class's properties
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              October 31, 2006
 *-------------------------------------------------------------------------
 */
static herr_t
H5P_fmnt_reg_prop(H5P_genclass_t *pclass)
{
    hbool_t local = H5F_MNT_SYM_LOCAL_DEF;      /* Whether symlinks are local to file */
    herr_t ret_value = SUCCEED;                 /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Register property of whether symlinks is local to file */
    if(H5P_register_real(pclass, H5F_MNT_SYM_LOCAL_NAME, H5F_MNT_SYM_LOCAL_SIZE, &local, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_fmnt_reg_prop() */

