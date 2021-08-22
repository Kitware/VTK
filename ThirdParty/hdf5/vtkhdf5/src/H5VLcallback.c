/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
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
 * Purpose:     The Virtual Object Layer as described in documentation.
 *              The pupose is to provide an abstraction on how to access the
 *              underlying HDF5 container, whether in a local file with
 *              a specific file format, or remotely on other machines, etc...
 */

/****************/
/* Module Setup */
/****************/

#include "H5VLmodule.h" /* This source code file is part of the H5VL module */

/***********/
/* Headers */
/***********/

#include "H5private.h"   /* Generic Functions                                */
#include "H5Eprivate.h"  /* Error handling                                   */
#include "H5Fprivate.h"  /* File access	                                     */
#include "H5Iprivate.h"  /* IDs                                              */
#include "H5MMprivate.h" /* Memory management                                */
#include "H5Pprivate.h"  /* Property lists                                   */
#include "H5PLprivate.h" /* Plugins                                          */
#include "H5VLpkg.h"     /* Virtual Object Layer                             */

/****************/
/* Local Macros */
/****************/

/******************/
/* Local Typedefs */
/******************/

/* Structure used when trying to find a
 * VOL connector to open a given file with.
 */
typedef struct H5VL_file_open_find_connector_t {
    const char *           filename;
    const H5VL_class_t *   cls;
    H5VL_connector_prop_t *connector_prop;
    hid_t                  fapl_id;
} H5VL_file_open_find_connector_t;

/********************/
/* Package Typedefs */
/********************/

/********************/
/* Local Prototypes */
/********************/
static void * H5VL__attr_create(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                                const char *name, hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id,
                                hid_t dxpl_id, void **req);
static void * H5VL__attr_open(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                              const char *name, hid_t aapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL__attr_read(void *obj, const H5VL_class_t *cls, hid_t mem_type_id, void *buf, hid_t dxpl_id,
                              void **req);
static herr_t H5VL__attr_write(void *obj, const H5VL_class_t *cls, hid_t mem_type_id, const void *buf,
                               hid_t dxpl_id, void **req);
static herr_t H5VL__attr_get(void *obj, const H5VL_class_t *cls, H5VL_attr_get_t get_type, hid_t dxpl_id,
                             void **req, va_list arguments);
static herr_t H5VL__attr_specific(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                                  H5VL_attr_specific_t specific_type, hid_t dxpl_id, void **req,
                                  va_list arguments);
static herr_t H5VL__attr_optional(void *obj, const H5VL_class_t *cls, H5VL_attr_optional_t opt_type,
                                  hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL__attr_close(void *obj, const H5VL_class_t *cls, hid_t dxpl_id, void **req);
static void * H5VL__dataset_create(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                                   const char *name, hid_t lcpl_id, hid_t type_id, hid_t space_id,
                                   hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req);
static void * H5VL__dataset_open(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                                 const char *name, hid_t dapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL__dataset_read(void *dset, const H5VL_class_t *cls, hid_t mem_type_id, hid_t mem_space_id,
                                 hid_t file_space_id, hid_t dxpl_id, void *buf, void **req);
static herr_t H5VL__dataset_write(void *obj, const H5VL_class_t *cls, hid_t mem_type_id, hid_t mem_space_id,
                                  hid_t file_space_id, hid_t dxpl_id, const void *buf, void **req);
static herr_t H5VL__dataset_get(void *obj, const H5VL_class_t *cls, H5VL_dataset_get_t get_type,
                                hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL__dataset_specific(void *obj, const H5VL_class_t *cls,
                                     H5VL_dataset_specific_t specific_type, hid_t dxpl_id, void **req,
                                     va_list arguments);
static herr_t H5VL__dataset_optional(void *obj, const H5VL_class_t *cls, H5VL_dataset_optional_t opt_type,
                                     hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL__dataset_close(void *obj, const H5VL_class_t *cls, hid_t dxpl_id, void **req);
static void * H5VL__datatype_commit(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                                    const char *name, hid_t type_id, hid_t lcpl_id, hid_t tcpl_id,
                                    hid_t tapl_id, hid_t dxpl_id, void **req);
static void * H5VL__datatype_open(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                                  const char *name, hid_t tapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL__datatype_get(void *obj, const H5VL_class_t *cls, H5VL_datatype_get_t get_type,
                                 hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL__datatype_specific(void *obj, const H5VL_class_t *cls,
                                      H5VL_datatype_specific_t specific_type, hid_t dxpl_id, void **req,
                                      va_list arguments);
static herr_t H5VL__datatype_optional(void *obj, const H5VL_class_t *cls, H5VL_datatype_optional_t opt_type,
                                      hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL__datatype_close(void *obj, const H5VL_class_t *cls, hid_t dxpl_id, void **req);
static void * H5VL__file_create(const H5VL_class_t *cls, const char *name, unsigned flags, hid_t fcpl_id,
                                hid_t fapl_id, hid_t dxpl_id, void **req);
static void * H5VL__file_open(const H5VL_class_t *cls, const char *name, unsigned flags, hid_t fapl_id,
                              hid_t dxpl_id, void **req);
static herr_t H5VL__file_open_find_connector_cb(H5PL_type_t plugin_type, const void *plugin_info,
                                                void *op_data);
static herr_t H5VL__file_get(void *obj, const H5VL_class_t *cls, H5VL_file_get_t get_type, hid_t dxpl_id,
                             void **req, va_list arguments);
static herr_t H5VL__file_specific(void *obj, const H5VL_class_t *cls, H5VL_file_specific_t specific_type,
                                  hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL__file_optional(void *obj, const H5VL_class_t *cls, H5VL_file_optional_t opt_type,
                                  hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL__file_close(void *obj, const H5VL_class_t *cls, hid_t dxpl_id, void **req);
static void * H5VL__group_create(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                                 const char *name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id,
                                 void **req);
static void * H5VL__group_open(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                               const char *name, hid_t gapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL__group_get(void *obj, const H5VL_class_t *cls, H5VL_group_get_t get_type, hid_t dxpl_id,
                              void **req, va_list arguments);
static herr_t H5VL__group_specific(void *obj, const H5VL_class_t *cls, H5VL_group_specific_t specific_type,
                                   hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL__group_optional(void *obj, const H5VL_class_t *cls, H5VL_group_optional_t opt_type,
                                   hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL__group_close(void *obj, const H5VL_class_t *cls, hid_t dxpl_id, void **req);
static herr_t H5VL__link_create(H5VL_link_create_type_t create_type, void *obj,
                                const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls, hid_t lcpl_id,
                                hid_t lapl_id, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL__link_copy(void *src_obj, const H5VL_loc_params_t *loc_params1, void *dst_obj,
                              const H5VL_loc_params_t *loc_params2, const H5VL_class_t *cls, hid_t lcpl_id,
                              hid_t lapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL__link_move(void *src_obj, const H5VL_loc_params_t *loc_params1, void *dst_obj,
                              const H5VL_loc_params_t *loc_params2, const H5VL_class_t *cls, hid_t lcpl_id,
                              hid_t lapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL__link_get(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                             H5VL_link_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL__link_specific(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                                  H5VL_link_specific_t specific_type, hid_t dxpl_id, void **req,
                                  va_list arguments);
static herr_t H5VL__link_optional(void *obj, const H5VL_class_t *cls, H5VL_link_optional_t opt_type,
                                  hid_t dxpl_id, void **req, va_list arguments);
static void * H5VL__object_open(void *obj, const H5VL_loc_params_t *params, const H5VL_class_t *cls,
                                H5I_type_t *opened_type, hid_t dxpl_id, void **req);
static herr_t H5VL__object_copy(void *src_obj, const H5VL_loc_params_t *src_loc_params, const char *src_name,
                                void *dst_obj, const H5VL_loc_params_t *dst_loc_params, const char *dst_name,
                                const H5VL_class_t *cls, hid_t ocpypl_id, hid_t lcpl_id, hid_t dxpl_id,
                                void **req);
static herr_t H5VL__object_get(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                               H5VL_object_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL__object_specific(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                                    H5VL_object_specific_t specific_type, hid_t dxpl_id, void **req,
                                    va_list arguments);
static herr_t H5VL__object_optional(void *obj, const H5VL_class_t *cls, H5VL_object_optional_t opt_type,
                                    hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL__introspect_get_conn_cls(void *obj, const H5VL_class_t *cls, H5VL_get_conn_lvl_t lvl,
                                            const H5VL_class_t **conn_cls);
static herr_t H5VL__introspect_opt_query(void *obj, const H5VL_class_t *cls, H5VL_subclass_t subcls,
                                         int opt_type, hbool_t *supported);
static herr_t H5VL__request_wait(void *req, const H5VL_class_t *cls, uint64_t timeout, H5ES_status_t *status);
static herr_t H5VL__request_notify(void *req, const H5VL_class_t *cls, H5VL_request_notify_t cb, void *ctx);
static herr_t H5VL__request_cancel(void *req, const H5VL_class_t *cls);
static herr_t H5VL__request_specific(void *req, const H5VL_class_t *cls,
                                     H5VL_request_specific_t specific_type, va_list arguments);
static herr_t H5VL__request_optional(void *req, const H5VL_class_t *cls, H5VL_request_optional_t opt_type,
                                     va_list arguments);
static herr_t H5VL__request_free(void *req, const H5VL_class_t *cls);
static herr_t H5VL__blob_put(void *obj, const H5VL_class_t *cls, const void *buf, size_t size, void *blob_id,
                             void *ctx);
static herr_t H5VL__blob_get(void *obj, const H5VL_class_t *cls, const void *blob_id, void *buf, size_t size,
                             void *ctx);
static herr_t H5VL__blob_specific(void *obj, const H5VL_class_t *cls, void *blob_id,
                                  H5VL_blob_specific_t specific_type, va_list arguments);
static herr_t H5VL__blob_optional(void *obj, const H5VL_class_t *cls, void *blob_id,
                                  H5VL_blob_optional_t opt_type, va_list arguments);
static herr_t H5VL__token_cmp(void *obj, const H5VL_class_t *cls, const H5O_token_t *token1,
                              const H5O_token_t *token2, int *cmp_value);
static herr_t H5VL__token_to_str(void *obj, H5I_type_t obj_type, const H5VL_class_t *cls,
                                 const H5O_token_t *token, char **token_str);
static herr_t H5VL__token_from_str(void *obj, H5I_type_t obj_type, const H5VL_class_t *cls,
                                   const char *token_str, H5O_token_t *token);
static herr_t H5VL__optional(void *obj, const H5VL_class_t *cls, int op_type, hid_t dxpl_id, void **req,
                             va_list arguments);

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
 * Function:    H5VLinitialize
 *
 * Purpose:     Calls the connector-specific callback to initialize the connector.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLinitialize(hid_t connector_id, hid_t vipl_id)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE2("e", "ii", connector_id, vipl_id);

    /* Check args */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Invoke class' callback, if there is one */
    if (cls->initialize && cls->initialize(vipl_id) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEOBJ, FAIL, "VOL connector did not initialize")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLinitialize() */

/*-------------------------------------------------------------------------
 * Function:    H5VLterminate
 *
 * Purpose:     Calls the connector-specific callback to terminate the connector.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLterminate(hid_t connector_id)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE1("e", "i", connector_id);

    /* Check args */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Invoke class' callback, if there is one */
    if (cls->terminate && cls->terminate() < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEOBJ, FAIL, "VOL connector did not terminate cleanly")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLterminate() */

/*---------------------------------------------------------------------------
 * Function:    H5VLget_cap_flags
 *
 * Purpose:     Retrieves the capability flag for a connector
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VLget_cap_flags(hid_t connector_id, unsigned *cap_flags)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE2("e", "i*Iu", connector_id, cap_flags);

    /* Check args */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Retrieve capability flags */
    if (cap_flags)
        *cap_flags = cls->cap_flags;

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* H5VLget_cap_flags */

/*---------------------------------------------------------------------------
 * Function:    H5VLget_value
 *
 * Purpose:     Retrieves the 'value' for a connector
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VLget_value(hid_t connector_id, H5VL_class_value_t *value)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE2("e", "i*VC", connector_id, value);

    /* Check args */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Retrieve connector value */
    if (value)
        *value = cls->value;

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* H5VLget_value */

/*-------------------------------------------------------------------------
 * Function:    H5VL_copy_connector_info
 *
 * Purpose:     Copy the VOL info for a connector
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_copy_connector_info(const H5VL_class_t *connector, void **dst_info, const void *src_info)
{
    void * new_connector_info = NULL;    /* Copy of connector info */
    herr_t ret_value          = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(connector);

    /* Check for actual source info */
    if (src_info) {
        /* Allow the connector to copy or do it ourselves */
        if (connector->info_cls.copy) {
            if (NULL == (new_connector_info = (connector->info_cls.copy)(src_info)))
                HGOTO_ERROR(H5E_VOL, H5E_CANTCOPY, FAIL, "connector info copy callback failed")
        } /* end if */
        else if (connector->info_cls.size > 0) {
            if (NULL == (new_connector_info = H5MM_malloc(connector->info_cls.size)))
                HGOTO_ERROR(H5E_VOL, H5E_CANTALLOC, FAIL, "connector info allocation failed")
            H5MM_memcpy(new_connector_info, src_info, connector->info_cls.size);
        } /* end else-if */
        else
            HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "no way to copy connector info")
    } /* end if */

    /* Set the connector info for the copy */
    *dst_info = new_connector_info;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_copy_connector_info() */

/*---------------------------------------------------------------------------
 * Function:    H5VLcopy_connector_info
 *
 * Purpose:     Copies a VOL connector's info object
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VLcopy_connector_info(hid_t connector_id, void **dst_vol_info, void *src_vol_info)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE3("e", "i**x*x", connector_id, dst_vol_info, src_vol_info);

    /* Check args and get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Copy the VOL connector's info object */
    if (H5VL_copy_connector_info(cls, dst_vol_info, src_vol_info) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCOPY, FAIL, "unable to copy VOL connector info object")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* H5VLcopy_connector_info() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_cmp_connector_info
 *
 * Purpose:     Compare VOL info for a connector.  Sets *cmp_value to
 *              positive if INFO1 is greater than INFO2, negative if
 *              INFO2 is greater than INFO1 and zero if INFO1 and
 *              INFO2 are equal.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_cmp_connector_info(const H5VL_class_t *connector, int *cmp_value, const void *info1, const void *info2)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(connector);
    HDassert(cmp_value);

    /* Take care of cases where one or both pointers is NULL */
    if (info1 == NULL && info2 != NULL) {
        *cmp_value = -1;
        HGOTO_DONE(SUCCEED);
    } /* end if */
    if (info1 != NULL && info2 == NULL) {
        *cmp_value = 1;
        HGOTO_DONE(SUCCEED);
    } /* end if */
    if (info1 == NULL && info2 == NULL) {
        *cmp_value = 0;
        HGOTO_DONE(SUCCEED);
    } /* end if */

    /* Use the class's info comparison routine to compare the info objects,
     * if there is a a callback, otherwise just compare the info objects as
     * memory buffers
     */
    if (connector->info_cls.cmp) {
        if ((connector->info_cls.cmp)(cmp_value, info1, info2) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTCOMPARE, FAIL, "can't compare connector info")
    } /* end if */
    else {
        HDassert(connector->info_cls.size > 0);
        *cmp_value = HDmemcmp(info1, info2, connector->info_cls.size);
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_cmp_connector_info() */

/*---------------------------------------------------------------------------
 * Function:    H5VLcmp_connector_info
 *
 * Purpose:     Compares two connector info objects
 *
 * Note:	Both info objects must be from the same VOL connector class
 *
 * Return:      Success:    Non-negative, with *cmp set to positive if
 *			    info1 is greater than info2, negative if info2
 *			    is greater than info1 and zero if info1 and info2
 *			    are equal.
 *              Failure:    Negative
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VLcmp_connector_info(int *cmp, hid_t connector_id, const void *info1, const void *info2)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "*Isi*x*x", cmp, connector_id, info1, info2);

    /* Check args and get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Compare the two VOL connector info objects */
    if (cmp)
        H5VL_cmp_connector_info(cls, cmp, info1, info2);

done:
    FUNC_LEAVE_API(ret_value)
} /* H5VLcmp_connector_info() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_free_connector_info
 *
 * Purpose:     Free VOL info for a connector
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_free_connector_info(hid_t connector_id, const void *info)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(connector_id > 0);

    /* Check args and get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_VOL, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Only free info object, if it's non-NULL */
    if (info) {
        /* Allow the connector to free info or do it ourselves */
        if (cls->info_cls.free) {
            /* Cast through uintptr_t to de-const memory */
            if ((cls->info_cls.free)((void *)(uintptr_t)info) < 0)
                HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "connector info free request failed")
        }
        else
            H5MM_xfree_const(info);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_free_connector_info() */

/*---------------------------------------------------------------------------
 * Function:    H5VLfree_connector_info
 *
 * Purpose:     Free VOL connector info object
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VLfree_connector_info(hid_t connector_id, void *info)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE2("e", "i*x", connector_id, info);

    /* Free the VOL connector info object */
    if (H5VL_free_connector_info(connector_id, info) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "unable to release VOL connector info object")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* H5VLfree_connector_info() */

/*---------------------------------------------------------------------------
 * Function:    H5VLconnector_info_to_str
 *
 * Purpose:     Serialize a connector's info into a string
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VLconnector_info_to_str(const void *info, hid_t connector_id, char **str)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE3("e", "*xi**s", info, connector_id, str);

    /* Only serialize info object, if it's non-NULL */
    if (info) {
        H5VL_class_t *cls; /* VOL connector's class struct */

        /* Check args and get class pointer */
        if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

        /* Allow the connector to serialize info */
        if (cls->info_cls.to_str) {
            if ((cls->info_cls.to_str)(info, str) < 0)
                HGOTO_ERROR(H5E_VOL, H5E_CANTSERIALIZE, FAIL, "can't serialize connector info")
        } /* end if */
        else
            *str = NULL;
    } /* end if */
    else
        *str = NULL;

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* H5VLconnector_info_to_str() */

/*---------------------------------------------------------------------------
 * Function:    H5VLconnector_str_to_info
 *
 * Purpose:     Deserialize a string into a connector's info
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VLconnector_str_to_info(const char *str, hid_t connector_id, void **info)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE3("e", "*si**x", str, connector_id, info);

    /* Call internal routine */
    if (H5VL__connector_str_to_info(str, connector_id, info) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTDECODE, FAIL, "can't deserialize connector info")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* H5VLconnector_str_to_info() */

/*---------------------------------------------------------------------------
 * Function:    H5VLget_object
 *
 * Purpose:     Retrieves an underlying object.
 *
 * Return:      Success:    Non-NULL
 *              Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
void *
H5VLget_object(void *obj, hid_t connector_id)
{
    H5VL_class_t *cls;              /* VOL connector's class struct */
    void *        ret_value = NULL; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE2("*x", "*xi", obj, connector_id);

    /* Check args */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Check for 'get_object' callback in connector */
    if (cls->wrap_cls.get_object)
        ret_value = (cls->wrap_cls.get_object)(obj);
    else
        ret_value = obj;

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* H5VLget_object */

/*-------------------------------------------------------------------------
 * Function:    H5VL_get_wrap_ctx
 *
 * Purpose:     Retrieve the VOL object wrapping context for a connector
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_get_wrap_ctx(const H5VL_class_t *connector, void *obj, void **wrap_ctx)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(connector);
    HDassert(obj);
    HDassert(wrap_ctx);

    /* Allow the connector to copy or do it ourselves */
    if (connector->wrap_cls.get_wrap_ctx) {
        /* Sanity check */
        HDassert(connector->wrap_cls.free_wrap_ctx);

        /* Invoke connector's callback */
        if ((connector->wrap_cls.get_wrap_ctx)(obj, wrap_ctx) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "connector wrap context callback failed")
    } /* end if */
    else
        *wrap_ctx = NULL;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_get_wrap_ctx() */

/*---------------------------------------------------------------------------
 * Function:    H5VLget_wrap_ctx
 *
 * Purpose:     Get a VOL connector's object wrapping context
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VLget_wrap_ctx(void *obj, hid_t connector_id, void **wrap_ctx)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE3("e", "*xi**x", obj, connector_id, wrap_ctx);

    /* Check args and get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Get the VOL connector's object wrapper */
    if (H5VL_get_wrap_ctx(cls, obj, wrap_ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "unable to retrieve VOL connector object wrap context")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* H5VLget_wrap_ctx() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_wrap_object
 *
 * Purpose:     Wrap an object with connector
 *
 * Return:      Success:    Non-NULL
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_wrap_object(const H5VL_class_t *connector, void *wrap_ctx, void *obj, H5I_type_t obj_type)
{
    void *ret_value = NULL; /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Sanity checks */
    HDassert(connector);
    HDassert(obj);

    /* Only wrap object if there's a wrap context */
    if (wrap_ctx) {
        /* Ask the connector to wrap the object */
        if (NULL == (ret_value = (connector->wrap_cls.wrap_object)(obj, obj_type, wrap_ctx)))
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, NULL, "can't wrap object")
    } /* end if */
    else
        ret_value = obj;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_wrap_object() */

/*---------------------------------------------------------------------------
 * Function:    H5VLwrap_object
 *
 * Purpose:     Asks a connector to wrap an underlying object.
 *
 * Return:      Success:    Non-NULL
 *              Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
void *
H5VLwrap_object(void *obj, H5I_type_t obj_type, hid_t connector_id, void *wrap_ctx)
{
    H5VL_class_t *cls;              /* VOL connector's class struct */
    void *        ret_value = NULL; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE4("*x", "*xIti*x", obj, obj_type, connector_id, wrap_ctx);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Wrap the object */
    if (NULL == (ret_value = H5VL_wrap_object(cls, wrap_ctx, obj, obj_type)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, NULL, "unable to wrap object")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* H5VLwrap_object */

/*-------------------------------------------------------------------------
 * Function:    H5VL_unwrap_object
 *
 * Purpose:     Unwrap an object from connector
 *
 * Return:      Success:    Non-NULL
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_unwrap_object(const H5VL_class_t *connector, void *obj)
{
    void *ret_value = NULL; /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Sanity checks */
    HDassert(connector);
    HDassert(obj);

    /* Only unwrap object if there's an unwrap callback */
    if (connector->wrap_cls.wrap_object) {
        /* Ask the connector to unwrap the object */
        if (NULL == (ret_value = (connector->wrap_cls.unwrap_object)(obj)))
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, NULL, "can't unwrap object")
    } /* end if */
    else
        ret_value = obj;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_unwrap_object() */

/*---------------------------------------------------------------------------
 * Function:    H5VLunwrap_object
 *
 * Purpose:     Unwrap an object from connector
 *
 * Return:      Success:    Non-NULL
 *              Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
void *
H5VLunwrap_object(void *obj, hid_t connector_id)
{
    H5VL_class_t *cls;              /* VOL connector's class struct */
    void *        ret_value = NULL; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE2("*x", "*xi", obj, connector_id);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Unwrap the object */
    if (NULL == (ret_value = H5VL_unwrap_object(cls, obj)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, NULL, "unable to unwrap object")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* H5VLunwrap_object */

/*-------------------------------------------------------------------------
 * Function:    H5VL_free_wrap_ctx
 *
 * Purpose:     Free object wrapping context for a connector
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_free_wrap_ctx(const H5VL_class_t *connector, void *wrap_ctx)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(connector);

    /* Only free wrap context, if it's non-NULL */
    if (wrap_ctx) {
        /* Free the connector's object wrapping context */
        if ((connector->wrap_cls.free_wrap_ctx)(wrap_ctx) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "connector wrap context free request failed")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_free_wrap_ctx() */

/*---------------------------------------------------------------------------
 * Function:    H5VLfree_wrap_ctx
 *
 * Purpose:     Release a VOL connector's object wrapping context
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VLfree_wrap_ctx(void *wrap_ctx, hid_t connector_id)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE2("e", "*xi", wrap_ctx, connector_id);

    /* Check args and get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Release the VOL connector's object wrapper */
    if (H5VL_free_wrap_ctx(cls, wrap_ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "unable to release VOL connector object wrap context")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* H5VLfree_wrap_ctx() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__attr_create
 *
 * Purpose:     Creates an attribute through the VOL
 *
 * Return:      Success: Pointer to the new attribute
 *              Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL__attr_create(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls, const char *name,
                  hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id, hid_t dxpl_id, void **req)
{
    void *ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->attr_cls.create)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, NULL, "VOL connector has no 'attr create' method")

    /* Call the corresponding VOL callback */
    if (NULL == (ret_value = (cls->attr_cls.create)(obj, loc_params, name, type_id, space_id, acpl_id,
                                                    aapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "attribute create failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__attr_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_attr_create
 *
 * Purpose:     Creates an attribute through the VOL
 *
 * Return:      Success: Pointer to the new attribute
 *              Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_attr_create(const H5VL_object_t *vol_obj, const H5VL_loc_params_t *loc_params, const char *name,
                 hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id, hid_t dxpl_id, void **req)
{
    hbool_t vol_wrapper_set = FALSE; /* Whether the VOL object wrapping context was set up */
    void *  ret_value       = NULL;  /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, NULL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__attr_create(vol_obj->data, loc_params, vol_obj->connector->cls, name,
                                               type_id, space_id, acpl_id, aapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "attribute create failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, NULL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_attr_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VLattr_create
 *
 * Purpose:     Creates an attribute
 *
 * Return:      Success:    Pointer to the new attribute
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VLattr_create(void *obj, const H5VL_loc_params_t *loc_params, hid_t connector_id, const char *name,
                hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;              /* VOL connector's class struct */
    void *        ret_value = NULL; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE10("*x", "*x*xi*siiiii**x", obj, loc_params, connector_id, name, type_id, space_id, acpl_id,
              aapl_id, dxpl_id, req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__attr_create(obj, loc_params, cls, name, type_id, space_id, acpl_id,
                                               aapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "unable to create attribute")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLattr_create() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__attr_open
 *
 * Purpose:	Opens an attribute through the VOL
 *
 * Return:      Success: Pointer to the attribute
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL__attr_open(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls, const char *name,
                hid_t aapl_id, hid_t dxpl_id, void **req)
{
    void *ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->attr_cls.open)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, NULL, "VOL connector has no 'attr open' method")

    /* Call the corresponding VOL open callback */
    if (NULL == (ret_value = (cls->attr_cls.open)(obj, loc_params, name, aapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "attribute open failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__attr_open() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_attr_open
 *
 * Purpose:	Opens an attribute through the VOL
 *
 * Return:      Success: Pointer to the attribute
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_attr_open(const H5VL_object_t *vol_obj, const H5VL_loc_params_t *loc_params, const char *name,
               hid_t aapl_id, hid_t dxpl_id, void **req)
{
    hbool_t vol_wrapper_set = FALSE; /* Whether the VOL object wrapping context was set up */
    void *  ret_value       = NULL;  /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, NULL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__attr_open(vol_obj->data, loc_params, vol_obj->connector->cls, name,
                                             aapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "attribute open failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, NULL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_attr_open() */

/*-------------------------------------------------------------------------
 * Function:	H5VLattr_open
 *
 * Purpose:     Opens an attribute
 *
 * Return:      Success:    Pointer to the attribute
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VLattr_open(void *obj, const H5VL_loc_params_t *loc_params, hid_t connector_id, const char *name,
              hid_t aapl_id, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;              /* VOL connector's class struct */
    void *        ret_value = NULL; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE7("*x", "*x*xi*sii**x", obj, loc_params, connector_id, name, aapl_id, dxpl_id, req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__attr_open(obj, loc_params, cls, name, aapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "unable to open attribute")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLattr_open() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__attr_read
 *
 * Purpose:	Reads data from attr through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__attr_read(void *obj, const H5VL_class_t *cls, hid_t mem_type_id, void *buf, hid_t dxpl_id, void **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->attr_cls.read)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'attr read' method")

    /* Call the corresponding VOL callback */
    if ((cls->attr_cls.read)(obj, mem_type_id, buf, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_READERROR, FAIL, "attribute read failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__attr_read() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_attr_read
 *
 * Purpose:	Reads data from attr through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_attr_read(const H5VL_object_t *vol_obj, hid_t mem_type_id, void *buf, hid_t dxpl_id, void **req)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__attr_read(vol_obj->data, vol_obj->connector->cls, mem_type_id, buf, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_READERROR, FAIL, "attribute read failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_attr_read() */

/*-------------------------------------------------------------------------
 * Function:    H5VLattr_read
 *
 * Purpose:     Reads data from an attribute
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLattr_read(void *obj, hid_t connector_id, hid_t mem_type_id, void *buf, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xii*xi**x", obj, connector_id, mem_type_id, buf, dxpl_id, req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__attr_read(obj, cls, mem_type_id, buf, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_READERROR, FAIL, "unable to read attribute")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLattr_read() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__attr_write
 *
 * Purpose:	Writes data to attr through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__attr_write(void *obj, const H5VL_class_t *cls, hid_t mem_type_id, const void *buf, hid_t dxpl_id,
                 void **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->attr_cls.write)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'attr write' method")

    /* Call the corresponding VOL callback */
    if ((cls->attr_cls.write)(obj, mem_type_id, buf, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_WRITEERROR, FAIL, "write failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__attr_write() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_attr_write
 *
 * Purpose:	Writes data to attr through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_attr_write(const H5VL_object_t *vol_obj, hid_t mem_type_id, const void *buf, hid_t dxpl_id, void **req)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__attr_write(vol_obj->data, vol_obj->connector->cls, mem_type_id, buf, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_WRITEERROR, FAIL, "write failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_attr_write() */

/*-------------------------------------------------------------------------
 * Function:    H5VLattr_write
 *
 * Purpose:     Writes data to an attribute
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLattr_write(void *obj, hid_t connector_id, hid_t mem_type_id, const void *buf, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xii*xi**x", obj, connector_id, mem_type_id, buf, dxpl_id, req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__attr_write(obj, cls, mem_type_id, buf, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_WRITEERROR, FAIL, "unable to write attribute")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLattr_write() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__attr_get
 *
 * Purpose:	Get specific information about the attribute through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__attr_get(void *obj, const H5VL_class_t *cls, H5VL_attr_get_t get_type, hid_t dxpl_id, void **req,
               va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->attr_cls.get)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'attr get' method")

    /* Call the corresponding VOL callback */
    if ((cls->attr_cls.get)(obj, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "attribute get failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__attr_get() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_attr_get
 *
 * Purpose:	Get specific information about the attribute through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_attr_get(const H5VL_object_t *vol_obj, H5VL_attr_get_t get_type, hid_t dxpl_id, void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if (H5VL__attr_get(vol_obj->data, vol_obj->connector->cls, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "attribute get failed")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_attr_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VLattr_get
 *
 * Purpose:     Gets information about the attribute
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLattr_get(void *obj, hid_t connector_id, H5VL_attr_get_t get_type, hid_t dxpl_id, void **req,
             va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVai**xx", obj, connector_id, get_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__attr_get(obj, cls, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "unable to get attribute information")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLattr_get() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__attr_specific
 *
 * Purpose:	Specific operation on attributes through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__attr_specific(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                    H5VL_attr_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->attr_cls.specific)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'attr specific' method")

    /* Call the corresponding VOL callback */
    if ((ret_value = (cls->attr_cls.specific)(obj, loc_params, specific_type, dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute attribute specific callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__attr_specific() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_attr_specific
 *
 * Purpose:	Specific operation on attributes through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_attr_specific(const H5VL_object_t *vol_obj, const H5VL_loc_params_t *loc_params,
                   H5VL_attr_specific_t specific_type, hid_t dxpl_id, void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if ((ret_value = H5VL__attr_specific(vol_obj->data, loc_params, vol_obj->connector->cls, specific_type,
                                         dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute attribute specific callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_attr_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VLattr_specific
 *
 * Purpose:     Performs a connector-specific operation on an attribute
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLattr_specific(void *obj, const H5VL_loc_params_t *loc_params, hid_t connector_id,
                  H5VL_attr_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE7("e", "*x*xiVbi**xx", obj, loc_params, connector_id, specific_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if ((ret_value = H5VL__attr_specific(obj, loc_params, cls, specific_type, dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute attribute specific callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLattr_specific() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__attr_optional
 *
 * Purpose:	Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__attr_optional(void *obj, const H5VL_class_t *cls, H5VL_attr_optional_t opt_type, hid_t dxpl_id,
                    void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->attr_cls.optional)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'attr optional' method")

    /* Call the corresponding VOL callback */
    if ((ret_value = (cls->attr_cls.optional)(obj, opt_type, dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute attribute optional callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__attr_optional() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_attr_optional
 *
 * Purpose:	Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_attr_optional(const H5VL_object_t *vol_obj, H5VL_attr_optional_t opt_type, hid_t dxpl_id, void **req,
                   ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if ((ret_value = H5VL__attr_optional(vol_obj->data, vol_obj->connector->cls, opt_type, dxpl_id, req,
                                         arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute attribute optional callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_attr_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VLattr_optional
 *
 * Purpose:     Performs an optional connector-specific operation on an attribute
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLattr_optional(void *obj, hid_t connector_id, H5VL_attr_optional_t opt_type, hid_t dxpl_id, void **req,
                  va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVsi**xx", obj, connector_id, opt_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if ((ret_value = H5VL__attr_optional(obj, cls, opt_type, dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute attribute optional callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLattr_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__attr_close
 *
 * Purpose:     Closes an attribute through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__attr_close(void *obj, const H5VL_class_t *cls, hid_t dxpl_id, void **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->attr_cls.close)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'attr close' method")

    /* Call the corresponding VOL callback */
    if ((cls->attr_cls.close)(obj, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEOBJ, FAIL, "attribute close failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__attr_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_attr_close
 *
 * Purpose:     Closes an attribute through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_attr_close(const H5VL_object_t *vol_obj, hid_t dxpl_id, void **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(vol_obj);

    /* Call the corresponding internal VOL routine */
    if (H5VL__attr_close(vol_obj->data, vol_obj->connector->cls, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEOBJ, FAIL, "attribute close failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_attr_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VLattr_close
 *
 * Purpose:     Closes an attribute
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLattr_close(void *obj, hid_t connector_id, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE4("e", "*xii**x", obj, connector_id, dxpl_id, req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__attr_close(obj, cls, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEOBJ, FAIL, "unable to close attribute")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLattr_close() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__dataset_create
 *
 * Purpose:	Creates a dataset through the VOL
 *
 * Return:      Success: Pointer to new dataset
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL__dataset_create(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                     const char *name, hid_t lcpl_id, hid_t type_id, hid_t space_id, hid_t dcpl_id,
                     hid_t dapl_id, hid_t dxpl_id, void **req)
{
    void *ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->dataset_cls.create)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, NULL, "VOL connector has no 'dataset create' method")

    /* Call the corresponding VOL callback */
    if (NULL == (ret_value = (cls->dataset_cls.create)(obj, loc_params, name, lcpl_id, type_id, space_id,
                                                       dcpl_id, dapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "dataset create failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__dataset_create() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_dataset_create
 *
 * Purpose:	Creates a dataset through the VOL
 *
 * Return:      Success: Pointer to new dataset
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_dataset_create(const H5VL_object_t *vol_obj, const H5VL_loc_params_t *loc_params, const char *name,
                    hid_t lcpl_id, hid_t type_id, hid_t space_id, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id,
                    void **req)
{
    hbool_t vol_wrapper_set = FALSE; /* Whether the VOL object wrapping context was set up */
    void *  ret_value       = NULL;  /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, NULL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (NULL ==
        (ret_value = H5VL__dataset_create(vol_obj->data, loc_params, vol_obj->connector->cls, name, lcpl_id,
                                          type_id, space_id, dcpl_id, dapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "dataset create failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, NULL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_dataset_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VLdataset_create
 *
 * Purpose:     Creates a dataset
 *
 * Return:      Success:    Pointer to the new dataset
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VLdataset_create(void *obj, const H5VL_loc_params_t *loc_params, hid_t connector_id, const char *name,
                   hid_t lcpl_id, hid_t type_id, hid_t space_id, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id,
                   void **req)
{
    H5VL_class_t *cls;              /* VOL connector's class struct */
    void *        ret_value = NULL; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE11("*x", "*x*xi*siiiiii**x", obj, loc_params, connector_id, name, lcpl_id, type_id, space_id,
              dcpl_id, dapl_id, dxpl_id, req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__dataset_create(obj, loc_params, cls, name, lcpl_id, type_id, space_id,
                                                  dcpl_id, dapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "unable to create dataset")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLdataset_create() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__dataset_open
 *
 * Purpose:	Opens a dataset through the VOL
 *
 * Return:      Success: Pointer to dataset
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL__dataset_open(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls, const char *name,
                   hid_t dapl_id, hid_t dxpl_id, void **req)
{
    void *ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->dataset_cls.open)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, NULL, "VOL connector has no 'dataset open' method")

    /* Call the corresponding VOL callback */
    if (NULL == (ret_value = (cls->dataset_cls.open)(obj, loc_params, name, dapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "dataset open failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__dataset_open() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_dataset_open
 *
 * Purpose:	Opens a dataset through the VOL
 *
 * Return:      Success: Pointer to dataset
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_dataset_open(const H5VL_object_t *vol_obj, const H5VL_loc_params_t *loc_params, const char *name,
                  hid_t dapl_id, hid_t dxpl_id, void **req)
{
    hbool_t vol_wrapper_set = FALSE; /* Whether the VOL object wrapping context was set up */
    void *  ret_value       = NULL;  /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, NULL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__dataset_open(vol_obj->data, loc_params, vol_obj->connector->cls, name,
                                                dapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "dataset open failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, NULL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_dataset_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VLdataset_open
 *
 * Purpose:     Opens a dataset
 *
 * Return:      Success:    Pointer to the new dataset
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VLdataset_open(void *obj, const H5VL_loc_params_t *loc_params, hid_t connector_id, const char *name,
                 hid_t dapl_id, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;              /* VOL connector's class struct */
    void *        ret_value = NULL; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE7("*x", "*x*xi*sii**x", obj, loc_params, connector_id, name, dapl_id, dxpl_id, req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__dataset_open(obj, loc_params, cls, name, dapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "unable to open dataset")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLdataset_open() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__dataset_read
 *
 * Purpose:	Reads data from dataset through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__dataset_read(void *obj, const H5VL_class_t *cls, hid_t mem_type_id, hid_t mem_space_id,
                   hid_t file_space_id, hid_t dxpl_id, void *buf, void **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->dataset_cls.read)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'dataset read' method")

    /* Call the corresponding VOL callback */
    if ((cls->dataset_cls.read)(obj, mem_type_id, mem_space_id, file_space_id, dxpl_id, buf, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_READERROR, FAIL, "dataset read failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__dataset_read() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_dataset_read
 *
 * Purpose:	Reads data from dataset through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_dataset_read(const H5VL_object_t *vol_obj, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id,
                  hid_t dxpl_id, void *buf, void **req)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__dataset_read(vol_obj->data, vol_obj->connector->cls, mem_type_id, mem_space_id, file_space_id,
                           dxpl_id, buf, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_READERROR, FAIL, "dataset read failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_dataset_read() */

/*-------------------------------------------------------------------------
 * Function:    H5VLdataset_read
 *
 * Purpose:     Reads data from a dataset
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLdataset_read(void *obj, hid_t connector_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id,
                 hid_t dxpl_id, void *buf, void **req)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE8("e", "*xiiiii*x**x", obj, connector_id, mem_type_id, mem_space_id, file_space_id, dxpl_id, buf,
             req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__dataset_read(obj, cls, mem_type_id, mem_space_id, file_space_id, dxpl_id, buf, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, FAIL, "unable to read dataset")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLdataset_read() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__dataset_write
 *
 * Purpose:	Writes data from dataset through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__dataset_write(void *obj, const H5VL_class_t *cls, hid_t mem_type_id, hid_t mem_space_id,
                    hid_t file_space_id, hid_t dxpl_id, const void *buf, void **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->dataset_cls.write)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'dataset write' method")

    /* Call the corresponding VOL callback */
    if ((cls->dataset_cls.write)(obj, mem_type_id, mem_space_id, file_space_id, dxpl_id, buf, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_WRITEERROR, FAIL, "dataset write failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__dataset_write() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_dataset_write
 *
 * Purpose:	Writes data from dataset through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_dataset_write(const H5VL_object_t *vol_obj, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id,
                   hid_t dxpl_id, const void *buf, void **req)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__dataset_write(vol_obj->data, vol_obj->connector->cls, mem_type_id, mem_space_id, file_space_id,
                            dxpl_id, buf, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_WRITEERROR, FAIL, "dataset write failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_dataset_write() */

/*-------------------------------------------------------------------------
 * Function:    H5VLdataset_write
 *
 * Purpose:     Writes data to a dataset
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLdataset_write(void *obj, hid_t connector_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id,
                  hid_t dxpl_id, const void *buf, void **req)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE8("e", "*xiiiii*x**x", obj, connector_id, mem_type_id, mem_space_id, file_space_id, dxpl_id, buf,
             req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__dataset_write(obj, cls, mem_type_id, mem_space_id, file_space_id, dxpl_id, buf, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, FAIL, "unable to write dataset")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLdataset_write() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__dataset_get
 *
 * Purpose:	Get specific information about the dataset through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__dataset_get(void *obj, const H5VL_class_t *cls, H5VL_dataset_get_t get_type, hid_t dxpl_id, void **req,
                  va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->dataset_cls.get)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'dataset get' method")

    /* Call the corresponding VOL callback */
    if ((cls->dataset_cls.get)(obj, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "dataset get failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__dataset_get() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_dataset_get
 *
 * Purpose:	Get specific information about the dataset through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_dataset_get(const H5VL_object_t *vol_obj, H5VL_dataset_get_t get_type, hid_t dxpl_id, void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if (H5VL__dataset_get(vol_obj->data, vol_obj->connector->cls, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "dataset get failed")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_dataset_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VLdataset_get
 *
 * Purpose:     Gets information about a dataset
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLdataset_get(void *obj, hid_t connector_id, H5VL_dataset_get_t get_type, hid_t dxpl_id, void **req,
                va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVci**xx", obj, connector_id, get_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__dataset_get(obj, cls, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "unable to execute dataset get callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLdataset_get() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__dataset_specific
 *
 * Purpose:	Specific operation on datasets through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__dataset_specific(void *obj, const H5VL_class_t *cls, H5VL_dataset_specific_t specific_type,
                       hid_t dxpl_id, void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->dataset_cls.specific)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'dataset specific' method")

    /* Call the corresponding VOL callback */
    if ((cls->dataset_cls.specific)(obj, specific_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute dataset specific callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__dataset_specific() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_dataset_specific
 *
 * Purpose:	Specific operation on datasets through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_dataset_specific(const H5VL_object_t *vol_obj, H5VL_dataset_specific_t specific_type, hid_t dxpl_id,
                      void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if (H5VL__dataset_specific(vol_obj->data, vol_obj->connector->cls, specific_type, dxpl_id, req,
                               arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute dataset specific callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_dataset_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VLdataset_specific
 *
 * Purpose:     Performs a connector-specific operation on a dataset
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLdataset_specific(void *obj, hid_t connector_id, H5VL_dataset_specific_t specific_type, hid_t dxpl_id,
                     void **req, va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVdi**xx", obj, connector_id, specific_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__dataset_specific(obj, cls, specific_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute dataset specific callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLdataset_specific() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__dataset_optional
 *
 * Purpose:	Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__dataset_optional(void *obj, const H5VL_class_t *cls, H5VL_dataset_optional_t opt_type, hid_t dxpl_id,
                       void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->dataset_cls.optional)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'dataset optional' method")

    /* Call the corresponding VOL callback */
    if ((cls->dataset_cls.optional)(obj, opt_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute dataset optional callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__dataset_optional() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_dataset_optional
 *
 * Purpose:	Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_dataset_optional(const H5VL_object_t *vol_obj, H5VL_dataset_optional_t opt_type, hid_t dxpl_id,
                      void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if (H5VL__dataset_optional(vol_obj->data, vol_obj->connector->cls, opt_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute dataset optional callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_dataset_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VLdataset_optional
 *
 * Purpose:     Performs an optional connector-specific operation on a dataset
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLdataset_optional(void *obj, hid_t connector_id, H5VL_dataset_optional_t opt_type, hid_t dxpl_id,
                     void **req, va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVti**xx", obj, connector_id, opt_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__dataset_optional(obj, cls, opt_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute dataset optional callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLdataset_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__dataset_close
 *
 * Purpose:     Closes a dataset through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__dataset_close(void *obj, const H5VL_class_t *cls, hid_t dxpl_id, void **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(obj);
    HDassert(cls);

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->dataset_cls.close)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'dataset close' method")

    /* Call the corresponding VOL callback */
    if ((cls->dataset_cls.close)(obj, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEOBJ, FAIL, "dataset close failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__dataset_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_dataset_close
 *
 * Purpose:     Closes a dataset through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_dataset_close(const H5VL_object_t *vol_obj, hid_t dxpl_id, void **req)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(vol_obj);
    HDassert(vol_obj->data);
    HDassert(vol_obj->connector);
    HDassert(vol_obj->connector->cls);

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__dataset_close(vol_obj->data, vol_obj->connector->cls, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEOBJ, FAIL, "dataset close failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_dataset_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VLdataset_close
 *
 * Purpose:     Closes a dataset
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLdataset_close(void *obj, hid_t connector_id, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE4("e", "*xii**x", obj, connector_id, dxpl_id, req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__dataset_close(obj, cls, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEOBJ, FAIL, "unable to close dataset")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLdataset_close() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__datatype_commit
 *
 * Purpose:	Commits a datatype to the file through the VOL
 *
 * Return:      Success:    Pointer to the new datatype
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL__datatype_commit(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                      const char *name, hid_t type_id, hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id,
                      hid_t dxpl_id, void **req)
{
    void *ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->datatype_cls.commit)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, NULL, "VOL connector has no 'datatype commit' method")

    /* Call the corresponding VOL callback */
    if (NULL == (ret_value = (cls->datatype_cls.commit)(obj, loc_params, name, type_id, lcpl_id, tcpl_id,
                                                        tapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "datatype commit failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__datatype_commit() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_datatype_commit
 *
 * Purpose:	Commits a datatype to the file through the VOL
 *
 * Return:      Success:    Pointer to the new datatype
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_datatype_commit(const H5VL_object_t *vol_obj, const H5VL_loc_params_t *loc_params, const char *name,
                     hid_t type_id, hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id, hid_t dxpl_id, void **req)
{
    hbool_t vol_wrapper_set = FALSE; /* Whether the VOL object wrapping context was set up */
    void *  ret_value       = NULL;  /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, NULL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__datatype_commit(vol_obj->data, loc_params, vol_obj->connector->cls, name,
                                                   type_id, lcpl_id, tcpl_id, tapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "datatype commit failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, NULL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_datatype_commit() */

/*-------------------------------------------------------------------------
 * Function:    H5VLdatatype_commit
 *
 * Purpose:     Commits a datatype to the file
 *
 * Return:      Success:    Pointer to the new datatype
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VLdatatype_commit(void *obj, const H5VL_loc_params_t *loc_params, hid_t connector_id, const char *name,
                    hid_t type_id, hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;              /* VOL connector's class struct */
    void *        ret_value = NULL; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE10("*x", "*x*xi*siiiii**x", obj, loc_params, connector_id, name, type_id, lcpl_id, tcpl_id,
              tapl_id, dxpl_id, req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__datatype_commit(obj, loc_params, cls, name, type_id, lcpl_id, tcpl_id,
                                                   tapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "unable to commit datatype")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLdatatype_commit() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__datatype_open
 *
 * Purpose:	Opens a named datatype through the VOL
 *
 * Return:      Success:    Pointer to the datatype
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL__datatype_open(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls, const char *name,
                    hid_t tapl_id, hid_t dxpl_id, void **req)
{
    void *ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->datatype_cls.open)
        HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, NULL, "no datatype open callback")

    /* Call the corresponding VOL callback */
    if (NULL == (ret_value = (cls->datatype_cls.open)(obj, loc_params, name, tapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "datatype open failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__datatype_open() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_datatype_open
 *
 * Purpose:	Opens a named datatype through the VOL
 *
 * Return:      Success:    Pointer to the datatype
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_datatype_open(const H5VL_object_t *vol_obj, const H5VL_loc_params_t *loc_params, const char *name,
                   hid_t tapl_id, hid_t dxpl_id, void **req)
{
    hbool_t vol_wrapper_set = FALSE; /* Whether the VOL object wrapping context was set up */
    void *  ret_value       = NULL;  /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, NULL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__datatype_open(vol_obj->data, loc_params, vol_obj->connector->cls, name,
                                                 tapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "datatype open failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, NULL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_datatype_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VLdatatype_open
 *
 * Purpose:     Opens a named datatype
 *
 * Return:      Success:    Pointer to the datatype
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VLdatatype_open(void *obj, const H5VL_loc_params_t *loc_params, hid_t connector_id, const char *name,
                  hid_t tapl_id, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;              /* VOL connector's class struct */
    void *        ret_value = NULL; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE7("*x", "*x*xi*sii**x", obj, loc_params, connector_id, name, tapl_id, dxpl_id, req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__datatype_open(obj, loc_params, cls, name, tapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "unable to open datatype")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLdatatype_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__datatype_get
 *
 * Purpose:     Get specific information about the datatype through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__datatype_get(void *obj, const H5VL_class_t *cls, H5VL_datatype_get_t get_type, hid_t dxpl_id,
                   void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->datatype_cls.get)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'datatype get' method")

    /* Call the corresponding VOL callback */
    if ((cls->datatype_cls.get)(obj, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "datatype get failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__datatype_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_datatype_get
 *
 * Purpose:     Get specific information about the datatype through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_datatype_get(const H5VL_object_t *vol_obj, H5VL_datatype_get_t get_type, hid_t dxpl_id, void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if (H5VL__datatype_get(vol_obj->data, vol_obj->connector->cls, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "datatype get failed")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_datatype_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VLdatatype_get
 *
 * Purpose:     Gets information about the datatype
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLdatatype_get(void *obj, hid_t connector_id, H5VL_datatype_get_t get_type, hid_t dxpl_id, void **req,
                 va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVei**xx", obj, connector_id, get_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->datatype_cls.get)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no `datatype get' method")

    /* Call the corresponding internal VOL routine */
    if (H5VL__datatype_get(obj, cls, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "unable to execute datatype get callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLdatatype_get() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__datatype_specific
 *
 * Purpose:	Specific operation on datatypes through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__datatype_specific(void *obj, const H5VL_class_t *cls, H5VL_datatype_specific_t specific_type,
                        hid_t dxpl_id, void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->datatype_cls.specific)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'datatype specific' method")

    /* Call the corresponding VOL callback */
    if ((cls->datatype_cls.specific)(obj, specific_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute datatype specific callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__datatype_specific() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_datatype_specific
 *
 * Purpose:	Specific operation on datatypes through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_datatype_specific(const H5VL_object_t *vol_obj, H5VL_datatype_specific_t specific_type, hid_t dxpl_id,
                       void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if (H5VL__datatype_specific(vol_obj->data, vol_obj->connector->cls, specific_type, dxpl_id, req,
                                arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute datatype specific callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_datatype_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VLdatatype_specific
 *
 * Purpose:     Performs a connector-specific operation on a datatype
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLdatatype_specific(void *obj, hid_t connector_id, H5VL_datatype_specific_t specific_type, hid_t dxpl_id,
                      void **req, va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVfi**xx", obj, connector_id, specific_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__datatype_specific(obj, cls, specific_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute datatype specific callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLdatatype_specific() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__datatype_optional
 *
 * Purpose:	Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__datatype_optional(void *obj, const H5VL_class_t *cls, H5VL_datatype_optional_t opt_type, hid_t dxpl_id,
                        void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->datatype_cls.optional)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'datatype optional' method")

    /* Call the corresponding VOL callback */
    if ((cls->datatype_cls.optional)(obj, opt_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute datatype optional callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__datatype_optional() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_datatype_optional
 *
 * Purpose:	Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_datatype_optional(const H5VL_object_t *vol_obj, H5VL_datatype_optional_t opt_type, hid_t dxpl_id,
                       void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if (H5VL__datatype_optional(vol_obj->data, vol_obj->connector->cls, opt_type, dxpl_id, req, arguments) <
        0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute datatype optional callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_datatype_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VLdatatype_optional
 *
 * Purpose:     Performs an optional connector-specific operation on a datatype
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLdatatype_optional(void *obj, hid_t connector_id, H5VL_datatype_optional_t opt_type, hid_t dxpl_id,
                      void **req, va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVui**xx", obj, connector_id, opt_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__datatype_optional(obj, cls, opt_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute datatype optional callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLdatatype_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__datatype_close
 *
 * Purpose:     Closes a datatype through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__datatype_close(void *obj, const H5VL_class_t *cls, hid_t dxpl_id, void **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->datatype_cls.close)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'datatype close' method")

    /* Call the corresponding VOL callback */
    if ((cls->datatype_cls.close)(obj, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEOBJ, FAIL, "datatype close failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__datatype_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_datatype_close
 *
 * Purpose:     Closes a datatype through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_datatype_close(const H5VL_object_t *vol_obj, hid_t dxpl_id, void **req)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__datatype_close(vol_obj->data, vol_obj->connector->cls, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEOBJ, FAIL, "datatype close failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_datatype_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VLdatatype_close
 *
 * Purpose:     Closes a datatype
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLdatatype_close(void *obj, hid_t connector_id, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE4("e", "*xii**x", obj, connector_id, dxpl_id, req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__datatype_close(obj, cls, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEOBJ, FAIL, "unable to close datatype")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLdatatype_close() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__file_create
 *
 * Purpose:	Creates a file through the VOL
 *
 * Note:	Does not have a 'static' version of the routine, since there's
 *		no objects in the container before this operation completes.
 *
 * Return:      Success: Pointer to new file
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL__file_create(const H5VL_class_t *cls, const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id,
                  hid_t dxpl_id, void **req)
{
    void *ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->file_cls.create)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, NULL, "VOL connector has no 'file create' method")

    /* Call the corresponding VOL callback */
    if (NULL == (ret_value = (cls->file_cls.create)(name, flags, fcpl_id, fapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "file create failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__file_create() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_file_create
 *
 * Purpose:	Creates a file through the VOL
 *
 * Note:	Does not have a 'static' version of the routine, since there's
 *		no objects in the container before this operation completes.
 *
 * Return:      Success: Pointer to new file
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_file_create(const H5VL_connector_prop_t *connector_prop, const char *name, unsigned flags, hid_t fcpl_id,
                 hid_t fapl_id, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;              /* VOL Class structure for callback info    */
    void *        ret_value = NULL; /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Get the connector's class */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_prop->connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__file_create(cls, name, flags, fcpl_id, fapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "file create failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_file_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VLfile_create
 *
 * Purpose:     Creates a file
 *
 * Return:      Success:    Pointer to the new file
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VLfile_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req)
{
    H5P_genplist_t *      plist;            /* Property list pointer */
    H5VL_connector_prop_t connector_prop;   /* Property for VOL connector ID & info */
    H5VL_class_t *        cls;              /* VOL connector's class struct */
    void *                ret_value = NULL; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("*x", "*sIuiii**x", name, flags, fcpl_id, fapl_id, dxpl_id, req);

    /* Get the VOL info from the fapl */
    if (NULL == (plist = (H5P_genplist_t *)H5I_object(fapl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list")
    if (H5P_peek(plist, H5F_ACS_VOL_CONN_NAME, &connector_prop) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get VOL connector info")

    /* Get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_prop.connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__file_create(cls, name, flags, fcpl_id, fapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "unable to create file")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLfile_create() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__file_open
 *
 * Purpose:	Opens a file through the VOL.
 *
 * Return:      Success: Pointer to file.
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL__file_open(const H5VL_class_t *cls, const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id,
                void **req)
{
    void *ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->file_cls.open)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, NULL, "VOL connector has no 'file open' method")

    /* Call the corresponding VOL callback */
    if (NULL == (ret_value = (cls->file_cls.open)(name, flags, fapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "open failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__file_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__file_open_find_connector_cb
 *
 * Purpose:     Iteration callback for H5PL_iterate that tries to find the
 *              correct VOL connector to open a file with when
 *              H5VL_file_open fails for its given VOL connector. Iterates
 *              through all of the available VOL connector plugins until
 *              H5Fis_accessible returns true for the given filename and
 *              VOL connector.
 *
 * Return:      H5_ITER_CONT if current plugin can't open the given file
 *              H5_ITER_STOP if current plugin can open given file
 *              H5_ITER_ERROR if an error occurs while trying to determine
 *                  if current plugin can open given file.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__file_open_find_connector_cb(H5PL_type_t plugin_type, const void *plugin_info, void *op_data)
{
    H5VL_file_open_find_connector_t *udata = (H5VL_file_open_find_connector_t *)op_data;
    const H5VL_class_t *             cls   = (const H5VL_class_t *)plugin_info;
    H5P_genplist_t *                 fapl_plist;
    H5P_genplist_t *                 fapl_plist_copy;
    herr_t                           status;
    htri_t                           is_accessible = FALSE;
    hid_t                            connector_id  = H5I_INVALID_HID;
    hid_t                            fapl_id       = H5I_INVALID_HID;
    herr_t                           ret_value     = H5_ITER_CONT;

    FUNC_ENTER_STATIC

    HDassert(udata);
    HDassert(udata->filename);
    HDassert(udata->connector_prop);
    HDassert(cls);
    HDassert(plugin_type == H5PL_TYPE_VOL);

    /* Silence compiler */
    (void)plugin_type;

    udata->cls = cls;

    /* Attempt to register plugin as a VOL connector */
    if ((connector_id = H5VL__register_connector_by_class(cls, TRUE, H5P_VOL_INITIALIZE_DEFAULT)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, H5_ITER_ERROR, "unable to register VOL connector")

    /* Setup FAPL with registered VOL connector */
    if (NULL == (fapl_plist = (H5P_genplist_t *)H5I_object_verify(udata->fapl_id, H5I_GENPROP_LST)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5_ITER_ERROR, "not a property list")
    if ((fapl_id = H5P_copy_plist(fapl_plist, TRUE)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, H5_ITER_ERROR, "can't copy fapl");
    if (NULL == (fapl_plist_copy = (H5P_genplist_t *)H5I_object_verify(fapl_id, H5I_GENPROP_LST)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5_ITER_ERROR, "not a property list")
    if (H5P_set_vol(fapl_plist_copy, connector_id, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, H5_ITER_ERROR, "can't set VOL connector on fapl")

    /* Check if file is accessible with given VOL connector */
    H5E_BEGIN_TRY
    {
        status = H5VL_file_specific(NULL, H5VL_FILE_IS_ACCESSIBLE, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                                    fapl_id, udata->filename, &is_accessible);
    }
    H5E_END_TRY;

    /* If the file was accessible with the current VOL connector, return
     * the FAPL with that VOL connector set on it. Errors are ignored here
     * as some VOL connectors may not support H5Fis_accessible.
     */
    if (status == SUCCEED && is_accessible > 0) {
        /* Modify 'connector_prop' to point to the VOL connector that
         * was actually used to open the file, rather than the original
         * VOL connector that was requested.
         */
        udata->connector_prop->connector_id   = connector_id;
        udata->connector_prop->connector_info = NULL;

        udata->fapl_id = fapl_id;
        ret_value      = H5_ITER_STOP;
    } /* end if */

done:
    if (ret_value != H5_ITER_STOP) {
        if (fapl_id >= 0 && H5I_dec_app_ref(fapl_id) < 0)
            HDONE_ERROR(H5E_PLIST, H5E_CANTCLOSEOBJ, H5_ITER_ERROR, "can't close fapl")
        if (connector_id >= 0 && H5I_dec_app_ref(connector_id) < 0)
            HDONE_ERROR(H5E_ATOM, H5E_CANTCLOSEOBJ, H5_ITER_ERROR, "can't close VOL connector ID")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__file_open_find_connector_cb() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_file_open
 *
 * Purpose:	Opens a file through the VOL.
 *
 * Note:	Does not have a 'static' version of the routine, since there's
 *		no objects in the container before this operation completes.
 *
 * Return:      Success: Pointer to file.
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_file_open(H5VL_connector_prop_t *connector_prop, const char *name, unsigned flags, hid_t fapl_id,
               hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;              /* VOL Class structure for callback info    */
    void *        ret_value = NULL; /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Get the connector's class */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_prop->connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__file_open(cls, name, flags, fapl_id, dxpl_id, req))) {
        H5VL_file_open_find_connector_t find_connector_ud;
        hbool_t                         find_connector;
        hbool_t                         connector_available = FALSE;

        /* Opening the file failed - Determine whether we should search
         * the plugin path to see if any other VOL connectors are available
         * to attempt to open the file with. This only occurs if no particular
         * VOL connector was specified (either via a FAPL or the
         * HDF5_VOL_CONNECTOR environment variable).
         */
        find_connector = !getenv("HDF5_VOL_CONNECTOR") && ((H5P_FILE_ACCESS_DEFAULT == fapl_id) ||
                                                           connector_prop->connector_id == H5_DEFAULT_VOL);

        if (find_connector) {
            herr_t iter_ret;

            find_connector_ud.connector_prop = connector_prop;
            find_connector_ud.filename       = name;
            find_connector_ud.cls            = NULL;
            find_connector_ud.fapl_id        = fapl_id;

            iter_ret = H5PL_iterate(H5PL_ITER_TYPE_VOL, H5VL__file_open_find_connector_cb,
                                    (void *)&find_connector_ud);
            if (iter_ret < 0)
                HGOTO_ERROR(H5E_VOL, H5E_BADITER, NULL,
                            "failed to iterate over available VOL connector plugins")
            else if (iter_ret)
                connector_available = TRUE;
        } /* end if */

        /* If one of the available VOL connector plugins is
         * able to open the file, clear the error stack from any
         * previous file open failures and then open the file.
         * Otherwise, if no VOL connectors are available, throw
         * error from original file open failure.
         */
        if (connector_available) {
            H5E_clear_stack(NULL);

            if (NULL == (ret_value = H5VL__file_open(find_connector_ud.cls, name, flags,
                                                     find_connector_ud.fapl_id, dxpl_id, req)))
                HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "can't open file '%s' with VOL connector '%s'",
                            name, find_connector_ud.cls->name)
        }
        else
            HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "open failed")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_file_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VLfile_open
 *
 * Purpose:     Opens a file
 *
 * Return:      Success:    Pointer to the file
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VLfile_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req)
{
    H5P_genplist_t *      plist;            /* Property list pointer */
    H5VL_connector_prop_t connector_prop;   /* Property for VOL connector ID & info */
    H5VL_class_t *        cls;              /* VOL connector's class struct */
    void *                ret_value = NULL; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE5("*x", "*sIuii**x", name, flags, fapl_id, dxpl_id, req);

    /* Get the VOL info from the fapl */
    if (NULL == (plist = (H5P_genplist_t *)H5I_object(fapl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list")
    if (H5P_peek(plist, H5F_ACS_VOL_CONN_NAME, &connector_prop) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "can't get VOL connector info")

    /* Get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_prop.connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__file_open(cls, name, flags, fapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "unable to open file")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLfile_open() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__file_get
 *
 * Purpose:	Get specific information about the file through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__file_get(void *obj, const H5VL_class_t *cls, H5VL_file_get_t get_type, hid_t dxpl_id, void **req,
               va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->file_cls.get)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'file get' method")

    /* Call the corresponding VOL callback */
    if ((cls->file_cls.get)(obj, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "file get failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__file_get() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_file_get
 *
 * Purpose:	Get specific information about the file through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_file_get(const H5VL_object_t *vol_obj, H5VL_file_get_t get_type, hid_t dxpl_id, void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if (H5VL__file_get(vol_obj->data, vol_obj->connector->cls, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "file get failed")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_file_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VLfile_get
 *
 * Purpose:     Gets information about the file
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLfile_get(void *obj, hid_t connector_id, H5VL_file_get_t get_type, hid_t dxpl_id, void **req,
             va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVgi**xx", obj, connector_id, get_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__file_get(obj, cls, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "unable to execute file get callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLfile_get() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__file_specific
 *
 * Purpose:	Perform File specific operations through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__file_specific(void *obj, const H5VL_class_t *cls, H5VL_file_specific_t specific_type, hid_t dxpl_id,
                    void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->file_cls.specific)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'file specific' method")

    /* Call the corresponding VOL callback */
    if ((cls->file_cls.specific)(obj, specific_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "file specific failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__file_specific() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_file_specific
 *
 * Purpose:	Perform file specific operations through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_file_specific(const H5VL_object_t *vol_obj, H5VL_file_specific_t specific_type, hid_t dxpl_id,
                   void **req, ...)
{
    const H5VL_class_t *cls;                       /* VOL connector's class struct */
    va_list             arguments;                 /* Argument list passed from the API call */
    hbool_t             arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t             vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t              ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Start access to the varargs, so they are available in all situations below */
    HDva_start(arguments, req);
    arg_started = TRUE;

    /* Special treatment of file access check & delete operations */
    /* (Retrieve the VOL connector from the FAPL, since the file isn't open) */
    if (specific_type == H5VL_FILE_IS_ACCESSIBLE || specific_type == H5VL_FILE_DELETE) {
        H5P_genplist_t *      plist;          /* Property list pointer */
        H5VL_connector_prop_t connector_prop; /* Property for VOL connector ID & info */
        va_list               tmp_args;       /* argument list passed from the API call */
        hid_t                 fapl_id;        /* File access property list for accessing the file */

        /* Get the file access property list to access the file */
        HDva_copy(tmp_args, arguments);
        fapl_id = HDva_arg(tmp_args, hid_t);
        HDva_end(tmp_args);

        /* Get the VOL info from the FAPL */
        if (NULL == (plist = (H5P_genplist_t *)H5I_object(fapl_id)))
            HGOTO_ERROR(H5E_VOL, H5E_BADTYPE, FAIL, "not a file access property list")
        if (H5P_peek(plist, H5F_ACS_VOL_CONN_NAME, &connector_prop) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get VOL connector info")

        /* Get class pointer */
        if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_prop.connector_id, H5I_VOL)))
            HGOTO_ERROR(H5E_VOL, H5E_BADTYPE, FAIL, "not a VOL connector ID")
    } /* end if */
    /* Set wrapper info in API context, for all other operations */
    else {
        /* Sanity check */
        HDassert(vol_obj);

        if (H5VL_set_vol_wrapper(vol_obj) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
        vol_wrapper_set = TRUE;

        /* Set the VOL connector class pointer */
        cls = vol_obj->connector->cls;
    } /* end else */

    /* Call the corresponding internal VOL routine */
    if (H5VL__file_specific(vol_obj ? vol_obj->data : NULL, cls, specific_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "file specific failed")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_file_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VLfile_specific
 *
 * Purpose:     Performs a connector-specific operation on a file
 *
 * Note:	The 'obj' parameter is allowed to be NULL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLfile_specific(void *obj, hid_t connector_id, H5VL_file_specific_t specific_type, hid_t dxpl_id,
                  void **req, va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVhi**xx", obj, connector_id, specific_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__file_specific(obj, cls, specific_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute file specific callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLfile_specific() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__file_optional
 *
 * Purpose:	Perform a connector specific operation
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__file_optional(void *obj, const H5VL_class_t *cls, H5VL_file_optional_t opt_type, hid_t dxpl_id,
                    void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->file_cls.optional)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'file optional' method")

    /* Call the corresponding VOL callback */
    if ((cls->file_cls.optional)(obj, opt_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "file optional failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__file_optional() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_file_optional
 *
 * Purpose:	Perform a connector specific operation
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_file_optional(const H5VL_object_t *vol_obj, H5VL_file_optional_t opt_type, hid_t dxpl_id, void **req,
                   ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if (H5VL__file_optional(vol_obj->data, vol_obj->connector->cls, opt_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "file optional failed")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_file_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VLfile_optional
 *
 * Purpose:     Performs an optional connector-specific operation on a file
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLfile_optional(void *obj, hid_t connector_id, H5VL_file_optional_t opt_type, hid_t dxpl_id, void **req,
                  va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVvi**xx", obj, connector_id, opt_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__file_optional(obj, cls, opt_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute file optional callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLfile_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__file_close
 *
 * Purpose:     Closes a file through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__file_close(void *obj, const H5VL_class_t *cls, hid_t dxpl_id, void **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(obj);
    HDassert(cls);

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->file_cls.close)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'file close' method")

    /* Call the corresponding VOL callback */
    if ((cls->file_cls.close)(obj, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEFILE, FAIL, "file close failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__file_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_file_close
 *
 * Purpose:     Closes a file through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_file_close(const H5VL_object_t *vol_obj, hid_t dxpl_id, void **req)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__file_close(vol_obj->data, vol_obj->connector->cls, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEFILE, FAIL, "file close failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_file_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VLfile_close
 *
 * Purpose:     Closes a file
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLfile_close(void *obj, hid_t connector_id, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE4("e", "*xii**x", obj, connector_id, dxpl_id, req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__file_close(obj, cls, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEFILE, FAIL, "unable to close file")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLfile_close() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__group_create
 *
 * Purpose:	Creates a group through the VOL
 *
 * Return:      Success: Pointer to new group
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL__group_create(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls, const char *name,
                   hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id, void **req)
{
    void *ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->group_cls.create)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, NULL, "VOL connector has no 'group create' method")

    /* Call the corresponding VOL callback */
    if (NULL ==
        (ret_value = (cls->group_cls.create)(obj, loc_params, name, lcpl_id, gcpl_id, gapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "group create failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__group_create() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_group_create
 *
 * Purpose:	Creates a group through the VOL
 *
 * Return:      Success: Pointer to new group
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_group_create(const H5VL_object_t *vol_obj, const H5VL_loc_params_t *loc_params, const char *name,
                  hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id, void **req)
{
    hbool_t vol_wrapper_set = FALSE; /* Whether the VOL object wrapping context was set up */
    void *  ret_value       = NULL;  /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, NULL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__group_create(vol_obj->data, loc_params, vol_obj->connector->cls, name,
                                                lcpl_id, gcpl_id, gapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "group create failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, NULL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_group_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VLgroup_create
 *
 * Purpose:     Creates a group
 *
 * Return:      Success:    Pointer to the new group
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VLgroup_create(void *obj, const H5VL_loc_params_t *loc_params, hid_t connector_id, const char *name,
                 hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;              /* VOL connector's class struct */
    void *        ret_value = NULL; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE9("*x", "*x*xi*siiii**x", obj, loc_params, connector_id, name, lcpl_id, gcpl_id, gapl_id, dxpl_id,
             req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (NULL ==
        (ret_value = H5VL__group_create(obj, loc_params, cls, name, lcpl_id, gcpl_id, gapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, NULL, "unable to create group")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLgroup_create() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__group_open
 *
 * Purpose:	Opens a group through the VOL
 *
 * Return:      Success: Pointer to group
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL__group_open(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls, const char *name,
                 hid_t gapl_id, hid_t dxpl_id, void **req)
{
    void *ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->group_cls.open)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, NULL, "VOL connector has no 'group open' method")

    /* Call the corresponding VOL callback */
    if (NULL == (ret_value = (cls->group_cls.open)(obj, loc_params, name, gapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "group open failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__group_open() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_group_open
 *
 * Purpose:	Opens a group through the VOL
 *
 * Return:      Success: Pointer to group
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_group_open(const H5VL_object_t *vol_obj, const H5VL_loc_params_t *loc_params, const char *name,
                hid_t gapl_id, hid_t dxpl_id, void **req)
{
    hbool_t vol_wrapper_set = FALSE; /* Whether the VOL object wrapping context was set up */
    void *  ret_value       = NULL;  /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, NULL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__group_open(vol_obj->data, loc_params, vol_obj->connector->cls, name,
                                              gapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "group open failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, NULL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_group_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VLgroup_open
 *
 * Purpose:     Opens a group
 *
 * Return:      Success:    Pointer to the group
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VLgroup_open(void *obj, const H5VL_loc_params_t *loc_params, hid_t connector_id, const char *name,
               hid_t gapl_id, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;              /* VOL connector's class struct */
    void *        ret_value = NULL; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE7("*x", "*x*xi*sii**x", obj, loc_params, connector_id, name, gapl_id, dxpl_id, req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__group_open(obj, loc_params, cls, name, gapl_id, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, NULL, "unable to open group")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLgroup_open() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__group_get
 *
 * Purpose:	Get specific information about the group through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__group_get(void *obj, const H5VL_class_t *cls, H5VL_group_get_t get_type, hid_t dxpl_id, void **req,
                va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->group_cls.get)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'group get' method")

    /* Call the corresponding VOL callback */
    if ((cls->group_cls.get)(obj, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "group get failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__group_get() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_group_get
 *
 * Purpose:	Get specific information about the group through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_group_get(const H5VL_object_t *vol_obj, H5VL_group_get_t get_type, hid_t dxpl_id, void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if (H5VL__group_get(vol_obj->data, vol_obj->connector->cls, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "group get failed")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_group_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VLgroup_get
 *
 * Purpose:     Gets information about the group
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLgroup_get(void *obj, hid_t connector_id, H5VL_group_get_t get_type, hid_t dxpl_id, void **req,
              va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVii**xx", obj, connector_id, get_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__group_get(obj, cls, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "unable to execute group get callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLgroup_get() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__group_specific
 *
 * Purpose:	Specific operation on groups through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__group_specific(void *obj, const H5VL_class_t *cls, H5VL_group_specific_t specific_type, hid_t dxpl_id,
                     void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->group_cls.specific)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'group specific' method")

    /* Call the corresponding VOL callback */
    if ((cls->group_cls.specific)(obj, specific_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute group specific callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__group_specific() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_group_specific
 *
 * Purpose:	Specific operation on groups through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_group_specific(const H5VL_object_t *vol_obj, H5VL_group_specific_t specific_type, hid_t dxpl_id,
                    void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if (H5VL__group_specific(vol_obj->data, vol_obj->connector->cls, specific_type, dxpl_id, req, arguments) <
        0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute group specific callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_group_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VLgroup_specific
 *
 * Purpose:     Performs a connector-specific operation on a group
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLgroup_specific(void *obj, hid_t connector_id, H5VL_group_specific_t specific_type, hid_t dxpl_id,
                   void **req, va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVji**xx", obj, connector_id, specific_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__group_specific(obj, cls, specific_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute group specific callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLgroup_specific() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__group_optional
 *
 * Purpose:	Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__group_optional(void *obj, const H5VL_class_t *cls, H5VL_group_optional_t opt_type, hid_t dxpl_id,
                     void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->group_cls.optional)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'group optional' method")

    /* Call the corresponding VOL callback */
    if ((ret_value = (cls->group_cls.optional)(obj, opt_type, dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute group optional callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__group_optional() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_group_optional
 *
 * Purpose:	Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_group_optional(const H5VL_object_t *vol_obj, H5VL_group_optional_t opt_type, hid_t dxpl_id, void **req,
                    ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if ((ret_value = H5VL__group_optional(vol_obj->data, vol_obj->connector->cls, opt_type, dxpl_id, req,
                                          arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute group optional callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_group_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VLgroup_optional
 *
 * Purpose:     Performs an optional connector-specific operation on a group
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLgroup_optional(void *obj, hid_t connector_id, H5VL_group_optional_t opt_type, hid_t dxpl_id, void **req,
                   va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVwi**xx", obj, connector_id, opt_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if ((ret_value = H5VL__group_optional(obj, cls, opt_type, dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute group optional callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLgroup_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__group_close
 *
 * Purpose:     Closes a group through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__group_close(void *obj, const H5VL_class_t *cls, hid_t dxpl_id, void **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    /* Sanity check */
    HDassert(obj);
    HDassert(cls);

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->group_cls.close)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'group close' method")

    /* Call the corresponding VOL callback */
    if ((cls->group_cls.close)(obj, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEOBJ, FAIL, "group close failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__group_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_group_close
 *
 * Purpose:     Closes a group through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_group_close(const H5VL_object_t *vol_obj, hid_t dxpl_id, void **req)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__group_close(vol_obj->data, vol_obj->connector->cls, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEOBJ, FAIL, "group close failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_group_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VLgroup_close
 *
 * Purpose:     Closes a group
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLgroup_close(void *obj, hid_t connector_id, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE4("e", "*xii**x", obj, connector_id, dxpl_id, req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__group_close(obj, cls, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCLOSEOBJ, FAIL, "unable to close group")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLgroup_close() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__link_create
 *
 * Purpose:	Creates a link through the VOL
 *
 * Note:	The 'obj' parameter is allowed to be NULL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__link_create(H5VL_link_create_type_t create_type, void *obj, const H5VL_loc_params_t *loc_params,
                  const H5VL_class_t *cls, hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id, void **req,
                  va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->link_cls.create)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'link create' method")

    /* Call the corresponding VOL callback */
    if ((cls->link_cls.create)(create_type, obj, loc_params, lcpl_id, lapl_id, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, FAIL, "link create failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__link_create() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_link_create
 *
 * Purpose:	Creates a link through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_link_create(H5VL_link_create_type_t create_type, const H5VL_object_t *vol_obj,
                 const H5VL_loc_params_t *loc_params, hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id, void **req,
                 ...)
{
    H5VL_object_t tmp_vol_obj;               /* Temporary object */
    va_list       arguments;                 /* Argument list passed from the API call */
    hbool_t       arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t       vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t        ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Start the varargs, so they can be copied */
    HDva_start(arguments, req);
    arg_started = TRUE;

    /* Special case for hard links */
    if (H5VL_LINK_CREATE_HARD == create_type && NULL == vol_obj->data) {
        va_list tmp_arguments; /* Copy of argument list passed in */

        /* Get the VOL data pointer from the varargs */
        HDva_copy(tmp_arguments, arguments);
        tmp_vol_obj.data = HDva_arg(tmp_arguments, void *);
        HDva_end(tmp_arguments);
    } /* end if */
    else
        /* Use the VOL object passed in */
        tmp_vol_obj.data = vol_obj->data;
    tmp_vol_obj.connector = vol_obj->connector;

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(&tmp_vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__link_create(create_type, vol_obj->data, loc_params, vol_obj->connector->cls, lcpl_id, lapl_id,
                          dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, FAIL, "link create failed")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_link_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VLlink_create
 *
 * Purpose:     Creates a link
 *
 * Note:	The 'obj' parameter is allowed to be NULL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLlink_create(H5VL_link_create_type_t create_type, void *obj, const H5VL_loc_params_t *loc_params,
                hid_t connector_id, hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id, void **req,
                va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE9("e", "Vk*x*xiiii**xx", create_type, obj, loc_params, connector_id, lcpl_id, lapl_id, dxpl_id,
             req, arguments);

    /* Get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__link_create(create_type, obj, loc_params, cls, lcpl_id, lapl_id, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCREATE, FAIL, "unable to create link")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLlink_create() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__link_copy
 *
 * Purpose:	Copys a link from src to dst.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__link_copy(void *src_obj, const H5VL_loc_params_t *loc_params1, void *dst_obj,
                const H5VL_loc_params_t *loc_params2, const H5VL_class_t *cls, hid_t lcpl_id, hid_t lapl_id,
                hid_t dxpl_id, void **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->link_cls.copy)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'link copy' method")

    /* Call the corresponding VOL callback */
    if ((cls->link_cls.copy)(src_obj, loc_params1, dst_obj, loc_params2, lcpl_id, lapl_id, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCOPY, FAIL, "link copy failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__link_copy() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_link_copy
 *
 * Purpose:	Copys a link from src to dst.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_link_copy(const H5VL_object_t *src_vol_obj, const H5VL_loc_params_t *loc_params1,
               const H5VL_object_t *dst_vol_obj, const H5VL_loc_params_t *loc_params2, hid_t lcpl_id,
               hid_t lapl_id, hid_t dxpl_id, void **req)
{
    const H5VL_object_t *vol_obj;                   /* VOL object for object with data */
    hbool_t              vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t               ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    vol_obj = (src_vol_obj->data ? src_vol_obj : dst_vol_obj);
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__link_copy(src_vol_obj->data, loc_params1, (dst_vol_obj ? dst_vol_obj->data : NULL), loc_params2,
                        vol_obj->connector->cls, lcpl_id, lapl_id, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCOPY, FAIL, "link copy failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_link_copy() */

/*-------------------------------------------------------------------------
 * Function:    H5VLlink_copy
 *
 * Purpose:     Copies a link to a new location
 *
 * Note:	The 'src_obj' and 'dst_obj' parameters are allowed to be NULL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLlink_copy(void *src_obj, const H5VL_loc_params_t *loc_params1, void *dst_obj,
              const H5VL_loc_params_t *loc_params2, hid_t connector_id, hid_t lcpl_id, hid_t lapl_id,
              hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE9("e", "*x*x*x*xiiii**x", src_obj, loc_params1, dst_obj, loc_params2, connector_id, lcpl_id,
             lapl_id, dxpl_id, req);

    /* Get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__link_copy(src_obj, loc_params1, dst_obj, loc_params2, cls, lcpl_id, lapl_id, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCOPY, FAIL, "unable to copy object")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLlink_copy() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__link_move
 *
 * Purpose:	Moves a link from src to dst.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__link_move(void *src_obj, const H5VL_loc_params_t *loc_params1, void *dst_obj,
                const H5VL_loc_params_t *loc_params2, const H5VL_class_t *cls, hid_t lcpl_id, hid_t lapl_id,
                hid_t dxpl_id, void **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->link_cls.move)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'link move' method")

    /* Call the corresponding VOL callback */
    if ((cls->link_cls.move)(src_obj, loc_params1, dst_obj, loc_params2, lcpl_id, lapl_id, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTMOVE, FAIL, "link move failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__link_move() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_link_move
 *
 * Purpose:	Moves a link from src to dst.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_link_move(const H5VL_object_t *src_vol_obj, const H5VL_loc_params_t *loc_params1,
               const H5VL_object_t *dst_vol_obj, const H5VL_loc_params_t *loc_params2, hid_t lcpl_id,
               hid_t lapl_id, hid_t dxpl_id, void **req)
{
    const H5VL_object_t *vol_obj;                   /* VOL object for object with data */
    hbool_t              vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t               ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    vol_obj = (src_vol_obj->data ? src_vol_obj : dst_vol_obj);
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__link_move(src_vol_obj->data, loc_params1, (dst_vol_obj ? dst_vol_obj->data : NULL), loc_params2,
                        vol_obj->connector->cls, lcpl_id, lapl_id, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTMOVE, FAIL, "link move failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_link_move() */

/*-------------------------------------------------------------------------
 * Function:    H5VLlink_move
 *
 * Purpose:     Moves a link to another location
 *
 * Note:	The 'src_obj' and 'dst_obj' parameters are allowed to be NULL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLlink_move(void *src_obj, const H5VL_loc_params_t *loc_params1, void *dst_obj,
              const H5VL_loc_params_t *loc_params2, hid_t connector_id, hid_t lcpl_id, hid_t lapl_id,
              hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE9("e", "*x*x*x*xiiii**x", src_obj, loc_params1, dst_obj, loc_params2, connector_id, lcpl_id,
             lapl_id, dxpl_id, req);

    /* Get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__link_move(src_obj, loc_params1, dst_obj, loc_params2, cls, lcpl_id, lapl_id, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTMOVE, FAIL, "unable to move object")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLlink_move() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__link_get
 *
 * Purpose:	Get specific information about the link through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__link_get(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
               H5VL_link_get_t get_type, hid_t dxpl_id, void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->link_cls.get)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'link get' method")

    /* Call the corresponding VOL callback */
    if ((cls->link_cls.get)(obj, loc_params, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "link get failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__link_get() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_link_get
 *
 * Purpose:	Get specific information about the link through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_link_get(const H5VL_object_t *vol_obj, const H5VL_loc_params_t *loc_params, H5VL_link_get_t get_type,
              hid_t dxpl_id, void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if (H5VL__link_get(vol_obj->data, loc_params, vol_obj->connector->cls, get_type, dxpl_id, req,
                       arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "link get failed")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_link_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VLlink_get
 *
 * Purpose:     Gets information about a link
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLlink_get(void *obj, const H5VL_loc_params_t *loc_params, hid_t connector_id, H5VL_link_get_t get_type,
             hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE7("e", "*x*xiVli**xx", obj, loc_params, connector_id, get_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__link_get(obj, loc_params, cls, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "unable to execute link get callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLlink_get() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__link_specific
 *
 * Purpose:	Specific operation on links through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__link_specific(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                    H5VL_link_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->link_cls.specific)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'link specific' method")

    /* Call the corresponding VOL callback */
    if ((ret_value = (cls->link_cls.specific)(obj, loc_params, specific_type, dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute link specific callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__link_specific() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_link_specific
 *
 * Purpose:	Specific operation on links through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_link_specific(const H5VL_object_t *vol_obj, const H5VL_loc_params_t *loc_params,
                   H5VL_link_specific_t specific_type, hid_t dxpl_id, void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if ((ret_value = H5VL__link_specific(vol_obj->data, loc_params, vol_obj->connector->cls, specific_type,
                                         dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute link specific callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_link_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VLlink_specific
 *
 * Purpose:     Performs a connector-specific operation on a link
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLlink_specific(void *obj, const H5VL_loc_params_t *loc_params, hid_t connector_id,
                  H5VL_link_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE7("e", "*x*xiVmi**xx", obj, loc_params, connector_id, specific_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if ((ret_value = H5VL__link_specific(obj, loc_params, cls, specific_type, dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute link specific callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLlink_specific() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__link_optional
 *
 * Purpose:	Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__link_optional(void *obj, const H5VL_class_t *cls, H5VL_link_optional_t opt_type, hid_t dxpl_id,
                    void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->link_cls.optional)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'link optional' method")

    /* Call the corresponding VOL callback */
    if ((cls->link_cls.optional)(obj, opt_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute link optional callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__link_optional() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_link_optional
 *
 * Purpose:	Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_link_optional(const H5VL_object_t *vol_obj, H5VL_link_optional_t opt_type, hid_t dxpl_id, void **req,
                   ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if (H5VL__link_optional(vol_obj->data, vol_obj->connector->cls, opt_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute link optional callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_link_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VLlink_optional
 *
 * Purpose:     Performs an optional connector-specific operation on a link
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLlink_optional(void *obj, hid_t connector_id, H5VL_link_optional_t opt_type, hid_t dxpl_id, void **req,
                  va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVxi**xx", obj, connector_id, opt_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__link_optional(obj, cls, opt_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute link optional callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLlink_optional() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__object_open
 *
 * Purpose:	Opens a object through the VOL
 *
 * Return:      Success: Pointer to the object
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL__object_open(void *obj, const H5VL_loc_params_t *params, const H5VL_class_t *cls,
                  H5I_type_t *opened_type, hid_t dxpl_id, void **req)
{
    void *ret_value = NULL; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->object_cls.open)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, NULL, "VOL connector has no 'object open' method")

    /* Call the corresponding VOL callback */
    if (NULL == (ret_value = (cls->object_cls.open)(obj, params, opened_type, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "object open failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__object_open() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_object_open
 *
 * Purpose:	Opens a object through the VOL
 *
 * Return:      Success: Pointer to the object
 *		Failure: NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_object_open(const H5VL_object_t *vol_obj, const H5VL_loc_params_t *params, H5I_type_t *opened_type,
                 hid_t dxpl_id, void **req)
{
    hbool_t vol_wrapper_set = FALSE; /* Whether the VOL object wrapping context was set up */
    void *  ret_value       = NULL;  /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, NULL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__object_open(vol_obj->data, params, vol_obj->connector->cls, opened_type,
                                               dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "object open failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, NULL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_object_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VLobject_open
 *
 * Purpose:     Opens an object
 *
 * Return:      Success:    Pointer to the object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VLobject_open(void *obj, const H5VL_loc_params_t *params, hid_t connector_id, H5I_type_t *opened_type,
                hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;              /* VOL connector's class struct */
    void *        ret_value = NULL; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("*x", "*x*xi*Iti**x", obj, params, connector_id, opened_type, dxpl_id, req);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (NULL == (ret_value = H5VL__object_open(obj, params, cls, opened_type, dxpl_id, req)))
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPENOBJ, NULL, "unable to open object")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLobject_open() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__object_copy
 *
 * Purpose:	Copies an object to another destination through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__object_copy(void *src_obj, const H5VL_loc_params_t *src_loc_params, const char *src_name, void *dst_obj,
                  const H5VL_loc_params_t *dst_loc_params, const char *dst_name, const H5VL_class_t *cls,
                  hid_t ocpypl_id, hid_t lcpl_id, hid_t dxpl_id, void **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->object_cls.copy)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'object copy' method")

    /* Call the corresponding VOL callback */
    if ((cls->object_cls.copy)(src_obj, src_loc_params, src_name, dst_obj, dst_loc_params, dst_name,
                               ocpypl_id, lcpl_id, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCOPY, FAIL, "object copy failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__object_copy() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_object_copy
 *
 * Purpose:	Copies an object to another destination through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_object_copy(const H5VL_object_t *src_obj, const H5VL_loc_params_t *src_loc_params, const char *src_name,
                 const H5VL_object_t *dst_obj, const H5VL_loc_params_t *dst_loc_params, const char *dst_name,
                 hid_t ocpypl_id, hid_t lcpl_id, hid_t dxpl_id, void **req)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Make sure that the VOL connectors are the same */
    if (src_obj->connector->cls->value != dst_obj->connector->cls->value)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                    "objects are accessed through different VOL connectors and can't be copied")

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(src_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__object_copy(src_obj->data, src_loc_params, src_name, dst_obj->data, dst_loc_params, dst_name,
                          src_obj->connector->cls, ocpypl_id, lcpl_id, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCOPY, FAIL, "object copy failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_object_copy() */

/*-------------------------------------------------------------------------
 * Function:    H5VLobject_copy
 *
 * Purpose:     Copies an object to another location
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLobject_copy(void *src_obj, const H5VL_loc_params_t *src_loc_params, const char *src_name, void *dst_obj,
                const H5VL_loc_params_t *dst_loc_params, const char *dst_name, hid_t connector_id,
                hid_t ocpypl_id, hid_t lcpl_id, hid_t dxpl_id, void **req)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE11("e", "*x*x*s*x*x*siiii**x", src_obj, src_loc_params, src_name, dst_obj, dst_loc_params,
              dst_name, connector_id, ocpypl_id, lcpl_id, dxpl_id, req);

    /* Check args and get class pointers */
    if (NULL == src_obj || NULL == dst_obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__object_copy(src_obj, src_loc_params, src_name, dst_obj, dst_loc_params, dst_name, cls,
                          ocpypl_id, lcpl_id, dxpl_id, req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCOPY, FAIL, "unable to copy object")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLobject_copy() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__object_get
 *
 * Purpose:	Get specific information about the object through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__object_get(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                 H5VL_object_get_t get_type, hid_t dxpl_id, void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->object_cls.get)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'object get' method")

    /* Call the corresponding VOL callback */
    if ((cls->object_cls.get)(obj, loc_params, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "get failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__object_get() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_object_get
 *
 * Purpose:	Get specific information about the object through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_object_get(const H5VL_object_t *vol_obj, const H5VL_loc_params_t *loc_params, H5VL_object_get_t get_type,
                hid_t dxpl_id, void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if (H5VL__object_get(vol_obj->data, loc_params, vol_obj->connector->cls, get_type, dxpl_id, req,
                         arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "get failed")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_object_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VLobject_get
 *
 * Purpose:     Gets information about an object
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLobject_get(void *obj, const H5VL_loc_params_t *loc_params, hid_t connector_id, H5VL_object_get_t get_type,
               hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE7("e", "*x*xiVni**xx", obj, loc_params, connector_id, get_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__object_get(obj, loc_params, cls, get_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "unable to execute object get callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLobject_get() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__object_specific
 *
 * Purpose:	Specific operation on objects through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__object_specific(void *obj, const H5VL_loc_params_t *loc_params, const H5VL_class_t *cls,
                      H5VL_object_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->object_cls.specific)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'object specific' method")

    /* Call the corresponding VOL callback */
    if ((ret_value = (cls->object_cls.specific)(obj, loc_params, specific_type, dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "object specific failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__object_specific() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_object_specific
 *
 * Purpose:	Specific operation on objects through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_object_specific(const H5VL_object_t *vol_obj, const H5VL_loc_params_t *loc_params,
                     H5VL_object_specific_t specific_type, hid_t dxpl_id, void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if ((ret_value = H5VL__object_specific(vol_obj->data, loc_params, vol_obj->connector->cls, specific_type,
                                           dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "object specific failed")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_object_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VLobject_specific
 *
 * Purpose:     Performs a connector-specific operation on an object
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLobject_specific(void *obj, const H5VL_loc_params_t *loc_params, hid_t connector_id,
                    H5VL_object_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE7("e", "*x*xiVoi**xx", obj, loc_params, connector_id, specific_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->object_cls.specific)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no `object specific' method")

    /* Bypass the H5VLint layer, calling the VOL callback directly */
    if ((ret_value = (cls->object_cls.specific)(obj, loc_params, specific_type, dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute object specific callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLobject_specific() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__object_optional
 *
 * Purpose:	Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__object_optional(void *obj, const H5VL_class_t *cls, H5VL_object_optional_t opt_type, hid_t dxpl_id,
                      void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->object_cls.optional)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'object optional' method")

    /* Call the corresponding VOL callback */
    if ((cls->object_cls.optional)(obj, opt_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute object optional callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__object_optional() */

/*-------------------------------------------------------------------------
 * Function:	H5VL_object_optional
 *
 * Purpose:	Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_object_optional(const H5VL_object_t *vol_obj, H5VL_object_optional_t opt_type, hid_t dxpl_id, void **req,
                     ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if (H5VL__object_optional(vol_obj->data, vol_obj->connector->cls, opt_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute object optional callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_object_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VLobject_optional
 *
 * Purpose:     Performs an optional connector-specific operation on an object
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLobject_optional(void *obj, hid_t connector_id, H5VL_object_optional_t opt_type, hid_t dxpl_id, void **req,
                    va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiVyi**xx", obj, connector_id, opt_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__object_optional(obj, cls, opt_type, dxpl_id, req, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute object optional callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLobject_optional() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__introspect_get_conn_cls
 *
 * Purpose:     Calls the connector-specific callback to query the connector
 *              class.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__introspect_get_conn_cls(void *obj, const H5VL_class_t *cls, H5VL_get_conn_lvl_t lvl,
                              const H5VL_class_t **conn_cls)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(obj);
    HDassert(cls);
    HDassert(lvl >= H5VL_GET_CONN_LVL_CURR && lvl <= H5VL_GET_CONN_LVL_TERM);
    HDassert(conn_cls);

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->introspect_cls.get_conn_cls)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'get_conn_cls' method")

    /* Call the corresponding VOL callback */
    if ((cls->introspect_cls.get_conn_cls)(obj, lvl, conn_cls) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't query connector class")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__introspect_get_conn_cls() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_introspect_get_conn_cls
 *
 * Purpose:     Calls the connector-specific callback to query the connector
 *              class.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_introspect_get_conn_cls(const H5VL_object_t *vol_obj, H5VL_get_conn_lvl_t lvl,
                             const H5VL_class_t **conn_cls)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__introspect_get_conn_cls(vol_obj->data, vol_obj->connector->cls, lvl, conn_cls) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't query connector class")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_introspect_get_conn_cls() */

/*-------------------------------------------------------------------------
 * Function:    H5VLintrospect_get_conn_cls
 *
 * Purpose:     Calls the connector-specific callback to query the connector
 *              class.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLintrospect_get_conn_cls(void *obj, hid_t connector_id, H5VL_get_conn_lvl_t lvl,
                            const H5VL_class_t **conn_cls)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE4("e", "*xiVL**x", obj, connector_id, lvl, conn_cls);

    /* Check args */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL obj pointer")
    if (NULL == conn_cls)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL conn_cls pointer")

    /* Get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__introspect_get_conn_cls(obj, cls, lvl, conn_cls) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't query connector class")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLintrospect_get_conn_cls() */

/*-------------------------------------------------------------------------
 * Function:	H5VL__introspect_opt_query
 *
 * Purpose:     Calls the connector-specific callback to query if an optional
 *              operation is supported.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__introspect_opt_query(void *obj, const H5VL_class_t *cls, H5VL_subclass_t subcls, int opt_type,
                           hbool_t *supported)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->introspect_cls.opt_query)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'opt_query' method")

    /* Call the corresponding VOL callback */
    if ((cls->introspect_cls.opt_query)(obj, subcls, opt_type, supported) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't query optional operation support")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__introspect_opt_query() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_introspect_opt_query
 *
 * Purpose:     Calls the connector-specific callback to query if an optional
 *              operation is supported.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_introspect_opt_query(const H5VL_object_t *vol_obj, H5VL_subclass_t subcls, int opt_type,
                          hbool_t *supported)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__introspect_opt_query(vol_obj->data, vol_obj->connector->cls, subcls, opt_type, supported) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't query optional operation support")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_introspect_opt_query() */

/*-------------------------------------------------------------------------
 * Function:    H5VLintrospect_opt_query
 *
 * Purpose:     Calls the connector-specific callback to query if an optional
 *              operation is supported.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLintrospect_opt_query(void *obj, hid_t connector_id, H5VL_subclass_t subcls, int opt_type,
                         hbool_t *supported)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE5("e", "*xiVSIs*b", obj, connector_id, subcls, opt_type, supported);

    /* Get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__introspect_opt_query(obj, cls, subcls, opt_type, supported) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't query optional operation support")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLintrospect_opt_query() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__request_wait
 *
 * Purpose:     Waits on an asychronous request through the VOL
 *
 * Note:	Releases the request if the operation has completed and the
 *		connector callback succeeds
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__request_wait(void *req, const H5VL_class_t *cls, uint64_t timeout, H5ES_status_t *status)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(req);
    HDassert(cls);
    HDassert(status);

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->request_cls.wait)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'async wait' method")

    /* Call the corresponding VOL callback */
    if ((cls->request_cls.wait)(req, timeout, status) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "request wait failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__request_wait() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_request_wait
 *
 * Purpose:     Waits on an asychronous request through the VOL
 *
 * Note:	Releases the request if the operation has completed and the
 *		connector callback succeeds
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_request_wait(const H5VL_object_t *vol_obj, uint64_t timeout, H5ES_status_t *status)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(vol_obj);

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__request_wait(vol_obj->data, vol_obj->connector->cls, timeout, status) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "request wait failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_request_wait() */

/*-------------------------------------------------------------------------
 * Function:    H5VLrequest_wait
 *
 * Purpose:     Waits on a request
 *
 * Note:	Releases the request if the operation has completed and the
 *		connector callback succeeds
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLrequest_wait(void *req, hid_t connector_id, uint64_t timeout, H5ES_status_t *status)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE4("e", "*xiUL*Es", req, connector_id, timeout, status);

    /* Get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__request_wait(req, cls, timeout, status) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "unable to wait on request")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLrequest_wait() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__request_notify
 *
 * Purpose:     Registers a user callback to be invoked when an asynchronous
 *		operation completes
 *
 * Note:	Releases the request, if connector callback succeeds
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__request_notify(void *req, const H5VL_class_t *cls, H5VL_request_notify_t cb, void *ctx)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(req);
    HDassert(cls);

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->request_cls.notify)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'async notify' method")

    /* Call the corresponding VOL callback */
    if ((cls->request_cls.notify)(req, cb, ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "request notify failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__request_notify() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_request_notify
 *
 * Purpose:     Registers a user callback to be invoked when an asynchronous
 *		operation completes
 *
 * Note:	Releases the request, if connector callback succeeds
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_request_notify(const H5VL_object_t *vol_obj, H5VL_request_notify_t cb, void *ctx)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(vol_obj);

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__request_notify(vol_obj->data, vol_obj->connector->cls, cb, ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "request notify failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_request_notify() */

/*-------------------------------------------------------------------------
 * Function:    H5VLrequest_notify
 *
 * Purpose:     Registers a user callback to be invoked when an asynchronous
 *		operation completes
 *
 * Note:	Releases the request, if connector callback succeeds
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLrequest_notify(void *req, hid_t connector_id, H5VL_request_notify_t cb, void *ctx)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE4("e", "*xix*x", req, connector_id, cb, ctx);

    /* Get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__request_notify(req, cls, cb, ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "unable to register notify callback for request")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLrequest_notify() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__request_cancel
 *
 * Purpose:     Cancels an asynchronous request through the VOL
 *
 * Note:	Releases the request, if connector callback succeeds
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__request_cancel(void *req, const H5VL_class_t *cls)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(req);
    HDassert(cls);

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->request_cls.cancel)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'async cancel' method")

    /* Call the corresponding VOL callback */
    if ((cls->request_cls.cancel)(req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "request cancel failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__request_cancel() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_request_cancel
 *
 * Purpose:     Cancels an asynchronous request through the VOL
 *
 * Note:	Releases the request, if connector callback succeeds
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_request_cancel(const H5VL_object_t *vol_obj)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(vol_obj);

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if (H5VL__request_cancel(vol_obj->data, vol_obj->connector->cls) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "request cancel failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_request_cancel() */

/*-------------------------------------------------------------------------
 * Function:    H5VLrequest_cancel
 *
 * Purpose:     Cancels a request
 *
 * Note:	Releases the request, if connector callback succeeds
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLrequest_cancel(void *req, hid_t connector_id)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE2("e", "*xi", req, connector_id);

    /* Get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__request_cancel(req, cls) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "unable to cancel request")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLrequest_cancel() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__request_specific
 *
 * Purpose:	Specific operation on asynchronous request through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__request_specific(void *req, const H5VL_class_t *cls, H5VL_request_specific_t specific_type,
                       va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(req);
    HDassert(cls);

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->request_cls.specific)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'async specific' method")

    /* Call the corresponding VOL callback */
    if ((ret_value = (cls->request_cls.specific)(req, specific_type, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL,
                    "unable to execute asynchronous request specific callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__request_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_request_specific
 *
 * Purpose:	Specific operation on asynchronous request through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_request_specific(const H5VL_object_t *vol_obj, H5VL_request_specific_t specific_type, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(vol_obj);

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, specific_type);
    arg_started = TRUE;
    if ((ret_value =
             H5VL__request_specific(vol_obj->data, vol_obj->connector->cls, specific_type, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL,
                    "unable to execute asynchronous request specific callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_request_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VLrequest_specific
 *
 * Purpose:     Performs a connector-specific operation on an asynchronous request
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLrequest_specific(void *req, hid_t connector_id, H5VL_request_specific_t specific_type, va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE4("e", "*xiVrx", req, connector_id, specific_type, arguments);

    /* Get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if ((ret_value = H5VL__request_specific(req, cls, specific_type, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL,
                    "unable to execute asynchronous request specific callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLrequest_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__request_optional
 *
 * Purpose:	Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__request_optional(void *req, const H5VL_class_t *cls, H5VL_request_optional_t opt_type,
                       va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(req);
    HDassert(cls);

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->request_cls.optional)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'async optional' method")

    /* Call the corresponding VOL callback */
    if ((ret_value = (cls->request_cls.optional)(req, opt_type, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL,
                    "unable to execute asynchronous request optional callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__request_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_request_optional
 *
 * Purpose:	Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_request_optional(const H5VL_object_t *vol_obj, H5VL_request_optional_t opt_type, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(vol_obj);

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, opt_type);
    arg_started = TRUE;
    if ((ret_value = H5VL__request_optional(vol_obj->data, vol_obj->connector->cls, opt_type, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL,
                    "unable to execute asynchronous request optional callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_request_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VLrequest_optional
 *
 * Purpose:     Performs an optional connector-specific operation on an asynchronous request
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLrequest_optional(void *req, hid_t connector_id, H5VL_request_optional_t opt_type, va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE4("e", "*xiVzx", req, connector_id, opt_type, arguments);

    /* Get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if ((ret_value = H5VL__request_optional(req, cls, opt_type, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL,
                    "unable to execute asynchronous request optional callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLrequest_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__request_free
 *
 * Purpose:     Frees an asynchronous request through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__request_free(void *req, const H5VL_class_t *cls)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(req);
    HDassert(cls);

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->request_cls.free)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'async free' method")

    /* Call the corresponding VOL callback */
    if ((cls->request_cls.free)(req) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "request free failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__request_free() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_request_free
 *
 * Purpose:     Frees an asynchronous request through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_request_free(const H5VL_object_t *vol_obj)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(vol_obj);

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding VOL callback */
    if (H5VL__request_free(vol_obj->data, vol_obj->connector->cls) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "request free failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_request_free() */

/*-------------------------------------------------------------------------
 * Function:    H5VLrequest_free
 *
 * Purpose:     Frees a request
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLrequest_free(void *req, hid_t connector_id)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE2("e", "*xi", req, connector_id);

    /* Get class pointer */
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if (H5VL__request_free(req, cls) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTRELEASE, FAIL, "unable to free request")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLrequest_free() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__blob_put
 *
 * Purpose:     Put a blob through the VOL
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__blob_put(void *obj, const H5VL_class_t *cls, const void *buf, size_t size, void *blob_id, void *ctx)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(obj);
    HDassert(cls);
    HDassert(size == 0 || buf);
    HDassert(blob_id);

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->blob_cls.put)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'blob put' method")

    /* Call the corresponding VOL callback */
    if ((cls->blob_cls.put)(obj, buf, size, blob_id, ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "blob put callback failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__blob_put() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_blob_put
 *
 * Purpose:     Put a blob through the VOL
 *
 * Return:      SUCCEED / FAIL
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, August 21, 2019
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_blob_put(const H5VL_object_t *vol_obj, const void *buf, size_t size, void *blob_id, void *ctx)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(vol_obj);
    HDassert(size == 0 || buf);
    HDassert(blob_id);

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding VOL callback */
    if (H5VL__blob_put(vol_obj->data, vol_obj->connector->cls, buf, size, blob_id, ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "blob put failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_blob_put() */

/*-------------------------------------------------------------------------
 * Function:    H5VLblob_put
 *
 * Purpose:     Put a blob through the VOL
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLblob_put(void *obj, hid_t connector_id, const void *buf, size_t size, void *blob_id, void *ctx)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xi*xz*x*x", obj, connector_id, buf, size, blob_id, ctx);

    /* Get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding VOL callback */
    if (H5VL__blob_put(obj, cls, buf, size, blob_id, ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "blob put failed")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLblob_put() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__blob_get
 *
 * Purpose:     Get a blob through the VOL
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__blob_get(void *obj, const H5VL_class_t *cls, const void *blob_id, void *buf, size_t size, void *ctx)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(obj);
    HDassert(cls);
    HDassert(blob_id);
    HDassert(buf);

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->blob_cls.get)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'blob get' method")

    /* Call the corresponding VOL callback */
    if ((cls->blob_cls.get)(obj, blob_id, buf, size, ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "blob get callback failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__blob_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_blob_get
 *
 * Purpose:     Get a blob through the VOL
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_blob_get(const H5VL_object_t *vol_obj, const void *blob_id, void *buf, size_t size, void *ctx)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(vol_obj);
    HDassert(blob_id);
    HDassert(buf);

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding VOL callback */
    if (H5VL__blob_get(vol_obj->data, vol_obj->connector->cls, blob_id, buf, size, ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "blob get failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_blob_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VLblob_get
 *
 * Purpose:     Get a blob through the VOL
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLblob_get(void *obj, hid_t connector_id, const void *blob_id, void *buf, size_t size, void *ctx)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xi*x*xz*x", obj, connector_id, blob_id, buf, size, ctx);

    /* Get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding VOL callback */
    if (H5VL__blob_get(obj, cls, blob_id, buf, size, ctx) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "blob get failed")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLblob_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__blob_specific
 *
 * Purpose:	Specific operation on blobs through the VOL
 *
 * Return:      SUCCEED / FAIL
 *
 * Programmer:	Quincey Koziol
 *		Saturday, August 17, 2019
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__blob_specific(void *obj, const H5VL_class_t *cls, void *blob_id, H5VL_blob_specific_t specific_type,
                    va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(obj);
    HDassert(cls);
    HDassert(blob_id);

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->blob_cls.specific)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'blob specific' method")

    /* Call the corresponding VOL callback */
    if ((cls->blob_cls.specific)(obj, blob_id, specific_type, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute blob specific callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__blob_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_blob_specific
 *
 * Purpose: Specific operation on blobs through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_blob_specific(const H5VL_object_t *vol_obj, void *blob_id, H5VL_blob_specific_t specific_type, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(vol_obj);
    HDassert(blob_id);

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, specific_type);
    arg_started = TRUE;
    if ((ret_value = H5VL__blob_specific(vol_obj->data, vol_obj->connector->cls, blob_id, specific_type,
                                         arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute blob specific callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_blob_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VLblob_specific
 *
 * Purpose: Specific operation on blobs through the VOL
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLblob_specific(void *obj, hid_t connector_id, void *blob_id, H5VL_blob_specific_t specific_type,
                  va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE5("e", "*xi*xVBx", obj, connector_id, blob_id, specific_type, arguments);

    /* Get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding VOL callback */
    if (H5VL__blob_specific(obj, cls, blob_id, specific_type, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "blob specific operation failed")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLblob_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__blob_optional
 *
 * Purpose:	Optional operation on blobs through the VOL
 *
 * Return:      SUCCEED / FAIL
 *
 * Programmer:	Quincey Koziol
 *		Thursday, November 14, 2019
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__blob_optional(void *obj, const H5VL_class_t *cls, void *blob_id, H5VL_blob_optional_t opt_type,
                    va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(obj);
    HDassert(cls);
    HDassert(blob_id);

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->blob_cls.optional)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'blob optional' method")

    /* Call the corresponding VOL callback */
    if ((cls->blob_cls.optional)(obj, blob_id, opt_type, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute blob optional callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__blob_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_blob_optional
 *
 * Purpose: Optional operation on blobs through the VOL
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_blob_optional(const H5VL_object_t *vol_obj, void *blob_id, H5VL_blob_optional_t opt_type, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(vol_obj);
    HDassert(blob_id);

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, opt_type);
    arg_started = TRUE;
    if ((ret_value =
             H5VL__blob_optional(vol_obj->data, vol_obj->connector->cls, blob_id, opt_type, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute blob optional callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_blob_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VLblob_optional
 *
 * Purpose: Optional operation on blobs through the VOL
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLblob_optional(void *obj, hid_t connector_id, void *blob_id, H5VL_blob_optional_t opt_type,
                  va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE5("e", "*xi*xVAx", obj, connector_id, blob_id, opt_type, arguments);

    /* Get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding VOL callback */
    if (H5VL__blob_optional(obj, cls, blob_id, opt_type, arguments) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "blob optional operation failed")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLblob_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__token_cmp
 *
 * Purpose:     Compares two VOL connector object tokens. Sets *cmp_value
 *              to positive if token1 is greater than token2, negative if
 *              token2 is greater than token1 and zero if token1 and
 *              token2 are equal.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__token_cmp(void *obj, const H5VL_class_t *cls, const H5O_token_t *token1, const H5O_token_t *token2,
                int *cmp_value)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(obj);
    HDassert(cls);
    HDassert(cmp_value);

    /* Take care of cases where one or both pointers is NULL */
    if (token1 == NULL && token2 != NULL)
        *cmp_value = -1;
    else if (token1 != NULL && token2 == NULL)
        *cmp_value = 1;
    else if (token1 == NULL && token2 == NULL)
        *cmp_value = 0;
    else {
        /* Use the class's token comparison routine to compare the tokens,
         * if there is a callback, otherwise just compare the tokens as
         * memory buffers.
         */
        if (cls->token_cls.cmp) {
            if ((cls->token_cls.cmp)(obj, token1, token2, cmp_value) < 0)
                HGOTO_ERROR(H5E_VOL, H5E_CANTCOMPARE, FAIL, "can't compare object tokens")
        } /* end if */
        else
            *cmp_value = HDmemcmp(token1, token2, sizeof(H5O_token_t));
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__token_cmp() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_token_cmp
 *
 * Purpose:     Compares two VOL connector object tokens. Sets *cmp_value
 *              to positive if token1 is greater than token2, negative if
 *              token2 is greater than token1 and zero if token1 and
 *              token2 are equal.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_token_cmp(const H5VL_object_t *vol_obj, const H5O_token_t *token1, const H5O_token_t *token2,
               int *cmp_value)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(vol_obj);
    HDassert(cmp_value);

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if ((ret_value = H5VL__token_cmp(vol_obj->data, vol_obj->connector->cls, token1, token2, cmp_value)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCOMPARE, FAIL, "token compare failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_token_cmp() */

/*---------------------------------------------------------------------------
 * Function:    H5VLtoken_cmp
 *
 * Purpose:     Compares two VOL connector object tokens
 *
 * Note:        Both object tokens must be from the same VOL connector class
 *
 * Return:      Success:    Non-negative, with *cmp_value set to positive if
 *                          token1 is greater than token2, negative if token2
 *                          is greater than token1 and zero if token1 and
 *                          token2 are equal.
 *              Failure:    Negative
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VLtoken_cmp(void *obj, hid_t connector_id, const H5O_token_t *token1, const H5O_token_t *token2,
              int *cmp_value)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE5("e", "*xi*k*k*Is", obj, connector_id, token1, token2, cmp_value);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")
    if (NULL == cmp_value)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid cmp_value pointer")

    /* Call the corresponding internal VOL routine */
    if (H5VL__token_cmp(obj, cls, token1, token2, cmp_value) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCOMPARE, FAIL, "object token comparison failed")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLtoken_cmp() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__token_to_str
 *
 * Purpose:     Serialize a connector's object token into a string
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__token_to_str(void *obj, H5I_type_t obj_type, const H5VL_class_t *cls, const H5O_token_t *token,
                   char **token_str)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(obj);
    HDassert(cls);
    HDassert(token);
    HDassert(token_str);

    /* Use the class's token serialization routine on the token if there is a
     *  callback, otherwise just set the token_str to NULL.
     */
    if (cls->token_cls.to_str) {
        if ((cls->token_cls.to_str)(obj, obj_type, token, token_str) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTSERIALIZE, FAIL, "can't serialize object token")
    } /* end if */
    else
        *token_str = NULL;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__token_to_str() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_token_to_str
 *
 * Purpose:     Serialize a connector's object token into a string
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_token_to_str(const H5VL_object_t *vol_obj, H5I_type_t obj_type, const H5O_token_t *token,
                  char **token_str)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(vol_obj);
    HDassert(token);
    HDassert(token_str);

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if ((ret_value = H5VL__token_to_str(vol_obj->data, obj_type, vol_obj->connector->cls, token, token_str)) <
        0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSERIALIZE, FAIL, "token serialization failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_token_to_str() */

/*---------------------------------------------------------------------------
 * Function:    H5VLtoken_to_str
 *
 * Purpose:     Serialize a connector's object token into a string
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VLtoken_to_str(void *obj, H5I_type_t obj_type, hid_t connector_id, const H5O_token_t *token,
                 char **token_str)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE5("e", "*xIti*k**s", obj, obj_type, connector_id, token, token_str);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")
    if (NULL == token)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid token pointer")
    if (NULL == token_str)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid token_str pointer")

    /* Call the corresponding internal VOL routine */
    if (H5VL__token_to_str(obj, obj_type, cls, token, token_str) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSERIALIZE, FAIL, "object token to string failed")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLtoken_to_str() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__token_from_str
 *
 * Purpose:     Deserialize a string into a connector object token
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__token_from_str(void *obj, H5I_type_t obj_type, const H5VL_class_t *cls, const char *token_str,
                     H5O_token_t *token)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(obj);
    HDassert(cls);
    HDassert(token_str);
    HDassert(token);

    /* Use the class's token deserialization routine on the token if there is a
     *  callback, otherwise just set the token to H5_TOKEN_UNDEF.
     */
    if (cls->token_cls.from_str) {
        if ((cls->token_cls.from_str)(obj, obj_type, token_str, token) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTUNSERIALIZE, FAIL, "can't deserialize object token string")
    } /* end if */
    else
        *token = H5O_TOKEN_UNDEF;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__token_from_str() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_token_from_str
 *
 * Purpose:     Deserialize a string into a connector object token
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_token_from_str(const H5VL_object_t *vol_obj, H5I_type_t obj_type, const char *token_str,
                    H5O_token_t *token)
{
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(vol_obj);
    HDassert(token);
    HDassert(token_str);

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    if ((ret_value =
             H5VL__token_from_str(vol_obj->data, obj_type, vol_obj->connector->cls, token_str, token)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTUNSERIALIZE, FAIL, "token deserialization failed")

done:
    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, FAIL, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_token_from_str() */

/*---------------------------------------------------------------------------
 * Function:    H5VLtoken_from_str
 *
 * Purpose:     Deserialize a string into a connector object token
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VLtoken_from_str(void *obj, H5I_type_t obj_type, hid_t connector_id, const char *token_str,
                   H5O_token_t *token)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE5("e", "*xIti*s*k", obj, obj_type, connector_id, token_str, token);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")
    if (NULL == token)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid token pointer")
    if (NULL == token_str)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid token_str pointer")

    /* Call the corresponding internal VOL routine */
    if (H5VL__token_from_str(obj, obj_type, cls, token_str, token) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTUNSERIALIZE, FAIL, "object token from string failed")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLtoken_from_str() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__optional
 *
 * Purpose:     Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL__optional(void *obj, const H5VL_class_t *cls, int op_type, hid_t dxpl_id, void **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the corresponding VOL callback exists */
    if (NULL == cls->optional)
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "VOL connector has no 'optional' method")

    /* Call the corresponding VOL callback */
    if ((ret_value = (cls->optional)(obj, op_type, dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, ret_value, "unable to execute optional callback")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_optional
 *
 * Purpose:     Optional operation specific to connectors.
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL_optional(const H5VL_object_t *vol_obj, int op_type, hid_t dxpl_id, void **req, ...)
{
    va_list arguments;                 /* Argument list passed from the API call */
    hbool_t arg_started     = FALSE;   /* Whether the va_list has been started */
    hbool_t vol_wrapper_set = FALSE;   /* Whether the VOL object wrapping context was set up */
    herr_t  ret_value       = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Set wrapper info in API context */
    if (H5VL_set_vol_wrapper(vol_obj) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTSET, FAIL, "can't set VOL wrapper info")
    vol_wrapper_set = TRUE;

    /* Call the corresponding internal VOL routine */
    HDva_start(arguments, req);
    arg_started = TRUE;
    if ((ret_value =
             H5VL__optional(vol_obj->data, vol_obj->connector->cls, op_type, dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, FAIL, "unable to execute optional callback")

done:
    /* End access to the va_list, if we started it */
    if (arg_started)
        HDva_end(arguments);

    /* Reset object wrapping info in API context */
    if (vol_wrapper_set && H5VL_reset_vol_wrapper() < 0)
        HDONE_ERROR(H5E_VOL, H5E_CANTRESET, ret_value, "can't reset VOL wrapper info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VLoptional
 *
 * Purpose:     Performs an optional connector-specific operation
 *
 * Return:      Success:    Non-negative
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VLoptional(void *obj, hid_t connector_id, int op_type, hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_class_t *cls;                 /* VOL connector's class struct */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT
    H5TRACE6("e", "*xiIsi**xx", obj, connector_id, op_type, dxpl_id, req, arguments);

    /* Check args and get class pointer */
    if (NULL == obj)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object")
    if (NULL == (cls = (H5VL_class_t *)H5I_object_verify(connector_id, H5I_VOL)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a VOL connector ID")

    /* Call the corresponding internal VOL routine */
    if ((ret_value = H5VL__optional(obj, cls, op_type, dxpl_id, req, arguments)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTOPERATE, ret_value, "unable to execute optional callback")

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5VLoptional() */
