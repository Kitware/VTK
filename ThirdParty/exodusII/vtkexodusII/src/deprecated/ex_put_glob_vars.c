/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expgv - ex_put_glo_vars
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     time_step               time step number
 *       int     num_glob_vars           number of global vars in file
 *       float*  glob_var_vals           array of global variable values
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_var, etc

/*!
\deprecated Use ex_put_var()(exoid, time_step, EX_GLOBAL, 1, 0, num_glob_vars,
glob_var_vals)

The function ex_put_glob_vars() writes the values of all the global
variables for a single time step. The function ex_put_variable_param()
must be invoked before this call is made.

Because global variables are floating point values, the application
code must declare the array passed to be the appropriate type
(float or double) to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, ex_put_glob_vars() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  ex_put_variable_param() not called previously specifying
     the number of global variables.

\param[in] exoid           exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in] time_step       The time step number, as described under
ex_put_time().
                           This is essentially a counter that is incremented
when results
                           variables are output. The first time step is 1.
\param[in] num_glob_vars   The number of global variables to be written to the
database.
\param[in]  glob_var_vals  Array of num_glob_vars global variable values for
                           the time_step-th time step.

As an example, the following coding will write the values of all the
global variables at one time step to an open exodus II file:

~~~{.c}
int num_glo_vars, error, exoid, time_step;

float *glob_var_vals

\comment{write global variables}
for (j=0; j < num_glo_vars; j++) {
   \comment{application code fills this array}
   glob_var_vals[j] = 10.0;
}
error = ex_put_glob_vars (exoid, time_step, num_glo_vars, glob_var_vals);

\comment{Using non-deprecated functions:}
error = ex_put_var (exoid, time_step, EX_GLOBAL, 1, 0, num_glo_vars,
glob_var_vals);

~~~

*/

int ex_put_glob_vars(int exoid, int time_step, int num_glob_vars, const void *glob_var_vals)
{
  return ex_put_var(exoid, time_step, EX_GLOBAL, 1, 0 /*N/A*/, num_glob_vars, glob_var_vals);
}
