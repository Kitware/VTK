/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "h5core/h5_log.h"
#include "h5core/h5_syscall.h"

#include "private/h5_file.h"
#include "private/h5t_types.h"
#include "private/h5t_model.h"
#include "private/h5t_access.h"

#include <string.h>
#include <stdlib.h>

#if defined(WITH_PARALLEL_H5GRID)
static MPI_Datatype
get_mpi_type_of_glb_elem (
        h5t_mesh_t* const m
        ) {
	return h5_dta_types.mpi_glb_triangle;
}
#endif

/*** op's on local elements ***/
static h5_loc_elem_t*
get_loc_elem (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return (h5_loc_elem_t*)(((h5_loc_tri_t*)m->loc_elems) + elem_idx);
}

static h5_glb_idx_t
get_loc_elem_glb_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_loc_tri_t*)m->loc_elems)[elem_idx].glb_idx;
}

static h5_glb_idx_t
set_loc_elem_glb_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_glb_idx_t glb_idx
        ) {
	((h5_loc_tri_t*)m->loc_elems)[elem_idx].glb_idx = glb_idx;
	return glb_idx;
}

static h5_loc_idx_t
get_loc_elem_parent_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_loc_tri_t*)m->loc_elems)[elem_idx].parent_idx;
}

static h5_loc_idx_t
set_loc_elem_parent_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t parent_idx
        ) {
	((h5_loc_tri_t*)m->loc_elems)[elem_idx].parent_idx = parent_idx;
	return parent_idx;
}

static h5_loc_idx_t
get_loc_elem_child_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_loc_tri_t*)m->loc_elems)[elem_idx].child_idx;
}

static h5_loc_idx_t
set_loc_elem_child_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t child_idx
        ) {
	((h5_loc_tri_t*)m->loc_elems)[elem_idx].child_idx = child_idx;
	return child_idx;
}

static h5_lvl_idx_t
get_loc_elem_level_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_loc_tri_t*)m->loc_elems)[elem_idx].level_idx;
}

static h5_lvl_idx_t
set_loc_elem_level_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_lvl_idx_t level_idx
        ) {
	((h5_loc_tri_t*)m->loc_elems)[elem_idx].level_idx = level_idx;
	return level_idx;
}

static h5_loc_idx_t*
get_loc_elem_vertex_indices (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_loc_tri_t*)m->loc_elems)[elem_idx].vertex_indices;
}

static h5_loc_idx_t*
get_loc_elem_vertex_indices_of_array (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_elem_t* loc_elems
        ) {
	return ((h5_loc_tri_t*)loc_elems)[elem_idx].vertex_indices;
}

static h5_loc_idx_t
get_loc_elem_vertex_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t face_idx
        ) {
	return ((h5_loc_tri_t*)m->loc_elems)[elem_idx].vertex_indices[face_idx];
}

static h5_loc_idx_t
set_loc_elem_vertex_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t face_idx,
        const h5_loc_idx_t vertex_idx
        ) {
	((h5_loc_tri_t*)m->loc_elems)[elem_idx].vertex_indices[face_idx] = vertex_idx;
	return vertex_idx;
}

static h5_loc_idx_t*
get_loc_elem_neighbor_indices (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_loc_tri_t*)m->loc_elems)[elem_idx].neighbor_indices;
}

static h5_loc_idx_t
get_loc_elem_neighbor_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t face_idx
        ) {
	return ((h5_loc_tri_t*)m->loc_elems)[elem_idx].neighbor_indices[face_idx];
}

static h5_loc_idx_t
set_loc_elem_neighbor_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t face_idx,
        const h5_loc_idx_t neighbor_idx
        ) {
	((h5_loc_tri_t*)m->loc_elems)[elem_idx].neighbor_indices[face_idx] = neighbor_idx;
	return neighbor_idx;
}


/*** op's on global elements ***/
static h5_glb_elem_t*
alloc_glb_elems (
        h5t_mesh_t* const m, const size_t size
        ) {
	H5_PRIV_FUNC_ENTER (h5_glb_elem_p, "m=%p, size=%zu", m, size);
	h5_glb_elem_p buf;
	TRY (buf = h5_calloc (size, sizeof(h5_glb_tri_t)));
	H5_RETURN (buf);
}

static h5_glb_elem_t*
get_glb_elem (
        h5_glb_elem_t* __elems,
        const h5_loc_idx_t idx
        ) {
	h5_glb_tri_t* elems = (h5_glb_tri_t*)__elems;
	return (h5_glb_elem_t*)(elems+idx);
}

static h5_glb_elem_t*
copy_glb_elems (
        h5_glb_elem_t* dstbuf,
        h5_loc_idx_t dstidx,
        h5_glb_elem_t* srcbuf,
        h5_loc_idx_t srcidx,
        size_t count
        ) {
	h5_glb_tri_t* dst = (h5_glb_tri_t*)dstbuf + dstidx;
	h5_glb_tri_t* src = (h5_glb_tri_t*)srcbuf + srcidx;
	memcpy (dst, src, count*sizeof(*src));
	return (h5_glb_elem_t*)dst;
}

static int
compare_glb_elems(const void *p_a, const void *p_b) {
	return ((h5_glb_tri_t*) p_a)->idx - ((h5_glb_tri_t*) p_b)->idx;
}


static h5_err_t
sort_glb_elems (
        h5_glb_elem_t* elems,
        size_t count
        ) {
	h5_glb_tri_t* tri = (h5_glb_tri_t*) elems;
	qsort(tri, count, sizeof (*tri), compare_glb_elems);
	return H5_SUCCESS;
}

static h5_glb_idx_t
get_glb_elem_idx (
        h5_glb_elem_t* const elems,
        const h5_loc_idx_t elem_idx
        ) {
	h5_glb_tri_t* tris = (h5_glb_tri_t*)elems;
	return tris[elem_idx].idx;
}

static h5_lvl_idx_t
get_glb_elem_level (
        h5_glb_elem_t* const elems,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_glb_tri_t*)elems)[elem_idx].level_idx;
}

static h5_glb_idx_t*
get_glb_elem_vertices (
        h5_glb_elem_t* const elems,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_glb_tri_t*)elems)[elem_idx].vertex_indices;
}

static h5_glb_idx_t*
get_glb_elem_neighbors (
        h5_glb_elem_t* const elems,
        const h5_loc_idx_t elem_idx
        ) {
	return ((h5_glb_tri_t*)elems)[elem_idx].neighbor_indices;
}

static h5_err_t
set_geom_boundary_elem_flag (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx
        ) {
	((h5_loc_tri_t*)m->loc_elems)[elem_idx].flags |= H5_GEOBORDER_ENTITY;
	return H5_SUCCESS;
}

static int
is_geom_boundary_elem  (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return (((h5_loc_tri_t*)m->loc_elems)[elem_idx].flags & H5_GEOBORDER_ENTITY) ? 1 : 0;
}

static int
is_boundary_facet (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t facet_idx
        ) {
	return (((h5_loc_tri_t*)m->loc_elems)[elem_idx].neighbor_indices[facet_idx] == -1);
}

static int
is_boundary_face (
        h5t_mesh_t* const m,
        const int dim,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t facet_idx
        ) {
	UNUSED_ARGUMENT (m);
	UNUSED_ARGUMENT (dim);
	UNUSED_ARGUMENT (elem_idx);
	UNUSED_ARGUMENT (facet_idx);
	return h5_error_internal ();
}

static h5_loc_id_t
get_loc_entity_parent (
        h5t_mesh_t* const m,
        h5_loc_id_t entity_id
        ) {
	// TODO
	return h5_error_not_implemented ();
}

static h5_err_t
get_children_of_loc_elem (
        h5t_mesh_t* const m,
        h5_loc_idx_t face_idx,          // in
        h5_loc_idx_t elem_idx,          // in
        h5_loc_id_t* children           // out
        ) {
	if (face_idx != 0) {
		return h5_error_internal ();
	}
	h5_loc_idx_t idx = ((h5_loc_tri_t*)m->loc_elems)[elem_idx].child_idx;
	children[0] = h5tpriv_build_tet_id ((h5_loc_idx_t)0, idx++);
	children[1] = h5tpriv_build_tet_id ((h5_loc_idx_t)0, idx++);
	children[2] = h5tpriv_build_tet_id ((h5_loc_idx_t)0, idx++);
	children[3] = h5tpriv_build_tet_id ((h5_loc_idx_t)0, idx++);

	return H5_SUCCESS;
}


static h5_err_t
get_children_of_loc_edge (
        h5t_mesh_t* const m,
        const h5_loc_idx_t face_idx,
        const h5_loc_idx_t elem_idx,
        h5_loc_id_t* children
        ) {
	/*
	   Please note: The face index of the children and the father is
	   always the same. The only think we have to know, is the offset
	   to the element index of the first child. This is either 0, 1 or
	   2. The third child is an "inner" child which doesn't superpose edges
	   of the parent.

	   The direct children of edge 0 of an element are edge 0 of the
	   first child and edge 0 of the second child, giving the offset 0
	   and 1 for this edge.
	 */
	int off[3][2] = { {0,1}, // edge 0
			  {0,2}, // edge 1
			  {1,2}  // edge 2
	};
	h5_loc_idx_t num_faces = h5tpriv_ref_elem_get_num_edges (m);
	if ((face_idx < 0) || (face_idx >= num_faces)) {
		return h5_error_internal ();
	}
	h5_loc_idx_t idx = ((h5_loc_tri_t*)m->loc_elems)[elem_idx].child_idx;
	children[0] = h5tpriv_build_edge_id (face_idx, idx+off[face_idx][0]);
	children[1] = h5tpriv_build_edge_id (face_idx, idx+off[face_idx][1]);
	return H5_SUCCESS;
}

static h5_err_t
get_loc_entity_children (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_id_t* const children
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "m=%p, entity_id=%llx, children=%p",
	                    m, (long long)entity_id, children);

	const h5_loc_id_t type_id = h5tpriv_get_entity_type (entity_id);
	const h5_loc_idx_t face_idx = h5tpriv_get_face_idx (entity_id);
	const h5_loc_idx_t elem_idx = h5tpriv_get_elem_idx (entity_id);

	if (h5tpriv_is_leaf_elem (m, &((h5_loc_tri_t*)m->loc_elems)[elem_idx])) {
		H5_LEAVE (H5_NOK);            // not refined
	}
	switch (type_id) {
	case H5T_TYPE_TRIANGLE: {
		H5_LEAVE (
		        get_children_of_loc_elem (m, face_idx, elem_idx, children));
		break;
	}
	case H5T_TYPE_EDGE: {
		H5_LEAVE (
		        get_children_of_loc_edge (m, face_idx, elem_idx, children));
	}
	}
	H5_RETURN (h5_error_internal ());
}

struct h5t_access_methods h5tpriv_access_trim_methods = {
#if defined(WITH_PARALLEL_H5GRID)
	get_mpi_type_of_glb_elem,
#endif
	get_loc_elem,
	get_loc_elem_glb_idx,
	set_loc_elem_glb_idx,
	get_loc_elem_parent_idx,
	set_loc_elem_parent_idx,
	get_loc_elem_child_idx,
	set_loc_elem_child_idx,
	get_loc_elem_level_idx,
	set_loc_elem_level_idx,
	get_loc_elem_vertex_indices,
	get_loc_elem_vertex_indices_of_array,
	get_loc_elem_vertex_idx,
	set_loc_elem_vertex_idx,
	get_loc_elem_neighbor_indices,
	get_loc_elem_neighbor_idx,
	set_loc_elem_neighbor_idx,
	get_loc_entity_parent,
	get_loc_entity_children,
	alloc_glb_elems,
	get_glb_elem,
	copy_glb_elems,
	sort_glb_elems,
	get_glb_elem_idx,
	get_glb_elem_level,
	get_glb_elem_vertices,
	get_glb_elem_neighbors,
	set_geom_boundary_elem_flag,
	is_geom_boundary_elem,
	is_boundary_facet,
	is_boundary_face,
};
