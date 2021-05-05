/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgcor - ex_get_n_coord
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     start_node_num          starting index of coordinates to be
 *returned.
 *       int     num_nodes               number of nodes to read coordinates for.
 *
 * exit conditions -
 *       float*  x_coord                 X coord array
 *       float*  y_coord                 y coord array
 *       float*  z_coord                 z coord array
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_partial_coord

/*!
 *       \deprecated Use ex_get_partial_coord()(exoid, start_node_num, num_nodes, x_coor, y_coor,
 * z_coor) instead reads the coordinates of the nodes. Memory must be allocated for the coordinate
 * arrays (x_coor, y_coor, and z_coor) before this call is made. The length of each of these arrays
 * is the number of nodes in the mesh.  Because the coordinates are floating point values, the
 * application code must declare the arrays passed to be the appropriate type "float" or "double" to
 * match the compute word size passed in ex_create() or ex_open() \param      exoid  exodus file id
 * \param      start_node_num  the starting index of the coordinates to be
 * returned.
 * \param      num_nodes  the number of nodes to read coordinates for.
 * \param[out] x_coor Returned X coordinates of the nodes. These are
 *                    returned only if x_coor is non-NULL.
 * \param[out] y_coor Returned Y coordinates of the nodes. These are
 *                    returned only if y_coor is non-NULL.
 * \param[out] z_coor Returned Z coordinates of the nodes. These are
 *                    returned only if z_coor is non-NULL.
 */

int ex_get_n_coord(int exoid, int64_t start_node_num, int64_t num_nodes, void *x_coor, void *y_coor,
                   void *z_coor)
{
  return ex_get_partial_coord(exoid, start_node_num, num_nodes, x_coor, y_coor, z_coor);
}
