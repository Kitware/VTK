/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
*
* expatn - ex_put_attr_names
*
* entry conditions -
*   input parameters:
*       int     exoid                   exodus file id
*       int     blk_type                block type (edge, face, elem)
*       int     blk_id                  block id
*       char*   names                   ptr to array of attribute names

*
* exit conditions -
*
* revision history -
*
*
*****************************************************************************/

#include "exodusII.h"     // for ex_err, ex_name_of_object, etc
#include "exodusII_int.h" // for EX_FATAL, exi_id_lkup, etc

/*!
 * writes the attribute names for a block
 * \param   exoid                   exodus file id
 * \param   blk_type                block type (edge, face, elem)
 * \param   blk_id                  block id
 * \param   names                   ptr to array of attribute names
 */
int ex_put_attr_names(int exoid, ex_entity_type blk_type, ex_entity_id blk_id, char *names[])
{
  int    varid, numattrdim, blk_id_ndx;
  size_t num_attr;

  int  status;
  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

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
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: no %s id %" PRId64 " in %s array in file id %d",
               ex_name_of_object(blk_type), blk_id, VAR_ID_EL_BLK, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* inquire id's of previously defined dimensions  */
  switch (blk_type) {
  case EX_SIDE_SET: status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_SS(blk_id_ndx), &numattrdim); break;
  case EX_NODE_SET: status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_NS(blk_id_ndx), &numattrdim); break;
  case EX_EDGE_SET: status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_ES(blk_id_ndx), &numattrdim); break;
  case EX_FACE_SET: status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_FS(blk_id_ndx), &numattrdim); break;
  case EX_ELEM_SET:
    status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_ELS(blk_id_ndx), &numattrdim);
    break;
  case EX_NODAL: status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_NBLK, &numattrdim); break;
  case EX_EDGE_BLOCK:
    status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_EBLK(blk_id_ndx), &numattrdim);
    break;
  case EX_FACE_BLOCK:
    status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_FBLK(blk_id_ndx), &numattrdim);
    break;
  case EX_ELEM_BLOCK:
    status = nc_inq_dimid(exoid, DIM_NUM_ATT_IN_BLK(blk_id_ndx), &numattrdim);
    break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Internal ERROR: unrecognized object type in switch: %d in file id %d", blk_type,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL); /* number of attributes not defined */
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: number of attributes not defined for %s %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MSG);
    EX_FUNC_LEAVE(EX_FATAL); /* number of attributes not defined */
  }

  if ((status = nc_inq_dimlen(exoid, numattrdim, &num_attr)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to get number of attributes for %s %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  switch (blk_type) {
  case EX_SIDE_SET: status = nc_inq_varid(exoid, VAR_NAME_SSATTRIB(blk_id_ndx), &varid); break;
  case EX_NODE_SET: status = nc_inq_varid(exoid, VAR_NAME_NSATTRIB(blk_id_ndx), &varid); break;
  case EX_EDGE_SET: status = nc_inq_varid(exoid, VAR_NAME_ESATTRIB(blk_id_ndx), &varid); break;
  case EX_FACE_SET: status = nc_inq_varid(exoid, VAR_NAME_FSATTRIB(blk_id_ndx), &varid); break;
  case EX_ELEM_SET: status = nc_inq_varid(exoid, VAR_NAME_ELSATTRIB(blk_id_ndx), &varid); break;
  case EX_NODAL: status = nc_inq_varid(exoid, VAR_NAME_NATTRIB, &varid); break;
  case EX_EDGE_BLOCK: status = nc_inq_varid(exoid, VAR_NAME_EATTRIB(blk_id_ndx), &varid); break;
  case EX_FACE_BLOCK: status = nc_inq_varid(exoid, VAR_NAME_FATTRIB(blk_id_ndx), &varid); break;
  case EX_ELEM_BLOCK: status = nc_inq_varid(exoid, VAR_NAME_ATTRIB(blk_id_ndx), &varid); break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Internal ERROR: unrecognized object type in switch: %d in file id %d", blk_type,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL); /* number of attributes not defined */
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate %s attribute names for %s %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* write out the attributes  */
  status = exi_put_names(exoid, varid, num_attr, names, blk_type, "attribute", __func__);

  EX_FUNC_LEAVE(status);
}
