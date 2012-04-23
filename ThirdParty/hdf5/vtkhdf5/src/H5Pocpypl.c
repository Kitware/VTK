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
 * Created:		H5Pocpypl.c
 *			Mar 13 2006
 *			Peter Cao <xcao@ncsa.uiuc.edu>
 *
 * Purpose:		Object copying property list class routines
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

/* ========= Object Copy properties ============ */
/* Definitions for copy options */
#define H5O_CPY_OPTION_SIZE			sizeof(unsigned)
#define H5O_CPY_OPTION_DEF			0


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
static herr_t H5P_ocpy_reg_prop(H5P_genclass_t *pclass);


/*********************/
/* Package Variables */
/*********************/

/* Object copy property list class library initialization object */
const H5P_libclass_t H5P_CLS_OCPY[1] = {{
    "object copy",		/* Class name for debugging     */
    &H5P_CLS_ROOT_g,		/* Parent class ID              */
    &H5P_CLS_OBJECT_COPY_g,	/* Pointer to class ID          */
    &H5P_LST_OBJECT_COPY_g,	/* Pointer to default property list ID */
    H5P_ocpy_reg_prop,		/* Default property registration routine */
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


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:    H5P_ocpy_reg_prop
 *
 * Purpose:     Initialize the object copy property list class
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              October 31, 2006
 *-------------------------------------------------------------------------
 */
herr_t
H5P_ocpy_reg_prop(H5P_genclass_t *pclass)
{
    unsigned ocpy_option = H5O_CPY_OPTION_DEF;  /* Default object copy flags */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5P_ocpy_reg_prop, FAIL)

    /* Register copy options property */
    if(H5P_register_real(pclass, H5O_CPY_OPTION_NAME, H5O_CPY_OPTION_SIZE, &ocpy_option, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_ocpy_reg_prop() */


/*-------------------------------------------------------------------------
 * Function:    H5Pset_copy_object
 *
 * Purpose:     Set properties when copying an object (group, dataset, and datatype)
 *              from one location to another
 *
 * Usage:       H5Pset_copy_group(plist_id, cpy_option)
 *              hid_t plist_id;			IN: Property list to copy object
 *              unsigned cpy_option; 		IN: Options to copy object such as
 *                  H5O_COPY_SHALLOW_HIERARCHY_FLAG    -- Copy only immediate members
 *                  H5O_COPY_EXPAND_SOFT_LINK_FLAG     -- Expand soft links into new objects/
 *                  H5O_COPY_EXPAND_EXT_LINK_FLAG      -- Expand external links into new objects
 *                  H5O_COPY_EXPAND_REFERENCE_FLAG -- Copy objects that are pointed by references
 *                  H5O_COPY_WITHOUT_ATTR_FLAG         -- Copy object without copying attributes
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Peter Cao
 *              March 13, 2006
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_copy_object(hid_t plist_id, unsigned cpy_option)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(H5Pset_copy_object, FAIL)
    H5TRACE2("e", "iIu", plist_id, cpy_option);

    /* Check parameters */
    if(cpy_option & ~H5O_COPY_ALL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unknown option specified")

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_OBJECT_COPY)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Set value */
    if(H5P_set(plist, H5O_CPY_OPTION_NAME, &cpy_option) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set copy object flag")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_copy_object() */


/*-------------------------------------------------------------------------
 * Function:    H5Pget_copy_object
 *
 * Purpose:     Returns the cpy_option, which is set for H5Ocopy(hid_t loc_id,
 *              const char* name, ... ) for copying objects
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Peter Cao
 *              March 13, 2006
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_copy_object(hid_t plist_id, unsigned *cpy_option /*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED; /* return value */

    FUNC_ENTER_API(H5Pget_copy_object, FAIL)
    H5TRACE2("e", "ix", plist_id, cpy_option);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_OBJECT_COPY)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get values */
    if(cpy_option)
        if(H5P_get(plist, H5O_CPY_OPTION_NAME, cpy_option) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get object copy flag")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_copy_object() */

