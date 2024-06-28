/*
 * Copyright(C) 1999-2020, 2023 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgvart - ex_get_var_time
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     var_type                variable type global, nodal,
 *                                         edge/face/elem block,
 *                                         node/edge/face/side/elem set
 *       int     var_index               element variable index
 *       int     id                      entry number
 *       int     beg_time_step           time step number
 *       int     end_time_step           time step number
 *
 * exit conditions -
 *       float*  var_vals                array of element variable values
 *
 * revision history -
 *   20061002 - David Thompson - Adapted from ex_get_var_time
 *
 *
 *****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*!
  \ingroup ResultsData

 * reads the values of a variable for a single entry of an object (block or set)
 * through a
 * specified number of time steps in the database; assume the first
 * variable index, entry number, and time step are 1
 */

int ex_get_var_time(int exoid, ex_entity_type var_type, int var_index, int64_t id,
                    int beg_time_step, int end_time_step, void *var_vals)
{
  int         dimid, varid;
  int         status;
  int        *stat_vals = NULL;
  size_t      numel     = 0;
  size_t      num_obj, i;
  size_t      num_entries_this_obj = 0;
  size_t      start[2], count[2];
  char        errmsg[MAX_ERR_LENGTH];
  const char *varobjids;
  const char *varobstat;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  switch (var_type) {
  case EX_GLOBAL:
    status = exi_get_glob_var_time(exoid, var_index, beg_time_step, end_time_step, var_vals);
    EX_FUNC_LEAVE(status);
  case EX_NODAL:
    status = exi_get_nodal_var_time(exoid, var_index, id, beg_time_step, end_time_step, var_vals);
    EX_FUNC_LEAVE(status);
  case EX_EDGE_BLOCK:
    varobjids = VAR_ID_ED_BLK;
    varobstat = VAR_STAT_ED_BLK;
    break;
  case EX_FACE_BLOCK:
    varobjids = VAR_ID_FA_BLK;
    varobstat = VAR_STAT_FA_BLK;
    break;
  case EX_ELEM_BLOCK:
    varobjids = VAR_ID_EL_BLK;
    varobstat = VAR_STAT_EL_BLK;
    break;
  case EX_NODE_SET:
    varobjids = VAR_NS_IDS;
    varobstat = VAR_NS_STAT;
    break;
  case EX_EDGE_SET:
    varobjids = VAR_ES_IDS;
    varobstat = VAR_ES_STAT;
    break;
  case EX_FACE_SET:
    varobjids = VAR_FS_IDS;
    varobstat = VAR_FS_STAT;
    break;
  case EX_SIDE_SET:
    varobjids = VAR_SS_IDS;
    varobstat = VAR_SS_STAT;
    break;
  case EX_ELEM_SET:
    varobjids = VAR_ELS_IDS;
    varobstat = VAR_ELS_STAT;
    break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Invalid variable type (%d) specified for file id %d",
             var_type, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* assume entry number is 1-based (the first entry of an object is 1, not 0);
   * adjust so it is 0-based
   */
  id--;

  /* find what object the entry is in */

  /* first, find out how many objects there are */
  status = exi_get_dimension(exoid, exi_dim_num_objects(var_type), ex_name_of_object(var_type),
                             &num_obj, &dimid, __func__);
  if (status != NC_NOERR) {
    EX_FUNC_LEAVE(status);
  }

  /* get the array of object ids */
  /* don't think we need this anymore since the netcdf variable names
     associated with objects don't contain the object ids */

  if ((status = nc_inq_varid(exoid, varobjids, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s ids in file id %d",
             ex_name_of_object(var_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* allocate space for stat array */
  if (!(stat_vals = calloc(num_obj, sizeof(int)))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate memory for %s status array for file id %d",
             ex_name_of_object(var_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* get variable id of status array */
  if (nc_inq_varid(exoid, varobstat, &varid) == NC_NOERR) {
    /* if status array exists, use it, otherwise assume, object exists
       to be backward compatible */

    if ((status = nc_get_var_int(exoid, varid, stat_vals)) != NC_NOERR) {
      free(stat_vals);
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get %s status array from file id %d",
               ex_name_of_object(var_type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  else { /* default: status is true */
    for (i = 0; i < num_obj; i++) {
      stat_vals[i] = 1;
    }
  }

  /* loop through each object until id is found;  since entry
   * numbers are sequential (beginning with 1) id is in obj_i
   * when id_first_i <= id <= id_last_i, where
   * id_first_i is the entry number of the first entry in
   * obj_i and id_last_i is the entry number of the last
   * entry in obj_i
   */

  i = 0;
  if (stat_vals[i] != 0) {
    if ((status = nc_inq_dimid(exoid, exi_dim_num_entries_in_object(var_type, i + 1), &dimid)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate number of entries in %zuth %s in file id %d", i,
               ex_name_of_object(var_type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      free(stat_vals);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = nc_inq_dimlen(exoid, dimid, &num_entries_this_obj)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get number of entries in %zuth %s in file id %d", i,
               ex_name_of_object(var_type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      free(stat_vals);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  } /* End NULL object check */

  numel = num_entries_this_obj;

  while (numel <= id) {
    if (stat_vals[++i] != 0) {
      if ((status = nc_inq_dimid(exoid, exi_dim_num_entries_in_object(var_type, i + 1), &dimid)) !=
          NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to locate number of entries in %zuth %s in file id %d", i,
                 ex_name_of_object(var_type), exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        free(stat_vals);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      if ((status = nc_inq_dimlen(exoid, dimid, &num_entries_this_obj)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to get number of entries in %zuth %s in file id %d", i,
                 ex_name_of_object(var_type), exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        free(stat_vals);
        EX_FUNC_LEAVE(EX_FATAL);
      }
      numel += num_entries_this_obj;
    }
  }
  size_t offset = id - (numel - num_entries_this_obj);

  /* inquire previously defined variable */
  if ((status = nc_inq_varid(exoid, exi_name_var_of_object(var_type, var_index, i + 1), &varid)) !=
      NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate variable %zu for %dth %s in file id %d", i, var_index,
             ex_name_of_object(var_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    free(stat_vals);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  free(stat_vals);

  /* Check that times are in range */
  {
    int num_time_steps = ex_inquire_int(exoid, EX_INQ_TIME);

    if (num_time_steps == 0) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: there are no time_steps on the file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (beg_time_step <= 0 || beg_time_step > num_time_steps) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: beginning time_step is out-of-range. Value = %d, "
               "valid range is 1 to %d in file id %d",
               beg_time_step, num_time_steps, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (end_time_step < 0) {
      /* user is requesting the maximum time step;  we find this out using the
       * database inquire function to get the number of time steps;  the ending
       * time step number is 1 less due to 0 based array indexing in C
       */
      end_time_step = ex_inquire_int(exoid, EX_INQ_TIME);
    }
    else if (end_time_step < beg_time_step || end_time_step > num_time_steps) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: end time_step is out-of-range. Value = %d, valid "
               "range is %d to %d in file id %d",
               beg_time_step, end_time_step, num_time_steps, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* read values of object variable */
  start[0] = --beg_time_step;
  start[1] = offset;

  end_time_step--;

  count[0] = end_time_step - beg_time_step + 1;
  count[1] = 1;

  if (exi_comp_ws(exoid) == 4) {
    status = nc_get_vara_float(exoid, varid, start, count, var_vals);
  }
  else {
    status = nc_get_vara_double(exoid, varid, start, count, var_vals);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get %s variable values in file id %d",
             ex_name_of_object(var_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
