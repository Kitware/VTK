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
 * Created:	H5Tdeprec.c
 *		April 5 2007
 *		Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:	Deprecated functions from the H5T interface.  These
 *              functions are here for compatibility purposes and may be
 *              removed in the future.  Applications should switch to the
 *              newer APIs.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5T_PACKAGE		/*suppress error about including H5Tpkg   */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5T__init_deprec_interface


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5ACprivate.h"        /* Metadata cache                       */
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FOprivate.h"	/* File objects				*/
#include "H5Iprivate.h"		/* IDs					*/
#include "H5Ppublic.h"		/* Property Lists			*/
#include "H5Tpkg.h"		/* Datatypes				*/


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



/*--------------------------------------------------------------------------
NAME
   H5T__init_deprec_interface -- Initialize interface-specific information
USAGE
    herr_t H5T__init_deprec_interface()
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5T_init() currently).

--------------------------------------------------------------------------*/
static herr_t
H5T__init_deprec_interface(void)
{
    FUNC_ENTER_STATIC_NOERR

    FUNC_LEAVE_NOAPI(H5T_init())
} /* H5T__init_deprec_interface() */


/*--------------------------------------------------------------------------
NAME
   H5T__term_deprec_interface -- Terminate interface
USAGE
    herr_t H5T__term_deprec_interface()
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Terminates interface.  (Just resets H5_interface_initialize_g
    currently).

--------------------------------------------------------------------------*/
herr_t
H5T__term_deprec_interface(void)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Mark closed */
    H5_interface_initialize_g = 0;

    FUNC_LEAVE_NOAPI(0)
} /* H5T__term_deprec_interface() */

#ifndef H5_NO_DEPRECATED_SYMBOLS

/*-------------------------------------------------------------------------
 * Function:	H5Tcommit1
 *
 * Purpose:	Save a transient datatype to a file and turn the type handle
 *		into a named, immutable type.
 *
 * Note:	Deprecated in favor of H5Tcommit2
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, June  1, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tcommit1(hid_t loc_id, const char *name, hid_t type_id)
{
    H5G_loc_t	loc;                    /* Location to create datatype */
    H5T_t	*type;                  /* Datatype for ID */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*si", loc_id, name, type_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
    if(NULL == (type = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

    /* Commit the datatype to the file, using default property list values */
    if(H5T__commit_named(&loc, name, type, H5P_LINK_CREATE_DEFAULT,
            H5P_DATATYPE_CREATE_DEFAULT, H5P_DATATYPE_ACCESS_DEFAULT, H5AC_dxpl_id) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to commit datatype")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tcommit1() */


/*-------------------------------------------------------------------------
 * Function:	H5Topen1
 *
 * Purpose:	Opens a named datatype.
 *
 * Note:	Deprecated in favor of H5Topen2
 *
 * Return:	Success:	Object ID of the named datatype.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Monday, June  1, 1998
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Topen1(hid_t loc_id, const char *name)
{
    H5T_t       *type = NULL;
    H5G_loc_t	 loc;
    H5G_name_t   path;            	/* Datatype group hier. path */
    H5O_loc_t    oloc;            	/* Datatype object location */
    H5O_type_t   obj_type;              /* Type of object at location */
    H5G_loc_t    type_loc;              /* Group object for datatype */
    hbool_t      obj_found = FALSE;     /* Object at 'name' found */
    hid_t        dxpl_id = H5AC_dxpl_id; /* dxpl to use to open datatype */
    hid_t        ret_value = FAIL;

    FUNC_ENTER_API(FAIL)
    H5TRACE2("i", "i*s", loc_id, name);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Set up datatype location to fill in */
    type_loc.oloc = &oloc;
    type_loc.path = &path;
    H5G_loc_reset(&type_loc);

    /*
     * Find the named datatype object header and read the datatype message
     * from it.
     */
    if(H5G_loc_find(&loc, name, &type_loc/*out*/, H5P_DEFAULT, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_NOTFOUND, FAIL, "not found")
    obj_found = TRUE;

    /* Check that the object found is the correct type */
    if(H5O_obj_type(&oloc, &obj_type, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't get object type")
    if(obj_type != H5O_TYPE_NAMED_DATATYPE)
        HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a named datatype")

    /* Open it */
    if((type = H5T_open(&type_loc, dxpl_id)) == NULL)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTOPENOBJ, FAIL, "unable to open named datatype")

    /* Register the type and return the ID */
    if((ret_value = H5I_register(H5I_DATATYPE, type, TRUE)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register named datatype")

done:
    if(ret_value < 0) {
        if(type != NULL)
            H5T_close(type);
        else {
            if(obj_found && H5F_addr_defined(type_loc.oloc->addr))
                H5G_loc_free(&type_loc);
        } /* end else */
    } /* end if */

    FUNC_LEAVE_API(ret_value)
} /* end H5Topen1() */
#endif /* H5_NO_DEPRECATED_SYMBOLS */

