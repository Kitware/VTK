/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5T_REF_ELEMENTS_H
#define __PRIVATE_H5T_REF_ELEMENTS_H

#include "h5core/h5_types.h"

#define H5T_MAX_DIM 3
#define H5T_MAX_FACES 6		// edges in tetrahedron
#define H5T_MAX_VERTICES 4	// tetrahedron

struct h5t_ref_elem {
	int dim;
	int entity_types[H5T_MAX_DIM+1];
	int num_faces[H5T_MAX_DIM+1];
	int num_vertices_of_face[H5T_MAX_DIM+1][H5T_MAX_FACES];
	int connect_count[H5T_MAX_DIM+1][H5T_MAX_DIM+1][H5T_MAX_FACES];
	int connect[H5T_MAX_DIM+1][H5T_MAX_DIM+1][H5T_MAX_FACES][H5T_MAX_FACES];
	h5_float64_t coords[H5T_MAX_VERTICES][H5T_MAX_DIM];
};

typedef struct h5t_ref_elem h5t_ref_elem_t;

extern const h5t_ref_elem_t h5t_tet_ref_elem;
extern const h5t_ref_elem_t h5t_tri_ref_elem;

#define h5tpriv_ref_elem_get_num_vertices(this) (this->ref_elem->num_faces[0])

#define h5tpriv_ref_elem_get_num_edges(this) (this->ref_elem->num_faces[1])

#define h5tpriv_ref_elem_get_num_facets(this)                   \
        (this->ref_elem->num_faces[this->ref_elem->dim - 1])

#define h5tpriv_ref_elem_get_num_faces(this, dim) (this->ref_elem->num_faces[dim])

#define h5tpriv_ref_elem_get_dim(this) (this->ref_elem->dim)

#define h5tpriv_ref_elem_get_entity_type(this,dim)      \
        (this->ref_elem->entity_types[dim])

#define h5tpriv_ref_elem_get_vertex_idx(this, dim, face_idx, i) \
        (h5_loc_idx_t)(this->ref_elem->connect[dim][0][face_idx][i])

#define h5tpriv_ref_elem_get_edge_idx(this, dim, face_idx, i)   \
        (h5_loc_idx_t)(this->ref_elem->connect[dim][1][face_idx][i])

#define h5tpriv_ref_elem_get_triangle_idx(this, dim, face_idx, i)       \
        (h5_loc_idx_t)(this->ref_elem->connect[dim][2][face_idx][i])

#define h5tpriv_ref_elem_get_num_facets_to_vertex(this, i)      \
        ((h5_loc_idx_t)this->ref_elem->connect_count[this->ref_elem->dim-1][0][i])

#define h5tpriv_ref_elem_get_num_facets_to_edge(this, i)        \
        ((h5_loc_idx_t)this->ref_elem->connect_count[this->ref_elem->dim-1][1][i])

#define h5tpriv_ref_elem_get_facet_to_vertex(this, i, j)                \
        ((h5_loc_idx_t)this->ref_elem->connect[this->ref_elem->dim-1][0][i][j])

#define h5tpriv_ref_elem_get_facet_to_edge(this, i, j)          \
        ((h5_loc_idx_t)this->ref_elem->connect[this->ref_elem->dim-1][1][i][j])

#endif
