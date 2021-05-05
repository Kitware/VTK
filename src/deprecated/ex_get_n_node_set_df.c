/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"

/*!
 * \deprecated Use ex_get_partial_set_dist_fact()(exoid, EX_NODE_SET, node_set_id, start_num,
 num_df_to_get, node_set_dist_fact) instead
 */

int ex_get_n_node_set_df(int exoid, ex_entity_id node_set_id, int64_t start_num,
                         int64_t num_df_to_get, void *node_set_dist_fact)
{
  return ex_get_partial_set_dist_fact(exoid, EX_NODE_SET, node_set_id, start_num, num_df_to_get,
                                      node_set_dist_fact);
}
