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
\deprecated Use ex_get_concat_sets()(exoid, EX_NODE_SET, set_specs) instead

The function ex_get_concat_node_sets() reads the node set ID's, node
set node count array, node set distribution factors count array, node
set node pointers array, node set distribution factors pointer array,
node set node list, and node set distribution factors for all of the
node sets. ``Concatenated node sets'' refers to the arrays required to
define all of the node sets (ID array, counts arrays, pointers arrays,
node list array, and distribution factors array) as described in
Section 3.10 on page 11.

Because the distribution factors are floating point values, the
application code must declare the array passed to be the appropriate
type (\c float or \c double) to match the compute word size passed in
ex_create() or ex_open().

The length of each of the returned arrays can be determined by
invoking ex_inquire() or ex_inquire_int().

\return In case of an error, ex_get_concat_node_sets() returns a
negative number; a warning will return a positive number. Possible
causes of errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  a warning value is returned if no node sets are stored in the file.


\param[in]  exoid                 exodus file ID returned from a previous call to ex_create()
                                  or ex_open().

\param[out]  node_set_ids         Returned array containing the node set ID for each set.

\param[out]  num_nodes_per_set    Returned array containing the number of nodes for each set.

\param[out]  num_df_per_set       Returned array containing the number of distribution factors for each
                                  set.

\param[out]  node_sets_node_index Returned array containing the indices into the \c node_set_node_list
                                  which are the locations of the first node for each set. These indices
          are 0-based.

\param[out]  node_sets_df_index   Returned array containing the indices into the \c node_set_dist_fact
                                  which are the locations of the first distribution factor for each
          set. These indices are 0-based.

\param[out]  node_sets_node_list  Returned array containing the nodes for all sets. Internal node IDs
                                  are used in this list (see Section LocalNodeIds).

\param[out]  node_sets_dist_fact  Returned array containing the distribution factors for all sets.


As an example, the following code segment will read concatenated node
sets:

\code
#include "exodusII.h"

int error, exoid, num_node_sets, list_len, *ids, 
    *num_nodes_per_set, *num_df_per_set, *node_ind, 
    *df_ind, *node_list;

float *dist_fact

\comment{read concatenated node sets}
num_node_sets = ex_inquire_int(exoid, EX_INQ_NODE_SETS);

ids               = (int *) calloc(num_node_sets, sizeof(int));
num_nodes_per_set = (int *) calloc(num_node_sets, sizeof(int));
num_df_per_set    = (int *) calloc(num_node_sets, sizeof(int));
node_ind          = (int *) calloc(num_node_sets, sizeof(int));
df_ind            = (int *) calloc(num_node_sets, sizeof(int));

list_len = ex_inquire_int(exoid, EX_INQ_NS_NODE_LEN);
node_list = (int *) calloc(list_len, sizeof(int));

list_len = ex_inquire_int(exoid, EX_INQ_NS_DF_LEN);
dist_fact = (float *) calloc(list_len, sizeof(float));

error = ex_get_concat_node_sets (exoid, ids, num_nodes_per_set, 
                                 num_df_per_set, node_ind, df_ind, 
                                 node_list, dist_fact);
\endcode
*/

int ex_get_concat_node_sets (int   exoid,
                             int  *node_set_ids,
                             int  *num_nodes_per_set, 
                             int  *num_df_per_set, 
                             int  *node_sets_node_index,
                             int  *node_sets_df_index,
                             int  *node_sets_node_list, 
                             void *node_sets_dist_fact)
{
  struct ex_set_specs set_specs;

  set_specs.sets_ids = node_set_ids;
  set_specs.num_entries_per_set = num_nodes_per_set;
  set_specs.num_dist_per_set = num_df_per_set;
  set_specs.sets_entry_index = node_sets_node_index;
  set_specs.sets_dist_index = node_sets_df_index;
  set_specs.sets_entry_list = node_sets_node_list;
  set_specs.sets_extra_list = NULL;
  set_specs.sets_dist_fact = node_sets_dist_fact;

  return ex_get_concat_sets(exoid, EX_NODE_SET, &set_specs);
}
