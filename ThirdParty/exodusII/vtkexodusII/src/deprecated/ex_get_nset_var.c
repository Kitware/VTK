/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgssv - ex_get_nset_var
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     time_step               time step number
 *       int     nset_var_index          nodeset variable index
 *       int     nset_blk_id             nodeset id
 *       int     num_node_this_nset      number of nodes in this nodeset
 *
 *
 * exit conditions -
 *       float*  nset_var_vals           array of nodeset variable values
 *
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_var, ex_entity_id, etc

/*!
 * reads the values of a single nodeset variable for one nodeset at
 * one time step in the database; assume the first time step and
 * nodeset variable index is 1
 * \deprecated Use ex_get_var()(exoid, time_step, EX_NODE_SET, nset_var_index,
 * nset_id, num_node_this_nset, nset_var_vals) instead
 */

int ex_get_nset_var(int exoid, int time_step, int nset_var_index, ex_entity_id nset_id,
                    int64_t num_node_this_nset, void *nset_var_vals)
{
  return ex_get_var(exoid, time_step, EX_NODE_SET, nset_var_index, nset_id, num_node_this_nset,
                    nset_var_vals);
}
