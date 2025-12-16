/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5T_STORE_H
#define __H5CORE_H5T_STORE_H

#include "h5core/h5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

VTKH5HUT_EXPORT
h5_err_t
h5t_begin_store_vertices (
        h5t_mesh_t* const, const h5_size_t);

VTKH5HUT_EXPORT
h5_loc_id_t
h5t_store_vertex (
        h5t_mesh_t* const, const h5_glb_id_t, const h5_float64_t[3]);

VTKH5HUT_EXPORT
h5_err_t
h5t_end_store_vertices (
        h5t_mesh_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5t_begin_store_elems (
        h5t_mesh_t* const, const h5_size_t);

VTKH5HUT_EXPORT
h5_loc_idx_t
h5t_add_lvl0_cell (
        h5t_mesh_t* const, const h5_loc_idx_t*, const h5_weight_t*);

VTKH5HUT_EXPORT
h5_err_t
h5t_end_store_elems (
        h5t_mesh_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5t_end_store_ckd_elems (
        h5t_mesh_t* const m);

VTKH5HUT_EXPORT
h5_err_t
h5t_begin_refine_elems (
        h5t_mesh_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5t_refine_marked_elems (
        h5t_mesh_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5t_end_refine_elems (
        h5t_mesh_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5t_mark_entity (
        h5t_mesh_t* const, const h5_loc_id_t);

VTKH5HUT_EXPORT
h5_err_t
h5t_pre_refine (
        h5t_mesh_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5t_refine (
        h5t_mesh_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5t_post_refine (
        h5t_mesh_t* const);

#ifdef __cplusplus
}
#endif

#endif
