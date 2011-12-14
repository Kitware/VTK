/*
 * Copyright (c) 2005 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Governement
 * retains certain rights in this software.
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
 *     * Neither the name of Sandia Corporation nor the names of its
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

#include <string.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!

The function ex_put_time() writes the time value for a specified time
step.

Because time values are floating point values, the application code
must declare the array passed to be the appropriate type (\c float or
\c double) to match the compute word size passed in ex_create() or
ex_open().

\return In case of an error, ex_put_time() returns a negative number;
a warning will return a positive number. Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.

\param[in]  exoid         exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in]  time_step     The time step number. This is essentially a counter that is
                          incremented only when results variables are output to the data
        file. The first time step is 1.
\param[in]  time_value    The time at the specified time step.

The following code segment will write out the simulation time value at
simulation time step n:

\code
int error, exoid, n;
float time_value;

\comment{write time value}
error = ex_put_time (exoid, n, &time_value);
\endcode

*/

int ex_put_time (int   exoid,
                 int   time_step,
                 const void *time_value)
{
  int status;
  int varid; 
  size_t start[1];
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  /* inquire previously defined variable */
  if ((status = nc_inq_varid(exoid, VAR_WHOLE_TIME, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to locate time variable in file id %d", exoid);
    ex_err("ex_put_time",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* store time value */
  start[0] = --time_step;

  if (ex_comp_ws(exoid) == 4) {
    status = nc_put_var1_float(exoid, varid, start, time_value);
  } else {
    status = nc_put_var1_double(exoid, varid, start, time_value);
  }

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to store time value in file id %d", exoid);
    ex_err("ex_put_time",errmsg,exerrval);
    return (EX_FATAL);
  }


  return (EX_NOERR);
}
