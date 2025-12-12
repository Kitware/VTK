/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "private/h5t_types.h"
#include "private/h5t_err.h"
#include "private/h5t_access.h"
#include "private/h5t_adjacencies.h"
#include "private/h5t_core.h"
#include "private/h5t_map.h"
#include "private/h5t_model.h"
#include "private/h5t_io.h"
#include "private/h5t_store.h"

#include "h5core/h5t_map.h"

static h5_err_t
alloc_loc_elems (
        h5t_mesh_t* const m,
        const size_t cur,
        const size_t new
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, cur=%zu, new=%zu", m, cur, new);

	/* alloc mem for local data of elements */
	TRY (m->loc_elems = h5_alloc (
	             m->loc_elems,
	             new * sizeof (h5_loc_tri_t)));
	memset (
	        (h5_loc_tri_t*)m->loc_elems + cur,
	        -1,
	        (new-cur) * sizeof (h5_loc_tri_t));

	H5_RETURN (H5_SUCCESS);
}

/*
   Bisect edge and return local vertex index of the bisecting point.
 */
static h5_loc_idx_t
bisect_edge (
        h5t_mesh_t* const m,
        const h5_loc_idx_t face_idx,
        const h5_loc_idx_t elem_idx
        ) {
	H5_PRIV_FUNC_ENTER (h5_loc_idx_t,
	                    "m=%p, face_idx=%lld, elem_idx=%lld",
	                    m, (long long)face_idx, (long long)elem_idx);
	h5_loc_idlist_t* retval;
	// get all elements sharing the given edge
	TRY (h5tpriv_find_te2 (m, face_idx, elem_idx, &retval));
	// check weather one of the found elements has been refined
	size_t i;
	for (i = 0; i < retval->num_items; i++) {
//		// check if it is shared boundary edge that has been refined already
//		if (num_b_edges > 0) {
//			h5_glb_idx_t my_glb_idx = m->loc_elems[elem_idx].glb_idx;
//			for (int j = 0; j < num_b_edges; j++) {
//				if (b_edges[j].idx == my_glb_idx &&
//						b_edges[j].face_idx == face_idx) {
//					H5_LEAVE (b_edges[j].vtx);
//				}
//			}
//		}
		h5_loc_id_t kids[2] = {-1,-1};
		TRY (h5tpriv_get_loc_entity_children (m, retval->items[i], kids));
		if (kids[0] >= 0) {
			// element has been refined, return bisecting point
			h5_loc_idx_t edge0[2], edge1[2];
			TRY (h5t_get_loc_vertex_indices_of_edge (m, kids[0], edge0));
			TRY (h5t_get_loc_vertex_indices_of_edge (m, kids[1], edge1));
			if ((edge0[0] == edge1[0]) || (edge0[0] == edge1[1])) {
				H5_LEAVE (edge0[0]);
			} else {
				H5_LEAVE (edge0[1]);
			}
		}
	}
	/*
	   None of the elements has been refined -> add new vertex.
	 */
	h5_loc_idx_t indices[2];
	TRY( h5t_get_loc_vertex_indices_of_edge2 (m, face_idx, elem_idx, indices) );
	h5_float64_t* P0 = m->vertices[indices[0]].P;
	h5_float64_t* P1 = m->vertices[indices[1]].P;
	h5_float64_t P[3];

	P[0] = (P0[0] + P1[0]) / 2.0;
	P[1] = (P0[1] + P1[1]) / 2.0;
	P[2] = (P0[2] + P1[2]) / 2.0;

	H5_RETURN (h5t_store_vertex (m, -1, P));
}

/*
   Please read note about number of new vertices in tetrahedral
   mesh implementation.
 */
static h5_err_t
pre_refine_triangle (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);
	unsigned int num_interior_elems_to_refine = m->marked_entities->num_items;
	TRY (h5t_begin_store_vertices (m, num_interior_elems_to_refine*3 + 64));
	TRY (h5t_begin_store_elems (m, num_interior_elems_to_refine*4));
	H5_RETURN (H5_SUCCESS);
}

/*!
   Refine triangle \c local_eid

   \return Local index of first new triangle or \c H5_ERR
 */
static h5_loc_idx_t
refine_triangle (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	H5_PRIV_FUNC_ENTER (h5_loc_idx_t,
	                    "m=%p, elem_idx=%lld",
	                    m, (long long)elem_idx);
	h5_loc_idx_t vertices[6];       // local vertex indices
	h5_loc_idx_t elem_idx_of_first_child;
	h5_loc_tri_t* el = (h5_loc_tri_t*)m->loc_elems + elem_idx;

	if (el->child_idx >= 0)
		H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Element %lld already refined.",
			(long long)elem_idx);

	vertices[0] = el->vertex_indices[0];
	vertices[1] = el->vertex_indices[1];
	vertices[2] = el->vertex_indices[2];

	vertices[3] = bisect_edge (m, 0, elem_idx);
	vertices[4] = bisect_edge (m, 1, elem_idx);
	vertices[5] = bisect_edge (m, 2, elem_idx);

	h5_loc_idx_t new_elem[3];

	new_elem[0] = vertices[0]; // V[0] < V[3] , V[4]
	new_elem[1] = vertices[3];
	new_elem[2] = vertices[4];
	TRY (elem_idx_of_first_child = h5tpriv_add_cell (m, elem_idx, new_elem, NULL));

	new_elem[0] = vertices[3];  // V[3] < V[1] , V[5]
	new_elem[1] = vertices[1];
	new_elem[2] = vertices[5];
	TRY (h5tpriv_add_cell (m, elem_idx, new_elem, NULL));

	new_elem[0] = vertices[4];  // V[4] < V[5] , V[2]
	new_elem[1] = vertices[5];
	new_elem[2] = vertices[2];
	TRY (h5tpriv_add_cell (m, elem_idx, new_elem, NULL));

	new_elem[0] = vertices[3];  // V[3] < V[4] , V[5]
	new_elem[1] = vertices[5];
	new_elem[2] = vertices[4]; // TODO check if that ordering is correct!
	TRY (h5tpriv_add_cell (m, elem_idx, new_elem, NULL));

	((h5_loc_tri_t*)m->loc_elems)[elem_idx].child_idx = elem_idx_of_first_child;
	m->num_interior_leaf_elems[m->leaf_level]--;

	H5_RETURN (elem_idx_of_first_child);
}

static inline h5_loc_idx_t
compute_neighbor_of_face (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx,
        const h5_loc_idx_t face_idx
        ) {
	H5_PRIV_FUNC_ENTER (h5_loc_idx_t
	                    , "m=%p, elem_idx=%lld, face_idx=%lld",
	                    m, (long long)elem_idx, (long long)face_idx);
	h5_loc_idlist_t* te;
	h5_loc_idx_t neighbor_idx = -2;

	do {
		TRY( h5tpriv_find_te2 (
		             m,
		             face_idx,
		             elem_idx,
		             &te) );
		if (te == NULL) {
			H5_LEAVE (h5_error_internal ());
		}
		if (te->num_items == 1) {
			h5_loc_idx_t old_elem_idx = elem_idx;
			// neighbor is coarser or face is on the boundary
			elem_idx = ((h5_loc_tri_t*)m->loc_elems)[elem_idx].parent_idx;
			if (elem_idx == -1) {
				// we are on the level of the macro grid
				neighbor_idx = -1;
			}
			if (elem_idx < -1) { // this should only happen if we are on the boarder
				// of a loaded chunk and the parent is on a different chunk
				if (__h5_debug_mask >= 6) {
					h5_debug ("Elem %d is on different proc than its parent %d \n"
						"therefore neighborhood idx is not correct resolved", old_elem_idx, elem_idx);
				}
				assert (m->f->myproc != h5priv_find_proc_to_write (m, old_elem_idx));
				H5_LEAVE (~0); //TODO what is a resonable output here?
			}
		} else if (te->num_items == 2) {
			// neighbor has same level of coarsness
			if (h5tpriv_get_elem_idx(te->items[0]) == elem_idx) {
				neighbor_idx = h5tpriv_get_elem_idx (te->items[1]);
			} else {
				neighbor_idx = h5tpriv_get_elem_idx (te->items[0]);
			}

		} else {
			printf ("elem %d face %d num_items %d", elem_idx, face_idx, te->num_items);
			H5_LEAVE (h5_error_internal ());
		}
	} while (neighbor_idx < -1);
	H5_RETURN (neighbor_idx);
}

/*
   Compute neighbors for elements on given level.
 */
static inline h5_err_t
compute_neighbors_of_elems (
        h5t_mesh_t* const m,
        h5_lvl_idx_t level
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p, level=%d", m, level);
	if (level < 0 || level >= m->num_leaf_levels) {
		H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"level idx %lld out of bound, must be in [%lld,%lld]",
			(long long)level,
			(long long)0,
			(long long)m->num_leaf_levels);
	}
	h5_loc_idx_t elem_idx = level == 0 ? 0 : m->num_interior_elems[level-1];
	const h5_loc_idx_t last_idx = m->num_interior_elems[level] - 1;
	h5_loc_tri_t *el = (h5_loc_tri_t*)m->loc_elems + elem_idx;
	while (elem_idx <= last_idx) {
		h5_loc_idx_t face_idx = 0;
		for (; face_idx < 3; face_idx++) {
			el->neighbor_indices[face_idx] =
			        compute_neighbor_of_face (m, elem_idx, face_idx);
		}
		elem_idx++;
		el++;
	}

	H5_RETURN (H5_SUCCESS);
}
/*
 * returns number of newly created triangles when refining a triangle
 */
static int
get_num_new_triangles (
		void
		) {
	return 4;
}
static h5_err_t
end_store_elems (
        h5t_mesh_t* const m
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);

	h5_loc_idx_t start_idx = (m->leaf_level > 0) ? m->num_interior_elems[m->leaf_level-1] : 0;
	h5_loc_idx_t count = m->num_interior_elems[m->leaf_level] - start_idx;

	TRY( h5tpriv_update_internal_structs (m, m->leaf_level) );
	TRY( compute_neighbors_of_elems (m, m->leaf_level) );
	TRY( h5tpriv_init_elem_flags (m, start_idx, count) );
	H5_RETURN (H5_SUCCESS);
}

struct h5t_store_methods h5tpriv_trim_store_methods = {
	alloc_loc_elems,
	pre_refine_triangle,
	refine_triangle,
	get_num_new_triangles,
	end_store_elems,
};
