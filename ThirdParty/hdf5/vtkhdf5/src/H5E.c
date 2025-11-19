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
 * Purpose:	Provides error handling in the form of a stack.  The
 *		FUNC_ENTER() macro clears the error stack whenever an API
 *		function is entered.  When an error is detected, an entry is
 *		pushed onto the stack.  As the functions unwind additional
 *		entries are pushed onto the stack. The API function will
 *		return some indication that an error occurred and the
 *		application can print the error stack.
 *
 *		Certain API functions in the H5E package (such as H5Eprint2())
 *		do not clear the error stack.  Otherwise, any function which
 *		doesn't have an underscore immediately after the package name
 *		will clear the error stack.  For instance, H5Fopen() clears
 *		the error stack while H5F_open() does not.
 *
 *		An error stack has a fixed maximum size.  If this size is
 *		exceeded then the stack will be truncated and only the
 *		inner-most functions will have entries on the stack. This is
 *		expected to be a rare condition.
 *
 *		Each thread has its own error stack, but since
 *		multi-threading has not been added to the library yet, this
 *		package maintains a single error stack. The error stack is
 *		statically allocated to reduce the complexity of handling
 *		errors within the H5E package.
 *
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

/****************/
/* Local Macros */
/****************/

/******************/
/* Local Typedefs */
/******************/

/********************/
/* Package Typedefs */
/********************/

/********************/
/* Local Prototypes */
/********************/

/*********************/
/* Package Variables */
/*********************/

/* Declare extern the free list to manage the H5E_stack_t struct */
H5FL_EXTERN(H5E_stack_t);

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/*-------------------------------------------------------------------------
 * Function:    H5Eregister_class
 *
 * Purpose:     Registers an error class.
 *
 * Return:      Success:    An ID for the error class
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Eregister_class(const char *cls_name, const char *lib_name, const char *version)
{
    H5E_cls_t *cls;                         /* Pointer to error class */
    hid_t      ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)

    /* Check arguments */
    if (cls_name == NULL || lib_name == NULL || version == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid string");

    /* Create the new error class object */
    if (NULL == (cls = H5E__register_class(cls_name, lib_name, version)))
        HGOTO_ERROR(H5E_ERROR, H5E_CANTCREATE, H5I_INVALID_HID, "can't create error class");

    /* Register the new error class to get an ID for it */
    if ((ret_value = H5I_register(H5I_ERROR_CLASS, cls, true)) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, H5I_INVALID_HID, "can't register error class");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eregister_class() */

/*-------------------------------------------------------------------------
 * Function:    H5Eunregister_class
 *
 * Purpose:     Closes an error class.
 *
 * Return:      Non-negative value on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eunregister_class(hid_t class_id)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)

    /* Check arguments */
    if (H5I_ERROR_CLASS != H5I_get_type(class_id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an error class");

    /*
     * Decrement the counter on the dataset.  It will be freed if the count
     * reaches zero.
     */
    if (H5I_dec_app_ref(class_id) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTDEC, FAIL, "unable to decrement ref count on error class");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eunregister_class() */

/*-------------------------------------------------------------------------
 * Function:    H5Eget_class_name
 *
 * Purpose:     Retrieves error class name.
 *
 * Return:      Success:    Name length (zero means no name)
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Eget_class_name(hid_t class_id, char *name /*out*/, size_t size)
{
    H5E_cls_t *cls;            /* Pointer to error class */
    ssize_t    ret_value = -1; /* Return value */

    FUNC_ENTER_API((-1))

    /* Get the error class */
    if (NULL == (cls = (H5E_cls_t *)H5I_object_verify(class_id, H5I_ERROR_CLASS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, (-1), "not a error class ID");

    /* Retrieve the class name */
    if ((ret_value = H5E__get_class_name(cls, name, size)) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, (-1), "can't get error class name");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eget_class_name() */

/*-------------------------------------------------------------------------
 * Function:    H5Eclose_msg
 *
 * Purpose:     Closes a major or minor error.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eclose_msg(hid_t err_id)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)

    /* Check arguments */
    if (H5I_ERROR_MSG != H5I_get_type(err_id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an error class");

    /* Decrement the counter.  It will be freed if the count reaches zero. */
    if (H5I_dec_app_ref(err_id) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTDEC, FAIL, "unable to decrement ref count on error message");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eclose_msg() */

/*-------------------------------------------------------------------------
 * Function:    H5Ecreate_msg
 *
 * Purpose:     Creates a major or minor error, returns an ID.
 *
 * Return:      Success:    An error ID
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Ecreate_msg(hid_t class_id, H5E_type_t msg_type, const char *msg_str)
{
    H5E_cls_t *cls;                         /* Pointer to error class */
    H5E_msg_t *msg;                         /* Pointer to new error message */
    hid_t      ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)

    /* Check arguments */
    if (msg_type != H5E_MAJOR && msg_type != H5E_MINOR)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "not a valid message type");
    if (msg_str == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "message is NULL");

    /* Get the error class */
    if (NULL == (cls = (H5E_cls_t *)H5I_object_verify(class_id, H5I_ERROR_CLASS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not an error class ID");

    /* Create the new error message object */
    if (NULL == (msg = H5E__create_msg(cls, msg_type, msg_str)))
        HGOTO_ERROR(H5E_ERROR, H5E_CANTCREATE, H5I_INVALID_HID, "can't create error message");

    /* Register the new error class to get an ID for it */
    if ((ret_value = H5I_register(H5I_ERROR_MSG, msg, true)) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, H5I_INVALID_HID, "can't register error message");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ecreate_msg() */

/*-------------------------------------------------------------------------
 * Function:    H5Eget_msg
 *
 * Purpose:     Retrieves an error message.
 *
 * Return:      Success:    Message length (zero means no message)
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Eget_msg(hid_t msg_id, H5E_type_t *type /*out*/, char *msg_str /*out*/, size_t size)
{
    H5E_msg_t *msg;            /* Pointer to error message */
    ssize_t    ret_value = -1; /* Return value */

    FUNC_ENTER_API_NOCLEAR((-1))

    /* Get the message object */
    if (NULL == (msg = (H5E_msg_t *)H5I_object_verify(msg_id, H5I_ERROR_MSG)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, (-1), "not a error message ID");

    /* Get the message's text */
    if ((ret_value = H5E__get_msg(msg, type, msg_str, size)) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, (-1), "can't get error message text");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eget_msg() */

/*-------------------------------------------------------------------------
 * Function:    H5Ecreate_stack
 *
 * Purpose:     Creates a new, empty, error stack.
 *
 * Return:      Success:    An error stack ID
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Ecreate_stack(void)
{
    H5E_stack_t *stk;                         /* Error stack */
    hid_t        ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)

    /* Allocate a new error stack */
    if (NULL == (stk = H5FL_CALLOC(H5E_stack_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, H5I_INVALID_HID, "memory allocation failed");

    /* Set the "automatic" error reporting info to the library default */
    H5E__set_default_auto(stk);

    /* Register the stack */
    if ((ret_value = H5I_register(H5I_ERROR_STACK, stk, true)) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, H5I_INVALID_HID, "can't create error stack");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ecreate_stack() */

/*-------------------------------------------------------------------------
 * Function:    H5Eget_current_stack
 *
 * Purpose:     Registers current error stack, returns object handle for it,
 *              clears it.
 *
 * Return:      Success:    An error stack ID
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Eget_current_stack(void)
{
    H5E_stack_t *stk;                         /* Error stack */
    hid_t        ret_value = H5I_INVALID_HID; /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(H5I_INVALID_HID)

    /* Get the current stack */
    if (NULL == (stk = H5E__get_current_stack()))
        HGOTO_ERROR(H5E_ERROR, H5E_CANTCREATE, H5I_INVALID_HID, "can't create error stack");

    /* Register the stack */
    if ((ret_value = H5I_register(H5I_ERROR_STACK, stk, true)) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, H5I_INVALID_HID, "can't create error stack");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eget_current_stack() */

/*-------------------------------------------------------------------------
 * Function:    H5Eset_current_stack
 *
 * Purpose:     Replaces current stack with specified stack. This closes the
 *              stack ID also.
 *
 * Return:      Non-negative value on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eset_current_stack(hid_t err_stack)
{
    H5E_stack_t *estack;
    herr_t       ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)

    if (err_stack != H5E_DEFAULT) {
        if (NULL == (estack = (H5E_stack_t *)H5I_object_verify(err_stack, H5I_ERROR_STACK)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a error stack ID");

        /* Set the current error stack */
        if (H5E__set_current_stack(estack) < 0)
            HGOTO_ERROR(H5E_ERROR, H5E_CANTSET, FAIL, "unable to set error stack");

        /*
         * Decrement the counter on the error stack.  It will be freed if the count
         * reaches zero.
         */
        if (H5I_dec_app_ref(err_stack) < 0)
            HGOTO_ERROR(H5E_ERROR, H5E_CANTDEC, FAIL, "unable to decrement ref count on error stack");
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eset_current_stack() */

/*-------------------------------------------------------------------------
 * Function:    H5Eclose_stack
 *
 * Purpose:     Closes an error stack.
 *
 * Return:      Non-negative value on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eclose_stack(hid_t stack_id)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)

    if (H5E_DEFAULT != stack_id) {
        /* Check arguments */
        if (H5I_ERROR_STACK != H5I_get_type(stack_id))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a error stack ID");

        /*
         * Decrement the counter on the error stack.  It will be freed if the count
         * reaches zero.
         */
        if (H5I_dec_app_ref(stack_id) < 0)
            HGOTO_ERROR(H5E_ERROR, H5E_CANTDEC, FAIL, "unable to decrement ref count on error stack");
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eclose_stack() */

/*-------------------------------------------------------------------------
 * Function:    H5Eget_num
 *
 * Purpose:     Retrieves the number of error messages.
 *
 * Return:      Success:    The number of errors
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Eget_num(hid_t error_stack_id)
{
    H5E_stack_t *estack;    /* Error stack to operate on */
    ssize_t      ret_value; /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR((-1))

    /* Need to check for errors */
    if (error_stack_id == H5E_DEFAULT) {
        if (NULL == (estack = H5E__get_my_stack()))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, (-1), "can't get current error stack");
    } /* end if */
    else {
        /* Only clear the error stack if it's not the default stack */
        H5E_clear_stack();

        /* Get the error stack to operate on */
        if (NULL == (estack = (H5E_stack_t *)H5I_object_verify(error_stack_id, H5I_ERROR_STACK)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, (-1), "not an error stack ID");
    } /* end else */

    /* Get the number of errors on stack */
    if ((ret_value = H5E__get_num(estack)) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, (-1), "can't get number of errors");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eget_num() */

/*-------------------------------------------------------------------------
 * Function:    H5Epop
 *
 * Purpose:     Deletes some error messages from the top of error stack.
 *
 * Return:      Non-negative value on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Epop(hid_t err_stack, size_t count)
{
    H5E_stack_t *estack;
    herr_t       ret_value = SUCCEED; /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(FAIL)

    /* Need to check for errors */
    if (err_stack == H5E_DEFAULT) {
        if (NULL == (estack = H5E__get_my_stack()))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack");
    } /* end if */
    else {
        /* Only clear the error stack if it's not the default stack */
        H5E_clear_stack();

        /* Get the error stack to operate on */
        if (NULL == (estack = (H5E_stack_t *)H5I_object_verify(err_stack, H5I_ERROR_STACK)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a error stack ID");
    } /* end else */

    /* Range limit the number of errors to pop off stack */
    if (count > estack->nused)
        count = estack->nused;

    /* Pop the errors off the stack */
    if (H5E__pop(estack, count) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTRELEASE, FAIL, "can't pop errors from stack");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Epop() */

/*-------------------------------------------------------------------------
 * Function:    H5Epush2
 *
 * Purpose:     Pushes a new error record onto error stack for the current
 *              thread. The error has major and minor IDs MAJ_ID and
 *              MIN_ID, the name of a function where the error was detected,
 *              the name of the file where the error was detected, the
 *              line within that file, and an error description string.  The
 *              function name, file name, and error description strings must
 *              be statically allocated.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Notes:       Basically a new public API wrapper around the H5E__push_stack
 *              function.
 *
 *-------------------------------------------------------------------------
 */
H5_ATTR_FORMAT(printf, 8, 9)
herr_t
H5Epush2(hid_t err_stack, const char *file, const char *func, unsigned line, hid_t cls_id, hid_t maj_id,
         hid_t min_id, const char *fmt, ...)
{
    H5E_stack_t *estack;              /* Pointer to error stack to modify */
    va_list      ap;                  /* Varargs info */
    bool         va_started = false;  /* Whether the variable argument list is open */
    const char  *tmp_file;            /* Copy of the file name */
    const char  *tmp_func;            /* Copy of the function name */
    herr_t       ret_value = SUCCEED; /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(FAIL)

    /* Check for 'default' error stack */
    if (err_stack == H5E_DEFAULT) {
        if (NULL == (estack = H5E__get_my_stack()))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get the default error stack");
    }
    else {
        /* Only clear the error stack if it's not the default stack */
        H5E_clear_stack();

        /* Get the error stack to operate on */
        if (NULL == (estack = (H5E_stack_t *)H5I_object_verify(err_stack, H5I_ERROR_STACK)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a error stack ID");
    } /* end else */

    /* Check if error reporting is paused for this stack */
    if (!estack->paused) {
        /* Start the variable-argument parsing */
        va_start(ap, fmt);
        va_started = true;

        /* Duplicate string information */
        if (NULL == (tmp_file = strdup(file)))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTALLOC, FAIL, "can't duplicate file string");
        if (NULL == (tmp_func = strdup(func)))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTALLOC, FAIL, "can't duplicate function string");

        /* Increment refcount on non-library IDs */
        if (cls_id != H5E_ERR_CLS_g)
            if (H5I_inc_ref(cls_id, false) < 0)
                HGOTO_ERROR(H5E_ERROR, H5E_CANTINC, FAIL, "can't increment class ID");
        if (maj_id < H5E_first_maj_id_g || maj_id > H5E_last_maj_id_g)
            if (H5I_inc_ref(maj_id, false) < 0)
                HGOTO_ERROR(H5E_ERROR, H5E_CANTINC, FAIL, "can't increment major error ID");
        if (min_id < H5E_first_min_id_g || min_id > H5E_last_min_id_g)
            if (H5I_inc_ref(min_id, false) < 0)
                HGOTO_ERROR(H5E_ERROR, H5E_CANTINC, FAIL, "can't increment minor error ID");

        /* Push the error on the stack */
        if (H5E__push_stack(estack, true, tmp_file, tmp_func, line, cls_id, maj_id, min_id, fmt, &ap) < 0)
            HGOTO_ERROR(H5E_ERROR, H5E_CANTSET, FAIL, "can't push error on stack");
    }

done:
    if (va_started)
        va_end(ap);

    FUNC_LEAVE_API(ret_value)
} /* end H5Epush2() */

/*-------------------------------------------------------------------------
 * Function:    H5Eclear2
 *
 * Purpose:     Clears the error stack for the specified error stack.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eclear2(hid_t err_stack)
{
    H5E_stack_t *estack;              /* Error stack to operate on */
    herr_t       ret_value = SUCCEED; /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(FAIL)

    /* Need to check for errors */
    if (err_stack == H5E_DEFAULT)
        estack = NULL;
    else {
        /* Only clear the error stack if it's not the default stack */
        H5E_clear_stack();

        if (NULL == (estack = (H5E_stack_t *)H5I_object_verify(err_stack, H5I_ERROR_STACK)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a error stack ID");
    } /* end else */

    /* Clear the error stack */
    if (H5E__clear_stack(estack) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTSET, FAIL, "can't clear error stack");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eclear2() */

/*-------------------------------------------------------------------------
 * Function:    H5Eprint2
 *
 * Purpose:     Prints the error stack in some default way. This is just a
 *              convenience function for H5Ewalk() with a function that
 *              prints error messages. Users are encouraged to write their
 *              own more specific error handlers.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eprint2(hid_t err_stack, FILE *stream)
{
    herr_t ret_value = SUCCEED; /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(FAIL)

    /* Print error stack */
    if ((ret_value = H5E__print2(err_stack, stream)) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTLIST, FAIL, "can't display error stack");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eprint2() */

/*-------------------------------------------------------------------------
 * Function:    H5Ewalk2
 *
 * Purpose:     Walks the error stack for the current thread and calls some
 *              function for each error along the way.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ewalk2(hid_t err_stack, H5E_direction_t direction, H5E_walk2_t stack_func, void *client_data)
{
    H5E_stack_t  *estack;              /* Error stack to operate on */
    H5E_walk_op_t op;                  /* Operator for walking error stack */
    herr_t        ret_value = SUCCEED; /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(FAIL)

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

    /* Walk the error stack */
    op.vers    = 2;
    op.u.func2 = stack_func;
    if ((ret_value = H5E__walk(estack, direction, &op, client_data)) < 0)
        HERROR(H5E_ERROR, H5E_CANTLIST, "can't walk error stack");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ewalk2() */

/*-------------------------------------------------------------------------
 * Function:    H5Eget_auto2
 *
 * Purpose:     Returns the current settings for the automatic error stack
 *              traversal function and its data for specific error stack.
 *              Either (or both) arguments may be null in which case the
 *              value is not returned.
 *
 * Return:      Non-negative value on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eget_auto2(hid_t estack_id, H5E_auto2_t *func /*out*/, void **client_data /*out*/)
{
    H5E_stack_t  *estack;              /* Error stack to operate on */
    H5E_auto_op_t op;                  /* Error stack function */
    herr_t        ret_value = SUCCEED; /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(FAIL)

    if (estack_id == H5E_DEFAULT) {
        if (NULL == (estack = H5E__get_my_stack()))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack");
    } /* end if */
    else {
        /* Only clear the error stack if it's not the default stack */
        H5E_clear_stack();

        if (NULL == (estack = (H5E_stack_t *)H5I_object_verify(estack_id, H5I_ERROR_STACK)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a error stack ID");
    } /* end else */

    /* Get the automatic error reporting information */
    if (H5E__get_auto(estack, &op, client_data) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get automatic error info");

#ifndef H5_NO_DEPRECATED_SYMBOLS
    /* Fail if the printing function isn't the default(user-set) and set through H5Eset_auto1 */
    if (!op.is_default && op.vers == 1)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "wrong API function, H5Eset_auto1 has been called");
#endif /* H5_NO_DEPRECATED_SYMBOLS */

    if (func)
        *func = op.func2;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eget_auto2() */

/*-------------------------------------------------------------------------
 * Function:    H5Eset_auto2
 *
 * Purpose:     Turns on or off automatic printing of errors for certain
 *              error stack.  When turned on (non-null FUNC pointer) any
 *              API function which returns an error indication will first
 *              call FUNC passing it CLIENT_DATA as an argument.
 *
 *              The default values before this function is called are
 *              H5Eprint2() with client data being the standard error stream,
 *              stderr.
 *
 *              Automatic stack traversal is always in the H5E_WALK_DOWNWARD
 *              direction.
 *
 * Return:      Non-negative value on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eset_auto2(hid_t estack_id, H5E_auto2_t func, void *client_data)
{
    H5E_stack_t  *estack;              /* Error stack to operate on */
    H5E_auto_op_t op;                  /* Error stack operator */
    herr_t        ret_value = SUCCEED; /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(FAIL)

    if (estack_id == H5E_DEFAULT) {
        if (NULL == (estack = H5E__get_my_stack()))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack");
    } /* end if */
    else {
        /* Only clear the error stack if it's not the default stack */
        H5E_clear_stack();

        if (NULL == (estack = (H5E_stack_t *)H5I_object_verify(estack_id, H5I_ERROR_STACK)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a error stack ID");
    } /* end else */

#ifndef H5_NO_DEPRECATED_SYMBOLS
    /* Get the automatic error reporting information */
    if (H5E__get_auto(estack, &op, NULL) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get automatic error info");

    /* Set the automatic error reporting information */
    if (func != op.func2_default)
        op.is_default = false;
    else
        op.is_default = true;

    op.vers = 2;
#endif /* H5_NO_DEPRECATED_SYMBOLS */

    /* Set the automatic error reporting function */
    op.func2 = func;

    if (H5E__set_auto(estack, &op, client_data) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTSET, FAIL, "can't set automatic error info");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eset_auto2() */

/*-------------------------------------------------------------------------
 * Function:    H5Eauto_is_v2
 *
 * Purpose:     Determines if the error auto reporting function for an
 *              error stack conforms to the H5E_auto_stack_t typedef
 *              or the H5E_auto_t typedef.  The IS_STACK parameter is set
 *              to 1 for the first case and 0 for the latter case.
 *
 * Return:      Non-negative value on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eauto_is_v2(hid_t estack_id, unsigned *is_stack)
{
    H5E_stack_t *estack;              /* Error stack to operate on */
    herr_t       ret_value = SUCCEED; /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(FAIL)

    if (estack_id == H5E_DEFAULT) {
        if (NULL == (estack = H5E__get_my_stack()))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack");
    } /* end if */
    else {
        /* Only clear the error stack if it's not the default stack */
        H5E_clear_stack();

        if (NULL == (estack = (H5E_stack_t *)H5I_object_verify(estack_id, H5I_ERROR_STACK)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a error stack ID");
    } /* end else */

    /* Check if the error stack reporting function is the "newer" stack type */
    if (is_stack)
#ifndef H5_NO_DEPRECATED_SYMBOLS
        *is_stack = estack->auto_op.vers > 1;
#else
        *is_stack = 1;
#endif

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eauto_is_v2() */

/*-------------------------------------------------------------------------
 * Function:    H5Eappend_stack
 *
 * Purpose:     Appends one error stack to another, optionally closing the
 *              source stack.
 *
 * Return:      Non-negative value on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eappend_stack(hid_t dst_stack_id, hid_t src_stack_id, hbool_t close_source_stack)
{
    H5E_stack_t *dst_stack, *src_stack; /* Error stacks */
    herr_t       ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_API(FAIL)

    /* Check args */
    if (NULL == (dst_stack = (H5E_stack_t *)H5I_object_verify(dst_stack_id, H5I_ERROR_STACK)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dst_stack_id not a error stack ID");
    if (NULL == (src_stack = (H5E_stack_t *)H5I_object_verify(src_stack_id, H5I_ERROR_STACK)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "src_stack_id not a error stack ID");

    /* Append the source stack to the destination stack */
    if (H5E__append_stack(dst_stack, src_stack) < 0)
        HGOTO_ERROR(H5E_ERROR, H5E_CANTAPPEND, FAIL, "can't append stack");

    /* Close source error stack, if requested */
    if (close_source_stack)
        /* Decrement the counter on the error stack.  It will be freed if the
         * count reaches zero.
         */
        if (H5I_dec_app_ref(src_stack_id) < 0)
            HGOTO_ERROR(H5E_ERROR, H5E_CANTDEC, FAIL, "unable to decrement ref count on source error stack");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eappend_stack() */

/*-------------------------------------------------------------------------
 * Function:    H5Eis_paused
 *
 * Purpose:     Check if pushing errors on an error stack is paused
 *
 * Return:      Non-negative value on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eis_paused(hid_t stack_id, hbool_t *is_paused)
{
    H5E_stack_t *stack;               /* Error stack */
    herr_t       ret_value = SUCCEED; /* Return value */

    /* Don't clear the error stack! :-) */
    FUNC_ENTER_API_NOCLEAR(FAIL)

    /* Get the correct error stack */
    if (stack_id == H5E_DEFAULT) {
        if (NULL == (stack = H5E__get_my_stack()))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack");
    } /* end if */
    else {
        /* Only clear the error stack if it's not the default stack */
        H5E_clear_stack();

        /* Get the error stack to operate on */
        if (NULL == (stack = (H5E_stack_t *)H5I_object_verify(stack_id, H5I_ERROR_STACK)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an error stack ID");
    } /* end else */

    /* Check arguments */
    if (NULL == is_paused)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "is_paused parameter is NULL");

    /* Check if the stack is paused */
    *is_paused = (stack->paused > 0);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eis_paused() */

/*-------------------------------------------------------------------------
 * Function:    H5Epause_stack
 *
 * Purpose:     Pause pushing errors on an error stack
 *
 * Return:      Non-negative value on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Epause_stack(hid_t stack_id)
{
    H5E_stack_t *stack;               /* Error stack */
    herr_t       ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)

    /* Get the correct error stack */
    if (stack_id == H5E_DEFAULT) {
        if (NULL == (stack = H5E__get_my_stack()))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack");
    } /* end if */
    else
        /* Get the error stack to operate on */
        if (NULL == (stack = (H5E_stack_t *)H5I_object_verify(stack_id, H5I_ERROR_STACK)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an error stack ID");

    /* Increment pause counter */
    stack->paused++;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Epause_stack() */

/*-------------------------------------------------------------------------
 * Function:    H5Eresume_stack
 *
 * Purpose:     Resume pushing errors on an error stack
 *
 * Return:      Non-negative value on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Eresume_stack(hid_t stack_id)
{
    H5E_stack_t *stack;               /* Error stack */
    herr_t       ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)

    /* Get the correct error stack */
    if (stack_id == H5E_DEFAULT) {
        if (NULL == (stack = H5E__get_my_stack()))
            HGOTO_ERROR(H5E_ERROR, H5E_CANTGET, FAIL, "can't get current error stack");
    } /* end if */
    else
        /* Get the error stack to operate on */
        if (NULL == (stack = (H5E_stack_t *)H5I_object_verify(stack_id, H5I_ERROR_STACK)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an error stack ID");

    /* Check for pause/resume imbalance */
    if (0 == stack->paused)
        HGOTO_ERROR(H5E_ERROR, H5E_BADRANGE, FAIL, "resuming more than paused");

    /* Decrement pause counter */
    stack->paused--;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Eresume_stack() */
