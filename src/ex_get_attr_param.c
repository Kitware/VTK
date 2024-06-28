/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expmp - ex_get_attr_param
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid           exodus file id
 *       int     obj_type        block/set type (node, edge, face, elem)
 *       int     obj_id          block/set id (ignored for NODAL)
 *       int     num_attrs       number of attributes
 *
 * exit conditions -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, EX_NOERR, etc

/*!
 * \undoc retrieves the number of attributes.
 */

int ex_get_attr_param(int exoid, ex_entity_type obj_type, ex_entity_id obj_id, int *num_attrs)
{
  int status;
  int dimid;

  char        errmsg[MAX_ERR_LENGTH];
  const char *dnumobjatt;

  int    obj_id_ndx;
  size_t lnum_attr_per_entry;

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
          *num_attrs = 0;
          EX_FUNC_LEAVE(EX_NOERR);
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
  case EX_SIDE_SET: dnumobjatt = DIM_NUM_ATT_IN_SS(obj_id_ndx); break;
  case EX_NODE_SET: dnumobjatt = DIM_NUM_ATT_IN_NS(obj_id_ndx); break;
  case EX_EDGE_SET: dnumobjatt = DIM_NUM_ATT_IN_ES(obj_id_ndx); break;
  case EX_FACE_SET: dnumobjatt = DIM_NUM_ATT_IN_FS(obj_id_ndx); break;
  case EX_ELEM_SET: dnumobjatt = DIM_NUM_ATT_IN_ELS(obj_id_ndx); break;
  case EX_NODAL: dnumobjatt = DIM_NUM_ATT_IN_NBLK; break;
  case EX_EDGE_BLOCK: dnumobjatt = DIM_NUM_ATT_IN_EBLK(obj_id_ndx); break;
  case EX_FACE_BLOCK: dnumobjatt = DIM_NUM_ATT_IN_FBLK(obj_id_ndx); break;
  case EX_ELEM_BLOCK: dnumobjatt = DIM_NUM_ATT_IN_BLK(obj_id_ndx); break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Bad block type (%d) specified for file id %d",
             obj_type, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_dimid(exoid, dnumobjatt, &dimid)) != NC_NOERR) {
    /* dimension is undefined */
    *num_attrs = 0;
  }
  else {
    if ((status = nc_inq_dimlen(exoid, dimid, &lnum_attr_per_entry)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get number of attributes in %s %" PRId64 " in file id %d",
               ex_name_of_object(obj_type), obj_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    *num_attrs = lnum_attr_per_entry;
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
