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

#include "exodusII.h" // for ex_put_partial_var, etc

/*!
\ingroup ResultsData

 * \deprecated Use ex_put_partial_var()(exoid, time_step, EX_NODAL, nodal_var_index, 1, start_node,
 num_nodes, nodal_var_vals)
 *
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

int ex_put_partial_nodal_var(int exoid, int time_step, int nodal_var_index, int64_t start_node,
                             int64_t num_nodes, const void *nodal_var_vals)

{
  return ex_put_partial_var(exoid, time_step, EX_NODAL, nodal_var_index, 1, start_node, num_nodes,
                            nodal_var_vals);
}
