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

\deprecated Use ex_put_id_map()(exoid, EX_NODE_MAP, node_map)

The function ex_put_node_num_map() writes out the optional node number
map to the database. See Section LocalNodeIds for a description of the
node number map. The function ex_put_init() must be invoked before
this call is made.

\return In case of an error, ex_put_node_num_map() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  a node number map already exists in the file.

\param[in]  exoid     exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in]  node_map  The node number map.

The following code generates a default node number map and outputs it
to an open exodus file. This is a trivial case and included just for
illustration. Since this map is optional, it should be written out
only if it contains something other than the default map.

\code
int error, exoid;
int *node_map = (int *)calloc(num_nodes, sizeof(int));

for (i=1; i <= num_nodes; i++)
   node_map[i-1] = i;

error = ex_put_node_num_map(exoid, node_map);

\comment{Equivalent using non-deprecated function}
error = ex_put_id_map(exoid, EX_NODE_MAP, node_map);
\endcode
 */

int ex_put_node_num_map (int  exoid,
                         const int *node_map)
{
  return ex_put_id_map(exoid, EX_NODE_MAP, node_map);
}
