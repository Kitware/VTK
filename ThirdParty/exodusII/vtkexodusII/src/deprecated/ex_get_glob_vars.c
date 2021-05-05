/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exggv - ex_get_glob_vars
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     time_step               time step number
 *       int     num_glob_vars           number of global vars in file
 *
 * exit conditions -
 *       float*  glob_var_vals           array of global variable values
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_var, etc

/*!
\deprecated Use ex_get_var()(exoid, time_step, EX_GLOBAL, 1, 1, num_glob_vars, global_var_vals)
\ingroup ResultsData

The function ex_get_glob_vars() reads the values of all the
global variables for a single time step. Memory must be allocated for
the global variables values array before this function is invoked.

Because global variables are floating point values, the application
code must declare the array passed to be the appropriate type
(float or double) to match the compute word size passed in
ex_create() or ex_open().

In case of an error, ex_get_glob_vars() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:

 - data file not properly opened with call to ex_create() or ex_open()
 - no global variables stored in the file.
 - a warning value is returned if no global variables are stored in the file.

\param[in] exoid        exodus file ID returned from a previous call to
ex_create() or ex_open().

\param[in] time_step    The time step, as described under ex_put_time(), at
                        which the global variable values are desired. This is
essentially
                        an index (in the time dimension) into the global
variable values
                        array stored in the database. The first time step is 1.

\param[in]  num_glob_vars The number of global variables stored in the database.
\param[out] glob_var_vals Returned array of num_glob_vars global variable values
                          for the time_step'th time step.

The following is an example code segment that reads all the global
variables at one time step:

~~~{.c}
int num_glo_vars, error, time_step;
float *var_values;

error = ex_get_variable_param (idexo, EX_GLOBAL, &num_glo_vars);
var_values = (float *) calloc (num_glo_vars, sizeof(float));
error = ex_get_glob_vars (idexo, time_step, num_glo_vars,
                          var_values);
~~~

 */

int ex_get_glob_vars(int exoid, int time_step, int num_glob_vars, void *glob_var_vals)
{
  return ex_get_var(exoid, time_step, EX_GLOBAL, 1, 1, num_glob_vars, glob_var_vals);
}
