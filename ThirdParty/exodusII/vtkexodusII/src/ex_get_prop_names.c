/*
 * Copyright(C) 1999-2021 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, ATT_PROP_NAME, etc

/*!

The function ex_get_prop_names() returns names of integer properties
stored for an element block, node set, or side set. The number of
properties (needed to allocate space for the property names) can be
obtained via a call to ex_inquire() or ex_inquire_int().

\return In case of an error, ex_get_prop_names() returns a negative number; a
warning will return a positive number.  Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  invalid object type specified.

\param[in]   exoid        exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in]   obj_type     Type of object; use one of the options in the table
below.
\param[out]  prop_names   Returned array containing num_props (obtained from
call to
                          ex_inquire() or ex_inquire_int()) names (of maximum
length
                          \p MAX_STR_LENGTH ) of properties to be stored. \b ID,
a
                          reserved property name, will be the first name in the
array.

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

As an example, the following code segment reads in properties assigned
to node sets:

~~~{.c}
int error, exoid, num_props, *prop_values;
char *prop_names[MAX_PROPS];

\comment{read node set properties}
num_props = ex_inquire_int(exoid, EX_INQ_NS_PROP);

for (i=0; i < num_props; i++) {
   prop_names[i] = (char *) malloc ((MAX_STR_LENGTH+1), sizeof(char));
   prop_values = (int *) malloc (num_node_sets, sizeof(int));
}

error = ex_get_prop_names(exoid,EX_NODE_SET,prop_names);

for (i=0; i < num_props; i++) {
   error = ex_get_prop_array(exoid, EX_NODE_SET, prop_names[i],
                             prop_values);
}
~~~

*/

int ex_get_prop_names(int exoid, ex_entity_type obj_type, char **prop_names)
{
  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* determine which type of object property names are desired for */
  char errmsg[MAX_ERR_LENGTH];
  int  num_props = ex_get_num_props(exoid, obj_type);

  char *var_name;
  for (int i = 0; i < num_props; i++) {
    switch (obj_type) {
    case EX_ELEM_BLOCK: var_name = VAR_EB_PROP(i + 1); break;
    case EX_FACE_BLOCK: var_name = VAR_FA_PROP(i + 1); break;
    case EX_EDGE_BLOCK: var_name = VAR_ED_PROP(i + 1); break;
    case EX_NODE_SET: var_name = VAR_NS_PROP(i + 1); break;
    case EX_SIDE_SET: var_name = VAR_SS_PROP(i + 1); break;
    case EX_EDGE_SET: var_name = VAR_ES_PROP(i + 1); break;
    case EX_FACE_SET: var_name = VAR_FS_PROP(i + 1); break;
    case EX_ELEM_SET: var_name = VAR_ELS_PROP(i + 1); break;
    case EX_ELEM_MAP: var_name = VAR_EM_PROP(i + 1); break;
    case EX_FACE_MAP: var_name = VAR_FAM_PROP(i + 1); break;
    case EX_EDGE_MAP: var_name = VAR_EDM_PROP(i + 1); break;
    case EX_NODE_MAP: var_name = VAR_NM_PROP(i + 1); break;
    default:
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: object type %d not supported; file id %d", obj_type,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    int status;
    int propid;
    if ((status = nc_inq_varid(exoid, var_name, &propid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate property array %s in file id %d",
               var_name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /*   for each property, read the "name" attribute of property array variable
     */
    size_t  att_len;
    nc_type att_type;
    if ((status = nc_inq_att(exoid, propid, ATT_PROP_NAME, &att_type, &att_len)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get property attributes (type, len) in file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    int api_name_size = ex_inquire_int(exoid, EX_INQ_MAX_READ_NAME_LENGTH);
    if (att_len - 1 <= api_name_size) {
      /* Client has large enough char string to hold text... */
      if ((status = nc_get_att_text(exoid, propid, ATT_PROP_NAME, prop_names[i])) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get property name in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
    else {
      /* FIXME */
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: property name length exceeds space available to "
               "store it in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, NC_ESTS);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
