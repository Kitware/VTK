/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5T_MAP_H
#define __PRIVATE_H5T_MAP_H

#include "private/h5_types.h"
#include "h5core/h5_log.h"
#include "h5core/h5_err.h"
#include "h5core/h5_syscall.h"
#include "private/h5_log.h"
#include "private/h5_maps.h"
#include "private/h5t_model.h"

/*
  Find ID in sorted list
  Special version of macro h5priv_find_in_xlist() for local and global entity IDs
*/
#define h5priv_find_in_idlist(type)                                     \
        static inline h5_loc_idx_t                                      \
        h5priv_find_in_ ## type ## list (                               \
                h5_ ## type ## list_t* list,                            \
                const h5_ ## type ## _t item                            \
                ) {                                                     \
                H5_PRIV_API_ENTER (h5_err_t,                            \
                                   "list=%p, item=%llu",                \
                                   list, (long long unsigned)item);     \
                if (!list) {                                            \
                        H5_LEAVE (-1);                         \
                }                                                       \
                register ssize_t low = 0;                               \
                register ssize_t mid;                                   \
                register ssize_t high = list->num_items - 1;            \
                register h5_ ## type ## x_t diff;                       \
                const h5_ ## type ## _t face_id =  h5tpriv_get_face_id(item); \
                const h5_ ## type ## x_t elem_idx =  h5tpriv_get_elem_idx(item); \
                while (low <= high) {                                   \
                        mid = (low + high) / 2;                         \
                        diff = h5tpriv_get_elem_idx(list->items[mid]) - elem_idx; \
                        if (diff == 0) {                                \
                                diff = h5tpriv_get_face_id (list->items[mid]) - face_id; \
                        }                                               \
                        if ( diff > 0 )                                 \
                                high = mid - 1;                         \
                        else if ( diff < 0 )                            \
                                low = mid + 1;                          \
                        else                                            \
                                H5_LEAVE (mid);                \
                }                                                       \
                H5_RETURN (-(low+1));                          \
        }


h5priv_alloc_xlist (loc_id)
h5priv_free_xlist (loc_id)
h5priv_insert_into_xlist (loc_id)
h5priv_find_in_idlist (loc_id)
h5priv_search_in_xlist (loc_id)

h5priv_alloc_xlist (glb_id)
h5priv_free_xlist (glb_id)
h5priv_insert_into_xlist (glb_id)
h5priv_find_in_idlist (glb_id)
h5priv_search_in_xlist (glb_id)

h5_err_t
h5tpriv_get_loc_vtx_idx_of_vtx (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        h5_loc_idx_t* vertex_index
        );

h5_err_t
h5tpriv_get_loc_vtx_idx_of_vtx2 (
        h5t_mesh_t* const m,
        const h5_loc_idx_t face_idx,
        const h5_loc_idx_t elem_idx,
        h5_loc_idx_t* vertex_indices
        );

h5_err_t
h5tpriv_sort_local_vertex_indices (
        h5t_mesh_t* const m,
        h5_loc_idx_t * const indices,
        const h5_size_t size
        );

h5_loc_idx_t
h5tpriv_get_local_vid (
        h5t_mesh_t* const m,
        h5_float64_t P[3]
        );

h5_err_t
h5tpriv_rebuild_map_vertex_g2l (
        h5t_mesh_t* const m,
        h5_lvl_idx_t from_lvl,
        h5_lvl_idx_t to_lvl
        );
h5_err_t
h5tpriv_rebuild_map_vertex_g2l_partial (
        h5t_mesh_t* const m
        );
h5_err_t
h5priv_exchange_loc_list_to_glb (
                h5t_mesh_t* const m,
                h5_glb_idxlist_t** glb_list
                );

h5_loc_idx_t
h5tpriv_find_glb_idx_in_map (
                h5_idxmap_t*  map,
        const h5_glb_idx_t glb_idx
        );
#endif
