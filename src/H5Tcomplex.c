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

/*
 * Module Info: This module contains the functionality for complex number
 *              datatypes in the H5T interface.
 */

/****************/
/* Module Setup */
/****************/

#include "H5Tmodule.h" /* This source code file is part of the H5T module */

/***********/
/* Headers */
/***********/
#include "H5private.h"  /* Generic Functions    */
#include "H5Eprivate.h" /* Error handling       */
#include "H5Iprivate.h" /* IDs                  */
#include "H5Tpkg.h"     /* Datatypes            */

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
/* Public Variables */
/*********************/

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
 * Function:    H5Tcomplex_create
 *
 * Purpose:     Create a new complex number datatype based on the specified
 *              base datatype ID.
 *
 * Return:      Success:    ID of new complex number datatype
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tcomplex_create(hid_t base_type_id)
{
    H5T_t *base      = NULL;            /* base datatype */
    H5T_t *dt        = NULL;            /* new datatype  */
    hid_t  ret_value = H5I_INVALID_HID; /* return value  */

    FUNC_ENTER_API(H5I_INVALID_HID)

    if (NULL == (base = (H5T_t *)H5I_object_verify(base_type_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid base datatype ID");

    if (NULL == (dt = H5T__complex_create(base)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, H5I_INVALID_HID,
                    "can't create complex number datatype from base datatype");

    if ((ret_value = H5I_register(H5I_DATATYPE, dt, true)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to register datatype");

done:
    FUNC_LEAVE_API(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5T__complex_create
 *
 * Purpose:     Create a new complex number datatype based on the specified
 *              base datatype.
 *
 * Return:      Success:    new complex number datatype
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T__complex_create(const H5T_t *base)
{
    H5T_t *dt        = NULL; /* New complex number datatype */
    H5T_t *ret_value = NULL; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    assert(base);

    /* Currently, only floating-point base datatypes are supported. */
    if (base->shared->type != H5T_FLOAT)
        HGOTO_ERROR(H5E_DATATYPE, H5E_BADVALUE, NULL, "base datatype is not a H5T_FLOAT datatype");
    if (base->shared->size == 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_BADVALUE, NULL, "invalid base datatype size");
    if (base->shared->size > SIZE_MAX / 2)
        HGOTO_ERROR(H5E_DATATYPE, H5E_BADVALUE, NULL,
                    "base datatype size too large - new datatype size would overflow");

    /* Build new type */
    if (NULL == (dt = H5T__alloc()))
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTALLOC, NULL, "memory allocation failed");
    dt->shared->type = H5T_COMPLEX;
    dt->shared->size = 2 * base->shared->size;

    if (NULL == (dt->shared->parent = H5T_copy(base, H5T_COPY_ALL)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, NULL, "can't copy base datatype");

    /* Set complex number-specific fields */
    dt->shared->u.cplx.form = H5T_COMPLEX_RECTANGULAR; /* Only rectangular form is currently supported */

    /* Complex number datatypes use a later version of the datatype object header message */
    dt->shared->version = MAX(base->shared->version, H5O_DTYPE_VERSION_5);

    ret_value = dt;

done:
    if (!ret_value)
        if (dt && H5T_close(dt) < 0)
            HDONE_ERROR(H5E_DATATYPE, H5E_CANTCLOSEOBJ, NULL, "can't close datatype");

    FUNC_LEAVE_NOAPI(ret_value)
}
