/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5T_ADJACENCIES_H
#define __H5CORE_H5T_ADJACENCIES_H

#include "h5core/h5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

VTKH5HUT_EXPORT
h5_err_t
h5t_get_adjacencies (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        const h5_int32_t dim,
        h5_loc_idlist_t **list
        );

VTKH5HUT_EXPORT
h5_err_t
h5t_release_list_of_adjacencies (
        h5t_mesh_t* const m,
        h5_loc_idlist_t **list
        );

VTKH5HUT_EXPORT
h5_err_t
h5t_find_te2 (
        h5t_mesh_t* const m,
        h5_loc_idx_t face_idx,
        h5_loc_idx_t elem_idx,
        h5_loc_idlist_t** retval
        );

#ifdef __cplusplus
}
#endif

#endif
