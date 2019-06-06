/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Purpose:    Deprecated functions from the H5O interface.  These
 *              functions are here for compatibility purposes and may be
 *              removed in the future.  Applications should switch to the
 *              newer APIs.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5Omodule.h"          /* This source code file is part of the H5O module */


/***********/
/* Headers */
/***********/
#include "H5private.h"      /* Generic Functions    */
#include "H5Eprivate.h"     /* Error handling       */
#include "H5CXprivate.h"    /* Property list handling       */
#include "H5Opkg.h"         /* Object headers       */


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


/* Compatibility function prototypes to replace non-versioned function for use by macro in next version
 * see HDFFV-10552
#ifndef H5_NO_DEPRECATED_SYMBOLS
 */

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
H5Oget_info1(hid_t loc_id, H5O_info_t *oinfo)
{
    H5G_loc_t   loc;                    /* Location of group */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", loc_id, oinfo);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!oinfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no info struct")

    /* Retrieve the object's information */
    if(H5G_loc_info(&loc, ".", oinfo/*out*/, H5O_INFO_ALL) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't retrieve object info")

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
H5Oget_info_by_name1(hid_t loc_id, const char *name, H5O_info_t *oinfo, hid_t lapl_id)
{
    H5G_loc_t   loc;                    /* Location of group */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "i*s*xi", loc_id, name, oinfo, lapl_id);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
    if(!oinfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no info struct")

     /* Verify access property list and set up collective metadata if appropriate */
    if(H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Retrieve the object's information */
    if(H5G_loc_info(&loc, name, oinfo/*out*/, H5O_INFO_ALL) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't get info for object: '%s'", name)

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
H5Oget_info_by_idx1(hid_t loc_id, const char *group_name, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t n, H5O_info_t *oinfo, hid_t lapl_id)
{
    H5G_loc_t   loc;                    /* Location of group */
    H5G_loc_t   obj_loc;                /* Location used to open group */
    H5G_name_t  obj_path;               /* Opened object group hier. path */
    H5O_loc_t   obj_oloc;               /* Opened object object location */
    hbool_t     loc_found = FALSE;      /* Entry at 'name' found */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE7("e", "i*sIiIoh*xi", loc_id, group_name, idx_type, order, n, oinfo,
             lapl_id);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!group_name || !*group_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if(!oinfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no info struct")

    /* Verify access property list and set up collective metadata if appropriate */
    if(H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Retrieve the object's information */
    if(H5O__get_info_by_idx(&loc, group_name, idx_type, order, n, oinfo, H5O_INFO_ALL) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't get info for object")

done:
    /* Release the object location */
    if(loc_found && H5G_loc_free(&obj_loc) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTRELEASE, FAIL, "can't free location")

    FUNC_LEAVE_API(ret_value)
} /* end H5Oget_info_by_idx1() */


/*-------------------------------------------------------------------------
 * Function:    H5Ovisit1
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
 *                returns non-zero, or zero if all members were
 *                processed with no operator returning non-zero.
 *
 *              Failure:    Negative if something goes wrong within the
 *                library, or the negative value returned by one
 *                of the operators.
 *
 * Programmer:    Quincey Koziol
 *              November 25 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ovisit1(hid_t obj_id, H5_index_t idx_type, H5_iter_order_t order,
    H5O_iterate_t op, void *op_data)
{
    herr_t      ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "iIiIox*x", obj_id, idx_type, order, op, op_data);

    /* Check args */
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if(!op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no callback operator specified")

    /* Call internal object visitation routine */
    if((ret_value = H5O__visit(obj_id, ".", idx_type, order, op, op_data, H5O_INFO_ALL)) < 0)
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
 *                returns non-zero, or zero if all members were
 *                processed with no operator returning non-zero.
 *
 *              Failure:    Negative if something goes wrong within the
 *                library, or the negative value returned by one
 *                of the operators.
 *
 * Programmer:    Quincey Koziol
 *              November 24 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ovisit_by_name1(hid_t loc_id, const char *obj_name, H5_index_t idx_type,
    H5_iter_order_t order, H5O_iterate_t op, void *op_data, hid_t lapl_id)
{
    herr_t      ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE7("e", "i*sIiIox*xi", loc_id, obj_name, idx_type, order, op, op_data,
             lapl_id);

    /* Check args */
    if(!obj_name || !*obj_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if(!op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no callback operator specified")

    /* Verify access property list and set up collective metadata if appropriate */
    if(H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Call internal object visitation routine */
    if((ret_value = H5O__visit(loc_id, obj_name, idx_type, order, op, op_data, H5O_INFO_ALL)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_BADITER, FAIL, "object visitation failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ovisit_by_name1() */

/* Compatibility function prototypes to replace non-versioned function for use by macro in next version
 * see HDFFV-10552
#endif (* H5_NO_DEPRECATED_SYMBOLS *)
 */
