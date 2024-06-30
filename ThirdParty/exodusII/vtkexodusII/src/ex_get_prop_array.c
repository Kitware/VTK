/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgpa - ex_get_prop_array: read object property array
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     obj_type                type of object (element block, node
 *                                               set or side set)
 *       char*   prop_name               name of the property for which the
 *                                               values will be read
 *
 * exit conditions -
 *       int*    values                  returned array of property values
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, ATT_PROP_NAME, etc
#include <stdbool.h>

/*!

The function ex_get_prop_array() reads an array of integer property
values for all element blocks, node sets, or side sets. The order of
the values in the array correspond to the order in which the element
blocks, node sets, or side sets were introduced into the file. Before
this function is invoked, memory must be allocated for the returned
array of(num_elem_blk, num_node_sets, or {num_side_sets})
integer values.

This function can be used in place of
 - ex_get_elem_blk_ids(),
 - ex_get_node_set_ids(), and
 - ex_get_side_set_ids()
to get element block, node set, and side set IDs, respectively, by
requesting the property name \b ID. One should also note that this
same function can be accomplished with multiple calls to
ex_get_prop().

\return In case of an error, ex_get_prop_array() returns a negative
number; a warning will return a positive number.  Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  invalid object type specified.
  -  a warning value is returned if a property with the specified name is not
found.

\param[in]  exoid      exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in]  obj_type   Type of object; use one of the options in the table
below.
\param[in]  prop_name  The name of the property (maximum length of \p
MAX_STR_LENGTH )
                       for which the values are desired.
\param[out]  values    Returned array of property values.

| #ex_entity_type | description               |
| -------------- | ------------------------- |
|  #EX_NODE_SET   |  Node Set entity type     |
|  #EX_EDGE_BLOCK |  Edge Block entity type   |
|  #EX_EDGE_SET   |  Edge Set entity type     |
|  #EX_FACE_BLOCK |  Face Block entity type   |
|  #EX_FACE_SET   |  Face Set entity type     |
|  #EX_ELEM_BLOCK |  Element Block entity type|
|  #EX_ELEM_SET   |  Element Set entity type  |
|  #EX_SIDE_SET   |  Side Set entity type     |
|  #EX_ELEM_MAP   |  Element Map entity type  |
|  #EX_NODE_MAP   |  Node Map entity type     |
|  #EX_EDGE_MAP   |  Edge Map entity type     |
|  #EX_FACE_MAP   |  Face Map entity type     |

For an example of code to read an array of object properties, refer to
the description for ex_get_prop_names().
*/

int ex_get_prop_array(int exoid, ex_entity_type obj_type, const char *prop_name, void_int *values)
{
  int   num_props, i, propid, status;
  int   found = false;
  char *name;
  char  tmpstr[MAX_STR_LENGTH + 1];

  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* open appropriate variable, depending on obj_type and prop_name */

  num_props = ex_get_num_props(exoid, obj_type);

  for (i = 1; i <= num_props; i++) {
    switch (obj_type) {
    case EX_ELEM_BLOCK: name = VAR_EB_PROP(i); break;
    case EX_EDGE_BLOCK: name = VAR_ED_PROP(i); break;
    case EX_FACE_BLOCK: name = VAR_FA_PROP(i); break;
    case EX_NODE_SET: name = VAR_NS_PROP(i); break;
    case EX_EDGE_SET: name = VAR_ES_PROP(i); break;
    case EX_FACE_SET: name = VAR_FS_PROP(i); break;
    case EX_ELEM_SET: name = VAR_ELS_PROP(i); break;
    case EX_SIDE_SET: name = VAR_SS_PROP(i); break;
    case EX_ELEM_MAP: name = VAR_EM_PROP(i); break;
    case EX_FACE_MAP: name = VAR_FAM_PROP(i); break;
    case EX_EDGE_MAP: name = VAR_EDM_PROP(i); break;
    case EX_NODE_MAP: name = VAR_NM_PROP(i); break;
    default:
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: object type %d not supported; file id %d", obj_type,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = nc_inq_varid(exoid, name, &propid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate property array %s in file id %d",
               name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /*   compare stored attribute name with passed property name   */
    memset(tmpstr, 0, MAX_STR_LENGTH + 1);
    if ((status = nc_get_att_text(exoid, propid, ATT_PROP_NAME, tmpstr)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get property name in file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (strcmp(tmpstr, prop_name) == 0) {
      found = true;
      break;
    }
  }

  /* if property is not found, return warning */
  if (!found) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Warning: object type %d, property %s not defined in file id %d", obj_type, prop_name,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_WARN);
  }

  /* read num_obj values from property variable */
  if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
    status = nc_get_var_longlong(exoid, propid, values);
  }
  else {
    status = nc_get_var_int(exoid, propid, values);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to read values in %s property array in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
