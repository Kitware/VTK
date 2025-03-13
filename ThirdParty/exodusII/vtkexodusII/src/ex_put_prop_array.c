/*
 * Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, ATT_PROP_NAME, etc
#include <stdbool.h>

/*!

The function ex_put_prop_array() stores an array of ({num_elem_blk},
num_node_sets, or num_side_sets) integer property values for all
element blocks, node sets, or side sets. The order of the values in
the array must correspond to the order in which the element blocks,
node sets, or side sets were introduced into the file. For instance,
if the parameters for element block with ID 20 were written to a file
(via ex_put_elem_block()), and then parameters for element block with
ID 10, followed by the parameters for element block with ID 30, the
first, second, and third elements in the property array would
correspond to element block 20, element block 10, and element block
30, respectively.

One should note that this same functionality (writing properties to
multiple objects) can be accomplished with multiple calls to
ex_put_prop().

Although it is not necessary to invoke ex_put_prop_names(), since
ex_put_prop_array() will allocate space within the data file if it
hasn't been previously allocated, it is more efficient to use
ex_put_prop_names() if there is more than one property to store.

\return In case of an error, ex_put_prop_array() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  invalid object type specified.

\param[in] exoid      exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in] obj_type   Type of object; use one of the options in the table below.
\param[in] prop_name  The name of the property for which the values will be
stored. Maximum
                      length of this string is \p MAX_STR_LENGTH .
\param[in] values     An array of property values.

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

For an example of code to write an array of object properties, refer
to the description for ex_put_prop_names().
 */

int ex_put_prop_array(int exoid, ex_entity_type obj_type, const char *prop_name,
                      const void_int *values)
{
  int    oldfill = 0;
  int    temp;
  int    num_props, i, propid, dimid, dims[1], status;
  bool   found = false;
  int    int_type;
  size_t num_obj;
  char  *name;
  char   tmpstr[MAX_STR_LENGTH + 1];

  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* check if property has already been created */

  num_props = ex_get_num_props(exoid, obj_type);

  /* inquire id of previously defined dimension (number of objects) */
  status = exi_get_dimension(exoid, exi_dim_num_objects(obj_type), ex_name_of_object(obj_type),
                             &num_obj, &dimid, __func__);
  if (status != NC_NOERR) {
    EX_FUNC_LEAVE(status);
  }

  for (i = 1; i <= num_props; i++) {
    switch (obj_type) {
    case EX_ELEM_BLOCK: name = VAR_EB_PROP(i); break;
    case EX_FACE_BLOCK: name = VAR_FA_PROP(i); break;
    case EX_EDGE_BLOCK: name = VAR_ED_PROP(i); break;
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
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get property array id in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* compare stored attribute name with passed property name   */
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

  /* if property array has not been created, create it */
  if (!found) {

    /* put netcdf file into define mode  */
    if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to place file id %d into define mode", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /*   create a variable with a name xx_prop#, where # is the new number   */
    /*   of properties                                                       */
    switch (obj_type) {
    case EX_ELEM_BLOCK: name = VAR_EB_PROP(num_props + 1); break;
    case EX_FACE_BLOCK: name = VAR_FA_PROP(num_props + 1); break;
    case EX_EDGE_BLOCK: name = VAR_ED_PROP(num_props + 1); break;
    case EX_NODE_SET: name = VAR_NS_PROP(num_props + 1); break;
    case EX_EDGE_SET: name = VAR_ES_PROP(num_props + 1); break;
    case EX_FACE_SET: name = VAR_FS_PROP(num_props + 1); break;
    case EX_ELEM_SET: name = VAR_ELS_PROP(num_props + 1); break;
    case EX_SIDE_SET: name = VAR_SS_PROP(num_props + 1); break;
    case EX_ELEM_MAP: name = VAR_EM_PROP(num_props + 1); break;
    case EX_FACE_MAP: name = VAR_FAM_PROP(num_props + 1); break;
    case EX_EDGE_MAP: name = VAR_EDM_PROP(num_props + 1); break;
    case EX_NODE_MAP: name = VAR_NM_PROP(num_props + 1); break;
    default:
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: object type %d not supported; file id %d", obj_type,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      goto error_ret; /* Exit define mode and return */
    }

    dims[0] = dimid;
    nc_set_fill(exoid, NC_FILL, &oldfill); /* fill with zeros per routine spec */

    int_type = NC_INT;
    if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
      int_type = NC_INT64;
    }

    if ((status = nc_def_var(exoid, name, int_type, 1, dims, &propid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to create property array variable in file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* Exit define mode and return */
    }
    nc_set_fill(exoid, oldfill, &temp); /* default: nofill */

    /*   store property name as attribute of property array variable */
    if ((status = nc_put_att_text(exoid, propid, ATT_PROP_NAME, strlen(prop_name) + 1,
                                  prop_name)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store property name %s in file id %d",
               prop_name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* Exit define mode and return */
    }

    /* leave define mode  */

    if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to exit define mode");
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* put num_obj values in property array */
  if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
    status = nc_put_var_longlong(exoid, propid, values);
  }
  else {
    status = nc_put_var_int(exoid, propid, values);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store property values in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  nc_set_fill(exoid, oldfill, &temp); /* default: nofill */
  exi_leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}
