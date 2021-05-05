/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expnm - ex_put_node_map
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     map_id                  node map id
 *       int     *node_map               node map
 *
 * exit conditions -
 *
 *****************************************************************************/

#include "exodusII.h"

/*!
 * writes an node map; this is a vector of integers of length number
 * of nodes
 * \param    exoid                   exodus file id
 * \param    map_id                  node map id
 * \param    node_map                node map
 * \deprecated Use ex_put_num_map()(exoid, EX_NODE_MAP, map_id, node_map)
 */

int ex_put_node_map(int exoid, ex_entity_id map_id, const void_int *node_map)
{
  return ex_put_num_map(exoid, EX_NODE_MAP, map_id, node_map);
}
