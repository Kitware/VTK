/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5T_RETRIEVE_H
#define __PRIVATE_H5T_RETRIEVE_H

#include "h5core/h5_types.h"
#include "private/h5t_types.h"
#include "h5core/h5t_retrieve.h"

struct h5t_retrieve_methods {
	h5_err_t (*init_entity_iterator)(
	        h5t_iterator_t*, const int);
};

extern struct h5t_retrieve_methods h5tpriv_trim_retrieve_methods;
extern struct h5t_retrieve_methods h5tpriv_tetm_retrieve_methods;

static inline h5_err_t
h5tpriv_init_entity_iterator (
        h5t_mesh_t* m,
        h5t_iterator_t* const iter,
        const int codim
        ) {
	return m->methods->retrieve->init_entity_iterator (iter, codim);
}

#endif
