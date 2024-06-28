/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, EX_WARN, etc
#include <stdbool.h>

/*!

The function ex_get_prop() reads an integer object property value
stored for a single element block, node set, or side set.

\return In case of an error, ex_get_prop() returns a negative number; a
warning will return a positive number.  Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  invalid object type specified.
  -  a warning value is returned if a property with the specified name is not
found.

\param[in] exoid      exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in] obj_type   Type of object; use one of the options in the table below.
\param[in] obj_id     The element block, node set, or side set ID.
\param[in] prop_name  The name of the property (maximum length is \p
MAX_STR_LENGTH ) for
                      which the value is desired.
\param[out]  value    Returned value of the property.

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

For an example of code to read an object property, refer to the
description for ex_get_prop_names().

*/

int ex_get_prop(int exoid, ex_entity_type obj_type, ex_entity_id obj_id, const char *prop_name,
                void_int *value)
{
  int    status;
  int    num_props, i, propid;
  int    found = false;
  size_t start[1];
  char  *name;
  char   tmpstr[MAX_STR_LENGTH + 1];

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
    snprintf(errmsg, MAX_ERR_LENGTH, "Warning: %s property %s not defined in file id %d",
             ex_name_of_object(obj_type), prop_name, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_WARN);
  }

  /* find index into property array using obj_id; read value from property */
  /* array at proper index; exi_id_lkup returns an index that is 1-based,   */
  /* but netcdf expects 0-based arrays so subtract 1                       */
  status = exi_id_lkup(exoid, obj_type, obj_id);
  if (status > 0) {
    start[0] = status - 1;
  }
  else {
    ex_get_err(NULL, NULL, &status);

    if (status != 0) {
      if (status == EX_NULLENTITY) {
        snprintf(errmsg, MAX_ERR_LENGTH, "Warning: %s id %" PRId64 " is NULL in file id %d",
                 ex_name_of_object(obj_type), obj_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);
        EX_FUNC_LEAVE(EX_WARN);
      }
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate id %" PRId64 " in %s property array in file id %d", obj_id,
               ex_name_of_object(obj_type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
    long long l_val;
    status = nc_get_var1_longlong(exoid, propid, start, &l_val);
    if (status == NC_NOERR) {
      int64_t *val = (int64_t *)value;
      *val         = l_val;
    }
  }
  else {
    int i_val;
    status = nc_get_var1_int(exoid, propid, start, &i_val);
    if (status == NC_NOERR) {
      int *val = (int *)value;
      *val     = i_val;
    }
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to read value in %s property array in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
