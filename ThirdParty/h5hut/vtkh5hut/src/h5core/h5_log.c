/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "h5core/h5_log.h"
#include "h5core/h5_err.h"

extern int h5_myproc;


VTKH5HUT_EXPORT h5_int64_t			__h5_log_level = H5_VERBOSE_ERROR;
VTKH5HUT_EXPORT h5_int64_t			__h5_debug_mask = 0;

VTKH5HUT_EXPORT struct call_stack		h5_call_stack;

VTKH5HUT_EXPORT char *h5_rfmts[] = {
	[e_int]			= "%d",
	[e_ssize_t]		= "%ld",
	[e_char_p]		= "%s",
	[e_void_p]		= "%p",
	[e_h5_err_t]		= "%lld",
	[e_h5_int32_t]		= "%ld",
	[e_h5_uint32_t]		= "%lu",
	[e_h5_int64_t]		= "%lld",
	[e_h5_uint64_t]		= "%llu",
	[e_h5_id_t]		= "%lld",
	[e_h5_ssize_t]		= "%lld",
	[e_h5_errorhandler_t]	= "%p",
	[e_h5_file_p]		= "%p",
	[e_h5_file_t]		= "0x%08x",
	[e_h5_lvl_idx_t]	= "%d",
	[e_h5t_iterator_p]	= "%p",
	[e_h5_loc_id_t]		= "%ld",
	[e_h5_loc_idx_t]	= "%ld",
	[e_hid_t]		= "%lld",
	[e_H5O_type_t]		= "%ld",
	[e_h5_glb_elem_p]       = "%p",
        [e_h5_prop_p]           = "%p",
        [e_h5_prop_t]           = "0x%08x",
        [e_h5_prop_file_p]      = "%p",
        [e_h5_prop_file_t]      = "0x%08x",
	[e_herr_t]		= "%ld"
};

/*!
   \ingroup h5_core_errorhandling

   Set debug/verbosity level. On level 0 all output will be supressed (even
   error messages). On level 1 error messages, on level 2 warning messages
   and on level 3 informational messages will be printed. On level 4 debug
   messages will be printed.

   Values less than 0 are equivalent to 0. Values greater than 4 are equivalent
   to 4.

   \return \c H5_SUCCESS on success.
   \return \c H5_ERR_INVAL if debug level is invalid.
 */
h5_err_t
h5_set_loglevel (
        const h5_int64_t level     /*!< log level */
        ) {
	__h5_log_level = level & 0x7;
	return H5_SUCCESS;
}

h5_err_t
h5_set_debug_mask (
        const h5_int64_t mask     /*!< debug level */
        ) {
	__h5_debug_mask = mask;
	return H5_SUCCESS;
}

/*!
   \ingroup h5_core_errorhandling

   Get current debug/verbosity level.

   \return current debug level
 */
h5_int64_t
h5_get_loglevel (
        void
        ) {
	return __h5_log_level;
}

void
h5priv_vprintf (
        FILE* f,
        const char* prefix,
        const char* __funcname,
        const char* fmt,
        va_list ap
        ) {
	char fmt2[2048];
	snprintf (fmt2, sizeof(fmt2), "[proc %d] %s: %s: %s\n", h5_myproc, prefix,
	          __funcname, fmt);
	vfprintf (f, fmt2, ap);
}

h5_err_t
h5_warn (
        const char* fmt,
        ...
        ) {
	if (__h5_log_level >= 2) {
		va_list ap;
		va_start (ap, fmt);
		h5priv_vprintf (stderr, "W", h5_get_funcname(), fmt, ap);
		va_end (ap);
	}
	return H5_NOK;
}

void
h5_info (
        const char* fmt,
        ...
        ) {
	if (__h5_log_level >= 3) {
		va_list ap;
		va_start (ap, fmt);
		h5priv_vprintf (stdout, "I", h5_get_funcname(), fmt, ap);
		va_end (ap);
	}
}

void
h5_debug (
        const char* const fmt,
        ...
        ) {
	if (__h5_log_level >= 4) {
		char prefix[1024];
		snprintf (prefix, sizeof(prefix), "%*s %s",
		          h5_call_stack_get_level(), "",
		          h5_call_stack_get_name());
		va_list ap;
		va_start (ap, fmt);
		h5priv_vprintf (stdout, "D", prefix, fmt, ap);
		va_end (ap);
	}
}
