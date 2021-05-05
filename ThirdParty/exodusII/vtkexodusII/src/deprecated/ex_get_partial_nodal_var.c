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
 *      int     exoid                   exodus file id
 *      int     time_step               whole time step number
 *      int     nodeal_var_index        index of desired nodal variable
 *       int     start_node             starting location for read
 *      int     num_nodes               number of nodal points
 *
 * exit conditions -
 *      float*  var_vals                array of nodal variable values
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include <exodusII.h> // for ex_get_partial_var, etc

/*!
\ingroup ResultsData

 * \deprecated use ex_get_partial_var()(exoid, time_step, EX_NODAL, nodal_var_index, 1, start_node,
 num_nodes, var_vals);

 */

/*
 * reads the values of a single nodal variable for a single time step from
 * the database; assume the first time step and nodal variable index is 1
 */

int ex_get_partial_nodal_var(int exoid, int time_step, int nodal_var_index, int64_t start_node,
                             int64_t num_nodes, void *var_vals)
{
  return ex_get_partial_var(exoid, time_step, EX_NODAL, nodal_var_index, 1, start_node, num_nodes,
                            var_vals);
}
