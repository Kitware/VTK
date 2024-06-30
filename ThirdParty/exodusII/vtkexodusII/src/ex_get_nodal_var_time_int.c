/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_WARN, exi_comp_ws, etc

/*!
The function ex_get_nodal_var_time() reads the values of a nodal
variable for a single node through a specified number of time
steps. Memory must be allocated for the nodal variable values array
before this function is invoked.

Because nodal variables are floating point values, the application
code must declare the array passed to be the appropriate type
(float or ``double'') to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, ex_get_nodal_var_time() returns a
negative number; a warning will return a positive number. Possible
causes of errors include:
  -  specified nodal variable does not exist.
  -  a warning value is returned if no nodal variables are stored in the file.

\param[in] exoid             exodus file ID returned from a previous call to
ex_create() or
                             ex_open().

\param[in] nodal_var_index   The index of the desired nodal variable. The first
variable has an
                             index of 1.

\param[in] node_number       The internal ID (see  Section LocalNodeIds) of the
desired
                             node. The first node is 1.

\param[in] beg_time_step     The beginning time step for which a nodal variable
value
                             is desired. This is not a time value but rather a
time step number,
                             as described under ex_put_time(). The first time
step is 1.

\param[in] end_time_step     The last time step for which a nodal variable value
is desired. If
                             negative, the last time step in the database will
be used. The first
                             time step is 1.

\param[out]  nodal_var_vals  Returned array of(end_time_step {-}
beg_time_step +1) values
                             of the node_number-th node for the
nodal_var_index-th nodal
                             variable.

For example, the following code segment will read the values of the
first nodal variable for node number one for all time steps stored in
the data file:

~~~{.c}
int num_time_steps, var_index, node_num, beg_time, end_time, error, exoid;
float *var_values;

\comment{determine how many time steps are stored}
num_time_steps = ex_inquire_int(exoid, EX_INQ_TIME);

\comment{read a nodal variable through time}
var_values = (float *) calloc (num_time_steps, sizeof(float));

var_index = 1; node_num = 1; beg_time = 1; end_time = -1;
error = ex_get_var_time(exoid, EX_NODAL, var_index, node_num, beg_time,
                              end_time, var_values);

~~~
*/

int exi_get_nodal_var_time(int exoid, int nodal_var_index, int64_t node_number, int beg_time_step,
                           int end_time_step, void *nodal_var_vals)
{
  int    status;
  int    varid;
  size_t start[3], count[3];
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

  beg_time_step--;
  end_time_step--;
  node_number--;

  if (ex_large_model(exoid) == 0) {
    /* read values of the nodal variable;
     * assume node number is 1-based (first node is numbered 1);  adjust
     * so it is 0-based
     */
    if ((status = nc_inq_varid(exoid, VAR_NOD_VAR, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "Warning: could not find nodal variable %d in file id %d",
               nodal_var_index, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_WARN);
    }

    start[0] = beg_time_step;
    start[1] = --nodal_var_index;
    start[2] = node_number;

    count[0] = end_time_step - beg_time_step + 1;
    count[1] = 1;
    count[2] = 1;
  }
  else {
    if ((status = nc_inq_varid(exoid, VAR_NOD_VAR_NEW(nodal_var_index), &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "Warning: could not find nodal variable %d in file id %d",
               nodal_var_index, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_WARN);
    }

    /* read values of the nodal variable;
     * assume node number is 1-based (first node is numbered 1);  adjust
     * so it is 0-based
     */

    start[0] = beg_time_step;
    start[1] = node_number;

    count[0] = end_time_step - beg_time_step + 1;
    count[1] = 1;
  }

  if (exi_comp_ws(exoid) == 4) {
    status = nc_get_vara_float(exoid, varid, start, count, nodal_var_vals);
  }
  else {
    status = nc_get_vara_double(exoid, varid, start, count, nodal_var_vals);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get nodal variables in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
