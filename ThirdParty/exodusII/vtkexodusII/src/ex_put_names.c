/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expnam - ex_put_names
 *
 * environment - UNIX
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid       exodus file id
 *       int     obj_type    object type
 *       char*   names       ptr array of entity names
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*!
 * writes the entity names to the database
 * \param exoid       exodus file id
 * \param obj_type    object type
 * \param names       ptr array of entity names
 */

int ex_put_names(int exoid, ex_entity_type obj_type, char *const names[])
{
  int    status;
  int    varid;
  size_t num_entity;
  char   errmsg[MAX_ERR_LENGTH];

  const char *vname = NULL;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  switch (obj_type) {
  /*  ======== ASSEMBLY ========= */
  case EX_ASSEMBLY:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: Assembly names are written using `ex_put_assembly()` function");
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
    break;

  /*  ======== BLOCKS ========= */
  case EX_EDGE_BLOCK: vname = VAR_NAME_ED_BLK; break;
  case EX_FACE_BLOCK: vname = VAR_NAME_FA_BLK; break;
  case EX_ELEM_BLOCK: vname = VAR_NAME_EL_BLK; break;

  /*  ======== SETS ========= */
  case EX_NODE_SET: vname = VAR_NAME_NS; break;
  case EX_EDGE_SET: vname = VAR_NAME_ES; break;
  case EX_FACE_SET: vname = VAR_NAME_FS; break;
  case EX_SIDE_SET: vname = VAR_NAME_SS; break;
  case EX_ELEM_SET: vname = VAR_NAME_ELS; break;

  /*  ======== MAPS ========= */
  case EX_NODE_MAP: vname = VAR_NAME_NM; break;
  case EX_EDGE_MAP: vname = VAR_NAME_EDM; break;
  case EX_FACE_MAP: vname = VAR_NAME_FAM; break;
  case EX_ELEM_MAP: vname = VAR_NAME_EM; break;

  /*  ======== ERROR (Invalid type) ========= */
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Invalid type specified in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  exi_get_dimension(exoid, exi_dim_num_objects(obj_type), ex_name_of_object(obj_type), &num_entity,
                    &varid, __func__);

  if ((status = nc_inq_varid(exoid, vname, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s names in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* write EXODUS entitynames */
  status = exi_put_names(exoid, varid, num_entity, names, obj_type, "", __func__);

  EX_FUNC_LEAVE(status);
}
