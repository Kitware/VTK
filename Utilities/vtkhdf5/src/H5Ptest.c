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
 *              Saturday May 31, 2003
 *
 * Purpose:	Generic Property Testing Functions
 */

#define H5P_PACKAGE		/*suppress error about including H5Ppkg	  */
#define H5P_TESTING		/*suppress warning about H5P testing funcs*/


/* Private header files */
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5Ppkg.h"		/* Property lists		  	*/
#include "H5Dprivate.h"		/* Dataset		  		*/

/* Local variables */

/* Local typedefs */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_class_path_test
 PURPOSE
    Routine to query the full path of a generic property list class
 USAGE
    char *H5P_get_class_name_test(pclass_id)
        hid_t pclass_id;         IN: Property class to query
 RETURNS
    Success: Pointer to a malloc'ed string containing the full path of class
    Failure: NULL
 DESCRIPTION
        This routine retrieves the full path name of a generic property list
    class, starting with the root of the class hierarchy.
    The pointer to the name must be free'd by the user for successful calls.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    DO NOT USE THIS FUNCTION FOR ANYTHING EXCEPT TESTING H5P_get_class_path()
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
char *
H5P_get_class_path_test(hid_t pclass_id)
{
    H5P_genclass_t	*pclass;    /* Property class to query */
    char *ret_value;       /* return value */

    FUNC_ENTER_NOAPI(H5P_get_class_path_test, NULL)

    /* Check arguments. */
    if(NULL == (pclass = (H5P_genclass_t *)H5I_object_verify(pclass_id, H5I_GENPROP_CLS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a property class");

    /* Get the property list class path */
    if(NULL == (ret_value = H5P_get_class_path(pclass)))
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, NULL, "unable to query full path of class")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_get_class_path_test() */


/*--------------------------------------------------------------------------
 NAME
    H5P_open_class_path_test
 PURPOSE
    Routine to open a [copy of] a class with its full path name
 USAGE
    hid_t H5P_open_class_name_test(path)
        const char *path;       IN: Full path name of class to open [copy of]
 RETURNS
    Success: ID of generic property class
    Failure: NULL
 DESCRIPTION
    This routine opens [a copy] of the class indicated by the full path.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    DO NOT USE THIS FUNCTION FOR ANYTHING EXCEPT TESTING H5P_open_class_path()
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5P_open_class_path_test(const char *path)
{
    H5P_genclass_t *pclass=NULL;/* Property class to query */
    hid_t ret_value;            /* Return value */

    FUNC_ENTER_NOAPI(H5P_open_class_path_test, FAIL);

    /* Check arguments. */
    if (NULL == path || *path=='\0')
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid class path");

    /* Open the property list class */
    if ((pclass=H5P_open_class_path(path))==NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "unable to find class with full path");

    /* Get an atom for the class */
    if ((ret_value=H5I_register(H5I_GENPROP_CLS, pclass, TRUE))<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to atomize property list class");

done:
    if(ret_value<0 && pclass)
        H5P_close_class(pclass);

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_open_class_path_test() */


/*--------------------------------------------------------------------------
 NAME
    H5P_reset_external_file_test
 PURPOSE
    Routine to reset external file list
 USAGE
    herr_t H5P_reset_external_file_test(plist)
           hid_t dcpl_id; IN: the property list

 RETURNS
    Non-negative on success/Negative on failure

 PROGRAMMER
    Peter Cao
    April 30, 2007
--------------------------------------------------------------------------*/
herr_t
H5P_reset_external_file_test(hid_t dcpl_id)
{
    H5O_efl_t       efl;                /* External file list */
    H5P_genplist_t *plist;              /* Property list */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5P_reset_external_file_test, FAIL)

    /* Check arguments */
    if(NULL == (plist = (H5P_genplist_t *)H5I_object(dcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset creation property list")

    /* get external file list */
    if(H5P_get(plist, H5D_CRT_EXT_FILE_LIST_NAME, &efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get external file list")

    /* Clean up any values set for the external file-list */
    if(H5O_msg_reset(H5O_EFL_ID, &efl) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTFREE, FAIL, "can't release external file list info")

    /* set external file list */
    if(H5P_set(plist, H5D_CRT_EXT_FILE_LIST_NAME, &efl) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get external file list")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_reset_external_file_test() */

