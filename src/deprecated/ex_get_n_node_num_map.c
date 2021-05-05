/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * ne_gnnnm - ex_get_n_node_num_map
 *
 * environment - UNIX
 *
 * entry conditions -
 *   input parameters:
 *      int     exoid                   nemesis file id
 *
 * exit conditions -
 *      int*    node_map                node numbering map array
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_partial_id_map, etc

/*!
 *  \deprecated Use ex_get_partial_id_map()(exoid, EX_NODE_MAP, start_ent, num_ents, node_map)
 * instead
 */

int ex_get_n_node_num_map(int exoid, int64_t start_ent, int64_t num_ents, void_int *node_map)
{
  return ex_get_partial_id_map(exoid, EX_NODE_MAP, start_ent, num_ents, node_map);
}
