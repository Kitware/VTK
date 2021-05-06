/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, ex__comp_ws, etc

/*!
\ingroup ResultsData
The function ex_get_time() reads the time value for a specified time
step.

Because time values are floating point values, the application code
must declare the array passed to be the appropriate type (float or
double) to match the compute word size passed in ex_create() or
ex_open().

\return In case of an error, ex_get_time() returns a negative number; a
warning will return a positive number. Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  no time steps have been stored in the file.

\param[in]  exoid       exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in]  time_step   The time step number. This is essentially an index (in
the time
                        dimension) into the global, nodal, and element variables
arrays stored
                        in the database. The first time step is 1.
\param[out] time_value  Returned time at the specified time step.

As an example, the following coding will read the time value stored in
the data file for time step n:

~~~{.c}
int n, error, exoid;
float time_value;

\comment{read time value at time step 3}
n = 3;
error = ex_get_time (exoid, n, &time_value);
~~~

*/

int ex_get_time(int exoid, int time_step, void *time_value)
{
  int                   status;
  int                   varid;
  size_t                start[1];
  char                  errmsg[MAX_ERR_LENGTH];
  struct ex__file_item *file = NULL;

  EX_FUNC_ENTER();

  file = ex__find_file_item(exoid);
  if (!file) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unknown file id %d.", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADFILEID);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  int num_time_steps = ex_inquire_int(exoid, EX_INQ_TIME);

  if (num_time_steps == 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: there are no time_steps on the file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (time_step <= 0) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: time_step must be greater than 0.  Entered value is %d in file id %d",
             time_step, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (time_step > num_time_steps) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: beginning time_step is out-of-range. Value = %d, "
             "valid range is 1 to %d in file id %d",
             time_step, num_time_steps, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  varid = file->time_varid;
  if (varid < 0) {
    /* inquire previously defined variable */
    if ((status = nc_inq_varid(exoid, VAR_WHOLE_TIME, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate time variable in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    file->time_varid = varid;
  }

  /* read time value */
  start[0] = --time_step;

  if (ex__comp_ws(exoid) == 4) {
    status = nc_get_var1_float(exoid, varid, start, time_value);
  }
  else {
    status = nc_get_var1_double(exoid, varid, start, time_value);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get time value in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
