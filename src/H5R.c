/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose:     Reference routines.
 */

/****************/
/* Module Setup */
/****************/

#include "H5Rmodule.h" /* This source code file is part of the H5R module */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions                        */
#include "H5CXprivate.h" /* API Contexts                             */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5MMprivate.h" /* Memory management                        */
#include "H5Rpkg.h"      /* References                               */
#include "H5Sprivate.h"  /* Dataspaces                               */

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
 * Function:    H5Rcreate_object
 *
 * Purpose:     Creates an object reference. The LOC_ID and NAME are used
 *              to locate the object pointed to.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Rcreate_object(hid_t loc_id, const char *name, hid_t oapl_id, H5R_ref_t *ref_ptr)
{
    H5VL_object_t *       vol_obj = NULL;                 /* Object of loc_id */
    H5I_type_t            obj_type;                       /* Object type of loc_id */
    hid_t                 file_id      = H5I_INVALID_HID; /* File ID */
    H5VL_object_t *       vol_obj_file = NULL;            /* Object of file_id */
    H5VL_loc_params_t     loc_params;                     /* Location parameters */
    H5O_token_t           obj_token = {0};                /* Object token */
    H5VL_file_cont_info_t cont_info = {H5VL_CONTAINER_INFO_VERSION, 0, 0, 0};
    herr_t                ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "i*si*Rr", loc_id, name, oapl_id, ref_ptr);

    /* Check args */
    if (ref_ptr == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference pointer")
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name given")
    if (oapl_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

    /* Get object access property list */
    if (H5P_DEFAULT == oapl_id)
        oapl_id = H5P_LINK_ACCESS_DEFAULT;
    else if (TRUE != H5P_isa_class(oapl_id, H5P_LINK_ACCESS))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "oapl_id is not a link access property list ID")

    /* Get the VOL object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Get object type */
    if ((obj_type = H5I_get_type(loc_id)) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Get the file for the object */
    if ((file_id = H5F_get_file_id(vol_obj, obj_type, FALSE)) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    /* Retrieve VOL file object */
    if (NULL == (vol_obj_file = H5VL_vol_object(file_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Get container info */
    if (H5VL_file_get(vol_obj_file, H5VL_FILE_GET_CONT_INFO, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                      &cont_info) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, FAIL, "unable to get container info")

    /* Set location parameters */
    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.loc_data.loc_by_name.name    = name;
    loc_params.loc_data.loc_by_name.lapl_id = oapl_id;
    loc_params.obj_type                     = obj_type;

    /* Get the object token */
    if (H5VL_object_specific(vol_obj, &loc_params, H5VL_OBJECT_LOOKUP, H5P_DATASET_XFER_DEFAULT,
                             H5_REQUEST_NULL, &obj_token) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, FAIL, "unable to retrieve object token")

    /* Create the reference (do not pass filename, since file_id is attached) */
    HDmemset(ref_ptr, 0, H5R_REF_BUF_SIZE);
    if (H5R__create_object((const H5O_token_t *)&obj_token, cont_info.token_size, (H5R_ref_priv_t *)ref_ptr) <
        0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTCREATE, FAIL, "unable to create object reference")

    /* Attach loc_id to reference and hold reference to it */
    if (H5R__set_loc_id((H5R_ref_priv_t *)ref_ptr, file_id, TRUE, TRUE) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTSET, FAIL, "unable to attach location id to reference")

done:
    if (file_id != H5I_INVALID_HID && H5I_dec_ref(file_id) < 0)
        HDONE_ERROR(H5E_REFERENCE, H5E_CANTDEC, FAIL, "unable to decrement refcount on file")
    FUNC_LEAVE_API(ret_value)
} /* end H5Rcreate_object() */

/*-------------------------------------------------------------------------
 * Function:    H5Rcreate_region
 *
 * Purpose:     Creates a region reference. The LOC_ID and NAME are used to
 *              locate the object pointed to and the SPACE_ID is used to
 *              choose the region pointed to.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Rcreate_region(hid_t loc_id, const char *name, hid_t space_id, hid_t oapl_id, H5R_ref_t *ref_ptr)
{
    H5VL_object_t *       vol_obj = NULL;                 /* Object of loc_id */
    H5I_type_t            obj_type;                       /* Object type of loc_id */
    hid_t                 file_id      = H5I_INVALID_HID; /* File ID */
    H5VL_object_t *       vol_obj_file = NULL;            /* Object of file_id */
    H5VL_loc_params_t     loc_params;                     /* Location parameters */
    H5O_token_t           obj_token = {0};                /* Object token */
    H5VL_file_cont_info_t cont_info = {H5VL_CONTAINER_INFO_VERSION, 0, 0, 0};
    struct H5S_t *        space     = NULL;    /* Pointer to dataspace containing region */
    herr_t                ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "i*sii*Rr", loc_id, name, space_id, oapl_id, ref_ptr);

    /* Check args */
    if (ref_ptr == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference pointer")
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name given")
    if ((space_id == H5I_INVALID_HID) || (space_id == H5S_ALL))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "reference region dataspace id must be valid")
    if (NULL == (space = (struct H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace")
    if (oapl_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

    /* Get object access property list */
    if (H5P_DEFAULT == oapl_id)
        oapl_id = H5P_LINK_ACCESS_DEFAULT;
    else if (TRUE != H5P_isa_class(oapl_id, H5P_LINK_ACCESS))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "oapl_id is not a link access property list ID")

    /* Get the VOL object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Get object type */
    if ((obj_type = H5I_get_type(loc_id)) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Get the file for the object */
    if ((file_id = H5F_get_file_id(vol_obj, obj_type, FALSE)) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    /* Retrieve VOL file object */
    if (NULL == (vol_obj_file = H5VL_vol_object(file_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Get container info */
    if (H5VL_file_get(vol_obj_file, H5VL_FILE_GET_CONT_INFO, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                      &cont_info) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, FAIL, "unable to get container info")

    /* Set location parameters */
    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.loc_data.loc_by_name.name    = name;
    loc_params.loc_data.loc_by_name.lapl_id = oapl_id;
    loc_params.obj_type                     = obj_type;

    /* Get the object token */
    if (H5VL_object_specific(vol_obj, &loc_params, H5VL_OBJECT_LOOKUP, H5P_DATASET_XFER_DEFAULT,
                             H5_REQUEST_NULL, &obj_token) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, FAIL, "unable to retrieve object token")

    /* Create the reference (do not pass filename, since file_id is attached) */
    HDmemset(ref_ptr, 0, H5R_REF_BUF_SIZE);
    if (H5R__create_region((const H5O_token_t *)&obj_token, cont_info.token_size, space,
                           (H5R_ref_priv_t *)ref_ptr) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTCREATE, FAIL, "unable to create region reference")

    /* Attach loc_id to reference and hold reference to it */
    if (H5R__set_loc_id((H5R_ref_priv_t *)ref_ptr, file_id, TRUE, TRUE) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTSET, FAIL, "unable to attach location id to reference")

done:
    if (file_id != H5I_INVALID_HID && H5I_dec_ref(file_id) < 0)
        HDONE_ERROR(H5E_REFERENCE, H5E_CANTDEC, FAIL, "unable to decrement refcount on file")
    FUNC_LEAVE_API(ret_value)
} /* end H5Rcreate_region() */

/*-------------------------------------------------------------------------
 * Function:    H5Rcreate_attr
 *
 * Purpose:     Creates an attribute reference. The LOC_ID, NAME and
 *              ATTR_NAME are used to locate the attribute pointed to.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Rcreate_attr(hid_t loc_id, const char *name, const char *attr_name, hid_t oapl_id, H5R_ref_t *ref_ptr)
{
    H5VL_object_t *       vol_obj = NULL;                 /* Object of loc_id */
    H5I_type_t            obj_type;                       /* Object type of loc_id */
    hid_t                 file_id      = H5I_INVALID_HID; /* File ID */
    H5VL_object_t *       vol_obj_file = NULL;            /* Object of file_id */
    H5VL_loc_params_t     loc_params;                     /* Location parameters */
    H5O_token_t           obj_token = {0};                /* Object token */
    H5VL_file_cont_info_t cont_info = {H5VL_CONTAINER_INFO_VERSION, 0, 0, 0};
    herr_t                ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "i*s*si*Rr", loc_id, name, attr_name, oapl_id, ref_ptr);

    /* Check args */
    if (ref_ptr == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference pointer")
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name given")
    if (!attr_name || !*attr_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no attribute name given")
    if (oapl_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

    /* Get object access property list */
    if (H5P_DEFAULT == oapl_id)
        oapl_id = H5P_LINK_ACCESS_DEFAULT;
    else if (TRUE != H5P_isa_class(oapl_id, H5P_LINK_ACCESS))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "oapl_id is not a link access property list ID")

    /* Get the VOL object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Get object type */
    if ((obj_type = H5I_get_type(loc_id)) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Get the file for the object */
    if ((file_id = H5F_get_file_id(vol_obj, obj_type, FALSE)) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    /* Retrieve VOL file object */
    if (NULL == (vol_obj_file = H5VL_vol_object(file_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Get container info */
    if (H5VL_file_get(vol_obj_file, H5VL_FILE_GET_CONT_INFO, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                      &cont_info) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, FAIL, "unable to get container info")

    /* Set location parameters */
    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.loc_data.loc_by_name.name    = name;
    loc_params.loc_data.loc_by_name.lapl_id = oapl_id;
    loc_params.obj_type                     = obj_type;

    /* Get the object token */
    if (H5VL_object_specific(vol_obj, &loc_params, H5VL_OBJECT_LOOKUP, H5P_DATASET_XFER_DEFAULT,
                             H5_REQUEST_NULL, &obj_token) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, FAIL, "unable to retrieve object token")

    /* Create the reference (do not pass filename, since file_id is attached) */
    HDmemset(ref_ptr, 0, H5R_REF_BUF_SIZE);
    if (H5R__create_attr((const H5O_token_t *)&obj_token, cont_info.token_size, attr_name,
                         (H5R_ref_priv_t *)ref_ptr) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTCREATE, FAIL, "unable to create attribute reference")

    /* Attach loc_id to reference and hold reference to it */
    if (H5R__set_loc_id((H5R_ref_priv_t *)ref_ptr, file_id, TRUE, TRUE) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTSET, FAIL, "unable to attach location id to reference")

done:
    if (file_id != H5I_INVALID_HID && H5I_dec_ref(file_id) < 0)
        HDONE_ERROR(H5E_REFERENCE, H5E_CANTDEC, FAIL, "unable to decrement refcount on file")
    FUNC_LEAVE_API(ret_value)
} /* end H5Rcreate_attr() */

/*-------------------------------------------------------------------------
 * Function:    H5Rdestroy
 *
 * Purpose:     Destroy reference and free resources allocated during
 *              H5Rcreate.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Rdestroy(H5R_ref_t *ref_ptr)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "*Rr", ref_ptr);

    /* Check args */
    if (NULL == ref_ptr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid reference pointer")

    /* Destroy reference */
    if (H5R__destroy((H5R_ref_priv_t *)ref_ptr) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTFREE, FAIL, "unable to destroy reference")

    /* Memset back to 0 for safety */
    HDmemset(ref_ptr, 0, H5R_REF_BUF_SIZE);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Rdestroy() */

/*-------------------------------------------------------------------------
 * Function:    H5Rget_type
 *
 * Purpose:     Given a reference to some object, return the type of that
 *              reference.
 *
 * Return:      Reference type / H5R_BADTYPE on failure
 *
 *-------------------------------------------------------------------------
 */
H5R_type_t
H5Rget_type(const H5R_ref_t *ref_ptr)
{
    H5R_type_t ret_value; /* Return value */

    FUNC_ENTER_API(H5R_BADTYPE)
    H5TRACE1("Rt", "*Rr", ref_ptr);

    /* Check args */
    if (NULL == ref_ptr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5R_BADTYPE, "invalid reference pointer")

    /* Get reference type */
    ret_value = H5R__get_type((const H5R_ref_priv_t *)ref_ptr);
    if ((ret_value <= H5R_BADTYPE) || (ret_value >= H5R_MAXTYPE))
        HGOTO_ERROR(H5E_REFERENCE, H5E_BADVALUE, H5R_BADTYPE, "invalid reference type")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Rget_type() */

/*-------------------------------------------------------------------------
 * Function:    H5Requal
 *
 * Purpose:     Compare two references
 *
 * Return:      TRUE if equal, FALSE if unequal, FAIL if error
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Requal(const H5R_ref_t *ref1_ptr, const H5R_ref_t *ref2_ptr)
{
    htri_t ret_value; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("t", "*Rr*Rr", ref1_ptr, ref2_ptr);

    /* Check args */
    if (!ref1_ptr || !ref2_ptr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference pointer")

    /* Compare references */
    if ((ret_value = H5R__equal((const H5R_ref_priv_t *)ref2_ptr, (const H5R_ref_priv_t *)ref2_ptr)) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTCOMPARE, FAIL, "cannot compare references")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Requal() */

/*-------------------------------------------------------------------------
 * Function:    H5Rcopy
 *
 * Purpose:     Copy a reference
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Rcopy(const H5R_ref_t *src_ref_ptr, H5R_ref_t *dst_ref_ptr)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "*Rr*Rr", src_ref_ptr, dst_ref_ptr);

    /* Check args */
    if (NULL == src_ref_ptr || NULL == dst_ref_ptr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid reference pointer")

    /* Copy reference */
    if (H5R__copy((const H5R_ref_priv_t *)src_ref_ptr, (H5R_ref_priv_t *)dst_ref_ptr) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTCOPY, FAIL, "cannot copy reference")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Rcopy() */

/*-------------------------------------------------------------------------
 * Function:    H5Ropen_object
 *
 * Purpose:     Given a reference to some object, open that object and
 *              return an ID for that object.
 *
 * Return:      Valid ID on success / H5I_INVALID_HID on failure
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Ropen_object(H5R_ref_t *ref_ptr, hid_t rapl_id, hid_t oapl_id)
{
    hid_t             loc_id;                       /* Reference location ID */
    H5VL_object_t *   vol_obj = NULL;               /* Object of loc_id */
    H5VL_loc_params_t loc_params;                   /* Location parameters */
    H5O_token_t       obj_token = {0};              /* Object token */
    H5I_type_t        opened_type;                  /* Opened object type */
    void *            opened_obj = NULL;            /* Opened object */
    hid_t             ret_value  = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE3("i", "*Rrii", ref_ptr, rapl_id, oapl_id);

    /* Check args */
    if (ref_ptr == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid reference pointer")
    if (H5R__get_type((const H5R_ref_priv_t *)ref_ptr) <= H5R_BADTYPE ||
        H5R__get_type((const H5R_ref_priv_t *)ref_ptr) >= H5R_MAXTYPE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid reference type")
    if (rapl_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a property list")
    if (oapl_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a property list")

    /* Retrieve loc_id from reference */
    if (H5I_INVALID_HID == (loc_id = H5R__get_loc_id((const H5R_ref_priv_t *)ref_ptr))) {
        /* Attempt to re-open file and pass rapl_id as a fapl_id */
        if ((loc_id = H5R__reopen_file((H5R_ref_priv_t *)ref_ptr, rapl_id)) < 0)
            HGOTO_ERROR(H5E_REFERENCE, H5E_CANTOPENFILE, H5I_INVALID_HID, "cannot re-open referenced file")
    }

    /* Get object token */
    if (H5R__get_obj_token((const H5R_ref_priv_t *)ref_ptr, &obj_token, NULL) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, H5I_INVALID_HID, "unable to get object token")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&oapl_id, H5P_CLS_DACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* Get the VOL object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid location identifier")

    /* Set location parameters */
    loc_params.type                        = H5VL_OBJECT_BY_TOKEN;
    loc_params.loc_data.loc_by_token.token = &obj_token;
    loc_params.obj_type                    = H5I_get_type(loc_id);

    /* Open object by token */
    if (NULL == (opened_obj = H5VL_object_open(vol_obj, &loc_params, &opened_type, H5P_DATASET_XFER_DEFAULT,
                                               H5_REQUEST_NULL)))
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTOPENOBJ, H5I_INVALID_HID, "unable to open object by token")

    /* Register object */
    if ((ret_value = H5VL_register(opened_type, opened_obj, vol_obj->connector, TRUE)) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to register object handle")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ropen_object() */

/*-------------------------------------------------------------------------
 * Function:    H5Ropen_region
 *
 * Purpose:     Given a reference to some object, creates a copy of the dataset
 *              pointed to's dataspace and defines a selection in the copy
 *              which is the region pointed to.
 *
 * Return:      Valid ID on success / H5I_INVALID_HID on failure
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Ropen_region(H5R_ref_t *ref_ptr, hid_t rapl_id, hid_t oapl_id)
{
    hid_t             loc_id;                          /* Reference location ID */
    H5VL_object_t *   vol_obj = NULL;                  /* Object of loc_id */
    H5VL_loc_params_t loc_params;                      /* Location parameters */
    H5O_token_t       obj_token = {0};                 /* Object token */
    H5I_type_t        opened_type;                     /* Opened object type */
    void *            opened_obj    = NULL;            /* Opened object */
    hid_t             opened_obj_id = H5I_INVALID_HID; /* Opened object ID */
    H5S_t *           space         = NULL;            /* Dataspace pointer (copy) */
    hid_t             space_id      = H5I_INVALID_HID; /* Dataspace ID */
    hid_t             ret_value     = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE3("i", "*Rrii", ref_ptr, rapl_id, oapl_id);

    /* Check args */
    if (ref_ptr == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid reference pointer")
    if ((H5R__get_type((const H5R_ref_priv_t *)ref_ptr) != H5R_DATASET_REGION1) &&
        (H5R__get_type((const H5R_ref_priv_t *)ref_ptr) != H5R_DATASET_REGION2))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid reference type")
    if (rapl_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a property list")
    if (oapl_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a property list")

    /* Retrieve loc_id from reference */
    if (H5I_INVALID_HID == (loc_id = H5R__get_loc_id((const H5R_ref_priv_t *)ref_ptr))) {
        /* Attempt to re-open file and pass rapl_id as a fapl_id */
        if ((loc_id = H5R__reopen_file((H5R_ref_priv_t *)ref_ptr, rapl_id)) < 0)
            HGOTO_ERROR(H5E_REFERENCE, H5E_CANTOPENFILE, H5I_INVALID_HID, "cannot re-open referenced file")
    }

    /* Get object token */
    if (H5R__get_obj_token((const H5R_ref_priv_t *)ref_ptr, &obj_token, NULL) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, H5I_INVALID_HID, "unable to get object token")

    /* Get the VOL object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid location identifier")

    /* Set location parameters */
    loc_params.type                        = H5VL_OBJECT_BY_TOKEN;
    loc_params.loc_data.loc_by_token.token = &obj_token;
    loc_params.obj_type                    = H5I_get_type(loc_id);

    /* Open object by token */
    if (NULL == (opened_obj = H5VL_object_open(vol_obj, &loc_params, &opened_type, H5P_DATASET_XFER_DEFAULT,
                                               H5_REQUEST_NULL)))
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTOPENOBJ, H5I_INVALID_HID, "unable to open object by token")

    /* Register object */
    if ((opened_obj_id = H5VL_register(opened_type, opened_obj, vol_obj->connector, FALSE)) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to register object handle")

    /* Get VOL object object */
    if (NULL == (opened_obj = H5VL_vol_object(opened_obj_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid location identifier")

    /* Get dataspace from object */
    if (H5VL_dataset_get(opened_obj, H5VL_DATASET_GET_SPACE, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                         &space_id) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, H5I_INVALID_HID, "unable to get dataspace from dataset")
    if (NULL == (space = (struct H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a dataspace")

    /* Get the dataspace with the correct region selected */
    if (H5R__get_region((const H5R_ref_priv_t *)ref_ptr, space) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, H5I_INVALID_HID, "unable to get selection on dataspace")

    /* Simply return space_id */
    ret_value = space_id;

done:
    if ((opened_obj_id != H5I_INVALID_HID) && (H5I_dec_ref(opened_obj_id) < 0))
        HDONE_ERROR(H5E_REFERENCE, H5E_CLOSEERROR, H5I_INVALID_HID, "can't close object")
    if (H5I_INVALID_HID == ret_value) /* Cleanup on failure */
        if ((space_id != H5I_INVALID_HID) && (H5I_dec_ref(space_id) < 0))
            HDONE_ERROR(H5E_REFERENCE, H5E_CLOSEERROR, H5I_INVALID_HID, "can't close dataspace")

    FUNC_LEAVE_API(ret_value)
} /* end H5Ropen_region() */

/*-------------------------------------------------------------------------
 * Function:    H5Ropen_attr
 *
 * Purpose:     Given a reference to some attribute, open that attribute and
 *              return an ID for that attribute.
 *
 * Return:      Valid ID on success / H5I_INVALID_HID on failure
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Ropen_attr(H5R_ref_t *ref_ptr, hid_t rapl_id, hid_t aapl_id)
{
    hid_t             loc_id;                          /* Reference location ID */
    H5VL_object_t *   vol_obj = NULL;                  /* Object of loc_id */
    H5VL_loc_params_t loc_params;                      /* Location parameters */
    H5O_token_t       obj_token = {0};                 /* Object token */
    H5I_type_t        opened_type;                     /* Opened object type */
    void *            opened_obj    = NULL;            /* Opened object */
    hid_t             opened_obj_id = H5I_INVALID_HID; /* Opened object ID */
    void *            opened_attr   = NULL;            /* Opened attribute */
    hid_t             ret_value     = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE3("i", "*Rrii", ref_ptr, rapl_id, aapl_id);

    /* Check args */
    if (ref_ptr == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid reference pointer")
    if (H5R__get_type((const H5R_ref_priv_t *)ref_ptr) != H5R_ATTR)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid reference type")
    if (rapl_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a property list")
    if (aapl_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a property list")

    /* Retrieve loc_id from reference */
    if (H5I_INVALID_HID == (loc_id = H5R__get_loc_id((const H5R_ref_priv_t *)ref_ptr))) {
        /* Attempt to re-open file and pass rapl_id as a fapl_id */
        if ((loc_id = H5R__reopen_file((H5R_ref_priv_t *)ref_ptr, rapl_id)) < 0)
            HGOTO_ERROR(H5E_REFERENCE, H5E_CANTOPENFILE, H5I_INVALID_HID, "cannot re-open referenced file")
    }

    /* Get object token */
    if (H5R__get_obj_token((const H5R_ref_priv_t *)ref_ptr, &obj_token, NULL) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, H5I_INVALID_HID, "unable to get object token")

    /* Get the VOL object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid location identifier")

    /* Set location parameters */
    loc_params.type                        = H5VL_OBJECT_BY_TOKEN;
    loc_params.loc_data.loc_by_token.token = &obj_token;
    loc_params.obj_type                    = H5I_get_type(loc_id);

    /* Open object by token */
    if (NULL == (opened_obj = H5VL_object_open(vol_obj, &loc_params, &opened_type, H5P_DATASET_XFER_DEFAULT,
                                               H5_REQUEST_NULL)))
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTOPENOBJ, H5I_INVALID_HID, "unable to open object by token")

    /* Register object */
    if ((opened_obj_id = H5VL_register(opened_type, opened_obj, vol_obj->connector, FALSE)) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to register object handle")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&aapl_id, H5P_CLS_AACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* Set location parameters */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = opened_type;

    /* Get VOL object object */
    if (NULL == (opened_obj = H5VL_vol_object(opened_obj_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid location identifier")

    /* Open the attribute */
    if (NULL == (opened_attr = H5VL_attr_open(opened_obj, &loc_params,
                                              H5R_REF_ATTRNAME((const H5R_ref_priv_t *)ref_ptr), aapl_id,
                                              H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL)))
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTOPENOBJ, H5I_INVALID_HID, "unable to open attribute: '%s'",
                    H5R_REF_ATTRNAME((const H5R_ref_priv_t *)ref_ptr))

    /* Register the attribute and get an ID for it */
    if ((ret_value = H5VL_register(H5I_ATTR, opened_attr, vol_obj->connector, TRUE)) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to atomize attribute handle")

done:
    if ((opened_obj_id != H5I_INVALID_HID) && (H5I_dec_ref(opened_obj_id) < 0))
        HDONE_ERROR(H5E_REFERENCE, H5E_CLOSEERROR, H5I_INVALID_HID, "can't close object")
    if (H5I_INVALID_HID == ret_value) /* Cleanup on failure */
        if (opened_attr && H5VL_attr_close(vol_obj, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL) < 0)
            HDONE_ERROR(H5E_REFERENCE, H5E_CLOSEERROR, H5I_INVALID_HID, "can't close attribute")

    FUNC_LEAVE_API(ret_value)
} /* end H5Ropen_attr() */

/*-------------------------------------------------------------------------
 * Function:    H5Rget_obj_type3
 *
 * Purpose:     Given a reference to some object, this function returns the
 *              type of object pointed to.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Rget_obj_type3(H5R_ref_t *ref_ptr, hid_t rapl_id, H5O_type_t *obj_type)
{
    hid_t             loc_id;              /* Reference location ID */
    H5VL_object_t *   vol_obj = NULL;      /* Object of loc_id */
    H5VL_loc_params_t loc_params;          /* Location parameters */
    H5O_token_t       obj_token = {0};     /* Object token */
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "*Rri*Ot", ref_ptr, rapl_id, obj_type);

    /* Check args */
    if (ref_ptr == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference pointer")
    if (H5R__get_type((const H5R_ref_priv_t *)ref_ptr) <= H5R_BADTYPE ||
        H5R__get_type((const H5R_ref_priv_t *)ref_ptr) >= H5R_MAXTYPE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference type")
    if (rapl_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

    /* Retrieve loc_id from reference */
    if (H5I_INVALID_HID == (loc_id = H5R__get_loc_id((const H5R_ref_priv_t *)ref_ptr))) {
        /* Attempt to re-open file and pass rapl_id as a fapl_id */
        if ((loc_id = H5R__reopen_file((H5R_ref_priv_t *)ref_ptr, rapl_id)) < 0)
            HGOTO_ERROR(H5E_REFERENCE, H5E_CANTOPENFILE, FAIL, "cannot re-open referenced file")
    }

    /* Get object token */
    if (H5R__get_obj_token((const H5R_ref_priv_t *)ref_ptr, &obj_token, NULL) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, FAIL, "unable to get object token")

    /* Get the VOL object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Set location parameters */
    loc_params.type                        = H5VL_OBJECT_BY_TOKEN;
    loc_params.loc_data.loc_by_token.token = &obj_token;
    loc_params.obj_type                    = H5I_get_type(loc_id);

    /* Retrieve object's type */
    if (H5VL_object_get(vol_obj, &loc_params, H5VL_OBJECT_GET_TYPE, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                        obj_type) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, FAIL, "can't retrieve object type")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Rget_obj_type3() */

/*-------------------------------------------------------------------------
 * Function:    H5Rget_file_name
 *
 * Purpose:     Given a reference to some object, determine a file name of the
 *              object located into.
 *
 * Return:      Non-negative length of the path on success / -1 on failure
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Rget_file_name(const H5R_ref_t *ref_ptr, char *buf, size_t size)
{
    hid_t   loc_id;    /* Reference location ID */
    ssize_t ret_value; /* Return value */

    FUNC_ENTER_API((-1))
    H5TRACE3("Zs", "*Rr*sz", ref_ptr, buf, size);

    /* Check args */
    if (ref_ptr == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "invalid reference pointer")
    if (H5R__get_type((const H5R_ref_priv_t *)ref_ptr) <= H5R_BADTYPE ||
        H5R__get_type((const H5R_ref_priv_t *)ref_ptr) >= H5R_MAXTYPE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "invalid reference type")

    /* Get name */
    if (H5I_INVALID_HID == (loc_id = H5R__get_loc_id((const H5R_ref_priv_t *)ref_ptr))) {
        /* Un-opened external references do not have loc_id set but hold a
         * copy of the filename */
        if ((ret_value = H5R__get_file_name((const H5R_ref_priv_t *)ref_ptr, buf, size)) < 0)
            HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, (-1), "unable to retrieve file name")
    }
    else {
        H5VL_object_t *vol_obj = NULL; /* Object of loc_id */

        /* Retrieve VOL file object */
        if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, (-1), "invalid location identifier")

        if (H5VL_file_get(vol_obj, H5VL_FILE_GET_NAME, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, H5I_FILE,
                          size, buf, &ret_value) < 0)
            HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, (-1), "unable to get file name")
    }

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Rget_file_name() */

/*-------------------------------------------------------------------------
 * Function:    H5Rget_obj_name
 *
 * Purpose:     Given a reference to some object, determine a path to the
 *              object referenced in the file.
 *
 * Return:      Non-negative length of the path on success / -1 on failure
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Rget_obj_name(H5R_ref_t *ref_ptr, hid_t rapl_id, char *buf, size_t size)
{
    hid_t             loc_id;          /* Reference location ID */
    H5VL_object_t *   vol_obj = NULL;  /* Object of loc_id */
    H5VL_loc_params_t loc_params;      /* Location parameters */
    H5O_token_t       obj_token = {0}; /* Object token */
    ssize_t           ret_value = 0;   /* Return value */

    FUNC_ENTER_API((-1))
    H5TRACE4("Zs", "*Rri*sz", ref_ptr, rapl_id, buf, size);

    /* Check args */
    if (ref_ptr == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "invalid reference pointer")
    if (H5R__get_type((const H5R_ref_priv_t *)ref_ptr) <= H5R_BADTYPE ||
        H5R__get_type((const H5R_ref_priv_t *)ref_ptr) >= H5R_MAXTYPE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "invalid reference type")
    if (rapl_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, (-1), "not a property list")

    /* Retrieve loc_id from reference */
    if (H5I_INVALID_HID == (loc_id = H5R__get_loc_id((const H5R_ref_priv_t *)ref_ptr))) {
        /* Attempt to re-open file and pass rapl_id as a fapl_id */
        if ((loc_id = H5R__reopen_file((H5R_ref_priv_t *)ref_ptr, rapl_id)) < 0)
            HGOTO_ERROR(H5E_REFERENCE, H5E_CANTOPENFILE, (-1), "cannot re-open referenced file")
    }

    /* Get object token */
    if (H5R__get_obj_token((const H5R_ref_priv_t *)ref_ptr, &obj_token, NULL) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, (-1), "unable to get object token")

    /* Get the VOL object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, (-1), "invalid location identifier")

    /* Set location parameters */
    loc_params.type                        = H5VL_OBJECT_BY_TOKEN;
    loc_params.loc_data.loc_by_token.token = &obj_token;
    loc_params.obj_type                    = H5I_get_type(loc_id);

    /* Retrieve object's name */
    if (H5VL_object_get(vol_obj, &loc_params, H5VL_OBJECT_GET_NAME, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                        &ret_value, buf, size) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, (-1), "can't retrieve object name")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Rget_obj_name() */

/*-------------------------------------------------------------------------
 * Function:    H5Rget_attr_name
 *
 * Purpose:     Given a reference to some attribute, determine its name.
 *
 * Return:      Non-negative length of the path on success / -1 on failure
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Rget_attr_name(const H5R_ref_t *ref_ptr, char *buf, size_t size)
{
    ssize_t ret_value; /* Return value */

    FUNC_ENTER_API((-1))
    H5TRACE3("Zs", "*Rr*sz", ref_ptr, buf, size);

    /* Check args */
    if (ref_ptr == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "invalid reference pointer")
    if (H5R__get_type((const H5R_ref_priv_t *)ref_ptr) != H5R_ATTR)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "invalid reference type")

    /* Get attribute name */
    if ((ret_value = H5R__get_attr_name((const H5R_ref_priv_t *)ref_ptr, buf, size)) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, (-1), "unable to determine attribute name")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Rget_attr_name() */
