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

/*-------------------------------------------------------------------------
 *
 * Created:	H5Eint.c
 *
 * Purpose:	General use, "internal" routines for error handling.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5Emodule.h" /* This source code file is part of the H5E module */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions                        */
#include "H5Epkg.h"      /* Error handling                           */
#include "H5FLprivate.h" /* Free lists                               */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5MMprivate.h" /* Memory management                        */
#include "H5TSprivate.h" /* Thread stuff                             */

/****************/
/* Local Macros */
/****************/

/* HDF5 error class */
#define H5E_CLS_NAME     "HDF5"
#define H5E_CLS_LIB_NAME "HDF5"

/******************/
/* Local Typedefs */
/******************/

/* Printing information */
typedef struct H5E_print_t {
    FILE     *stream;
    H5E_cls_t cls;
} H5E_print_t;

/********************/
/* Package Typedefs */
/********************/

/********************/
/* Local Prototypes */
/********************/
#ifndef H5_NO_DEPRECATED_SYMBOLS
static herr_t H5E__walk1_cb(int n, H5E_error1_t *err_desc, void *client_data);
#endif /* H5_NO_DEPRECATED_SYMBOLS */
static herr_t H5E__walk2_cb(unsigned n, const H5E_error2_t *err_desc, void *client_data);
static herr_t H5E__copy_stack_entry(H5E_entry_t *dst_entry, const H5E_entry_t *src_entry);
static herr_t H5E__set_stack_entry(H5E_error2_t *err_entry, const char *file, const char *func, unsigned line,
                                   hid_t cls_id, hid_t maj_id, hid_t min_id, const char *desc, va_list *ap);
static herr_t H5E__clear_entries(H5E_stack_t *estack, size_t nentries);
static herr_t H5E__unregister_class(H5E_cls_t *cls, void **request);
static int    H5E__close_msg_cb(void *obj_ptr, hid_t obj_id, void *udata);
static void   H5E__free_msg(H5E_msg_t *msg);
static herr_t H5E__close_msg(H5E_msg_t *err, void **request);
static herr_t H5E__close_stack(H5E_stack_t *err_stack, void **request);

/*********************/
/* Package Variables */
/*********************/

#ifndef H5_HAVE_THREADSAFE
/*
 * The current error stack.
 */
H5E_stack_t H5E_stack_g[1];
#endif /* H5_HAVE_THREADSAFE */

/* Declare a free list to manage the H5E_stack_t struct */
H5FL_DEFINE(H5E_stack_t);

/* Declare a free list to manage the H5E_cls_t struct */
H5FL_DEFINE_STATIC(H5E_cls_t);

/* Declare a free list to manage the H5E_msg_t struct */
H5FL_DEFINE_STATIC(H5E_msg_t);

/*****************************/
/* Library Private Variables */
/*****************************/

/* HDF5 error class ID */
hid_t H5E_ERR_CLS_g = FAIL;

/*
 * Predefined errors. These are initialized at runtime in H5E_init_interface()
 * in this source file.
 */
/* Include the automatically generated error code definitions */
#include "H5Edefin.h"

/*******************/
/* Local Variables */
/*******************/

#ifdef H5_HAVE_PARALLEL
/*
 * variables used for MPI error reporting
 */
char H5E_mpi_error_str[MPI_MAX_ERROR_STRING];
int  H5E_mpi_error_str_len;
#endif /* H5_HAVE_PARALLEL */

/* First & last major and minor error codes registered by the library */
hid_t H5E_first_maj_id_g = H5I_INVALID_HID;
hid_t H5E_last_maj_id_g  = H5I_INVALID_HID;
hid_t H5E_first_min_id_g = H5I_INVALID_HID;
hid_t H5E_last_min_id_g  = H5I_INVALID_HID;

/* Error class ID class */
static const H5I_class_t H5I_ERRCLS_CLS[1] = {{
    H5I_ERROR_CLASS,                  /* ID class value */
    0,                                /* Class flags */
    0,                                /* # of reserved IDs for class */
    (H5I_free_t)H5E__unregister_class /* Callback routine for closing objects of this class */
}};

/* Error message ID class */
static const H5I_class_t H5I_ERRMSG_CLS[1] = {{
    H5I_ERROR_MSG,             /* ID class value */
    0,                         /* Class flags */
    0,                         /* # of reserved IDs for class */
    (H5I_free_t)H5E__close_msg /* Callback routine for closing objects of this class */
}};

/* Error stack ID class */
static const H5I_class_t H5I_ERRSTK_CLS[1] = {{
    H5I_ERROR_STACK,             /* ID class value */
    0,                           /* Class flags */
    0,                           /* # of reserved IDs for class */
    (H5I_free_t)H5E__close_stack /* Callback routine for closing objects of this class */
}};

/* Library's error class */
static const H5E_cls_t H5E_err_cls_s = {false, H5E_CLS_NAME, H5E_CLS_LIB_NAME, H5_VERS_STR};

/* Include the automatically generated major error message definitions */
#include "H5Emajdef.h"

/* Include the automatically generated minor error message definitions */
#include "H5Emindef.h"

/*-------------------------------------------------------------------------
 * Function:    H5E_init
 *
 * Purpose:     Initialize the interface from some other layer.
 *
 * Return:      Success:        non-negative
 *              Failure:        negative
 *-------------------------------------------------------------------------
 */
herr_t
H5E_init(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Initialize the ID group for the error class IDs */
    if (H5I_register_type(H5I_ERRCLS_CLS) < 0)
        HGOTO_ERROR(H5E_ID, H5E_CANTINIT, FAIL, "unable to initialize ID group");

    /* Initialize the ID group for the major error IDs */
    if (H5I_register_type(H5I_ERRMSG_CLS) < 0)
        HGOTO_ERROR(H5E_ID, H5E_CANTINIT, FAIL, "unable to initialize ID group");

    /* Initialize the ID group for the error stacks */
    if (H5I_register_type(H5I_ERRSTK_CLS) < 0)
        HGOTO_ERROR(H5E_ID, H5E_CANTINIT, FAIL, "unable to initialize ID group");

#ifndef H5_HAVE_THREADSAFE
    H5E__set_default_auto(H5E_stack_g);
#endif /* H5_HAVE_THREADSAFE */

    /* Register the HDF5 error class */
    if ((H5E_ERR_CLS_g = H5I_register(H5I_ERROR_CLASS, &H5E_err_cls_s, false)) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error class");

/* Include the automatically generated error code initialization */
#include "H5Einit.h"

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5E_term_package
 *
 * Purpose:     Terminates the H5E interface
 *
 * Return:      Success:    Positive if anything is done that might
 *                          affect other interfaces; zero otherwise.
 *
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
int
H5E_term_package(void)
{
    int n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    int64_t ncls, nmsg, nstk;

    /* Check if there are any open error stacks, classes or messages */
    ncls = H5I_nmembers(H5I_ERROR_CLASS);
    nmsg = H5I_nmembers(H5I_ERROR_MSG);
    nstk = H5I_nmembers(H5I_ERROR_STACK);

    if ((ncls + nmsg + nstk) > 0) {
        /* Clear the default error stack. Note that
         * the following H5I_clear_type calls do not
         * force the clears and will not be able to
         * clear any error message IDs that are still
         * in use by the default error stack unless we
         * clear that stack manually.
         *
         * Error message IDs will typically still be
         * in use by the default error stack when the
         * application does H5E_BEGIN/END_TRY cleanup
         * at the very end.
         */
        H5E_clear_stack();

        /* Clear any outstanding error stacks */
        if (nstk > 0)
            (void)H5I_clear_type(H5I_ERROR_STACK, false, false);

        /* Clear all the error classes */
        if (ncls > 0) {
            (void)H5I_clear_type(H5I_ERROR_CLASS, false, false);

            /* Reset the HDF5 error class, if its been closed */
            if (H5I_nmembers(H5I_ERROR_CLASS) == 0)
                H5E_ERR_CLS_g = H5I_INVALID_HID;
        } /* end if */

        /* Clear all the error messages */
        if (nmsg > 0) {
            (void)H5I_clear_type(H5I_ERROR_MSG, false, false);

            /* Reset the HDF5 error messages, if they've been closed */
            if (H5I_nmembers(H5I_ERROR_MSG) == 0) {
/* Include the automatically generated error code termination */
#include "H5Eterm.h"
            } /* end if */
        }     /* end if */

        n++; /*H5I*/
    }        /* end if */
    else {
        /* Destroy the error class, message, and stack id groups */
        n += (H5I_dec_type_ref(H5I_ERROR_STACK) > 0);
        n += (H5I_dec_type_ref(H5I_ERROR_CLASS) > 0);
        n += (H5I_dec_type_ref(H5I_ERROR_MSG) > 0);

    } /* end else */

    FUNC_LEAVE_NOAPI(n)
} /* end H5E_term_package() */

#ifdef H5_HAVE_THREADSAFE
/*-------------------------------------------------------------------------
 * Function:    H5E__get_stack
 *
 * Purpose:     Support function for H5E__get_my_stack() to initialize and
 *              acquire per-thread error stack.
 *
 * Return:      Success:    Pointer to an error stack struct (H5E_t *)
 *
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
H5E_stack_t *
H5E__get_stack(void)
{
    H5E_stack_t *estack = NULL;

    FUNC_ENTER_PACKAGE_NOERR

    estack = (H5E_stack_t *)H5TS_get_thread_local_value(H5TS_errstk_key_g);

    if (!estack) {
        /* No associated value with current thread - create one */
#ifdef H5_HAVE_WIN_THREADS
        /* Win32 has to use LocalAlloc to match the LocalFree in DllMain */
        estack = (H5E_stack_t *)LocalAlloc(LPTR, sizeof(H5E_stack_t));
#else
        /* Use malloc here since this has to match the free in the
         * destructor and we want to avoid the codestack there.
         */
        estack = (H5E_stack_t *)malloc(sizeof(H5E_stack_t));
#endif /* H5_HAVE_WIN_THREADS */
        assert(estack);

        /* Set the thread-specific info */
        H5E__set_default_auto(estack);

        /* (It's not necessary to release this in this API, it is
         *      released by the "key destructor" set up in the H5TS
         *      routines.  See calls to pthread_key_create() in H5TS.c -QAK)
         */
        H5TS_set_thread_local_value(H5TS_errstk_key_g, (void *)estack);
    } /* end if */

    /* Set return value */
    FUNC_LEAVE_NOAPI(estack)
} /* end H5E__get_stack() */
#endif /* H5_HAVE_THREADSAFE */

/*-------------------------------------------------------------------------
 * Function:    H5E__free_class
 *
 * Purpose:     Private function to free an error class.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5E__free_class(H5E_cls_t *cls)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Check arguments */
    assert(cls);

    /* Free resources, if application registered this class */
    if (cls->app_cls) {
        /* Free error class structure */
        cls->cls_name = H5MM_xfree_const(cls->cls_name);
        cls->lib_name = H5MM_xfree_const(cls->lib_name);
        cls->lib_vers = H5MM_xfree_const(cls->lib_vers);
        cls           = H5FL_FREE(H5E_cls_t, cls);
    }

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5E__free_class() */

/*-------------------------------------------------------------------------
 * Function:    H5E__register_class
 *
 * Purpose:     Private function to register an error class.
 *
 * Return:      Success:    Pointer to an error class struct
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
H5E_cls_t *
H5E__register_class(const char *cls_name, const char *lib_name, const char *version)
{
    H5E_cls_t *cls       = NULL; /* Pointer to error class */
    H5E_cls_t *ret_value = NULL; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    assert(cls_name);
    assert(lib_name);
    assert(version);

    /* Allocate space for new error class */
    if (NULL == (cls = H5FL_CALLOC(H5E_cls_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Application registered class */
    cls->app_cls = true;

    /* Duplicate string information */
    if (NULL == (cls->cls_name = strdup(cls_name)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    if (NULL == (cls->lib_name = strdup(lib_name)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    if (NULL == (cls->lib_vers = strdup(version)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Set the return value */
    ret_value = cls;

done:
    if (!ret_value)
        if (cls && H5E__free_class(cls) < 0)
            HDONE_ERROR(H5E_ERROR, H5E_CANTRELEASE, NULL, "unable to free error class");

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__register_class() */

/*-------------------------------------------------------------------------
 * Function:    H5E__unregister_class
 *
 * Purpose:     Private function to close an error class.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5E__unregister_class(H5E_cls_t *cls, void H5_ATTR_UNUSED **request)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    assert(cls);

    /* Iterate over all the messages and delete those in this error class */
    if (H5I_iterate(H5I_ERROR_MSG, H5E__close_msg_cb, cls, false) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_BADITER, FAIL, "unable to free all messages in this error class");

    /* Free error class structure */
    if (H5E__free_class(cls) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTRELEASE, FAIL, "unable to free error class");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__unregister_class() */

/*-------------------------------------------------------------------------
 * Function:    H5E__get_class_name
 *
 * Purpose:     Private function to retrieve error class name.
 *
 * Return:      Success:    Name length (zero means no name)
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5E__get_class_name(const H5E_cls_t *cls, char *name, size_t size)
{
    ssize_t len = -1; /* Length of error class's name */

    FUNC_ENTER_PACKAGE_NOERR

    /* Check arguments */
    assert(cls);

    /* Get the class's name */
    len = (ssize_t)strlen(cls->cls_name);

    /* Set the user's buffer, if provided */
    if (name) {
        strncpy(name, cls->cls_name, size);
        if ((size_t)len >= size)
            name[size - 1] = '\0';
    } /* end if */

    /* Return the full length */
    FUNC_LEAVE_NOAPI(len)
} /* end H5E__get_class_name() */

/*-------------------------------------------------------------------------
 * Function:    H5E__close_msg_cb
 *
 * Purpose:     H5I_iterate callback function to close error messages in the
 *              error class.
 *
 * Return:      Success:    H5_ITER_CONT (0)
 *              Failure:    H5_ITER_ERROR (-1)
 *
 *-------------------------------------------------------------------------
 */
static int
H5E__close_msg_cb(void *obj_ptr, hid_t obj_id, void *udata)
{
    H5E_msg_t *err_msg   = (H5E_msg_t *)obj_ptr;
    H5E_cls_t *cls       = (H5E_cls_t *)udata;
    int        ret_value = H5_ITER_CONT; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    assert(err_msg);

    /* Close the message if it is in the class being closed */
    if (err_msg->cls == cls) {
        if (H5E__close_msg(err_msg, NULL) < 0)
            HGOTO_ERROR(H5E_ERROR, H5E_CANTCLOSEOBJ, H5_ITER_ERROR, "unable to close error message");
        if (NULL == H5I_remove(obj_id))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTREMOVE, H5_ITER_ERROR, "unable to remove error message");
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__close_msg_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5E__free_msg
 *
 * Purpose:     Private function to free an application error message.
 *
 * Return:      none
 *
 *-------------------------------------------------------------------------
 */
static void
H5E__free_msg(H5E_msg_t *msg)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Check arguments */
    assert(msg);
    assert(msg->app_msg);

    /* Free resources */
    msg->msg = H5MM_xfree_const(msg->msg);
    msg      = H5FL_FREE(H5E_msg_t, msg);

    FUNC_LEAVE_NOAPI_VOID
} /* end H5E__free_msg() */

/*-------------------------------------------------------------------------
 * Function:    H5E__close_msg
 *
 * Purpose:     Private function to close an error message.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5E__close_msg(H5E_msg_t *err, void H5_ATTR_UNUSED **request)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Check arguments */
    assert(err);

    /* Free resources, if application registered this message */
    if (err->app_msg)
        /* Free message */
        H5E__free_msg(err);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5E__close_msg() */

/*-------------------------------------------------------------------------
 * Function:    H5E__create_msg
 *
 * Purpose:     Private function to create a major or minor error.
 *
 * Return:      Success:    Pointer to a message struct
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
H5E_msg_t *
H5E__create_msg(H5E_cls_t *cls, H5E_type_t msg_type, const char *msg_str)
{
    H5E_msg_t *msg       = NULL; /* Pointer to new error message */
    H5E_msg_t *ret_value = NULL; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    assert(cls);
    assert(msg_type == H5E_MAJOR || msg_type == H5E_MINOR);
    assert(msg_str);

    /* Allocate new message object */
    if (NULL == (msg = H5FL_CALLOC(H5E_msg_t)))
        HGOTO_ERROR(H5E_ERROR, H5E_CANTALLOC, NULL, "memory allocation failed");

    /* Fill new message object */
    msg->app_msg = true;
    msg->cls     = cls;
    msg->type    = msg_type;
    if (NULL == (msg->msg = strdup(msg_str)))
        HGOTO_ERROR(H5E_ERROR, H5E_CANTALLOC, NULL, "memory allocation failed");

    /* Set return value */
    ret_value = msg;

done:
    if (!ret_value)
        if (msg)
            H5E__free_msg(msg);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__create_msg() */

/*-------------------------------------------------------------------------
 * Function:    H5E__get_current_stack
 *
 * Purpose:     Private function to register an error stack.
 *
 * Return:      Success:    Pointer to an error class struct
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
H5E_stack_t *
H5E__get_current_stack(void)
{
    H5E_stack_t *current_stack;      /* Pointer to the current error stack */
    H5E_stack_t *estack_copy = NULL; /* Pointer to new error stack to return */
    unsigned     u;                  /* Local index variable */
    H5E_stack_t *ret_value = NULL;   /* Return value */

    FUNC_ENTER_PACKAGE

    /* Get a pointer to the current error stack */
    if (NULL == (current_stack = H5E__get_my_stack()))
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, NULL, "can't get current error stack");

    /* Allocate a new error stack */
    if (NULL == (estack_copy = H5FL_CALLOC(H5E_stack_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Make a copy of current error stack */
    estack_copy->nused = current_stack->nused;
    for (u = 0; u < current_stack->nused; u++)
        if (H5E__copy_stack_entry(&estack_copy->entries[u], &current_stack->entries[u]) < 0)
            HGOTO_ERROR(H5E_ERROR, H5E_CANTSET, NULL, "can't set error entry");

    /* Copy the "automatic" error reporting information */
    estack_copy->auto_op   = current_stack->auto_op;
    estack_copy->auto_data = current_stack->auto_data;

    /* Empty current error stack */
    H5E__clear_stack(current_stack);

    /* Set the return value */
    ret_value = estack_copy;

done:
    if (ret_value == NULL)
        if (estack_copy)
            estack_copy = H5FL_FREE(H5E_stack_t, estack_copy);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__get_current_stack() */

/*-------------------------------------------------------------------------
 * Function:    H5E__set_current_stack
 *
 * Purpose:     Private function to replace an error stack.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E__set_current_stack(H5E_stack_t *estack)
{
    H5E_stack_t *current_stack;       /* Default error stack */
    unsigned     u;                   /* Local index variable */
    herr_t       ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    assert(estack);

    /* Get a pointer to the current error stack */
    if (NULL == (current_stack = H5E__get_my_stack()))
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack");

    /* Empty current error stack */
    H5E__clear_stack(current_stack);

    /* Copy new stack to current error stack */
    current_stack->nused = estack->nused;
    for (u = 0; u < current_stack->nused; u++)
        if (H5E__copy_stack_entry(&current_stack->entries[u], &estack->entries[u]) < 0)
            HGOTO_ERROR(H5E_ERROR, H5E_CANTSET, FAIL, "can't set error entry");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__set_current_stack() */

/*-------------------------------------------------------------------------
 * Function:    H5E__close_stack
 *
 * Purpose:     Private function to close an error stack.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5E__close_stack(H5E_stack_t *estack, void H5_ATTR_UNUSED **request)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Sanity check */
    assert(estack);

    /* Release the stack's error information */
    H5E__clear_stack(estack);

    /* Free the stack structure */
    estack = H5FL_FREE(H5E_stack_t, estack);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5E__close_stack() */

/*-------------------------------------------------------------------------
 * Function:    H5E__get_num
 *
 * Purpose:     Private function to retrieve number of errors in error stack.
 *
 * Return:      Success:    The number of errors
 *              Failure:    -1 (can't fail at this time)
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5E__get_num(const H5E_stack_t *estack)
{
    FUNC_ENTER_PACKAGE_NOERR

    assert(estack);

    FUNC_LEAVE_NOAPI((ssize_t)estack->nused)
} /* end H5E__get_num() */

/*-------------------------------------------------------------------------
 * Function:    H5E__print2
 *
 * Purpose:     Internal helper routine for H5Eprint2.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E__print2(hid_t err_stack, FILE *stream)
{
    H5E_stack_t *estack;              /* Error stack to operate on */
    herr_t       ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Need to check for errors */
    if (err_stack == H5E_DEFAULT) {
        if (NULL == (estack = H5E__get_my_stack()))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack");
    } /* end if */
    else {
        /* Only clear the error stack if it's not the default stack */
        H5E_clear_stack();

        if (NULL == (estack = (H5E_stack_t *)H5I_object_verify(err_stack, H5I_ERROR_STACK)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a error stack ID");
    } /* end else */

    /* Print error stack */
    if (H5E__print(estack, stream, false) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTLIST, FAIL, "can't display error stack");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__print2() */

/*-------------------------------------------------------------------------
 * Function:    H5E__append_stack
 *
 * Purpose:     Private function to append error stacks.
 *
 * Return:      Non-negative value on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E__append_stack(H5E_stack_t *dst_stack, const H5E_stack_t *src_stack)
{
    unsigned u;                   /* Local index variable */
    herr_t   ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity checks */
    assert(dst_stack);
    assert(src_stack);

    /* Copy the errors from the source stack to the destination stack */
    for (u = 0; u < src_stack->nused; u++) {
        /* Copy error stack entry */
        if (H5E__copy_stack_entry(&dst_stack->entries[dst_stack->nused], &src_stack->entries[u]) < 0)
            HGOTO_ERROR(H5E_ERROR, H5E_CANTSET, FAIL, "can't set error entry");

        /* Increment # of errors in destination stack */
        dst_stack->nused++;

        /* Check for destination stack full */
        if (dst_stack->nused >= H5E_MAX_ENTRIES)
            break;
    } /* end for */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__append_stack() */

/*--------------------------------------------------------------------------
 * Function:    H5E__set_default_auto
 *
 * Purpose:     Initialize error stack to library default
 *
 * Return:      SUCCEED/FAIL
 *
 *--------------------------------------------------------------------------
 */
void
H5E__set_default_auto(H5E_stack_t *stk)
{
    FUNC_ENTER_PACKAGE_NOERR

    stk->nused = 0;

#ifndef H5_NO_DEPRECATED_SYMBOLS
#ifdef H5_USE_16_API_DEFAULT
    stk->auto_op.vers = 1;
#else  /* H5_USE_16_API */
    stk->auto_op.vers = 2;
#endif /* H5_USE_16_API_DEFAULT */

    stk->auto_op.func1 = stk->auto_op.func1_default = (H5E_auto1_t)H5Eprint1;
    stk->auto_op.func2 = stk->auto_op.func2_default = (H5E_auto2_t)H5E__print2;
    stk->auto_op.is_default                         = true;
#else  /* H5_NO_DEPRECATED_SYMBOLS */
    stk->auto_op.func2 = (H5E_auto2_t)H5E__print2;
#endif /* H5_NO_DEPRECATED_SYMBOLS */

    stk->auto_data = NULL;
    stk->paused    = 0;

    FUNC_LEAVE_NOAPI_VOID
} /* end H5E__set_default_auto() */

/*-------------------------------------------------------------------------
 * Function:    H5E__get_msg
 *
 * Purpose:     Private function to retrieve an error message.
 *
 * Return:      Success:    Message length (zero means no message)
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5E__get_msg(const H5E_msg_t *msg, H5E_type_t *type, char *msg_str, size_t size)
{
    ssize_t len = -1; /* Length of error message */

    FUNC_ENTER_PACKAGE_NOERR

    /* Check arguments */
    assert(msg);

    /* Get the length of the message string */
    len = (ssize_t)strlen(msg->msg);

    /* Copy the message into the user's buffer, if given */
    if (msg_str) {
        strncpy(msg_str, msg->msg, size);
        if ((size_t)len >= size)
            msg_str[size - 1] = '\0';
    } /* end if */

    /* Give the message type, if asked */
    if (type)
        *type = msg->type;

    /* Set the return value to the full length of the message */
    FUNC_LEAVE_NOAPI(len)
} /* end H5E__get_msg() */

#ifndef H5_NO_DEPRECATED_SYMBOLS

/*-------------------------------------------------------------------------
 * Function:    H5E__walk1_cb
 *
 * Purpose:     This function is for backward compatibility.
 *              This is a default error stack traversal callback function
 *              that prints error messages to the specified output stream.
 *              This function is for backward compatibility with v1.6.
 *              It is not meant to be called directly but rather as an
 *              argument to the H5Ewalk() function.  This function is called
 *              also by H5Eprint().  Application writers are encouraged to
 *              use this function as a model for their own error stack
 *              walking functions.
 *
 *              N is a counter for how many times this function has been
 *              called for this particular traversal of the stack.  It always
 *              begins at zero for the first error on the stack (either the
 *              top or bottom error, or even both, depending on the traversal
 *              direction and the size of the stack).
 *
 *              ERR_DESC is an error description.  It contains all the
 *              information about a particular error.
 *
 *              CLIENT_DATA is the same pointer that was passed as the
 *              CLIENT_DATA argument of H5Ewalk().  It is expected to be a
 *              file pointer (or stderr if null).
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5E__walk1_cb(int n, H5E_error1_t *err_desc, void *client_data)
{
    H5E_print_t     *eprint = (H5E_print_t *)client_data;
    FILE            *stream;                             /* I/O stream to print output to */
    const H5E_cls_t *cls_ptr;                            /* Pointer to error class */
    H5E_msg_t       *maj_ptr;                            /* Pointer to major error info */
    H5E_msg_t       *min_ptr;                            /* Pointer to minor error info */
    const char      *maj_str   = "No major description"; /* Major error description */
    const char      *min_str   = "No minor description"; /* Minor error description */
    bool             have_desc = true; /* Flag to indicate whether the error has a "real" description */
    herr_t           ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE_NOERR

    /* Check arguments */
    assert(err_desc);

    /* If no client data was passed, output to stderr */
    if (!client_data)
        stream = stderr;
    else
        stream = eprint->stream;

    /* Get descriptions for the major and minor error numbers */
    maj_ptr = (H5E_msg_t *)H5I_object_verify(err_desc->maj_num, H5I_ERROR_MSG);
    min_ptr = (H5E_msg_t *)H5I_object_verify(err_desc->min_num, H5I_ERROR_MSG);

    /* Check for bad pointer(s), but can't issue error, just leave */
    if (!maj_ptr || !min_ptr)
        HGOTO_DONE(FAIL);

    if (maj_ptr->msg)
        maj_str = maj_ptr->msg;
    if (min_ptr->msg)
        min_str = min_ptr->msg;

    /* Get error class info */
    cls_ptr = maj_ptr->cls;

    /* Print error class header if new class */
    if (eprint->cls.lib_name == NULL || strcmp(cls_ptr->lib_name, eprint->cls.lib_name) != 0) {
        /* update to the new class information */
        if (cls_ptr->cls_name)
            eprint->cls.cls_name = cls_ptr->cls_name;
        if (cls_ptr->lib_name)
            eprint->cls.lib_name = cls_ptr->lib_name;
        if (cls_ptr->lib_vers)
            eprint->cls.lib_vers = cls_ptr->lib_vers;

        fprintf(stream, "%s-DIAG: Error detected in %s (%s)",
                (cls_ptr->cls_name ? cls_ptr->cls_name : "(null)"),
                (cls_ptr->lib_name ? cls_ptr->lib_name : "(null)"),
                (cls_ptr->lib_vers ? cls_ptr->lib_vers : "(null)"));

        /* try show the process or thread id in multiple processes cases*/
#ifdef H5_HAVE_PARALLEL
        {
            int mpi_rank, mpi_initialized, mpi_finalized;

            MPI_Initialized(&mpi_initialized);
            MPI_Finalized(&mpi_finalized);

            if (mpi_initialized && !mpi_finalized) {
                MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
                fprintf(stream, " MPI-process %d", mpi_rank);
            } /* end if */
#ifdef H5_HAVE_THREADSAFE
            else
                fprintf(stream, " thread %" PRIu64, H5TS_thread_id());
#endif
        } /* end block */
#else
#ifdef H5_HAVE_THREADSAFE
        fprintf(stream, " thread %" PRIu64, H5TS_thread_id());
#endif
#endif
        fprintf(stream, ":\n");
    } /* end if */

    /* Check for "real" error description - used to format output more nicely */
    if (err_desc->desc == NULL || strlen(err_desc->desc) == 0)
        have_desc = false;

    /* Print error message */
    fprintf(stream, "%*s#%03d: %s line %u in %s()%s%s\n", H5E_INDENT, "", n, err_desc->file_name,
            err_desc->line, err_desc->func_name, (have_desc ? ": " : ""), (have_desc ? err_desc->desc : ""));
    fprintf(stream, "%*smajor: %s\n", (H5E_INDENT * 2), "", maj_str);
    fprintf(stream, "%*sminor: %s\n", (H5E_INDENT * 2), "", min_str);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__walk1_cb() */
#endif /* H5_NO_DEPRECATED_SYMBOLS */

/*-------------------------------------------------------------------------
 * Function:    H5E__walk2_cb
 *
 * Purpose:     This is a default error stack traversal callback function
 *              that prints error messages to the specified output stream.
 *              It is not meant to be called directly but rather as an
 *              argument to the H5Ewalk2() function.  This function is
 *              called also by H5Eprint2().  Application writers are
 *              encouraged to use this function as a model for their own
 *              error stack walking functions.
 *
 *              N is a counter for how many times this function has been
 *              called for this particular traversal of the stack.  It always
 *              begins at zero for the first error on the stack (either the
 *              top or bottom error, or even both, depending on the traversal
 *              direction and the size of the stack).
 *
 *              ERR_DESC is an error description.  It contains all the
 *              information about a particular error.
 *
 *              CLIENT_DATA is the same pointer that was passed as the
 *              CLIENT_DATA argument of H5Ewalk().  It is expected to be a
 *              file pointer (or stderr if null).
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5E__walk2_cb(unsigned n, const H5E_error2_t *err_desc, void *client_data)
{
    H5E_print_t *eprint = (H5E_print_t *)client_data;
    FILE        *stream;                             /* I/O stream to print output to */
    H5E_cls_t   *cls_ptr;                            /* Pointer to error class */
    H5E_msg_t   *maj_ptr;                            /* Pointer to major error info */
    H5E_msg_t   *min_ptr;                            /* Pointer to minor error info */
    const char  *maj_str   = "No major description"; /* Major error description */
    const char  *min_str   = "No minor description"; /* Minor error description */
    bool         have_desc = true; /* Flag to indicate whether the error has a "real" description */
    herr_t       ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE_NOERR

    /* Check arguments */
    assert(err_desc);

    /* If no client data was passed, output to stderr */
    if (!client_data)
        stream = stderr;
    else
        stream = eprint->stream;

    /* Get descriptions for the major and minor error numbers */
    maj_ptr = (H5E_msg_t *)H5I_object_verify(err_desc->maj_num, H5I_ERROR_MSG);
    min_ptr = (H5E_msg_t *)H5I_object_verify(err_desc->min_num, H5I_ERROR_MSG);

    /* Check for bad pointer(s), but can't issue error, just leave */
    if (!maj_ptr || !min_ptr)
        HGOTO_DONE(FAIL);

    if (maj_ptr->msg)
        maj_str = maj_ptr->msg;
    if (min_ptr->msg)
        min_str = min_ptr->msg;

    /* Get error class info.  Don't use the class of the major or minor error because
     * they might be different. */
    cls_ptr = (H5E_cls_t *)H5I_object_verify(err_desc->cls_id, H5I_ERROR_CLASS);

    /* Check for bad pointer(s), but can't issue error, just leave */
    if (!cls_ptr)
        HGOTO_DONE(FAIL);

    /* Print error class header if new class */
    if (eprint->cls.lib_name == NULL || strcmp(cls_ptr->lib_name, eprint->cls.lib_name) != 0) {
        /* update to the new class information */
        if (cls_ptr->cls_name)
            eprint->cls.cls_name = cls_ptr->cls_name;
        if (cls_ptr->lib_name)
            eprint->cls.lib_name = cls_ptr->lib_name;
        if (cls_ptr->lib_vers)
            eprint->cls.lib_vers = cls_ptr->lib_vers;

        fprintf(stream, "%s-DIAG: Error detected in %s (%s)",
                (cls_ptr->cls_name ? cls_ptr->cls_name : "(null)"),
                (cls_ptr->lib_name ? cls_ptr->lib_name : "(null)"),
                (cls_ptr->lib_vers ? cls_ptr->lib_vers : "(null)"));

        /* try show the process or thread id in multiple processes cases*/
#ifdef H5_HAVE_PARALLEL
        {
            int mpi_rank, mpi_initialized, mpi_finalized;

            MPI_Initialized(&mpi_initialized);
            MPI_Finalized(&mpi_finalized);

            if (mpi_initialized && !mpi_finalized) {
                MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
                fprintf(stream, " MPI-process %d", mpi_rank);
            } /* end if */
#ifdef H5_HAVE_THREADSAFE
            else
                fprintf(stream, " thread %" PRIu64, H5TS_thread_id());
#endif
        } /* end block */
#else
#ifdef H5_HAVE_THREADSAFE
        fprintf(stream, " thread %" PRIu64, H5TS_thread_id());
#endif
#endif
        fprintf(stream, ":\n");
    } /* end if */

    /* Check for "real" error description - used to format output more nicely */
    if (err_desc->desc == NULL || strlen(err_desc->desc) == 0)
        have_desc = false;

    /* Print error message */
    fprintf(stream, "%*s#%03u: %s line %u in %s()%s%s\n", H5E_INDENT, "", n, err_desc->file_name,
            err_desc->line, err_desc->func_name, (have_desc ? ": " : ""), (have_desc ? err_desc->desc : ""));
    fprintf(stream, "%*smajor: %s\n", (H5E_INDENT * 2), "", maj_str);
    fprintf(stream, "%*sminor: %s\n", (H5E_INDENT * 2), "", min_str);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__walk2_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5E__print
 *
 * Purpose:     Private function to print the error stack in some default
 *              way.  This is just a convenience function for H5Ewalk() and
 *              H5Ewalk2() with a function that prints error messages.
 *              Users are encouraged to write their own more specific error
 *              handlers.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E__print(const H5E_stack_t *estack, FILE *stream, bool bk_compatible)
{
    H5E_print_t   eprint;  /* Callback information to pass to H5E_walk() */
    H5E_walk_op_t walk_op; /* Error stack walking callback */
    herr_t        ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    assert(estack);

    /* If no stream was given, use stderr */
    if (!stream)
        eprint.stream = stderr;
    else
        eprint.stream = stream;

    /* Reset the original error class information */
    memset(&eprint.cls, 0, sizeof(H5E_cls_t));

    /* Walk the error stack */
    if (bk_compatible) {
#ifndef H5_NO_DEPRECATED_SYMBOLS
        walk_op.vers    = 1;
        walk_op.u.func1 = H5E__walk1_cb;
        if (H5E__walk(estack, H5E_WALK_DOWNWARD, &walk_op, (void *)&eprint) < 0)
            HGOTO_ERROR(H5E_ERROR, H5E_CANTLIST, FAIL, "can't walk error stack");
#else  /* H5_NO_DEPRECATED_SYMBOLS */
        assert(0 && "version 1 error stack print without deprecated symbols!");
#endif /* H5_NO_DEPRECATED_SYMBOLS */
    }  /* end if */
    else {
        walk_op.vers    = 2;
        walk_op.u.func2 = H5E__walk2_cb;
        if (H5E__walk(estack, H5E_WALK_DOWNWARD, &walk_op, (void *)&eprint) < 0)
            HGOTO_ERROR(H5E_ERROR, H5E_CANTLIST, FAIL, "can't walk error stack");
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__print() */

/*-------------------------------------------------------------------------
 * Function:    H5E__walk
 *
 * Purpose:     Private function for H5Ewalk.
 *              Walks the error stack, calling the specified function for
 *              each error on the stack.  The DIRECTION argument determines
 *              whether the stack is walked from the inside out or the
 *              outside in.  The value H5E_WALK_UPWARD means begin with the
 *              most specific error and end at the API; H5E_WALK_DOWNWARD
 *              means to start at the API and end at the inner-most function
 *              where the error was first detected.
 *
 *              The function pointed to by STACK_FUNC will be called for
 *              each error record in the error stack. It's arguments will
 *              include an index number (beginning at zero regardless of
 *              stack traversal	direction), an error stack entry, and the
 *              CLIENT_DATA pointer passed to H5E_print.
 *
 *              The function FUNC is also provided for backward compatibility.
 *              When BK_COMPATIBLE is set to be true, FUNC is used to be
 *              compatible with older library.  If BK_COMPATIBLE is false,
 *              STACK_FUNC is used.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E__walk(const H5E_stack_t *estack, H5E_direction_t direction, const H5E_walk_op_t *op, void *client_data)
{
    int    i;                        /* Local index variable */
    herr_t ret_value = H5_ITER_CONT; /* Return value */

    FUNC_ENTER_PACKAGE_NOERR

    /* Sanity check */
    assert(estack);
    assert(op);

    /* check args, but rather than failing use some default value */
    if (direction != H5E_WALK_UPWARD && direction != H5E_WALK_DOWNWARD)
        direction = H5E_WALK_UPWARD;

    /* Walk the stack if a callback function was given */
    if (op->vers == 1) {
#ifndef H5_NO_DEPRECATED_SYMBOLS
        if (op->u.func1) {
            H5E_error1_t old_err;

            ret_value = SUCCEED;
            if (H5E_WALK_UPWARD == direction) {
                for (i = 0; i < (int)estack->nused && ret_value == H5_ITER_CONT; i++) {
                    /* Point to each error record on the stack and pass it to callback function.*/
                    old_err.maj_num   = estack->entries[i].err.maj_num;
                    old_err.min_num   = estack->entries[i].err.min_num;
                    old_err.file_name = estack->entries[i].err.file_name;
                    old_err.func_name = estack->entries[i].err.func_name;
                    old_err.line      = estack->entries[i].err.line;
                    old_err.desc      = estack->entries[i].err.desc;

                    ret_value = (op->u.func1)(i, &old_err, client_data);
                } /* end for */
            }     /* end if */
            else {
                H5_CHECK_OVERFLOW(estack->nused - 1, size_t, int);
                for (i = (int)(estack->nused - 1); i >= 0 && ret_value == H5_ITER_CONT; i--) {
                    /* Point to each error record on the stack and pass it to callback function.*/
                    old_err.maj_num   = estack->entries[i].err.maj_num;
                    old_err.min_num   = estack->entries[i].err.min_num;
                    old_err.file_name = estack->entries[i].err.file_name;
                    old_err.func_name = estack->entries[i].err.func_name;
                    old_err.line      = estack->entries[i].err.line;
                    old_err.desc      = estack->entries[i].err.desc;

                    ret_value = (op->u.func1)((int)(estack->nused - (size_t)(i + 1)), &old_err, client_data);
                } /* end for */
            }     /* end else */

            if (ret_value < 0)
                HERROR(H5E_ERROR, H5E_CANTLIST, "can't walk error stack");
        } /* end if */
#else     /* H5_NO_DEPRECATED_SYMBOLS */
        assert(0 && "version 1 error stack walk without deprecated symbols!");
#endif    /* H5_NO_DEPRECATED_SYMBOLS */
    }     /* end if */
    else {
        assert(op->vers == 2);
        if (op->u.func2) {
            ret_value = SUCCEED;
            if (H5E_WALK_UPWARD == direction) {
                for (i = 0; i < (int)estack->nused && ret_value == H5_ITER_CONT; i++)
                    ret_value = (op->u.func2)((unsigned)i, &estack->entries[i].err, client_data);
            } /* end if */
            else {
                H5_CHECK_OVERFLOW(estack->nused - 1, size_t, int);
                for (i = (int)(estack->nused - 1); i >= 0 && ret_value == H5_ITER_CONT; i--)
                    ret_value = (op->u.func2)((unsigned)(estack->nused - (size_t)(i + 1)),
                                              &estack->entries[i].err, client_data);
            } /* end else */

            if (ret_value < 0)
                HERROR(H5E_ERROR, H5E_CANTLIST, "can't walk error stack");
        } /* end if */
    }     /* end else */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__walk() */

/*-------------------------------------------------------------------------
 * Function:    H5E__get_auto
 *
 * Purpose:     Private function to return the current settings for the
 *              automatic error stack traversal function and its data
 *              for specific error stack. Either (or both) arguments may
 *              be null in which case the value is not returned.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E__get_auto(const H5E_stack_t *estack, H5E_auto_op_t *op, void **client_data)
{
    FUNC_ENTER_PACKAGE_NOERR

    assert(estack);

    /* Retrieve the requested information */
    if (op)
        *op = estack->auto_op;
    if (client_data)
        *client_data = estack->auto_data;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5E__get_auto() */

/*-------------------------------------------------------------------------
 * Function:    H5E__set_auto
 *
 * Purpose:     Private function to turn on or off automatic printing of
 *              errors for certain error stack.  When turned on (non-null
 *              FUNC pointer) any API function which returns an error
 *              indication will first call FUNC passing it CLIENT_DATA
 *              as an argument.
 *
 *              The default values before this function is called are
 *              H5Eprint2() with client data being the standard error stream,
 *              stderr.
 *
 *              Automatic stack traversal is always in the H5E_WALK_DOWNWARD
 *              direction.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E__set_auto(H5E_stack_t *estack, const H5E_auto_op_t *op, void *client_data)
{
    FUNC_ENTER_PACKAGE_NOERR

    assert(estack);

    /* Set the automatic error reporting info */
    estack->auto_op   = *op;
    estack->auto_data = client_data;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5E__set_auto() */

/*-------------------------------------------------------------------------
 * Function:    H5E_printf_stack
 *
 * Purpose:     Printf-like wrapper around H5E__push_stack.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E_printf_stack(const char *file, const char *func, unsigned line, hid_t maj_id, hid_t min_id,
                 const char *fmt, ...)
{
    H5E_stack_t *estack;               /* Pointer to error stack to modify */
    va_list      ap;                   /* Varargs info */
    bool         va_started = false;   /* Whether the variable argument list is open */
    herr_t       ret_value  = SUCCEED; /* Return value */

    /*
     * WARNING: We cannot call HERROR() from within this function or else we
     *		could enter infinite recursion.  Furthermore, we also cannot
     *		call any other HDF5 macro or function which might call
     *		HERROR().  HERROR() is called by HRETURN_ERROR() which could
     *		be called by FUNC_ENTER().
     */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    assert(maj_id >= H5E_first_maj_id_g && maj_id <= H5E_last_maj_id_g);
    assert(min_id >= H5E_first_min_id_g && min_id <= H5E_last_min_id_g);
    assert(fmt);

    /* Get the 'default' error stack */
    if (NULL == (estack = H5E__get_my_stack()))
        HGOTO_DONE(FAIL);

    /* Check if error reporting is paused for this stack */
    if (!estack->paused) {
        /* Start the variable-argument parsing */
        va_start(ap, fmt);
        va_started = true;

        /* Push the error on the stack */
        if (H5E__push_stack(estack, false, file, func, line, H5E_ERR_CLS_g, maj_id, min_id, fmt, &ap) < 0)
            HGOTO_DONE(FAIL);
    }

done:
    if (va_started)
        va_end(ap);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E_printf_stack() */

/*-------------------------------------------------------------------------
 * Function:    H5E__push_stack
 *
 * Purpose:     Pushes a new error record onto error stack for the current
 *              thread.  The error has major and minor IDs MAJ_ID and
 *              MIN_ID, the name of a function where the error was detected,
 *              the name of the file where the error was detected, the
 *              line within that file, and an error description string.  The
 *              function name, file name, and error description strings must
 *              be statically allocated (the FUNC_ENTER() macro takes care of
 *              the function name and file name automatically, but the
 *              programmer is responsible for the description string).
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E__push_stack(H5E_stack_t *estack, bool app_entry, const char *file, const char *func, unsigned line,
                hid_t cls_id, hid_t maj_id, hid_t min_id, const char *fmt, va_list *ap)
{
    herr_t ret_value = SUCCEED; /* Return value */

    /*
     * WARNING: We cannot call HERROR() from within this function or else we
     *		could enter infinite recursion.  Furthermore, we also cannot
     *		call any other HDF5 macro or function which might call
     *		HERROR().  HERROR() is called by HRETURN_ERROR() which could
     *		be called by FUNC_ENTER().
     */
    FUNC_ENTER_PACKAGE_NOERR

    /* Sanity check */
    assert(cls_id > 0);
    assert(maj_id > 0);
    assert(min_id > 0);

    /*
     * Push the error if there's room.  Otherwise just forget it.
     */
    if (estack->nused < H5E_MAX_ENTRIES) {
        estack->entries[estack->nused].app_entry = app_entry;
        if (H5E__set_stack_entry(&estack->entries[estack->nused].err, file, func, line, cls_id, maj_id,
                                 min_id, fmt, ap) < 0)
            HGOTO_DONE(FAIL);
        estack->nused++;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__push_stack() */

/*-------------------------------------------------------------------------
 * Function:    H5E__copy_stack_entry
 *
 * Purpose:     Copy a stack entry
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5E__copy_stack_entry(H5E_entry_t *dst_entry, const H5E_entry_t *src_entry)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    assert(dst_entry);
    assert(src_entry);

    /* Shallow copy all fields */
    *dst_entry = *src_entry;

    /* Deep copy application entries */
    if (dst_entry->app_entry) {
        /* Note: don't waste time incrementing library internal error IDs */
        if (dst_entry->err.cls_id != H5E_ERR_CLS_g)
            if (H5I_inc_ref(dst_entry->err.cls_id, false) < 0)
                HGOTO_ERROR(H5E_ERROR, H5E_CANTINC, FAIL, "unable to increment ref count on error class");
        if (dst_entry->err.maj_num < H5E_first_maj_id_g || dst_entry->err.maj_num > H5E_last_maj_id_g)
            if (H5I_inc_ref(dst_entry->err.maj_num, false) < 0)
                HGOTO_ERROR(H5E_ERROR, H5E_CANTINC, FAIL, "unable to increment ref count on error message");
        if (dst_entry->err.min_num < H5E_first_min_id_g || dst_entry->err.min_num > H5E_last_min_id_g)
            if (H5I_inc_ref(dst_entry->err.min_num, false) < 0)
                HGOTO_ERROR(H5E_ERROR, H5E_CANTINC, FAIL, "unable to increment ref count on error message");
        /* The library's 'func' & 'file' strings are statically allocated (by the compiler)
         * there's no need to duplicate them.
         */
        if (NULL == (dst_entry->err.file_name = strdup(src_entry->err.file_name)))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTCOPY, FAIL, "unable to duplicate file name");
        if (NULL == (dst_entry->err.func_name = strdup(src_entry->err.func_name)))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTCOPY, FAIL, "unable to duplicate function name");
    }
    if (NULL == (dst_entry->err.desc = strdup(src_entry->err.desc)))
        HGOTO_ERROR(H5E_ERROR, H5E_CANTCOPY, FAIL, "unable to duplicate error description");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__copy_stack_entry() */

/*-------------------------------------------------------------------------
 * Function:    H5E__set_stack_entry
 *
 * Purpose:     Sets the information for a given stack entry.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5E__set_stack_entry(H5E_error2_t *err_entry, const char *file, const char *func, unsigned line, hid_t cls_id,
                     hid_t maj_id, hid_t min_id, const char *fmt, va_list *ap)
{
    herr_t ret_value = SUCCEED; /* Return value */

    /*
     * WARNING: We cannot call HERROR() from within this function or else we
     *		could enter infinite recursion.  Furthermore, we also cannot
     *		call any other HDF5 macro or function which might call
     *		HERROR().  HERROR() is called by HRETURN_ERROR() which could
     *		be called by FUNC_ENTER().
     */
    FUNC_ENTER_PACKAGE_NOERR

    /* Sanity check */
    assert(err_entry);
    assert(cls_id > 0);
    assert(maj_id > 0);
    assert(min_id > 0);

    /*
     * Don't fail if arguments are bad.  Instead, substitute some default
     * value.
     */
    if (!func)
        func = "Unknown_Function";
    if (!file)
        file = "Unknown_File";
    if (!fmt)
        fmt = "No description given";

    /* Set the entry fields */
    /* NOTE: non-library IDs have already been incremented */
    err_entry->cls_id  = cls_id;
    err_entry->maj_num = maj_id;
    err_entry->min_num = min_id;
    /* The 'func' & 'file' strings are either statically allocated (by the
     * compiler), for internal error messages, or have already been duplicated,
     * for application errors, so there's no need to duplicate them here.
     */
    err_entry->func_name = func;
    err_entry->file_name = file;
    err_entry->line      = line;
    if (ap) {
        char *desc = NULL;

        /* GCC complains about the 'fmt' parameter, but it's either from static
         * strings in the library, which we know are OK, or from application
         * error push calls, and the application should be sanity checking their
         * strings.
         */
        H5_GCC_CLANG_DIAG_OFF("format-nonliteral")
        if (HDvasprintf(&desc, fmt, *ap) < 0)
            HGOTO_DONE(FAIL);
        H5_GCC_CLANG_DIAG_ON("format-nonliteral")

        err_entry->desc = desc;
    }
    else {
        if (NULL == (err_entry->desc = strdup(fmt)))
            HGOTO_DONE(FAIL);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__set_stack_entry() */

/*-------------------------------------------------------------------------
 * Function:    H5E__clear_entries
 *
 * Purpose:     Private function to clear the error stack entries for the
 *              specified error stack.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5E__clear_entries(H5E_stack_t *estack, size_t nentries)
{
    unsigned u;                   /* Local index variable */
    herr_t   ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    assert(estack);
    assert(estack->nused >= nentries);

    /* Empty the error stack from the top down */
    for (u = 0; nentries > 0; nentries--, u++) {
        H5E_entry_t *error; /* Pointer to error stack entry to clear */

        error = &(estack->entries[estack->nused - (u + 1)]);

        /* Decrement the IDs to indicate that they are no longer used by this stack */
        /* (In reverse order that they were incremented, so that reference counts work well) */
        /* Note: don't decrement library internal error IDs, since they weren't incremented */
        if (error->err.min_num < H5E_first_min_id_g || error->err.min_num > H5E_last_min_id_g)
            if (H5I_dec_ref(error->err.min_num) < 0)
                HGOTO_ERROR(H5E_ERROR, H5E_CANTDEC, FAIL, "unable to decrement ref count on error message");
        if (error->err.maj_num < H5E_first_maj_id_g || error->err.maj_num > H5E_last_maj_id_g)
            if (H5I_dec_ref(error->err.maj_num) < 0)
                HGOTO_ERROR(H5E_ERROR, H5E_CANTDEC, FAIL, "unable to decrement ref count on error message");
        if (error->err.cls_id != H5E_ERR_CLS_g)
            if (H5I_dec_ref(error->err.cls_id) < 0)
                HGOTO_ERROR(H5E_ERROR, H5E_CANTDEC, FAIL, "unable to decrement ref count on error class");

        /* Release strings */
        /* The library's 'func' & 'file' strings are statically allocated (by the
         * compiler) and are not allocated, so there's no need to free them.
         */
        if (error->app_entry) {
            H5MM_xfree_const(error->err.file_name);
            H5MM_xfree_const(error->err.func_name);
        }
        error->err.file_name = NULL;
        error->err.func_name = NULL;
        error->err.desc      = (const char *)H5MM_xfree_const(error->err.desc);
    }

    /* Decrement number of errors on stack */
    estack->nused -= u;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__clear_entries() */

/*-------------------------------------------------------------------------
 * Function:    H5E_clear_stack
 *
 * Purpose:     Clear the default error stack
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E_clear_stack(void)
{
    H5E_stack_t *estack;              /* Error stack to clear */
    herr_t       ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Get 'default' error stack */
    if (NULL == (estack = H5E__get_my_stack()))
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack");

    /* Empty the error stack */
    if (estack->nused)
        if (H5E__clear_entries(estack, estack->nused) < 0)
            HGOTO_ERROR(H5E_ERROR, H5E_CANTSET, FAIL, "can't clear error stack");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E_clear_stack() */

/*-------------------------------------------------------------------------
 * Function:    H5E__clear_stack
 *
 * Purpose:     Clear the specified error stack
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E__clear_stack(H5E_stack_t *estack)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check for 'default' error stack */
    if (estack == NULL)
        if (NULL == (estack = H5E__get_my_stack()))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack");

    /* Empty the error stack */
    if (estack->nused)
        if (H5E__clear_entries(estack, estack->nused) < 0)
            HGOTO_ERROR(H5E_ERROR, H5E_CANTSET, FAIL, "can't clear error stack");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__clear_stack() */

/*-------------------------------------------------------------------------
 * Function:    H5E__pop
 *
 * Purpose:     Private function to delete some error messages from the top
 *              of error stack.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E__pop(H5E_stack_t *estack, size_t count)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    assert(estack);
    assert(estack->nused >= count);

    /* Remove the entries from the error stack */
    if (H5E__clear_entries(estack, count) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTRELEASE, FAIL, "can't remove errors from stack");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E__pop() */

/*-------------------------------------------------------------------------
 * Function:    H5E_dump_api_stack
 *
 * Purpose:     Private function to dump the error stack during an error in
 *              an API function if a callback function is defined for the
 *              current error stack.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5E_dump_api_stack(void)
{
    H5E_stack_t *estack    = H5E__get_my_stack();
    herr_t       ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOERR

    assert(estack);

#ifdef H5_NO_DEPRECATED_SYMBOLS
    if (estack->auto_op.func2)
        (void)((estack->auto_op.func2)(H5E_DEFAULT, estack->auto_data));
#else  /* H5_NO_DEPRECATED_SYMBOLS */
    if (estack->auto_op.vers == 1) {
        if (estack->auto_op.func1)
            (void)((estack->auto_op.func1)(estack->auto_data));
    } /* end if */
    else {
        if (estack->auto_op.func2)
            (void)((estack->auto_op.func2)(H5E_DEFAULT, estack->auto_data));
    } /* end else */
#endif /* H5_NO_DEPRECATED_SYMBOLS */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5E_dump_api_stack() */

/*-------------------------------------------------------------------------
 * Function:    H5E_pause_stack
 *
 * Purpose:     Pause pushing errors on the default error stack.
 *
 *              Generally used via the H5E_PAUSE_ERRORS / H5E_RESUME_ERRORS
 *              macros.
 *
 *              When one needs to temporarily disable recording errors while
 *              trying something that's likely or expected to fail.  The code
 *              to try can be nested between these macros like:
 *
 *              H5E_PAUSE_ERRORS {
 *                  ...stuff here that's likely to fail...
 *              } H5E_RESUME_ERRORS
 *
 *              Warning: don't break, return, or longjmp() from the block of
 *              code or the error reporting won't be properly restored!
 *
 * Return:      none
 *
 *-------------------------------------------------------------------------
 */
void
H5E_pause_stack(void)
{
    H5E_stack_t *estack = H5E__get_my_stack();

    FUNC_ENTER_NOAPI_NOERR

    assert(estack);

    /* Increment pause counter */
    estack->paused++;

    FUNC_LEAVE_NOAPI_VOID
} /* end H5E_pause_stack() */

/*-------------------------------------------------------------------------
 * Function:    H5E_resume_stack
 *
 * Purpose:     Resume pushing errors on the default error stack.
 *
 *              Generally used via the H5E_PAUSE_ERRORS / H5E_RESUME_ERRORS
 *              macros.
 *
 *              When one needs to temporarily disable recording errors while
 *              trying something that's likely or expected to fail.  The code
 *              to try can be nested between these macros like:
 *
 *              H5E_PAUSE_ERRORS {
 *                  ...stuff here that's likely to fail...
 *              } H5E_RESUME_ERRORS
 *
 *              Warning: don't break, return, or longjmp() from the block of
 *              code or the error reporting won't be properly restored!
 *
 * Return:      none
 *
 *-------------------------------------------------------------------------
 */
void
H5E_resume_stack(void)
{
    H5E_stack_t *estack = H5E__get_my_stack();

    FUNC_ENTER_NOAPI_NOERR

    assert(estack);
    assert(estack->paused);

    /* Decrement pause counter */
    estack->paused--;

    FUNC_LEAVE_NOAPI_VOID
} /* end H5E_resume_stack() */
