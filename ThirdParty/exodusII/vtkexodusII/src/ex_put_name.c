/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expnam - ex_put_name
 *
 * environment - UNIX
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid          exodus file id
 *       int     obj_type       object type
 *       int     entity_id      id of entity name to write
 *       char*   name           ptr to entity name
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, ex__id_lkup, etc

/*!
 * writes the name of the specified entity to the database. The entity
 * with id `entity_id` must exist before calling ex_put_name().
 *
 * \param  exoid          exodus file id
 * \param  obj_type       object type
 * \param  entity_id      id of entity name to write
 * \param  name           ptr to entity name
 */

int ex_put_name(int exoid, ex_entity_type obj_type, ex_entity_id entity_id, const char *name)
{
  int         status;
  int         varid, ent_ndx;
  char        errmsg[MAX_ERR_LENGTH];
  const char *vobj;

  EX_FUNC_ENTER();
  if (ex__check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  switch (obj_type) {
  case EX_ASSEMBLY:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: Assembly name is written using `ex_put_assembly()` function");
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
    break;
  case EX_EDGE_BLOCK: vobj = VAR_NAME_ED_BLK; break;
  case EX_FACE_BLOCK: vobj = VAR_NAME_FA_BLK; break;
  case EX_ELEM_BLOCK: vobj = VAR_NAME_EL_BLK; break;
  case EX_NODE_SET: vobj = VAR_NAME_NS; break;
  case EX_SIDE_SET: vobj = VAR_NAME_SS; break;
  case EX_EDGE_SET: vobj = VAR_NAME_ES; break;
  case EX_FACE_SET: vobj = VAR_NAME_FS; break;
  case EX_ELEM_SET: vobj = VAR_NAME_ELS; break;
  case EX_NODE_MAP: vobj = VAR_NAME_NM; break;
  case EX_EDGE_MAP: vobj = VAR_NAME_EDM; break;
  case EX_FACE_MAP: vobj = VAR_NAME_FAM; break;
  case EX_ELEM_MAP: vobj = VAR_NAME_EM; break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Invalid type specified in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_varid(exoid, vobj, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s names in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  ent_ndx = ex__id_lkup(exoid, obj_type, entity_id);

  if (ent_ndx == -EX_LOOKUPFAIL) { /* could not find the element block id */
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: %s id %" PRId64 " not found in file id %d",
             ex_name_of_object(obj_type), entity_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LOOKUPFAIL);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* If this is a null entity, then 'ent_ndx' will be negative.
   * We don't care in this __func__, so make it positive and continue...
   */
  if (ent_ndx < 0) {
    ent_ndx = -ent_ndx;
  }

  /* write EXODUS entityname */
  status = ex__put_name(exoid, varid, ent_ndx - 1, name, obj_type, "", __func__);

  EX_FUNC_LEAVE(status);
}
