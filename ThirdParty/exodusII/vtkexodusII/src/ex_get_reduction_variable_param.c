/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, EX_NOERR, etc

/*!
\ingroup ResultsData

The function ex_get_reduction_variable_param() reads the number of global,
nodal, or element variables stored in the database.

\return In case of an error, ex_get_reduction_variable_param() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  invalid variable type specified.

\param[in]  exoid     exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in]  obj_type  Variable indicating the type of reduction variable which is
described. Use one
                      of the options in the table below.
\param[out] num_vars  Returned number of  var_type variables that are stored
in the database.

| ex_entity_type|  description              |
|---------------|---------------------------|
| #EX_GLOBAL     |  Global entity type       |
| #EX_NODE_SET   |  Node Set entity type     |
| #EX_EDGE_BLOCK |  Edge Block entity type   |
| #EX_EDGE_SET   |  Edge Set entity type     |
| #EX_FACE_BLOCK |  Face Block entity type   |
| #EX_FACE_SET   |  Face Set entity type     |
| #EX_ELEM_BLOCK |  Element Block entity type|
| #EX_ELEM_SET   |  Element Set entity type  |
| #EX_SIDE_SET   |  Side Set entity type     |

As an example, the following coding will determine the number of
global variables stored in the data file:

~~~{.c}
int num_glo_vars, error, exoid;

\comment{read global variables parameters}
error = ex_get_variable_param(exoid, EX_GLOBAL, &num_glo_vars);
~~~

*/

int ex_get_reduction_variable_param(int exoid, ex_entity_type obj_type, int *num_vars)
{
  int         dimid;
  size_t      dimlen;
  char        errmsg[MAX_ERR_LENGTH];
  const char *dnumvar;
  int         status;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  *num_vars = 0;

  switch (obj_type) {
  case EX_GLOBAL: dnumvar = DIM_NUM_GLO_VAR; break;
  case EX_ASSEMBLY: dnumvar = DIM_NUM_ASSEMBLY_RED_VAR; break;
  case EX_BLOB: dnumvar = DIM_NUM_BLOB_RED_VAR; break;
  case EX_EDGE_BLOCK: dnumvar = DIM_NUM_EDG_RED_VAR; break;
  case EX_FACE_BLOCK: dnumvar = DIM_NUM_FAC_RED_VAR; break;
  case EX_ELEM_BLOCK: dnumvar = DIM_NUM_ELE_RED_VAR; break;
  case EX_NODE_SET: dnumvar = DIM_NUM_NSET_RED_VAR; break;
  case EX_EDGE_SET: dnumvar = DIM_NUM_ESET_RED_VAR; break;
  case EX_FACE_SET: dnumvar = DIM_NUM_FSET_RED_VAR; break;
  case EX_SIDE_SET: dnumvar = DIM_NUM_SSET_RED_VAR; break;
  case EX_ELEM_SET: dnumvar = DIM_NUM_ELSET_RED_VAR; break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Warning: invalid reduction variable type %d requested from file id %d", obj_type,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_WARN);
  }

  if ((status = nc_inq_dimid(exoid, dnumvar, &dimid)) != NC_NOERR) {
    *num_vars = 0;
    if (status == NC_EBADDIM) {
      EX_FUNC_LEAVE(EX_NOERR); /* no reduction variables defined */
    }

    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate %s reduction variable names in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_dimlen(exoid, dimid, &dimlen)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to get number of %s reduction variables in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  *num_vars = dimlen;

  EX_FUNC_LEAVE(EX_NOERR);
}
