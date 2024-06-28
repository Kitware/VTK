/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expatt - ex_put_attr
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     blk_type                block type
 *       int     blk_id                  block id
 *       float*  attrib                  array of attributes
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, exi_comp_ws, etc

/*!
 * writes the attributes for an edge/face/element block
 * \param   exoid                   exodus file id
 * \param   blk_type                block type
 * \param   blk_id                  block id
 * \param   attrib                  array of attributes
 */

int ex_put_attr(int exoid, ex_entity_type blk_type, ex_entity_id blk_id, const void *attrib)
{
  int  status;
  int  attrid, blk_id_ndx;
  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (blk_type != EX_NODAL) {
    /* Determine index of blk_id in VAR_ID_EL_BLK array */
    blk_id_ndx = exi_id_lkup(exoid, blk_type, blk_id);
    if (blk_id_ndx <= 0) {
      ex_get_err(NULL, NULL, &status);

      if (status != 0) {
        if (status == EX_NULLENTITY) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "Warning: no attributes allowed for NULL %s %" PRId64 " in file id %d",
                   ex_name_of_object(blk_type), blk_id, exoid);
          ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);
          EX_FUNC_LEAVE(EX_WARN); /* no attributes for this block */
        }
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: no %s id %" PRId64 " in in file id %d",
                 ex_name_of_object(blk_type), blk_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }

  switch (blk_type) {
  case EX_SIDE_SET: status = nc_inq_varid(exoid, VAR_SSATTRIB(blk_id_ndx), &attrid); break;
  case EX_NODE_SET: status = nc_inq_varid(exoid, VAR_NSATTRIB(blk_id_ndx), &attrid); break;
  case EX_EDGE_SET: status = nc_inq_varid(exoid, VAR_ESATTRIB(blk_id_ndx), &attrid); break;
  case EX_FACE_SET: status = nc_inq_varid(exoid, VAR_FSATTRIB(blk_id_ndx), &attrid); break;
  case EX_ELEM_SET: status = nc_inq_varid(exoid, VAR_ELSATTRIB(blk_id_ndx), &attrid); break;
  case EX_NODAL: status = nc_inq_varid(exoid, VAR_NATTRIB, &attrid); break;
  case EX_EDGE_BLOCK: status = nc_inq_varid(exoid, VAR_EATTRIB(blk_id_ndx), &attrid); break;
  case EX_FACE_BLOCK: status = nc_inq_varid(exoid, VAR_FATTRIB(blk_id_ndx), &attrid); break;
  case EX_ELEM_BLOCK: status = nc_inq_varid(exoid, VAR_ATTRIB(blk_id_ndx), &attrid); break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Internal ERROR: unrecognized object type in switch: %d in file id %d", blk_type,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL); /* number of attributes not defined */
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate attribute variable for %s %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* write out the attributes  */
  if (exi_comp_ws(exoid) == 4) {
    status = nc_put_var_float(exoid, attrid, attrib);
  }
  else {
    status = nc_put_var_double(exoid, attrid, attrib);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to put attributes for %s %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
