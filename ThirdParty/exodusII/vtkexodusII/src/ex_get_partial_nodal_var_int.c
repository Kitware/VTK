/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgnnv - ex_get_partial_nodal_var
 *
 * environment - UNIX
 *
 * entry conditions -
 *   input parameters:
 *	int	exoid			exodus file id
 *	int	time_step		whole time step number
 *	int	nodeal_var_index	index of desired nodal variable
 *       int     start_node		starting location for read
 *	int	num_nodes		number of nodal points
 *
 * exit conditions -
 *	float*	var_vals		array of nodal variable values
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include <exodusII.h>     // for ex_err, etc
#include <exodusII_int.h> // for EX_WARN, exi_comp_ws, etc

/*!
  \internal
  \ingroup ResultsData
  \note This function is called internally by ex_get_partial_var() to handle
  the reading of nodal variable values.
 */

int exi_get_partial_nodal_var(int exoid, int time_step, int nodal_var_index, int64_t start_node,
                              int64_t num_nodes, void *var_vals)
{
  int    varid;
  int    status;
  size_t start[3], count[3];
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_large_model(exoid) == 0) {
    /* read values of the nodal variable */
    if ((status = nc_inq_varid(exoid, VAR_NOD_VAR, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "Warning: could not find nodal variables in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_WARN);
    }

    start[0] = --time_step;
    start[1] = --nodal_var_index;
    start[2] = --start_node;

    count[0] = 1;
    count[1] = 1;
    count[2] = num_nodes;
  }
  else {
    /* read values of the nodal variable  -- stored as separate variables... */
    /* Get the varid.... */
    if ((status = nc_inq_varid(exoid, VAR_NOD_VAR_NEW(nodal_var_index), &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "Warning: could not find nodal variable %d in file id %d",
               nodal_var_index, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_WARN);
    }

    start[0] = --time_step;
    start[1] = --start_node;

    count[0] = 1;
    count[1] = num_nodes;
    if (count[1] == 0) {
      start[1] = 0;
    }
  }

  if (exi_comp_ws(exoid) == 4) {
    status = nc_get_vara_float(exoid, varid, start, count, var_vals);
  }
  else {
    status = nc_get_vara_double(exoid, varid, start, count, var_vals);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get nodal variables in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
