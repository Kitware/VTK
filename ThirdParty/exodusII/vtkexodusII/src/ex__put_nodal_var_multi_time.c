/*
 * Copyright(C) 1999-2020, 2023 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_WARN, exi_comp_ws, etc

/*!
\internal
\ingroup ResultsData
\note This function is called internally by ex_put_var_time() to handle
the writing of nodal variable values.

The function exi_put_nodal_var_time() writes the values of a single nodal
variable for multiple time steps. The function ex_put_variable_param()
must be invoked before this call is made.

Because nodal variables are floating point values, the application
code must declare the array passed to be the appropriate type
(float or double) to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, exi_put_nodal_var() returns a negative number; a
warning will return a positive number. Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  ex_put_variable_param() not called previously specifying the number of
nodal variables.

\param[in] exoid              exodus file ID returned from a previous call to
ex_create() or
                              ex_open().

\param[in] nodal_var_index    The index of the nodal variable. The first
variable has an index of 1.

\param[in] num_nodes          The number of nodal points.

\param[in] beg_time_step      The beginning time step number, as described under ex_put_time().
                              This is counter that is incremented when results variables
                              are output. The first time step is 1.

\param[in] end_time_step      The ending time step number, as described under ex_put_time().
                              This is counter that is incremented when results variables
                              are output. The first time step is 1.

\param[in]  nodal_var_vals    Array of num_nodes values of the
nodal_var_index-th nodal
                              variable for the time_step-th time step.

As an example, the following code segment writes all the nodal
variables for a single time step:

~~~{.c}
int num_nod_vars, num_nodes, error, exoid, time_step;
float *nodal_var_vals;

\comment{write nodal variables}
nodal_var_vals = (float *) calloc(num_nodes, sizeof(float));
for (k=1; k <= num_nod_vars; k++) {
   for (j=0; j < num_nodes; j++) {
      \comment{application code fills in this array}
      nodal_var_vals[j] = 10.0;
   }
   error = ex_put_var_time(exoid, EX_NODAL, k, 0, num_nodes, beg_time_step, end_time_step
                            nodal_var_vals);
}
~~~

*/

int exi_put_nodal_var_time(int exoid, int nodal_var_index, int64_t num_nodes, int beg_time_step,
                           int end_time_step, const void *nodal_var_vals)

{
  int    status;
  int    varid;
  size_t start[2], count[2];
  char   errmsg[MAX_ERR_LENGTH];

  if ((status = nc_inq_varid(exoid, VAR_NOD_VAR_NEW(nodal_var_index), &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "Warning: could not find nodal variable %d in file id %d",
             nodal_var_index, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    return (EX_WARN);
  }
  start[0] = --beg_time_step;
  start[1] = 0;

  count[0] = end_time_step - beg_time_step;
  count[1] = num_nodes;

  if (exi_comp_ws(exoid) == 4) {
    status = nc_put_vara_float(exoid, varid, start, count, nodal_var_vals);
  }
  else {
    status = nc_put_vara_double(exoid, varid, start, count, nodal_var_vals);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store nodal variables in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    return (EX_FATAL);
  }
  return (EX_NOERR);
}
