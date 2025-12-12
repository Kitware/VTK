/*
   Copyright 2006-2011
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
#include <assert.h>

#include "private/h5t_types.h"
#include "private/h5t_model.h"
#include "private/h5t_core.h"
#include "private/h5t_core.h"

#define H5_LOC_ELEM_T h5_loc_tri_t

/*
   loop over all connected cells: flags |= cell->flags

   if result is:
   H5_GHOST_ENTITY==0 && H5_BORDER_ENTITY==0 && H5_INTERIOR_ENTITY==0: illegal
   H5_GHOST_ENTITY==0 && H5_BORDER_ENTITY==0 && H5_INTERIOR_ENTITY==1: interior
   H5_GHOST_ENTITY==0 && H5_BORDER_ENTITY==1 && H5_INTERIOR_ENTITY==0: illegal(?)
   H5_GHOST_ENTITY==0 && H5_BORDER_ENTITY==1 && H5_INTERIOR_ENTITY==1: interior
   H5_GHOST_ENTITY==1 && H5_BORDER_ENTITY==0 && H5_INTERIOR_ENTITY==0: front
   H5_GHOST_ENTITY==1 && H5_BORDER_ENTITY==0 && H5_INTERIOR_ENTITY==1: illegal
   H5_GHOST_ENTITY==1 && H5_BORDER_ENTITY==1 && H5_INTERIOR_ENTITY==0: border
   H5_GHOST_ENTITY==1 && H5_BORDER_ENTITY==1 && H5_INTERIOR_ENTITY==1: border

 */
static inline void
set_vertex_flags (
        h5t_mesh_t* const m,
        h5_loc_idlist_t* list,          // in: all upward adj cell of vertex
        h5_loc_idx_t* num_all_partition // in/out: increment for each vertex
        ) {
	H5_LOC_ELEM_T* cell;
	h5_loc_idx_t cell_idx;
	list->flags = 0;
	for (size_t i = 0; i < list->num_items; i++) {
		cell_idx = h5tpriv_get_elem_idx (list->items[i]);
		cell = (H5_LOC_ELEM_T*)m->loc_elems + cell_idx;
		list->flags |= cell->flags;
	}
	int32_t flags = list->flags & (H5_INTERIOR_ENTITY|H5_BORDER_ENTITY|H5_GHOST_ENTITY);
	if ((flags & ~H5_BORDER_ENTITY) == H5_INTERIOR_ENTITY) {
		// interior flag set, border doesn't matter
		flags = H5_INTERIOR_ENTITY;

	} else if (flags == H5_GHOST_ENTITY) {
		// only ghost flag set
		flags = H5_FRONT_ENTITY;

	} else if ((flags & ~H5_INTERIOR_ENTITY) == (H5_BORDER_ENTITY|H5_GHOST_ENTITY)) {
		// ghost and border flag is set, interior doesn't matter
		flags = H5_BORDER_ENTITY;

	} else {
		// entity not on current level
	}
	if (list->flags & H5_GEOBORDER_ENTITY) {
		for (size_t i = 0; i < list->num_items; i++) {
			h5_loc_idx_t face_idx = h5tpriv_get_face_idx (list->items[i]);
			cell_idx = h5tpriv_get_elem_idx (list->items[i]);
			cell = (H5_LOC_ELEM_T*)m->loc_elems + cell_idx;
			int num_facets = h5tpriv_ref_elem_get_num_facets_to_vertex (m, face_idx);
			for (int i = 0; i < num_facets; i++) {
				int j = h5tpriv_ref_elem_get_facet_to_vertex (m, face_idx, i);
				if (cell->neighbor_indices[j] == -1) {
					flags |= H5_GEOBORDER_ENTITY;
				}
			}
		}
	}
	list->flags = flags;
}

/*
   edges are either interior-, border or front-entities
 */
static inline void
set_edge_flags (
        h5t_mesh_t* const m,
        h5_loc_idlist_t* list,          // in
        h5_loc_idx_t* num_all_partition // in/out
        ) {
	H5_LOC_ELEM_T* cell;
	h5_loc_idx_t cell_idx;
	list->flags = 0;
	for (size_t i = 0; i < list->num_items; i++) {
		cell_idx = h5tpriv_get_elem_idx (list->items[i]);
		cell = (H5_LOC_ELEM_T*)m->loc_elems + cell_idx;
		list->flags |= cell->flags;
	}
	int32_t flags = list->flags & (H5_INTERIOR_ENTITY|H5_BORDER_ENTITY|H5_GHOST_ENTITY);
	if ((flags & ~H5_BORDER_ENTITY) == H5_INTERIOR_ENTITY) {
		// interior flag set, border doesn't matter
		flags = H5_INTERIOR_ENTITY;

	} else if (flags == H5_GHOST_ENTITY) {
		// only ghost flag set
		flags = H5_FRONT_ENTITY;

	} else if ((flags & ~H5_INTERIOR_ENTITY) == (H5_BORDER_ENTITY|H5_GHOST_ENTITY)) {
		// ghost and border flag is set, interior doesn't matter
		flags = H5_BORDER_ENTITY;

	} else {
		// entity not on current level
	}
	if (list->flags & H5_GEOBORDER_ENTITY) {
		if (list->num_items == 1) {
			flags |= H5_GEOBORDER_ENTITY;
		}
	}
	list->flags = flags;
}


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

/*
   add new entities
   set flags for all entities and count entities in all-partition
   create index sets
 */
static inline h5_err_t
update_internal_structs (
        h5t_mesh_t* const m,
        const h5_lvl_idx_t from_lvl
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, from_lvl=%u",
	                    m, (unsigned)from_lvl);

	//////
	// add new entities
	h5_lvl_idx_t to_lvl = m->num_loaded_levels - 1;
	assert (to_lvl >= 0);

	/* expand structure */
	TRY( alloc_tv (m, from_lvl) );

	int num_vertices_of_elem = h5tpriv_ref_elem_get_num_vertices(m);
	int num_edges_of_elem = h5tpriv_ref_elem_get_num_edges(m);

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
			h5_loc_idlist_t* idlist;
			TRY (h5tpriv_enter_te2 (m, face_idx, elem_idx, &idlist));
#if (!NDEBUG)
			if (idlist->num_items > 2) {
				h5_debug ("Error %d edge neighbors %d %d %d",
                                          idlist->num_items, idlist->items[0],
                                          idlist->items[1], idlist->items[2]);
			}


#endif
		}
	}

	//////
	// set flags for all entities

	// traverse all entries in tv
	unsigned int i = 0;
	h5_loc_idlist_t* entry;
	h5_loc_idx_t num_vertices_all_partition = 0;
	while ((entry = h5tpriv_traverse_tv (m, &i))) {
		set_vertex_flags (m, entry, &num_vertices_all_partition);
	}

	// traverse all entries in te
	i = 0;
	h5_loc_idx_t num_edges_all_partition = 0;
	while ((entry = h5tpriv_traverse_te (m, &i))) {
		set_edge_flags (m, entry, &num_edges_all_partition);
	}

	//////
	// create index sets


#if (!defined(NDEBUG) && (__h5_debug_mask & (1<<5)))
	if (!m->is_chunked) {
	h5t_adjacencies_t* adj = &m->adjacencies;
	h5_loc_idx_t idx = 0;
	h5_loc_idx_t num_loc_vertices = m->num_loc_vertices[m->num_loaded_levels-1];
	for (; idx < num_loc_vertices; idx++) {
		h5_debug ("vertex idx: %llu, flags: %llu",
		          (long long unsigned)idx, (long long unsigned)adj->tv.v[idx]->flags);
	}

	h5_loc_idlist_t* list;
	i = 1;
	while ((list = h5tpriv_traverse_te (m, &i))) {
		h5_debug ("edge id: %llx, flags: %llu",
		          (long long unsigned)list->items[0], (long long unsigned)list->flags);
	}
	}
#endif
	H5_RETURN (H5_SUCCESS);
}

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

static inline h5_err_t
release_internal_structs (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	TRY( release_tv (m) );
	TRY( h5priv_hdestroy (&m->adjacencies.te_hash) );
	memset (&m->adjacencies, 0, sizeof (m->adjacencies));
	H5_RETURN (H5_SUCCESS);
}

struct h5t_core_methods h5tpriv_trim_core_methods = {
	update_internal_structs,
	release_internal_structs,
};
