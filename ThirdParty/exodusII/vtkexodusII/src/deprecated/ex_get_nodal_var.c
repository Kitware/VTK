/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h" // for ex_get_var, etc

/*!
\ingroup ResultsData
\deprecated Use ex_get_var()(exoid, time_step, EX_NODAL, nodal_var_index, 1, num_nodes,
nodal_var_vals)
The function ex_get_nodal_var() reads the values of a single nodal
variable for a single time step. Memory must be allocated for the
nodal variable values array before this function is invoked.

Because nodal variables are floating point values, the application
code must declare the array passed to be the appropriate type
(float or double) to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, ex_get_nodal_var() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  specified nodal variable does not exist.
  -  a warning value is returned if no nodal variables are stored in the file.

\param[in] exoid                exodus file ID returned from a previous call to
ex_create()
                                or ex_open().

\param[in] time_step            The time step, as described under ex_put_time(),
at which the
                                nodal variable values are desired. This is
essentially an index (in
                                the time dimension) into the nodal variable
values array stored in
                                the database. The first time step is 1.

\param[in] nodal_var_index      The index of the desired nodal variable. The
first variable
                                has an index of 1.

\param[in] num_nodes            The number of nodal points.

\param[out]  nodal_var_vals     Returned array of num_nodes values of the
nodal_var_index-th
                                nodal variable for the time_step-th time
step.

For example, the following demonstrates how this function would be
used:

~~~{.c}
int num_nodes, time_step, var_index;
float *var_values;

\comment{read the second nodal variable at the first time step}
time_step = 1;
var_index = 2;

var_values = (float *) calloc (num_nodes, sizeof(float));
error = ex_get_nodal_var(exoid, time_step, var_index, num_nodes,
                         var_values);
~~~

*/

int ex_get_nodal_var(int exoid, int time_step, int nodal_var_index, int64_t num_nodes,
                     void *nodal_var_vals)
{
  return ex_get_var(exoid, time_step, EX_NODAL, nodal_var_index, 1, num_nodes, nodal_var_vals);
}
