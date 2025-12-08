/*
   Copyright 2006-2010
        Paul Scherrer Institut, Villigen, Switzerland;
        Achim Gsell
        All rights reserved.

   Authors
        Achim Gsell

   Warning
        This code is under development.
 */

#include <time.h>

#include <string.h>


#include "private/h5_maps.h"
#include "private/h5t_model.h"
#include "private/h5t_types.h"
#include "private/h5t_core.h"
#include "private/h5t_retrieve.h"


#define H5_LOC_ELEM_T   h5_loc_tet_t


static inline void
set_vertex_flags (
        h5t_mesh_t* const m,
        h5_loc_idx_t face_idx,          // in
        h5_loc_idx_t elem_idx,          // in
        h5_int32_t* flags               // in/out
        ) {
	H5_LOC_ELEM_T* elems = (H5_LOC_ELEM_T*)m->loc_elems;
	H5_LOC_ELEM_T* elem = elems + elem_idx;

	if (!h5tpriv_is_leaf_elem (m, elem)) {
		return; // skip non-leaf cells
	}
	if (elem->flags & H5_GHOST_ENTITY) {
		if (!(*flags & H5_BORDER_ENTITY)) {
			// vertex is either border- or front-vertex
			// if it is border-vertex, this flag will be reset later
			*flags |= H5_FRONT_ENTITY;
		}
		return;
	}

	// vertex is either interior- or border-vertex
	if (!(*flags & H5_BORDER_ENTITY)) {
		*flags |= H5_INTERIOR_ENTITY;
	}
	if (elem->flags & H5_BORDER_ENTITY) {
		// cell is border-cell: vertex is either boder- or interior-vertex
		// loop over all facets connected to given vertex
		int num_facets = h5tpriv_ref_elem_get_num_facets_to_vertex (m, face_idx);
		for (int i = 0; i < num_facets; i++) {
			int j = h5tpriv_ref_elem_get_facet_to_vertex (m, face_idx, i);
			// if neighbor is ghost-cell: vertex is border-vertex
			h5_loc_idx_t neighbor_idx = elem->neighbor_indices[j];
			if (neighbor_idx >= 0 &&
			    elems[neighbor_idx].flags & H5_GHOST_ENTITY) {
				*flags |= H5_BORDER_ENTITY;
				*flags &= ~(H5_INTERIOR_ENTITY|H5_FRONT_ENTITY);
			}
		}
	}
}

/*
   edges are either interior-, border or front-entities
 */
static inline void
set_edge_flags (
        h5t_mesh_t* const m,
        h5_loc_idx_t face_idx,          // in
        h5_loc_idx_t elem_idx,          // in
        h5_int32_t* flags               // in/out
        ) {
	H5_LOC_ELEM_T* elems = (H5_LOC_ELEM_T*)m->loc_elems;
	H5_LOC_ELEM_T* elem = elems + elem_idx;

	if (!h5tpriv_is_leaf_elem (m, elem)) {
		return; // skip non-leaf cells
	}

	if (elem->flags & H5_GHOST_ENTITY) {
		if (!(*flags & H5_BORDER_ENTITY)) {
			// entity is either border- or front-entity
			// if it is border-entity, this flag will be reset later
			*flags |= H5_FRONT_ENTITY;
		}
		return;
	}

	// entity is either interior- or border-entity
	if (!(*flags & H5_BORDER_ENTITY)) {
		*flags |= H5_INTERIOR_ENTITY;
	}
	if (elem->flags & H5_BORDER_ENTITY) {
		// cell is border-cell: vertex is either boder- or interior-vertex
		// loop over all facets connected to given vertex
		int num_facets = h5tpriv_ref_elem_get_num_facets_to_edge (m, face_idx);
		for (int i = 0; i < num_facets; i++) {
			int j = h5tpriv_ref_elem_get_facet_to_edge (m, face_idx, i);
			// if neighbor is ghost-cell: vertex is border-vertex
			h5_loc_idx_t neighbor_idx = elem->neighbor_indices[j];
			if (neighbor_idx >= 0 &&
			    elems[neighbor_idx].flags & H5_GHOST_ENTITY) {
				*flags |= H5_BORDER_ENTITY;
				*flags &= ~(H5_INTERIOR_ENTITY|H5_FRONT_ENTITY);
			}
		}
	}
}


/*
   triangle are either interior-, border or front-entities
 */
static inline void
set_triangle_flags (
        h5t_mesh_t* const m,
        h5_loc_idx_t face_idx,          // in
        h5_loc_idx_t elem_idx,          // in
        h5_int32_t* flags               // in/out
        ) {
	H5_LOC_ELEM_T* elems = (H5_LOC_ELEM_T*)m->loc_elems;
	H5_LOC_ELEM_T* elem = elems + elem_idx;

	if (!h5tpriv_is_leaf_elem (m, elem)) {
		return; // skip non-leaf cells
	}

	if (elem->flags & H5_GHOST_ENTITY) {
		if (!(*flags & H5_BORDER_ENTITY)) {
			// entity is either border- or front-entity
			// if it is border-entity, this flag will be reset later
			*flags |= H5_FRONT_ENTITY;
		}
		return;
	}

	// entity is either interior- or border-entity
	if (!(*flags & H5_BORDER_ENTITY)) {
		*flags |= H5_INTERIOR_ENTITY;
	}
	if (elem->flags & H5_BORDER_ENTITY) {
		// cell is border-cell: vertex is either boder- or interior-vertex
		// if neighbor is ghost-cell: vertex is border-vertex
		h5_loc_idx_t neighbor_idx = elem->neighbor_indices[face_idx];
		if (neighbor_idx >= 0 &&
		    elems[neighbor_idx].flags & H5_GHOST_ENTITY) {
			*flags |= H5_BORDER_ENTITY;
			*flags &= ~(H5_INTERIOR_ENTITY|H5_FRONT_ENTITY);
		}
	}
}

/*
   Allocate structure keeping the upward adjacent elements for each vertex.
 */
static inline h5_err_t
alloc_tv (
        h5t_mesh_t* const m,
        const h5_lvl_idx_t from_lvl
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, from_lvl=%u",
	                    m, (unsigned)from_lvl);
	h5_loc_idx_t num_loc_vertices = m->num_loc_vertices[m->num_loaded_levels-1];

	h5t_adjacencies_t* adj = &m->adjacencies;
	// allocate ptr to ID-list per vertex
	TRY( adj->tv.v = h5_alloc (adj->tv.v, num_loc_vertices*sizeof(*adj->tv.v)) );

	size_t i = from_lvl <= 0 ? 0 : m->num_loc_vertices[from_lvl-1];
	memset (adj->tv.v+i, 0, (num_loc_vertices-i)*sizeof(*adj->tv.v));

	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
reset_flags (
        h5t_mesh_t* const m
        ) {
	// traverse all entries in tv
	unsigned int i = 0;
	h5_loc_idlist_t* entry;
	while ((entry = h5tpriv_traverse_tv (m, &i))) {
		entry->flags = 0;
	}

	// traverse all entries in te
	i = 0;
	while ((entry = h5tpriv_traverse_te (m, &i))) {
		entry->flags = 0;
	}

	// traverse all entries in td
	i = 0;
	while ((entry = h5tpriv_traverse_td (m, &i))) {
		entry->flags = 0;
	}

	return H5_SUCCESS;
}

static h5_err_t
update_internal_structs (
        h5t_mesh_t* const m,
        const h5_lvl_idx_t from_lvl
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, from_lvl=%u",
	                    m, (unsigned)from_lvl);
	h5_lvl_idx_t to_lvl = m->num_loaded_levels - 1;
	assert (to_lvl >= 0);

	/* expand structure */
	TRY (alloc_tv (m, from_lvl));

	int num_vertices_of_elem = h5tpriv_ref_elem_get_num_vertices(m);
	int num_edges_of_elem = h5tpriv_ref_elem_get_num_edges(m);
	int num_facets_of_elem = h5tpriv_ref_elem_get_num_facets (m);

	/* loop over all elements starting at from_lvl up to the last loaded level
	   including all ghost elements */
	h5_loc_idx_t elem_idx = (from_lvl <= 0) ? 0 : m->num_interior_elems[from_lvl-1];
	h5_loc_idx_t last = m->num_interior_elems[to_lvl] + m->num_ghost_elems[to_lvl];

	for (; elem_idx < last; elem_idx++) {
		int face_idx;
		// Compute upward adjacent elements for each vertex.
		for (face_idx = 0; face_idx < num_vertices_of_elem; face_idx++) {
			TRY (h5tpriv_enter_tv2 (m, face_idx, elem_idx, NULL));
		}
		// Compute upward adjacent elements for each edge.
		for (face_idx = 0; face_idx < num_edges_of_elem; face_idx++) {
			TRY (h5tpriv_enter_te2 (m, face_idx, elem_idx, NULL));
		}
		// Compute upward adjacent elements for each triangle.
		for (face_idx = 0; face_idx < num_facets_of_elem; face_idx++) {
			TRY (h5tpriv_enter_td2 (m, face_idx, elem_idx, NULL));
		}
	}
	H5_RETURN (H5_SUCCESS);
}

/*
   Release structure keeping the upward adjacent elements for each vertex.
 */
static inline h5_err_t
release_tv (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	h5t_adjacencies_t* adj = &m->adjacencies;
	if (adj->tv.v == NULL)
		H5_LEAVE (H5_SUCCESS);

	h5_loc_idx_t vertex_idx = 0;
	h5_loc_idx_t last = m->num_loc_vertices[m->num_loaded_levels-1];
	for (; vertex_idx < last; vertex_idx++) {
		TRY( h5priv_free_loc_idlist (&adj->tv.v[vertex_idx]) );
	}
	TRY( h5_free (adj->tv.v) );
	adj->tv.v = NULL;
	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
release_internal_structs (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	TRY( release_tv (m) );
	TRY( h5priv_hdestroy (&m->adjacencies.te_hash) );
	TRY( h5priv_hdestroy (&m->adjacencies.td_hash) );
	memset (&m->adjacencies, 0, sizeof (m->adjacencies));
	H5_RETURN (H5_SUCCESS);
}

struct h5t_core_methods h5tpriv_tetm_core_methods = {
	update_internal_structs,
	release_internal_structs,
};
