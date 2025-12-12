/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5T_TYPES_H
#define __PRIVATE_H5T_TYPES_H

#include "h5core/h5_types.h"
#include "private/h5_types.h"
#include "private/h5_hsearch.h"
//#include "private/h5_maps.h"

#include "private/h5t_ref_elements.h"

#include "private/h5_io.h"
#include "private/h5t_octree.h"


typedef struct h5_glb_vertex {
	h5_glb_idx_t idx;
	h5_coord3d_t P;
} h5_glb_vertex_t;

typedef struct h5_glb_vertex h5_loc_vertex_t;


typedef struct h5_glb_elem {
	h5_glb_idx_t idx;
	h5_glb_idx_t parent_idx;
	h5_glb_idx_t child_idx;
	h5_lvl_idx_t level_idx;
	h5_lvl_idx_t refinement;
	h5_uint32_t flags;
	h5_glb_idx_t indices[1];
} h5_glb_elem_t;
typedef h5_glb_elem_t* h5_glb_elem_p;


typedef struct h5_glb_tri {
	h5_glb_idx_t idx;
	h5_glb_idx_t parent_idx;
	h5_glb_idx_t child_idx;
	h5_lvl_idx_t level_idx;
	h5_lvl_idx_t refinement;
	h5_uint32_t flags;
	h5_glb_idx_t vertex_indices[3];
	h5_glb_idx_t neighbor_indices[3];
} h5_glb_tri_t;

typedef h5_glb_tri_t h5_glb_triangle_t;

typedef struct h5_glb_tet {
	h5_glb_idx_t idx;
	h5_glb_idx_t parent_idx;
	h5_glb_idx_t child_idx;
	h5_lvl_idx_t level_idx;
	h5_lvl_idx_t refinement;
	h5_uint32_t flags;
	h5_glb_idx_t vertex_indices[4];
	h5_glb_idx_t neighbor_indices[4];
} h5_glb_tet_t;
typedef	h5_glb_tet_t h5_glb_tetrahedron_t;


typedef struct h5_loc_tri {
	h5_glb_idx_t glb_idx;           // global index of element
	h5_loc_idx_t parent_idx;        // index of parent element
	h5_loc_idx_t child_idx;         // index of (first) children
	h5_lvl_idx_t level_idx;         // leaf level on which this element has been created
	h5_lvl_idx_t refinement;        // number of refinements of father in macro-grid
	h5_uint32_t flags;
	h5_int32_t my_proc;
	h5_int32_t neighbor_proc;
	h5_loc_idx_t vertex_indices[3];
	h5_loc_idx_t neighbor_indices[3];
} h5_loc_tri_t;

typedef struct h5_loc_tet {
	h5_glb_idx_t glb_idx;
	h5_loc_idx_t parent_idx;
	h5_loc_idx_t child_idx;
	h5_lvl_idx_t level_idx;
	h5_lvl_idx_t refinement;
	h5_uint32_t flags;
	h5_int32_t my_proc;
	h5_int32_t neighbor_proc;
	h5_loc_idx_t vertex_indices[4];
	h5_loc_idx_t neighbor_indices[4];
} h5_loc_tet_t;


// generic data type for local elements
typedef struct h5_loc_elem {
	h5_glb_idx_t glb_idx;
	h5_loc_idx_t parent_idx;
	h5_loc_idx_t child_idx;
	h5_lvl_idx_t level_idx;
	h5_lvl_idx_t refinement;
	h5_uint32_t flags;
	h5_int32_t my_proc;
	h5_int32_t neighbor_proc;
	h5_loc_idx_t indices[1];
} h5_loc_elem_t;

/*** type ids' for compound types ***/
typedef struct h5_dta_types {
	hid_t h5_glb_idx_t;                     /* ID's */
	hid_t h5_int32_t;
	hid_t h5_int64_t;                       /* 64 bit signed integer */
	hid_t h5_float64_t;                     /* 64 bit floating point */
	hid_t h5_coord3d_t;                     /* 3-tuple of 64-bit float */
	hid_t h5_coord6d_t;                     /* 6-tuple of 64-bit float */
	hid_t h5_3glb_idx_t;                    /* 3-tuple of indices */
	hid_t h5_4glb_idx_t;                    /* 4-tuple of indices */
	hid_t h5_4chk_idx_t;
	hid_t h5_vertex_t;                      /* vertex structure */
	hid_t h5_triangle_t;                    /* triangle structure */
	hid_t h5_tet_t;                         /* tetrahedron structure */
	hid_t h5t_glb_tag_idx_t;
#if defined(WITH_PARALLEL_H5GRID)
	hid_t h5_chunk_t;			// chunk structure
	hid_t h5_octree_t;			// octree structure
	hid_t h5_userdata_t;			// userdata structure
	MPI_Datatype mpi_glb_triangle;
	MPI_Datatype mpi_glb_tet;
	MPI_Datatype mpi_glb_vtx;
	MPI_Datatype mpi_chunk;
	MPI_Datatype mpi_edge_list_elem;
#endif
} h5_dta_types_t;

typedef struct h5t_adjacencies {
	struct {
		// h5_size_t size;
		h5_loc_idlist_t** v;
	} tv;
	h5_hashtable_t te_hash;
	h5_hashtable_t td_hash;
} h5t_adjacencies_t;

#define OCT_USERDATA_SIZE 4

typedef struct h5t_oct_userdata {
	h5_chk_idx_t idx[OCT_USERDATA_SIZE];
} h5t_oct_userdata_t;

typedef struct h5t_chunk {
	h5_chk_idx_t idx;
	h5_oct_idx_t oct_idx;
	h5_glb_idx_t elem;
	h5_chk_weight_t weight;
	h5_chk_size_t num_elems;
//	h5_glb_idx_t vtx;
//	h5_glb_idx_t num_vtx;
//	h5_chk_size_t* elems_subchk;
} h5t_chunk_t;

typedef struct h5t_chunks {
	h5_chk_idx_t curr_idx;
	h5_chk_idx_t num_alloc;
	h5_chk_size_t num_levels;
	h5_chk_idx_t* num_chunks_p_level;
	h5t_chunk_t* chunks;
} h5t_chunks_t;

typedef struct h5t_edge_list_elem {
	h5_glb_idx_t vtx1;
	h5_glb_idx_t vtx2;
	h5_glb_idx_t new_vtx;
	h5_int32_t proc;
} h5t_edge_list_elem_t;

typedef struct h5t_edge_list_t {
	h5_int32_t num_items;
	h5t_edge_list_elem_t* items;
	h5_int32_t num_alloc;
} h5_edge_list_t;

typedef struct h5t_vtx_chk_list {
	h5_loc_idx_t vtx;
	h5_chk_idx_t chk;
} h5t_vtx_chk_list_t;

#define NUM_TIMING 27

typedef struct h5t_timing {
	int num_timing;
	int next_time;
 h5_time_t measure[NUM_TIMING];
 char* f;
// h5_time_t t1; // after init mesh
// h5_time_t t2; // after reading octree
// h5_time_t t3; // after reading chunks
// h5_time_t t4; // after reading weights
// h5_time_t t5; // after distributing chunks
// h5_time_t t6; // after reading elems
// h5_time_t t7; // after reading vertices
// h5_time_t t8; // after internal updates
// h5_time_t t9; // begin end refinement
// h5_time_t t10; // after pre ref
// h5_time_t t11; // after refine
// h5_time_t t12; // after boundary edges exchange
// h5_time_t t13; // after weights exchange
// h5_time_t t14; // after init glb elems struct
// h5_time_t t15; // after init glb vertices struct
// h5_time_t t16; // after exchange glb_struct
// h5_time_t t17; // after store glb elems struct
// h5_time_t t18; // after post_refine
// h5_time_t t19; // before close
// h5_time_t t20; // after write weights
// h5_time_t t21; // after write chunks
// h5_time_t t22; // after write octree
// h5_time_t t23; // after calc vtx map
// h5_time_t t24; // after vtx hyperslap select
// h5_time_t t25; // after write vertices
// h5_time_t t26; // after write elems

} h5t_timing_t;

typedef struct h5t_oct_count {
	h5_oct_idx_t oct;
	h5_int32_t count;
} h5t_oct_count_t;

typedef struct h5t_oct_count_list {
	h5_int32_t num_items;
	h5_int32_t size;
	h5t_oct_count_t* items;
} h5t_oct_count_list_t;

struct h5t_read_methods;
struct h5t_store_methods;
struct h5t_retrieve_methods;
struct h5t_access_methods;
struct h5t_adjacency_methods;
struct h5t_core_methods;

typedef struct h5t_methods {
	struct h5t_read_methods* read;
	struct h5t_store_methods* store;
	struct h5t_retrieve_methods* retrieve;
	struct h5t_access_methods* access;
	struct h5t_adjacency_methods* adjacency;
	struct h5t_core_methods* core;
} h5t_methods_t;


struct h5t_mesh {
	/*** book-keeping ***/
	char mesh_name[256];
	const h5t_ref_elem_t*  ref_elem;
	h5_id_t mesh_changed;           /* true if new or has been changed */
	h5_lvl_idx_t leaf_level;        /* idx of current level */
	h5_lvl_idx_t num_leaf_levels;   /* number of levels */
	h5_lvl_idx_t num_loaded_levels;

	/*** chunking ***/
	h5_lvl_idx_t is_chunked;		/* == 1 if mesh is chunked */
#if defined(WITH_PARALLEL_H5GRID)
	h5t_octree_t* octree;
	h5t_chunks_t* chunks;
	h5_dsinfo_t dsinfo_chunks;
	h5_dsinfo_t dsinfo_octree;
	h5_dsinfo_t dsinfo_userdata;
#endif
	h5t_timing_t timing;

	h5_strlist_t*   mtagsets;

	/*** HDF5 IDs ***/
	hid_t mesh_gid;

	/*** functions to handle differnt mesh types ***/
	h5t_methods_t*  methods;

	/*** vertices ***/
	h5_loc_vertex_t* vertices;
	h5_glb_idx_t*   num_glb_vertices;
	h5_loc_idx_t*   num_loc_vertices;
	h5_idxmap_t map_vertex_g2l;     /* map global to local idx */
	h5_loc_idx_t last_stored_vid;
	h5_loc_idx_t last_stored_vid_before_ref; // needed for parallel refinement
	h5_glb_idx_t* num_b_vtx;   // stores the number of boundary vertices per level
	h5_glb_idx_t* first_b_vtx;  // stores the first boundary vertex per level
	h5_dsinfo_t dsinfo_vertices;

	/*** Elements ***/
	h5_loc_elem_t*  loc_elems;

	// number of global elements in mesh including refined for all levels
	h5_glb_idx_t*   num_glb_elems;

	// number of global leaf elements in mesh for all levels
	h5_glb_idx_t*   num_glb_leaf_elems;

	// number of interior (local) elements including refined for all loaded levels
	h5_loc_idx_t*   num_interior_elems;

	// number of interior (local) leaf elements for all loaded levels
	h5_loc_idx_t*   num_interior_leaf_elems;

	// number of ghost elements including refined for all loaded levels
	h5_loc_idx_t*   num_ghost_elems;

	h5_idxmap_t map_elem_g2l;       /* map global id to local index */

	h5_loc_idx_t last_stored_eid;
	h5_loc_idx_t last_stored_eid_before_ref;
	h5_dsinfo_t dsinfo_elems;

	h5_loc_idlist_t* marked_entities;

	/*** Weights ***/

	h5_weight_t num_weights;
	h5_weight_t* weights;
	h5_dsinfo_t dsinfo_weights;

	/*** Adjacencies ***/
	h5t_adjacencies_t adjacencies;

	/*** IndexSets ***/
	h5_loc_idlist_t* index_sets[3];

	/*** File ***/
	h5_file_p	f;
};

#endif
