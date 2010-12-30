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
 * Module Info: This module contains the functionality for setting & querying
 *      the datatype string padding for the H5T interface.
 */

#define H5T_PACKAGE		/*suppress error about including H5Tpkg	  */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5T_init_strpad_interface


#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5Tpkg.h"		/* Datatypes				*/


/*--------------------------------------------------------------------------
NAME
   H5T_init_strpad_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_strpad_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5T_init_iterface currently).

--------------------------------------------------------------------------*/
static herr_t
H5T_init_strpad_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_init_strpad_interface)

    FUNC_LEAVE_NOAPI(H5T_init())
} /* H5T_init_strpad_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Tget_strpad
 *
 * Purpose:	The method used to store character strings differs with the
 *		programming language: C usually null terminates strings while
 *		Fortran left-justifies and space-pads strings.	This property
 *		defines the storage mechanism for the string.
 *
 * Return:	Success:	The character set of a string type.
 *
 *		Failure:	H5T_STR_ERROR (Negative)
 *
 * Programmer:	Robb Matzke
 *		Friday, January	 9, 1998
 *
 * Modifications:
 * 	Robb Matzke, 22 Dec 1998
 *	Also works for derived datatypes.
 *
 *-------------------------------------------------------------------------
 */
H5T_str_t
H5Tget_strpad(hid_t type_id)
{
    H5T_t	*dt = NULL;
    H5T_str_t	ret_value;

    FUNC_ENTER_API(H5Tget_strpad, H5T_STR_ERROR)
    H5TRACE1("Tz", "i", type_id);

    /* Check args */
    if (NULL == (dt = H5I_object_verify(type_id,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_STR_ERROR, "not a datatype")
    while (dt->shared->parent && !H5T_IS_STRING(dt->shared))
        dt = dt->shared->parent;  /*defer to parent*/
    if (!H5T_IS_STRING(dt->shared))
	HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, H5T_STR_ERROR, "operation not defined for datatype class")

    /* result */
    if(H5T_IS_FIXED_STRING(dt->shared))
        ret_value = dt->shared->u.atomic.u.s.pad;
    else
        ret_value = dt->shared->u.vlen.pad;

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5Tset_strpad
 *
 * Purpose:	The method used to store character strings differs with the
 *		programming language: C usually null terminates strings while
 *		Fortran left-justifies and space-pads strings.	This property
 *		defines the storage mechanism for the string.
 *
 *		When converting from a long string to a short string if the
 *		short string is H5T_STR_NULLPAD or H5T_STR_SPACEPAD then the
 *		string is simply truncated; otherwise if the short string is
 *		H5T_STR_NULLTERM it will be truncated and a null terminator
 *		is appended.
 *
 *		When converting from a short string to a long string, the
 *		long string is padded on the end by appending nulls or
 *		spaces.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Friday, January	 9, 1998
 *
 * Modifications:
 * 	Robb Matzke, 22 Dec 1998
 *	Also works for derived datatypes.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_strpad(hid_t type_id, H5T_str_t strpad)
{
    H5T_t	*dt = NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Tset_strpad, FAIL)
    H5TRACE2("e", "iTz", type_id, strpad);

    /* Check args */
    if (NULL == (dt = H5I_object_verify(type_id,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    if (H5T_STATE_TRANSIENT!=dt->shared->state)
	HGOTO_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "datatype is read-only")
    if (strpad < H5T_STR_NULLTERM || strpad >= H5T_NSTR)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "illegal string pad type")
    while (dt->shared->parent && !H5T_IS_STRING(dt->shared))
        dt = dt->shared->parent;  /*defer to parent*/
    if (!H5T_IS_STRING(dt->shared))
	HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "operation not defined for datatype class")

    /* Commit */
    if(H5T_IS_FIXED_STRING(dt->shared))
        dt->shared->u.atomic.u.s.pad = strpad;
    else
        dt->shared->u.vlen.pad = strpad;

done:
    FUNC_LEAVE_API(ret_value)
}

