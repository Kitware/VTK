/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, ex_name_of_object, etc
#include "exodusII_int.h" // for exi_check_valid_file_id, etc

/*
 * reads the values of a single element variable for one element block at
 * one time step in the database; assume the first time step and
 * element variable index is 1
 */

/*!
\ingroup ResultsData

 * reads the values of a single variable for a partial block at one time
 * step from the database; assume the first time step and variable index
 * and start_index are 1
 * \param      exoid           exodus file id
 * \param      time_step       time step number
 * \param      var_type        type (edge block, face block, edge set, ... )
 * \param      var_index       element variable index
 * \param      obj_id          element block id
 * \param      start_index     index of first entity in block to read (1-based)
 * \param      num_entities    number of entries to read in this block/set
 * \param      var_vals        the values to read
 */

int ex_get_partial_var(int exoid, int time_step, ex_entity_type var_type, int var_index,
                       ex_entity_id obj_id, int64_t start_index, int64_t num_entities,
                       void *var_vals)
{
  int    status = 0;
  int    varid, obj_id_ndx;
  size_t start[2], count[2];
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();

#if !defined(PARALLEL_AWARE_EXODUS)
  if (num_entities == 0) {
    EX_FUNC_LEAVE(status);
  }
#endif

  if (var_type == EX_NODAL) {
    /* FIXME: Special case: ignore obj_id, possible large_file complications,
     * etc. */
    status =
        exi_get_partial_nodal_var(exoid, time_step, var_index, start_index, num_entities, var_vals);
    EX_FUNC_LEAVE(status);
  }
  if (var_type == EX_GLOBAL) {
    /* FIXME: Special case: all vars stored in 2-D single array. */
    status = exi_get_glob_vars(exoid, time_step, num_entities, var_vals);
    EX_FUNC_LEAVE(status);
  }

  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Determine index of obj_id in VAR_ID_EL_BLK array */
  obj_id_ndx = exi_id_lkup(exoid, var_type, obj_id);
  if (obj_id_ndx <= 0) {
    ex_get_err(NULL, NULL, &status);

    if (status != 0) {
      if (status == EX_NULLENTITY) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "Warning: no %s variables for NULL block %" PRId64 " in file id %d",
                 ex_name_of_object(var_type), obj_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);
        EX_FUNC_LEAVE(EX_WARN);
      }
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s id %" PRId64 " in id variable in file id %d",
               ex_name_of_object(var_type), obj_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* inquire previously defined variable */

  if ((status = nc_inq_varid(exoid, exi_name_var_of_object(var_type, var_index, obj_id_ndx),
                             &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s %" PRId64 " var %d in file id %d",
             ex_name_of_object(var_type), obj_id, var_index, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* read values of variable */
  start[0] = --time_step;
  start[1] = start_index - 1;

  count[0] = 1;
  count[1] = num_entities;
  if (count[1] == 0) {
    start[1] = 0;
  }

  if (exi_comp_ws(exoid) == 4) {
    status = nc_get_vara_float(exoid, varid, start, count, var_vals);
  }
  else {
    status = nc_get_vara_double(exoid, varid, start, count, var_vals);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to get %s %" PRId64 " variable %d in file id %d",
             ex_name_of_object(var_type), obj_id, var_index, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
