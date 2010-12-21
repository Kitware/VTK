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
 *      the character set (cset) for the H5T interface.
 */

#define H5T_PACKAGE		/*suppress error about including H5Tpkg	  */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5T_init_cset_interface


#include "H5private.h"		/*generic functions			  */
#include "H5Eprivate.h"		/*error handling			  */
#include "H5Iprivate.h"		/*ID functions		   		  */
#include "H5Tpkg.h"		/*data-type functions			  */


/*--------------------------------------------------------------------------
NAME
   H5T_init_cset_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_cset_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5T_init_iterface currently).

--------------------------------------------------------------------------*/
static herr_t
H5T_init_cset_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_init_cset_interface)

    FUNC_LEAVE_NOAPI(H5T_init())
} /* H5T_init_cset_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Tget_cset
 *
 * Purpose:	HDF5 is able to distinguish between character sets of
 *		different nationalities and to convert between them to the
 *		extent possible.
 *
 * Return:	Success:	The character set of a string type.
 *
 *		Failure:	H5T_CSET_ERROR (Negative)
 *
 * Programmer:	Robb Matzke
 *		Friday, January	 9, 1998
 *
 * Modifications:
 * 	Robb Matzke, 22 Dec 1998
 *	Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
H5T_cset_t
H5Tget_cset(hid_t type_id)
{
    H5T_t	*dt;
    H5T_cset_t	ret_value;

    FUNC_ENTER_API(H5Tget_cset, H5T_CSET_ERROR)
    H5TRACE1("Tc", "i", type_id);

    /* Check args */
    if (NULL == (dt = (H5T_t *)H5I_object_verify(type_id,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_CSET_ERROR, "not a data type")
    while (dt->shared->parent && !H5T_IS_STRING(dt->shared))
        dt = dt->shared->parent;  /*defer to parent*/
    if (!H5T_IS_STRING(dt->shared))
	HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, H5T_CSET_ERROR, "operation not defined for data type class")

    /* result */
    if(H5T_IS_FIXED_STRING(dt->shared))
        ret_value = dt->shared->u.atomic.u.s.cset;
    else
        ret_value = dt->shared->u.vlen.cset;

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5Tset_cset
 *
 * Purpose:	HDF5 is able to distinguish between character sets of
 *		different nationalities and to convert between them to the
 *		extent possible.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Friday, January	 9, 1998
 *
 * Modifications:
 * 	Robb Matzke, 22 Dec 1998
 *	Also works with derived data types.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_cset(hid_t type_id, H5T_cset_t cset)
{
    H5T_t	*dt;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Tset_cset, FAIL)
    H5TRACE2("e", "iTc", type_id, cset);

    /* Check args */
    if (NULL == (dt = (H5T_t *)H5I_object_verify(type_id,H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type")
    if (H5T_STATE_TRANSIENT!=dt->shared->state)
	HGOTO_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only")
    if (cset < H5T_CSET_ASCII || cset >= H5T_NCSET)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "illegal character set type")
    while (dt->shared->parent && !H5T_IS_STRING(dt->shared))
        dt = dt->shared->parent;  /*defer to parent*/
    if (!H5T_IS_STRING(dt->shared))
	HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "operation not defined for data type class")

    /* Commit */
    if(H5T_IS_FIXED_STRING(dt->shared))
        dt->shared->u.atomic.u.s.cset = cset;
    else
        dt->shared->u.vlen.cset = cset;

done:
    FUNC_LEAVE_API(ret_value)
}

