/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5T_OCTREE_H
#define __PRIVATE_H5T_OCTREE_H


#include "h5core/h5_types.h"
#include "h5core/h5t_octree.h"

#if defined(H5_HAVE_PARALLEL)
#include <vtk_mpi.h>
#endif

#define OCT_MAX_NEIGHBORS 1000

struct h5_oct_point {
	h5_float64_t x;
	h5_float64_t y;
	h5_float64_t z;
	h5_oct_idx_t oct;
	h5_glb_idx_t elem;
};

// Iterators
struct h5t_oct_iterator {
	h5t_octree_t* octree;
	h5_oct_idx_t (*iter)(struct h5t_oct_iterator* iter);
};

/*
 * Inititialization methods for octree
 */
struct h5t_getter_oct_methods;
struct h5t_init_oct_methods;

/*
 * All methods provided by the octree
 */

typedef struct h5t_oct_methods {
	struct h5t_getter_oct_methods* get;
//	struct h5t_set_oct_methods* set;
//	struct h5t_iter_oct_methods* iter;
	struct h5t_init_oct_methods* init;
} h5t_oct_methods_t;

/*
 * Define Octant data type
 */
struct h5_octant {
	h5_oct_idx_t idx;
	h5_oct_idx_t parent_idx;
	h5_oct_idx_t child_idx;
	h5_lvl_idx_t level_idx; // first 3 bits for type x,y,z forth bit for user data changed
//	h5_float64_t bounding_box[6];
	h5_int32_t processor;
	h5_oct_userlev_t userlevels;

};

/*
 * Define octree data type
 */
struct h5_octree {
	/*** variables ***/
	h5_int32_t size_userdata;
	MPI_Comm comm;

	/*** octants ***/
	h5t_octant_t*   octants;
	void*           userdata;
	h5_oct_idx_t 	current_oct_idx;
	h5_oct_idx_t 	nbr_alloc_oct;
	h5_oct_idx_t 	ref_oct_idx;
	// during refinement: ref_oct_idx == current_oct_idx before refinement, otherwise -1
	h5_float64_t bounding_box[6];
	/* */
	h5_int32_t 		maxpoints;

};

typedef struct {
	h5t_octree_t* octree;
	h5_oct_idx_t (*iter)(struct h5t_oct_iterator* iter);
	h5_oct_idx_t current_octant;
	h5_oct_level_t level;
} h5t_oct_iter_t;

///*
// * Getter methods working on a octree
// */
//typedef struct h5t_getter_oct_methods {
//	h5_oct_idx_t (*get_parent)(h5t_octree_t* const,
//	                           const h5_oct_idx_t);
//	h5_oct_idx_t (*get_children)(h5t_octree_t* const,
//	                             const h5_oct_idx_t);
//
//};
//
//typedef struct h5t_init_oct_methods {
//	h5_err_t (*init_octree)(h5t_octree_t*, h5_int32_t, h5_float64_t*);
//};

/*** type ids' for compound types ***/
typedef struct h5_oct_dtypes {
	MPI_Datatype mpi_octant;
} h5_oct_dta_types_t;

h5_err_t H5t_create_mpi_type_octant ( void );

// Setter functions
h5_err_t H5t_set_userlevel (h5t_octree_t* octree, h5_oct_idx_t oct_idx, h5_oct_level_t level);
h5_err_t H5t_set_bounding_box(h5t_octree_t* octree, h5_float64_t* bounding_box);
h5_err_t H5t_set_proc (h5t_octree_t* octree, h5_oct_idx_t oct_idx, h5_int32_t proc);
h5_err_t H5t_set_proc_int (h5t_octree_t* octree, h5_oct_idx_t oct_idx, h5_int32_t proc);

h5_err_t H5t_init_octree (h5t_octree_t** octree, h5_int32_t size_userdata, h5_float64_t* const bounding_box, h5_int32_t maxpoints, const MPI_Comm comm);
h5_err_t H5t_refine_w_points (h5t_octree_t* octree, h5_oct_point_t* points, h5_int32_t nbr_points,	h5_int32_t max_points);
h5_err_t H5t_add_points_to_leaf  (h5t_octree_t* octree, h5_oct_point_t* points, h5_int32_t nbr_points);

h5_err_t H5t_read_octree (h5t_octree_t** octree, h5_oct_idx_t current_oct_idx, h5_int32_t size_userdata, h5_int32_t maxpoints, h5t_octant_t** octants, void** userdata, const MPI_Comm comm);

// Getter functions
h5_oct_level_t H5t_get_userlevel (h5t_octree_t* octree, h5_oct_idx_t oct_idx);
h5_oct_idx_t H5t_get_sibling(h5t_octree_t* octree, h5_oct_idx_t oct_idx);
h5_oct_idx_t H5t_get_children (h5t_octree_t* const octree, const h5_oct_idx_t oct_idx);
h5_oct_idx_t H5t_get_parent (h5t_octree_t* const octree, const h5_oct_idx_t oct_idx);
h5_err_t H5t_get_userdata_r (h5t_octree_t* octree, h5_oct_idx_t oct_idx, void** userdata);
h5_err_t H5t_get_userdata_rw (h5t_octree_t* octree, h5_oct_idx_t oct_idx, void** userdata);
h5_err_t H5t_get_neighbors ( h5t_octree_t* octree, h5_oct_idx_t oct_idx, h5_oct_idx_t** neighbors, h5_oct_idx_t* nbr_neigh, h5_oct_idx_t** ancestor_of_neigh, h5_oct_idx_t* nbr_anc_of_neigh, h5_oct_idx_t kind_of_neigh, h5_oct_level_t userlevel);
h5_err_t H5t_get_point_neighbors ( h5t_octree_t* octree, h5_oct_idx_t oct_idx, h5_oct_idx_t** neighbors, h5_oct_idx_t* nbr_neigh, h5_oct_idx_t** ancestor_of_neigh, h5_oct_idx_t* nbr_anc_of_neigh,  h5_oct_level_t userlevel);
h5_oct_idx_t H5t_get_num_oct_leaflevel (h5t_octree_t* octree);
h5_oct_idx_t H5t_get_num_octants (h5t_octree_t* octree);


h5_int32_t H5t_get_proc (h5t_octree_t* octree, h5_oct_idx_t oct_idx);
int H5t_get_maxpoints (h5t_octree_t* const octree);
h5_float64_t* H5t_get_bounding_box (h5t_octree_t* const octree);
h5_err_t H5t_get_bounding_box_of_octant (h5t_octree_t* const octree, h5_oct_idx_t oct_idx, h5_float64_t* bounding_box);

h5_err_t H5t_complete_userlevel(h5t_octree_t* octree, h5_oct_level_t level);

h5_err_t H5t_set_maxpoints (h5t_octree_t* const octree, int maxpoints);

h5_err_t H5t_update_userdata (h5t_octree_t* const octree);
h5_err_t H5t_update_internal (h5t_octree_t* const octree);

h5_oct_idx_t H5t_iterate_oct (h5t_oct_iterator_t* iter);
h5_err_t H5t_init_oct_iterator (h5t_octree_t* octree, h5t_oct_iterator_t** iter, h5_oct_level_t level);
h5_err_t H5t_init_leafoct_iterator (h5t_octree_t* octree, h5t_oct_iterator_t** iter);
h5_err_t H5t_end_iterate_oct (h5t_oct_iterator_t* iter);

h5_err_t H5t_free_octree (h5t_octree_t* octree);
h5_err_t H5t_write_octree ( h5t_mesh_t* const m);



h5_oct_idx_t H5t_find_leafoctant_of_point (h5t_octree_t* octree, h5_oct_idx_t oct_idx, h5_float64_t* bounding_box, h5_oct_point_t* point);

h5_oct_level_t H5t_oct_has_level (h5t_octree_t* octree, h5_oct_idx_t oct_idx, h5_oct_level_t level);

// for debug purposes only
h5_err_t h5priv_plot_octants (h5t_octree_t* octree);
h5_err_t h5priv_plot_octant_anc (h5t_octree_t* octree, h5_oct_idx_t oct_idx);

h5_err_t plot_leaf_octants (h5t_octree_t* octree);
void print_array (h5_int32_t* neigh, h5_oct_idx_t nbr_neigh, int rank);

#endif
