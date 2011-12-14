/*
 * Copyright (c) 2005 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Governement
 * retains certain rights in this software.
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
 *     * Neither the name of Sandia Corporation nor the names of its
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


\param[in] exoid        exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in] elem_blk_id  The element block ID.
\param[in] connect      Size [num_elem_this_blk,num_nodes_per_elem],
                        The connectivity array; a list of nodes (internal node IDs; 
                        See Section LocalNodeIds) that define each element in the element
                        block. The node index cycles faster than the element index.

Refer to the code example in ex_put_elem_block() for an example of
writing the connectivity array for an element block.

*/

int ex_put_elem_conn (int   exoid,
                      int   elem_blk_id,
                      const int  *connect)
{
  return ex_put_conn(exoid, EX_ELEM_BLOCK, elem_blk_id, connect, 0, 0);
}
