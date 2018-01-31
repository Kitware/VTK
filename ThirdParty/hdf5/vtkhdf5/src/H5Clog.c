/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:             H5Clog.c
 *                      May 30 2016
 *                      Quincey Koziol
 *
 * Purpose:             Functions for generic cache logging in JSON format
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/
#include "H5Cmodule.h"         /* This source code file is part of the H5C module */

/***********/
/* Headers */
/***********/
#include "H5private.h"          /* Generic Functions                    */
#ifdef H5_HAVE_PARALLEL
#define H5AC_FRIEND		/*suppress error about including H5ACpkg	  */
#include "H5ACpkg.h"            /* Metadata cache                       */
#endif /* H5_HAVE_PARALLEL */
#include "H5Cpkg.h"            /* Metadata cache                       */
#include "H5Eprivate.h"         /* Error handling                       */
#include "H5MMprivate.h"	/* Memory management			*/


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


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:    H5C_set_up_logging
 *
 * Purpose:     Setup for metadata cache logging.
 *
 *              Metadata logging is enabled and disabled at two levels. This
 *              function and the associated tear_down function open and close
 *              the log file. the start_ and stop_logging functions are then
 *              used to switch logging on/off. Optionally, logging can begin
 *              as soon as the log file is opened (set via the start_immediately
 *              parameter to this function).
 *
 *              The log functionality is split between the H5C and H5AC
 *              packages. Log state and direct log manipulation resides in
 *              H5C. Log messages are generated in H5AC and sent to
 *              the H5C_write_log_message function.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:	Dana Robinson
 *              Sunday, March 16, 2014
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_set_up_logging(H5C_t *cache_ptr, const char log_location[],
    hbool_t start_immediately)
{
#ifdef H5_HAVE_PARALLEL
    H5AC_aux_t *aux_ptr = NULL;
#endif /*H5_HAVE_PARALLEL*/
    char *file_name = NULL;
    size_t n_chars;
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    if(NULL == cache_ptr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cache_ptr == NULL")
    if(H5C__H5C_T_MAGIC != cache_ptr->magic)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cache magic value incorrect")
    if(cache_ptr->logging_enabled)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "logging already set up")
    if(NULL == log_location)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL log location not allowed")

    /* Possibly fix up the log file name.
     * The extra 39 characters are for adding the rank to the file name
     * under parallel HDF5. 39 characters allows > 2^127 processes which
     * should be enough for anybody.
     *
     * allocation size = <path length> + dot + <rank # length> + \0
     */
    n_chars = HDstrlen(log_location) + 1 + 39 + 1;
    if(NULL == (file_name = (char *)H5MM_calloc(n_chars * sizeof(char))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "can't allocate memory for mdc log file name manipulation")

#ifdef H5_HAVE_PARALLEL
    /* Add the rank to the log file name when MPI is in use */
    aux_ptr = (H5AC_aux_t *)(cache_ptr->aux_ptr);

    if(NULL == aux_ptr)
        HDsnprintf(file_name, n_chars, "%s", log_location);
    else {
        if(aux_ptr->magic != H5AC__H5AC_AUX_T_MAGIC)
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "bad aux_ptr->magic")
        HDsnprintf(file_name, n_chars, "%s.%d", log_location, aux_ptr->mpi_rank);
    } /* end else */
#else /* H5_HAVE_PARALLEL */
    HDsnprintf(file_name, n_chars, "%s", log_location);
#endif /* H5_HAVE_PARALLEL */

    /* Open log file */
    if(NULL == (cache_ptr->log_file_ptr = HDfopen(file_name, "w")))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "can't create mdc log file")

    /* Set logging flags */
    cache_ptr->logging_enabled = TRUE;
    cache_ptr->currently_logging = start_immediately;

 done:
    if(file_name)
        file_name = (char *)H5MM_xfree(file_name);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_set_up_logging() */


/*-------------------------------------------------------------------------
 * Function:    H5C_tear_down_logging
 *
 * Purpose:     Tear-down for metadata cache logging.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:	Dana Robinson
 *              Sunday, March 16, 2014
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_tear_down_logging(H5C_t *cache_ptr)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    if(NULL == cache_ptr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cache_ptr == NULL")
    if(H5C__H5C_T_MAGIC != cache_ptr->magic)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cache magic value incorrect")
    if(FALSE == cache_ptr->logging_enabled)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "logging not enabled")

    /* Unset logging flags */
    cache_ptr->logging_enabled = FALSE;
    cache_ptr->currently_logging = FALSE;

    /* Close log file */
    if(EOF == HDfclose(cache_ptr->log_file_ptr))
        HGOTO_ERROR(H5E_FILE, H5E_CANTCLOSEFILE, FAIL, "problem closing mdc log file")
    cache_ptr->log_file_ptr = NULL;

 done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_tear_down_logging() */


/*-------------------------------------------------------------------------
 * Function:    H5C_start_logging
 *
 * Purpose:     Start logging metadata cache operations.
 *
 *              TODO: Add a function that dumps the current state of the
 *                    metadata cache.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:	Dana Robinson
 *              Sunday, March 16, 2014
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_start_logging(H5C_t *cache_ptr)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    if(NULL == cache_ptr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cache_ptr == NULL")
    if(H5C__H5C_T_MAGIC != cache_ptr->magic)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cache magic value incorrect")
    if(FALSE == cache_ptr->logging_enabled)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "logging not enabled")
    if(cache_ptr->currently_logging)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "logging already in progress")

    /* Set logging flags */
    cache_ptr->currently_logging = TRUE;

    /* TODO - Dump cache state */

 done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_start_logging() */


/*-------------------------------------------------------------------------
 * Function:    H5C_stop_logging
 *
 * Purpose:     Stop logging metadata cache operations.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:	Dana Robinson
 *              Sunday, March 16, 2014
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_stop_logging(H5C_t *cache_ptr)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    if(NULL == cache_ptr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cache_ptr == NULL")
    if(H5C__H5C_T_MAGIC != cache_ptr->magic)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cache magic value incorrect")
    if(FALSE == cache_ptr->logging_enabled)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "logging not enabled")
    if(FALSE == cache_ptr->currently_logging)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "logging not in progress")

    /* Set logging flags */
    cache_ptr->currently_logging = FALSE;

 done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_stop_logging() */


/*-------------------------------------------------------------------------
 * Function:    H5C_get_logging_status
 *
 * Purpose:     Determines if the cache is actively logging (via the OUT
 *              parameter).
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:	Dana Robinson
 *              Sunday, March 16, 2014
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_get_logging_status(const H5C_t *cache_ptr, /*OUT*/ hbool_t *is_enabled,
                       /*OUT*/ hbool_t *is_currently_logging)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    if(NULL == cache_ptr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cache_ptr == NULL")
    if(H5C__H5C_T_MAGIC != cache_ptr->magic)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cache magic value incorrect")
    if(NULL == is_enabled)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cache magic value incorrect")
    if(NULL == is_currently_logging)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cache magic value incorrect")

    *is_enabled = cache_ptr->logging_enabled;
    *is_currently_logging = cache_ptr->currently_logging;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_get_logging_status() */


/*-------------------------------------------------------------------------
 * Function:    H5C_write_log_message
 *
 * Purpose:     Write a message to the log file and flush the file. 
 *              The message string is neither modified nor freed.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:	Dana Robinson
 *              Sunday, March 16, 2014
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_write_log_message(const H5C_t *cache_ptr, const char message[])
{
    size_t n_chars;
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    if(NULL == cache_ptr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cache_ptr == NULL")
    if(H5C__H5C_T_MAGIC != cache_ptr->magic)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cache magic value incorrect")
    if(FALSE == cache_ptr->currently_logging)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "not currently logging")
    if(NULL == message)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL log message not allowed")

    /* Write the log message and flush */
    n_chars = HDstrlen(message);
    if((int)n_chars != HDfprintf(cache_ptr->log_file_ptr, message))
        HGOTO_ERROR(H5E_FILE, H5E_WRITEERROR, FAIL, "error writing log message")
    if(EOF == HDfflush(cache_ptr->log_file_ptr))
        HGOTO_ERROR(H5E_FILE, H5E_WRITEERROR, FAIL, "error flushing log message")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_write_log_message() */

