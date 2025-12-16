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


#include <stdlib.h>

static h5_errorhandler_t	h5_errhandler = h5_report_errorhandler;
h5_err_t			h5_errno;

/*!
   \ingroup h5_core
   \defgroup h5_core_errorhandling

   TODO: this is broken by design ...
 */
const char* const H5_O_MODES[] = {
        "unknown",              // 0
	"H5_O_RDWR",            // 1
        "H5_O_RDONLY",          // 2
        "unknown",              // 3
	"H5_O_WRONLY",          // 4
        "unknown",              // 5
        "unknown",              // 6
        "unknown",              // 7
	"H5_O_APPENDONLY"
};

/*!
   \ingroup h5_core_errorhandling

   Set own error handler.

   \return \c H5_SUCCESS
 */
h5_err_t
h5_set_errorhandler (
        const h5_errorhandler_t handler
        ) {
	h5_errhandler = handler;
	return H5_SUCCESS;
}

/*!
   \ingroup h5_core_errorhandling

   Return pointer to current error handler.

   \return \c H5_SUCCESS
 */
h5_errorhandler_t
h5_get_errorhandler (
        void
        ) {
	return h5_errhandler;
}

/*!
   \ingroup h5_core_errorhandling

   Get current error number.

   \return \c H5_SUCCESS
 */
h5_err_t
h5_get_errno (
        void
        ) {
	return h5_errno;
}

/*!
   \ingroup h5_core_errorhandling

   Set error number.

   \return \c H5_SUCCESS
 */
void
h5_set_errno (
        const h5_err_t err
        ) {
	h5_errno = err;
}

/*!
   \ingroup h5_core_errorhandling

   Print error message to \c stderr. For use in error handlers only.
 */
void
h5_verror (
        const char* const fmt,
        va_list ap
        ) {
	if (__h5_log_level == 0) return;
	char fmt2[2048];
	snprintf (fmt2,
		  sizeof(fmt2), "[proc %d] E: %s: %s\n",
		  h5_myproc,
		  h5_call_stack.entry[0].name,
		  fmt);
	vfprintf (stderr, fmt2, ap);
	
}

/*!
   \ingroup h5_core_errorhandling

   This is the H5 default error handler.  If an error occures, the
   error message will be printed, if debug level is greater than 0.

   \return \c f->__errno
 */
h5_err_t
h5_report_errorhandler (
        const char* const fmt,
        va_list ap
        ) {
	if (__h5_log_level > 0) {
		h5_verror (fmt, ap);
	}
	return h5_errno;
}

/*!
   \ingroup h5_core_errorhandling

   If an error occures, the error message will be printed and the
   program exists with the error code given in \c f->__errno.
 */
h5_err_t
h5_abort_errorhandler (
        const char* const fmt,
        va_list ap
        ) {
	if (__h5_log_level > 0) {
		h5_verror (fmt, ap);
	}
#ifdef H5_HAVE_PARALLEL
	MPI_Abort(MPI_COMM_WORLD, -(int)h5_errno);
#else
	exit (-(int)h5_errno);
#endif
	return -(int)h5_errno; // never executed, just to supress a warning
}

/*!
   \ingroup h5_core_errorhandling

   Print error message via error handler.

   \return \c f->__errno
 */
h5_err_t
h5_error (
        const h5_err_t errno_,
        const char* const fmt,
        ...
        ) {
	h5_errno = errno_;
	va_list ap;
	va_start (ap, fmt);

	(*h5_errhandler)(fmt, ap);

	va_end (ap);
	return h5_errno;
}
