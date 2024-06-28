/*
 * Copyright(C) 1999-2020, 2023 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_NOERR, EX_WARN, etc

/*!
\internal
\ingroup ResultsData
\note This function is called internally by ex_get_var_multi_time() to handle
the reading of nodal variable values.

The function exi_get_nodal_var_multi_time() reads the values of a single nodal
variable for a one or more time steps. Memory must be allocated for the
nodal variable values array before this function is invoked.

Because nodal variables are floating point values, the application
code must declare the array passed to be the appropriate type
(float or double) to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, exi_get_nodal_var_multi_time() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
-  data file not properly opened with call to ex_create() or ex_open()
-  specified nodal variable does not exist.
-  a warning value is returned if no nodal variables are stored in the file.

\param[in] exoid                exodus file ID returned from a previous call to
ex_create()
or ex_open().

\param[in] nodal_var_index      The index of the desired nodal variable. The
                                first variable has an index of 1.

\param[in] num_nodes            The number of nodal points.

\param[in] beg_time_step        The first time step to get values for.
\param[in] end_time_step        The last time step to get values for.
\param[out]  nodal_var_vals     Returned array of num_nodes values of the
                                nodal_var_index-th nodal variable for the
                                desired timesteps.

For example, the following demonstrates how this function would be
used:

~~~{.c}
int num_nodes, time_step, var_index;
float *var_values;

\comment{read the second nodal variable at the first time step}
time_step = 1;
var_index = 2;

var_values = (float *) calloc (num_nodes, sizeof(float));
error = exi_get_nodal_var_multi_time(exoid, var_index, num_nodes,
                                     time_step, time_step, var_values);
~~~

*/

int exi_get_nodal_var_multi_time(int exoid, int nodal_var_index, int64_t num_nodes,
                                 int beg_time_step, int end_time_step, void *nodal_var_vals)
{
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* inquire previously defined variable */
  /* Need to see how this works in the parallel-aware exodus... */
  if (num_nodes == 0) {
    return EX_NOERR;
  }

  size_t start[3];
  size_t count[3];
  int    status;
  int    varid;
  if (ex_large_model(exoid) == 0) {
    /* read values of the nodal variable */
    if ((status = nc_inq_varid(exoid, VAR_NOD_VAR, &varid)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "Warning: could not find nodal variables in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_WARN;
    }

    start[0] = --beg_time_step;
    start[1] = --nodal_var_index;
    start[2] = 0;

    count[0] = end_time_step - beg_time_step;
    count[1] = 1;
    count[2] = num_nodes;
  }
  else {
    /* read values of the nodal variable  -- stored as separate variables... */
    /* Get the varid.... */
    if ((status = nc_inq_varid(exoid, VAR_NOD_VAR_NEW(nodal_var_index), &varid)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "Warning: could not find nodal variable %d in file id %d",
               nodal_var_index, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_WARN;
    }

    start[0] = --beg_time_step;
    start[1] = 0;

    count[0] = end_time_step - beg_time_step;
    count[1] = num_nodes;
  }

  if (exi_comp_ws(exoid) == 4) {
    status = nc_get_vara_float(exoid, varid, start, count, nodal_var_vals);
  }
  else {
    status = nc_get_vara_double(exoid, varid, start, count, nodal_var_vals);
  }

  if (status != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get nodal variables in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    return EX_FATAL;
  }
  return EX_NOERR;
}
