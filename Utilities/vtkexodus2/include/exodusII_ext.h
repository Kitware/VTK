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

#ifndef __exodus_ext_h
#define __exodus_ext_h

#ifdef __cplusplus
extern "C" {
#endif

  /**
 * \defgroup APIStructs Structures used by external API functions.
 * @{
 */
typedef struct ex_init_params {
  char title[MAX_LINE_LENGTH + 1];
  int num_dim;
  int num_nodes;
  int num_edge;
  int num_edge_blk;
  int num_face;
  int num_face_blk;
  int num_elem;
  int num_elem_blk;
  int num_node_sets;
  int num_edge_sets;
  int num_face_sets;
  int num_side_sets;
  int num_elem_sets;
  int num_node_maps;
  int num_edge_maps;
  int num_face_maps;
  int num_elem_maps;
} ex_init_params;

typedef struct ex_block_params {
  int*   edge_blk_id;
  char** edge_type;
  int*   num_edge_this_blk;
  int*   num_nodes_per_edge;
  int*   num_attr_edge;
  int*   face_blk_id;
  char** face_type;
  int*   num_face_this_blk;
  int*   num_nodes_per_face;
  int*   num_attr_face;
  int*   elem_blk_id;
  char** elem_type;
  int*   num_elem_this_blk;
  int*   num_nodes_per_elem;
  int*   num_edges_per_elem;
  int*   num_faces_per_elem;
  int*   num_attr_elem;
  int    define_maps;
} ex_block_params;

typedef struct ex_set_specs {
  int* sets_ids;
  int* num_entries_per_set;
  int* num_dist_per_set;
  int* sets_entry_index;
  int* sets_dist_index;
  int* sets_entry_list;
  int* sets_extra_list;
  void* sets_dist_fact;
} ex_set_specs;

typedef struct ex_var_params {
  int  num_glob;
  int  num_node;
  int  num_edge;
  int* edge_var_tab;
  int  num_face;
  int* face_var_tab;
  int  num_elem;
  int* elem_var_tab;
  int  num_nset;
  int* nset_var_tab;
  int  num_eset;
  int* eset_var_tab;
  int  num_fset;
  int* fset_var_tab;
  int  num_sset;
  int* sset_var_tab;
  int  num_elset;
  int* elset_var_tab;
} ex_var_params;
  /* @} */

#ifndef EXODUS_EXPORT
#define EXODUS_EXPORT extern
#endif /* EXODUS_EXPORT */

/*  (MODIFIED) Write Initialization Parameters */
EXODUS_EXPORT int ex_put_init_ext       (int, const ex_init_params*);

/*  (MODIFIED) Read Initialization Parameters */
EXODUS_EXPORT int ex_get_init_ext       (int, ex_init_params*);

/*  Write Node Edge Face or Element Number Map */
EXODUS_EXPORT int ex_put_num_map        (int, ex_entity_type, int, const int*);

/*  Read Number Map */
EXODUS_EXPORT int ex_get_num_map        (int, ex_entity_type, int, int*);

/*  Write Edge Face or Element Block Parameters */
EXODUS_EXPORT int ex_put_block          (int, ex_entity_type, int, const char*, int, int,
                                         int, int, int);

/*  Read Edge Face or Element Block Parameters */
EXODUS_EXPORT int ex_get_block          (int, ex_entity_type, int, char*, int*, int*, int*,
                                         int*, int*);

/*  Write All Edge Face and Element Block Parameters */
EXODUS_EXPORT int ex_put_concat_all_blocks(int, const ex_block_params*);

/*  Write Edge Face or Element Block Connectivity */
EXODUS_EXPORT int ex_put_conn           (int, ex_entity_type, int, const int*, const int*,
                                         const int*);

/*  Read Edge Face or Element Block Connectivity */
EXODUS_EXPORT int ex_get_conn           (int, ex_entity_type, int, int*, int*, int*);

/*  Write Edge Face or Element Block Attributes */
EXODUS_EXPORT int ex_put_attr           (int, ex_entity_type, int, const void*);

/*  Read Edge Face or Element Block Attributes */
EXODUS_EXPORT int ex_get_attr           (int, ex_entity_type, int, void*);

/*  Write One Edge Face or Element Block Attribute */
EXODUS_EXPORT int ex_put_one_attr       (int, ex_entity_type, int, int, const void*);

/*  Read One Edge Face or Element Block Attribute */
EXODUS_EXPORT int ex_get_one_attr       (int, ex_entity_type, int, int, void*);

/*  Write Edge Face or Element Block Attribute Names */
EXODUS_EXPORT int ex_put_attr_names     (int, ex_entity_type, int, char**);

/*  Read Edge Face or Element Block Attribute Names */
EXODUS_EXPORT int ex_get_attr_names     (int, ex_entity_type, int, char**);

/*  Write Node Edge Face or Side Set Parameters */
EXODUS_EXPORT int ex_put_set_param      (int, ex_entity_type, int, int, int);

/*  Read Node Edge Face or Side Set Parameters */
EXODUS_EXPORT int ex_get_set_param      (int, ex_entity_type, int, int*, int*);

/*  Write a Node Edge Face or Side Set */
EXODUS_EXPORT int ex_put_set            (int, ex_entity_type, int, const int*, const int*);

/*  Read a Node Edge Face or Side Set */
EXODUS_EXPORT int ex_get_set            (int, ex_entity_type, int, int*, int*);

/*  Write Node Edge Face or Side Set Distribution Factors */
EXODUS_EXPORT int ex_put_set_dist_fact  (int, ex_entity_type, int, const void*);

/*  Read Node Edge Face or Side Set Distribution Factors */
EXODUS_EXPORT int ex_get_set_dist_fact  (int, ex_entity_type, int, void*);

/*  Write Concatenated Node Edge Face or Side Sets */
EXODUS_EXPORT int ex_put_concat_sets    (int, ex_entity_type, const ex_set_specs*);

/*  Read Concatenated Node Edge Face or Side Sets */
EXODUS_EXPORT int ex_get_concat_sets(int, ex_entity_type, ex_set_specs*);

/*  (MODIFIED) Write All Results Variables Parameters */
EXODUS_EXPORT int ex_put_all_var_param_ext(int, const ex_var_params*);

/*  Write Edge Face or Element Variable Values on Blocks or Sets at a Time Step */
EXODUS_EXPORT int ex_put_var            (int, int, ex_entity_type, int, int, int,
                                         const void*);

/*  Read Edge Face or Element Variable Values Defined On Blocks or Sets at a Time Step */
EXODUS_EXPORT int ex_get_var            (int, int, ex_entity_type, int, int, int, void*);

/*  Read Edge Face or Element Variable Values Defined On Blocks or Sets Through Time */
EXODUS_EXPORT int ex_get_var_time       (int, ex_entity_type, int, int, int, int,
                                         void*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __exodus_ext_h */
