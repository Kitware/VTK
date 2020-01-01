/*
 * Copyright (c) 2005-2017 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of NTESS nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_WARN, ex__comp_ws, etc

/*!
\deprecated Use ex_put_var()(exoid, time_step, EX_NODAL, nodal_var_index, 1, num_nodes,
nodal_var_vals)

The function ex_put_nodal_var() writes the values of a single nodal
variable for a single time step. The function ex_put_variable_param()
must be invoked before this call is made.

Because nodal variables are floating point values, the application
code must declare the array passed to be the appropriate type
(float or double) to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, ex_put_nodal_var() returns a negative number; a
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

\param[in] time_step          The time step number, as described under
ex_put_time(). This
                              is essentially a counter that is incremented when
results variables
                              are output. The first time step is 1.

\param[in] nodal_var_index    The index of the nodal variable. The first
variable has an index of 1.

\param[in] num_nodes          The number of nodal points.

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
   error = ex_put_nodal_var(exoid, time_step, k, num_nodes,
                            nodal_var_vals);
}
~~~

*/

int ex_put_nodal_var(int exoid, int time_step, int nodal_var_index, int64_t num_nodes,
                     const void *nodal_var_vals)

{
  return ex_put_var(exoid, time_step, EX_NODAL, nodal_var_index, 1, num_nodes, nodal_var_vals);
}
