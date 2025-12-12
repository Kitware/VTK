/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5FED_MODEL_H
#define __H5FED_MODEL_H

#include "h5core/h5_types.h"
#include "h5core/h5_log.h"
#include "h5core/h5t_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  Get number of meshes of tetrahedral meshes in file.

  \return   number of tetrahedral meshes
  \return   \c H5_FAILURE
 */
static inline h5_ssize_t
H5FedGetNumTetrahedralMeshes ( 
	const h5_file_t f	///< file object
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p",
                      (h5_file_p)f);
	H5_API_RETURN (h5t_get_num_tetmeshes (f));
}

static inline h5_ssize_t
H5FedGetNumTriangleMeshes ( 
	const h5_file_t f
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p",
                      (h5_file_p)f);
	H5_API_RETURN (h5t_get_num_trimeshes (f));
}

/**
  Get the number of hierarchical mesh levels.

  \return	Number of hierarchical mesh levels or error code.
 */
static inline h5_ssize_t
H5FedGetNumLevels (
	h5t_mesh_t* const m	///< [in] mesh object
	) {
	H5_API_ENTER (h5_ssize_t, "m=%p", m);
	H5_API_RETURN (h5t_get_num_leaf_levels (m));
}

/**
  Get current mesh levels.

  \return	ID of current mesh levels or error code.
 */
static inline h5_lvl_idx_t
H5FedGetLevel (
	h5t_mesh_t* const m	///< [in] mesh object
	) {
	H5_API_ENTER (h5_lvl_idx_t, "m=%p", m);
	H5_API_RETURN (h5t_get_level (m));
}

/**
  Returns the number of vertices used for defining the (sub-)mesh
  at current level on this compute node.

  \return	Number of vertices or error code.
*/
static inline h5_ssize_t
H5FedGetNumVertices (
	h5t_mesh_t* const m	///< [in] mesh object
	) {
	H5_API_ENTER (h5_ssize_t, "m=%p", m);
	H5_API_RETURN (h5t_get_num_vertices (m, -1));
}

/**
  Returns the number of vertices used for defining the (sub-)mesh
  at current level on compute node \c cnode.

  \return	Number of vertices or error code.
*/
static inline h5_ssize_t
H5FedGetNumVerticesCnode (
	h5t_mesh_t* const m,	///< [in] mesh object
	const int cnode		///< [in] compute node to query
	) {
	H5_API_ENTER (h5_ssize_t, "m=%p, cnode=%d", m, cnode);
	H5_API_RETURN (h5t_get_num_vertices (m, cnode));
}

/**
  Returns the number of vertices used for defining the (sub-)mesh
  at current level overl all compute nodes.

  \return	Total number of vertices or error code.
*/
static inline h5_ssize_t
H5FedGetNumVerticesTotal (
	h5t_mesh_t* const m	///< [in] mesh object
	) {
	H5_API_ENTER (h5_ssize_t, "m=%p", m);
	H5_API_RETURN (h5t_get_num_vertices (m, -1));
}

/**
  Returns the number of elements present in the (sub-)mesh
  at current level on this compute node.

  \return	Number of elements or error code.
*/
static inline h5_ssize_t
H5FedGetNumElements (
	h5t_mesh_t* const m	///< [in] mesh object
	) {
	H5_API_ENTER (h5_ssize_t, "m=%p", m);
	H5_API_RETURN (h5t_get_num_leaf_elems (m, -1));
}

/**
  Returns the number of elements present in the (sub-)mesh
  at current level on compute node \c cnode.

  \return	Number of elements or error code.
*/
static inline h5_ssize_t
H5FedGetNumElementsCnode (
	h5t_mesh_t* const m,	///< [in] mesh object
	const int cnode		///< [in] compute node to query
	) {
	H5_API_ENTER (h5_ssize_t, "m=%p, cnode=%d", m, cnode);
	H5_API_RETURN (h5t_get_num_leaf_elems (m, cnode));
}

/**
  Returns the number of elements present in the mesh
  at current level over all compute nodes.

  \return	Number of elements or error code.
*/
static inline h5_ssize_t
H5FedGetNumElementsTotal (
	h5t_mesh_t* const m	///< [in] mesh object
	) {
	H5_API_ENTER (h5_ssize_t, "m=%p", m);
	H5_API_RETURN (h5t_get_num_leaf_elems (m, -1));
}

static inline h5_err_t
H5FedOpenTetrahedralMesh (
	const h5_file_t f,
	const char* name,
	h5t_mesh_t** mesh
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, name=%s, mesh=%p", 
                      (h5_file_p)f, name, mesh);
	H5_API_RETURN (h5t_open_tetrahedral_mesh (f, name, mesh));
}

static inline h5_err_t
H5FedOpenTetrahedralMeshByIndex (
	const h5_file_t f,
	const h5_id_t idx,
	h5t_mesh_t** mesh
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, idx=%lld, mesh=%p",
                      (h5_file_p)f, (long long)idx, mesh);
	H5_API_RETURN (h5t_open_tetrahedral_mesh_by_idx (f, idx, mesh));
}

static inline h5_err_t
H5FedOpenTriangleMesh (
	const h5_file_t f,
	const char* name,
	h5t_mesh_t** mesh
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, name=%s, mesh=%p",
                      (h5_file_p)f, name, mesh);
	H5_API_RETURN (h5t_open_triangle_mesh (f, name, mesh));
}

static inline h5_err_t
H5FedOpenTriangleMeshPart (
        const h5_file_t f,
        const char* name,
        h5t_mesh_t** mesh,
        h5_glb_idx_t* const elem_indices,
        const h5_glb_idx_t num_elems
        ) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, name=%s, mesh=%p",
                      (h5_file_p)f, name, mesh);
	H5_API_RETURN (h5t_open_triangle_mesh_part (f, name, mesh, elem_indices, num_elems));
}

static inline h5_err_t
H5FedOpenTriangleMeshByIndex (
	const h5_file_t f,
	const h5_id_t idx,
	h5t_mesh_t** mesh
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, idx=%lld, mesh=%p",
                      (h5_file_p)f, (long long)idx, mesh);
	H5_API_RETURN (h5t_open_triangle_mesh_by_idx (f, idx, mesh));
}

static inline h5_err_t
H5FedCloseMesh (
        h5t_mesh_t* const m
        ) {
	H5_API_ENTER (h5_err_t, "m=%p", m);
	H5_API_RETURN (h5t_close_mesh (m));
}

static inline h5_err_t
H5FedSetLevel (
        h5t_mesh_t* const m,
        const h5_lvl_idx_t level_id
        ) {
	H5_API_ENTER (h5_err_t, "m=%p, level_id=%d", m, level_id);
	H5_API_RETURN (h5t_set_level (m, level_id));
}

static inline h5_err_t
H5FedSetMeshChanged (
        h5t_mesh_t* const m
        ) {
	H5_API_ENTER (h5_err_t, "m=%p", m);
	H5_API_RETURN (h5t_set_mesh_changed (m));
}

#ifdef __cplusplus
}
#endif

#endif
