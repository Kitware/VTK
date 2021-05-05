/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"

/*!
\deprecated Use ex_put_conn()(exoid, EX_ELEM_BLOCK, elem_blk_id, connect, 0, 0)

The function ex_put_elem_conn() writes the connectivity array for an
element block. The function ex_put_elem_block() must be invoked before
this call is made.

\return In case of an error, ex_put_elem_conn() returns a negative
number; a warning will return a positive number.  Possible causes of
errors include:
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  ex_put_elem_block() was not called previously.

\param[in] exoid        exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in] elem_blk_id  The element block ID.
\param[in] connect      Size [num_elem_this_blk,num_nodes_per_elem],
                        The connectivity array; a list of nodes (internal node
IDs;
                        See Section LocalNodeIds) that define each element in
the element
                        block. The node index cycles faster than the element
index.

Refer to the code example in ex_put_elem_block() for an example of
writing the connectivity array for an element block.

*/

int ex_put_elem_conn(int exoid, ex_entity_id elem_blk_id, const void_int *connect)
{
  return ex_put_conn(exoid, EX_ELEM_BLOCK, elem_blk_id, connect, 0, 0);
}
