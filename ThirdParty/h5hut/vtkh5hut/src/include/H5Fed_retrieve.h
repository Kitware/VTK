/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5FED_RETRIEVE_H
#define __H5FED_RETRIEVE_H

#include "h5core/h5_types.h"
#include "h5core/h5_log.h"
#include "h5core/h5_syscall.h"
#include "h5core/h5t_map.h"
#include "h5core/h5t_retrieve.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
   Begin traverse over all entities on this compute node.
   Initialize internal data structures.

   \remark
   Entities might be on processor boundaries! Therefore the same entity might be
   processed on several compute nodes.

   \return	H5_SUCCESS or error code
 */
static inline h5t_iterator_p
H5FedBeginTraverseEntities (
        h5t_mesh_t* const m,	///< mesh object
        const int codim		///< co-dimension of entity to traverse
        ) {
	H5_API_ENTER (h5t_iterator_p, "m=%p, codim=%d", m, codim);
	h5t_iterator_p iter;
	TRY (iter = (h5t_iterator_p)h5_calloc (1, sizeof (h5t_leaf_iterator_t)));
	TRY (h5t_init_leaf_iterator (iter, m, codim));
	H5_API_RETURN (iter);
}

static inline h5t_iterator_p
H5FedBeginTraverseBoundaryFaces (
        h5t_mesh_t* const m,
        const int codim
        ) {
	H5_API_ENTER (h5t_iterator_p, "m=%p, codim=%d", m, codim);
	h5t_iterator_p iter;
	TRY (iter = (h5t_iterator_p)h5_calloc (1, sizeof (h5t_leaf_iterator_t)));
	TRY (h5t_init_boundary_face_iterator (iter, m, codim));
	H5_API_RETURN (iter);
}

/**
   Get next local entity ID.

   \return		Local entity ID
   \return		-1, if done
   \return		error code on error
 */
static inline h5_loc_id_t
H5FedTraverseEntities (
        h5t_iterator_t* iter	///< [in,out] entity iterator
        ) {
	H5_API_ENTER (h5_loc_id_t, "iter=%p", iter);
	H5_API_RETURN (h5t_iterate_entities (iter));
}

/**
   End of traversing. Release internal data structures.

   \return	H5_SUCCESS or error code
 */
static inline h5_err_t
H5FedEndTraverseEntities (
	h5t_iterator_t* iter	///< [in,out] entity iterator
        ) {
	H5_API_ENTER (h5_err_t, "iter=%p", iter);
	H5_API_RETURN (h5t_release_entity_iterator (iter));
}


/**
   Get coordinates of vertex given by local index

   \return	H5_SUCCESS or error code
 */
static inline h5_err_t
H5FedGetVertexCoordsByIndex (
        h5t_mesh_t* const m,	    ///< [in] mesh object
        h5_loc_idx_t vertex_index,  ///< [in] local index of vertex
        h5_float64_t P[3]	    ///< [out] vertex coordinates
        ) {
	H5_API_ENTER (h5_err_t,
	              "m=%p, vertex_index=%lld, P=%p",
	              m, (long long)vertex_index, P);
	H5_API_RETURN (h5t_get_vertex_coords_by_index (m, vertex_index, P));
}

static inline h5_err_t
H5FedGetVertexCoordsByID (
        h5t_mesh_t* const m,
        h5_loc_id_t vertex_id,
        h5_float64_t P[3]
        ) {
	H5_API_ENTER (h5_err_t,
	              "m=%p, vertex_id=%lld, P=%p",
	              m, (long long)vertex_id, P);
	H5_API_RETURN (h5t_get_vertex_coords_by_id (m, vertex_id, P));
}

static inline h5_err_t
H5FedGetVertexIndicesOfEdge (
        h5t_mesh_t* const m,
        h5_loc_id_t entity_id,
        h5_loc_idx_t* vertex_indices
        ) {
	H5_API_ENTER (h5_err_t,
	              "m=%p, entity_id=%lld, vertex_indices=%p",
	              m, (long long)entity_id, vertex_indices);
	H5_API_RETURN (h5t_get_loc_vertex_indices_of_edge (m, entity_id, vertex_indices));
}

static inline h5_err_t
H5FedGetVertexIndicesOfTriangle (
        h5t_mesh_t* const m,
        h5_loc_id_t entity_id,
        h5_loc_idx_t* vertex_indices
        ) {
	H5_API_ENTER (h5_err_t,
	              "m=%p, entity_id=%lld, vertex_indices=%p",
	              m, (long long)entity_id, vertex_indices);
	H5_API_RETURN (h5t_get_loc_vertex_indices_of_triangle (m, entity_id, vertex_indices));
}

static inline h5_err_t
H5FedGetVertexIndicesOfTet (
        h5t_mesh_t* const m,
        h5_loc_id_t entity_id,
        h5_loc_idx_t* vertex_indices
        ) {
	H5_API_ENTER (h5_err_t,
	              "m=%p, entity_id=%lld, vertex_indices=%p",
	              m, (long long)entity_id, vertex_indices);
	H5_API_RETURN (h5t_get_loc_vertex_indices_of_tet (m, entity_id, vertex_indices));
}

static inline h5_err_t
H5FedGetVertexIndicesOfEntity (
        h5t_mesh_t* const m,
        h5_loc_id_t entity_id,
        h5_loc_idx_t* indices
        ) {
	H5_API_ENTER (h5_err_t,
	              "m=%p, entity_id=%lld, indices=%p",
	              m, (long long)entity_id, indices);
	H5_API_RETURN (h5t_get_loc_vertex_indices_of_entity (m, entity_id, indices));
}

static inline h5_err_t
H5FedGetGlobalVertexIndicesOfEntity (
        h5t_mesh_t* const m,
        h5_loc_id_t entity_id,
        h5_glb_idx_t* indices
        ) {
	H5_API_ENTER (h5_err_t,
	              "m=%p, entity_id=%lld, indices=%p",
	              m, (long long)entity_id, indices);
	H5_API_RETURN (h5t_get_glb_vertex_indices_of_entity (m, entity_id, indices));
}

static inline h5_err_t
H5FedGetVertexByID (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_glb_idx_t* glb_idx,
        h5_float64_t* P[]
        ) {
	H5_API_ENTER (h5_err_t,
	              "m=%p, entity_id=%lld, glb_idx=%p, P[]=%p",
	              m, (long long)entity_id, glb_idx, P);
	H5_API_RETURN (h5t_get_vertex_by_id (m, entity_id, glb_idx, P));
}

static inline h5_err_t
H5FedGetNeighborIndicesOfElement (
        h5t_mesh_t* const m,
        h5_loc_id_t entity_id,
        h5_loc_idx_t* neighbor_indices
        ) {
	H5_API_ENTER (h5_err_t,
	              "m=%p, entity_id=%lld, neighbor_indices=%p",
	              m, (long long)entity_id, neighbor_indices);
	H5_API_RETURN (h5t_get_neighbor_indices (m, entity_id, neighbor_indices));
}
#ifdef __cplusplus
}
#endif

#endif
