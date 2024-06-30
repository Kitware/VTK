/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, ex_name_of_object, etc
#include "exodusII_int.h" // for exi_check_valid_file_id, etc

/*!
\ingroup ResultsData

The function ex_get_reduction_vars() reads the values of the selected
entity reduction variables for a single time step. Memory must be
allocated for the variables values array before this function is
invoked.

Because variables are floating point values, the application
code must declare the array passed to be the appropriate type
(float or double) to match the compute word size passed in
ex_create() or ex_open().

In case of an error, ex_get_reduction_vars() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:

 - data file not properly opened with call to ex_create() or ex_open()
 - variable does not exist for the desired block or set.
 - invalid block or set.
 - no variables of the selected type stored in the file.
 - a warning value is returned if no variables of the selected entity type are stored in the file.

\param[in] exoid        exodus file ID returned from a previous call to
ex_create() or ex_open().

\param[in] time_step    The time step, as described under ex_put_time(), at
                        which the variable values are desired. This is
                        essentially an index (in the time dimension) into the entity
                        variable values array stored in the database. The first time step is 1.

\param[in]  var_type    block/variable type node, edge/face/element block, or
                        node/edge/face/side/element set of type ex_entity_type.
\param[in]  obj_id      object id, see ex_get_ids()
\param[in]  num_variables The number of reduction variables in this object stored in the database.
\param[out] var_vals  Returned array of num_variables variable values
                          for the time_step'th time step.

The following is an example code segment that reads the element reduction variables for element
block with id 100 at time step 5.  There are 'num_variables' reduction variables for element block
100.

~~~{.c}
int num_variables, error, time_step;
int blk_id = 100;
float *var_values;

var_values = (float *) callo(num_variables, sizeof(float));
error = ex_get_reduction_vars (idexo, time_step, EX_ELEM_BLOCK, blk_id,
                              num_variables, var_values);
~~~

 */

int ex_get_reduction_vars(int exoid, int time_step, ex_entity_type var_type, ex_entity_id obj_id,
                          int64_t num_variables, void *var_vals)
{
  int    status;
  int    varid, obj_id_ndx;
  size_t start[2], count[2];
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (var_type == EX_GLOBAL) {
    /* FIXME: Special case: all vars stored in 2-D single array. */
    status = exi_get_glob_vars(exoid, time_step, num_variables, var_vals);
    EX_FUNC_LEAVE(status);
  }

  /* Determine index of obj_id in VAR_ID_XXX array */
  obj_id_ndx = exi_id_lkup(exoid, var_type, obj_id);
  if (obj_id_ndx <= 0) {
    ex_get_err(NULL, NULL, &status);

    if (status != 0) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: could not find %s %" PRId64 " in file id %d",
               ex_name_of_object(var_type), obj_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* inquire previously defined variable */
  if ((status = nc_inq_varid(exoid, exi_name_red_var_of_object(var_type, obj_id_ndx), &varid)) !=
      NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Warning: no reduction variables for %s %" PRId64 " in file id %d",
             ex_name_of_object(var_type), obj_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_WARN);
  }

  /* read values of reduction variables */
  start[0] = --time_step;
  start[1] = 0;

  count[0] = 1;
  count[1] = num_variables;

  if (exi_comp_ws(exoid) == 4) {
    status = nc_get_vara_float(exoid, varid, start, count, var_vals);
  }
  else {
    status = nc_get_vara_double(exoid, varid, start, count, var_vals);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to get reduction variable values for %s %" PRId64 " in file id %d",
             ex_name_of_object(var_type), obj_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
