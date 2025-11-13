/*********************************************************************
*    Copyright 2018, UCAR/Unidata
*    See netcdf/COPYRIGHT file for copying and redistribution conditions.
* ********************************************************************/

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

/*****************************************************************************
 * Read-Only S3 Virtual File Driver (VFD)
 *
 * This is the header for the S3 Communications module
 *
 * ***NOT A FILE DRIVER***
 *
 * Purpose:
 *
 *     - Provide structures and functions related to communicating with
 *       Amazon S3 (Simple Storage Service).
 *     - Abstract away the REST API (HTTP,
 *       networked communications) behind a series of uniform function calls.
 *     - Handle AWS4 authentication, if appropriate.
 *     - Fail predictably in event of errors.
 *     - Eventually, support more S3 operations, such as creating, writing to,
 *       and removing Objects remotely.
 *
 *     translates:
 *     `read(some_file, bytes_offset, bytes_length, &dest_buffer);`
 *     to:
 *     ```
 *     GET myfile HTTP/1.1
 *     Host: somewhere.me
 *     Range: bytes=4096-5115
 *     ```
 *     and places received bytes from HTTP response...
 *     ```
 *     HTTP/1.1 206 Partial-Content
 *     Content-Range: 4096-5115/63239
 *
 *     <bytes>
 *     ```
 *     ...in destination buffer.
 *
 * TODO: put documentation in a consistent place and point to it from here.
 *
 * Programmer: Jacob Smith
 *             2017-11-30
 *
 *****************************************************************************/

/**
 * Unidata Changes:
 * Derived from HDF5-1.14.0 H5FDs3comms.[ch]
 * Modified to be in netcdf-c style
 * Support Write operations and support NCZarr.
 * See ncs3comms.c for detailed list of changes.
 * Author: Dennis Heimbigner
 */
 
#ifndef NCS3COMMS_H
#define NCS3COMMS_H

/*****************/

/* Opaque Handles */
struct CURL;
struct NCURI;
struct VString;

/*****************
 * PUBLIC MACROS *
 *****************/

/* hexadecimal string of pre-computed sha256 checksum of the empty string
 * hex(sha256sum(""))
 */
#define EMPTY_SHA256 "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"

/* string length (plus null terminator)
 * example ISO8601-format string: "20170713T145903Z" (YYYYmmdd'T'HHMMSS'_')
 */
#define ISO8601_SIZE 17

/* string length (plus null terminator)
 * example RFC7231-format string: "Fri, 30 Jun 2017 20:41:55 GMT"
 */
#define RFC7231_SIZE 30

/*
 *String length (including nul term) for HTTP Verb
 */
#define S3COMMS_VERB_MAX 16

/*
 * Size of a SHA256 digest in bytes
 */
#ifndef SHA256_DIGEST_LENGTH
#define SHA256_DIGEST_LENGTH 32
#endif


/*---------------------------------------------------------------------------
 *
 * Macro: ISO8601NOW()
 *
 * Purpose:
 *
 *     write "YYYYmmdd'T'HHMMSS'Z'" (less single-quotes) to dest
 *     e.g., "20170630T204155Z"
 *
 *     wrapper for strftime()
 *
 *     It is left to the programmer to check return value of
 *     ISO8601NOW (should equal ISO8601_SIZE - 1).
 *
 *---------------------------------------------------------------------------
 */
#define ISO8601NOW(dest, now_gm) strftime((dest), ISO8601_SIZE, "%Y%m%dT%H%M%SZ", (now_gm))

/*---------------------------------------------------------------------------
 *
 * Macro: RFC7231NOW()
 *
 * Purpose:
 *
 *     write "Day, dd Mmm YYYY HH:MM:SS GMT" to dest
 *     e.g., "Fri, 30 Jun 2017 20:41:55 GMT"
 *
 *     wrapper for strftime()
 *
 *     It is left to the programmer to check return value of
 *     RFC7231NOW (should equal RFC7231_SIZE - 1).
 *
 *---------------------------------------------------------------------------
 */
#define RFC7231NOW(dest, now_gm) strftime((dest), RFC7231_SIZE, "%a, %d %b %Y %H:%M:%S GMT", (now_gm))

/* Reasonable maximum length of a credential string.
 * Provided for error-checking S3COMMS_FORMAT_CREDENTIAL (below).
 *  17 <- "////aws4_request\0"
 *   2 < "s3" (service)
 *   8 <- "YYYYmmdd" (date)
 * 128 <- (access_id)
 * 155 :: sum
 */
#define S3COMMS_MAX_CREDENTIAL_SIZE 155

/*---------------------------------------------------------------------------
 *
 * Macro: H5FD_S3COMMS_FORMAT_CREDENTIAL()
 *
 * Purpose:
 *
 *     Format "S3 Credential" string from inputs, for AWS4.
 *
 *     Wrapper for HDsnprintf().
 *
 *     _HAS NO ERROR-CHECKING FACILITIES_
 *     It is left to programmer to ensure that return value confers success.
 *     e.g.,
 *     ```
 *     assert( S3COMMS_MAX_CREDENTIAL_SIZE >=
 *             S3COMMS_FORMAT_CREDENTIAL(...) );
 *     ```
 *
 *     "<access-id>/<date>/<aws-region>/<aws-service>/aws4_request"
 *     assuming that `dest` has adequate space.
 *
 *     ALL inputs must be null-terminated strings.
 *
 *     `access` should be the user's access key ID.
 *     `date` must be of format "YYYYmmdd".
 *     `region` should be relevant AWS region, i.e. "us-east-1".
 *     `service` should be "s3".
 *
 *---------------------------------------------------------------------------
 */
#define S3COMMS_FORMAT_CREDENTIAL(dest, access, iso8601_date, region, service)	\
    vscat((dest),(access)); vscat((dest),"/");			\
    vscat((dest),(iso8601_date)); vscat((dest),"/");			\
    vscat((dest),(region)); vscat((dest),"/");			\
    vscat((dest),(service)); vscat((dest),"/");			\
    vscat((dest),"aws4_request");
			
#if 0
    snprintf((dest), S3COMMS_MAX_CREDENTIAL_SIZE, "%s/%s/%s/%s/aws4_request", (access), (iso8601_date),    \
               (region), (service))
#endif

/*********************
 * PUBLIC STRUCTURES *
 *********************/

/*----------------------------------------------------------------------------
 *
 * Structure: hrb_node_t
 *
 * HTTP Header Field Node
 *
 *
 *
 * Maintain a ordered (linked) list of HTTP Header fields.
 *
 * Provides efficient access and manipulation of a logical sequence of
 * HTTP header fields, of particular use when composing an
 * "S3 Canonical Request" for authentication.
 *
 * - The creation of a Canonical Request involves:
 *     - convert field names to lower case
 *     - sort by this lower-case name
 *     - convert ": " name-value separator in HTTP string to ":"
 *     - get sorted lowercase names without field or separator
 *
 * As HTTP headers allow headers in any order (excepting the case of multiple
 * headers with the same name), the list ordering can be optimized for Canonical
 * Request creation, suggesting alphabtical order. For more expedient insertion
 * and removal of elements in the list, linked list seems preferable to a
 * dynamically-expanding array. The usually-smaller number of entries (5 or
 * fewer) makes performance overhead of traversing the list trivial.
 *
 * The above requirements of creating at Canonical Request suggests a reasonable
 * trade-off of speed for space with the option to compute elements as needed
 * or to have the various elements prepared and stored in the structure
 * (e.g. name, value, lowername, concatenated name:value)
 * The structure currently is implemented to pre-compute.
 *
 * At all times, the "first" node of the list should be the least,
 * alphabetically. For all nodes, the `next` node should be either NULL or
 * of greater alphabetical value.
 *
 * Each node contains its own header field information, plus a pointer to the
 * next node.
 *
 * It is not allowed to have multiple nodes with the same _lowercase_ `name`s
 * in the same list
 * (i.e., name is case-insensitive for access and modification.)
 *
 * All data (`name`, `value`, `lowername`, and `cat`) are null-terminated
 * strings allocated specifically for their node.
 *
 *
 *
 * `magic` (unsigned long)
 *
 *     "unique" idenfier number for the structure type
 *
 * `name` (char *)
 *
 *     Case-meaningful name of the HTTP field.
 *     Given case is how it is supplied to networking code.
 *     e.g., "Range"
 *
 * `lowername` (char *)
 *
 *     Lowercase copy of name.
 *     e.g., "range"
 *
 * `value` (char *)
 *
 *     Case-meaningful value of HTTP field.
 *     e.g., "bytes=0-9"
 *
 * `cat` (char *)
 *
 *     Concatenated, null-terminated string of HTTP header line,
 *     as the field would appear in an HTTP request.
 *     e.g., "Range: bytes=0-9"
 *
 *----------------------------------------------------------------------------
 */
typedef struct hrb_node_t {
    unsigned long      magic;
    char              *name;
    char              *value;
    char              *cat;
    char              *lowername;
    struct hrb_node_t *next;
} hrb_node_t;
#define S3COMMS_HRB_NODE_MAGIC 0x7F5757UL

/*----------------------------------------------------------------------------
 *
 * Structure: hrb_t
 *
 * HTTP Request Buffer structure
 *
 *
 *
 * Logically represent an HTTP request
 *
 *     GET /myplace/myfile.h5 HTTP/1.1
 *     Host: over.rainbow.oz
 *     Date: Fri, 01 Dec 2017 12:35:04 CST
 *
 *     <body>
 *
 * ...with fast, efficient access to and modification of primary and field
 * elements.
 *
 * Structure for building HTTP requests while hiding much of the string
 * processing required "under the hood."
 *
 * Information about the request target -- the first line -- and the body text,
 * if any, are managed directly with this structure. All header fields, e.g.,
 * "Host" and "Date" above, are created with a linked list of `hrb_node_t` and
 * included in the request by a pointer to the head of the list.
 *
 *
 *
 * `magic` (unsigned long)
 *
 *     "Magic" number confirming that this is an hrb_t structure and
 *     what operations are valid for it.
 *
 *     Must be S3COMMS_HRB_MAGIC to be valid.
 *
 * `body` (char *) :
 *
 *     Pointer to start of HTTP body.
 *
 *     Can be NULL, in which case it is treated as the empty string, "".
 *
 * `body_len` (size_t) :
 *
 *     Number of bytes (characters) in `body`. 0 if empty or NULL `body`.
 *
 * `first_header` (hrb_node_t *) :
 *
 *     Pointer to first SORTED header node, if any.
 *     It is left to the programmer to ensure that this node and associated
 *     list is destroyed when done.
 *
 * `resource` (char *) :
 *
 *     Pointer to resource URL string, e.g., "/folder/page.xhtml".
 *
 * `verb` (char *) :
 *
 *     Pointer to HTTP verb string, e.g., "GET".
 *
 * `version` (char *) :
 *
 *     Pointer to HTTP version string, e.g., "HTTP/1.1".
 *
 *----------------------------------------------------------------------------
 */
typedef struct {
    unsigned long  magic;
    struct VString *body;
    struct VList   *headers;
    char           *resource;
    char           *version;
} hrb_t;
#define S3COMMS_HRB_MAGIC 0x6DCC84UL

/*----------------------------------------------------------------------------
 * Structure: s3r_byterange
 * HTTP Request byterange info
 *
 * `magic` (unsigned long)
 *
 *     "Magic" number confirming that this is an s3r_byterange structure and
 *     what operations are valid for it.
 *
 *     Must be S3COMMS_BYTERANGE_MAGIC to be valid.
 *
 * `offset` (size_t) :
 *     Read bytes starting at position `offset`
 *
 * `len` (size_t) :
 *     Read `len` bytes
 *----------------------------------------------------------------------------
 */
typedef struct {
    unsigned long magic;
    size_t        offset;
    size_t        len;
} s3r_byterange;
#define S3COMMS_BYTERANGE_MAGIC 0x41fab3UL

/*----------------------------------------------------------------------------
 *
 * Structure: s3r_t
 *
 *
 *
 * S3 request structure "handle".
 *
 * Holds persistent information for Amazon S3 requests.
 *
 * Instantiated through `NCH5_s3comms_s3r_open()`, copies data into self.
 *
 * Intended to be re-used for operations on a remote object.
 *
 * Cleaned up through `NCH5_s3comms_s3r_close()`.
 *
 * _DO NOT_ share handle between threads: curl easy handle `curlhandle` has
 * undefined behavior if called to perform in multiple threads.
 *
 *
 *
 * `magic` (unsigned long)
 *
 *     "magic" number identifying this structure as unique type.
 *     MUST equal `S3R_MAGIC` to be valid.
 *
 * `curlhandle` (CURL)
 *
 *     Pointer to the curl_easy handle generated for the request.
 *
 * `httpverb` (char *)
 *
 *     Pointer to NULL-terminated string. HTTP verb,
 *     e.g. "GET", "HEAD", "PUT", etc.
 *
 *     Default is NULL, resulting in a "GET" request.
 *
 * `purl` (NCuri*) see ncuri.h
 *     Cannot be NULL.
 *
 * `region` (char *)
 *
 *     Pointer to NULL-terminated string, specifying S3 "region",
 *     e.g., "us-east-1".
 *
 *     Required to authenticate.
 *
 * `secret_id` (char *)
 *
 *     Pointer to NULL-terminated string for "secret" access id to S3 resource.
 *
 *     Required to authenticate.
 *
 * `signing_key` (unsigned char *)
 *
 *     Pointer to `SHA256_DIGEST_LENGTH`-long string for "re-usable" signing
 *     key, generated via
 *     `HMAC-SHA256(HMAC-SHA256(HMAC-SHA256(HMAC-SHA256("AWS4<secret_key>",
 *         "<yyyyMMDD"), "<aws-region>"), "<aws-service>"), "aws4_request")`
 *     which may be re-used for several (up to seven (7)) days from creation?
 *     Computed once upon file open.
 *
 *     Required to authenticate.
 *
 *----------------------------------------------------------------------------
 */
typedef struct {
    unsigned long  magic;
    struct CURL   *curlhandle;
    char          *rootpath; /* All keys are WRT this path */
    char          *region;
    char          *accessid;
    char          *accesskey;
    char           httpverb[S3COMMS_VERB_MAX];
    unsigned char *signing_key; /*|signing_key| = SHA256_DIGEST_LENGTH*/
    char          iso8601now[ISO8601_SIZE];
    char         *reply;
    struct curl_slist *curlheaders;
} s3r_t;

/* Combined storage for space + size */
typedef struct s3r_buf_t {
    unsigned long long count; /* |content| */
    void* content;
} s3r_buf_t;


#define S3COMMS_S3R_MAGIC 0x44d8d79

typedef enum HTTPVerb {
HTTPNONE=0, HTTPGET=1, HTTPPUT=2, HTTPPOST=3, HTTPHEAD=4, HTTPDELETE=5
} HTTPVerb;

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************
 * DECLARATION OF HTTP FIELD LIST ROUTINES *
 *******************************************/

EXTERNL int NCH5_s3comms_hrb_node_set(hrb_node_t **L, const char *name, const char *value);

/***********************************************
 * DECLARATION OF HTTP REQUEST BUFFER ROUTINES *
 ***********************************************/

EXTERNL int NCH5_s3comms_hrb_destroy(hrb_t *buf);

EXTERNL hrb_t *NCH5_s3comms_hrb_init_request(const char *resource, const char *host);

/*************************************
 * DECLARATION OF S3REQUEST ROUTINES *
 *************************************/

EXTERNL s3r_t *NCH5_s3comms_s3r_open(const char* root, NCS3SVC svc, const char* region, const char* id, const char* access_key);

EXTERNL int NCH5_s3comms_s3r_close(s3r_t *handle);

EXTERNL int NCH5_s3comms_s3r_read(s3r_t *handle, const char* url, size_t offset, size_t len, s3r_buf_t* data);

EXTERNL int NCH5_s3comms_s3r_write(s3r_t *handle, const char* url, const s3r_buf_t* data);

EXTERNL int NCH5_s3comms_s3r_getkeys(s3r_t *handle, const char* url, s3r_buf_t* response);

EXTERNL int NCH5_s3comms_s3r_getsize(s3r_t *handle, const char* url, long long * sizep);

EXTERNL int NCH5_s3comms_s3r_deletekey(s3r_t *handle, const char* url, long* httpcodep);

EXTERNL int NCH5_s3comms_s3r_head(s3r_t *handle, const char* url, const char* header, const char* query, long* httpcodep, char** valuep);

/*********************************
 * DECLARATION OF OTHER ROUTINES *
 *********************************/

EXTERNL struct tm *gmnow(void);

EXTERNL int NCH5_s3comms_aws_canonical_request(struct VString* canonical_request_dest,
                                                 struct VString* signed_headers_dest,
						 HTTPVerb verb,
						 const char* query,
 						 const char* payloadsha256,
						 hrb_t *http_request);

EXTERNL int NCH5_s3comms_bytes_to_hex(char *dest, const unsigned char *msg, size_t msg_len,
                                        int lowercase);

EXTERNL int NCH5_s3comms_HMAC_SHA256(const unsigned char *key, size_t key_len, const char *msg,
                                       size_t msg_len, char *dest);

EXTERNL int NCH5_s3comms_load_aws_profile(const char *name, char *key_id_out, char *secret_access_key_out,
                                            char *aws_region_out);

EXTERNL int NCH5_s3comms_nlowercase(char *dest, const char *s, size_t len);

EXTERNL int NCH5_s3comms_percent_encode_char(char *repr, const unsigned char c, size_t *repr_len);

EXTERNL int NCH5_s3comms_signing_key(unsigned char **mdp, const char *secret, const char *region,
                                       const char *iso8601now);

EXTERNL int NCH5_s3comms_tostringtosign(struct VString* dest, const char *req_str, const char *now,
                                          const char *region);

EXTERNL int NCH5_s3comms_trim(char *dest, char *s, size_t s_len, size_t *n_written);

EXTERNL int NCH5_s3comms_uriencode(char** destp, const char *s, size_t s_len, int encode_slash, size_t *n_written);

#ifdef __cplusplus
}
#endif

#endif /*NCS3COMMS_H*/
