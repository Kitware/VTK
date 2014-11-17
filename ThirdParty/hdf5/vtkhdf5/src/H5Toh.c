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

#define H5O_PACKAGE		/*suppress error about including H5Opkg	  */
#define H5T_PACKAGE		/*suppress error about including H5Tpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5Opkg.h"             /* Object headers			*/
#include "H5Tpkg.h"		/* Datatypes				*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Local Prototypes */
/********************/

static htri_t H5O_dtype_isa(H5O_t *loc);
static hid_t H5O_dtype_open(const H5G_loc_t *obj_loc, hid_t lapl_id,
    hid_t dxpl_id, hbool_t app_ref);
static void *H5O_dtype_create(H5F_t *f, void *_crt_info, H5G_loc_t *obj_loc,
    hid_t dxpl_id);
static H5O_loc_t *H5O_dtype_get_oloc(hid_t obj_id);


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

/* This message derives from H5O object class */
const H5O_obj_class_t H5O_OBJ_DATATYPE[1] = {{
    H5O_TYPE_NAMED_DATATYPE,	/* object type			*/
    "named datatype",		/* object name, for debugging	*/
    NULL,			/* get 'copy file' user data	*/
    NULL,			/* free 'copy file' user data	*/
    H5O_dtype_isa, 		/* "isa" 			*/
    H5O_dtype_open, 		/* open an object of this class */
    H5O_dtype_create, 		/* create an object of this class */
    H5O_dtype_get_oloc,		/* get an object header location for an object */
    NULL,			/* get the index & heap info for an object */
    NULL 			/* flush an opened object of this class */
}};


/*-------------------------------------------------------------------------
 * Function:	H5O_dtype_isa
 *
 * Purpose:	Determines if an object has the requisite messages for being
 *		a datatype.
 *
 * Return:	Success:	TRUE if the required data type messages are
 *				present; FALSE otherwise.
 *
 *		Failure:	FAIL if the existence of certain messages
 *				cannot be determined.
 *
 * Programmer:	Robb Matzke
 *              Monday, November  2, 1998
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5O_dtype_isa(struct H5O_t *oh)
{
    htri_t	ret_value;              /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(oh);

    if((ret_value = H5O_msg_exists_oh(oh, H5O_DTYPE_ID)) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to read object header")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dtype_isa() */


/*-------------------------------------------------------------------------
 * Function:	H5O_dtype_open
 *
 * Purpose:	Open a datatype at a particular location
 *
 * Return:	Success:	Open object identifier
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, November  6, 2006
 *
 *-------------------------------------------------------------------------
 */
static hid_t
H5O_dtype_open(const H5G_loc_t *obj_loc, hid_t UNUSED lapl_id, hid_t dxpl_id, hbool_t app_ref)
{
    H5T_t       *type = NULL;           /* Datatype opened */
    hid_t	ret_value;              /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(obj_loc);

    /* Open the datatype */
    if(NULL == (type = H5T_open(obj_loc, dxpl_id)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTOPENOBJ, FAIL, "unable to open datatype")

    /* Register an ID for the datatype */
    if((ret_value = H5I_register(H5I_DATATYPE, type, app_ref)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register datatype")

done:
    if(ret_value < 0)
        if(type && H5T_close(type) < 0)
            HDONE_ERROR(H5E_DATATYPE, H5E_CLOSEERROR, FAIL, "unable to release datatype")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dtype_open() */


/*-------------------------------------------------------------------------
 * Function:	H5O_dtype_create
 *
 * Purpose:	Create a named datatype in a file
 *
 * Return:	Success:	Pointer to the named datatype data structure
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, April 11, 2007
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_dtype_create(H5F_t *f, void *_crt_info, H5G_loc_t *obj_loc, hid_t dxpl_id)
{
    H5T_obj_create_t *crt_info = (H5T_obj_create_t *)_crt_info; /* Named datatype creation parameters */
    void *ret_value;            /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity checks */
    HDassert(f);
    HDassert(crt_info);
    HDassert(obj_loc);

    /* Commit the type to the file */
    if(H5T__commit(f, crt_info->dt, crt_info->tcpl_id, dxpl_id) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to commit datatype")

    /* Set up the new named datatype's location */
    if(NULL == (obj_loc->oloc = H5T_oloc(crt_info->dt)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "unable to get object location of named datatype")
    if(NULL == (obj_loc->path = H5T_nameof(crt_info->dt)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "unable to get path of named datatype")

    /* Set the return value */
    ret_value = crt_info->dt;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dtype_create() */


/*-------------------------------------------------------------------------
 * Function:	H5O_dtype_get_oloc
 *
 * Purpose:	Retrieve the object header location for an open object
 *
 * Return:	Success:	Pointer to object header location
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Monday, November  6, 2006
 *
 *-------------------------------------------------------------------------
 */
static H5O_loc_t *
H5O_dtype_get_oloc(hid_t obj_id)
{
    H5T_t       *type;                  /* Datatype opened */
    H5O_loc_t	*ret_value;             /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Get the datatype */
    if(NULL == (type = (H5T_t *)H5I_object(obj_id)))
        HGOTO_ERROR(H5E_OHDR, H5E_BADATOM, NULL, "couldn't get object from ID")

    /* Get the datatype's object header location */
    if(NULL == (ret_value = H5T_oloc(type)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, NULL, "unable to get object location from object")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dtype_get_oloc() */

