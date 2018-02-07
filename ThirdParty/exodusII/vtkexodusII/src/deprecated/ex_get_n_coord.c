/*
 * Copyright (c) 2005-2017 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of NTESS nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
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
#include <stdint.h>   // for int64_t

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
