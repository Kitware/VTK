/*
 * Copyright(C) 1999-2023 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*!
\ingroup ResultsData
writes the values of a single variable of the specified type for a
single time step. The function ex_put_variable_param() must be invoked
before this call is made.

Because variables are floating point values, the application
code must declare the array passed to be the appropriate type
(float or double) to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, ex_put_var() returns a negative number; a
warning will return a positive number. Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  ex_put_variable_param() not called previously specifying the number of
variables.

\param[in] exoid
exodus file ID returned from a previous call to
ex_create() or ex_open().

\param[in] time_step
The time step number, as described under
ex_put_time(). This is essentially a counter that is incremented when
results variables are output. The first time step is 1.

\param[in] var_type
type (edge block, face block, edge set, ... )

\param[in] var_index
The index of the variable. The first variable has an index of 1.

\param[in]  obj_id
entity block/set id (ignored for global and nodal variables)

\param[in] num_entries_this_obj
The number of items in this block/set

\param[in]  var_vals
Array of `num_entries_this_obj` values of the `var_index`-th variable for the `time_step`-th time
step.

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
   error = ex_put_var(exoid, time_step, EX_NODAL, 0, k, num_nodes,
                            nodal_var_vals);
}
~~~
 */

int ex_put_var(int exoid, int time_step, ex_entity_type var_type, int var_index,
               ex_entity_id obj_id, int64_t num_entries_this_obj, const void *var_vals)
{
  return ex_put_var_multi_time(exoid, var_type, var_index, obj_id, num_entries_this_obj, time_step,
                               time_step, var_vals);
}
