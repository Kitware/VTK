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

/*-------------------------------------------------------------------------
 *
 * Purpose:     Deprecated functions from the H5O interface.  These
 *              functions are here for compatibility purposes and may be
 *              removed in the future.  Applications should switch to the
 *              newer APIs.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5Omodule.h" /* This source code file is part of the H5O module */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions                        */
#include "H5CXprivate.h" /* API Contexts                             */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5Opkg.h"      /* Object headers                           */

#include "H5VLnative_private.h" /* Native VOL connector                     */

#ifndef H5_NO_DEPRECATED_SYMBOLS

/****************/
/* Local Macros */
/****************/

/******************/
/* Local Typedefs */
/******************/

/* Adapter for using deprecated H5Ovisit1 callbacks with the VOL */
typedef struct H5O_visit1_adapter_t {
    H5O_iterate1_t real_op;      /* Application callback to invoke */
    unsigned       fields;       /* Original fields passed to H5Ovisit */
    void *         real_op_data; /* Application op_data */
} H5O_visit1_adapter_t;

/********************/
/* Package Typedefs */
/********************/

/********************/
/* Local Prototypes */
/********************/
static herr_t H5O__reset_info1(H5O_info1_t *oinfo);
static herr_t H5O__iterate1_adapter(hid_t obj_id, const char *name, const H5O_info2_t *oinfo2, void *op_data);
static herr_t H5O__get_info_old(H5VL_object_t *vol_obj, H5VL_loc_params_t *loc_params, H5O_info1_t *oinfo,
                                unsigned fields);

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
 * Function:    H5O__reset_info1
 *
 * Purpose:     Resets/initializes an H5O_info1_t struct.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__reset_info1(H5O_info1_t *oinfo)
{
    FUNC_ENTER_STATIC_NOERR;

    /* Reset the passed-in info struct */
    HDmemset(oinfo, 0, sizeof(H5O_info1_t));
    oinfo->type = H5O_TYPE_UNKNOWN;
    oinfo->addr = HADDR_UNDEF;

    FUNC_LEAVE_NOAPI(SUCCEED);
} /* end H5O__reset_info1() */

/*-------------------------------------------------------------------------
 * Function:    H5O__iterate1_adapter
 *
 * Purpose:     Retrieve information about an object, according to the order
 *              of an index.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 * Programmer:  Quincey Koziol
 *              November 26 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__iterate1_adapter(hid_t obj_id, const char *name, const H5O_info2_t *oinfo2, void *op_data)
{
    H5O_visit1_adapter_t *shim_data = (H5O_visit1_adapter_t *)op_data;
    H5O_info1_t           oinfo;                    /* Deprecated object info struct */
    unsigned              dm_fields;                /* Fields for data model query */
    unsigned              nat_fields;               /* Fields for native query */
    H5VL_object_t *       vol_obj;                  /* Object of obj_id */
    H5VL_loc_params_t     loc_params;               /* Location parameters for VOL callback */
    herr_t                ret_value = H5_ITER_CONT; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(oinfo2);
    HDassert(op_data);

    /* Reset the legacy info struct */
    if (H5O__reset_info1(&oinfo) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "can't reset object data struct")

    /* Check for retrieving data model information */
    dm_fields = shim_data->fields & (H5O_INFO_BASIC | H5O_INFO_TIME | H5O_INFO_NUM_ATTRS);
    if (dm_fields) {
        /* Set the data model fields */
        if (shim_data->fields & H5O_INFO_BASIC) {
            oinfo.fileno = oinfo2->fileno;
            oinfo.type   = oinfo2->type;
            oinfo.rc     = oinfo2->rc;

            /* Deserialize VOL object token into object address */
            if (H5VLnative_token_to_addr(obj_id, oinfo2->token, &oinfo.addr) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTUNSERIALIZE, FAIL,
                            "can't deserialize object token into address")
        }
        if (shim_data->fields & H5O_INFO_TIME) {
            oinfo.atime = oinfo2->atime;
            oinfo.mtime = oinfo2->mtime;
            oinfo.ctime = oinfo2->ctime;
            oinfo.btime = oinfo2->btime;
        }
        if (shim_data->fields & H5O_INFO_NUM_ATTRS)
            oinfo.num_attrs = oinfo2->num_attrs;
    }

    /* Fill out location struct */
    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.loc_data.loc_by_name.name    = name;
    loc_params.loc_data.loc_by_name.lapl_id = H5P_LINK_ACCESS_DEFAULT;
    loc_params.obj_type                     = H5I_get_type(obj_id);

    /* Get the location object */
    if (NULL == (vol_obj = H5VL_vol_object(obj_id)))
        HGOTO_ERROR(H5E_OHDR, H5E_BADTYPE, H5_ITER_ERROR, "invalid location identifier")

    /* Check for retrieving native information */
    nat_fields = shim_data->fields & (H5O_INFO_HDR | H5O_INFO_META_SIZE);
    if (nat_fields) {
        H5O_native_info_t nat_info; /* Native object info */

        /* Retrieve the object's native information */
        if (H5VL_object_optional(vol_obj, H5VL_NATIVE_OBJECT_GET_NATIVE_INFO, H5P_DATASET_XFER_DEFAULT,
                                 H5_REQUEST_NULL, &loc_params, &nat_info, nat_fields) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't get native info for object")

        /* Set the native fields */
        if (shim_data->fields & H5O_INFO_HDR)
            H5MM_memcpy(&(oinfo.hdr), &(nat_info.hdr), sizeof(H5O_hdr_info_t));
        if (shim_data->fields & H5O_INFO_META_SIZE) {
            H5MM_memcpy(&(oinfo.meta_size.obj), &(nat_info.meta_size.obj), sizeof(H5_ih_info_t));
            H5MM_memcpy(&(oinfo.meta_size.attr), &(nat_info.meta_size.attr), sizeof(H5_ih_info_t));
        }
    }

    /* Invoke the application callback */
    ret_value = (shim_data->real_op)(obj_id, name, &oinfo, shim_data->real_op_data);

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5O__iterate1_adapter() */

/*-------------------------------------------------------------------------
 * Function:    H5O__get_info_old
 *
 * Purpose:     Retrieve deprecated info about an object.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              December 21 2019
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__get_info_old(H5VL_object_t *vol_obj, H5VL_loc_params_t *loc_params, H5O_info1_t *oinfo, unsigned fields)
{
    unsigned dm_fields;           /* Fields for data model query */
    unsigned nat_fields;          /* Fields for native query */
    herr_t   ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(vol_obj);
    HDassert(loc_params);

    /* Reset the passed-in info struct */
    if (H5O__reset_info1(oinfo) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "can't reset object data struct")

    /* Check for retrieving data model information */
    dm_fields = fields & (H5O_INFO_BASIC | H5O_INFO_TIME | H5O_INFO_NUM_ATTRS);
    if (dm_fields) {
        H5O_info2_t dm_info; /* Data model object info */

        /* Retrieve the object's data model information */
        if (H5VL_object_get(vol_obj, loc_params, H5VL_OBJECT_GET_INFO, H5P_DATASET_XFER_DEFAULT,
                            H5_REQUEST_NULL, &dm_info, dm_fields) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't get data model info for object")

        /* Set the data model fields */
        if (fields & H5O_INFO_BASIC) {
            void *vol_obj_data;

            if (NULL == (vol_obj_data = H5VL_object_data(vol_obj)))
                HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get underlying VOL object")

            oinfo->fileno = dm_info.fileno;
            oinfo->type   = dm_info.type;
            oinfo->rc     = dm_info.rc;

            /* Deserialize VOL object token into object address */
            if (H5VL_native_token_to_addr(vol_obj_data, loc_params->obj_type, dm_info.token, &oinfo->addr) <
                0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTUNSERIALIZE, FAIL,
                            "can't deserialize object token into address")
        } /* end if */
        if (fields & H5O_INFO_TIME) {
            oinfo->atime = dm_info.atime;
            oinfo->mtime = dm_info.mtime;
            oinfo->ctime = dm_info.ctime;
            oinfo->btime = dm_info.btime;
        } /* end if */
        if (fields & H5O_INFO_NUM_ATTRS)
            oinfo->num_attrs = dm_info.num_attrs;
    } /* end if */

    /* Check for retrieving native information */
    nat_fields = fields & (H5O_INFO_HDR | H5O_INFO_META_SIZE);
    if (nat_fields) {
        H5O_native_info_t nat_info; /* Native object info */

        /* Retrieve the object's native information */
        if (H5VL_object_optional(vol_obj, H5VL_NATIVE_OBJECT_GET_NATIVE_INFO, H5P_DATASET_XFER_DEFAULT,
                                 H5_REQUEST_NULL, loc_params, &nat_info, nat_fields) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't get native info for object")

        /* Set the native fields */
        if (fields & H5O_INFO_HDR)
            H5MM_memcpy(&(oinfo->hdr), &(nat_info.hdr), sizeof(H5O_hdr_info_t));
        if (fields & H5O_INFO_META_SIZE) {
            H5MM_memcpy(&(oinfo->meta_size.obj), &(nat_info.meta_size.obj), sizeof(H5_ih_info_t));
            H5MM_memcpy(&(oinfo->meta_size.attr), &(nat_info.meta_size.attr), sizeof(H5_ih_info_t));
        } /* end if */
    }     /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__get_info_old() */

/*-------------------------------------------------------------------------
 * Function:	H5Oopen_by_addr
 *
 * Purpose:	Warning! This function is EXTREMELY DANGEROUS!
 *              Improper use can lead to FILE CORRUPTION, INACCESSIBLE DATA,
 *              and other VERY BAD THINGS!
 *
 *              This function opens an object using its address within the
 *              HDF5 file, similar to an HDF5 hard link. The open object
 *              is identical to an object opened with H5Oopen() and should
 *              be closed with H5Oclose() or a type-specific closing
 *              function (such as H5Gclose() ).
 *
 *              This function is very dangerous if called on an invalid
 *              address. For this reason, H5Oincr_refcount() should be
 *              used to prevent HDF5 from deleting any object that is
 *              referenced by address (e.g. by a user-defined link).
 *              H5Odecr_refcount() should be used when the object is
 *              no longer being referenced by address (e.g. when the UD link
 *              is deleted).
 *
 *              The address of the HDF5 file on disk has no effect on
 *              H5Oopen_by_addr(), nor does the use of any unusual file
 *              drivers. The "address" is really the offset within the
 *              HDF5 file, and HDF5's file drivers will transparently
 *              map this to an address on disk for the filesystem.
 *
 * Return:	Success:	An open object identifier
 *		Failure:	H5I_INVALID_HID
 *
 * Programmer:	James Laird
 *		July 14 2006
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Oopen_by_addr(hid_t loc_id, haddr_t addr)
{
    H5VL_object_t *   vol_obj;                  /* Object of loc_id */
    H5I_type_t        vol_obj_type = H5I_BADID; /* Object type of loc_id */
    H5I_type_t        opened_type;              /* Opened object type */
    void *            opened_obj = NULL;        /* Opened object */
    H5VL_loc_params_t loc_params;               /* Location parameters */
    H5O_token_t       obj_token = {0};          /* Object token */
    hbool_t           is_native_vol_obj;
    hid_t             ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE2("i", "ia", loc_id, addr);

    /* Get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid location identifier")

    /* Get object type */
    if ((vol_obj_type = H5I_get_type(loc_id)) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid location identifier")

    /* Check if the VOL object is a native VOL connector object */
    if (H5VL_object_is_native(vol_obj, &is_native_vol_obj) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, H5I_INVALID_HID,
                    "can't determine if VOL object is native connector object")
    if (is_native_vol_obj) {
        /* This is a native-specific routine that requires serialization of the token */
        if (H5VLnative_addr_to_token(loc_id, addr, &obj_token) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTSERIALIZE, H5I_INVALID_HID,
                        "can't serialize address into object token")
    } /* end if */
    else
        HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, H5I_INVALID_HID,
                    "H5Oopen_by_addr is only meant to be used with the native VOL connector")

    loc_params.type                        = H5VL_OBJECT_BY_TOKEN;
    loc_params.loc_data.loc_by_token.token = &obj_token;
    loc_params.obj_type                    = vol_obj_type;

    /* Open the object */
    if (NULL == (opened_obj = H5VL_object_open(vol_obj, &loc_params, &opened_type, H5P_DATASET_XFER_DEFAULT,
                                               H5_REQUEST_NULL)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTOPENOBJ, H5I_INVALID_HID, "unable to open object")

    /* Register the object's ID */
    if ((ret_value = H5VL_register(opened_type, opened_obj, vol_obj->connector, TRUE)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to atomize object handle")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oopen_by_addr() */

/*-------------------------------------------------------------------------
 * Function:    H5Oget_info1
 *
 * Purpose:     Retrieve information about an object.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Oget_info1(hid_t loc_id, H5O_info1_t *oinfo)
{
    H5VL_object_t *   vol_obj = NULL; /* Object of loc_id */
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", loc_id, oinfo);

    /* Check args */
    if (!oinfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "oinfo parameter cannot be NULL")

    /* Set location struct fields */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(loc_id);

    /* Get the location object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Retrieve the object's information */
    if (H5O__get_info_old(vol_obj, &loc_params, oinfo, H5O_INFO_ALL) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't get deprecated info for object")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oget_info1() */

/*-------------------------------------------------------------------------
 * Function:    H5Oget_info_by_name1
 *
 * Purpose:     Retrieve information about an object.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Oget_info_by_name1(hid_t loc_id, const char *name, H5O_info1_t *oinfo, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj = NULL; /* object of loc_id */
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "i*s*xi", loc_id, name, oinfo, lapl_id);

    /* Check args */
    if (!name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "name parameter cannot be NULL")
    if (!*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "name parameter cannot be an empty string")
    if (!oinfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "oinfo parameter cannot be NULL")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Fill out location struct */
    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.loc_data.loc_by_name.name    = name;
    loc_params.loc_data.loc_by_name.lapl_id = lapl_id;
    loc_params.obj_type                     = H5I_get_type(loc_id);

    /* Get the location object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Retrieve the object's information */
    if (H5O__get_info_old(vol_obj, &loc_params, oinfo, H5O_INFO_ALL) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't get deprecated info for object")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oget_info_by_name1() */

/*-------------------------------------------------------------------------
 * Function:    H5Oget_info_by_idx1
 *
 * Purpose:     Retrieve information about an object, according to the order
 *              of an index.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 * Programmer:  Quincey Koziol
 *              November 26 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Oget_info_by_idx1(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order,
                    hsize_t n, H5O_info1_t *oinfo, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj = NULL; /* object of loc_id */
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE7("e", "i*sIiIoh*xi", loc_id, group_name, idx_type, order, n, oinfo, lapl_id);

    /* Check args */
    if (!group_name || !*group_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if (!oinfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no info struct")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "can't set access property list info")

    loc_params.type                         = H5VL_OBJECT_BY_IDX;
    loc_params.loc_data.loc_by_idx.name     = group_name;
    loc_params.loc_data.loc_by_idx.idx_type = idx_type;
    loc_params.loc_data.loc_by_idx.order    = order;
    loc_params.loc_data.loc_by_idx.n        = n;
    loc_params.loc_data.loc_by_idx.lapl_id  = lapl_id;
    loc_params.obj_type                     = H5I_get_type(loc_id);

    /* Get the location object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Retrieve the object's information */
    if (H5O__get_info_old(vol_obj, &loc_params, oinfo, H5O_INFO_ALL) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't get deprecated info for object")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oget_info_by_idx1() */

/*-------------------------------------------------------------------------
 * Function:    H5Oget_info2
 *
 * Purpose:     Retrieve information about an object.
 *
 *              NOTE: Add a parameter "fields" to indicate selection of object info.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Neil Fortner
 *              July 7 2010
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Oget_info2(hid_t loc_id, H5O_info1_t *oinfo, unsigned fields)
{
    H5VL_object_t *   vol_obj; /* Object of loc_id */
    H5VL_loc_params_t loc_params;
    hbool_t           is_native_vol_obj;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*xIu", loc_id, oinfo, fields);

    /* Check args */
    if (!oinfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "oinfo parameter cannot be NULL")
    if (fields & ~H5O_INFO_ALL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid fields")

    /* Set location struct fields */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(loc_id);

    /* Get the location object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Check if the VOL object is a native VOL connector object */
    if (H5VL_object_is_native(vol_obj, &is_native_vol_obj) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, H5I_INVALID_HID,
                    "can't determine if VOL object is native connector object")
    if (!is_native_vol_obj)
        HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, H5I_INVALID_HID,
                    "H5Oget_info2 is only meant to be used with the native VOL connector")

    /* Retrieve deprecated info struct */
    if (H5O__get_info_old(vol_obj, &loc_params, oinfo, fields) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't get deprecated info for object")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oget_info2() */

/*-------------------------------------------------------------------------
 * Function:    H5Oget_info_by_name2
 *
 * Purpose:     Retrieve information about an object
 *
 *              NOTE: Add a parameter "fields" to indicate selection of object info.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Neil Fortner
 *              July 7 2010
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Oget_info_by_name2(hid_t loc_id, const char *name, H5O_info1_t *oinfo, unsigned fields, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj; /* Object of loc_id */
    H5VL_loc_params_t loc_params;
    hbool_t           is_native_vol_obj;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "i*s*xIui", loc_id, name, oinfo, fields, lapl_id);

    /* Check args */
    if (!name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "name parameter cannot be NULL")
    if (!*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "name parameter cannot be an empty string")
    if (!oinfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "oinfo parameter cannot be NULL")
    if (fields & ~H5O_INFO_ALL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid fields")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Fill out location struct */
    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.loc_data.loc_by_name.name    = name;
    loc_params.loc_data.loc_by_name.lapl_id = lapl_id;
    loc_params.obj_type                     = H5I_get_type(loc_id);

    /* Get the location object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Check if the VOL object is a native VOL connector object */
    if (H5VL_object_is_native(vol_obj, &is_native_vol_obj) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, H5I_INVALID_HID,
                    "can't determine if VOL object is native connector object")
    if (!is_native_vol_obj)
        HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, H5I_INVALID_HID,
                    "H5Oget_info_by_name2 is only meant to be used with the native VOL connector")

    /* Retrieve deprecated info struct */
    if (H5O__get_info_old(vol_obj, &loc_params, oinfo, fields) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't get deprecated info for object")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oget_info_by_name2() */

/*-------------------------------------------------------------------------
 * Function:    H5Oget_info_by_idx2
 *
 * Purpose:     Retrieve information about an object, according to the order
 *              of an index.
 *
 *              NOTE: Add a parameter "fields" to indicate selection of object info.
 *
 * Return:      Success:	Non-negative
 *              Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              November 26 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Oget_info_by_idx2(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order,
                    hsize_t n, H5O_info1_t *oinfo, unsigned fields, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj; /* Object of loc_id */
    H5VL_loc_params_t loc_params;
    hbool_t           is_native_vol_obj;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE8("e", "i*sIiIoh*xIui", loc_id, group_name, idx_type, order, n, oinfo, fields, lapl_id);

    /* Check args */
    if (!group_name || !*group_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if (!oinfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no info struct")
    if (fields & ~H5O_INFO_ALL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid fields")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "can't set access property list info")

    loc_params.type                         = H5VL_OBJECT_BY_IDX;
    loc_params.loc_data.loc_by_idx.name     = group_name;
    loc_params.loc_data.loc_by_idx.idx_type = idx_type;
    loc_params.loc_data.loc_by_idx.order    = order;
    loc_params.loc_data.loc_by_idx.n        = n;
    loc_params.loc_data.loc_by_idx.lapl_id  = lapl_id;
    loc_params.obj_type                     = H5I_get_type(loc_id);

    /* Get the location object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Check if the VOL object is a native VOL connector object */
    if (H5VL_object_is_native(vol_obj, &is_native_vol_obj) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, H5I_INVALID_HID,
                    "can't determine if VOL object is native connector object")
    if (!is_native_vol_obj)
        HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, H5I_INVALID_HID,
                    "H5Oget_info_by_idx2 is only meant to be used with the native VOL connector")

    /* Retrieve deprecated info struct */
    if (H5O__get_info_old(vol_obj, &loc_params, oinfo, fields) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't get deprecated info for object")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oget_info_by_idx2() */

/*-------------------------------------------------------------------------
 * Function:	H5Ovisit1
 *
 * Purpose:     Recursively visit an object and all the objects reachable
 *              from it.  If the starting object is a group, all the objects
 *              linked to from that group will be visited.  Links within
 *              each group are visited according to the order within the
 *              specified index (unless the specified index does not exist for
 *              a particular group, then the "name" index is used).
 *
 *              NOTE: Soft links and user-defined links are ignored during
 *              this operation.
 *
 *              NOTE: Each _object_ reachable from the initial group will only
 *              be visited once.  If multiple hard links point to the same
 *              object, the first link to the object's path (according to the
 *              iteration index and iteration order given) will be used to in
 *              the callback about the object.
 *
 * Return:      Success:    The return value of the first operator that
 *				returns non-zero, or zero if all members were
 *				processed with no operator returning non-zero.
 *
 *              Failure:    Negative if something goes wrong within the
 *				library, or the negative value returned by one
 *				of the operators.
 *
 * Programmer:	Quincey Koziol
 *              November 25 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ovisit1(hid_t obj_id, H5_index_t idx_type, H5_iter_order_t order, H5O_iterate1_t op, void *op_data)
{
    H5VL_object_t *      vol_obj = NULL; /* Object of loc_id */
    H5VL_loc_params_t    loc_params;
    H5O_visit1_adapter_t shim_data; /* Adapter for passing app callback & user data */
    herr_t               ret_value; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "iIiIox*x", obj_id, idx_type, order, op, op_data);

    /* Check args */
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if (!op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no callback operator specified")

    /* Get the location object */
    if (NULL == (vol_obj = H5VL_vol_object(obj_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Set location parameters */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(obj_id);

    /* Set up adapter */
    shim_data.real_op      = op;
    shim_data.fields       = H5O_INFO_ALL;
    shim_data.real_op_data = op_data;

    /* Visit the objects */
    if ((ret_value = H5VL_object_specific(vol_obj, &loc_params, H5VL_OBJECT_VISIT, H5P_DATASET_XFER_DEFAULT,
                                          H5_REQUEST_NULL, (int)idx_type, (int)order, H5O__iterate1_adapter,
                                          (void *)&shim_data, H5O_INFO_ALL)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_BADITER, FAIL, "object visitation failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ovisit1() */

/*-------------------------------------------------------------------------
 * Function:    H5Ovisit_by_name1
 *
 * Purpose:     Recursively visit an object and all the objects reachable
 *              from it.  If the starting object is a group, all the objects
 *              linked to from that group will be visited.  Links within
 *              each group are visited according to the order within the
 *              specified index (unless the specified index does not exist for
 *              a particular group, then the "name" index is used).
 *
 *              NOTE: Soft links and user-defined links are ignored during
 *              this operation.
 *
 *              NOTE: Each _object_ reachable from the initial group will only
 *              be visited once.  If multiple hard links point to the same
 *              object, the first link to the object's path (according to the
 *              iteration index and iteration order given) will be used to in
 *              the callback about the object.
 *
 * Return:      Success:    The return value of the first operator that
 *				returns non-zero, or zero if all members were
 *				processed with no operator returning non-zero.
 *
 *              Failure:    Negative if something goes wrong within the
 *				library, or the negative value returned by one
 *				of the operators.
 *
 * Programmer:	Quincey Koziol
 *              November 24 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ovisit_by_name1(hid_t loc_id, const char *obj_name, H5_index_t idx_type, H5_iter_order_t order,
                  H5O_iterate1_t op, void *op_data, hid_t lapl_id)
{
    H5VL_object_t *      vol_obj = NULL; /* Object of loc_id */
    H5VL_loc_params_t    loc_params;
    H5O_visit1_adapter_t shim_data; /* Adapter for passing app callback & user data */
    herr_t               ret_value; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE7("e", "i*sIiIox*xi", loc_id, obj_name, idx_type, order, op, op_data, lapl_id);

    /* Check args */
    if (!obj_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "obj_name parameter cannot be NULL")
    if (!*obj_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "obj_name parameter cannot be an empty string")
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if (!op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no callback operator specified")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Get the location object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Set location parameters */
    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.loc_data.loc_by_name.name    = obj_name;
    loc_params.loc_data.loc_by_name.lapl_id = lapl_id;
    loc_params.obj_type                     = H5I_get_type(loc_id);

    /* Set up adapter */
    shim_data.real_op      = op;
    shim_data.fields       = H5O_INFO_ALL;
    shim_data.real_op_data = op_data;

    /* Visit the objects */
    if ((ret_value = H5VL_object_specific(vol_obj, &loc_params, H5VL_OBJECT_VISIT, H5P_DATASET_XFER_DEFAULT,
                                          H5_REQUEST_NULL, (int)idx_type, (int)order, H5O__iterate1_adapter,
                                          (void *)&shim_data, H5O_INFO_ALL)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_BADITER, FAIL, "object visitation failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ovisit_by_name1() */

/*-------------------------------------------------------------------------
 * Function:    H5Ovisit2
 *
 * Purpose:     Recursively visit an object and all the objects reachable
 *              from it.  If the starting object is a group, all the objects
 *              linked to from that group will be visited.  Links within
 *              each group are visited according to the order within the
 *              specified index (unless the specified index does not exist for
 *              a particular group, then the "name" index is used).
 *
 *              NOTE: Soft links and user-defined links are ignored during
 *              this operation.
 *
 *              NOTE: Each _object_ reachable from the initial group will only
 *              be visited once.  If multiple hard links point to the same
 *              object, the first link to the object's path (according to the
 *              iteration index and iteration order given) will be used to in
 *              the callback about the object.
 *
 *              NOTE: Add a a parameter "fields" to indicate selection of
 *              object info to be retrieved to the callback "op".
 *
 * Return:      Success:    The return value of the first operator that
 *                          returns non-zero, or zero if all members were
 *                          processed with no operator returning non-zero.
 *
 *              Failure:    Negative if something goes wrong within the
 *                          library, or the negative value returned by one
 *                          of the operators.
 *
 * Programmer:	Quincey Koziol
 *              November 25 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ovisit2(hid_t obj_id, H5_index_t idx_type, H5_iter_order_t order, H5O_iterate1_t op, void *op_data,
          unsigned fields)
{
    H5VL_object_t *      vol_obj; /* Object of loc_id */
    H5VL_loc_params_t    loc_params;
    H5O_visit1_adapter_t shim_data; /* Adapter for passing app callback & user data */
    hbool_t              is_native_vol_obj;
    herr_t               ret_value; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "iIiIox*xIu", obj_id, idx_type, order, op, op_data, fields);

    /* Check args */
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if (!op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no callback operator specified")
    if (fields & ~H5O_INFO_ALL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid fields")

    /* Get the location object */
    if (NULL == (vol_obj = H5VL_vol_object(obj_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Check if the VOL object is a native VOL connector object */
    if (H5VL_object_is_native(vol_obj, &is_native_vol_obj) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, H5I_INVALID_HID,
                    "can't determine if VOL object is native connector object")
    if (!is_native_vol_obj)
        HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, H5I_INVALID_HID,
                    "H5Ovisit2 is only meant to be used with the native VOL connector")

    /* Set location parameters */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(obj_id);

    /* Set up adapter */
    shim_data.real_op      = op;
    shim_data.fields       = fields;
    shim_data.real_op_data = op_data;

    /* Visit the objects */
    if ((ret_value = H5VL_object_specific(vol_obj, &loc_params, H5VL_OBJECT_VISIT, H5P_DATASET_XFER_DEFAULT,
                                          H5_REQUEST_NULL, (int)idx_type, (int)order, H5O__iterate1_adapter,
                                          (void *)&shim_data, fields)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_BADITER, FAIL, "object iteration failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ovisit2() */

/*-------------------------------------------------------------------------
 * Function:    H5Ovisit_by_name2
 *
 * Purpose:     Recursively visit an object and all the objects reachable
 *              from it.  If the starting object is a group, all the objects
 *              linked to from that group will be visited.  Links within
 *              each group are visited according to the order within the
 *              specified index (unless the specified index does not exist for
 *              a particular group, then the "name" index is used).
 *
 *              NOTE: Soft links and user-defined links are ignored during
 *              this operation.
 *
 *              NOTE: Each _object_ reachable from the initial group will only
 *              be visited once.  If multiple hard links point to the same
 *              object, the first link to the object's path (according to the
 *              iteration index and iteration order given) will be used to in
 *              the callback about the object.
 *
 *              NOTE: Add a a parameter "fields" to indicate selection of
 *              object info to be retrieved to the callback "op".
 *
 * Return:      Success:    The return value of the first operator that
 *                          returns non-zero, or zero if all members were
 *                          processed with no operator returning non-zero.
 *
 *              Failure:    Negative if something goes wrong within the
 *                          library, or the negative value returned by one
 *                          of the operators.
 *
 * Programmer:	Quincey Koziol
 *              November 24 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ovisit_by_name2(hid_t loc_id, const char *obj_name, H5_index_t idx_type, H5_iter_order_t order,
                  H5O_iterate1_t op, void *op_data, unsigned fields, hid_t lapl_id)
{
    H5VL_object_t *      vol_obj; /* Object of loc_id */
    H5VL_loc_params_t    loc_params;
    H5O_visit1_adapter_t shim_data; /* Adapter for passing app callback & user data */
    hbool_t              is_native_vol_obj;
    herr_t               ret_value; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE8("e", "i*sIiIox*xIui", loc_id, obj_name, idx_type, order, op, op_data, fields, lapl_id);

    /* Check args */
    if (!obj_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "obj_name parameter cannot be NULL")
    if (!*obj_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "obj_name parameter cannot be an empty string")
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if (!op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no callback operator specified")
    if (fields & ~H5O_INFO_ALL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid fields")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Get the location object */
    if (NULL == (vol_obj = H5VL_vol_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Check if the VOL object is a native VOL connector object */
    if (H5VL_object_is_native(vol_obj, &is_native_vol_obj) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, H5I_INVALID_HID,
                    "can't determine if VOL object is native connector object")
    if (!is_native_vol_obj)
        HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, H5I_INVALID_HID,
                    "H5Ovisit_by_name2 is only meant to be used with the native VOL connector")

    /* Set location parameters */
    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.loc_data.loc_by_name.name    = obj_name;
    loc_params.loc_data.loc_by_name.lapl_id = lapl_id;
    loc_params.obj_type                     = H5I_get_type(loc_id);

    /* Set up adapter */
    shim_data.real_op      = op;
    shim_data.fields       = fields;
    shim_data.real_op_data = op_data;

    /* Visit the objects */
    if ((ret_value = H5VL_object_specific(vol_obj, &loc_params, H5VL_OBJECT_VISIT, H5P_DATASET_XFER_DEFAULT,
                                          H5_REQUEST_NULL, (int)idx_type, (int)order, H5O__iterate1_adapter,
                                          (void *)&shim_data, fields)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_BADITER, FAIL, "object iteration failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ovisit_by_name2() */

#endif /* H5_NO_DEPRECATED_SYMBOLS */
