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
\deprecated Use ex_put_concat_sets()(exoid, EX_NODE_SET, &set_specs)

The function ex_put_concat_node_sets() writes the node set ID's, node
sets node count array, node sets distribution factor count array, node
sets node list pointers array, node sets distribution factor pointer,
node set node list, and node set distribution factors for all of the
node sets. ``Concatenated node sets'' refers to the arrays required to
define all of the node sets (ID array, counts arrays, pointers arrays,
node list array, and distribution factors array) as described in
Section 3.10 on page 11. Writing concatenated node sets is more
efficient than writing individual node sets. See #Efficiency for a
discussion of efficiency issues.

Because the distribution factors are floating point values, the
application code must declare the array passed to be the appropriate
type (\c float or \c double) to match the compute word size passed in
ex_create() or ex_open().

\return In case of an error, ex_put_concat_node_sets() returns a
negative number; a warning will return a positive number.  Possible
causes of errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  the number of node sets specified in a call to ex_put_init() was zero or has been exceeded.
  -  a node set with the same ID has already been stored.
  -  the number of distribution factors specified for one of the
     node sets is not zero and is not equal to the number of nodes in the
     same node set.

\param[in] exoid                exodus file ID returned from a previous call to ex_create()
                                or ex_open().

\param[in] node_set_ids         Array containing the node set ID for each set.

\param[in] num_nodes_per_set    Array containing the number of nodes for each set.

\param[in] num_dist_per_set     Array containing the number of distribution factors for each set.

\param[in] node_sets_node_index Array containing the indices into the \c node_set_node_list which
                                are the locations of the first node for each set. These indices are
        0-based. Pass \c NULL for remaining parameters to just set the
        nodeset parameters and not output nodeset data.

\param[in] node_sets_df_index   Array containing the indices into the \c node_set_dist_list which
                                are the locations of the first distribution factor for each set. These
                                indices are 0-based.

\param[in] node_sets_node_list  Array containing the nodes for all sets. Internal node IDs are used in
                                this list (See Section LocalNodeIds).
\param[in] node_sets_dist_fact  Array containing the distribution factors for all sets.


For example, the following code will write out two node sets 
in a concatenated format:

\code
int ids[2], num_nodes_per_set[2], node_ind[2], node_list[8],
    num_df_per_set[2], df_ind[2], error, exoid;

float dist_fact[8];

ids[0] = 20; ids[1] = 21;
num_nodes_per_set[0] = 5; num_nodes_per_set[1] = 3;

node_ind[0] = 0; node_ind[1] = 5;

node_list[0] = 100; node_list[1] = 101; node_list[2] = 102;
node_list[3] = 103; node_list[4] = 104;
node_list[5] = 200; node_list[6] = 201; node_list[7] = 202;

num_df_per_set[0] = 5; num_df_per_set[1] = 3;

df_ind[0] = 0; df_ind[1] = 5;

dist_fact[0] = 1.0; dist_fact[1] = 2.0; dist_fact[2] = 3.0;
dist_fact[3] = 4.0; dist_fact[4] = 5.0;
dist_fact[5] = 1.1; dist_fact[6] = 2.1; 
dist_fact[7] = 3.1;

error = ex_put_concat_node_sets (exoid, ids, num_nodes_per_set, 
                                 num_df_per_set, node_ind, df_ind, 
                                 node_list, dist_fact);
\endcode

 */

int ex_put_concat_node_sets (int   exoid,
                             int  *node_set_ids,
                             int  *num_nodes_per_set,
                             int  *num_dist_per_set,
                             int  *node_sets_node_index,
                             int  *node_sets_df_index,
                             int  *node_sets_node_list,
                             void *node_sets_dist_fact)
{
  struct ex_set_specs set_specs;

  set_specs.sets_ids = node_set_ids;
  set_specs.num_entries_per_set = num_nodes_per_set;
  set_specs.num_dist_per_set = num_dist_per_set;
  set_specs.sets_entry_index = node_sets_node_index;
  set_specs.sets_dist_index = node_sets_df_index;
  set_specs.sets_entry_list = node_sets_node_list;
  set_specs.sets_extra_list = NULL;
  set_specs.sets_dist_fact = node_sets_dist_fact;

  return ex_put_concat_sets(exoid, EX_NODE_SET, &set_specs);
}
