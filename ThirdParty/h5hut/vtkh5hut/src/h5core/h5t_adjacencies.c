/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "h5core/h5_log.h"

#include "private/h5_file.h"
#include "private/h5t_types.h"
#include "private/h5t_adjacencies.h"
#include "private/h5t_core.h"

#include <time.h>

h5_err_t
h5t_get_adjacencies (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        const h5_int32_t dim,
        h5_loc_idlist_t** list
        ) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "m=%p, entity_id=%llu, dim=%d, list=%p",
	                   m, (long long unsigned)entity_id, dim, list);
	TRY (ret_value = h5tpriv_get_adjacencies (m, entity_id, dim, list));
	H5_RETURN (ret_value);
}

h5_err_t
h5t_release_list_of_adjacencies (
        h5t_mesh_t* const m,
        h5_loc_idlist_t** list
        ) {
	H5_CORE_API_ENTER (h5_err_t, "m=%p, list=%p", m, list);
	UNUSED_ARGUMENT (m);
	TRY (ret_value = h5priv_free_loc_idlist (list));
	H5_RETURN (ret_value);
}

h5_err_t
h5t_find_te2 (
        h5t_mesh_t* const m,
        h5_loc_idx_t face_idx,
        h5_loc_idx_t elem_idx,
        h5_loc_idlist_t** retval
        ) {
	H5_CORE_API_ENTER (h5_err_t,
	                   "m=%p, face_idx=%lld, elem_idx=%lld, retval=%p",
	                   m,
	                   (long long)face_idx,
	                   (long long)elem_idx,
	                   retval);
	TRY (ret_value = h5tpriv_find_te2 (m,face_idx,elem_idx,retval));
	H5_RETURN (ret_value);
}
