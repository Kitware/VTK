/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h" // for ex_put_set, ex_entity_id, etc

/*!
\deprecated Use ex_put_set()(exoid, EX_NODE_SET, node_set_id,
node_set_node_list, NULL)

The function ex_put_node_set() writes the node list for a single node
set. The function ex_put_node_set_param() must be called before this
routine is invoked.

\return In case of an error, ex_put_node_set() returns a negative
number; a warning will return a positive number.  Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  ex_put_node_set_param() not called previously.

\param[in] exoid              exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in] node_set_id        The node set ID.
\param[in] node_set_node_list Array containing the node list for the node set.
Internal node IDs are
                              used in this list (See Section LocalNodeIds).

Refer to the description of ex_put_node_set_param() for a sample code
segment to write out a node set.
*/

int ex_put_node_set(int exoid, ex_entity_id node_set_id, const void_int *node_set_node_list)
{
  return ex_put_set(exoid, EX_NODE_SET, node_set_id, node_set_node_list, NULL);
}
