/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgatt - ex_get_attr
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     obj_type                object type (edge/face/element block)
 *       int     obj_id                  object id (edge id, face id, elem id)
 *
 * exit conditions -
 *       float*  attrib                  array of attributes
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, EX_WARN, etc

/*!
 * \undoc reads the attributes for an edge, face, or element block
 */

int ex_get_attr(int exoid, ex_entity_type obj_type, ex_entity_id obj_id, void *attrib)

{
  int         status;
  int         attrid, obj_id_ndx;
  char        errmsg[MAX_ERR_LENGTH];
  const char *vattrbname;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Determine index of obj_id in vobjids array */
  if (obj_type == EX_NODAL) {
    obj_id_ndx = 0;
  }
  else {
    obj_id_ndx = exi_id_lkup(exoid, obj_type, obj_id);
    if (obj_id_ndx <= 0) {
      ex_get_err(NULL, NULL, &status);

      if (status != 0) {
        if (status == EX_NULLENTITY) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "Warning: no attributes found for NULL %s %" PRId64 " in file id %d",
                   ex_name_of_object(obj_type), obj_id, exoid);
          ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);
          EX_FUNC_LEAVE(EX_WARN); /* no attributes for this object */
        }
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "Warning: failed to locate %s id %" PRId64 " in id array in file id %d",
                 ex_name_of_object(obj_type), obj_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_WARN);
      }
    }
  }

  switch (obj_type) {
  case EX_SIDE_SET: vattrbname = VAR_SSATTRIB(obj_id_ndx); break;
  case EX_NODE_SET: vattrbname = VAR_NSATTRIB(obj_id_ndx); break;
  case EX_EDGE_SET: vattrbname = VAR_ESATTRIB(obj_id_ndx); break;
  case EX_FACE_SET: vattrbname = VAR_FSATTRIB(obj_id_ndx); break;
  case EX_ELEM_SET: vattrbname = VAR_ELSATTRIB(obj_id_ndx); break;
  case EX_NODAL: vattrbname = VAR_NATTRIB; break;
  case EX_EDGE_BLOCK: vattrbname = VAR_EATTRIB(obj_id_ndx); break;
  case EX_FACE_BLOCK: vattrbname = VAR_FATTRIB(obj_id_ndx); break;
  case EX_ELEM_BLOCK: vattrbname = VAR_ATTRIB(obj_id_ndx); break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Internal ERROR: unrecognized object type in switch: %d in file id %d", obj_type,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL); /* number of attributes not defined */
  }

  /* inquire id's of previously defined dimensions  */
  if ((status = nc_inq_varid(exoid, vattrbname, &attrid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate attributes for %s %" PRId64 " in file id %d",
             ex_name_of_object(obj_type), obj_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* read in the attributes */
  if (exi_comp_ws(exoid) == 4) {
    status = nc_get_var_float(exoid, attrid, attrib);
  }
  else {
    status = nc_get_var_double(exoid, attrid, attrib);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to get attributes for %s %" PRId64 " in file id %d",
             ex_name_of_object(obj_type), obj_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
