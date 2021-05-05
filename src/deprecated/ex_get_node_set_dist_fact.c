/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"

/*!
\deprecated Use ex_get_set_dist_fact()(exoid, EX_NODE_SET, node_set_id,
node_set_dist_fact)

The function ex_get_node_set_dist_fact() returns the node set
distribution factors for a single node set. Memory must be allocated
for the list of distribution factors(num_dist_in_set in length)
before this function is invoked.

Because the distribution factors are floating point values, the
application code must declare the array passed to be the appropriate
type (float or double) to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, ex_get_node_set_dist_fact() returns a
negative number; a warning will return a positive number. Possible
causes of errors include:
  -  a warning value is returned if no distribution factors were stored.

\param[in]  exoid               exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in]  node_set_id         The node set ID.
\param[out] node_set_dist_fact  Returned array containing the distribution
factors in the node set.

Refer to the description of ex_get_node_set_param() for a sample code
segment to read a node set's distribution factors.
*/

int ex_get_node_set_dist_fact(int exoid, ex_entity_id node_set_id, void *node_set_dist_fact)
{
  return ex_get_set_dist_fact(exoid, EX_NODE_SET, node_set_id, node_set_dist_fact);
}
