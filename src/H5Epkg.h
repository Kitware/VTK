/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose: This file contains declarations which are visible only within
 *          the H5E package.  Source files outside the H5E package should
 *          include H5Eprivate.h instead.
 */
#if !(defined H5E_FRIEND || defined H5E_MODULE)
#error "Do not include this file outside the H5E package!"
#endif

#ifndef H5Epkg_H
#define H5Epkg_H

/* Get package's private header */
#include "H5Eprivate.h"

/* Other private headers needed by this file */

/**************************/
/* Package Private Macros */
/**************************/

/* Amount to indent each error */
#define H5E_INDENT 2

/* Number of entries in an error stack */
#define H5E_MAX_ENTRIES 32

#ifdef H5_HAVE_THREADSAFE
/*
 * The per-thread error stack. pthread_once() initializes a special
 * key that will be used by all threads to create a stack specific to
 * each thread individually. The association of stacks to threads will
 * be handled by the pthread library.
 *
 * In order for this macro to work, H5E__get_my_stack() must be preceded
 * by "H5E_stack_t *estack =".
 */
#define H5E__get_my_stack() H5E__get_stack()
#else /* H5_HAVE_THREADSAFE */
/*
 * The current error stack.
 */
#define H5E__get_my_stack() (H5E_stack_g + 0)
#endif /* H5_HAVE_THREADSAFE */

/****************************/
/* Package Private Typedefs */
/****************************/

/* Some syntactic sugar to make the compiler happy with two different kinds of callbacks */
#ifndef H5_NO_DEPRECATED_SYMBOLS
typedef struct {
    unsigned    vers;          /* Which version callback to use */
    bool        is_default;    /* If the printing function is the library's own. */
    H5E_auto1_t func1;         /* Old-style callback, NO error stack param. */
    H5E_auto2_t func2;         /* New-style callback, with error stack param. */
    H5E_auto1_t func1_default; /* The saved library's default function - old style. */
    H5E_auto2_t func2_default; /* The saved library's default function - new style. */
} H5E_auto_op_t;
#else  /* H5_NO_DEPRECATED_SYMBOLS */
typedef struct {
    H5E_auto2_t func2; /* Only the new style callback function is available. */
} H5E_auto_op_t;
#endif /* H5_NO_DEPRECATED_SYMBOLS */

/* Some syntactic sugar to make the compiler happy with two different kinds of callbacks */
typedef struct {
    unsigned vers; /* Which version callback to use */
    union {
#ifndef H5_NO_DEPRECATED_SYMBOLS
        H5E_walk1_t func1; /* Old-style callback, NO error stack param. */
#endif                     /* H5_NO_DEPRECATED_SYMBOLS */
        H5E_walk2_t func2; /* New-style callback, with error stack param. */
    } u;
} H5E_walk_op_t;

/* Error class */
typedef struct H5E_cls_t {
    bool        app_cls;  /* Whether the class came from app */
    const char *cls_name; /* Name of error class */
    const char *lib_name; /* Name of library within class */
    const char *lib_vers; /* Version of library */
} H5E_cls_t;

/* Major or minor message */
typedef struct H5E_msg_t {
    bool             app_msg; /* Whether the message came from app */
    const char      *msg;     /* Message for error */
    H5E_type_t       type;    /* Type of error (major or minor) */
    const H5E_cls_t *cls;     /* Which error class this message belongs to */
} H5E_msg_t;

/* Error stack entry */
typedef struct H5E_entry_t {
    bool         app_entry; /* Whether the entry came from app */
    H5E_error2_t err;       /* Error entry */
} H5E_entry_t;

/* Error stack */
typedef struct H5E_stack_t {
    size_t        nused;                    /* Num entries currently on stack  */
    H5E_entry_t   entries[H5E_MAX_ENTRIES]; /* Array of error entries */
    H5E_auto_op_t auto_op;                  /* Operator for 'automatic' error reporting */
    void         *auto_data;                /* Callback data for 'automatic error reporting */
    unsigned      paused;                   /* Whether error reporting is paused (>0) for this stack */
} H5E_stack_t;

/*****************************/
/* Package Private Variables */
/*****************************/

#ifndef H5_HAVE_THREADSAFE
/*
 * The current error stack.
 */
H5_DLLVAR H5E_stack_t H5E_stack_g[1];
#endif

/* First & last major and minor error codes registered by the library */
H5_DLLVAR hid_t H5E_first_maj_id_g;
H5_DLLVAR hid_t H5E_last_maj_id_g;
H5_DLLVAR hid_t H5E_first_min_id_g;
H5_DLLVAR hid_t H5E_last_min_id_g;

/******************************/
/* Package Private Prototypes */
/******************************/
#ifdef H5_HAVE_THREADSAFE
H5_DLL H5E_stack_t *H5E__get_stack(void);
#endif /* H5_HAVE_THREADSAFE */
H5_DLL H5E_cls_t   *H5E__register_class(const char *cls_name, const char *lib_name, const char *version);
H5_DLL ssize_t      H5E__get_class_name(const H5E_cls_t *cls, char *name, size_t size);
H5_DLL H5E_msg_t   *H5E__create_msg(H5E_cls_t *cls, H5E_type_t msg_type, const char *msg);
H5_DLL ssize_t      H5E__get_msg(const H5E_msg_t *msg_ptr, H5E_type_t *type, char *msg, size_t size);
H5_DLL void         H5E__set_default_auto(H5E_stack_t *stk);
H5_DLL H5E_stack_t *H5E__get_current_stack(void);
H5_DLL herr_t       H5E__set_current_stack(H5E_stack_t *estack);
H5_DLL ssize_t      H5E__get_num(const H5E_stack_t *err_stack);
H5_DLL herr_t       H5E__push_stack(H5E_stack_t *estack, bool app_entry, const char *file, const char *func,
                                    unsigned line, hid_t cls_id, hid_t maj_id, hid_t min_id, const char *fmt,
                                    va_list *ap);
H5_DLL herr_t       H5E__print(const H5E_stack_t *estack, FILE *stream, bool bk_compat);
H5_DLL herr_t       H5E__print2(hid_t err_stack, FILE *stream);
H5_DLL herr_t       H5E__walk(const H5E_stack_t *estack, H5E_direction_t direction, const H5E_walk_op_t *op,
                              void *client_data);
H5_DLL herr_t       H5E__get_auto(const H5E_stack_t *estack, H5E_auto_op_t *op, void **client_data);
H5_DLL herr_t       H5E__set_auto(H5E_stack_t *estack, const H5E_auto_op_t *op, void *client_data);
H5_DLL herr_t       H5E__pop(H5E_stack_t *err_stack, size_t count);
H5_DLL herr_t       H5E__append_stack(H5E_stack_t *dst_estack, const H5E_stack_t *src_stack);
H5_DLL herr_t       H5E__clear_stack(H5E_stack_t *estack);

#endif /* H5Epkg_H */
