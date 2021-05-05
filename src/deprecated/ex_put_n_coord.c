/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expcor - ex_put_n_coord
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     start_node_num          starting index (1-based) of coordinates
 *to be written.
 *       int     num_nodes               number of nodes to write coordinates
 *for.
 *       float*  x_coord                 X coord array
 *       float*  y_coord                 y coord array
 *       float*  z_coord                 z coord array
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_partial_coord

/*!
 * \deprecated Use ex_put_partial_coord()(exoid, start_node_num, num_nodes, x_coor, y_coor, z_coor)
 * instead. writes the coordinates of some of the nodes in the model Only writes the 'non-null'
 * arrays. \param   exoid           exodus file id \param   start_node_num  the starting index
 * (1-based) of the coordinates to be written \param   num_nodes       the number of nodes to write
 * coordinates for. \param   x_coor          x coord array \param   y_coor          y coord array
 * \param   z_coor          z coord array
 */

int ex_put_n_coord(int exoid, int64_t start_node_num, int64_t num_nodes, const void *x_coor,
                   const void *y_coor, const void *z_coor)
{
  return ex_put_partial_coord(exoid, start_node_num, num_nodes, x_coor, y_coor, z_coor);
}
