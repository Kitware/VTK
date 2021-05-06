/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgnm - ex_get_node_map
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     map_id                  node map id
 *
 * exit conditions -
 *       int*    node_map                node map
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_num_map, etc

/*!
 * reads the node map with specified ID
 * \deprecated Use ex_get_num_map()(exoid, EX_NODE_MAP, map_id, node_map
 */

int ex_get_node_map(int exoid, ex_entity_id map_id, void_int *node_map)
{
  return ex_get_num_map(exoid, EX_NODE_MAP, map_id, node_map);
}
