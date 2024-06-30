/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expnv - ex_put_partial_nodal_var
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     time_step               whole time step number
 *       int     nodel_var_index         index of desired nodal variable
 *       int     start_node              index (1-based) of first node to put
 *       int     num_nodes               number of nodal points
 *       float*  nodal_var_vals          array of nodal variable values
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_WARN, exi_comp_ws, etc

/*!
  \internal
  \ingroup ResultsData
  \note This function is called internally by ex_put_partial_var() to handle
  the reading of nodal variable values.

 * writes the values of a single nodal variable for a partial block at
 * one single time step to the database; assume the first time step
 * and nodal variable index is 1
 * \param exoid            exodus file id
 * \param time_step        whole time step number
 * \param nodal_var_index  index of desired nodal variable
 * \param start_node       index of first node to write data for (1-based)
 * \param num_nodes        number of nodal points
 * \param nodal_var_vals   array of nodal variable values
 */

int exi_put_partial_nodal_var(int exoid, int time_step, int nodal_var_index, int64_t start_node,
                              int64_t num_nodes, const void *nodal_var_vals)

{
  int    status;
  int    varid;
  size_t start[3], count[3];
  char   errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

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
  if (num_nodes == 0) {
    start[1] = 0;
  }

  if (exi_comp_ws(exoid) == 4) {
    status = nc_put_vara_float(exoid, varid, start, count, nodal_var_vals);
  }
  else {
    status = nc_put_vara_double(exoid, varid, start, count, nodal_var_vals);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store nodal variables in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}
