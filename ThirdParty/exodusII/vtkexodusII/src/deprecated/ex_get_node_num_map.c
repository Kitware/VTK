/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgnnm - ex_get_node_num_map
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *
 * exit conditions -
 *       int*    node_map                node numbering map array
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"

/*!
\deprecated Use ex_get_id_map()(exoid, EX_NODE_MAP, node_map)

The function ex_get_node_num_map() reads the optional node number
mapnode number map from the database. See Section LocalNodeIds for a
description of the node number map. If a node number map is not stored
in the data file, a default array (1,2,3,. .. num_nodes) is
returned. Memory must be allocated for the node number map array
(num_nodes in length) before this call is made.

\return In case of an error, ex_get_node_num_map() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  if a node number map is not stored, a default map and a warning value are
returned.

\param[in]   exoid      exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[out]  node_map   Returned node number map.

The following code will read a node number map from an open exodus
file :

~~~{.c}
int *node_map, error, exoid;

\comment{read node number map}
node_map = (int *)calloc(num_nodes, sizeof(int));
error = ex_get_node_num_map(exoid, node_map);

\comment{Equivalent using non-deprecated function}
error = ex_get_id_map(exoid, EX_NODE_MAP, node_map);
~~~

*/

int ex_get_node_num_map(int exoid, void_int *node_map)
{
  return ex_get_id_map(exoid, EX_NODE_MAP, node_map);
}
