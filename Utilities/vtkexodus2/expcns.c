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
/*****************************************************************************
*
* expcns - ex_put_concat_node_sets
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int*    node_set_ids            array of node set ids
*       int*    num_nodes_per_set       array of number of nodes per set
*       int*    num_dist_per_set        array of number of dist fact  per set
* ----------pass in NULL for remaining args if just want to set params -------------
*       int*    node_sets_node_index    array of set indices into node list
*       int*    node_sets_df_index      array of set indices into dist fact list
*       int*    node_set_node_list      array of node list #'s for node set
*       void*   node_set_dist_fact      array of dist factors for node set
*
* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * writes the node set ID's, node set count array, node set pointers array, 
 * and node set node list for all of the node sets
 * \param    exoid                   exodus file id
 * \param    node_set_ids            array of node set ids
 * \param    num_nodes_per_set       array of number of nodes per set
 * \param    num_dist_per_set        array of number of dist fact  per set
 * \param    node_sets_node_index    array of set indices into node list
 * \param    node_sets_df_index      array of set indices into dist fact list
 * \param    node_sets_node_list     array of node list #'s for node set
 * \param    node_sets_dist_fact     array of dist factors for node set
 * \deprecated Use ex_put_concat_sets()(exoid, EX_NODE_SET, &set_specs)
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
