/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5T_MODEL_H
#define __H5CORE_H5T_MODEL_H

#include "h5core/h5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define H5_GEOBORDER_ENTITY     1
#define H5_INTERIOR_ENTITY      2
#define H5_BORDER_ENTITY        4
#define H5_OVERLAP_ENTITY       8
#define H5_FRONT_ENTITY         16
#define H5_GHOST_ENTITY         32
#define H5_LEAF_ENTITY          64

VTKH5HUT_EXPORT
h5_err_t h5t_open_tetrahedral_mesh (const h5_file_t, const char*, h5t_mesh_t**);
VTKH5HUT_EXPORT
h5_err_t h5t_open_tetrahedral_mesh_by_idx (const h5_file_t, const h5_id_t, h5t_mesh_t**);
VTKH5HUT_EXPORT
h5_err_t h5t_open_tetrahedral_mesh_part (const h5_file_p, const char*, h5t_mesh_t** mesh,
                                         h5_glb_idx_t*, h5_glb_idx_t);
VTKH5HUT_EXPORT
h5_ssize_t h5t_get_num_tetmeshes (const h5_file_t f);


// TODO just removed svn comment, need to check if ok
VTKH5HUT_EXPORT
h5_err_t
h5t_open_triangle_mesh (
        const h5_file_t, const char*, h5t_mesh_t**);

VTKH5HUT_EXPORT
h5_err_t
h5t_open_triangle_mesh_by_idx (
        const h5_file_t, const h5_id_t, h5t_mesh_t**);

VTKH5HUT_EXPORT
h5_err_t
h5t_open_triangle_mesh_part (
        const h5_file_t, const char*, h5t_mesh_t**,
        h5_glb_idx_t*, h5_glb_idx_t);

VTKH5HUT_EXPORT
h5_lvl_idx_t
h5t_is_chunked (
        h5t_mesh_t* const);

VTKH5HUT_EXPORT
h5_ssize_t
h5t_get_num_trimeshes (
        const h5_file_t f);

VTKH5HUT_EXPORT
h5_ssize_t
h5t_get_num_leaf_levels (
        h5t_mesh_t* const);

VTKH5HUT_EXPORT
h5_ssize_t
h5t_get_num_vertices (
        h5t_mesh_t* const, const h5_id_t);

VTKH5HUT_EXPORT
h5_ssize_t
h5t_get_num_leaf_elems (
        h5t_mesh_t* const, const h5_id_t);

VTKH5HUT_EXPORT
h5_id_t
h5t_add_chunked_tetrahedral_mesh (
        const h5_file_t, const char*, const h5_weight_t, h5t_mesh_t**);

VTKH5HUT_EXPORT
h5_id_t
h5t_add_chunked_triangle_mesh (
        const h5_file_t, const char*, const h5_weight_t, h5t_mesh_t**);

VTKH5HUT_EXPORT
h5_lvl_idx_t
h5t_get_level (
        h5t_mesh_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5t_open_tetrahedral_mesh (
        const h5_file_t, const char*, h5t_mesh_t**);

VTKH5HUT_EXPORT
h5_err_t
h5t_open_tetrahedral_mesh_by_idx (
        const h5_file_t, const h5_id_t, h5t_mesh_t**);

VTKH5HUT_EXPORT
h5_err_t
h5t_open_triangle_mesh (
        const h5_file_t, const char*, h5t_mesh_t**);

VTKH5HUT_EXPORT
h5_err_t
h5t_open_triangle_mesh_by_idx (
        const h5_file_t, const h5_id_t, h5t_mesh_t**);

VTKH5HUT_EXPORT
h5_id_t
h5t_add_tetrahedral_mesh (
        const h5_file_t, const char*, const h5_weight_t, h5t_mesh_t**);

VTKH5HUT_EXPORT
h5_id_t
h5t_add_triangle_mesh (
        const h5_file_t, const char*, const h5_weight_t, h5t_mesh_t**);

VTKH5HUT_EXPORT
h5_err_t
h5t_set_level (
        h5t_mesh_t* const, const h5_lvl_idx_t);

VTKH5HUT_EXPORT
h5_err_t
h5t_set_mesh_changed (
        h5t_mesh_t* const m);

VTKH5HUT_EXPORT
h5_err_t
h5t_close_mesh (
        h5t_mesh_t* const);

#ifdef __cplusplus
}
#endif

#endif
