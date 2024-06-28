/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, exi_comp_ws, etc

/*!
\ingroup ResultsData

The function ex_put_time() writes the time value for a specified time
step.

Because time values are floating point values, the application code
must declare the array passed to be the appropriate type (float or
double) to match the compute word size passed in ex_create() or
ex_open().

\return In case of an error, ex_put_time() returns a negative number;
a warning will return a positive number. Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.

\param[in]  exoid         exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in]  time_step     The time step number. This is essentially a counter
that is
                          incremented only when results variables are output to
the data
                          file. The first time step is 1.
\param[in]  time_value    The time at the specified time step.

The following code segment will write out the simulation time value at
simulation time step n:

~~~{.c}
int error, exoid, n;
float time_value;

\comment{write time value}
error = ex_put_time (exoid, n, &time_value);
~~~

*/

int ex_put_time(int exoid, int time_step, const void *time_value)
{
  int                   status;
  int                   varid;
  size_t                start[1];
  char                  errmsg[MAX_ERR_LENGTH];
  struct exi_file_item *file = NULL;

  EX_FUNC_ENTER();

  file = exi_find_file_item(exoid);
  if (!file) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unknown file id %d.", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADFILEID);
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

  /* store time value */
  start[0] = --time_step;

  if (exi_comp_ws(exoid) == 4) {
    status = nc_put_var1_float(exoid, varid, start, time_value);
  }
  else {
    status = nc_put_var1_double(exoid, varid, start, time_value);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store time value in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
