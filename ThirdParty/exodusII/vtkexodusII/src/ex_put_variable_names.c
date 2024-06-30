/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expvan - ex_put_variable_names
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     obj_type                variable type: G,N, or E
 *       int     num_vars                # of variables to read
 *       char*   var_names               ptr array of variable names
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

static int ex_put_var_names_int(int exoid, char *tname, char *dnumvar, char *vnames, int *varid)
{
  int  status;
  int  dimid;
  char errmsg[MAX_ERR_LENGTH];

  if ((status = nc_inq_dimid(exoid, dnumvar, &dimid)) != NC_NOERR) {
    if (status == NC_EBADDIM) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: no %s variables defined in file id %d", tname,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
    }
    else {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate number of %s variables in file id %d", tname, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
    }
    return (EX_FATAL);
  }

  if ((status = nc_inq_varid(exoid, vnames, varid)) != NC_NOERR) {
    if (status == NC_ENOTVAR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: no %s variable names defined in file id %d", tname,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
    }
    else {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: %s name variable names not found in file id %d",
               tname, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
    }
    return (EX_FATAL);
  }
  return (EX_NOERR);
}

/*!
\ingroup ResultsData

The function ex_put_variable_names() writes the names of the results
variables to the database. The maximum length of the names returned is
specified by the return value from ex_inquire_int()(exoid,
EX_INQ_MAX_READ_NAME_LENGTH). The function ex_put_variable_param()
must be called before this function is invoked.

\return In case of an error, ex_put_variable_names() returns a negative
number; a warning will return a positive number.  Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file not initialized properly with call to ex_put_init().
  -  invalid variable type specified.
  -  ex_put_variable_param() was not called previously or was
     called with zero variables of the specified type.
  -  ex_put_variable_names() has been called previously for the
     specified variable type.

\param[in]  exoid      exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in]  obj_type   Variable indicating the type of variable which is
described.
                       Use one of the options in the table below.
\param[in]  num_vars   The number of var_type variables that will be written
                       to the database.
\param[in]  var_names  Array of pointers to num_vars variable names.

| ex_entity_type|  description              |
|---------------|---------------------------|
| #EX_GLOBAL     |  Global entity type       |
| #EX_NODAL      |  Nodal entity type        |
| #EX_NODE_SET   |  Node Set entity type     |
| #EX_EDGE_BLOCK |  Edge Block entity type   |
| #EX_EDGE_SET   |  Edge Set entity type     |
| #EX_FACE_BLOCK |  Face Block entity type   |
| #EX_FACE_SET   |  Face Set entity type     |
| #EX_ELEM_BLOCK |  Element Block entity type|
| #EX_ELEM_SET   |  Element Set entity type  |
| #EX_SIDE_SET   |  Side Set entity type     |

The following coding will write out the names associated with the
nodal variables:
~~~{.c}
int num_nod_vars, error, exoid;
char *var_names[2];

\comment{write results variables parameters and names}
num_nod_vars = 2;

var_names[0] = "disx";
var_names[1] = "disy";

error = ex_put_variable_param (exoid, EX_NODAL, num_nod_vars);
error = ex_put_variable_names (exoid, EX_NODAL, num_nod_vars, var_names);
~~~

*/

int ex_put_variable_names(int exoid, ex_entity_type obj_type, int num_vars, char *const var_names[])
{
  int  varid  = 0;
  int  status = 0;
  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  switch (obj_type) {
  case EX_GLOBAL:
    ex_put_var_names_int(exoid, "global", DIM_NUM_GLO_VAR, VAR_NAME_GLO_VAR, &varid);
    break;
  case EX_NODAL:
    ex_put_var_names_int(exoid, "nodal", DIM_NUM_NOD_VAR, VAR_NAME_NOD_VAR, &varid);
    break;
  case EX_ASSEMBLY:
    ex_put_var_names_int(exoid, "assembly", DIM_NUM_ASSEMBLY_VAR, VAR_NAME_ASSEMBLY_VAR, &varid);
    break;
  case EX_BLOB:
    ex_put_var_names_int(exoid, "blob", DIM_NUM_BLOB_VAR, VAR_NAME_BLOB_VAR, &varid);
    break;
  case EX_EDGE_BLOCK:
    ex_put_var_names_int(exoid, "edge", DIM_NUM_EDG_VAR, VAR_NAME_EDG_VAR, &varid);
    break;
  case EX_FACE_BLOCK:
    ex_put_var_names_int(exoid, "face", DIM_NUM_FAC_VAR, VAR_NAME_FAC_VAR, &varid);
    break;
  case EX_ELEM_BLOCK:
    ex_put_var_names_int(exoid, "element", DIM_NUM_ELE_VAR, VAR_NAME_ELE_VAR, &varid);
    break;
  case EX_NODE_SET:
    ex_put_var_names_int(exoid, "node set", DIM_NUM_NSET_VAR, VAR_NAME_NSET_VAR, &varid);
    break;
  case EX_EDGE_SET:
    ex_put_var_names_int(exoid, "edge set", DIM_NUM_ESET_VAR, VAR_NAME_ESET_VAR, &varid);
    break;
  case EX_FACE_SET:
    ex_put_var_names_int(exoid, "face set", DIM_NUM_FSET_VAR, VAR_NAME_FSET_VAR, &varid);
    break;
  case EX_SIDE_SET:
    ex_put_var_names_int(exoid, "side set", DIM_NUM_SSET_VAR, VAR_NAME_SSET_VAR, &varid);
    break;
  case EX_ELEM_SET:
    ex_put_var_names_int(exoid, "element set", DIM_NUM_ELSET_VAR, VAR_NAME_ELSET_VAR, &varid);
    break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Invalid variable type %d specified in file id %d",
             obj_type, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* write EXODUS variable names */
  status = exi_put_names(exoid, varid, num_vars, var_names, obj_type, "variable", __func__);

  EX_FUNC_LEAVE(status);
}
