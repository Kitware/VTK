/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgns - ex_get_node_set
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     node_set_id             node set id
 *
 * exit conditions -
 *       int*    node_set_node_list      node list array for the node set
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_set, ex_entity_id, etc

/*!
 * reads the node list for a single node set
 * \deprecated Use ex_get_set()(exoid, EX_NODE_SET, node_set_id,
 * node_set_node_list, NULL)
 */

int ex_get_node_set(int exoid, ex_entity_id node_set_id, void_int *node_set_node_list)
{
  return ex_get_set(exoid, EX_NODE_SET, node_set_id, node_set_node_list, NULL);
}
