/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5T_STORE_H
#define __PRIVATE_H5T_STORE_H

#include "h5core/h5_types.h"
#include "private/h5t_types.h"
#include "h5core/h5t_store.h"

struct h5t_store_methods {
	h5_err_t (*alloc_loc_elems)(h5t_mesh_t* const, const size_t, const size_t);
	h5_err_t (*pre_refine)(h5t_mesh_t* const);
	h5_loc_idx_t (*refine_elem)(h5t_mesh_t* const, const h5_loc_idx_t);
	int (*get_num_new_elems)(void);
	h5_err_t (*end_store_elems)(h5t_mesh_t* const);
};

extern struct h5t_store_methods h5tpriv_trim_store_methods;
extern struct h5t_store_methods h5tpriv_tetm_store_methods;

h5_lvl_idx_t
h5tpriv_add_level (h5t_mesh_t* const);

static inline h5_err_t
h5tpriv_alloc_loc_elems (
        h5t_mesh_t* const m,
        const size_t cur,
        const size_t new
        ) {
	return m->methods->store->alloc_loc_elems (m, cur, new);
}

h5_loc_idx_t
h5tpriv_add_cell (
        h5t_mesh_t* const, const h5_loc_idx_t, const h5_loc_idx_t*, const h5_weight_t*);


static inline h5_loc_idx_t
h5tpriv_refine_elem (
        h5t_mesh_t * const m,
        const h5_loc_idx_t elem_idx
        ) {
	return m->methods->store->refine_elem (m, elem_idx);
}

static inline int
h5tpriv_get_num_new_elems (
		h5t_mesh_t * const m
		) {
	return m->methods->store->get_num_new_elems ();
}

#if defined(WITH_PARALLEL_H5GRID)
h5_err_t
h5tpriv_init_chunks (h5t_mesh_t* const);

h5_err_t
h5tpriv_grow_chunks (h5t_mesh_t* const, h5_chk_idx_t const size);

h5_err_t
h5tpriv_store_chunks (h5t_mesh_t* const m, h5t_oct_count_list_t* list, h5_chk_idx_t num_chunks, h5_glb_idx_t* elem_range, h5_glb_idx_t* chk_range);

h5_err_t
h5tpriv_create_chunk (h5t_mesh_t* m, h5_oct_idx_t const oct_idx, h5_glb_idx_t const first_elem, h5_chk_weight_t const weight, h5_chk_size_t num_elems, h5_glb_idx_t* chk_range);

h5_err_t
h5tpriv_update_chunks (h5t_mesh_t* m, h5_glb_idx_t* chk_range);

h5_err_t
h5tpriv_free_chunks (h5t_mesh_t* const m);

h5_err_t
h5tpriv_print_chunks (h5t_mesh_t* const m);

h5_err_t
h5tpriv_mark_chk_elems_to_refine (h5t_mesh_t* const m, h5_glb_idxlist_t* glb_marked_entities, h5_oct_point_t* midpoint_list);

int
h5tpriv_octant_is_full (h5t_octree_t* octree, h5_oct_idx_t oct_idx);

h5_err_t
h5tpriv_get_ranges (h5t_mesh_t* const m, h5_glb_idx_t* range, h5_glb_idx_t mycount, h5_glb_idx_t glb_start);
#endif

#endif
