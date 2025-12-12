/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5_SYSCALL_H
#define __H5CORE_H5_SYSCALL_H

#include <stdlib.h>
#include <string.h>

#include "h5core/h5_types.h"
#include "private/h5_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MALLOC_WRAPPER_ENTER(type, fmt, ...)			\
	__FUNC_ENTER(type, H5_DEBUG_MALLOC, fmt, __VA_ARGS__)

h5_err_t
h5_free (
        void* ptr
        ) {
	MALLOC_WRAPPER_ENTER (h5_err_t, "ptr=%p", ptr);
	if (ptr) {
		free (ptr);
	}
	H5_RETURN (H5_SUCCESS);
}


void_p
h5_alloc (
        void* ptr,
        const size_t size
        ) {
	MALLOC_WRAPPER_ENTER (void_p, "ptr=%p, size=%lu", ptr, size);
	if (size < 1) {
		ret_value = (void_p) h5_free (ptr);
		H5_LEAVE (NULL);
	}
	ptr = realloc (ptr, size);
	if (ptr == NULL) {
		H5_LEAVE (
		        (void_p)h5_error (
				H5_ERR_NOMEM,
				"Out of memory. Tried to alloc %lld", (long long int)size));
	}
	H5_RETURN (ptr);
}

void_p
h5_calloc (
        const size_t count,
        const size_t size
        ) {
	MALLOC_WRAPPER_ENTER (void_p, "count=%zu , size=%zu", count, size);
	void* ptr = NULL;
	if (count * size < 1) {
		H5_LEAVE (ptr);
	}
	ptr = calloc (count, size);
	if (ptr == NULL) {
		H5_LEAVE (
		        (void_p)h5_error (
				H5_ERR_NOMEM,
				"Out of memory. Tried to alloc %lld", (long long int)count* size));
	}
	H5_RETURN (ptr);
}


char_p
h5_strdup (
        const char* s1
        ) {
	MALLOC_WRAPPER_ENTER (char_p, "s='%s'", s1);

	char_p s2 = (char_p)h5_calloc (1, strlen (s1)+1 );
	if (s2 == NULL) {
		H5_LEAVE (
		        (char_p)h5_error (
				H5_ERR_NOMEM,
				"Out of memory."));
	}
	H5_RETURN (strcpy (s2, s1));
}

#ifdef __cplusplus
}
#endif

#endif
