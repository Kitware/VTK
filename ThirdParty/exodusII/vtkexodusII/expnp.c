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
\deprecated Use ex_put_set_param()(exoid, EX_NODE_SET, node_set_id, num_nodes_in_set, num_dist_in_set)

The function ex_put_node_set_param() writes the node set ID, the
number of nodes which describe a single node set, and the number of
node set distribution factors for the node set.

\return In case of an error, ex_put_node_set_param() returns a
negative number; a warning will return a positive number.  Possible
causes of errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  the number of node sets specified in the call to ex_put_init() was zero or has been exceeded.
  -  a node set with the same ID has already been stored.
  -  the specified number of distribution factors is not zero and is not equal to the number of nodes.

\param[in] exoid              exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in] node_set_id        The node set ID.
\param[in] num_nodes_in_set   The number of nodes in the node set.
\param[in] num_dist_in_set    The number of distribution factors in the node set. This should be
                              either 0 (zero) for no factors, or should equal \c num_nodes_in_set.


The following code segment will write out a node set to an open exodus
file :

\code
int id, num_nodes_in_set, num_dist_in_set, error, exoid, *node_list;
float *dist_fact;

\comment{write node set parameters}
id = 20; num_nodes_in_set = 5; num_dist_in_set = 5;
error = ex_put_node_set_param(exoid, id, num_nodes_in_set, 
                              num_dist_in_set);

\comment{write node set node list}
node_list = (int *) calloc (num_nodes_in_set, sizeof(int));
node_list[0] = 100; node_list[1] = 101; node_list[2] = 102;
node_list[3] = 103; node_list[4] = 104;

error = ex_put_node_set(exoid, id, node_list);

\comment{write node set distribution factors}
dist_fact = (float *) calloc (num_dist_in_set, sizeof(float));
dist_fact[0] = 1.0; dist_fact[1] = 2.0; dist_fact[2] = 3.0;
dist_fact[3] = 4.0; dist_fact[4] = 5.0;

error = ex_put_node_set_dist_fact(exoid, id, dist_fact);

\comment{Same result using non-deprecated functions}
error = ex_put_set_param(exoid, EX_NODE_SET, id, num_nodes_in_set, num_dist_in_set);
error = ex_put_set(exoid, EX_NODE_SET, id, node_list);
error = ex_put_set_dist_fact(exoid, EX_NODE_SET, id, dist_fact);
\endcode
 */

int ex_put_node_set_param (int exoid,
                           int node_set_id,
                           int num_nodes_in_set,
                           int num_dist_in_set)
{
  return ex_put_set_param(exoid, EX_NODE_SET, node_set_id, num_nodes_in_set, num_dist_in_set);
}
