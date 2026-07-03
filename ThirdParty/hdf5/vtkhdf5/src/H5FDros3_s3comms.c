/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*****************************************************************************
 * Module for performing S3 communications using the aws-c-s3 library
 *
 * ***NOT A FILE DRIVER***
 *
 * Provides functions and structures required for interfacing with Amazon
 * Simple Storage Service (S3).
 *
 *****************************************************************************/

/****************/
/* Module Setup */
/****************/

/***********/
/* Headers */
/***********/

#include "H5private.h"
#include "H5Eprivate.h"
#include "H5FDros3_s3comms.h"
#include "H5MMprivate.h"

#ifdef H5_HAVE_ROS3_VFD

#include <aws/s3/s3.h>
#include <aws/s3/s3_client.h>
#include <aws/auth/credentials.h>
#include <aws/common/condition_variable.h>
#include <aws/common/mutex.h>
#include <aws/io/channel_bootstrap.h>
#include <aws/io/event_loop.h>
#include <aws/io/uri.h>
#include <aws/http/request_response.h>
#include <aws/sdkutils/aws_profile.h>

/****************/
/* Local Macros */
/****************/

/*
 * Size of buffer to allocate for host name
 */
#define HOST_NAME_LEN 128

/*
 * Size of buffer to allocate for "bytes=<first_byte>[-<last_byte>]"
 * HTTP Range value string (including a NUL terminator)
 */
#define S3COMMS_MAX_RANGE_STRING_SIZE 128

/*
 * Size of buffer to allocate for User-Agent HTTP header
 */
#define MAX_USER_AGENT_STRING_SIZE 128

/*
 * The maximum number of entries for the host resolver to
 * keep cached
 */
#define HOST_RESOLVER_MAX_NUM_ENTRIES 8

/******************/
/* Local Typedefs */
/******************/

/********************/
/* Local Structures */
/********************/

/*
 * Parameters that aws-c-s3 was configured with. Most of these parameters
 * must be freed when no longer in use:
 *
 *  - aws_s3_client_release(client);
 *  - aws_client_bootstrap_release(client_bootstrap);
 *  - aws_credentials_provider_release(credentials_provider);
 *  - aws_host_resolver_release(host_resolver);
 *  - aws_event_loop_group_release(event_loop_group);
 *  - aws_uri_clean_up(&parsed_uri);
 *  - aws_uri_clean_up(&alt_parsed_uri);
 */
typedef struct H5FD__s3comms_aws_params_t {
    struct aws_allocator            *allocator;
    struct aws_s3_client            *client;
    struct aws_client_bootstrap     *client_bootstrap;
    struct aws_credentials_provider *credentials_provider;
    struct aws_host_resolver        *host_resolver;
    struct aws_event_loop_group     *event_loop_group;
    struct aws_signing_config_aws    signing_config;
    struct aws_uri                   parsed_uri;
    struct aws_uri                   alt_parsed_uri;
    bool                             force_path_style;

    /* Shared MT state for requests */
    struct aws_mutex              req_mutex; /* Mutex for condition variable */
    struct aws_condition_variable req_cvar;  /* Condition variable for waiting on request to finish */
} H5FD__s3comms_aws_params_t;

/*
 * Common structure for requests
 */
typedef struct H5FD__s3comms_request_t {
    /* Final status of operation - used for error checking */
    herr_t status;

    /* Flag for when request is finished - must ONLY be set in a finish callback */
    bool finished;

    /* Error message to be returned through HDF5 error stack */
    const char *err_msg;

    /* Mutex for condition variable */
    struct aws_mutex *mutex;

    /* Condition variable for waiting on request to finish */
    struct aws_condition_variable *cvar;
} H5FD__s3comms_request_t;

/*
 * Parameters for the 'read' operation to get data from an S3 object.
 */
typedef struct H5FD__s3comms_read_params_t {
    H5FD__s3comms_request_t request; /* Must be first */

    uint8_t *read_buf;
    size_t   read_buf_size;
    haddr_t  read_offset;
} H5FD__s3comms_read_params_t;

/*
 * Parameters for the 'getsize' operation to get the size of an S3 object.
 */
typedef struct H5FD__s3comms_getsize_params_t {
    H5FD__s3comms_request_t request; /* Must be first */

    struct aws_byte_buf    response_body;
    struct aws_byte_cursor response_body_cursor;
    size_t                 object_size;
} H5FD__s3comms_getsize_params_t;

typedef struct H5FD__s3comms_check_cred_provider_params_t {
    H5FD__s3comms_request_t request; /* Must be first */

    struct aws_credentials *credentials;
    int                     error_code;
} H5FD__s3comms_check_cred_provider_params_t;

/********************/
/* Local Prototypes */
/********************/

/* atexit() function to avoid issue when cleaning up aws-c-s3 library
 * from HDF5's default atexit() handler.
 */
static void H5FD__s3comms_term_func(void);

/* Callbacks for processing general requests */
static void H5FD__s3comms_s3r_req_finish_cb(struct aws_s3_meta_request              *meta_request,
                                            const struct aws_s3_meta_request_result *meta_request_result,
                                            void                                    *user_data);
static bool H5FD__s3comms_s3r_req_finish_pred(void *user_data);

/* Callbacks for operation to read an s3 object */
static int H5FD__s3comms_s3r_read_cb(struct aws_s3_meta_request   *meta_request,
                                     const struct aws_byte_cursor *body, uint64_t range_start,
                                     void *user_data);

/* Callbacks for operation to get the size of an s3 object */
static herr_t H5FD__s3comms_s3r_getsize(s3r_t *handle);
static int    H5FD__s3comms_s3r_getsize_headers_cb(struct aws_s3_meta_request    *meta_request,
                                                   const struct aws_http_headers *headers, int response_status,
                                                   void *user_data);

/* Utility functions */
static parsed_url_t *H5FD__s3comms_parse_url(s3r_t *handle, const char *url, struct aws_uri *uri);
static herr_t        H5FD__s3comms_free_purl(parsed_url_t *purl);

static herr_t H5FD__s3comms_get_aws_region(const H5FD__s3comms_aws_params_t *aws_params,
                                           const H5FD_ros3_fapl_t *fa, char **aws_region_out);
/* clang-format off */
static herr_t H5FD__s3comms_get_credentials_provider(H5FD__s3comms_aws_params_t *aws_params, 
                                                     const H5FD_ros3_fapl_t *fa,
                                                     const char             *fapl_token,
                                                     struct aws_credentials_provider **credentials_provider_out);
/* clang-format on */

static herr_t H5FD__s3comms_format_http_request_message(const H5FD__s3comms_aws_params_t *aws_params,
                                                        const char *HTTP_method, const char *bucket_name,
                                                        const char               *object_key,
                                                        struct aws_http_message **message_out);
static herr_t H5FD__s3comms_format_host_header(const H5FD__s3comms_aws_params_t *aws_params,
                                               struct aws_http_message *message, const char *bucket_name,
                                               const char *host_name, const char *port);
static herr_t H5FD__s3comms_format_range_header(struct aws_http_message *message, haddr_t offset, size_t len);
static herr_t H5FD__s3comms_format_user_agent_header(struct aws_http_message *message);

static const char *H5FD__s3comms_httpcode_to_str(long httpcode, bool *handled);

/*********************/
/* Package Variables */
/*********************/

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/* Boolean to determine if the AWS library has been initialized */
static bool H5FD_ros3_aws_init_g = false;

/* Pointer to allocator used for AWS operations */
static struct aws_allocator *H5FD_ros3_aws_allocator_g = NULL;

/* Pointer to event loop group used for AWS client and host resolver */
static struct aws_event_loop_group *H5FD_ros3_aws_event_loop_group_g = NULL;

/* Pointer to host resolver used for resolving and caching hosts */
static struct aws_host_resolver *H5FD_ros3_aws_host_resolver_g = NULL;

/* Structure used for logging when enabled */
static struct aws_logger H5FD_ros3_aws_logger_g   = {0};
static bool              H5FD_ros3_aws_log_init_g = false;

/* Boolean controlling whether debugging output is enabled */
static bool H5FD_ros3_debug_g = false;

/*************/
/* Functions */
/*************/

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_init
 *
 * Purpose:     Initialize the S3 communications interface.
 *
 * Return:      Non-negative on success/Negative on failure
 *----------------------------------------------------------------------------
 */
herr_t
H5FD__s3comms_init(void)
{
    struct aws_event_loop_group_options      event_loop_group_opts = {0};
    struct aws_host_resolver_default_options host_resolver_opts    = {0};
    struct aws_logger_standard_options       log_opts              = {0};
    char                                    *debug                 = NULL;
    char                                    *log_level             = NULL;
    herr_t                                   ret_value             = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (H5FD_ros3_aws_init_g)
        HGOTO_DONE(SUCCEED);

    /* Initialize aws-c-s3 with default allocator. Refer to allocator.h
     * in the aws-c-common dependency for alternative allocators that
     * could be better for specific use cases.
     */
    H5FD_ros3_aws_allocator_g = aws_default_allocator();

    aws_s3_library_init(H5FD_ros3_aws_allocator_g);

    /* Set up an event loop group using platform default event loop */
    event_loop_group_opts.loop_count = 0;
    event_loop_group_opts.type       = AWS_EVENT_LOOP_PLATFORM_DEFAULT;
    H5FD_ros3_aws_event_loop_group_g =
        aws_event_loop_group_new(H5FD_ros3_aws_allocator_g, &event_loop_group_opts);
    if (!H5FD_ros3_aws_event_loop_group_g)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "couldn't initialize AWS event loop group");

    /* Set up host resolver using default options taken from AWS sample code */
    host_resolver_opts.el_group    = H5FD_ros3_aws_event_loop_group_g;
    host_resolver_opts.max_entries = HOST_RESOLVER_MAX_NUM_ENTRIES;
    H5FD_ros3_aws_host_resolver_g =
        aws_host_resolver_new_default(H5FD_ros3_aws_allocator_g, &host_resolver_opts);
    if (!H5FD_ros3_aws_host_resolver_g)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "couldn't initialize AWS host resolver");

        /* Check if debugging output should be enabled */
#if S3COMMS_DEBUG > 0
    H5FD_ros3_debug_g = true;
#else
    debug = getenv(HDF5_ROS3_VFD_DEBUG);
    if (debug && (*debug != '\0')) {
        if (0 != HDstrcasecmp(debug, "false") && 0 != HDstrcasecmp(debug, "off") && 0 != strcmp(debug, "0"))
            H5FD_ros3_debug_g = true;
    }
#endif

    /* Configure aws-c-s3 logging if enabled */
    log_level = getenv(HDF5_ROS3_VFD_LOG_LEVEL);
    if (log_level && (*log_level != '\0')) {
        log_opts.level = AWS_LL_NONE;
        if (0 == HDstrcasecmp(log_level, "trace"))
            log_opts.level = AWS_LL_TRACE;
        else if (0 == HDstrcasecmp(log_level, "debug"))
            log_opts.level = AWS_LL_DEBUG;
        else if (0 == HDstrcasecmp(log_level, "info"))
            log_opts.level = AWS_LL_INFO;
        else if (0 == HDstrcasecmp(log_level, "error"))
            log_opts.level = AWS_LL_ERROR;

        if (log_opts.level != AWS_LL_NONE) {
            char *log_file = NULL;

            log_file = getenv(HDF5_ROS3_VFD_LOG_FILE);
            if (log_file) {
                if (0 == HDstrcasecmp(log_file, "stdout"))
                    log_opts.file = stdout;
                else if (0 == HDstrcasecmp(log_file, "stderr"))
                    log_opts.file = stderr;
                else
                    log_opts.filename = log_file;
            }
            else {
                log_opts.filename = H5FD_ROS3_VFD_DEFAULT_LOG_FILE;
            }

            aws_logger_init_standard(&H5FD_ros3_aws_logger_g, H5FD_ros3_aws_allocator_g, &log_opts);
            aws_logger_set(&H5FD_ros3_aws_logger_g);
            H5FD_ros3_aws_log_init_g = true;
        }
    }

    /* Work around issue where aws-c-s3 library doesn't shut down
     * cleanly when called from HDF5's default atexit() handler.
     */
    if (0 != atexit(H5FD__s3comms_term_func))
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                    "couldn't register function for cleaning up aws-c-s3 library");

    H5FD_ros3_aws_init_g = true;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__s3comms_init() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_term_func
 *
 * Purpose:     atexit() handler function to terminate the S3 communications
 *              interface. This is currently needed to work around ordering
 *              issues that cause a crash when the aws-c-s3 library is cleaned
 *              up from HDF5's default atexit() handler. A better solution
 *              would be to require a call to H5close() to explicitly
 *              terminate access to HDF5 before exiting main(), but this will
 *              suffice for the time being.
 *
 * Return:      Non-negative on success/Negative on failure
 *----------------------------------------------------------------------------
 */
static void
H5FD__s3comms_term_func(void)
{
    if (H5FD_ros3_aws_init_g) {
        aws_host_resolver_release(H5FD_ros3_aws_host_resolver_g);
        aws_event_loop_group_release(H5FD_ros3_aws_event_loop_group_g);
        aws_s3_library_clean_up();

        /* Clean up logger interface after being sure everything else
         * has finished cleaning up
         */
        if (H5FD_ros3_aws_log_init_g)
            aws_logger_clean_up(&H5FD_ros3_aws_logger_g);

        H5FD_ros3_debug_g = false;

        H5FD_ros3_aws_init_g = false;
    }
} /* end H5FD__s3comms_term_func() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_term
 *
 * Purpose:     Terminate the S3 communications interface.
 *
 * Return:      Non-negative on success/Negative on failure
 *----------------------------------------------------------------------------
 */
herr_t
H5FD__s3comms_term(void)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE_NOERR

    /* Currently handled by atexit() function above to work around cleanup
     * ordering issues.
     */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__s3comms_term() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_s3r_req_finish_cb
 *
 * Purpose:     Callback to process a finished request
 *
 * Return:      Nothing
 *----------------------------------------------------------------------------
 */
static void
H5FD__s3comms_s3r_req_finish_cb(struct aws_s3_meta_request H5_ATTR_UNUSED *meta_request,
                                const struct aws_s3_meta_request_result *meta_request_result, void *user_data)
{
    H5FD__s3comms_request_t *s3comms_request = NULL;
    int                      ret_value       = AWS_OP_SUCCESS;

    s3comms_request = (H5FD__s3comms_request_t *)user_data;
    if (!s3comms_request) {
        ret_value = aws_raise_error(AWS_ERROR_S3_CANCELED);
        goto done;
    }

    /*
     * From aws-c-s3 docs: If error_code is equal to AWS_ERROR_S3_INVALID_RESPONSE_STATUS,
     * then error_response_headers, error_response_body, and response_status will be
     * populated by the failed request.
     *
     * For all other error codes, response_status will be 0, and the error_response variables
     * will be NULL.
     */
    if (AWS_ERROR_SUCCESS != meta_request_result->error_code) {
        bool handled_http_status = false;

        /* Process some generic errors */
        s3comms_request->err_msg =
            H5FD__s3comms_httpcode_to_str(meta_request_result->response_status, &handled_http_status);

        if (handled_http_status)
            ret_value = aws_raise_error(AWS_ERROR_S3_CANCELED);
        else {
            /* Propagate previous error */
            s3comms_request->err_msg = aws_error_str(meta_request_result->error_code);
            ret_value                = AWS_OP_ERR;
        }

        /* If debugging is enabled, print out the error from the AWS library and the
         * HTTP response headers.
         */
        if (H5FD_ros3_debug_g) {
            if (meta_request_result->error_response_operation_name)
                fprintf(stderr, " -- %s request failed with error: %s\n",
                        aws_string_bytes(meta_request_result->error_response_operation_name),
                        aws_error_str(meta_request_result->error_code));
            else
                fprintf(stderr, " -- request failed with error: %s\n",
                        aws_error_str(meta_request_result->error_code));

            if (AWS_ERROR_S3_INVALID_RESPONSE_STATUS == meta_request_result->error_code) {
                if (meta_request_result->error_response_headers) {
                    size_t num_headers = aws_http_headers_count(meta_request_result->error_response_headers);

                    if (num_headers) {
                        fprintf(stderr, " -- response headers:\n");
                        for (size_t hdr_idx = 0; hdr_idx < num_headers; hdr_idx++) {
                            struct aws_http_header header = {0};
                            int                    ret    = 0;

                            ret = aws_http_headers_get_index(meta_request_result->error_response_headers,
                                                             hdr_idx, &header);
                            if (AWS_ERROR_INVALID_INDEX == ret) {
                                ret_value = aws_raise_error(AWS_ERROR_S3_CANCELED);
                                goto done;
                            }

                            fprintf(stderr, PRInSTR ": " PRInSTR "\n", AWS_BYTE_CURSOR_PRI(header.name),
                                    AWS_BYTE_CURSOR_PRI(header.value));
                        }
                    }
                }

                if (meta_request_result->error_response_body) {
                    fprintf(stderr, " -- response body:\n");
                    fprintf(stderr, "    ");
                    fprintf(stderr, PRInSTR "\n",
                            AWS_BYTE_BUF_PRI((*meta_request_result->error_response_body)));
                }
            }

            fflush(stderr);
        }
    }

    if (H5FD_ros3_debug_g) {
        fprintf(stderr, " -- final HTTP status code: %d\n", meta_request_result->response_status);
        fflush(stderr);
    }

done:
    if (s3comms_request) {
        /* Note that if we fail to lock the mutex here, any thread
         * waiting on the condition variable until s3comms_request->finished
         * is true will hang, unless the wait has a timeout.
         */
        if (AWS_OP_SUCCESS != aws_mutex_lock(s3comms_request->mutex)) {
            /* Propagate previous error */
            s3comms_request->err_msg = aws_error_str(aws_last_error());
            ret_value                = AWS_OP_ERR;
        }
        else {
            s3comms_request->status   = (ret_value == AWS_OP_SUCCESS) ? SUCCEED : FAIL;
            s3comms_request->finished = true;

            if (AWS_OP_SUCCESS != aws_condition_variable_notify_one(s3comms_request->cvar)) {
                /* Propagate previous error */
                s3comms_request->err_msg = aws_error_str(aws_last_error());
                ret_value                = AWS_OP_ERR;
            }

            if (AWS_OP_SUCCESS != aws_mutex_unlock(s3comms_request->mutex)) {
                /* Propagate previous error */
                s3comms_request->err_msg = aws_error_str(aws_last_error());
                ret_value                = AWS_OP_ERR;
            }
        }
    }
} /* end H5FD__s3comms_s3r_req_finish_cb() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_s3r_req_finish_pred
 *
 * Purpose:     Predicate to signal that a request has finished and a thread
 *              waiting on the condition variable for the request may proceed
 *
 * Return:      true if request is finished / false otherwise
 *----------------------------------------------------------------------------
 */
static bool
H5FD__s3comms_s3r_req_finish_pred(void *user_data)
{
    H5FD__s3comms_request_t *request = (H5FD__s3comms_request_t *)user_data;

    return request->finished;
} /* end H5FD__s3comms_s3r_req_finish_pred() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_cred_provider_get_creds_cb
 *
 * Purpose:     Callback function called when sourcing credentials from a
 *              credentials provider. Once this callback has completed
 *              successfully, a reference will be held on the credentials
 *              object if credentials were available and the caller should
 *              release that reference with aws_credentials_release().
 *
 * Return:      Nothing
 *----------------------------------------------------------------------------
 */
static void
H5FD__s3comms_cred_provider_get_creds_cb(struct aws_credentials *credentials, int error_code, void *user_data)
{
    H5FD__s3comms_check_cred_provider_params_t *check_params = NULL;
    bool                                        cred_ref_inc = false;
    int                                         ret_value    = AWS_OP_SUCCESS;

    check_params = (H5FD__s3comms_check_cred_provider_params_t *)user_data;
    if (!check_params) {
        ret_value = aws_raise_error(AWS_ERROR_S3_CANCELED);
        goto done;
    }

    /* Note that if we fail to lock the mutex here, any thread
     * waiting on the condition variable until check_params->request.finished
     * is true will hang, unless the wait has a timeout.
     */
    if (AWS_OP_SUCCESS != aws_mutex_lock(check_params->request.mutex)) {
        /* Propagate previous error */
        check_params->request.err_msg = aws_error_str(aws_last_error());
        ret_value                     = AWS_OP_ERR;
        goto done;
    }

    check_params->credentials = credentials;
    if (credentials) {
        /* Hold a reference on the credentials until the code waiting on this callback can check them */
        aws_credentials_acquire(credentials);
        cred_ref_inc = true;
    }

    check_params->error_code       = error_code;
    check_params->request.status   = (ret_value == AWS_OP_SUCCESS) ? SUCCEED : FAIL;
    check_params->request.finished = true;

    if (AWS_OP_SUCCESS != aws_condition_variable_notify_one(check_params->request.cvar)) {
        /* Propagate previous error */
        aws_mutex_unlock(check_params->request.mutex);
        check_params->request.err_msg = aws_error_str(aws_last_error());
        ret_value                     = AWS_OP_ERR;
        goto done;
    }

    if (AWS_OP_SUCCESS != aws_mutex_unlock(check_params->request.mutex)) {
        /* Propagate previous error */
        check_params->request.err_msg = aws_error_str(aws_last_error());
        ret_value                     = AWS_OP_ERR;
        goto done;
    }

done:
    if (AWS_OP_SUCCESS != ret_value) {
        if (cred_ref_inc)
            aws_credentials_release(credentials);
    }
} /* end H5FD__s3comms_cred_provider_get_creds_cb() */

static bool
H5FD__s3comms_cred_provider_pred(void *user_data)
{
    H5FD__s3comms_request_t *request = (H5FD__s3comms_request_t *)user_data;

    return request->finished;
} /* end H5FD__s3comms_cred_provider_pred() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_s3r_open
 *
 * Purpose:     Logically open a file hosted on S3
 *
 *              fa can be NULL (implies no authentication)
 *              fapl_token can be NULL
 *
 * Return:      SUCCESS:    Pointer to new request handle.
 *              FAILURE:    NULL
 *----------------------------------------------------------------------------
 */
s3r_t *
H5FD__s3comms_s3r_open(const char *url, const H5FD_ros3_fapl_t *fa, const char *fapl_token,
                       const char *alt_endpoint)
{
    struct aws_s3_client               *client                = NULL;
    struct aws_client_bootstrap        *client_bootstrap      = NULL;
    struct aws_credentials_provider    *credentials_provider  = NULL;
    struct aws_s3_client_config         client_config         = {0};
    struct aws_client_bootstrap_options client_bootstrap_opts = {0};
    struct aws_byte_cursor              region_cursor         = {0};
    H5FD__s3comms_aws_params_t         *aws_params            = NULL;
    s3r_t                              *handle                = NULL;
    char                               *pathstyle_env         = NULL;
    char                               *endpoint_env          = NULL;
    bool                                mutex_init            = false;
    bool                                cond_var_init         = false;
    s3r_t                              *ret_value             = NULL;

    FUNC_ENTER_PACKAGE

    if (!url)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "url cannot be NULL");
    if (url[0] == '\0')
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "url cannot be an empty string");

    /* Create handle and set fields */
    if (NULL == (handle = calloc(1, sizeof(s3r_t))))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "could not allocate space for handle");

    if (NULL == (aws_params = calloc(1, sizeof(H5FD__s3comms_aws_params_t))))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "could not allocate space for private handle data");
    handle->priv_data = aws_params;

    if (AWS_OP_SUCCESS != aws_mutex_init(&aws_params->req_mutex))
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, NULL, "couldn't initialize mutex: %s",
                    aws_error_str(aws_last_error()));
    mutex_init = true;

    if (AWS_OP_SUCCESS != aws_condition_variable_init(&aws_params->req_cvar))
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, NULL, "couldn't initialize condition variable: %s",
                    aws_error_str(aws_last_error()));
    cond_var_init = true;

    /* Check if path-style requests should be forced */
    pathstyle_env = getenv(HDF5_ROS3_VFD_FORCE_PATH_STYLE);
    if (pathstyle_env && (*pathstyle_env != '\0')) {
        if (0 != HDstrcasecmp(pathstyle_env, "false") && 0 != HDstrcasecmp(pathstyle_env, "off") &&
            0 != strcmp(pathstyle_env, "0"))
            aws_params->force_path_style = true;
    }

    /*
     * Set up a new AWS client with default options
     */

    aws_params->allocator        = H5FD_ros3_aws_allocator_g;
    aws_params->event_loop_group = H5FD_ros3_aws_event_loop_group_g;
    aws_params->host_resolver    = H5FD_ros3_aws_host_resolver_g;

    /* Create client bootstrap information from event loop group and host resolver */
    client_bootstrap_opts.event_loop_group = H5FD_ros3_aws_event_loop_group_g;
    client_bootstrap_opts.host_resolver    = H5FD_ros3_aws_host_resolver_g;
    client_bootstrap = aws_client_bootstrap_new(aws_params->allocator, &client_bootstrap_opts);
    if (!client_bootstrap)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, NULL, "couldn't bootstrap AWS client: %s",
                    aws_error_str(aws_last_error()));

    aws_params->client_bootstrap = client_bootstrap;

    /* Setup AWS region */
    if (H5FD__s3comms_get_aws_region(aws_params, fa, &handle->aws_region) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, NULL, "couldn't determine AWS region");

    /* Require that a region is specified rather than defaulting to
     * a pre-chosen region, as this could potentially incur unintended
     * data transfer costs.
     */
    if (!handle->aws_region)
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, NULL, "AWS region wasn't specified");

    H5_WARN_AGGREGATE_RETURN_OFF
    region_cursor = aws_byte_cursor_from_c_str(handle->aws_region);
    H5_WARN_AGGREGATE_RETURN_ON

    /* Create a credentials provider for authentication */
    if (H5FD__s3comms_get_credentials_provider(aws_params, fa, fapl_token, &credentials_provider) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, NULL,
                    "couldn't create AWS credentials provider for authentication");

    assert(credentials_provider);
    aws_params->credentials_provider = credentials_provider;

    /* Initialize signing configuration information */
    aws_s3_init_default_signing_config(&aws_params->signing_config, region_cursor, credentials_provider);

    /* AWS C S3 sample code turns this flag off, but it's not clear when this is needed */
    aws_params->signing_config.flags.use_double_uri_encode = false;

    client_config.client_bootstrap = client_bootstrap;
    client_config.region           = region_cursor;
    client_config.signing_config   = &aws_params->signing_config;

    client = aws_s3_client_new(aws_params->allocator, &client_config);
    if (!client)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, NULL, "couldn't initialize AWS client: %s",
                    aws_error_str(aws_last_error()));

    aws_params->client = client;

    /* Parse URL */
    if (NULL == (handle->purl = H5FD__s3comms_parse_url(handle, url, &aws_params->parsed_uri)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "could not allocate and create parsed URL");
    if (!handle->purl->bucket_name)
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "invalid URL specified - could not parse bucket name");
    if (!handle->purl->key)
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "invalid URL specified - could not parse object key");

    /* If no alternate endpoint URL was specified in the FAPL, check to see
     * if one of the AWS environment variables were specified
     */
    if (!alt_endpoint) {
        endpoint_env = getenv("AWS_ENDPOINT_URL_S3");
        if (endpoint_env && (*endpoint_env != '\0'))
            alt_endpoint = endpoint_env;
        else {
            endpoint_env = getenv("AWS_ENDPOINT_URL");
            if (endpoint_env && (*endpoint_env != '\0'))
                alt_endpoint = endpoint_env;
        }
    }

    if (alt_endpoint && (*alt_endpoint != '\0')) {
        if (H5FD_ros3_debug_g)
            fprintf(stderr, " -- parsing alternative endpoint URL\n");

        if (NULL == (handle->alternate_purl =
                         H5FD__s3comms_parse_url(handle, alt_endpoint, &aws_params->alt_parsed_uri)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL,
                        "could not allocate and create parsed alternate endpoint URL");
        if (!handle->alternate_purl->host)
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL,
                        "invalid alternate endpoint URL specified - could not parse host name");
    }

    /* Get the S3 object's size. This is the only time we touch the S3 object
     * (and thus ensure it exists) during the VFD's open callback.
     */
    if (H5FD__s3comms_s3r_getsize(handle) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, NULL, "couldn't get S3 object's size");

    ret_value = handle;

done:
    if (ret_value == NULL) {
        aws_s3_client_release(client);
        aws_credentials_provider_release(credentials_provider);
        aws_client_bootstrap_release(client_bootstrap);

        if (aws_params) {
            if (cond_var_init)
                aws_condition_variable_clean_up(&aws_params->req_cvar);
            if (mutex_init)
                aws_mutex_clean_up(&aws_params->req_mutex);

            aws_uri_clean_up(&aws_params->parsed_uri);
            if (alt_endpoint)
                aws_uri_clean_up(&aws_params->alt_parsed_uri);
            free(aws_params);
        }

        if (handle) {
            /* Freed above */
            handle->priv_data = NULL;

            if (H5FD__s3comms_s3r_close(handle) < 0)
                HDONE_ERROR(H5E_VFL, H5E_CANTFREE, NULL, "can't free handle");
        }
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__s3comms_s3r_open() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_s3r_close
 *
 * Purpose:     Close communications through given S3 Request Handle (s3r_t)
 *              and clean up associated resources
 *
 * Return:      SUCCEED/FAIL
 *----------------------------------------------------------------------------
 */
herr_t
H5FD__s3comms_s3r_close(s3r_t *handle)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (handle == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "handle cannot be NULL");

    free(handle->aws_region);

    if (handle->priv_data) {
        H5FD__s3comms_aws_params_t *aws_params;

        aws_params = (H5FD__s3comms_aws_params_t *)handle->priv_data;

        aws_uri_clean_up(&aws_params->parsed_uri);
        if (handle->alternate_purl)
            aws_uri_clean_up(&aws_params->alt_parsed_uri);

        aws_s3_client_release(aws_params->client);
        aws_params->client = NULL;

        aws_credentials_provider_release(aws_params->credentials_provider);
        aws_params->credentials_provider = NULL;

        aws_client_bootstrap_release(aws_params->client_bootstrap);
        aws_params->client_bootstrap = NULL;

        aws_condition_variable_clean_up(&aws_params->req_cvar);
        aws_mutex_clean_up(&aws_params->req_mutex);

        free(handle->priv_data);
    }

    if (H5FD__s3comms_free_purl(handle->purl) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "unable to release parsed url structure");
    if (H5FD__s3comms_free_purl(handle->alternate_purl) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "unable to release parsed url structure");

    free(handle);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5FD__s3comms_s3r_close */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_s3r_read
 *
 * Purpose:     Read file pointed to by request handle, writing specified
 *              offset .. (offset + len - 1) bytes to buffer dest
 *
 *              If `len` is 0, reads entirety of file starting at `offset`.
 *              If `offset` and `len` are both 0, reads entire file.
 *
 *              If `offset + len` is greater than the file size, read is
 *              aborted and returns FAIL.
 *
 * Return:      Non-negative on success/Negative on failure
 *----------------------------------------------------------------------------
 */
herr_t
H5FD__s3comms_s3r_read(s3r_t *handle, haddr_t offset, size_t len, void *dest, size_t dest_size)
{
    H5FD__s3comms_read_params_t        read_params          = {0};
    H5FD__s3comms_aws_params_t        *aws_params           = NULL;
    struct aws_http_message           *request_http_message = NULL;
    struct aws_s3_meta_request        *request              = NULL;
    struct aws_s3_meta_request_options request_opts         = {0};
    herr_t                             ret_value            = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (!handle)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid handle");
    if (!handle->purl)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid parsed url in handle");
    if (!handle->priv_data)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid handle private data");
    if ((offset >= handle->filesize) || (offset + len > handle->filesize))
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "unable to read past EOF (%zu)", handle->filesize);

    if (H5FD_ros3_debug_g) {
        if (len > 0) {
            fprintf(stderr, " -- GET: Bytes %" PRIuHADDR " - %" PRIuHADDR ", Request Size: %zu\n", offset,
                    offset + len - 1, len);
        }
        else {
            fprintf(stderr, " -- GET: Bytes %" PRIuHADDR " - %" PRIuHADDR ", Request Size: %zu\n", offset,
                    handle->filesize - 1, handle->filesize - offset);
        }

        fflush(stderr);
    }

    aws_params = (H5FD__s3comms_aws_params_t *)handle->priv_data;

    /* Setup HTTP GET request message */
    if (H5FD__s3comms_format_http_request_message(aws_params, "GET", handle->purl->bucket_name,
                                                  handle->purl->key, &request_http_message) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "couldn't create HTTP request message");

    /* Add "Host" header to HTTP message */
    if (H5FD__s3comms_format_host_header(
            aws_params, request_http_message, handle->purl->bucket_name,
            (handle->alternate_purl ? handle->alternate_purl->host : handle->purl->host),
            (handle->alternate_purl ? handle->alternate_purl->port : handle->purl->port)) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "couldn't create HTTP 'Host' header");

    /* Add "User-Agent" header to HTTP message */
    if (H5FD__s3comms_format_user_agent_header(request_http_message) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "couldn't create HTTP 'User-Agent' header");

    /* Setup "Range: " header for retrieving a specific byte range if desired */
    if (H5FD__s3comms_format_range_header(request_http_message, offset, len) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "couldn't create HTTP 'Range' header");

    /* Print out request headers if debugging is enabled */
    if (H5FD_ros3_debug_g) {
        struct aws_http_headers *request_headers;

        if ((request_headers = aws_http_message_get_headers(request_http_message))) {
            size_t num_headers = aws_http_headers_count(request_headers);

            if (num_headers) {
                fprintf(stderr, " -- request headers:\n");
                for (size_t hdr_idx = 0; hdr_idx < num_headers; hdr_idx++) {
                    struct aws_http_header header = {0};
                    int                    ret    = 0;

                    ret = aws_http_headers_get_index(request_headers, hdr_idx, &header);
                    if (AWS_ERROR_INVALID_INDEX != ret) {
                        fprintf(stderr, PRInSTR ": " PRInSTR "\n", AWS_BYTE_CURSOR_PRI(header.name),
                                AWS_BYTE_CURSOR_PRI(header.value));
                    }
                }

                fflush(stderr);
            }
        }
    }

    /* Initialize callback parameters */
    read_params.request.status   = SUCCEED;
    read_params.request.finished = false;
    read_params.request.mutex    = &aws_params->req_mutex;
    read_params.request.cvar     = &aws_params->req_cvar;
    read_params.read_buf         = dest;
    read_params.read_buf_size    = dest_size;
    read_params.read_offset      = offset;

    /* Setup a GetObject request to retrieve the requested data */
    request_opts.type            = AWS_S3_META_REQUEST_TYPE_GET_OBJECT;
    request_opts.signing_config  = NULL; /* Use signing config from client */
    request_opts.message         = request_http_message;
    request_opts.body_callback   = H5FD__s3comms_s3r_read_cb;
    request_opts.finish_callback = H5FD__s3comms_s3r_req_finish_cb;
    request_opts.user_data       = &read_params;

    /* Set alternate endpoint if specified */
    if (handle->alternate_purl)
        request_opts.endpoint = &aws_params->alt_parsed_uri;

    request = aws_s3_client_make_meta_request(aws_params->client, &request_opts);
    if (!request)
        HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "couldn't create AWS meta-request: %s",
                    aws_error_str(aws_last_error()));

    /* Wait for request to complete */
    if (AWS_OP_SUCCESS != aws_mutex_lock(read_params.request.mutex))
        HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "couldn't lock mutex: %s", aws_error_str(aws_last_error()));

    if (AWS_OP_SUCCESS !=
        aws_condition_variable_wait_pred(read_params.request.cvar, read_params.request.mutex,
                                         H5FD__s3comms_s3r_req_finish_pred, &read_params.request)) {
        aws_mutex_unlock(read_params.request.mutex);
        HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "couldn't wait on condition variable: %s",
                    aws_error_str(aws_last_error()));
    }

    if (AWS_OP_SUCCESS != aws_mutex_unlock(read_params.request.mutex))
        HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "couldn't unlock mutex: %s",
                    aws_error_str(aws_last_error()));

    assert(read_params.request.finished);

    if (read_params.request.status != SUCCEED) {
        /* If error message isn't set, some internal error occurred in aws-c-s3 */
        if (!read_params.request.err_msg)
            read_params.request.err_msg = "internal error during request setup";

        HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "error occurred while reading s3 object: %s",
                    read_params.request.err_msg);
    }

done:
    aws_http_message_release(request_http_message);
    aws_s3_meta_request_release(request);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__s3comms_s3r_read() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_s3r_read_cb
 *
 * Purpose:     Callback to receive the body for the request to get an
 *              object's data
 *
 * Return:      AWS_OP_SUCCESS on success / AWS_OP_ERR on failure
 *----------------------------------------------------------------------------
 */
static int
H5FD__s3comms_s3r_read_cb(struct aws_s3_meta_request H5_ATTR_UNUSED *meta_request,
                          const struct aws_byte_cursor *body, uint64_t range_start, void *user_data)
{
    H5FD__s3comms_read_params_t *read_params = NULL;
    size_t                       buf_offset  = 0;
    int                          ret_value   = AWS_OP_SUCCESS;

    read_params = (H5FD__s3comms_read_params_t *)user_data;
    if (!read_params) {
        ret_value = aws_raise_error(AWS_ERROR_S3_CANCELED);
        goto done;
    }

    /* If no read buffer is supplied, do nothing */
    if (!read_params->read_buf)
        goto done;

    if (range_start < read_params->read_offset) {
        read_params->request.err_msg =
            "internal error - data chunk offset was smaller than initial buffer offset";
        ret_value = aws_raise_error(AWS_ERROR_S3_CANCELED);
        goto done;
    }

    if (read_params->read_offset > UINT64_MAX) {
        read_params->request.err_msg = "internal error - initial buffer offset was too large";
        ret_value                    = aws_raise_error(AWS_ERROR_S3_CANCELED);
        goto done;
    }

    buf_offset = range_start - (uint64_t)read_params->read_offset;

    if (buf_offset >= read_params->read_buf_size) {
        read_params->request.err_msg = "internal error - data chunk offset was outside read buffer";
        ret_value                    = aws_raise_error(AWS_ERROR_S3_CANCELED);
        goto done;
    }

    /* Check for overflowing read buffer */
    if (buf_offset + body->len > read_params->read_buf_size) {
        read_params->request.err_msg = "internal error - buffer overflow during read";
        ret_value                    = aws_raise_error(AWS_ERROR_S3_CANCELED);
        goto done;
    }

    memcpy(read_params->read_buf + buf_offset, body->ptr, body->len);

done:
    if (AWS_OP_SUCCESS != ret_value) {
        if (read_params)
            read_params->request.status = FAIL;
    }

    return ret_value;
} /* end H5FD__s3comms_s3r_read_cb() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_s3r_get_filesize
 *
 * Purpose:     Retrieve the filesize of an open request handle
 *
 * Return:      SUCCEED/FAIL
 *----------------------------------------------------------------------------
 */
size_t
H5FD__s3comms_s3r_get_filesize(s3r_t *handle)
{
    size_t ret_value = 0;

    FUNC_ENTER_PACKAGE_NOERR

    if (handle != NULL)
        ret_value = handle->filesize;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__s3comms_s3r_get_filesize() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_s3r_getsize
 *
 * Purpose:     Get the number of bytes of handle's target resource
 *
 * Return:      SUCCEED/FAIL
 *----------------------------------------------------------------------------
 */
static herr_t
H5FD__s3comms_s3r_getsize(s3r_t *handle)
{
    H5FD__s3comms_getsize_params_t     getsize_params       = {0};
    H5FD__s3comms_aws_params_t        *aws_params           = NULL;
    struct aws_http_message           *request_http_message = NULL;
    struct aws_s3_meta_request        *request              = NULL;
    struct aws_s3_meta_request_options request_opts         = {0};
    struct aws_byte_cursor             aws_op_name_cursor   = {0};
    herr_t                             ret_value            = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(handle);

    if (H5FD_ros3_debug_g) {
        fprintf(stderr, " -- HEAD: Bucket: %s / Key: %s\n", handle->purl->bucket_name, handle->purl->key);
        fflush(stderr);
    }

    aws_params = (H5FD__s3comms_aws_params_t *)handle->priv_data;

    /* Setup HTTP HEAD request message */
    if (H5FD__s3comms_format_http_request_message(aws_params, "HEAD", handle->purl->bucket_name,
                                                  handle->purl->key, &request_http_message) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "couldn't create HTTP request message");

    /* Add "Host" header to HTTP message */
    if (H5FD__s3comms_format_host_header(
            aws_params, request_http_message, handle->purl->bucket_name,
            (handle->alternate_purl ? handle->alternate_purl->host : handle->purl->host),
            (handle->alternate_purl ? handle->alternate_purl->port : handle->purl->port)) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "couldn't create HTTP 'Host' header");

    /* Add "User-Agent" header to HTTP message */
    if (H5FD__s3comms_format_user_agent_header(request_http_message) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "couldn't create HTTP 'User-Agent' header");

    /* Print out request headers if debugging is enabled */
    if (H5FD_ros3_debug_g) {
        struct aws_http_headers *request_headers;

        if ((request_headers = aws_http_message_get_headers(request_http_message))) {
            size_t num_headers = aws_http_headers_count(request_headers);

            if (num_headers) {
                fprintf(stderr, " -- request headers:\n");
                for (size_t hdr_idx = 0; hdr_idx < num_headers; hdr_idx++) {
                    struct aws_http_header header = {0};
                    int                    ret    = 0;

                    ret = aws_http_headers_get_index(request_headers, hdr_idx, &header);
                    if (AWS_ERROR_INVALID_INDEX != ret) {
                        fprintf(stderr, PRInSTR ": " PRInSTR "\n", AWS_BYTE_CURSOR_PRI(header.name),
                                AWS_BYTE_CURSOR_PRI(header.value));
                    }
                }

                fflush(stderr);
            }
        }
    }

    /* Initialize callback parameters */
    getsize_params.request.status   = SUCCEED;
    getsize_params.request.finished = false;
    getsize_params.request.mutex    = &aws_params->req_mutex;
    getsize_params.request.cvar     = &aws_params->req_cvar;
    aws_byte_buf_init(&getsize_params.response_body, aws_params->allocator, 1024);

    /* Setup a HeadObject request to get the object's size */
    H5_WARN_AGGREGATE_RETURN_OFF
    aws_op_name_cursor = aws_byte_cursor_from_c_str("HeadObject");
    H5_WARN_AGGREGATE_RETURN_ON

    request_opts.type             = AWS_S3_META_REQUEST_TYPE_DEFAULT;
    request_opts.operation_name   = aws_op_name_cursor;
    request_opts.signing_config   = NULL; /* Use signing config from client */
    request_opts.message          = request_http_message;
    request_opts.headers_callback = H5FD__s3comms_s3r_getsize_headers_cb;
    request_opts.finish_callback  = H5FD__s3comms_s3r_req_finish_cb;
    request_opts.user_data        = &getsize_params;

    /* Set alternate endpoint if specified */
    if (handle->alternate_purl)
        request_opts.endpoint = &aws_params->alt_parsed_uri;

    request = aws_s3_client_make_meta_request(aws_params->client, &request_opts);
    if (!request)
        HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "couldn't create AWS meta-request: %s",
                    aws_error_str(aws_last_error()));

    /* Wait for request to complete */
    if (AWS_OP_SUCCESS != aws_mutex_lock(getsize_params.request.mutex))
        HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "couldn't lock mutex: %s", aws_error_str(aws_last_error()));

    if (AWS_OP_SUCCESS !=
        aws_condition_variable_wait_pred(getsize_params.request.cvar, getsize_params.request.mutex,
                                         H5FD__s3comms_s3r_req_finish_pred, &getsize_params.request)) {
        aws_mutex_unlock(getsize_params.request.mutex);
        HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "couldn't wait on condition variable: %s",
                    aws_error_str(aws_last_error()));
    }

    if (AWS_OP_SUCCESS != aws_mutex_unlock(getsize_params.request.mutex))
        HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "couldn't unlock mutex: %s",
                    aws_error_str(aws_last_error()));

    assert(getsize_params.request.finished);

    if (getsize_params.request.status != SUCCEED) {
        /* If error message isn't set, some internal error occurred in aws-c-s3 */
        if (!getsize_params.request.err_msg)
            getsize_params.request.err_msg = "internal error during request setup";

        HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "error occurred while getting s3 object size: %s",
                    getsize_params.request.err_msg);
    }

    if (getsize_params.object_size == 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "Content-Length of S3 object was 0");

    handle->filesize = getsize_params.object_size;

    if (H5FD_ros3_debug_g) {
        fprintf(stderr, " -- file size: %zu bytes\n", handle->filesize);
        fflush(stderr);
    }

done:
    aws_http_message_release(request_http_message);
    aws_byte_buf_clean_up(&getsize_params.response_body);
    aws_s3_meta_request_release(request);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__s3comms_s3r_getsize() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_s3r_getsize_headers_cb
 *
 * Purpose:     Callback to receive the response headers for the request to
 *              get an object's size
 *
 * Return:      AWS_OP_SUCCESS on success / AWS_OP_ERR on failure
 *----------------------------------------------------------------------------
 */
static int
H5FD__s3comms_s3r_getsize_headers_cb(struct aws_s3_meta_request H5_ATTR_UNUSED *meta_request,
                                     const struct aws_http_headers *headers, int response_status,
                                     void *user_data)
{
    H5FD__s3comms_getsize_params_t *params         = NULL;
    struct aws_byte_cursor          header_name    = {0};
    struct aws_byte_cursor          out_cursor     = {0};
    uint64_t                        content_length = 0;
    int                             ret_value      = AWS_OP_SUCCESS;

    params = (H5FD__s3comms_getsize_params_t *)user_data;
    if (!params) {
        ret_value = aws_raise_error(AWS_ERROR_S3_CANCELED);
        goto done;
    }

    if (H5FD_ros3_debug_g) {
        fprintf(stderr, " -- response status: %d\n", response_status);
        fflush(stderr);
    }

    if (HTTP_CLIENT_SUCCESS(response_status)) {
        /* If debugging is enabled, print the response headers. On error,
         * they will be printed in the request finish callback.
         */
        if (H5FD_ros3_debug_g) {
            size_t num_headers = aws_http_headers_count(headers);

            if (num_headers) {
                fprintf(stderr, " -- response headers:\n");
                for (size_t hdr_idx = 0; hdr_idx < num_headers; hdr_idx++) {
                    struct aws_http_header header = {0};
                    int                    ret    = 0;

                    ret = aws_http_headers_get_index(headers, hdr_idx, &header);
                    if (AWS_ERROR_INVALID_INDEX == ret) {
                        ret_value = aws_raise_error(AWS_ERROR_S3_CANCELED);
                        goto done;
                    }

                    fprintf(stderr, PRInSTR ": " PRInSTR "\n", AWS_BYTE_CURSOR_PRI(header.name),
                            AWS_BYTE_CURSOR_PRI(header.value));
                }

                fflush(stderr);
            }
        }

        H5_WARN_AGGREGATE_RETURN_OFF
        header_name = aws_byte_cursor_from_c_str("Content-Length");
        H5_WARN_AGGREGATE_RETURN_ON

        if (AWS_OP_SUCCESS != aws_http_headers_get(headers, header_name, &out_cursor)) {
            if (AWS_ERROR_HTTP_HEADER_NOT_FOUND == aws_last_error()) {
                params->request.err_msg = aws_error_str(AWS_ERROR_S3_MISSING_CONTENT_LENGTH_HEADER);
                ret_value               = aws_raise_error(AWS_ERROR_S3_MISSING_CONTENT_LENGTH_HEADER);
            }
            else {
                ret_value = aws_raise_error(AWS_ERROR_S3_CANCELED);
            }

            goto done;
        }

        if (AWS_OP_SUCCESS != aws_byte_cursor_utf8_parse_u64(out_cursor, &content_length)) {
            params->request.err_msg = "couldn't parse valid value from Content-Length header";
            ret_value               = aws_raise_error(AWS_ERROR_S3_INVALID_CONTENT_LENGTH_HEADER);
            goto done;
        }

        params->object_size = (size_t)content_length;
    }

done:
    if (AWS_OP_SUCCESS != ret_value) {
        if (params)
            params->request.status = FAIL;
    }

    return ret_value;
} /* end H5FD__s3comms_s3r_getsize_headers_cb() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_parse_url
 *
 * Purpose:     Parse a URL into separate components. An aws_uri structure
 *              is populated after parsing the URL, but a more generic
 *              parsed_url_t structure is also returned in order to keep
 *              compatibility with the generic s3comms interface.
 *
 * Return:      Success:    A pointer to a parsed_url_t
 *              Failure:    NULL
 *----------------------------------------------------------------------------
 */
static parsed_url_t *
H5FD__s3comms_parse_url(s3r_t *handle, const char *url, struct aws_uri *uri)
{
    H5FD__s3comms_aws_params_t *aws_params       = NULL;
    struct aws_byte_cursor      url_cursor       = {0};
    struct aws_byte_cursor      s3_cursor        = {0};
    parsed_url_t               *purl             = NULL;
    char                       *host_copy        = NULL;
    char                       *host_component   = NULL;
    bool                        is_s3_url        = false;
    bool                        is_virtual_style = false;
    parsed_url_t               *ret_value        = NULL;

    FUNC_ENTER_PACKAGE

    assert(handle);
    assert(url);
    assert(uri);

    aws_params = (H5FD__s3comms_aws_params_t *)handle->priv_data;

    /* Allocate memory for the parsed URL to return */
    if (NULL == (purl = calloc(1, sizeof(parsed_url_t))))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "can't allocate space for parsed_url_t");

    H5_WARN_AGGREGATE_RETURN_OFF
    url_cursor = aws_byte_cursor_from_c_str(url);
    s3_cursor  = aws_byte_cursor_from_c_str("s3");
    H5_WARN_AGGREGATE_RETURN_ON

    if (AWS_OP_SUCCESS != aws_uri_init_parse(uri, aws_params->allocator, &url_cursor))
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, NULL, "unable to parse url");

    /* Is this a 's3://' url? */
    if (aws_byte_cursor_eq_ignore_case(&uri->scheme, &s3_cursor))
        is_s3_url = true;

    /* URI section buffers use library-specific structure and may also not
     * be NUL-terminated, so make our own s3comms API-compatible C string
     * copies
     */
    if (NULL == (purl->scheme = malloc(uri->scheme.len + 1)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "can't allocate space for URL scheme portion");
    memcpy(purl->scheme, uri->scheme.ptr, uri->scheme.len);
    purl->scheme[uri->scheme.len] = '\0';

    /* Special processing for 's3://' urls */
    if (is_s3_url) {
        if (NULL == (purl->host = malloc(HOST_NAME_LEN)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "can't allocate space for URL host portion");

        /* Bucket name will be prepended to host later on if necessary */
        snprintf(purl->host, HOST_NAME_LEN, "s3.%s.amazonaws.com", handle->aws_region);
    }
    else {
        if (NULL == (purl->host = malloc(uri->host_name.len + 1)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "can't allocate space for URL host portion");
        memcpy(purl->host, uri->host_name.ptr, uri->host_name.len);
        purl->host[uri->host_name.len] = '\0';

        if (uri->port != 0) {
            if (NULL == (purl->port = malloc(11)))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "can't allocate space for URL port portion");
            snprintf(purl->port, 11, "%" PRIu32, uri->port);
        }
    }

    if (NULL == (purl->path = malloc(uri->path.len + 1)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "can't allocate space for URL path portion");
    memcpy(purl->path, uri->path.ptr, uri->path.len);
    purl->path[uri->path.len] = '\0';

    if (NULL == (purl->query = malloc(uri->query_string.len + 1)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "can't allocate space for URL query string portion");
    memcpy(purl->query, uri->query_string.ptr, uri->query_string.len);
    purl->query[uri->query_string.len] = '\0';

    /* Parse out the bucket name and object key */
    if (is_s3_url) {
        if (NULL == (purl->bucket_name = HDstrndup((char *)uri->host_name.ptr, uri->host_name.len)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "can't duplicate bucket name");

        if (uri->path.len == 0)
            HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, NULL, "invalid path parsed from URL");

        /* Path will always include a leading '/' */
        if (NULL == (purl->key = HDstrndup((char *)uri->path.ptr + 1, uri->path.len - 1)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, NULL, "can't duplicate path");
    }
    else if (uri->path.len > 0) {
        /* Attempt to determine whether the URL is a virtual-hosted-style
         * or path-style URL using very simple heuristics
         */
        if (NULL == (host_copy = strdup(purl->host)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, NULL, "can't duplicate host string");

        if (NULL != strstr(host_copy, ".")) {
            if (NULL == (host_component = strtok(host_copy, ".")))
                HGOTO_ERROR(H5E_VFL, H5E_INTERNAL, NULL, "internal error occurred when parsing url");

            /* If the first component is 's3' or a specific endpoint
             * or legacy s3-region_code format, assume this is a
             * path-style URL. This could be problematic for specific
             * bucket names like 's3-files', but should be good enough
             * for now.
             */
            if (strcmp(host_component, "s3") == 0 || strncmp(host_component, "s3-", 3) == 0)
                is_virtual_style = false;
            else {
                /* Attempt to find the ".s3." or ".s3-" portion of the string */
                do {
                    host_component = strtok(NULL, ".");
                } while (host_component && strcmp(host_component, "s3") != 0 &&
                         strncmp(host_component, "s3-", 3) != 0);

                if (!host_component) {
                    /* No '.s3.' or '.s3-' in host; assume path-style for now
                     * to cover testing cases like http://localhost/bucket/key,
                     * though this could be problematic with aliasing mechanisms
                     * or URLs to other s3-compatible storage.
                     */
                    is_virtual_style = false;
                }
                else {
                    is_virtual_style = true;
                }
            }
        }
        else {
            /* No '.' in host; assume path-style for now to cover testing
             * cases like http://localhost/bucket/key, though this could
             * be problematic with aliasing mechanisms.
             */
            is_virtual_style = false;
        }

        if (is_virtual_style) {
            ptrdiff_t bucket_name_len;
            char     *s3_begin;

            /* Copy up to the ".s3." or ".s3-" portion of the string to cover
             * bucket names with "." in them. Note that this could have issues
             * with specific bucket names like 's3-files', but should be good
             * enough for now.
             */
            s3_begin = strstr(purl->host, ".s3.");
            if (!s3_begin)
                s3_begin = strstr(purl->host, ".s3-");
            if (!s3_begin)
                HGOTO_ERROR(H5E_VFL, H5E_CANTGET, NULL, "can't parse bucket name from url");

            bucket_name_len = s3_begin - purl->host;
            if (bucket_name_len < 0)
                HGOTO_ERROR(H5E_VFL, H5E_INTERNAL, NULL,
                            "internal error occurred when parsing bucket name from url");

            if (NULL == (purl->bucket_name = HDstrndup(purl->host, (size_t)bucket_name_len)))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "can't duplicate bucket name");

            /* Path will always include a leading '/' */
            if (NULL == (purl->key = strdup(purl->path + 1)))
                HGOTO_ERROR(H5E_VFL, H5E_CANTGET, NULL, "can't duplicate path");
        }
        else {
            char *slash;

            /* Assume path-style URI */

            /* Path will always include a leading '/' */
            if (NULL == (purl->bucket_name = strdup(purl->path + 1)))
                HGOTO_ERROR(H5E_VFL, H5E_CANTGET, NULL, "can't duplicate path");

            /* Copy object key string before mutating bucket name string */
            if (NULL == (slash = strstr(purl->bucket_name, "/")))
                HGOTO_ERROR(H5E_VFL, H5E_CANTGET, NULL, "can't parse object key from path");

            if (NULL == (purl->key = strdup(slash + 1)))
                HGOTO_ERROR(H5E_VFL, H5E_CANTGET, NULL, "can't duplicate path");

            /* Isolate bucket name component */
            if (NULL == strtok(purl->bucket_name, "/"))
                HGOTO_ERROR(H5E_VFL, H5E_CANTGET, NULL, "can't parse bucket name from path");
        }
    }

    ret_value = purl;

done:
    free(host_copy);

    if (ret_value == NULL) {
        if (H5FD__s3comms_free_purl(purl) < 0)
            HDONE_ERROR(H5E_VFL, H5E_BADVALUE, NULL, "unable to free parsed url structure");
    }
    else if (H5FD_ros3_debug_g && purl) {
        fprintf(stderr, " -- parsed URL as:\n");
        fprintf(stderr, "    - Scheme: %s\n", purl->scheme);
        fprintf(stderr, "    - Host: %s\n", purl->host);
        if (purl->port)
            fprintf(stderr, "    - Port: %s\n", purl->port);
        fprintf(stderr, "    - Path: %s\n", purl->path);
        fprintf(stderr, "    - Query: %s\n", purl->query);
        fprintf(stderr, "    - Bucket: %s\n", purl->bucket_name);
        fprintf(stderr, "    - Key: %s\n", purl->key);
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__s3comms_parse_url() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_free_purl
 *
 * Purpose:     Release resources from a parsed_url_t pointer
 *
 * Return:      SUCCEED (Can't fail - passing NULL is okay)
 *----------------------------------------------------------------------------
 */
static herr_t
H5FD__s3comms_free_purl(parsed_url_t *purl)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE_NOERR

    if (NULL == purl)
        HGOTO_DONE(SUCCEED);

    free(purl->scheme);
    free(purl->host);
    free(purl->port);
    free(purl->path);
    free(purl->query);
    free(purl->bucket_name);
    free(purl->key);

    free(purl);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__s3comms_free_purl() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_get_aws_region
 *
 * Purpose:     Helper function to get a specified AWS region string and
 *              return an allocated copy of it, which the caller must free
 *              with free(). The following are checked in order:
 *
 *              - The File Access Property List specified during file open
 *              - The AWS_REGION environment variable
 *              - The AWS_DEFAULT_REGION environment variable
 *              - The AWS configuration file (~/.aws/config by default)
 *                - The 'default' profile from this file is used, unless a
 *                  different profile is specified with the AWS_PROFILE
 *                  environment variable
 *
 * Return:      Non-negative on success/Negative on failure
 *----------------------------------------------------------------------------
 */
static herr_t
H5FD__s3comms_get_aws_region(const H5FD__s3comms_aws_params_t *aws_params, const H5FD_ros3_fapl_t *fa,
                             char **aws_region_out)
{
    struct aws_profile_collection *config_coll      = NULL;
    const struct aws_profile      *aws_profile      = NULL;
    struct aws_string             *config_file_path = NULL;
    struct aws_string             *profile_name_str = NULL;
    struct aws_string             *region_str       = NULL;
    char                          *region_copy      = NULL;
    herr_t                         ret_value        = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(aws_params);
    assert(aws_region_out);

    /* From FAPL */
    if (fa && fa->aws_region[0] != '\0') {
        if (NULL == (region_copy = HDstrndup(fa->aws_region, H5FD_ROS3_MAX_REGION_LEN + 1)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't copy AWS region from FAPL");
    }

    /* From AWS_REGION environment variable */
    if (!region_copy) {
        char *env_region = getenv("AWS_REGION");

        if (env_region && (*env_region != '\0')) {
            if (NULL == (region_copy = strdup(env_region)))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL,
                            "couldn't copy AWS region string from environment variable");
        }
    }

    /* From AWS_DEFAULT_REGION environment variable */
    if (!region_copy) {
        char *env_default_region = getenv("AWS_DEFAULT_REGION");

        if (env_default_region && (*env_default_region != '\0')) {
            if (NULL == (region_copy = strdup(env_default_region)))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL,
                            "couldn't copy default AWS region string from environment variable");
        }
    }

    /* From AWS configuration file */
    if (!region_copy) {
        const struct aws_profile_property *region   = NULL;
        const struct aws_string           *prop_val = NULL;

        /* Resolve the configuration file name, which could be overridden with AWS_CONFIG_FILE */
        if (NULL == (config_file_path = aws_get_config_file_path(aws_params->allocator, NULL)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't get AWS configuration file path");

        config_coll =
            aws_profile_collection_new_from_file(aws_params->allocator, config_file_path, AWS_PST_CONFIG);
        if (!config_coll)
            HGOTO_DONE(SUCCEED); /* No configuration file to read */

        /* Resolve the profile name to use, which could be overridden with AWS_PROFILE */
        if (NULL == (profile_name_str = aws_get_profile_name(aws_params->allocator, NULL)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL,
                        "couldn't determine AWS profile to retrieve region information from");

        /* Read from the profile if it exists in the configuration file */
        aws_profile = aws_profile_collection_get_profile(config_coll, profile_name_str);
        if (!aws_profile)
            HGOTO_DONE(SUCCEED); /* No such profile in configuration file */

        if (NULL == (region_str = aws_string_new_from_c_str(aws_params->allocator, "region")))
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't allocate space for AWS string");

        region = aws_profile_get_property(aws_profile, region_str);
        if (!region)
            HGOTO_DONE(SUCCEED); /* No 'region' entry under this profile in configuration file */

        prop_val = aws_profile_property_get_value(region);
        if (prop_val)
            if (NULL == (region_copy = strdup(aws_string_c_str(prop_val))))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL,
                            "couldn't copy AWS region string from AWS configuration file");
    }

    *aws_region_out = region_copy;

done:
    aws_profile_collection_destroy(config_coll);
    aws_string_destroy(config_file_path);
    aws_string_destroy(profile_name_str);
    aws_string_destroy(region_str);

    if (ret_value < 0) {
        free(region_copy);
        *aws_region_out = NULL;
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__s3comms_get_aws_region() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_get_credentials_provider
 *
 * Purpose:     Helper function to create a credentials provider object for
 *              use in authentication. If a FAPL is provided and
 *              'authenticate' is true in the FAPL, credentials are only
 *              sourced from the FAPL. If 'authenticate' is false (or no FAPL
 *              is provided), an attempt is made to source credentials with
 *              the default provider chain from the aws-c-s3 library, which
 *              looks at several sources, including the standard AWS
 *              configuration files and environment variables. If no
 *              credentials can be sourced from those places, an anonymous
 *              credentials provider is used.
 *
 * Return:      Non-negative on success/Negative on failure
 *----------------------------------------------------------------------------
 */
static herr_t
H5FD__s3comms_get_credentials_provider(H5FD__s3comms_aws_params_t *aws_params, const H5FD_ros3_fapl_t *fa,
                                       const char                       *fapl_token,
                                       struct aws_credentials_provider **credentials_provider_out)
{
    struct aws_credentials_provider *credentials_provider = NULL;
    herr_t                           ret_value            = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(aws_params);
    assert(credentials_provider_out);

    /* From FAPL */
    if (fa && fa->authenticate) {
        struct aws_credentials_provider_static_options credentials_provider_opts = {0};

        if (fa->secret_id[0] == '\0' || fa->secret_key[0] == '\0')
            HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL,
                        "'secret_id' and 'secret_key' must both be non-empty strings when 'authenticate' is "
                        "true in FAPL");

        H5_WARN_AGGREGATE_RETURN_OFF
        credentials_provider_opts.access_key_id     = aws_byte_cursor_from_c_str(fa->secret_id);
        credentials_provider_opts.secret_access_key = aws_byte_cursor_from_c_str(fa->secret_key);
        if (fapl_token)
            credentials_provider_opts.session_token = aws_byte_cursor_from_c_str(fapl_token);
        H5_WARN_AGGREGATE_RETURN_ON

        credentials_provider =
            aws_credentials_provider_new_static(aws_params->allocator, &credentials_provider_opts);
        if (!credentials_provider)
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                        "couldn't create credentials provider for AWS client: %s",
                        aws_error_str(aws_last_error()));
    }

    /* From standard AWS sources */
    if (!credentials_provider) {
        struct aws_credentials_provider_chain_default_options credentials_provider_opts  = {0};
        H5FD__s3comms_check_cred_provider_params_t            check_cred_provider_params = {0};
        struct aws_byte_cursor                                access_key_id              = {0};
        struct aws_byte_cursor                                secret_access_key          = {0};

        /* Use default credential provider chain. This will currently look for
         * credentials in the following order:
         *
         *  - From the environment (environment variables)
         *  - From profile files (~/.aws/config and ~/.aws/credentials by default)
         *  - STS web identity (temporary security credentials)
         *  - EC2 instance metadata
         */
        credentials_provider_opts.bootstrap = aws_params->client_bootstrap;

        credentials_provider =
            aws_credentials_provider_new_chain_default(aws_params->allocator, &credentials_provider_opts);
        if (!credentials_provider)
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                        "couldn't create default credentials provider chain for AWS client: %s",
                        aws_error_str(aws_last_error()));

        /* Check to see if credentials could be sourced from this provider */
        check_cred_provider_params.request.status   = SUCCEED;
        check_cred_provider_params.request.finished = false;
        check_cred_provider_params.request.mutex    = &aws_params->req_mutex;
        check_cred_provider_params.request.cvar     = &aws_params->req_cvar;
        if (AWS_OP_SUCCESS !=
            aws_credentials_provider_get_credentials(
                credentials_provider, H5FD__s3comms_cred_provider_get_creds_cb, &check_cred_provider_params))
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                        "couldn't check for credentials sourced from default credentials provider chain: %s",
                        aws_error_str(aws_last_error()));

        /* Wait for request to complete */
        if (AWS_OP_SUCCESS != aws_mutex_lock(&aws_params->req_mutex))
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "couldn't lock mutex: %s",
                        aws_error_str(aws_last_error()));

        if (AWS_OP_SUCCESS != aws_condition_variable_wait_pred(&aws_params->req_cvar, &aws_params->req_mutex,
                                                               H5FD__s3comms_cred_provider_pred,
                                                               &check_cred_provider_params.request)) {
            aws_mutex_unlock(&aws_params->req_mutex);
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "couldn't wait on condition variable: %s",
                        aws_error_str(aws_last_error()));
        }

        if (AWS_OP_SUCCESS != aws_mutex_unlock(&aws_params->req_mutex))
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "couldn't unlock mutex: %s",
                        aws_error_str(aws_last_error()));

        assert(check_cred_provider_params.request.finished);

        if (check_cred_provider_params.request.status != SUCCEED) {
            /* If error message isn't set, some internal error occurred in aws-c-s3 */
            if (!check_cred_provider_params.request.err_msg)
                check_cred_provider_params.request.err_msg = "internal error during request setup";

            aws_credentials_release(check_cred_provider_params.credentials);
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                        "couldn't determine if credentials could be source from default credentials provider "
                        "chain: %s",
                        check_cred_provider_params.request.err_msg);
        }

        if (check_cred_provider_params.credentials) {
            H5_WARN_AGGREGATE_RETURN_OFF
            access_key_id     = aws_credentials_get_access_key_id(check_cred_provider_params.credentials);
            secret_access_key = aws_credentials_get_secret_access_key(check_cred_provider_params.credentials);
            H5_WARN_AGGREGATE_RETURN_ON
        }

        if (!check_cred_provider_params.credentials ||
            (access_key_id.len == 0 && secret_access_key.len == 0)) {
            /* Fallback to anonymous credentials */
            aws_credentials_provider_release(credentials_provider);
            credentials_provider = NULL;
        }

        /* Release potential reference taken on credentials object by callback function */
        aws_credentials_release(check_cred_provider_params.credentials);
    }

    /* Use anonymous credentials */
    if (!credentials_provider) {
        credentials_provider = aws_credentials_provider_new_anonymous(aws_params->allocator, NULL);
        if (!credentials_provider)
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                        "couldn't create anonymous credentials provider for AWS client: %s",
                        aws_error_str(aws_last_error()));
    }

    *credentials_provider_out = credentials_provider;

done:
    if (ret_value < 0) {
        aws_credentials_provider_release(credentials_provider);
        *credentials_provider_out = NULL;
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__s3comms_get_credentials_provider() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_format_http_request_message
 *
 * Purpose:     Helper function to format the HTTP message for a request. Note
 *              this only includes the leading request line and excludes any
 *              additional headers, request body, etc.
 *
 *              The returned struct aws_http_message pointer must be freed
 *              by the caller with aws_http_message_release() when finished
 *              with it.
 *
 * Return:      Non-negative on success/Negative on failure
 *----------------------------------------------------------------------------
 */
static herr_t
H5FD__s3comms_format_http_request_message(const H5FD__s3comms_aws_params_t *aws_params,
                                          const char *HTTP_method, const char *bucket_name,
                                          const char *object_key, struct aws_http_message **message_out)
{
    struct aws_http_message *request_http_message = NULL;
    struct aws_byte_cursor   http_method_cursor   = {0};
    struct aws_byte_cursor   path_cursor          = {0};
    struct aws_byte_cursor   key_cursor           = {0};
    struct aws_byte_cursor   slash_cursor         = {0};
    struct aws_byte_buf      path_buf             = {0};
    bool                     use_virtual_style    = true; /* Use virtual-hosted-style request by default */
    herr_t                   ret_value            = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(aws_params);
    assert(HTTP_method);
    assert(bucket_name);
    assert(object_key);
    assert(message_out);

    if (aws_params->force_path_style)
        use_virtual_style = false;

    /*
     * If an alternate endpoint URL was specified, use a path-style
     * request for now. This isn't the most ideal, but is currently
     * required for testing.
     */
    if (aws_params->alt_parsed_uri.self_size != 0)
        use_virtual_style = false;

    /*
     * If bucket has a "." in the name, use a path-style request
     * as virtual-hosted-style requests don't directly support buckets
     * with "." when using HTTPS.
     */
    if (strstr(bucket_name, "."))
        use_virtual_style = false;

    AWS_ZERO_STRUCT(path_buf);

    H5_WARN_AGGREGATE_RETURN_OFF
    http_method_cursor = aws_byte_cursor_from_c_str(HTTP_method);
    key_cursor         = aws_byte_cursor_from_c_str(object_key);
    slash_cursor       = aws_byte_cursor_from_c_str("/");
    H5_WARN_AGGREGATE_RETURN_ON

    request_http_message = aws_http_message_new_request(aws_params->allocator);
    if (!request_http_message)
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't allocate memory for HTTP header: %s",
                    aws_error_str(aws_last_error()));

    if (AWS_OP_SUCCESS != aws_http_message_set_request_method(request_http_message, http_method_cursor))
        HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "couldn't set HTTP request method: %s",
                    aws_error_str(aws_last_error()));

    if (AWS_OP_SUCCESS != aws_byte_buf_init_copy_from_cursor(&path_buf, aws_params->allocator, slash_cursor))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't allocate memory for path string: %s",
                    aws_error_str(aws_last_error()));

    if (!use_virtual_style) {
        struct aws_byte_cursor bucket_cursor;

        H5_WARN_AGGREGATE_RETURN_OFF
        bucket_cursor = aws_byte_cursor_from_c_str(bucket_name);
        H5_WARN_AGGREGATE_RETURN_ON

        if (AWS_OP_SUCCESS != aws_byte_buf_append_dynamic(&path_buf, &bucket_cursor))
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't allocate memory for HTTP request string: %s",
                        aws_error_str(aws_last_error()));

        if (AWS_OP_SUCCESS != aws_byte_buf_append_dynamic(&path_buf, &slash_cursor))
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't allocate memory for HTTP request string: %s",
                        aws_error_str(aws_last_error()));
    }

    if (AWS_OP_SUCCESS != aws_byte_buf_append_dynamic(&path_buf, &key_cursor))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't allocate memory for HTTP request string: %s",
                    aws_error_str(aws_last_error()));

    H5_WARN_AGGREGATE_RETURN_OFF
    path_cursor = aws_byte_cursor_from_buf(&path_buf);
    H5_WARN_AGGREGATE_RETURN_ON

    if (AWS_OP_SUCCESS != aws_http_message_set_request_path(request_http_message, path_cursor))
        HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "couldn't set HTTP request path: %s",
                    aws_error_str(aws_last_error()));

    /* Message body not supported here currently */
    aws_http_message_set_body_stream(request_http_message, NULL);

    *message_out = request_http_message;

done:
    if (ret_value < 0) {
        aws_http_message_release(request_http_message);
    }

    aws_byte_buf_clean_up(&path_buf);

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5FD__s3comms_format_http_request_message() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_format_host_header
 *
 * Purpose:     Helper function to format the "Host: " header for a request
 *              and add it to the specified HTTP message structure.
 *
 * Return:      Non-negative on success/Negative on failure
 *----------------------------------------------------------------------------
 */
static herr_t
H5FD__s3comms_format_host_header(const H5FD__s3comms_aws_params_t *aws_params,
                                 struct aws_http_message *message, const char *bucket_name,
                                 const char *host_name, const char *port)
{
    struct aws_http_header host_header       = {0};
    struct aws_byte_cursor host_cursor       = {0};
    struct aws_byte_buf    host_buf          = {0};
    bool                   use_virtual_style = true; /* Use virtual-hosted-style request by default */
    herr_t                 ret_value         = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(aws_params);
    assert(message);
    assert(bucket_name);
    assert(host_name);

    if (aws_params->force_path_style)
        use_virtual_style = false;

    /*
     * If an alternate endpoint URL was specified, use a path-style
     * request for now. This isn't the most ideal, but is currently
     * required for testing.
     */
    if (aws_params->alt_parsed_uri.self_size != 0)
        use_virtual_style = false;

    /*
     * If bucket has a "." in the name, use a path-style request
     * as virtual-hosted-style requests don't directly support buckets
     * with "." when using HTTPS.
     */
    if (strstr(bucket_name, "."))
        use_virtual_style = false;

    H5_WARN_AGGREGATE_RETURN_OFF
    host_cursor = aws_byte_cursor_from_c_str(host_name);
    H5_WARN_AGGREGATE_RETURN_ON

    if (use_virtual_style) {
        struct aws_byte_cursor bucket_cursor;

        H5_WARN_AGGREGATE_RETURN_OFF
        bucket_cursor = aws_byte_cursor_from_c_str(bucket_name);
        H5_WARN_AGGREGATE_RETURN_ON

        AWS_ZERO_STRUCT(host_buf);

        /*
         * Check to see if the original host name already has the bucket
         * name included (i.e., a virtual-hosted style URL), in which
         * case we can just use it directly. Otherwise, form the host
         * header by prepending the bucket name to the host name.
         */
        if (aws_byte_cursor_starts_with_ignore_case(&host_cursor, &bucket_cursor)) {
            H5_WARN_AGGREGATE_RETURN_OFF
            host_header.value = host_cursor;
            H5_WARN_AGGREGATE_RETURN_ON
        }
        else {
            struct aws_byte_cursor period_cursor;

            H5_WARN_AGGREGATE_RETURN_OFF
            period_cursor = aws_byte_cursor_from_c_str(".");
            H5_WARN_AGGREGATE_RETURN_ON

            if (AWS_OP_SUCCESS !=
                aws_byte_buf_init_copy_from_cursor(&host_buf, aws_params->allocator, bucket_cursor))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL,
                            "couldn't allocate memory for 'Host' header string: %s",
                            aws_error_str(aws_last_error()));

            if (AWS_OP_SUCCESS != aws_byte_buf_append_dynamic(&host_buf, &period_cursor))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL,
                            "couldn't allocate memory for 'Host' header string: %s",
                            aws_error_str(aws_last_error()));

            if (AWS_OP_SUCCESS != aws_byte_buf_append_dynamic(&host_buf, &host_cursor))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL,
                            "couldn't allocate memory for 'Host' header string: %s",
                            aws_error_str(aws_last_error()));

            H5_WARN_AGGREGATE_RETURN_OFF
            host_header.value = aws_byte_cursor_from_buf(&host_buf);
            H5_WARN_AGGREGATE_RETURN_ON
        }
    }
    else {
        /* If port was specified, include in Host header instead of using default */
        if (port) {
            struct aws_byte_cursor port_cursor;
            struct aws_byte_cursor colon_cursor;

            H5_WARN_AGGREGATE_RETURN_OFF
            port_cursor  = aws_byte_cursor_from_c_str(port);
            colon_cursor = aws_byte_cursor_from_c_str(":");
            H5_WARN_AGGREGATE_RETURN_ON

            AWS_ZERO_STRUCT(host_buf);

            if (AWS_OP_SUCCESS !=
                aws_byte_buf_init_copy_from_cursor(&host_buf, aws_params->allocator, host_cursor))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL,
                            "couldn't allocate memory for 'Host' header string: %s",
                            aws_error_str(aws_last_error()));

            if (AWS_OP_SUCCESS != aws_byte_buf_append_dynamic(&host_buf, &colon_cursor))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL,
                            "couldn't allocate memory for 'Host' header string: %s",
                            aws_error_str(aws_last_error()));

            if (AWS_OP_SUCCESS != aws_byte_buf_append_dynamic(&host_buf, &port_cursor))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL,
                            "couldn't allocate memory for 'Host' header string: %s",
                            aws_error_str(aws_last_error()));

            H5_WARN_AGGREGATE_RETURN_OFF
            host_header.value = aws_byte_cursor_from_buf(&host_buf);
            H5_WARN_AGGREGATE_RETURN_ON
        }
        else {
            host_header.value = host_cursor;
        }
    }

    H5_WARN_AGGREGATE_RETURN_OFF
    host_header.name = aws_byte_cursor_from_c_str("Host");
    H5_WARN_AGGREGATE_RETURN_ON

    if (AWS_OP_SUCCESS != aws_http_message_add_header(message, host_header))
        HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "couldn't add 'Host' header to HTTP message: %s",
                    aws_error_str(aws_last_error()));

done:
    aws_byte_buf_clean_up(&host_buf);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__s3comms_format_host_header() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_format_range_header
 *
 * Purpose:     Helper function to format the "Range: " header for a request
 *              and add it to the specified HTTP message structure. No action
 *              is performed if both `offset` and `len` are 0, as this implies
 *              reading of the whole file.
 *
 * Return:      Non-negative on success/Negative on failure
 *----------------------------------------------------------------------------
 */
static herr_t
H5FD__s3comms_format_range_header(struct aws_http_message *message, haddr_t offset, size_t len)
{
    struct aws_http_header range_header = {0};
    int                    ret          = 0;
    char                   byte_range_str[S3COMMS_MAX_RANGE_STRING_SIZE];
    herr_t                 ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(message);

    if (offset == 0 && len == 0)
        HGOTO_DONE(SUCCEED);

    if (len > 0) {
        ret = snprintf(byte_range_str, sizeof(byte_range_str), "bytes=%" PRIuHADDR "-%" PRIuHADDR, offset,
                       offset + len - 1);
    }
    else if (offset > 0) {
        ret = snprintf(byte_range_str, sizeof(byte_range_str), "bytes=%" PRIuHADDR "-", offset);
    }

    if (ret < 0)
        HGOTO_ERROR(H5E_VFL, H5E_SYSTEM, FAIL, "snprintf error");
    if ((size_t)ret >= sizeof(byte_range_str))
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "unable to format HTTP Range value");

    /* Add "Range: " header to HTTP message */
    H5_WARN_AGGREGATE_RETURN_OFF
    range_header.name  = aws_byte_cursor_from_c_str("Range");
    range_header.value = aws_byte_cursor_from_c_str(byte_range_str);
    H5_WARN_AGGREGATE_RETURN_ON

    if (AWS_OP_SUCCESS != aws_http_message_add_header(message, range_header))
        HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "couldn't add 'Range' header to HTTP message: %s",
                    aws_error_str(aws_last_error()));

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__s3comms_format_range_header() */

/*----------------------------------------------------------------------------
 * Function:    H5FD__s3comms_format_user_agent_header
 *
 * Purpose:     Helper function to format the "User-Agent: " header for a
 *              request and add it to the specified HTTP message structure.
 *
 * Return:      Non-negative on success/Negative on failure
 *----------------------------------------------------------------------------
 */
static herr_t
H5FD__s3comms_format_user_agent_header(struct aws_http_message *message)
{
    struct aws_http_header user_agent_header = {0};
    int                    ret               = 0;
    char                   user_agent_str[MAX_USER_AGENT_STRING_SIZE];
    herr_t                 ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(message);

    ret = snprintf(user_agent_str, sizeof(user_agent_str), "libhdf5/%u.%u.%u (vfd:ros3) libaws-c-s3",
                   H5_VERS_MAJOR, H5_VERS_MINOR, H5_VERS_RELEASE);

    if (ret < 0)
        HGOTO_ERROR(H5E_VFL, H5E_SYSTEM, FAIL, "snprintf error");
    if ((size_t)ret >= sizeof(user_agent_str))
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "unable to format HTTP User-Agent value");

    /* Add "User-Agent: " header to HTTP message */
    H5_WARN_AGGREGATE_RETURN_OFF
    user_agent_header.name  = aws_byte_cursor_from_c_str("User-Agent");
    user_agent_header.value = aws_byte_cursor_from_c_str(user_agent_str);
    H5_WARN_AGGREGATE_RETURN_ON

    if (AWS_OP_SUCCESS != aws_http_message_add_header(message, user_agent_header))
        HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "couldn't add 'User-Agent' header to HTTP message: %s",
                    aws_error_str(aws_last_error()));

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__s3comms_format_user_agent_header() */

/*
 * Maps HTTP status codes to generic strings for cases where
 * the aws-c-s3 library doesn't have a particular message
 * for the response
 */
static const char *
H5FD__s3comms_httpcode_to_str(long httpcode, bool *handled)
{
    *handled = true;

    switch (httpcode) {
        case 301:
            return "resource has been permanently moved";
            break;
        case 400:
            return "malformed/Bad request for resource; possible mismatch between specified AWS region and "
                   "region in URL (if any)";
            break;
        case 401:
            return "valid authentication needed to access resource";
            break;
        case 403:
            return "unauthorized access to resource";
            break;
        case 404:
            return "resource not found";
            break;
        case 405:
            return "method not allowed";
            break;
        case 408:
            return "request timed out";
            break;
        case 409:
            return "resource already exists";
            break;
        case 410:
            return "resource has been deleted";
            break;
        case 413:
            return "request for resource was too large";
            break;
        case 416:
            return "requested resource byte range was not satisfiable";
            break;
        case 429:
            return "too many requests";
            break;
        case 500:
            return "internal server error";
            break;
        case 501:
            return "request method not implemented";
            break;
        case 502:
            return "bad gateway";
            break;
        case 503:
            return "service unavailable";
            break;
        default:
            *handled = false;
            return "unknown/unhandled HTTP status code";
            break;
    }
}

#endif /* H5_HAVE_ROS3_VFD */
