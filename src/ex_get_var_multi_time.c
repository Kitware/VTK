/*
 * Copyright(C) 1999-2020, 2023 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, ex_name_of_object, etc
#include "exodusII_int.h" // for exi_check_valid_file_id, etc

/*!
\ingroup ResultsData

The function ex_get_var() reads the values of the
selected entity variables for a single time step. Memory must be allocated for
the variables values array before this function is invoked.

Because variables are floating point values, the application
code must declare the array passed to be the appropriate type
(float or double) to match the compute word size passed in
ex_create() or ex_open().

In case of an error, ex_get_var() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:

 - data file not properly opened with call to ex_create() or ex_open()
 - variable does not exist for the desired block or set.
 - invalid block or set.
 - no variables of the selected type stored in the file.
 - a warning value is returned if no variables of the selected entity type are stored in the file.

\param[in] exoid        exodus file ID returned from a previous call to
ex_create() or ex_open().

\param[in] var_type    block/variable type node, edge/face/element block, or
                        node/edge/face/side/element set of type ex_entity_type.
\param[in] var_index   variable index; 1-based
\param[in] obj_id      object id, see ex_get_ids()
\param[in] num_entry_this_obj The number of entities in this object stored in the database.
\param[in] beg_time_step    The first time step to access variable data from.  1-based.
\param[in] end_time_step    The last time step to access variable data from.  1-based.

\param[out] var_vals  Returned array of num_entry_this_obj variable values
                          for the time_step'th time step.

The following is an example code segment that reads the 10th element
variable for element block with id 100 over all 10 time steps. There
are 'num_elements_this_block' elements in element block 100.

~~~{.c}
int num_elements_this_block, error, time_step;
int var_index = 10;
int blk_id = 100;
int num_time_step = 10;
int num_variables;
float *var_values;

var_values = (float *) calloc(num_elements_this_block * num_time_step, sizeof(float));
error = ex_get_var_multi_time (idexo, EX_ELEM_BLOCK, var_index, blk_id,
                               num_elements_this_block, 1, num_time_step, var_values);
~~~

 */

int ex_get_var_multi_time(int exoid, ex_entity_type var_type, int var_index, ex_entity_id obj_id,
                          int64_t num_entry_this_obj, int beg_time_step, int end_time_step,
                          void *var_vals)
{
  int    status;
  int    varid, obj_id_ndx;
  size_t start[2], count[2];
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (var_type == EX_NODAL) {
    /* FIXME: Special case: ignore obj_id, possible large_file complications,
     * etc. */
    status = exi_get_nodal_var_multi_time(exoid, var_index, num_entry_this_obj, beg_time_step,
                                          end_time_step, var_vals);
    EX_FUNC_LEAVE(status);
  }
  if (var_type == EX_GLOBAL) {
    /* FIXME: Special case: all vars stored in 2-D single array. */
    status = exi_get_glob_vars_multi_time(exoid, num_entry_this_obj, beg_time_step, end_time_step,
                                          var_vals);
    EX_FUNC_LEAVE(status);
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

  /* read values of element variable */
  start[0] = --beg_time_step;
  start[1] = 0;

  count[0] = end_time_step - beg_time_step;
  count[1] = num_entry_this_obj;

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
