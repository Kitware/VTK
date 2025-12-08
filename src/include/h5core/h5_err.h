/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5_ERROR_H
#define __H5CORE_H5_ERROR_H

#include <stdarg.h>
#include <stdio.h>
#include "h5core/h5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
   \addtogroup h5_error
   @{
*/

#define H5_SUCCESS		0               ///< Function performs successfully
#define H5_OK			H5_SUCCESS      ///< Alias for \c H5_SUCCESS
#define H5_NOK			-1              ///< 
#define H5_FAILURE 		-2              ///< Function runs into an error
#define H5_ERR			H5_FAILURE      ///< Alias for H5_FAILURE

#define H5_ERR_BADF		-9              ///< Something is wrong with the file handle.
#define H5_ERR_NOMEM		-12             ///< Out of memory.
#define H5_ERR_INVAL		-22             ///< Invalid argument.

#define H5_ERR_VIEW		-100            ///< Something is wrong with the view.
#define H5_ERR_NOENTRY		-101            ///< A lookup failed.

#define H5_ERR_MPI		-201            ///< A MPI error occured.
#define H5_ERR_HDF5		-202            ///< A HDF5 error occured.
#define H5_ERR_H5		-203            ///< Unspecified error in H5 module.
#define H5_ERR_H5PART		-204            ///< Unspecified error in H5Part module.
#define H5_ERR_H5BLOCK		-205            ///< Unspecified error in H5Block module.
#define H5_ERR_H5FED		-206            ///< Unspecified error in H5Fed module.

#define H5_ERR_INTERNAL		-253            ///< Internal error.
#define H5_ERR_NOT_IMPLEMENTED	-254            ///< Function not yet implemented.

/** @}*/

extern h5_err_t h5_errno;

#define h5_error_not_implemented()				     \
	h5_error(						     \
		H5_ERR_NOT_IMPLEMENTED,				     \
		"%s: Function '%s', line %d not yet implemented!",   \
		__FILE__, __func__, __LINE__);

#define h5_error_internal()   \
        h5_error(                                  \
                H5_ERR_INTERNAL,                   \
                "%s: Internal error: %s line %d!", \
                __FILE__, __func__, __LINE__)


VTKH5HUT_EXPORT
h5_err_t
h5_set_errorhandler (
	const h5_errorhandler_t);

VTKH5HUT_EXPORT
h5_errorhandler_t
h5_get_errorhandler (
	void);

VTKH5HUT_EXPORT
h5_err_t
h5_get_errno (
	void);

VTKH5HUT_EXPORT
void
h5_set_errno (
	const h5_err_t);


VTKH5HUT_EXPORT
h5_err_t
h5_report_errorhandler (
        const char *fmt,
        va_list ap
        );

VTKH5HUT_EXPORT
h5_err_t
h5_abort_errorhandler (
        const char *fmt,
        va_list ap
        );

VTKH5HUT_EXPORT
h5_err_t
h5_error (
        const h5_err_t error_no,
        const char *fmt,
        ...
        )
#ifdef __GNUC__
__attribute__ ((format (printf, 2, 3)))
#endif
;

#ifdef __cplusplus
}
#endif

#endif
