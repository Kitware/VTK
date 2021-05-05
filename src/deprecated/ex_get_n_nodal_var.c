/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include <exodusII.h> // for ex_get_partial_var, etc

/*!
\ingroup ResultsData

 * \deprecated Use ex_get_partial_var()(exoid, time_step, EX_NODAL, nodal_var_index, 1, start_node,
num_nodes, var_vals) instead
 */

int ex_get_n_nodal_var(int exoid, int time_step, int nodal_var_index, int64_t start_node,
                       int64_t num_nodes, void *var_vals)
{
  return ex_get_partial_var(exoid, time_step, EX_NODAL, nodal_var_index, 1, start_node, num_nodes,
                            var_vals);
}
