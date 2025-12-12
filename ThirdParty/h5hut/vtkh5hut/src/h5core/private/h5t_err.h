/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5T_ERR_H
#define __PRIVATE_H5T_ERR_H

#include "h5core/h5_types.h"
#include "h5core/h5_err.h"
#include "private/h5_log.h"

#define ERR_ELEM_NEXIST  "Element with local vertex IDs (%s) doesn't exist!"

static inline h5_err_t
h5tpriv_inval_codim (
        int codim,
        int min_codim,
        int max_codim
        ) {
	return h5_error (H5_ERR_INVAL,
	                 "Co-dimension %d requested, "
	                 "but must be between %d and %d",
	                 codim, min_codim, max_codim);
}

#define h5tpriv_error_undef_mesh()              \
        h5_error(                               \
                H5_ERR_INVAL,                   \
                "Mesh not yet defined." );

#define h5tpriv_error_undef_level()             \
        h5_error(                               \
                H5_ERR_INVAL,                   \
                "Level not defined." );


#define h5tpriv_error_nexist_level( level_id )  \
        h5_error(                               \
                H5_ERR_INVAL,                   \
                "Level %lld doesn't exist.", (long long)level_id );

#define h5tpriv_error_local_triangle_nexist( indices )	\
	h5_error(							\
		H5_ERR_NOENTRY,						\
		"Triangle with local vertex IDs (%lld,%lld,%lld) doesn't exist!", \
		(long long)indices[0], (long long)indices[1], (long long)indices[2] );

#endif
