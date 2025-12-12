/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5_LOG_H
#define __H5_LOG_H

#include "h5core/h5_types.h"
#include "h5core/h5_log.h"

#if H5HUT_API_VERSION == 2
#define H5SetVerbosityLevel2 H5SetVerbosityLevel
#elif H5HUT_API_VERSION == 1
#define H5SetVerbosityLevel1 H5SetVerbosityLevel
#endif

/**
   \addtogroup  h5_log
   @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
  Set verbosity level to \c level.

  Verbosity levels are:
  - \c H5_VERBOSE_NONE: be quiet
  - \c H5_VERBOSE_ERROR: output error messages
  - \c H5_VERBOSE_WARN: output error messages and warning
  - \c H5_VERBOSE_INFO: output error messages, warnings and informational messages

  The default verbosity level is \c H5_VERBOSE_ERROR.

  \return \c H5_SUCCESS

  \see H5GetVerbosityLevel()

  \note 
  | Release    | Change                               |
  | :------    | :-----			  	      |
  | \c 1.99.15 | Function cannot be used to set the debug level. |
*/
static inline h5_err_t
H5SetVerbosityLevel2 (
	const h5_int32_t level     ///< [in] verbosity level.
	) {
	return h5_set_loglevel (level & 0x03);
}

/**
  Set verbosity and debug level to \p level.

  Verbosity levels are:
  - \c H5_VERBOSE_NONE: be quiet
  - \c H5_VERBOSE_ERROR: output error messages
  - \c H5_VERBOSE_WARN: output error messages and warning
  - \c H5_VERBOSE_INFO: output error messages, warnings and informational messages

  The default verbosity level is \c H5_VERBOSE_ERROR.

  \return \c H5_SUCCESS

  \see H5GetVerbosityLevel()
*/
static inline h5_err_t
H5SetVerbosityLevel1 (
	const h5_int64_t level     ///< [in] verbosity level.
	) {
	return h5_set_loglevel (level);
}

/**
  Get verbosity level.

  \return   verbosity level

  \see H5SetVerbosityLevel()
*/
static inline h5_int64_t
H5GetVerbosityLevel (
	void
	) {
	return h5_get_loglevel () & 0x03;
}

///< @}


/**
   \addtogroup h5_debug
   @{
*/

/**
  Set debug mask. The debug mask is an or'ed value of

  - \c H5_DEBUG_NONE:	    debug output disabled
  - \c H5_DEBUG_API:	    C-API calls
  - \c H5_DEBUG_CORE_API:   core API calls.
  - \c H5_DEBUG_PRIV_API:   private API calls
  - \c H5_DEBUG_PRIV_FUNC:  static functions
  - \c H5_DEBUG_HDF5:	    HDF5 wrapper calls
  - \c H5_DEBUG_MPI:	    MPI wrapper calls
  - \c H5_DEBUG_MALLOC:	    memory allocation
  - \c H5_DEBUG_ALL:	    enable all

  \return \c H5_SUCCESS

  \see H5GetDebugMask()

  \note 
  | Release    | Change                               |
  | :------    | :-----			  	      |
  | \c 1.99.15 | Function introduced in this release. |
*/
static inline h5_err_t
H5SetDebugMask (
	const h5_int64_t mask     ///< [in] debug mask
	) {
	return h5_set_debug_mask (mask);
}

/**
  Get debug mask.

  \return   debug mask

  \see H5SetDebugMask()

  \note 
  | Release    | Change                               |
  | :------    | :-----			  	      |
  | \c 1.99.15 | Function introduced in this release. |
*/
static inline h5_int32_t
H5GetDebugMask (
	void
	) {
	return (h5_get_loglevel () & ~0x03);
}

#ifdef __cplusplus
}
#endif

///< @}


#endif

