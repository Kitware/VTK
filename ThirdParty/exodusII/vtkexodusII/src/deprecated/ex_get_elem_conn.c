/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
#include "exodusII.h"

/*!
\deprecated Use ex_get_conn()(exoid, EX_ELEM_BLOCK, elem_blk_id, connect, 0, 0)
instead

The function ex_get_elem_conn() reads the connectivity array for an
element block. Memory must be allocated for the connectivity array
(num_elem_this_blk * num_nodes_per_elem in length) before
this routine is called.

\return In case of an error, ex_get_elem_conn() returns a negative number; a
warning will return a positive number.  Possible causes of errors
include:
  -  an element block with the specified ID is not stored in the file.

\param[in]  exoid        exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in]  elem_blk_id  The element block ID.
\param[out] connect      Size [num_elem_this_blk,num_nodes_per_elem].
                         Returned connectivity array; a list of nodes (internal
node
                         IDs; See Section LocalNodeIds) that define each
element. The
                         node index cycles faster than the element index.

Refer to the code example in ex_get_elem_block() for an example of
reading the connectivity for an element block.

 */

int ex_get_elem_conn(int exoid, ex_entity_id elem_blk_id, void_int *connect)
{
  return ex_get_conn(exoid, EX_ELEM_BLOCK, elem_blk_id, connect, 0, 0);
}
