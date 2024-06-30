/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expoea - ex_put_one_attr
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     obj_type                object type (edge, face, elem block)
 *       int     obj_id                  object id (edge, face, elem block ID)
 *       int     attrib_index            index of attribute to write
 *       float*  attrib                  array of attributes
 *
 * exit conditions -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, exi_get_dimension, etc

/*!
 * writes the specified attribute for a block
 * \param      exoid                   exodus file id
 * \param      obj_type                object type (edge, face, elem block)
 * \param      obj_id                  object id (edge, face, elem block ID)
 * \param      attrib_index            index of attribute to write
 * \param      attrib                  array of attributes
 */

int ex_put_one_attr(int exoid, ex_entity_type obj_type, ex_entity_id obj_id, int attrib_index,
                    const void *attrib)
{
  int         status;
  int         attrid, obj_id_ndx, temp;
  size_t      num_entries_this_obj, num_attr;
  size_t      start[2], count[2];
  ptrdiff_t   stride[2];
  char        errmsg[MAX_ERR_LENGTH];
  const char *dnumobjent;
  const char *dnumobjatt;
  const char *vattrbname;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Determine index of obj_id in id array */
  if (obj_type != EX_NODAL) {
    obj_id_ndx = exi_id_lkup(exoid, obj_type, obj_id);
    if (obj_id_ndx <= 0) {
      ex_get_err(NULL, NULL, &status);
      if (status != 0) {
        if (status == EX_NULLENTITY) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "Warning: no attributes allowed for NULL %s %" PRId64 " in file id %d",
                   ex_name_of_object(obj_type), obj_id, exoid);
          ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);
          EX_FUNC_LEAVE(EX_WARN); /* no attributes for this element block */
        }
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: no %s id %" PRId64 " in id array in file id %d",
                 ex_name_of_object(obj_type), obj_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }

  switch (obj_type) {
  case EX_SIDE_SET:
    dnumobjent = DIM_NUM_SIDE_SS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_SS(obj_id_ndx);
    vattrbname = VAR_SSATTRIB(obj_id_ndx);
    break;
  case EX_NODE_SET:
    dnumobjent = DIM_NUM_NOD_NS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_NS(obj_id_ndx);
    vattrbname = VAR_NSATTRIB(obj_id_ndx);
    break;
  case EX_EDGE_SET:
    dnumobjent = DIM_NUM_EDGE_ES(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_ES(obj_id_ndx);
    vattrbname = VAR_ESATTRIB(obj_id_ndx);
    break;
  case EX_FACE_SET:
    dnumobjent = DIM_NUM_FACE_FS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_FS(obj_id_ndx);
    vattrbname = VAR_FSATTRIB(obj_id_ndx);
    break;
  case EX_ELEM_SET:
    dnumobjent = DIM_NUM_ELE_ELS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_ELS(obj_id_ndx);
    vattrbname = VAR_ELSATTRIB(obj_id_ndx);
    break;
  case EX_NODAL:
    dnumobjent = DIM_NUM_NODES;
    dnumobjatt = DIM_NUM_ATT_IN_NBLK;
    vattrbname = VAR_NATTRIB;
    break;
  case EX_EDGE_BLOCK:
    dnumobjent = DIM_NUM_ED_IN_EBLK(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_EBLK(obj_id_ndx);
    vattrbname = VAR_EATTRIB(obj_id_ndx);
    break;
  case EX_FACE_BLOCK:
    dnumobjent = DIM_NUM_FA_IN_FBLK(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_FBLK(obj_id_ndx);
    vattrbname = VAR_FATTRIB(obj_id_ndx);
    break;
  case EX_ELEM_BLOCK:
    dnumobjent = DIM_NUM_EL_IN_BLK(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_BLK(obj_id_ndx);
    vattrbname = VAR_ATTRIB(obj_id_ndx);
    break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Internal ERROR: unrecognized object type in switch: %d in file id %d", obj_type,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL); /* number of attributes not defined */
  }

  /* inquire id's of previously defined dimensions  */
  if (exi_get_dimension(exoid, dnumobjent, "entries", &num_entries_this_obj, &temp, __func__) !=
      NC_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (exi_get_dimension(exoid, dnumobjatt, "attributes", &num_attr, &temp, __func__) != NC_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (attrib_index < 1 || attrib_index > (int)num_attr) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: Invalid attribute index specified: %d.  Valid "
             "range is 1 to %d for %s %" PRId64 " in file id %d",
             attrib_index, (int)num_attr, ex_name_of_object(obj_type), obj_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_varid(exoid, vattrbname, &attrid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate attribute variable for %s %" PRId64 " in file id %d",
             ex_name_of_object(obj_type), obj_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* write out the attributes  */

  start[0] = 0;
  start[1] = attrib_index - 1;

  count[0] = num_entries_this_obj;
  count[1] = 1;

  stride[0] = 1;
  stride[1] = num_attr;

  if (exi_comp_ws(exoid) == 4) {
    status = nc_put_vars_float(exoid, attrid, start, count, stride, attrib);
  }
  else {
    status = nc_put_vars_double(exoid, attrid, start, count, stride, attrib);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to put attribute %d for %s %" PRId64 " in file id %d", attrib_index,
             ex_name_of_object(obj_type), obj_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
