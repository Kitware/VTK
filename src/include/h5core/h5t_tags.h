/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5T_TAGS_H 
#define __H5CORE_H5T_TAGS_H

#include "h5core/h5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct h5t_tagset h5t_tagset_t;
typedef struct h5t_tagcontainer h5t_tagcontainer_t;

VTKH5HUT_EXPORT
h5_ssize_t
h5t_get_num_mtagsets (h5t_mesh_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5t_get_mtagset_info (h5t_mesh_t* const, const h5_size_t, char[],
                      const h5_size_t, h5_int64_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5t_mtagset_exists (h5t_mesh_t* const, const char* const);

VTKH5HUT_EXPORT
h5_err_t
h5t_create_mtagset (h5t_mesh_t* const, const char[], const h5_types_t, h5t_tagset_t**);

VTKH5HUT_EXPORT
h5_err_t
h5t_open_mtagset (h5t_mesh_t* const, const char* const, h5t_tagset_t**);

VTKH5HUT_EXPORT
h5_err_t
h5t_close_mtagset (h5t_tagset_t*);

VTKH5HUT_EXPORT
h5_err_t
h5t_remove_mtagset (h5t_mesh_t* const, const char[]);

VTKH5HUT_EXPORT
h5_err_t
h5t_set_tag (h5t_tagset_t* const, const h5_loc_id_t, const h5_size_t, void*);

VTKH5HUT_EXPORT
h5_loc_id_t
h5t_get_tag (const h5t_tagset_t*, const h5_loc_id_t, h5_size_t* const, void* const);

VTKH5HUT_EXPORT
h5_err_t
h5t_remove_tag (h5t_tagset_t*, const h5_loc_id_t);

#ifdef __cplusplus
}
#endif

#endif
