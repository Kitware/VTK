/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5_DEBUG_H
#define __PRIVATE_H5_DEBUG_H

#include "h5core/h5_types.h"
#include "h5core/h5_log.h"
#include "private/h5_init.h"

#if defined(NDEBUG)

#define __FUNC_ENTER(type, mask, fmt, ...)	\
	type ret_value = (type)H5_ERR;

#else   // NDEBUG not defined

#define __FUNC_ENTER(type, mask, fmt, ...)				\
	type ret_value = (type)H5_ERR;					\
	int __log__ = __h5_debug_mask & mask;				\
	if (__log__) {							\
		h5_call_stack_push (__func__,e_##type);			\
		h5_debug ("(" fmt ")", __VA_ARGS__);			\
	}

#endif

#define H5_CORE_API_ENTER(type, fmt, ...)				\
	if (!h5_initialized) {						\
		h5_initialize();					\
	}								\
	__FUNC_ENTER(type, H5_DEBUG_CORE_API, fmt, __VA_ARGS__)

#define H5_PRIV_API_ENTER(type, fmt, ...)				\
	__FUNC_ENTER(type, H5_DEBUG_PRIV_API, fmt, __VA_ARGS__)

#define H5_PRIV_FUNC_ENTER(type, fmt, ...)				\
	__FUNC_ENTER(type, H5_DEBUG_PRIV_FUNC, fmt, __VA_ARGS__ )

#ifdef NDEBUG
#define H5_INLINE_FUNC_ENTER(type)			\
	type ret_value = (type)H5_ERR;

#else
#define H5_INLINE_FUNC_ENTER(type)			\
	type ret_value = (type)H5_ERR; int __log__ = 0;
#endif
	
#define HDF5_WRAPPER_ENTER(type, fmt, ...)			\
	__FUNC_ENTER(type, H5_DEBUG_HDF5, fmt, __VA_ARGS__ )

#endif
