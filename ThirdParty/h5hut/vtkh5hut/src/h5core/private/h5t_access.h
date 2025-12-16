/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5T_ACCESS_H
#define __PRIVATE_H5T_ACCESS_H

#include "h5core/h5_types.h"
#include "private/h5t_types.h"

struct h5t_access_methods {
#if defined(WITH_PARALLEL_H5GRID)
	MPI_Datatype (*get_mpi_type_of_glb_elem)(
	        h5t_mesh_t* const m);
#endif
	h5_loc_elem_t* (*get_loc_elem)(
	        h5t_mesh_t* const, const h5_loc_idx_t);
	h5_glb_idx_t (*get_loc_elem_glb_idx)(
	        h5t_mesh_t* const, const h5_loc_idx_t);
	h5_glb_idx_t (*set_loc_elem_glb_idx)(
	        h5t_mesh_t* const, const h5_loc_idx_t, const h5_glb_idx_t);
	h5_loc_idx_t (*get_loc_elem_parent_idx)(
	        h5t_mesh_t* const, const h5_loc_idx_t);
	h5_loc_idx_t (*set_loc_elem_parent_idx)(
	        h5t_mesh_t* const, const h5_loc_idx_t, const h5_loc_idx_t);
	h5_loc_id_t (*get_loc_elem_child_idx)(
	        h5t_mesh_t* const, const h5_loc_idx_t);
	h5_loc_id_t (*set_loc_elem_child_idx)(
	        h5t_mesh_t* const, const h5_loc_idx_t, const h5_loc_idx_t);
	h5_lvl_idx_t (*get_loc_elem_level_idx)(
	        h5t_mesh_t* const, const h5_loc_idx_t);
	h5_lvl_idx_t (*set_loc_elem_level_idx)(
	        h5t_mesh_t* const, const h5_loc_idx_t, const h5_lvl_idx_t);
	h5_loc_idx_t* (*get_loc_elem_vertex_indices)(
	        h5t_mesh_t* const, const h5_loc_idx_t);
	h5_loc_idx_t* (*get_loc_elem_vertex_indices_of_array)(
	        h5t_mesh_t* const, const h5_loc_idx_t, const h5_loc_elem_t* loc_elems);
	h5_loc_idx_t (*get_loc_elem_vertex_idx)(
	        h5t_mesh_t* const, const h5_loc_idx_t, const h5_loc_idx_t);
	h5_loc_idx_t (*set_loc_elem_vertex_idx)(
	        h5t_mesh_t* const, const h5_loc_idx_t, const h5_loc_idx_t, const h5_loc_idx_t);
	h5_loc_idx_t* (*get_loc_elem_neighbor_indices)(
	        h5t_mesh_t* const, const h5_loc_idx_t);
	h5_loc_idx_t (*get_loc_elem_neighbor_idx)(
	        h5t_mesh_t* const, const h5_loc_idx_t, const h5_loc_idx_t);
	h5_loc_idx_t (*set_loc_elem_neighbor_idx)(
	        h5t_mesh_t* const, const h5_loc_idx_t, const h5_loc_idx_t, const h5_loc_idx_t);
	h5_loc_id_t (*get_loc_entity_parent)(
	        h5t_mesh_t* const, h5_loc_id_t);
	h5_err_t (*get_loc_entity_children)(
	        h5t_mesh_t* const, const h5_loc_id_t, h5_loc_id_t*);

	h5_glb_elem_t* (*alloc_glb_elems)(h5t_mesh_t* const, const size_t);
	h5_glb_elem_t* (*get_glb_elem)(h5_glb_elem_t*, const h5_loc_idx_t);
	h5_glb_elem_t* (*copy_glb_elems)(h5_glb_elem_t*, h5_loc_idx_t,
	                                 h5_glb_elem_t*, h5_loc_idx_t,
	                                 size_t);
	h5_err_t (*sort_glb_elems)(h5_glb_elem_t*, size_t);
	h5_glb_idx_t (*get_glb_elem_idx)(h5_glb_elem_t* const, const h5_loc_idx_t);
	h5_lvl_idx_t (*get_glb_elem_level)(h5_glb_elem_t* const, const h5_loc_idx_t);
	h5_glb_idx_t* (*get_glb_elem_vertices)(h5_glb_elem_t* const, const h5_loc_idx_t);
	h5_glb_idx_t* (*get_glb_elem_neighbors)(h5_glb_elem_t* const, const h5_loc_idx_t);

	h5_err_t (*set_geom_boundary_elem_flag)(h5t_mesh_t* const, const h5_loc_idx_t);
	int (*is_geom_boundary_elem)(h5t_mesh_t* const, const h5_loc_idx_t);
	int (*is_boundary_facet)(h5t_mesh_t* const, const h5_loc_idx_t, const h5_loc_idx_t);
	int (*is_boundary_face)(h5t_mesh_t* const, const int, const h5_loc_idx_t, const h5_loc_idx_t);
};

extern struct h5t_access_methods h5tpriv_access_trim_methods;
extern struct h5t_access_methods h5tpriv_access_tetm_methods;

#if defined(WITH_PARALLEL_H5GRID)
static inline MPI_Datatype
h5tpriv_get_mpi_type_of_glb_elem (
        h5t_mesh_t* const m
        ) {
	return m->methods->access->get_mpi_type_of_glb_elem (m);
}
#endif

static inline h5_loc_elem_t*
h5tpriv_get_loc_elem (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return m->methods->access->get_loc_elem(m, elem_idx);
}

static inline h5_glb_idx_t
h5tpriv_set_loc_elem_glb_idx (
        h5t_mesh_t* const m,
        h5_loc_idx_t loc_elem_idx,
        h5_glb_idx_t glb_elem_idx
        ) {
	return m->methods->access->set_loc_elem_glb_idx(m, loc_elem_idx, glb_elem_idx);
}

static inline h5_glb_idx_t
h5tpriv_get_loc_elem_glb_idx (
        h5t_mesh_t* const m,
        h5_loc_idx_t loc_elem_idx
        ) {
	return m->methods->access->get_loc_elem_glb_idx(m, loc_elem_idx);
}

static inline h5_loc_idx_t
h5tpriv_get_loc_elem_parent_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return m->methods->access->get_loc_elem_parent_idx(m, elem_idx);
}

static inline h5_loc_idx_t
h5tpriv_set_loc_elem_parent_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t parent_idx
        ) {
	return m->methods->access->set_loc_elem_parent_idx(m, elem_idx, parent_idx);
}

static inline h5_loc_idx_t
h5tpriv_get_loc_elem_child_idx (
        h5t_mesh_t* const m,
        const h5_loc_idx_t elem_idx
        ) {
	return m->methods->access->get_loc_elem_child_idx(m, elem_idx);
}

static inline h5_loc_idx_t
h5tpriv_set_loc_elem_child_idx (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx,
        h5_loc_idx_t child_idx) {
	return m->methods->access->set_loc_elem_child_idx (m, elem_idx, child_idx);
}

static inline h5_lvl_idx_t
h5tpriv_get_loc_elem_level_idx (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx
        ) {
	return m->methods->access->get_loc_elem_level_idx (m, elem_idx);
}

static inline h5_lvl_idx_t
h5tpriv_set_loc_elem_level_idx (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx,
        h5_lvl_idx_t lvl_idx
        ) {
	return m->methods->access->set_loc_elem_level_idx (m, elem_idx, lvl_idx);
}

static inline h5_loc_idx_t*
h5tpriv_get_loc_elem_vertex_indices (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx
        ) {
	return m->methods->access->get_loc_elem_vertex_indices (m, elem_idx);
}

static inline h5_loc_idx_t*
h5tpriv_get_loc_elem_vertex_indices_of_array (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx,
        h5_loc_elem_t* loc_elems
        ) {
	return m->methods->access->get_loc_elem_vertex_indices_of_array (m, elem_idx, loc_elems);
}

static inline h5_loc_idx_t
h5tpriv_get_loc_elem_vertex_idx (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx,
        h5_loc_idx_t face_idx
        ) {
	return m->methods->access->get_loc_elem_vertex_idx(m, elem_idx, face_idx);
}

static inline h5_loc_idx_t
h5tpriv_set_loc_elem_vertex_idx (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx,
        h5_loc_idx_t face_idx,
        h5_loc_idx_t vertex_idx
        ) {
	return m->methods->access->set_loc_elem_vertex_idx(m, elem_idx, face_idx, vertex_idx);
}

static inline h5_loc_idx_t*
h5tpriv_get_loc_elem_neighbor_indices (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx
        ) {
	return m->methods->access->get_loc_elem_neighbor_indices(m, elem_idx);
}

static inline h5_loc_idx_t
h5tpriv_get_loc_elem_neighbor_idx (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx,
        h5_loc_idx_t face_idx
        ) {
	return m->methods->access->get_loc_elem_neighbor_idx(m, elem_idx, face_idx);
}

static inline h5_loc_idx_t
h5tpriv_set_loc_elem_neighbor_idx (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx,
        h5_loc_idx_t face_idx,
        h5_loc_idx_t idx
        ) {
	return m->methods->access->set_loc_elem_neighbor_idx(m, elem_idx, face_idx, idx);
}

static inline h5_loc_id_t
h5tpriv_get_loc_entity_parent (
        h5t_mesh_t* const m,
        h5_loc_id_t entity_id
        ) {
	return m->methods->access->get_loc_entity_parent (m, entity_id);
}

static inline h5_err_t
h5tpriv_get_loc_entity_children (
        h5t_mesh_t* const m,
        const h5_loc_id_t elem_id,
        h5_loc_id_t* const children
        ) {
	return m->methods->access->get_loc_entity_children (m, elem_id, children);
}

static inline h5_glb_elem_t*
h5tpriv_alloc_glb_elems (
        h5t_mesh_t* const m, const size_t n
        ) {
	return m->methods->access->alloc_glb_elems(m, n);
}

static inline h5_glb_elem_t*
h5tpriv_get_glb_elem (
        h5t_mesh_t* m,
        h5_glb_elem_t* const elems,
        h5_loc_idx_t idx
        ) {
	return m->methods->access->get_glb_elem (elems, idx);
}

static inline h5_glb_elem_t*
h5tpriv_copy_glb_elems (
        h5t_mesh_t* const m,
        h5_glb_elem_t* dstbuf,
        h5_loc_idx_t dstidx,
        h5_glb_elem_t* srcbuf,
        h5_loc_idx_t srcidx,
        size_t count
        ) {
	return m->methods->access->copy_glb_elems (dstbuf, dstidx, srcbuf, srcidx, count);
}

static inline h5_err_t
h5tpriv_sort_glb_elems (
        h5t_mesh_t* const m,
        h5_glb_elem_t* elems,
        size_t count
        ) {
	return m->methods->access->sort_glb_elems (elems, count);
}

static inline h5_glb_idx_t
h5tpriv_get_glb_elem_idx (
        h5t_mesh_t* const m,
        h5_glb_elem_t* const elems,
        const h5_loc_idx_t idx
        ) {
	return m->methods->access->get_glb_elem_idx (elems, idx);
}

static inline h5_lvl_idx_t
h5tpriv_get_glb_elem_level(
        h5t_mesh_t* const m,
        h5_glb_elem_t* const elems,
        h5_loc_idx_t elem_idx
        ) {
	return m->methods->access->get_glb_elem_level (elems, elem_idx);
}

static inline h5_glb_idx_t*
h5tpriv_get_glb_elem_vertices(
        h5t_mesh_t* const m,
        h5_glb_elem_t* const elems,
        h5_loc_idx_t elem_idx
        ) {
	return m->methods->access->get_glb_elem_vertices (elems, elem_idx);
}

static inline h5_glb_idx_t*
h5tpriv_get_glb_elem_neighbors(
        h5t_mesh_t* const m,
        h5_glb_elem_t* const elems,
        h5_loc_idx_t elem_idx
        ) {
	return m->methods->access->get_glb_elem_neighbors (elems, elem_idx);
}


static inline h5_err_t
h5tpriv_set_geom_boundary_elem_flag (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx
        ) {
	return m->methods->access->set_geom_boundary_elem_flag (m, elem_idx);
}

static inline int
h5tpriv_is_geom_boundary_elem (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx
        ) {
	return m->methods->access->is_geom_boundary_elem (m, elem_idx);
}

static inline int
h5tpriv_is_boundary_facet (
        h5t_mesh_t* const m,
        h5_loc_idx_t elem_idx,
        h5_loc_idx_t facet_idx
        ) {
	return m->methods->access->is_boundary_facet (m, elem_idx, facet_idx);
}

static inline int
h5tpriv_is_boundary_face (
        h5t_mesh_t* const m,
        const int dim,
        const h5_loc_idx_t elem_idx,
        const h5_loc_idx_t facet_idx
        ) {
	return m->methods->access->is_boundary_face (m, dim, elem_idx, facet_idx);
}


#endif
