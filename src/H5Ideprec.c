/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:	H5Ideprec.c
 *
 * Purpose:	Deprecated functions from the H5I interface.  These
 *          functions are here for compatibility purposes and may be
 *          removed in the future.  Applications should switch to the
 *          newer APIs.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5Imodule.h" /* This source code file is part of the H5I module */

/***********/
/* Headers */
/***********/
#include "H5private.h"  /* Generic Functions                        */
#include "H5Eprivate.h" /* Error handling                           */
#include "H5Ipkg.h"     /* File access                              */

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

#ifndef H5_NO_DEPRECATED_SYMBOLS
/*-------------------------------------------------------------------------
 * Function:    H5Iregister_type1
 *
 * Purpose:     Public interface to H5I_register_type.  Creates a new type
 *              of ID's to give out.  A specific number (RESERVED) of type
 *              entries may be reserved to enable "constant" values to be handed
 *              out which are valid IDs in the type, but which do not map to any
 *              data structures and are not allocated dynamically later. HASH_SIZE is
 *              the minimum hash table size to use for the type. FREE_FUNC is
 *              called with an object pointer when the object is removed from
 *              the type.
 *
 * Return:      Success:    Type ID of the new type
 *              Failure:    H5I_BADID
 *
 *-------------------------------------------------------------------------
 */
H5I_type_t
H5Iregister_type1(size_t H5_ATTR_UNUSED hash_size, unsigned reserved, H5I_free_t free_func)
{
    H5I_type_t ret_value = H5I_BADID;

    FUNC_ENTER_API(H5I_BADID)

    if (H5I_BADID == (ret_value = H5I__register_type_common(reserved, free_func)))
        HGOTO_ERROR(H5E_ID, H5E_CANTINIT, H5I_BADID, "can't initialize ID class");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Iregister_type1() */
#endif /* H5_NO_DEPRECATED_SYMBOLS */
