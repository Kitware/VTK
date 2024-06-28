/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expvnm - ex_put_variable_name
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     obj_type                variable type: G,N, or E
 *       int     var_num                 variable number name to write 1..num_var
 *       char*   var_name                ptr of variable name
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_WARN, etc

/*!
\ingroup ResultsData

 * writes the name of a particular results variable to the database
 *  \param     exoid                   exodus file id
 *  \param     obj_type                variable type
 *  \param     var_num                 variable number name to write 1..num_var
 *  \param    *var_name                ptr of variable name
 */

int ex_put_reduction_variable_name(int exoid, ex_entity_type obj_type, int var_num,
                                   const char *var_name)
{
  int         status;
  int         varid;
  char        errmsg[MAX_ERR_LENGTH];
  const char *vname;

  EX_FUNC_ENTER();

  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* inquire previously defined variables  */
  switch (obj_type) {
  case EX_GLOBAL: vname = VAR_NAME_GLO_VAR; break;
  case EX_EDGE_BLOCK: vname = VAR_NAME_EDG_RED_VAR; break;
  case EX_FACE_BLOCK: vname = VAR_NAME_FAC_RED_VAR; break;
  case EX_ELEM_BLOCK: vname = VAR_NAME_ELE_RED_VAR; break;
  case EX_NODE_SET: vname = VAR_NAME_NSET_RED_VAR; break;
  case EX_EDGE_SET: vname = VAR_NAME_ESET_RED_VAR; break;
  case EX_FACE_SET: vname = VAR_NAME_FSET_RED_VAR; break;
  case EX_SIDE_SET: vname = VAR_NAME_SSET_RED_VAR; break;
  case EX_ELEM_SET: vname = VAR_NAME_ELSET_RED_VAR; break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Invalid variable type (%d) given for file id %d",
             obj_type, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_WARN);
  }

  if ((status = nc_inq_varid(exoid, vname, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "Warning: no %s variables names stored in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_WARN);
  }

  /* write EXODUS variable name */
  status = exi_put_name(exoid, varid, var_num - 1, var_name, obj_type, "variable", __func__);

  EX_FUNC_LEAVE(status);
}
