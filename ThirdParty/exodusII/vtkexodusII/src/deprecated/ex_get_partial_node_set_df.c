/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *
 *      ex_get_partial_node_set_df()
 *
 *****************************************************************************
 *
 *  Variable Index:
 *
 *      exoid               - The NetCDF ID of an already open NemesisI file.
 *      node_set_id        - ID of node set to read.
 *      start_num          - The starting index of the dist fact to be read.
 *      num_df_to_get      - The number of distribution factors to read in.
 *      node_set_dist_fact - List of distribution factors in node set.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
#include "exodusII.h"

/*!
 * \deprecated use ex_get_partial_set_dist_fact()(exoid, EX_NODE_SET, node_set_id, start_num,
 num_df_to_get, node_set_dist_fact)
 */

/*
 * reads the distribution factors for a single node set
 */

int ex_get_partial_node_set_df(int exoid, ex_entity_id node_set_id, int64_t start_num,
                               int64_t num_df_to_get, void *node_set_dist_fact)
{
  return ex_get_partial_set_dist_fact(exoid, EX_NODE_SET, node_set_id, start_num, num_df_to_get,
                                      node_set_dist_fact);
}
