/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *
 *      ex_get_partial_node_set()
 *
 *****************************************************************************
 *
 *  Variable Index:
 *
 *      exoid               - The NetCDF ID of an already open NemesisI file.
 *      node_set_id        - ID of node set to read.
 *      start_node_num     - The starting index of the nodes to be read.
 *      num_nodes          - The number of nodes to read in.
 *      node_set_node_list - List of node IDs in node set.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
#include "exodusII.h" // for ex_get_partial_set, etc

/*!
 * \deprecated use ex_get_partial_set()(exoid, EX_NODE_SET, node_set_id, start_node_num, num_nodes,
                            node_set_node_list, NULL)
 */

/*
 * reads the node list for a single node set
 */

int ex_get_partial_node_set(int exoid, ex_entity_id node_set_id, int64_t start_node_num,
                            int64_t num_nodes, void_int *node_set_node_list)
{
  return ex_get_partial_set(exoid, EX_NODE_SET, node_set_id, start_node_num, num_nodes,
                            node_set_node_list, NULL);
}
