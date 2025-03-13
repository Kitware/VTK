/*
 * Copyright(C) 1999-2020, 2023 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expmp - ex_put_attr_param
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
#include "exodusII_int.h" // for EX_FATAL, EX_WARN, etc

/*!
 * defines the number of attributes.
 * \param   exoid           exodus file id
 * \param   obj_type        block/set type (node, edge, face, elem)
 * \param   obj_id          block/set id (ignored for NODAL)
 * \param   num_attrs       number of attributes
 */

int ex_put_attr_param(int exoid, ex_entity_type obj_type, ex_entity_id obj_id, int num_attrs)
{
  int status;
  int dims[2];
  int strdim, varid;

  char        errmsg[MAX_ERR_LENGTH];
  const char *dnumobjent;
  const char *dnumobjatt;
  const char *vobjatt;
  const char *vattnam;
  int         numobjentdim;
  int         obj_id_ndx;
  int         numattrdim;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Determine index of obj_id in obj_type id array */
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
  case EX_SIDE_SET:
    dnumobjent = DIM_NUM_SIDE_SS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_SS(obj_id_ndx);
    vobjatt    = VAR_SSATTRIB(obj_id_ndx);
    vattnam    = VAR_NAME_SSATTRIB(obj_id_ndx);
    break;
  case EX_NODE_SET:
    dnumobjent = DIM_NUM_NOD_NS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_NS(obj_id_ndx);
    vobjatt    = VAR_NSATTRIB(obj_id_ndx);
    vattnam    = VAR_NAME_NSATTRIB(obj_id_ndx);
    break;
  case EX_EDGE_SET:
    dnumobjent = DIM_NUM_EDGE_ES(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_ES(obj_id_ndx);
    vobjatt    = VAR_ESATTRIB(obj_id_ndx);
    vattnam    = VAR_NAME_ESATTRIB(obj_id_ndx);
    break;
  case EX_FACE_SET:
    dnumobjent = DIM_NUM_FACE_FS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_FS(obj_id_ndx);
    vobjatt    = VAR_FSATTRIB(obj_id_ndx);
    vattnam    = VAR_NAME_FSATTRIB(obj_id_ndx);
    break;
  case EX_ELEM_SET:
    dnumobjent = DIM_NUM_ELE_ELS(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_ELS(obj_id_ndx);
    vobjatt    = VAR_ELSATTRIB(obj_id_ndx);
    vattnam    = VAR_NAME_ELSATTRIB(obj_id_ndx);
    break;
  case EX_NODAL:
    dnumobjent = DIM_NUM_NODES;
    dnumobjatt = DIM_NUM_ATT_IN_NBLK;
    vobjatt    = VAR_NATTRIB;
    vattnam    = VAR_NAME_NATTRIB;
    break;
  case EX_EDGE_BLOCK:
    dnumobjent = DIM_NUM_ED_IN_EBLK(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_EBLK(obj_id_ndx);
    vobjatt    = VAR_EATTRIB(obj_id_ndx);
    vattnam    = VAR_NAME_EATTRIB(obj_id_ndx);
    break;
  case EX_FACE_BLOCK:
    dnumobjent = DIM_NUM_FA_IN_FBLK(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_FBLK(obj_id_ndx);
    vobjatt    = VAR_FATTRIB(obj_id_ndx);
    vattnam    = VAR_NAME_FATTRIB(obj_id_ndx);
    break;
  case EX_ELEM_BLOCK:
    dnumobjent = DIM_NUM_EL_IN_BLK(obj_id_ndx);
    dnumobjatt = DIM_NUM_ATT_IN_BLK(obj_id_ndx);
    vobjatt    = VAR_ATTRIB(obj_id_ndx);
    vattnam    = VAR_NAME_ATTRIB(obj_id_ndx);
    break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Bad block type (%d) specified for file id %d",
             obj_type, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_dimid(exoid, dnumobjent, &numobjentdim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate number of entries for %s %" PRId64 " in file id %d",
             ex_name_of_object(obj_type), obj_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* put netcdf file into define mode  */
  if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to place file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_def_dim(exoid, dnumobjatt, num_attrs, &numattrdim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to define number of attributes in %s %" PRId64 " in file id %d",
             ex_name_of_object(obj_type), obj_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret; /* exit define mode and return */
  }

  dims[0] = numobjentdim;
  dims[1] = numattrdim;

  if ((status = nc_def_var(exoid, vobjatt, nc_flt_code(exoid), 2, dims, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR:  failed to define attributes for %s %" PRId64 " in file id %d",
             ex_name_of_object(obj_type), obj_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret; /* exit define mode and return */
  }
  exi_compress_variable(exoid, varid, 2);

  /* inquire previously defined dimensions  */
  if ((status = nc_inq_dimid(exoid, DIM_STR_NAME, &strdim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get string length in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Attribute names... */
  dims[0] = numattrdim;
  dims[1] = strdim;

  if ((status = nc_def_var(exoid, vattnam, NC_CHAR, 2, dims, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to define %s attribute name array in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret; /* exit define mode and return */
  }
#if defined(EX_CAN_USE_NC_DEF_VAR_FILL)
  int fill = NC_FILL_CHAR;
  nc_def_var_fill(exoid, varid, 0, &fill);
#endif

  /* leave define mode  */
  if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  exi_leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}
