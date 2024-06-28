/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for exi_comp_ws, EX_FATAL, etc

/*!
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

int exi_get_glob_var_time(int exoid, int glob_var_index, int beg_time_step, int end_time_step,
                          void *glob_var_vals)
{
  int    status;
  int    varid;
  size_t start[2], count[2];
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Check that times are in range */
  {
    int num_time_steps = ex_inquire_int(exoid, EX_INQ_TIME);

    if (num_time_steps == 0) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: there are no time_steps on the file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (beg_time_step <= 0 || beg_time_step > num_time_steps) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: beginning time_step is out-of-range. Value = %d, "
               "valid range is 1 to %d in file id %d",
               beg_time_step, num_time_steps, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (end_time_step < 0) {
      /* user is requesting the maximum time step;  we find this out using the
       * database inquire function to get the number of time steps;  the ending
       * time step number is 1 less due to 0 based array indexing in C
       */
      end_time_step = num_time_steps;
    }
    else if (end_time_step < beg_time_step || end_time_step > num_time_steps) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: end time_step is out-of-range. Value = %d, valid "
               "range is %d to %d in file id %d",
               beg_time_step, end_time_step, num_time_steps, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  end_time_step--;

  /* read values of global variables */
  start[0] = --beg_time_step;
  start[1] = --glob_var_index;

  count[0] = end_time_step - beg_time_step + 1;
  count[1] = 1;

  /* inquire previously defined variable */
  if ((status = nc_inq_varid(exoid, VAR_GLO_VAR, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate global variables in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_WARN);
  }

  if (exi_comp_ws(exoid) == 4) {
    status = nc_get_vara_float(exoid, varid, start, count, glob_var_vals);
  }
  else {
    status = nc_get_vara_double(exoid, varid, start, count, glob_var_vals);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to get global variable %d values from file id %d", glob_var_index,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
