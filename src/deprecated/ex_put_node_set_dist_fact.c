/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"

/*!
\deprecated Use ex_put_set_dist_fact()(exoid, EX_NODE_SET, node_set_id,
node_set_dist_fact)

The function ex_put_node_set_dist_fact() writes node set distribution
factors for a single node set. The function ex_put_node_set_param()
must be called before this routine is invoked.

Because the distribution factors are floating point values, the
application code must declare the array passed to be the appropriate
type (float or double) to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, ex_put_node_set_dist_fact() returns a
negative number; a warning will return a positive number.  Possible
causes of errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  ex_put_node_set_param() not called previously.
  -  a call to ex_put_node_set_param() specified zero distribution factors.

\param[in] exoid              exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in] node_set_id        The node set ID.
\param[in] node_set_dist_fact Array containing the distribution factors in the
node set.

Refer to the description of ex_put_node_set_param() for a sample code
segment to write out the distribution factors for a node set.
*/

int ex_put_node_set_dist_fact(int exoid, ex_entity_id node_set_id, const void *node_set_dist_fact)
{
  return ex_put_set_dist_fact(exoid, EX_NODE_SET, node_set_id, node_set_dist_fact);
}
