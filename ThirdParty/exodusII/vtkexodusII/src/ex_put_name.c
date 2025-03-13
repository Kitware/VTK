/*
 * Copyright(C) 1999-2020, 2022, 2023 National Technology & Engineering Solutions
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
#include "exodusII_int.h" // for EX_FATAL, exi_id_lkup, etc

int exi_put_assembly_name(int exoid, ex_entity_type obj_type, ex_entity_id entity_id,
                          const char *name)
{
  /* Internal function to handle renaming of an existing assembly.
     Note that assembly must exist or an error will be returned.
  */
  /* See if an assembly with this id has already been defined or exists on file... */
  int  entlst_id = 0;
  char errmsg[MAX_ERR_LENGTH];
  if (nc_inq_varid(exoid, VAR_ENTITY_ASSEMBLY(entity_id), &entlst_id) == NC_NOERR) {
    int status;
    if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    size_t length = strlen(name) + 1;
    if ((status = nc_put_att_text(exoid, entlst_id, EX_ATTRIBUTE_NAME, length, name)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store assembly name %s in file id %d",
               name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to exit define mode in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      EX_FUNC_LEAVE(EX_FATAL);
    }
    /* Update the maximum_name_length attribute on the file. */
    exi_update_max_name_length(exoid, length - 1);
    if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to exit define mode in file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    EX_FUNC_LEAVE(EX_NOERR);
  }

  /* Assembly not found... */
  snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: %s id %" PRId64 " not found in file id %d",
           ex_name_of_object(obj_type), entity_id, exoid);
  ex_err_fn(exoid, __func__, errmsg, EX_LOOKUPFAIL);
  EX_FUNC_LEAVE(EX_FATAL);
}

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
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  switch (obj_type) {
  case EX_ASSEMBLY: return exi_put_assembly_name(exoid, obj_type, entity_id, name);
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

  ent_ndx = exi_id_lkup(exoid, obj_type, entity_id);
  if (ent_ndx == -EX_LOOKUPFAIL) { /* could not find the entity with `entity_id` */
    if (obj_type == EX_NODE_MAP || obj_type == EX_ELEM_MAP || obj_type == EX_FACE_MAP ||
        obj_type == EX_EDGE_MAP) {
      ent_ndx = entity_id;
    }
    else {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: %s id %" PRId64 " not found in file id %d",
               ex_name_of_object(obj_type), entity_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_LOOKUPFAIL);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* If this is a null entity, then 'ent_ndx' will be negative.
   * We don't care in this function, so make it positive and continue...
   */
  if (ent_ndx < 0) {
    ent_ndx = -ent_ndx;
  }

  /* write EXODUS entityname */
  status = exi_put_name(exoid, varid, ent_ndx - 1, name, obj_type, "", __func__);

  EX_FUNC_LEAVE(status);
}
