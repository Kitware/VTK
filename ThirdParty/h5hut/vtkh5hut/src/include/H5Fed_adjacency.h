/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5FED_ADJACENCY_H
#define __H5FED_ADJACENCY_H

#include "h5core/h5_types.h"
#include "h5core/h5_log.h"
#include "h5core/h5t_adjacencies.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline h5_err_t
H5FedGetAdjacencies (
        h5t_mesh_t* const m,
        const h5_loc_id_t entity_id,
        const h5_int32_t dim,
        h5_loc_idlist_t** list
        ) {
	H5_API_ENTER (h5_err_t,
	              "m=%p, entity_id=%lld, dim=%d, list=%p",
	              m, (long long)entity_id, dim, list);
	H5_API_RETURN (h5t_get_adjacencies (m, entity_id, dim, list));
}

static inline h5_err_t
H5FedReleaseListOfAdjacencies (
        h5t_mesh_t* const m,
        h5_loc_idlist_t** list
        ) {
	H5_API_ENTER (h5_err_t, "f=%p, list=%p", m, list);
	H5_API_RETURN (h5t_release_list_of_adjacencies (m, list));
}

#ifdef __cplusplus
}
#endif

#endif
