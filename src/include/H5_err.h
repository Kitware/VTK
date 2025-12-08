/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5_ERROR_H
#define __H5_ERROR_H

#include "h5core/h5_log.h"

/**
   \addtogroup h5_error
   @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
  Report error, do not abort program. The error must be handled in the programm.

  \return \c H5_SUCCESS

  \see H5SetErrorHandler()
  \see H5ReportErrorhandler()
*/
static inline h5_err_t
H5ReportOnError (
        void
        ) {
	H5_API_ENTER (h5_err_t, "%s", "");
	H5_API_RETURN (h5_set_errorhandler (h5_report_errorhandler));
}

/**
  Abort program on error.

  \return \c H5_SUCCESS

  \see H5SetErrorHandler()
  \see H5AbortErrorhandler()
*/
static inline h5_err_t
H5AbortOnError (
        void
        ) {
	return h5_set_errorhandler (h5_abort_errorhandler);
}

/**
  Set error handler to \c handler.

  \return \c H5_SUCCESS

  \see H5GetErrorHandler()
  \see H5ReportErrorhandler()
  \see H5AbortErrorhandler()
*/
static inline h5_err_t
H5SetErrorHandler (
	h5_errorhandler_t handler ///< [in] error handler to set.
	) {
	H5_API_ENTER (h5_err_t, "handler=%p", handler);
	H5_API_RETURN (h5_set_errorhandler (handler));
}

/**
  Get current error handler.

  \return Pointer to error handler.

  \see H5SetErrorHandler()
  \see H5ReportErrorhandler()
  \see H5AbortErrorhandler()
*/
static inline h5_errorhandler_t
H5GetErrorHandler (
	void
	) {
	H5_API_ENTER (h5_errorhandler_t, "%s", "void");
	H5_API_RETURN (h5_get_errorhandler());
}


/**
  The report error handler writes a message to stderr, sets the error number
  and returns.

  \return   \c H5_FAILURE
 */
static inline h5_err_t
H5ReportErrorhandler (
	const char* fmt,        ///< [in] format string of error message.
	va_list ap              ///< [in] arguments to format string.
	) {
	return h5_report_errorhandler (fmt, ap);
}

/**
  The abort error handler writes a message to stderr and exits the programm.

  \return       does not return.
 */
static inline h5_err_t
H5AbortErrorhandler (
	const char* fmt,        ///< [in] format string of error message.
	va_list ap              ///< [in] arguments to format string.
	) {
	return h5_abort_errorhandler (fmt, ap);
}

/**
  Get last error code.

  Error codes are:

  - \c H5_ERR_BADF:	Something is wrong with the file handle.
  - \c H5_ERR_NOMEM:	Out of memory.
  - \c H5_ERR_INVAL:	Invalid argument.
  
  - \c H5_ERR_VIEW:	Something is wrong with the view.
  - \c H5_ERR_NOENTRY:	A lookup failed.
  
  - \c H5_ERR_MPI:	A MPI error occured.
  - \c H5_ERR_HDF5:	A HDF5 error occured.
  - \c H5_ERR_H5:	Unspecified error in H5 module.
  - \c H5_ERR_H5PART:	Unspecified error in H5Part module.
  - \c H5_ERR_H5BLOCK:	Unspecified error in H5Block module.
  - \c H5_ERR_H5FED:	Unspecified error in H5Fed module.
  
  - \c H5_ERR_INTERNAL:	Internal error.
  - \c H5_ERR_NOT_IMPLEMENTED: Function not yet implemented.

  \return error code
*/
static inline h5_err_t
H5GetErrno (
	void
	) {
	return h5_get_errno ();
}

#ifdef __cplusplus
}
#endif

///< @}
#endif
