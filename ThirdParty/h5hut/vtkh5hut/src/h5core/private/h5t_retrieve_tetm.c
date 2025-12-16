/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "private/h5t_types.h"
#include "private/h5t_err.h"
#include "private/h5t_core.h"
#include "private/h5t_map.h"
#include "private/h5t_model.h"
#include "private/h5t_access.h"
#include "private/h5t_tags.h"
#include "private/h5t_retrieve.h"

static h5_err_t
init_entity_iterator (
        h5t_iterator_t* const iter,
        const int codim
        ) {
	h5t_leaf_iterator_t* it = (h5t_leaf_iterator_t*)iter;
	switch (it->ref_elem->dim - codim) {
	case 0: // iterate vertices
		it->find = h5tpriv_find_tv2;
		break;
	case 1: // iterate edges
		it->find = h5tpriv_find_te2;
		break;
	case 2: // iterate faces
		it->find = h5tpriv_find_td2;
		break;
	case 3: // iterate elems
		it->find = NULL;
		break;
	default:
		return h5_error_internal ();
	}
	return H5_SUCCESS;
}

struct h5t_retrieve_methods h5tpriv_tetm_retrieve_methods = {
	init_entity_iterator
};

