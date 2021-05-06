/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expmv - ex_put_nset_var
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     time_step               time step number
 *       int     nset_var_index          nodeset variable index
 *       int     nset_id                 nodeset id
 *       int     num_nodes_this_nset     number of nodes in this nodeset
 *
 * exit conditions -
 *
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_var, ex_entity_id, etc

/*!
 * writes the values of a single nodeset variable for one nodeset at
 * one time step to the database; assume the first time step and
 * nodeset variable index are 1
 * \param      exoid                   exodus file id
 * \param      time_step               time step number
 * \param      nset_var_index          nodeset variable index
 * \param      nset_id                 nodeset id
 * \param      num_nodes_this_nset     number of nodes in this nodeset
 * \param      nset_var_vals           the values to be written
 * \deprecated Use ex_put_var()(exoid, time_step, EX_NODE_SET, nset_var_index,
 nset_id, num_nodes_this_nset, nset_var_vals)

 */

int ex_put_nset_var(int exoid, int time_step, int nset_var_index, ex_entity_id nset_id,
                    int64_t num_nodes_this_nset, const void *nset_var_vals)
{
  return ex_put_var(exoid, time_step, EX_NODE_SET, nset_var_index, nset_id, num_nodes_this_nset,
                    nset_var_vals);
}
