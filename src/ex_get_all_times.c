/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, VAR_WHOLE_TIME, etc

/*!
\ingroup ResultsData

The function ex_get_all_times() reads the time values for all time
steps. Memory must be allocated for the time values array before this
function is invoked. The storage requirements (equal to the number of
time steps) can be determined by using the ex_inquire() or
ex_inquire_int() routines.

Because time values are floating point values, the application code
must declare the array passed to be the appropriate type (float or
double) to match the compute word size passed in ex_create() or
ex_open().

\return In case of an error, ex_get_all_times() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  no time steps have been stored in the file.

\param[in]   exoid        exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[out]  time_values  Returned array of times. These are the time values at
all time steps.

The following code segment will read the time values for all time
steps stored in the data file:

~~~{.c}
int error, exoid, num_time_steps;
float *time_values;

\comment{determine how many time steps are stored}
num_time_steps = ex_inquire_int(exoid, EX_INQ_TIME);

\comment{read time values at all time steps}
time_values = (float *) calloc(num_time_steps, sizeof(float));

error = ex_get_all_times(exoid, time_values);
~~~

*/

int ex_get_all_times(int exoid, void *time_values)
{
  int  varid;
  int  status;
  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_varid(exoid, VAR_WHOLE_TIME, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate time variable %s in file id %d",
             VAR_WHOLE_TIME, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /*read time values */
  if (exi_comp_ws(exoid) == 4) {
    status = nc_get_var_float(exoid, varid, time_values);
  }
  else {
    status = nc_get_var_double(exoid, varid, time_values);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get time values from file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}
