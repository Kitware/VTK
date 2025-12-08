/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5_LOG_H
#define __H5CORE_H5_LOG_H

#include <stdio.h>
#include <stddef.h>


extern const char* H5_VER_STRING;

#include "h5core/h5_types.h"
#include "h5core/h5_err.h"

#define H5_VERBOSE_NONE    	(0)
#define H5_VERBOSE_ERROR   	(1)
#define H5_VERBOSE_WARN    	(2)
#define H5_VERBOSE_INFO    	(3)
#define H5_VERBOSE_DEBUG    	(4)

#define H5_VERBOSE_DEFAULT      H5_VERBOSE_ERROR

#define H5_DEBUG_USER		(1<<2)
#define H5_DEBUG_API		(1<<3)
#define H5_DEBUG_CORE_API	(1<<4)
#define H5_DEBUG_PRIV_API	(1<<5)
#define H5_DEBUG_PRIV_FUNC	(1<<6)
#define H5_DEBUG_HDF5		(1<<7)
#define H5_DEBUG_MPI		(1<<8)
#define H5_DEBUG_MALLOC		(1<<9)
#define H5_DEBUG_CLIB		(1<<10)

#define H5_DEBUG_NONE		(0)
#define H5_DEBUG_ALL		(-1 & ~0x3)


#ifdef __cplusplus
extern "C" {
#endif

extern VTKH5HUT_EXPORT char* h5_rfmts[];
enum h5_rtypes {
	e_int = 0,
	e_ssize_t,
	e_char_p,
	e_void_p,
	e_h5_err_t,
	e_h5_int32_t,
	e_h5_uint32_t,
	e_h5_int64_t,
	e_h5_uint64_t,
	e_h5_id_t,
	e_h5_ssize_t,
	e_h5_errorhandler_t,
	e_h5_file_p,
	e_h5_file_t,
	e_h5_lvl_idx_t,
	e_h5t_iterator_p,
	e_h5_loc_id_t,
	e_h5_loc_idx_t,
	e_hid_t,
	e_H5O_type_t,
	e_h5_glb_elem_p,
    	e_h5_prop_p,
        e_h5_prop_t,
    	e_h5_prop_file_p,
        e_h5_prop_file_t,
	e_herr_t
};

struct call_stack_entry {
	char* name;
	enum h5_rtypes type;
};

struct call_stack {
	int level;
	struct call_stack_entry entry[1024];
};

extern VTKH5HUT_EXPORT h5_int64_t __h5_log_level;
extern VTKH5HUT_EXPORT h5_int64_t __h5_debug_mask;

extern VTKH5HUT_EXPORT struct call_stack h5_call_stack;

// :FIXME: Should go to another header file
VTKH5HUT_EXPORT
h5_err_t
h5_initialize (void);

static inline void
h5_call_stack_push (
        const char* fname,
        enum h5_rtypes type
        ) {
	h5_call_stack.entry[h5_call_stack.level].name = (char *)fname;
	h5_call_stack.entry[h5_call_stack.level].type = type;
	h5_call_stack.level++;
}

static inline const char*
h5_call_stack_pop (
        void
        ) {
	return h5_call_stack.entry[--h5_call_stack.level].name;
}

static inline const char*
h5_call_stack_get_name (
        void
        ) {
	return h5_call_stack.entry[h5_call_stack.level-1].name;
}

static inline const char*
h5_get_funcname (
        void
        ) {
	return h5_call_stack.entry[0].name;
}

static inline enum h5_rtypes
h5_call_stack_get_type (
        void
        ) {
	return h5_call_stack.entry[h5_call_stack.level-1].type;
}

static inline int
h5_call_stack_get_level (
        void
        ) {
	return h5_call_stack.level;
}

static inline const char*
h5_call_stack_reset (
        void
        ) {
	h5_call_stack.level = 0;
	return h5_call_stack.entry[0].name;
}

/*!
   \ingroup h5_core_errorhandling

   Print a debug message to \c stdout.
 */
VTKH5HUT_EXPORT
void
h5_debug (
        const char *fmt,
        ...
        )
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
;

/*!
   \ingroup h5_core_errorhandling

   Print a warning message to \c stderr.
 */
VTKH5HUT_EXPORT
h5_err_t
h5_warn (
        const char *fmt,
        ...
        )
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
;

/*!
   \ingroup h5_core_errorhandling

   Print an informational message to \c stdout.
 */
VTKH5HUT_EXPORT
void
h5_info (
        const char *fmt,
        ...
        )
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
;

VTKH5HUT_EXPORT
h5_err_t
h5_set_loglevel (
	const h5_int64_t);

VTKH5HUT_EXPORT
h5_err_t
h5_set_debug_mask (
	const h5_int64_t);

VTKH5HUT_EXPORT
h5_int64_t
h5_get_loglevel (
	void);

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////////////
// function enter macro
#if defined(NDEBUG)

#define H5_API_ENTER(type, fmt, ...)					\
	type ret_value = (type)H5_ERR;					\
	h5_initialize();						\
	h5_call_stack_reset ();						\
	h5_call_stack_push (__func__,e_##type);

#else   // NDEBUG not defined

#define H5_API_ENTER(type, fmt, ...)					\
	type ret_value = (type)H5_ERR;					\
	h5_initialize();						\
	h5_call_stack_reset ();						\
	h5_call_stack_push (__func__,e_##type);				\
	int __log__ = __h5_debug_mask & H5_DEBUG_API;			\
	if (__log__) {							\
		h5_debug ("(" fmt ")", __VA_ARGS__);			\
	}

#endif
//
//////////////////////////////////////////////////////////////////////////////

#define H5_LEAVE(expr) {						\
		ret_value = expr;					\
		goto done;						\
	}

#define H5_RETURN_ERROR(errno, fmt, ...) {				\
		ret_value = h5_error (errno, fmt, __VA_ARGS__);		\
		goto done;						\
	}

//////////////////////////////////////////////////////////////////////////////
// function return macro
#if defined(NDEBUG)

#define H5_RETURN(expr)							\
	ret_value = expr;						\
	goto done;							\
done:									\
	return ret_value;

#else  // NDEBUG not defined

#define H5_RETURN(expr)							\
	ret_value = expr;						\
	goto done;							\
done:									\
	if (__log__ ) {							\
		char fmt[256];						\
		snprintf (fmt, sizeof(fmt), "return: %s",		\
			  h5_rfmts[h5_call_stack_get_type()]);		\
		h5_debug (fmt, ret_value);				\
		h5_call_stack_pop();					\
	}								\
	return ret_value;

#endif
//
//////////////////////////////////////////////////////////////////////////////

#define H5_API_LEAVE(expr)		H5_LEAVE(expr)
#define H5_API_RETURN(expr)		H5_RETURN(expr);


#define TRY( func )							\
	if ((int64_t)(ptrdiff_t)(func) <= (int64_t)H5_ERR) {		\
		goto done;						\
	}


#endif
