/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

/*****************************************************************************
 *
 * ne_pnnnm - ex_put_n_node_num_map
 *
 * entry conditions -
 *   input parameters:
 *      int     exoid                   exodus file id
 *      int     start_ent               first entry in node_map
 *      int     num_ents                number of entries in node_map
 *       int*    node_map                node numbering map
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_partial_id_map, etc

/*!
 * \deprecated Use ex_put_partial_id_map()(exoid, EX_NODE_MAP, start_ent, num_ents, node_map)
 */

/*
 * writes out the node numbering map to the database; allows node numbers
 * to be non-contiguous
 */

int ex_put_n_node_num_map(int exoid, int64_t start_ent, int64_t num_ents, const void_int *node_map)
{
  return ex_put_partial_id_map(exoid, EX_NODE_MAP, start_ent, num_ents, node_map);
}
