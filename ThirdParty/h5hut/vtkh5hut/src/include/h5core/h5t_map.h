/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5T_MAP_H
#define __H5T_MAP_H

#include "h5core/h5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

VTKH5HUT_EXPORT
h5_loc_idx_t
h5t_map_global_vertex_idx2local (
        h5t_mesh_t* const m,
        h5_glb_idx_t glb_idx
        );

VTKH5HUT_EXPORT
h5_err_t
h5t_map_global_vertex_indices2local (
        h5t_mesh_t* f,
        const h5_glb_id_t* const glb_indices,
        const h5_size_t size,
        h5_loc_idx_t* const loc_indices
        );

VTKH5HUT_EXPORT
h5_loc_idx_t
h5t_map_glb_elem_idx2loc (
        h5t_mesh_t* const m,
        const h5_glb_idx_t glb_idx
        );

VTKH5HUT_EXPORT
h5_err_t
h5t_map_glb_elem_indices2loc (
        h5t_mesh_t* const m,
        const h5_glb_idx_t* glb_indices,
        const h5_size_t size,
        h5_loc_idx_t* loc_indices
        );

VTKH5HUT_EXPORT
h5_err_t
h5t_get_loc_vertex_indices_of_edge (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_idx_t *vertex_indices
        );

VTKH5HUT_EXPORT
h5_err_t
h5t_get_loc_vertex_indices_of_edge2 (
        h5t_mesh_t* const m,
        const h5_loc_idx_t face_idx,
        const h5_loc_idx_t elem_id,
        h5_loc_idx_t* vertex_indices
        );

VTKH5HUT_EXPORT
h5_err_t
h5t_get_loc_vertex_indices_of_triangle (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_idx_t* vertex_indices
        );

VTKH5HUT_EXPORT
h5_err_t
h5t_get_loc_vertex_indices_of_triangle2 (
        h5t_mesh_t* const m,
        const h5_loc_idx_t face_idx,
        const h5_loc_idx_t elem_idx,
        h5_loc_idx_t* vertex_indices
        );

VTKH5HUT_EXPORT
h5_err_t
h5t_get_loc_vertex_indices_of_tet (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_idx_t *vertex_indices
        );

VTKH5HUT_EXPORT
h5_err_t
h5t_get_loc_vertex_indices_of_entity (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_idx_t *vertex_indices
        );

VTKH5HUT_EXPORT
h5_err_t
h5t_get_glb_vertex_indices_of_entity (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_glb_idx_t *vertex_indices
        );

#ifdef __cplusplus
}
#endif

#endif
