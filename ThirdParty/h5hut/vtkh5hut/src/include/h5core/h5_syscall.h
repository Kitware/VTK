/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5_SYSCALL_H
#define __H5CORE_H5_SYSCALL_H

#include "h5core/h5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

	VTKH5HUT_EXPORT
	h5_err_t h5_free (void* ptr);
	VTKH5HUT_EXPORT
	void_p h5_alloc (void* ptr, const size_t size);
	VTKH5HUT_EXPORT
	void_p h5_calloc (const size_t count, const size_t size);
	VTKH5HUT_EXPORT
	char_p h5_strdup (const char* s1);

#ifdef __cplusplus
}
#endif

#endif
