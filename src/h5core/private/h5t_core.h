/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5T_CORE_H
#define __PRIVATE_H5T_CORE_H


#define MAX_CHUNKS_PER_OCTANT 4 // WARNING there are probably places where the makro isn't used
// also it's not simply changeable since types ect. have to be adjusted!
#define NUM_OCTANTS 8

#define UPDATE_WEIGHTS 1 // if 1 it is split, if 2 they are copied. checkout h5t_store.c update_weight_children()

#define MAX_NUM_ELEMS_TO_REFINE_LOCALLY 2048 // used define instead of having number in code


#include "private/h5t_tags.h"

#include "private/h5t_types.h"

#include "private/h5t_access.h"
#include "private/h5t_adjacencies.h"
#include "private/h5t_map.h"
#include "private/h5t_model.h"
#include "private/h5t_ref_elements.h"
#include "private/h5t_io.h"
#include "private/h5t_retrieve.h"
#include "private/h5t_store.h"

#include "private/h5t_err.h"

#include "private/h5t_octree.h"

#include <assert.h>

typedef struct h5t_te_entry_key {
	h5_loc_idx_t vids[2];
} h5t_te_entry_key_t;

typedef struct h5t_td_entry_key {
	h5_loc_idx_t vids[3];
} h5t_td_entry_key_t;

/*
   List of all upward adjacent elements of same coarsness of a specific face.
   The face is specified by its local vertex IDs.
 */
typedef struct h5_te_entry {
	h5t_te_entry_key_t key;
	h5_loc_idlist_t* value;
} h5t_te_entry_t;

typedef struct h5_td_entry {
	h5t_td_entry_key_t key;
	h5_loc_idlist_t* value;
} h5t_td_entry_t;

h5_err_t
h5tpriv_enter_tv2 (
        h5t_mesh_t* const m,
        h5_loc_idx_t face_idx,          // in
        h5_loc_idx_t elem_idx,          // in
        h5_loc_idlist_t** idlist        // out
        );

h5_err_t
h5tpriv_enter_te2 (
        h5t_mesh_t* const m,
        h5_loc_idx_t face_idx,
        h5_loc_idx_t elem_idx,
        h5_loc_idlist_t** idlist
        );

h5_err_t
h5tpriv_enter_td2 (
        h5t_mesh_t* const m,
        h5_loc_idx_t face_idx,
        h5_loc_idx_t elem_idx,
        h5_loc_idlist_t** idlist        // out
        );

h5_err_t
h5tpriv_find_tv2 (
        h5t_mesh_t* const m,
        h5_loc_idx_t face_idx,
        h5_loc_idx_t elem_idx,
        h5_loc_idlist_t** idlist
        );

h5_err_t
h5tpriv_find_tv3 (
        h5t_mesh_t* const m,
        h5_loc_idx_t vtx_idx,
        h5_loc_idlist_t** idlist
        );

h5_err_t
h5tpriv_find_te (
        h5t_mesh_t* const m,
        h5_loc_idx_t edge_id,   // in
        h5_loc_idlist_t** idlist        // out
        );

h5_err_t
h5tpriv_find_te2 (
        h5t_mesh_t* const m,
        h5_loc_idx_t face_idx,  // in
        h5_loc_idx_t elem_idx,  // in
        h5_loc_idlist_t** idlist        // out
        );

h5_err_t
h5tpriv_find_td (
        h5t_mesh_t* const m,
        h5_loc_idx_t triangle_id,
        h5_loc_idlist_t** idlist
        );

h5_err_t
h5tpriv_find_td2 (
        h5t_mesh_t* const m,
        h5_loc_idx_t face_idx,
        h5_loc_idx_t elem_idx,
        h5_loc_idlist_t** idlist
        );

static inline h5_loc_idlist_t*
h5tpriv_traverse_tv (
        h5t_mesh_t* const m,
        unsigned int* i
        ) {
	assert (i != NULL);
	if (*i >= m->num_loc_vertices[m->num_loaded_levels-1]) {
		return NULL;
	}
	h5_loc_idlist_t* result = m->adjacencies.tv.v[*i];
	(*i)++;
	return result;
}

static inline h5_loc_idlist_t*
h5tpriv_traverse_te (
        h5t_mesh_t* const m,
        unsigned int* i
        ) {
	assert (i != NULL);
	if (*i < 1) *i = 1;
	h5t_te_entry_t* entry = h5priv_htraverse (&m->adjacencies.te_hash, i);
	h5_loc_idlist_t* result = NULL;
	if (entry) {
		result = entry->value;
	}
	return result;
}

static inline h5_loc_idlist_t*
h5tpriv_traverse_td (
        h5t_mesh_t* const m,
        unsigned int* i
        ) {
	assert (i != NULL);
	h5t_td_entry_t* entry = h5priv_htraverse (&m->adjacencies.td_hash, i);
	h5_loc_idlist_t* result = NULL;
	if (entry) {
		result = entry->value;
	}
	return result;
}

struct h5t_core_methods {
	h5_err_t (*update_internal_structs)(
	        h5t_mesh_t* const,
	        h5_lvl_idx_t);
	h5_err_t (*release_internal_structs)(
	        h5t_mesh_t* const);
};

extern struct h5t_core_methods h5tpriv_trim_core_methods;
extern struct h5t_core_methods h5tpriv_tetm_core_methods;

static inline h5_err_t
h5tpriv_release_adjacency_structs (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p", m);
	if (m->methods->adjacency == NULL) {
		H5_LEAVE (H5_OK);
	}
	H5_RETURN (m->methods->core->release_internal_structs(m));
}

static inline h5_err_t
h5tpriv_update_internal_structs (
        h5t_mesh_t* const m,
        const h5_lvl_idx_t level_id
        ) {
	H5_PRIV_API_ENTER (h5_err_t, "m=%p, level_id=%d", m, level_id);
	if (m->methods->adjacency == NULL) {
		H5_LEAVE (H5_OK);
	}
	H5_RETURN (m->methods->core->update_internal_structs(m, level_id));
}

#endif
