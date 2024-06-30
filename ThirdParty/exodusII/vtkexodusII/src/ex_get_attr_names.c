/*
 * Copyright(C) 1999-2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgeat - ex_get_attr_names
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     obj_type                object type (edge/face/elem block)
 *       int     obj_id                  object id (edge/face/elem block id)
 *
 * exit conditions -
 *       char*   names[]                 ptr array of attribute names
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, ex_name_of_object, etc
#include "exodusII_int.h" // for EX_FATAL, EX_WARN, etc

/*! \undoc */
/*
 * reads the attribute names for an element block
 */
int ex_get_attr_names(int exoid, ex_entity_type obj_type, ex_entity_id obj_id, char **names)
{
  int         status;
  int         varid, numattrdim, obj_id_ndx;
  char        errmsg[MAX_ERR_LENGTH];
  const char *dnumobjatt;
  const char *vattrbname;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Determine index of obj_id in vobjids array */
  if (obj_type != EX_NODAL) {
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
  case EX_NODE_SET:
    dnumobjatt = DIM_NUM_ATT_IN_NS(obj_id_ndx);
    vattrbname = VAR_NAME_NSATTRIB(obj_id_ndx);
    break;
  case EX_SIDE_SET:
    dnumobjatt = DIM_NUM_ATT_IN_SS(obj_id_ndx);
    vattrbname = VAR_NAME_SSATTRIB(obj_id_ndx);
    break;
  case EX_EDGE_SET:
    dnumobjatt = DIM_NUM_ATT_IN_ES(obj_id_ndx);
    vattrbname = VAR_NAME_ESATTRIB(obj_id_ndx);
    break;
  case EX_FACE_SET:
    dnumobjatt = DIM_NUM_ATT_IN_FS(obj_id_ndx);
    vattrbname = VAR_NAME_FSATTRIB(obj_id_ndx);
    break;
  case EX_ELEM_SET:
    dnumobjatt = DIM_NUM_ATT_IN_ELS(obj_id_ndx);
    vattrbname = VAR_NAME_ELSATTRIB(obj_id_ndx);
    break;
  case EX_NODAL:
    dnumobjatt = DIM_NUM_ATT_IN_NBLK;
    vattrbname = VAR_NAME_NATTRIB;
    break;
  case EX_EDGE_BLOCK:
    dnumobjatt = DIM_NUM_ATT_IN_EBLK(obj_id_ndx);
    vattrbname = VAR_NAME_EATTRIB(obj_id_ndx);
    break;
  case EX_FACE_BLOCK:
    dnumobjatt = DIM_NUM_ATT_IN_FBLK(obj_id_ndx);
    vattrbname = VAR_NAME_FATTRIB(obj_id_ndx);
    break;
  case EX_ELEM_BLOCK:
    dnumobjatt = DIM_NUM_ATT_IN_BLK(obj_id_ndx);
    vattrbname = VAR_NAME_ATTRIB(obj_id_ndx);
    break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Internal ERROR: unrecognized object type in switch: %d in file id %d", obj_type,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL); /* number of attributes not defined */
  }
  /* inquire id's of previously defined dimensions  */

  if ((status = nc_inq_dimid(exoid, dnumobjatt, &numattrdim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Warning: no attributes found for %s %" PRId64 " in file id %d",
             ex_name_of_object(obj_type), obj_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_WARN); /* no attributes for this object */
  }

  size_t num_attr = 0;
  if ((status = nc_inq_dimlen(exoid, numattrdim, &num_attr)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to get number of attributes for %s %" PRId64 " in file id %d",
             ex_name_of_object(obj_type), obj_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* It is OK if we don't find the attribute names since they were
     added at version 4.26; earlier databases won't have the names.
  */
  status = nc_inq_varid(exoid, vattrbname, &varid);

  /* read in the attributes */

  if (status == NC_NOERR) {
    /* read the names */
    status = exi_get_names(exoid, varid, num_attr, names, obj_type, __func__);
    if (status != NC_NOERR) {
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  else {
    /* Names variable does not exist on the database; probably since this is an
     * older version of the database.  Return an empty array...
     */
    for (size_t i = 0; i < num_attr; i++) {
      names[i][0] = '\0';
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
