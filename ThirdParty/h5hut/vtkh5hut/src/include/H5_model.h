/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5_MODEL_H
#define __H5_MODEL_H

#include "h5core/h5_log.h"
#include "h5core/h5_model.h"

/**
   \addtogroup h5_model
   @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
  Define format of the step/iteration names.

  Example: ==H5SetStepNameFormat( f, "Step", 6 )== defines step names 
  like ==Step#000042==.

  \return   \c H5_SUCCESS on success
  \return   \c H5_FAILURE on error
*/
static inline h5_err_t
H5SetStepNameFormat (
	const h5_file_t f,	///< [in] file handle
	const char* name,	///< [in] prefix, defaults to \c Step
	const h5_int64_t width	///< [in] width of step/iteration number
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, name='%s', width=%lld",
		      (h5_file_p)f, name, (long long) width);
	H5_API_RETURN (h5_set_iteration_name_fmt (f, name, width));
}

/**
  Get format of the step/iteration names.

  \return   \c H5_SUCCESS on success
  \return   \c H5_FAILURE on error
*/
static inline h5_err_t
H5GetStepNameFormat (
	const h5_file_t f,	///< [in]  file handle
	char* name,		///< [out] prefix
	const h5_size_t l_name,	///< [in]  length of buffer name
	int* width		///< [out] width of step/iteration number
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, name=%p, l_name=%llu, width=%p",
		      (h5_file_p)f, name, (unsigned long long)l_name, width);
	H5_API_RETURN (h5_get_iteration_name_fmt (f, name, l_name, width));
}

/**
  Set the current step/iteration.

  \return   \c H5_SUCCESS on success
  \return   \c H5_FAILURE on error
*/
static inline h5_err_t
H5SetStep (
	const h5_file_t f,	///< [in]  file handle.
	const h5_id_t step	///< [in]  step/iteration to set.
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, step=%lld",
                      (h5_file_p)f, (long long)step);
	H5_API_RETURN (h5_set_iteration (f, step));
}

/**
  Get current step/iteration.

  \return   Step/iteration number
  \return   \c H5_FAILURE on error
*/
static inline h5_id_t
H5GetStep (
	const h5_file_t f	///< [in] file handle.
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p",
                      (h5_file_p)f);
	H5_API_RETURN (h5_get_iteration (f));
}

/**
  Get the number of steps/iterations that are currently stored
  in the file \c f.

  It works for both reading and writing of files, but is probably
  only typically used when you are reading.

  \return	Number of steps/iterations
  \return       \c H5_FAILURE on error.
*/
static inline h5_ssize_t
H5GetNumSteps (
	const h5_file_t f	///< [in] file handle.
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p",
                      (h5_file_p)f);
	H5_API_RETURN (h5_get_num_iterations(f));
}

/**
  Query whether a particular step/iteration already exists in the file.

  \return      true (value \c >0) if step/iteration exists
  \return      false (\c 0) if step/iteration does not exist
  \return      \c H5_FAILURE on error
*/
static inline h5_err_t
H5HasStep (
	const h5_file_t f,	///< [in] file handle.
	h5_id_t stepno          ///< [in] step number to query for existence.
	) {
	H5_API_ENTER (h5_err_t, 
		      "f=%p, stepno=%lld",
		      (h5_file_p)f, (long long)stepno);
	H5_API_RETURN (h5_has_iteration (f, stepno));
}

/**
  Get the number of processors.

  \return   Number of processors.
  \return   \c H5_FAILURE on error.
 */
static inline int
H5GetNumProcs (
	const h5_file_t f	///< [in] file handle.
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p",
                      (h5_file_p)f);
	H5_API_RETURN (h5_get_num_procs(f));
}

/**
   @}
*/
	
#ifdef __cplusplus
}
#endif

#endif
