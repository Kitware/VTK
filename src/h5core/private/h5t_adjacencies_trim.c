/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "private/h5t_types.h"
#include "private/h5t_model.h"
#include "private/h5t_map.h"
#include "private/h5t_access.h"
#include "private/h5t_adjacencies.h"
#include "private/h5t_core.h"

#include <time.h>

#define H5_LOC_ELEM_T h5_loc_tri_t

/*
   Please read the note about "descendants" and "sections" in the
   corresponding file for tetrahedral meshes.
 */
static inline h5_err_t
get_descendant_of_edge (
        h5t_mesh_t* const m,
        h5_loc_id_t entity_id,
        h5_loc_idlist_t** children
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, entity_id=%llu, children=%p",
	                    m,
	                    (long long unsigned)entity_id,
	                    children);
	h5_loc_idlist_t* te;
	TRY( h5tpriv_find_te (m, entity_id, &te ) );
	h5_loc_id_t* edge_idp = te->items;
	h5_loc_id_t* end = te->items + te->num_items;
	do {
		h5_loc_id_t edge_ids[2] = {-1,-1};
		TRY (h5tpriv_get_loc_entity_children (m, *edge_idp, edge_ids));
		if (edge_ids[0] == -1) {
			TRY( h5priv_insert_into_loc_idlist (children, *edge_idp, -1) );
		} else {
			TRY( get_descendant_of_edge (m, edge_ids[0], children) );
			TRY( get_descendant_of_edge (m, edge_ids[1], children) );
		}
	} while (++edge_idp < end);

	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
get_sections_of_edge (
        h5t_mesh_t* const m,
        h5_loc_id_t entity_id,
        h5_loc_idlist_t** children
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, entity_id=%llu, children=%p",
	                    m,
	                    (long long unsigned)entity_id,
	                    children);
	h5_loc_idlist_t* te;
	TRY (h5tpriv_find_te (m, entity_id, &te));
	h5_loc_id_t* edge_idp = te->items;
	h5_loc_id_t *end = te->items+te->num_items;
	int refined = 0;
	do {
		h5_loc_id_t edge_ids[2] = {-1,-1};
		TRY (h5tpriv_get_loc_entity_children (m, *edge_idp, edge_ids));
		if (edge_ids[0] >= 0) {
			refined = 1;
			TRY (get_sections_of_edge (m, edge_ids[0], children));
			TRY (get_sections_of_edge (m, edge_ids[1], children));
		}
	} while (++edge_idp < end);
	if (!refined) {
		TRY (h5priv_insert_into_loc_idlist (children, te->items[0], -1));
	}
	H5_RETURN (H5_SUCCESS);
}

/*
   Add unique ID of vertex given by face and element index to list.
 */
static inline h5_err_t
add_vertex2 (
        h5t_mesh_t* const m,    // in
        h5_loc_idlist_t** list, // out
        h5_loc_idx_t face_idx,  // in
        h5_loc_idx_t elem_idx   // in
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, list=%p, face_idx=%llu, elem_idx=%llu",
	                    m, list,
	                    (long long unsigned)face_idx,
	                    (long long unsigned)elem_idx);
	h5_loc_idlist_t* tv;
	TRY( h5tpriv_find_tv2 (m, face_idx, elem_idx, &tv) );
	TRY( h5priv_search_in_loc_idlist (list, tv->items[0]) );
	H5_RETURN (H5_SUCCESS);
}

/*
   Add unique ID of edge given by ID or face and element index.
 */
static inline h5_err_t
add_edge (
        h5t_mesh_t* const m,    // in
        h5_loc_idlist_t** list, // out
        h5_loc_id_t entity_id   // in
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, list=%p, entity_id=%llu",
	                    m, list,
	                    (long long unsigned)entity_id);
	h5_loc_idlist_t* te;
	TRY( h5tpriv_find_te (m, entity_id, &te) );
	TRY( h5priv_search_in_loc_idlist (list, te->items[0]) );
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
add_edge2 (
        h5t_mesh_t* const m,
        h5_loc_idlist_t** list,
        h5_loc_idx_t face_idx,
        h5_loc_idx_t elem_idx
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, list=%p, face_idx=%llu, elem_idx=%llu",
	                    m, list,
	                    (long long unsigned)face_idx,
	                    (long long unsigned)elem_idx);
	h5_loc_idlist_t *te;
	TRY( h5tpriv_find_te2 (m, face_idx, elem_idx, &te) );
	TRY( h5priv_search_in_loc_idlist (list, te->items[0]) );
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
add_elem2 (
        h5_loc_idlist_t** list, // out
        h5_loc_idx_t elem_idx   // in
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "list=%p, elem_idx=%llu",
	                    list,
	                    (long long unsigned)elem_idx);
	h5_loc_id_t elem_id = h5tpriv_build_triangle_id ((h5_loc_idx_t)0, elem_idx);
	TRY( h5priv_search_in_loc_idlist (list, elem_id) );

	H5_RETURN (H5_SUCCESS);
}

/*
   Get upward adjacent edges to vertex given by ID
 */
static inline h5_err_t
get_edges_uadj_to_vertex (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_idlist_t** list
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, entity_id=%llu, list=%p",
	                    m, (long long unsigned)entity_id, list);
	h5_loc_idx_t vertex_idx;
	TRY( h5tpriv_get_loc_vtx_idx_of_vtx (m, entity_id, &vertex_idx) );
	h5_loc_idlist_t* tv = m->adjacencies.tv.v[vertex_idx];

	TRY( h5priv_alloc_loc_idlist (list, 8) );
	h5_loc_id_t* vertex_idp = tv->items;
	h5_loc_id_t* end = vertex_idp + tv->num_items;
	do {
		h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (*vertex_idp);
		h5_loc_idx_t face_idx = h5tpriv_get_face_idx (*vertex_idp);
		H5_LOC_ELEM_T* elem= ((H5_LOC_ELEM_T*)m->loc_elems) + elem_idx;

		if (!h5tpriv_is_leaf_elem (m, elem)) {
			continue;
		}
		TRY( add_edge2 (m, list,
		                h5tpriv_ref_elem_get_edge_idx (
		                        m, 0, face_idx, 0),
		                elem_idx) );
		TRY( add_edge2 (m, list,
		                h5tpriv_ref_elem_get_edge_idx (
		                        m, 0, face_idx, 1),
		                elem_idx) );
	} while (++vertex_idp < end);
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
get_triangles_uadj_to_vertex (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_idlist_t** list
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, entity_id=%llu, list=%p",
	                    m, (long long unsigned)entity_id, list);
	h5_loc_idx_t vertex_idx;
	TRY( h5tpriv_get_loc_vtx_idx_of_vtx (m, entity_id, &vertex_idx) );
	h5_loc_idlist_t* tv = m->adjacencies.tv.v[vertex_idx];

	TRY ( h5priv_alloc_loc_idlist (list, 8) );
	h5_loc_id_t *vertex_idp = tv->items;
	h5_loc_id_t* end = vertex_idp + tv->num_items;
	do {
		h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (*vertex_idp);
		H5_LOC_ELEM_T* elem = ((H5_LOC_ELEM_T*)m->loc_elems) + elem_idx;

		if (!h5tpriv_is_leaf_elem (m, elem)) {
			continue;
		}
		TRY (add_elem2 (list, elem_idx));
	} while (++vertex_idp < end);
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
get_triangles_uadj_to_edge (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_idlist_t** list
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, entity_id=%llu, list=%p",
	                    m, (long long unsigned)entity_id, list);
	h5_loc_idlist_t* children;
	TRY( h5priv_alloc_loc_idlist (&children, 8) );
	TRY( get_descendant_of_edge (m, entity_id, &children) );
	TRY( h5priv_alloc_loc_idlist (list, 8) );
	h5_loc_id_t *edge_idp = children->items;
	h5_loc_id_t *end = children->items+children->num_items;
	do {
		h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (*edge_idp);
		TRY (add_elem2 (list, elem_idx));
	} while (++edge_idp < end);
	TRY( h5priv_free_loc_idlist (&children) );

	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
get_edges_adj_to_edge (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_idlist_t** list
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, entity_id=%llu, list=%p",
	                    m, (long long unsigned)entity_id, list);

	TRY( h5priv_alloc_loc_idlist (list, 8) );
	h5_loc_idlist_t* children;
	TRY( h5priv_alloc_loc_idlist (&children, 8) );
	TRY( get_sections_of_edge (m, entity_id, &children) );
	TRY( h5priv_alloc_loc_idlist (list, 8) );
	h5_loc_id_t* edge_idp = children->items;
	h5_loc_id_t* end = children->items+children->num_items;
	do {
		TRY( add_edge (m, list, *edge_idp) );
	} while (++edge_idp < end);
	TRY( h5priv_free_loc_idlist(&children) );
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
get_vertices_dadj_to_edge (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_idlist_t** list
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, entity_id=%llu, list=%p",
	                    m, (long long unsigned)entity_id, list);
	h5_loc_idlist_t* children;
	TRY( h5priv_alloc_loc_idlist (&children, 8) );
	TRY( get_sections_of_edge (m, entity_id, &children) );
	TRY( h5priv_alloc_loc_idlist (list, 8) );
	h5_loc_id_t* edge_idp = children->items;
	h5_loc_id_t* end = edge_idp + children->num_items;
	do {
		h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (*edge_idp);
		h5_loc_idx_t face_idx = h5tpriv_get_face_idx (*edge_idp);

		TRY( add_vertex2 (m, list,
		                  h5tpriv_ref_elem_get_vertex_idx (
		                          m, 1, face_idx, 0),
		                  elem_idx) );
		TRY( add_vertex2 (m, list,
		                  h5tpriv_ref_elem_get_vertex_idx (
		                          m, 1, face_idx, 1),
		                  elem_idx) );
	} while (++edge_idp < end);
	TRY( h5priv_free_loc_idlist (&children) );
	H5_RETURN (H5_SUCCESS);
}

/*
   Compute downward adjacent vertices of all edges of triangle.
 */
static inline h5_err_t
get_vertices_dadj_to_triangle (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_idlist_t** list
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, entity_id=%llu, list=%p",
	                    m, (long long unsigned)entity_id, list);
	h5_loc_idlist_t* children;
	TRY (h5priv_alloc_loc_idlist (&children, 8));

	h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx ( entity_id );
	// loop over all edges of triangle
	h5_loc_idx_t face_idx;
	h5_loc_idx_t num_faces = h5tpriv_ref_elem_get_num_edges (m);
	for (face_idx = 0; face_idx < num_faces; face_idx++) {
		TRY( get_sections_of_edge (
		             m,
		             h5tpriv_build_edge_id (face_idx, elem_idx),
		             &children) );
	}
	TRY (h5priv_alloc_loc_idlist (list, 8));
	h5_loc_id_t *edge_idp = children->items;
	h5_loc_id_t *end = edge_idp + children->num_items;
	do {
		elem_idx = h5tpriv_get_elem_idx (*edge_idp);
		face_idx = h5tpriv_get_face_idx (*edge_idp);

		TRY( add_vertex2 (m, list,
		                  h5tpriv_ref_elem_get_vertex_idx (
		                          m, 1, face_idx, 0),
		                  elem_idx) );
		TRY( add_vertex2 (m, list,
		                  h5tpriv_ref_elem_get_vertex_idx (
		                          m, 1, face_idx, 1),
		                  elem_idx) );
	} while (++edge_idp < end);
	TRY (h5priv_free_loc_idlist(&children));
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
get_edges_dadj_to_triangle (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_idlist_t** list
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, entity_id=%llu, list=%p",
	                    m, (long long unsigned)entity_id, list);
	h5_loc_idlist_t* children;
	TRY (h5priv_alloc_loc_idlist (&children, 8));
	h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (entity_id);
	// loop over all edges of triangle
	h5_loc_idx_t edge_idx = h5tpriv_ref_elem_get_num_edges (m);
	while (--edge_idx >= 0) {
		TRY( get_sections_of_edge (
		             m,
		             h5tpriv_build_edge_id (edge_idx, elem_idx),
		             &children) );
	}
	TRY (h5priv_alloc_loc_idlist (list, 8));
	h5_loc_id_t* edge_idp = children->items;
	h5_loc_id_t* end = edge_idp + children->num_items;
	do {
		TRY( add_edge (m, list, *edge_idp) );
	} while (++edge_idp < end);
	TRY (h5priv_free_loc_idlist(&children));
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
dim_error(
        const h5_int32_t dim
        ) {
	return h5_error (
	               H5_ERR_INVAL,
	               "Illegal dimension %ld", (long)dim);
}

static inline h5_err_t
get_adjacencies_to_vertex (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        const h5_int32_t dim,
        h5_loc_idlist_t** list
        ) {
	switch (dim) {
	case 1:
		return get_edges_uadj_to_vertex(m, entity_id, list);
	case 2:
		return get_triangles_uadj_to_vertex(m, entity_id, list);
	default:
		return dim_error (dim);
	}
}

static inline h5_err_t
get_adjacencies_to_edge (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        const h5_int32_t dim,
        h5_loc_idlist_t** list
        ) {
	switch (dim) {
	case 0:
		return get_vertices_dadj_to_edge(m, entity_id, list);
	case 1:
		return get_edges_adj_to_edge(m, entity_id, list);
	case 2:
		return get_triangles_uadj_to_edge(m, entity_id, list);
	default:
		return dim_error (dim);
	}
}

static inline h5_err_t
get_adjacencies_to_triangle (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        const h5_int32_t dim,
        h5_loc_idlist_t** list
        ) {
	switch (dim) {
	case 0:
		return get_vertices_dadj_to_triangle(m, entity_id, list);
	case 1:
		return get_edges_dadj_to_triangle(m, entity_id, list);
	default:
		return dim_error (dim);
	}
}

static h5_err_t
get_adjacencies (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        const h5_int32_t dim,
        h5_loc_idlist_t** list
        ) {
	h5_loc_id_t entity_type = h5tpriv_get_entity_type (entity_id);
	switch (entity_type) {
	case H5T_TYPE_VERTEX:
		return get_adjacencies_to_vertex (m, entity_id, dim, list);
	case H5T_TYPE_EDGE:
		return get_adjacencies_to_edge (m, entity_id, dim, list);
	case H5T_TYPE_TRIANGLE:
		return get_adjacencies_to_triangle (m, entity_id, dim, list);
	default:
		break;
	}
	return h5_error_internal ();
}

struct h5t_adjacency_methods h5tpriv_trim_adjacency_methods = {
	get_adjacencies
};

