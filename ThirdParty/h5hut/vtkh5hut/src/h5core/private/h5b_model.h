/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5B_MODEL_H
#define __PRIVATE_H5B_MODEL_H

#include "h5core/h5_types.h"

#define CHECK_LAYOUT(f)                                         \
	TRY (f->b->have_layout ? H5_SUCCESS : h5_error (        \
                     H5_ERR_VIEW,                               \
                     "No view has been defined!"));

h5_err_t
h5bpriv_open_field_group (
	const h5_file_p f,			/*!< IN: file handle */
	const char *name
	);

h5_err_t
h5bpriv_create_field_group (
	const h5_file_p f,		/*!< IN: file handle */
	const char *name		/*!< IN: name of field group to create */
	);

h5_err_t
h5bpriv_open_block_group (
	const h5_file_p f			/*!< IN: file handle */
	);

h5_err_t
h5bpriv_release_hyperslab (
	const h5_file_p f			/*!< IN: file handle */
	);

#endif
