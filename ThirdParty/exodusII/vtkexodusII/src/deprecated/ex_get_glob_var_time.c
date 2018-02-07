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

#include "exodusII.h" // for ex_get_var_time, etc

/*!
\deprecated Use ex_get_var_time()(exoid, EX_GLOBAL, global_var_index,
1, beg_time_step, end_time_step, glob_var_vals)

 The function ex_get_glob_var_time() reads the values of a
 single global variable through a specified number of time
 steps. Memory must be allocated for the global variable values array
 before this function is invoked.

 Because global variables are floating point values, the application
 code must declare the array passed to be the appropriate type
 (float or double) to match the compute word size passed in
 ex_create() or ex_open().

\return In case of an error, ex_get_glob_var_time() returns a
 negative number; a warning will return a positive number. Possible
 causes of errors include:
  - data file not properly opened with call to ex_create() or ex_open()
  - specified global variable does not exist.
  - a warning value is returned if no global variables are stored in the file.

 \param exoid           exodus file ID returned from a previous call to
ex_create() or ex_open().

 \param glob_var_index  The index of the desired global variable. The first
variable has an index of 1.

 \param beg_time_step   The beginning time step for which a global variable
value is desired.
                        This is not a time value but rather a time step number,
as
                        described under ex_put_time(). The first time step is 1.

 \param end_time_step   The last time step for which a global variable value is
desired. If
                        negative, the last time step in the database will be
used. The first
                        time step is 1.

 \param[out] glob_var_vals Returned array of (end_time_step - beg_time_step + 1)
values
                           for the glob_var_index$^{th}$ global variable.

The following is an example of using this function:

~~~{.c}
int error, exoid, num_time_steps, var_index;
int beg_time, end_time;

float *var_values;

num_time_steps = ex_inquire_int(exoid, EX_INQ_TIME);

var_index = 1;
beg_time = 1;
end_time = -1;

var_values = (float *) calloc (num_time_steps, sizeof(float));

error = ex_get_glob_var_time(exoid, var_index, beg_time,
                             end_time, var_values);
~~~

*/

int ex_get_glob_var_time(int exoid, int glob_var_index, int beg_time_step, int end_time_step,
                         void *glob_var_vals)
{
  return ex_get_var_time(exoid, EX_GLOBAL, glob_var_index, 1, beg_time_step, end_time_step,
                         glob_var_vals);
}
